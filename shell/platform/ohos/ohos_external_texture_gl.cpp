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

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2ext.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <utility>

#include "impeller/toolkit/egl/image.h"
#include "third_party/skia/include/core/SkAlphaType.h"
#include "third_party/skia/include/core/SkColorSpace.h"
#include "third_party/skia/include/core/SkColorType.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/gpu/GrBackendSurface.h"
#include "third_party/skia/include/gpu/GrDirectContext.h"
#include "third_party/skia/include/gpu/ganesh/SkImageGanesh.h"
#include "third_party/skia/include/gpu/ganesh/gl/GrGLBackendSurface.h"

namespace flutter {

PFNEGLCREATESYNCKHRPROC OHOSExternalTextureGL::eglCreateSyncKHR_ = nullptr;
PFNEGLDUPNATIVEFENCEFDANDROIDPROC
OHOSExternalTextureGL::eglDupNativeFenceFDANDROID_ = nullptr;
PFNEGLDESTROYSYNCKHRPROC OHOSExternalTextureGL::eglDestroySyncKHR_ = nullptr;
PFNEGLWAITSYNCKHRPROC OHOSExternalTextureGL::eglWaitSyncKHR_ = nullptr;
PFNEGLCREATEIMAGEKHRPROC OHOSExternalTextureGL::eglCreateImageKHR_ = nullptr;
PFNGLEGLIMAGETARGETTEXTURE2DOESPROC
OHOSExternalTextureGL::glEGLImageTargetTexture2DOES_ = nullptr;
PFNEGLDESTROYIMAGEKHRPROC OHOSExternalTextureGL::eglDestroyImageKHR_ = nullptr;

OHOSExternalTextureGL::OHOSExternalTextureGL(
    int64_t id,
    OH_OnFrameAvailableListener listener)
    : OHOSExternalTexture(id, listener) {
  InitEGLFunPtr();
}

OHOSExternalTextureGL::~OHOSExternalTextureGL() {}

void OHOSExternalTextureGL::SetGPUFence(OHNativeWindowBuffer* window_buffer,
                                        int* fence_fd) {
  EGLDisplay disp = eglGetCurrentDisplay();
  if (disp == EGL_NO_DISPLAY) {
    return;
  }

  OH_NativeBuffer* native_buffer = nullptr;
  int ret =
      OH_NativeBuffer_FromNativeWindowBuffer(window_buffer, &native_buffer);
  if (ret != 0 || native_buffer == nullptr) {
    FML_LOG(ERROR) << "OHOSExternalTextureGL get OH_NativeBuffer error:" << ret;
    return;
  }

  // ensure buffer_id > 0 (may get seqNum = 0)
  uint32_t buffer_id = OH_NativeBuffer_GetSeqNum(native_buffer) + 1;

  if (eglCreateSyncKHR_ != nullptr && eglDupNativeFenceFDANDROID_ != nullptr &&
      eglDestroySyncKHR_ != nullptr) {
    EGLSyncKHR fence_sync =
        eglCreateSyncKHR_(disp, EGL_SYNC_NATIVE_FENCE_ANDROID, nullptr);
    *fence_fd = eglDupNativeFenceFDANDROID_(disp, fence_sync);
    FML_DLOG(INFO) << "create norma fence sync fd " << *fence_fd
                   << " fence_sync " << fence_sync << " eglError "
                   << eglGetError();
    glFlush();
    gl_resources_[buffer_id].wait_sync = UniqueEGLSync(fence_sync);
  } else {
    FML_LOG(ERROR) << "get null proc ptr eglCreateSyncKHR:" << eglCreateSyncKHR_
                   << " eglDupNativeFenceFDANDROID:"
                   << eglDupNativeFenceFDANDROID_
                   << " eglDestroySyncKHR:" << eglDestroySyncKHR_;
  }

  EGLenum err = eglGetError();
  // 12288 is EGL_SUCCESS
  if (err != EGL_SUCCESS) {
    FML_LOG(ERROR) << "eglCreateSync get error" << err;
  }
}

void OHOSExternalTextureGL::WaitGPUFence(int fence_fd) {
  EGLDisplay disp = eglGetCurrentDisplay();
  if (disp == EGL_NO_DISPLAY || fence_fd <= 0) {
    return;
  }
  if (FenceIsSignal(fence_fd)) {
    // If the fence_fd is already signaled, it means the related data has
    // already been produced, so there's no need to import it into OpenGL.
    close(fence_fd);
    return;
  }
  if (eglCreateSyncKHR_ != nullptr && eglWaitSyncKHR_ != nullptr &&
      eglDestroySyncKHR_ != nullptr) {
    EGLint attribs[] = {EGL_SYNC_NATIVE_FENCE_FD_ANDROID, fence_fd, EGL_NONE};
    EGLSyncKHR fence_sync =
        eglCreateSyncKHR_(disp, EGL_SYNC_NATIVE_FENCE_ANDROID, attribs);
    if (fence_sync != EGL_NO_SYNC_KHR) {
      eglWaitSyncKHR_(disp, fence_sync, 0);
      gl_resources_[now_key_].wait_sync = UniqueEGLSync(fence_sync);
    } else {
      // eglDestroySync will close the fence_fd.
      close(fence_fd);
    }
  } else {
    close(fence_fd);
  }

  EGLenum err = eglGetError();
  if (err != EGL_SUCCESS) {
    FML_LOG(ERROR) << "eglWaitSync get error" << err;
  }
}

void OHOSExternalTextureGL::GPUResourceDestroy() {
  gl_resources_.clear();
  // here we should have context.
  GLenum err = glGetError();
  if (err != GL_NO_ERROR) {
    FML_LOG(ERROR) << "GPUResourceDestroy get gl error:" << glGetError();
  }
}

sk_sp<flutter::DlImage> OHOSExternalTextureGL::CreateDlImage(
    PaintContext& context,
    const SkRect& bounds,
    NativeBufferKey key,
    OHNativeWindowBuffer* nw_buffer) {
  GLuint texture_name = 0;
  glGenTextures(1, &texture_name);
  impeller::UniqueGLTexture unique_texture(impeller::GLTexture{texture_name});
  OHOSUniqueEGLImageKHR unique_eglimage = CreateEGLImage(nw_buffer);
  if (!unique_eglimage.is_valid() || texture_name == 0) {
    return nullptr;
  }
  glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_name);
  if (glEGLImageTargetTexture2DOES_ != nullptr) {
    glEGLImageTargetTexture2DOES_(GL_TEXTURE_EXTERNAL_OES,
                                  (GLeglImageOES)unique_eglimage.get().image);
  } else {
    FML_LOG(ERROR) << "get null glEGLImageTargetTexture2DOES";
    return nullptr;
  }
  GrGLTextureInfo textureInfo = {
      GL_TEXTURE_EXTERNAL_OES, unique_texture.get().texture_name, GL_RGBA8_OES};
  auto backendTexture =
      GrBackendTextures::MakeGL(1, 1, skgpu::Mipmapped::kNo, textureInfo);
  gl_resources_[key] = GlResource{std::move(unique_eglimage),
                                  std::move(unique_texture), UniqueEGLSync()};

  sk_sp<SkImage> image = SkImages::BorrowTextureFrom(
      context.gr_context, backendTexture, kTopLeft_GrSurfaceOrigin,
      kRGBA_8888_SkColorType, kPremul_SkAlphaType, nullptr);
  sk_sp<flutter::DlImage> dl_image = DlImage::Make(image);

  // lru: oldest resource need earse
  now_key_ = key;
  gl_resources_.erase(image_lru_.AddImage(dl_image, key));
  return dl_image;
}

OHOSUniqueEGLImageKHR OHOSExternalTextureGL::CreateEGLImage(
    OHNativeWindowBuffer* nw_buffer) {
  EGLDisplay disp = eglGetCurrentDisplay();
  if (disp == EGL_NO_DISPLAY || nw_buffer == nullptr) {
    return OHOSUniqueEGLImageKHR();
  }
  EGLint attrs[] = {EGL_IMAGE_PRESERVED, EGL_TRUE, EGL_NONE};

  if (eglCreateImageKHR_ == nullptr) {
    FML_LOG(ERROR) << "get null eglCreateImageKHR";
    return OHOSUniqueEGLImageKHR();
  }

  impeller::EGLImageKHRWithDisplay ohos_eglimage =
      impeller::EGLImageKHRWithDisplay{
          eglCreateImageKHR_(disp, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_OHOS,
                             nw_buffer, attrs),
          disp};
  EGLenum err = eglGetError();
  if (err != EGL_SUCCESS) {
    FML_LOG(ERROR) << "eglCreateImageKHR get error" << err;
  }

  return OHOSUniqueEGLImageKHR(ohos_eglimage);
}

void OHOSExternalTextureGL::InitEGLFunPtr() {
  static void* handle = dlopen("libEGL.so", RTLD_NOW);
  // if we use eglGetProcAddress, we may get the libhvgr.so's func address.
  // But normal egl func address(from libEGL.so) is pointed to a egl wrapper
  // layer. Their parameters, despite having the same type name, refer to
  // different underlying data structures and are not interchangeable(such as
  // EGLDisplay). So we get address from dlsym first, if not then
  // eglGetProcAddress.
  if (eglCreateSyncKHR_ == nullptr) {
    eglCreateSyncKHR_ =
        (PFNEGLCREATESYNCKHRPROC)dlsym(handle, "eglCreateSyncKHR");
    if (eglCreateSyncKHR_ != nullptr) {
      eglCreateSyncKHR_ =
          (PFNEGLCREATESYNCKHRPROC)eglGetProcAddress("eglCreateSyncKHR");
    }
  }
  if (eglDupNativeFenceFDANDROID_ == nullptr) {
    eglDupNativeFenceFDANDROID_ = (PFNEGLDUPNATIVEFENCEFDANDROIDPROC)dlsym(
        handle, "eglDupNativeFenceFDANDROID");

    if (eglDupNativeFenceFDANDROID_ == nullptr) {
      eglDupNativeFenceFDANDROID_ =
          (PFNEGLDUPNATIVEFENCEFDANDROIDPROC)eglGetProcAddress(
              "eglDupNativeFenceFDANDROID");
    }
  }
  if (eglDestroySyncKHR_ == nullptr) {
    eglDestroySyncKHR_ =
        (PFNEGLDESTROYSYNCKHRPROC)dlsym(handle, "eglDestroySyncKHR");
    if (eglDestroySyncKHR_ == nullptr) {
      eglDestroySyncKHR_ =
          (PFNEGLDESTROYSYNCKHRPROC)eglGetProcAddress("eglDestroySyncKHR");
    }
  }
  if (eglWaitSyncKHR_ == nullptr) {
    eglWaitSyncKHR_ = (PFNEGLWAITSYNCKHRPROC)dlsym(handle, "eglWaitSyncKHR");
    if (eglWaitSyncKHR_ == nullptr) {
      eglWaitSyncKHR_ =
          (PFNEGLWAITSYNCKHRPROC)eglGetProcAddress("eglWaitSyncKHR");
    }
  }
  if (eglCreateImageKHR_ == nullptr) {
    eglCreateImageKHR_ =
        (PFNEGLCREATEIMAGEKHRPROC)dlsym(handle, "eglCreateImageKHR");
    if (eglCreateImageKHR_ == nullptr) {
      eglCreateImageKHR_ =
          (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
    }
  }
  if (eglDestroyImageKHR_ == nullptr) {
    eglDestroyImageKHR_ =
        (PFNEGLDESTROYIMAGEKHRPROC)dlsym(handle, "eglDestroyImageKHR");
    if (eglDestroyImageKHR_ == nullptr) {
      eglDestroyImageKHR_ =
          (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
    }
  }
  if (glEGLImageTargetTexture2DOES_ == nullptr) {
    glEGLImageTargetTexture2DOES_ = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)dlsym(
        handle, "glEGLImageTargetTexture2DOES");
    if (glEGLImageTargetTexture2DOES_ == nullptr) {
      glEGLImageTargetTexture2DOES_ =
          (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress(
              "glEGLImageTargetTexture2DOES");
    }
  }
}

}  // namespace flutter