/*
 * Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ohos_external_texture_gl.h"

#include <GLES2/gl2ext.h>
#include <sys/mman.h>
#include <utility>

#include "ohos_main.h"
#include "third_party/skia/include/core/SkAlphaType.h"
#include "third_party/skia/include/core/SkColorSpace.h"
#include "third_party/skia/include/core/SkColorType.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/gpu/GrDirectContext.h"
#include "third_party/skia/include/gpu/ganesh/SkImageGanesh.h"
#include "third_party/skia/include/gpu/ganesh/gl/GrGLBackendSurface.h"

#define EGL_PLATFORM_OHOS_KHR 0x34E0

const int PIXEL_SIZE = 4;  // 像素点占用4个字节

namespace flutter {
using GetPlatformDisplayExt = PFNEGLGETPLATFORMDISPLAYEXTPROC;
constexpr char CHARACTER_WHITESPACE = ' ';
constexpr const char* CHARACTER_STRING_WHITESPACE = " ";
constexpr const char* EGL_EXT_PLATFORM_WAYLAND = "EGL_EXT_platform_wayland";
constexpr const char* EGL_KHR_PLATFORM_WAYLAND = "EGL_KHR_platform_wayland";
constexpr const char* EGL_GET_PLATFORM_DISPLAY_EXT = "eglGetPlatformDisplayEXT";

OHOSExternalTextureGL::OHOSExternalTextureGL(
    int64_t id,
    const std::shared_ptr<OHOSSurface>& ohos_surface)
    : Texture(id),
      ohos_surface_(std::move(ohos_surface)),
      transform_(SkMatrix::I()) {
  nativeImage_ = nullptr;
  nativeWindow_ = nullptr;
  eglContext_ = EGL_NO_CONTEXT;
  eglDisplay_ = EGL_NO_DISPLAY;
  buffer_ = nullptr;
  pixelMap_ = nullptr;
  lastImage_ = nullptr;
  isEmulator_ = OhosMain::IsEmulator();
}

OHOSExternalTextureGL::~OHOSExternalTextureGL() {
  if (state_ == AttachmentState::attached) {
    glDeleteTextures(1, &texture_name_);
  }
}

void OHOSExternalTextureGL::Attach() {
  OHOSSurface* ohos_surface_ptr = ohos_surface_.get();
  OhosSurfaceGLSkia* ohosSurfaceGLSkia_ = (OhosSurfaceGLSkia*)ohos_surface_ptr;
  auto result = ohosSurfaceGLSkia_->GLContextMakeCurrent();
  if (result->GetResult()) {
    FML_DLOG(INFO) << "ResourceContextMakeCurrent successed";
    glGenTextures(1, &texture_name_);
    FML_DLOG(INFO)
        << "OHOSExternalTextureGL::Paint, glGenTextures texture_name_="
        << texture_name_;
    if (nativeImage_ == nullptr) {
      nativeImage_ =
          OH_NativeImage_Create(texture_name_, GL_TEXTURE_EXTERNAL_OES);
      if (nativeImage_ == nullptr) {
        FML_DLOG(ERROR) << "Error with OH_NativeImage_Create";
        return;
      }
      nativeWindow_ = OH_NativeImage_AcquireNativeWindow(nativeImage_);
      if (nativeWindow_ == nullptr) {
        FML_DLOG(ERROR) << "Error with OH_NativeImage_AcquireNativeWindow";
        return;
      }
    }

    int32_t ret = OH_NativeImage_AttachContext(nativeImage_, texture_name_);
    if (ret != 0) {
      FML_DLOG(FATAL)
          << "OHOSExternalTextureGL OH_NativeImage_AttachContext err code:"
          << ret;
    }
    state_ = AttachmentState::attached;
  } else {
    FML_DLOG(FATAL) << "ResourceContextMakeCurrent failed";
  }
}

void OHOSExternalTextureGL::Paint(PaintContext& context,
                                  const SkRect& bounds,
                                  bool freeze,
                                  DlImageSampling sampling) {
  if (state_ == AttachmentState::detached) {
    FML_DLOG(ERROR) << "OHOSExternalTextureGL::Paint";
    return;
  }
  if (state_ == AttachmentState::uninitialized) {
    Attach();
  }

  if (!freeze) {
    if (!first_update_) {
      setBackground(bounds.width(), bounds.height());
      Update();
      first_update_ = true;
    } else if (new_frame_ready_ && pixelMap_ != nullptr) {
      ProducePixelMapToNativeImage();
      Update();
      new_frame_ready_ = false;
    }

  }
  FML_CHECK(state_ == AttachmentState::attached);

  GrGLTextureInfo textureInfo = {GL_TEXTURE_EXTERNAL_OES, texture_name_,
                                 GL_RGBA8_OES};
  auto backendTexture =
      GrBackendTextures::MakeGL(1, 1, skgpu::Mipmapped::kNo, textureInfo);
  sk_sp<SkImage> image = SkImages::BorrowTextureFrom(
      context.gr_context, backendTexture, kTopLeft_GrSurfaceOrigin,
      kRGBA_8888_SkColorType, kPremul_SkAlphaType, nullptr);
  sk_sp<flutter::DlImage> dl_image_ = DlImage::Make(image);

  if (dl_image_) {
    DlAutoCanvasRestore autoRestore(context.canvas, true);

    // The incoming texture is vertically flipped, so we flip it
    // back. OpenGL's coordinate system has Positive Y equivalent to up, while
    // Skia's coordinate system has Negative Y equvalent to up.
    // 模拟器和真机在外接纹理功能的表现不一致，需要进行适配
    if (isEmulator_) {
      context.canvas->Translate(bounds.x(), bounds.y());
      context.canvas->Scale(bounds.width(), bounds.height());
    } else {
      context.canvas->Translate(bounds.x(), bounds.y() + bounds.height());
      context.canvas->Scale(bounds.width(), -bounds.height());
    }

    if (!transform_.isIdentity()) {
      DlImageColorSource source(dl_image_, DlTileMode::kClamp,
                                DlTileMode::kClamp, sampling, &transform_);

      DlPaint paintWithShader;
      if (context.paint) {
        paintWithShader = *context.paint;
      }
      paintWithShader.setColorSource(&source);
      context.canvas->DrawRect(SkRect::MakeWH(1, 1), paintWithShader);
    } else {
      context.canvas->DrawImage(dl_image_, {0, 0}, sampling, context.paint);
    }
  }
}

void OHOSExternalTextureGL::OnGrContextCreated() {
  FML_DLOG(INFO) << " OHOSExternalTextureGL::OnGrContextCreated";
  state_ = AttachmentState::uninitialized;
}

void OHOSExternalTextureGL::OnGrContextDestroyed() {
  FML_DLOG(INFO) << " OHOSExternalTextureGL::OnGrContextDestroyed";
  if (state_ == AttachmentState::attached && pixelMap_ == nullptr) {
    Detach();
    glDeleteTextures(1, &texture_name_);
  }
  state_ = AttachmentState::detached;
}

void OHOSExternalTextureGL::MarkNewFrameAvailable() {
  FML_DLOG(INFO) << " OHOSExternalTextureGL::MarkNewFrameAvailable";
  new_frame_ready_ = true;
  first_update_ = true;
  Update();
}

void OHOSExternalTextureGL::OnTextureUnregistered() {
  FML_DLOG(INFO) << " OHOSExternalTextureGL::OnTextureUnregistered";
  OH_NativeImage_UnsetOnFrameAvailableListener(nativeImage_);
  OH_NativeImage_Destroy(&nativeImage_);
}

void OHOSExternalTextureGL::Update() {
  int32_t ret = OH_NativeImage_UpdateSurfaceImage(nativeImage_);
  if (ret != 0) {
    FML_DLOG(FATAL)
        << "OHOSExternalTextureGL OH_NativeImage_UpdateSurfaceImage err code:"
        << ret;
  }
  UpdateTransform();
}

void OHOSExternalTextureGL::Detach() {
  OH_NativeImage_DetachContext(nativeImage_);
  OH_NativeWindow_DestroyNativeWindow(nativeWindow_);
}

void OHOSExternalTextureGL::UpdateTransform()
{
  float m[16] = { 0.0f };
  int32_t ret = OH_NativeImage_GetTransformMatrixV2(nativeImage_, m);
  if (ret != 0) {
    FML_DLOG(FATAL)<<"OHOSExternalTextureGL OH_NativeImage_GetTransformMatrixV2 err code:"<< ret;
  }
  // transform_ ohos 4x4 matrix to skia 3x3 matrix
  SkScalar matrix3[] = {
      m[0], m[4], m[12],  //
      m[1], m[5], m[13],  //
      m[3], m[7], m[15],  //
  };
  transform_.set9(matrix3);
  SkMatrix inverted;
  if (!transform_.invert(&inverted)) {
    FML_LOG(FATAL) << "OHOSExternalTextureGL Invalid SurfaceTexture "
                      "transform_ation matrix";
  }
  transform_ = inverted;
}

void OHOSExternalTextureGL::DispatchImage(ImageNative* image) {
  lastImage_ = image;
}

void OHOSExternalTextureGL::setBackground(int32_t width, int32_t height)
{
  if (nativeWindow_ == nullptr) {
    nativeWindow_ = OH_NativeImage_AcquireNativeWindow(nativeImage_);
    if (nativeWindow_ == nullptr) {
      FML_DLOG(ERROR) << "OHOSExternalTextureGL  in setBackground Error with OH_NativeImage_AcquireNativeWindow";
      return;
    }
  }

  int code = SET_BUFFER_GEOMETRY;
  int32_t ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_, code, width, height);
  if (ret != 0) {
    FML_DLOG(ERROR) << "OHOSExternalTextureGL in setBackground OH_NativeWindow_NativeWindowHandleOpt err:" << ret;
  }

  ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow_, &buffer_, &fenceFd);
  if (ret != 0) {
    FML_DLOG(ERROR) << "OHOSExternalTextureGL in setBackground OH_NativeWindow_NativeWindowRequestBuffer err:" << ret;
  }

  BufferHandle *handle = OH_NativeWindow_GetBufferHandleFromNative(buffer_);
  void *mappedAddr = mmap(handle->virAddr, handle->size, PROT_READ | PROT_WRITE, MAP_SHARED, handle->fd, 0);
  if (mappedAddr == MAP_FAILED) {
    FML_DLOG(FATAL)<<"OHOSExternalTextureGL in setBackground mmap failed";
    return;
  }

  uint32_t* destAddr = static_cast<uint32_t *>(mappedAddr);
  uint32_t value = 0xFFFFFFFF;

  for(int32_t x = 0; x < handle->width; x++) {
    for (int32_t y = 0; y < handle->height; y++) {
      *destAddr++ = value;
    }
  }

    // munmap after use
  ret = munmap(mappedAddr, handle->size);
  if (ret == -1) {
    FML_DLOG(FATAL)<<"OHOSExternalTextureGL in setBackground munmap failed";
    return;
  }

  Region region{nullptr, 0};
  ret = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow_, buffer_, fenceFd, region);
  if (ret != 0) {
    FML_DLOG(FATAL)<<"OHOSExternalTextureGL in setBackground OH_NativeWindow_NativeWindowFlushBuffer err:"<< ret;
  }
}

void OHOSExternalTextureGL::HandlePixelMapBuffer() {
  BufferHandle* handle = OH_NativeWindow_GetBufferHandleFromNative(buffer_);
  // get virAddr of bufferHandl by mmap sys interface
  uint32_t stride = handle->stride;
  FML_DLOG(INFO) << "OHOSExternalTextureGL stride:" << stride;
  void* mappedAddr = mmap(handle->virAddr, handle->size, PROT_READ | PROT_WRITE,
                          MAP_SHARED, handle->fd, 0);
  if (mappedAddr == MAP_FAILED) {
    FML_DLOG(FATAL) << "OHOSExternalTextureGL mmap failed";
    return;
  }

  void* pixelAddr = nullptr;
  int64_t ret = OH_PixelMap_AccessPixels(pixelMap_, &pixelAddr);
  if (ret != IMAGE_RESULT_SUCCESS) {
    FML_DLOG(FATAL) << "OHOSExternalTextureGL OH_PixelMap_AccessPixels err:"
                    << ret;
    return;
  }

  uint32_t* pixel = static_cast<uint32_t*>(pixelAddr);
  uint32_t* destAddr = static_cast<uint32_t*>(mappedAddr);

  FML_DLOG(INFO) << "OHOSExternalTextureGL pixelMapInfo w:"
                 << pixelMapInfo.width << " h:" << pixelMapInfo.height;
  FML_DLOG(INFO) << "OHOSExternalTextureGL pixelMapInfo rowSize:"
                 << pixelMapInfo.rowSize
                 << " format:" << pixelMapInfo.pixelFormat;

  // 复制图片纹理数据到内存中，需要处理DMA内存补齐相关的逻辑
  if (pixelMapInfo.width * PIXEL_SIZE != pixelMapInfo.rowSize) {
    // 直接复制整块内存
    memcpy(destAddr, pixel, pixelMapInfo.height * pixelMapInfo.rowSize);
  } else {
    // 需要处理DMA内存补齐相关的逻辑
    for (uint32_t i = 0; i < pixelMapInfo.height; i++) {
      memcpy(destAddr, pixel, pixelMapInfo.rowSize);
      destAddr += stride / PIXEL_SIZE;
      pixel += pixelMapInfo.width;
    }
  }
  OH_PixelMap_UnAccessPixels(pixelMap_);
  // munmap after use
  ret = munmap(mappedAddr, handle->size);
  if (ret == -1) {
    FML_DLOG(FATAL) << "OHOSExternalTextureGL munmap failed";
    return;
  }
}

void OHOSExternalTextureGL::ProducePixelMapToNativeImage() {
  if (state_ == AttachmentState::detached) {
    FML_DLOG(ERROR) << "OHOSExternalTextureGL AttachmentState err";
    return;
  }
  int32_t ret = -1;
  ret = OH_PixelMap_GetImageInfo(pixelMap_, &pixelMapInfo);
  if (ret != 0) {
    FML_DLOG(ERROR) << "OHOSExternalTextureGL OH_PixelMap_GetImageInfo err:"
                    << ret;
  }

  int code = SET_BUFFER_GEOMETRY;
  ret = OH_NativeWindow_NativeWindowHandleOpt(
      nativeWindow_, code, pixelMapInfo.width, pixelMapInfo.height);
  if (ret != 0) {
    FML_DLOG(ERROR)
        << "OHOSExternalTextureGL OH_NativeWindow_NativeWindowHandleOpt err:"
        << ret;
  }

  if (buffer_ != nullptr) {
    OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow_, buffer_);
    buffer_ = nullptr;
  }
  ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow_, &buffer_,
                                                  &fenceFd);
  if (ret != 0) {
    FML_DLOG(ERROR) << "OHOSExternalTextureGL "
                       "OH_NativeWindow_NativeWindowRequestBuffer err:"
                    << ret;
  }
  HandlePixelMapBuffer();
  Region region{nullptr, 0};
  ret = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow_, buffer_, fenceFd,
                                                region);
  if (ret != 0) {
    FML_DLOG(FATAL)
        << "OHOSExternalTextureGL OH_NativeWindow_NativeWindowFlushBuffer err:"
        << ret;
  }
}

EGLDisplay OHOSExternalTextureGL::GetPlatformEglDisplay(
    EGLenum platform,
    void* native_display,
    const EGLint* attrib_list) {
  GetPlatformDisplayExt eglGetPlatformDisplayExt = NULL;

  if (!eglGetPlatformDisplayExt) {
    const char* extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    if (extensions &&
        (CheckEglExtension(extensions, EGL_EXT_PLATFORM_WAYLAND) ||
         CheckEglExtension(extensions, EGL_KHR_PLATFORM_WAYLAND))) {
      eglGetPlatformDisplayExt = (GetPlatformDisplayExt)eglGetProcAddress(
          EGL_GET_PLATFORM_DISPLAY_EXT);
    }
  }

  if (eglGetPlatformDisplayExt) {
    return eglGetPlatformDisplayExt(platform, native_display, attrib_list);
  }

  return eglGetDisplay((EGLNativeDisplayType)native_display);
}

bool OHOSExternalTextureGL::CheckEglExtension(const char* extensions,
                                              const char* extension) {
  size_t extlen = strlen(extension);
  const char* end = extensions + strlen(extensions);
  while (extensions < end) {
    size_t n = 0;
    if (*extensions == CHARACTER_WHITESPACE) {
      extensions++;
      continue;
    }
    n = strcspn(extensions, CHARACTER_STRING_WHITESPACE);
    if (n == extlen && strncmp(extension, extensions, n) == 0) {
      return true;
    }
    extensions += n;
  }
  return false;
}

void OHOSExternalTextureGL::DispatchPixelMap(NativePixelMap* pixelMap) {
  if (pixelMap != nullptr) {
    pixelMap_ = pixelMap;
  }
}

}  // namespace flutter