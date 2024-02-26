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

#include "third_party/skia/include/core/SkAlphaType.h"
#include "third_party/skia/include/core/SkColorSpace.h"
#include "third_party/skia/include/core/SkColorType.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/gpu/GrBackendSurface.h"
#include "third_party/skia/include/gpu/GrDirectContext.h"

#define EGL_PLATFORM_OHOS_KHR             0x34E0

const int DMA_SIZE = 256 // DMA内存分配的分块大小
const int PIXEL_SIZE = 4 // 像素点占用4个字节

namespace flutter {
using GetPlatformDisplayExt = PFNEGLGETPLATFORMDISPLAYEXTPROC;
constexpr char CHARACTER_WHITESPACE = ' ';
constexpr const char *CHARACTER_STRING_WHITESPACE = " ";
constexpr const char *EGL_EXT_PLATFORM_WAYLAND = "EGL_EXT_platform_wayland";
constexpr const char *EGL_KHR_PLATFORM_WAYLAND = "EGL_KHR_platform_wayland";
constexpr const char *EGL_GET_PLATFORM_DISPLAY_EXT = "eglGetPlatformDisplayEXT";

OHOSExternalTextureGL::OHOSExternalTextureGL(int64_t id, const std::shared_ptr<OHOSSurface>& ohos_surface)
  : Texture(id), ohos_surface_(std::move(ohos_surface)), transform(SkMatrix::I())
{
    nativeImage_ = nullptr;
    nativeWindow_ = nullptr;
    eglContext_ =  EGL_NO_CONTEXT;
    eglDisplay_ = EGL_NO_DISPLAY;
    buffer_ = nullptr;
    pixelMap_ = nullptr;
    lastImage_ = nullptr;
}

OHOSExternalTextureGL::~OHOSExternalTextureGL()
{
  if (state_ == AttachmentState::attached) {
    glDeleteTextures(1, &texture_name_);
  }
}

void OHOSExternalTextureGL::Attach()
{
  OHOSSurface* ohos_surface_ptr = ohos_surface_.get();
  OhosSurfaceGLSkia* ohosSurfaceGLSkia_ = (OhosSurfaceGLSkia*)ohos_surface_ptr;
  auto result = ohosSurfaceGLSkia_->GLContextMakeCurrent();
  if (result->GetResult()) {
    FML_DLOG(INFO)<<"ResourceContextMakeCurrent successed";
    glGenTextures(1, &texture_name_);
    FML_DLOG(INFO) << "OHOSExternalTextureGL::Paint, glGenTextures texture_name_=" << texture_name_;
    if (nativeImage_ == nullptr) {
      nativeImage_ = OH_NativeImage_Create(texture_name_, GL_TEXTURE_EXTERNAL_OES);
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
      FML_DLOG(FATAL)<<"OHOSExternalTextureGL OH_NativeImage_AttachContext err code:"<< ret;
    }
    state_ = AttachmentState::attached;
  } else {
    FML_DLOG(FATAL)<<"ResourceContextMakeCurrent failed";
  }
}

void OHOSExternalTextureGL::Paint(PaintContext& context,
                                  const SkRect& bounds,
                                  bool freeze,
                                  const SkSamplingOptions& sampling)
{
  if (state_ == AttachmentState::detached) {
    FML_DLOG(ERROR) << "OHOSExternalTextureGL::Paint";
    return;
  }
  if (state_ == AttachmentState::uninitialized) {
    Attach();
  }
  if (!freeze && new_frame_ready_) {
    Update();
    new_frame_ready_ = false;
  }

  GrGLTextureInfo textureInfo = {
    GL_TEXTURE_EXTERNAL_OES, texture_name_, GL_RGBA8_OES};
  GrBackendTexture backendTexture(1, 1, GrMipMapped::kNo, textureInfo);
  sk_sp<SkImage> image = SkImage::MakeFromTexture(
      context.gr_context, backendTexture, kTopLeft_GrSurfaceOrigin,
      kRGBA_8888_SkColorType, kPremul_SkAlphaType, nullptr);
  if (image) {
    SkAutoCanvasRestore autoRestore(context.canvas, true);

    // The incoming texture is vertically flipped, so we flip it
    // back. OpenGL's coordinate system has Positive Y equivalent to up, while
    // Skia's coordinate system has Negative Y equvalent to up.
    context.canvas->translate(bounds.x(), bounds.y());
    context.canvas->scale(bounds.width(), bounds.height());

    if (!transform.isIdentity()) {
      sk_sp<SkShader> shader = image->makeShader(
          SkTileMode::kRepeat, SkTileMode::kRepeat, sampling, transform);

      SkPaint paintWithShader;
      if (context.sk_paint) {
        paintWithShader = *context.sk_paint;
      }
      paintWithShader.setShader(shader);
      context.canvas->drawRect(SkRect::MakeWH(1, 1), paintWithShader);
    } else {
      context.canvas->drawImage(image, 0, 0, sampling, context.sk_paint);
    }
  }
}

void OHOSExternalTextureGL::OnGrContextCreated()
{
  FML_DLOG(INFO)<<" OHOSExternalTextureGL::OnGrContextCreated";
  state_ = AttachmentState::uninitialized;
}

void OHOSExternalTextureGL::OnGrContextDestroyed()
{
  FML_DLOG(INFO)<<" OHOSExternalTextureGL::OnGrContextDestroyed";
  if (state_ == AttachmentState::attached) {
    Detach();
    glDeleteTextures(1, &texture_name_);
    OH_NativeImage_Destroy(&nativeImage_);
  }
  state_ = AttachmentState::detached;
}

void OHOSExternalTextureGL::MarkNewFrameAvailable()
{
  FML_DLOG(INFO)<<" OHOSExternalTextureGL::MarkNewFrameAvailable";
  new_frame_ready_ = true;
}

void OHOSExternalTextureGL::OnTextureUnregistered()
{
  FML_DLOG(INFO)<<" OHOSExternalTextureGL::OnTextureUnregistered";
  // do nothing
}

void OHOSExternalTextureGL::Update()
{
  ProducePixelMapToNativeImage();
  int32_t ret = OH_NativeImage_UpdateSurfaceImage(nativeImage_);
  if (ret != 0) {
    FML_DLOG(FATAL)<<"OHOSExternalTextureGL OH_NativeImage_UpdateSurfaceImage err code:"<< ret;
  }
  UpdateTransform();
}

void OHOSExternalTextureGL::Detach()
{
  OH_NativeImage_DetachContext(nativeImage_);
  OH_NativeWindow_DestroyNativeWindow(nativeWindow_);
}

void OHOSExternalTextureGL::UpdateTransform()
{
  float m[16] = { 0.0f };
  int32_t ret = OH_NativeImage_GetTransformMatrix(nativeImage_, m);
  if (ret != 0) {
    FML_DLOG(FATAL)<<"OHOSExternalTextureGL OH_NativeImage_GetTransformMatrix err code:"<< ret;
  }
  // transform ohos 4x4 matrix to skia 3x3 matrix
  SkScalar matrix3[] = {
    m[0], m[4], m[12],  //
    m[1], m[5], m[13],  //
    m[3], m[7], m[15],  //
  };
  transform.set9(matrix3);
  SkMatrix inverted;
  if (!transform.invert(&inverted)) {
    FML_LOG(FATAL) << "OHOSExternalTextureGL Invalid SurfaceTexture transformation matrix";
  }
  transform = inverted;
}

void OHOSExternalTextureGL::DispatchImage(ImageNative* image)
{
  lastImage_ = image;
}

void OHOSExternalTextureGL::HandlePixelMapBuffer()
{
  BufferHandle *handle = OH_NativeWindow_GetBufferHandleFromNative(buffer_);
  // get virAddr of bufferHandl by mmap sys interface
  void *mappedAddr = mmap(handle->virAddr, handle->size, PROT_READ | PROT_WRITE, MAP_SHARED, handle->fd, 0);
  if (mappedAddr == MAP_FAILED) {
    FML_DLOG(FATAL)<<"OHOSExternalTextureGL mmap failed";
    return;
  }

  void *pixelAddr = nullptr;
  int64_t ret = OH_PixelMap_AccessPixels(pixelMap_, &pixelAddr);
  if (ret != IMAGE_RESULT_SUCCESS) {
    FML_DLOG(FATAL)<<"OHOSExternalTextureGL OH_PixelMap_AccessPixels err:"<< ret;
    return;
  }

  uint32_t *value = static_cast<uint32_t *>(pixelAddr);
  uint32_t *pixel = static_cast<uint32_t *>(mappedAddr);
  uint32_t rowDataSize = DMA_SIZE; // DMA内存会自动补齐，分配内存时是 256 的整数倍
  while (rowDataSize < pixelMapInfo.rowSize) {
    rowDataSize += DMA_SIZE;
  }

  FML_DLOG(INFO) << "OHOSExternalTextureGL pixelMapInfo w:" << pixelMapInfo.width
    << " h:" << pixelMapInfo.height;
  FML_DLOG(INFO) << "OHOSExternalTextureGL pixelMapInfo rowSize:" << pixelMapInfo.rowSize
    << " format:" << pixelMapInfo.pixelFormat;
  FML_DLOG(INFO) << "OHOSExternalTextureGL pixelMapInfo rowDataSize:" << rowDataSize;

  // 复制图片纹理数据到内存中，需要处理DMA内存补齐相关的逻辑
  for (uint32_t i = 0; i < pixelMapInfo.height; i++) {
    memcpy(pixel, value, rowDataSize);
    pixel += rowDataSize / PIXEL_SIZE;
    value += pixelMapInfo.width;
  }

  OH_PixelMap_UnAccessPixels(pixelMap_);
  // munmap after use
  ret = munmap(mappedAddr, handle->size);
  if (ret == -1) {
    FML_DLOG(FATAL)<<"OHOSExternalTextureGL munmap failed";
    return;
  }
}

void OHOSExternalTextureGL::ProducePixelMapToNativeImage()
{
  if (pixelMap_ == nullptr) {
    FML_DLOG(ERROR) << "OHOSExternalTextureGL pixelMap in null";
    return;
  }
  if (state_ == AttachmentState::detached) {
    FML_DLOG(ERROR) << "OHOSExternalTextureGL AttachmentState err";
    return;
  }
  int32_t ret = -1;
  ret = OH_PixelMap_GetImageInfo(pixelMap_, &pixelMapInfo);
  if (ret != 0) {
    FML_DLOG(ERROR) << "OHOSExternalTextureGL OH_PixelMap_GetImageInfo err:" << ret;
  }
  
  int code = SET_BUFFER_GEOMETRY;
  ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow_, code, pixelMapInfo.width, pixelMapInfo.height);
  if (ret != 0) {
    FML_DLOG(ERROR) << "OHOSExternalTextureGL OH_NativeWindow_NativeWindowHandleOpt err:" << ret;
  }

  if (buffer_ != nullptr) {
    OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow_, buffer_);
    buffer_ = nullptr;
  }
  ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow_, &buffer_, &fenceFd);
  if (ret != 0) {
    FML_DLOG(ERROR) << "OHOSExternalTextureGL OH_NativeWindow_NativeWindowRequestBuffer err:" << ret;
  }
  HandlePixelMapBuffer();
  Region region{nullptr, 0};
  ret = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow_, buffer_, fenceFd, region);
  if (ret != 0) {
    FML_DLOG(FATAL)<<"OHOSExternalTextureGL OH_NativeWindow_NativeWindowFlushBuffer err:"<< ret;
  }
}

EGLDisplay OHOSExternalTextureGL::GetPlatformEglDisplay(EGLenum platform, void *native_display,
    const EGLint *attrib_list)
{
  GetPlatformDisplayExt eglGetPlatformDisplayExt = NULL;

  if (!eglGetPlatformDisplayExt) {
    const char* extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    if (extensions &&
        (CheckEglExtension(extensions, EGL_EXT_PLATFORM_WAYLAND) ||
         CheckEglExtension(extensions, EGL_KHR_PLATFORM_WAYLAND))) {
      eglGetPlatformDisplayExt = (GetPlatformDisplayExt)eglGetProcAddress(EGL_GET_PLATFORM_DISPLAY_EXT);
    }
  }

  if (eglGetPlatformDisplayExt) {
    return eglGetPlatformDisplayExt(platform, native_display, attrib_list);
  }

  return eglGetDisplay((EGLNativeDisplayType)native_display);
}

bool OHOSExternalTextureGL::CheckEglExtension(const char *extensions, const char *extension)
{
  size_t extlen = strlen(extension);
  const char *end = extensions + strlen(extensions);
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

void OHOSExternalTextureGL::DispatchPixelMap(NativePixelMap* pixelMap)
{
  if (pixelMap != nullptr) {
    pixelMap_ = pixelMap;
  }
}

}  // namespace flutter