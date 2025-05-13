/*
 * Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd.
 * All rights reserved. Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE_KHZG file.
 */
#ifndef FLUTTER_SHELL_PLATFORM_OHOS_OHOS_EXTERNAL_TEXTURE_GL_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_OHOS_EXTERNAL_TEXTURE_GL_H_

#include "ohos_external_texture.h"

#include "flutter/impeller/toolkit/egl/image.h"
#include "flutter/impeller/toolkit/gles/texture.h"

namespace flutter {

struct OHOSEGLImageKHRWithDisplayTraits;
struct EGLSyncKHRTraits;
struct GlResource;

using OHOSUniqueEGLImageKHR =
    fml::UniqueObject<impeller::EGLImageKHRWithDisplay,
                      OHOSEGLImageKHRWithDisplayTraits>;
using UniqueEGLSync = fml::UniqueObject<EGLSyncKHR, EGLSyncKHRTraits>;

class OHOSExternalTextureGL : public OHOSExternalTexture {
 public:
  explicit OHOSExternalTextureGL(int64_t id,
                                 OH_OnFrameAvailableListener listener);

  ~OHOSExternalTextureGL() override;

  static PFNEGLCREATESYNCKHRPROC eglCreateSyncKHR_;
  static PFNEGLDUPNATIVEFENCEFDANDROIDPROC eglDupNativeFenceFDANDROID_;
  static PFNEGLDESTROYSYNCKHRPROC eglDestroySyncKHR_;
  static PFNEGLWAITSYNCKHRPROC eglWaitSyncKHR_;
  static PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR_;
  static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES_;
  static PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR_;

  static void InitEGLFunPtr();

 protected:
  void SetGPUFence(OHNativeWindowBuffer* window_buffer, int* fence_fd) override;
  void WaitGPUFence(int fence_fd) override;
  void GPUResourceDestroy() override;
  void DeleteBufferGPUResource(NativeBufferKey key) override;

  sk_sp<flutter::DlImage> CreateDlImage(
      PaintContext& context,
      const SkRect& bounds,
      NativeBufferKey key,
      OH_NativeBuffer_Config& config,
      OHNativeWindowBuffer* nw_buffer) override;

 private:
  std::unordered_map<NativeBufferKey, GlResource> gl_resources_;
  NativeBufferKey now_key_;

  // void UpdateTransform();
  OHOSUniqueEGLImageKHR CreateEGLImage(OHNativeWindowBuffer* nw_buffer);

  FML_DISALLOW_COPY_AND_ASSIGN(OHOSExternalTextureGL);
};

// ohos' sdk don't have eglDestroyImageKHR symbol, so we manually get the
// eglDestroyImageKHR address.
struct OHOSEGLImageKHRWithDisplayTraits {
  static impeller::EGLImageKHRWithDisplay InvalidValue() {
    return {EGL_NO_IMAGE_KHR, EGL_NO_DISPLAY};
  }

  static bool IsValid(const impeller::EGLImageKHRWithDisplay& value) {
    return value != InvalidValue();
  }

  static void Free(impeller::EGLImageKHRWithDisplay image) {
    if (OHOSExternalTextureGL::eglDestroyImageKHR_) {
      OHOSExternalTextureGL::eglDestroyImageKHR_(image.display, image.image);
    }
  }
};

struct EGLSyncKHRTraits {
  static EGLSyncKHR InvalidValue() { return EGL_NO_SYNC_KHR; }

  static bool IsValid(const EGLSyncKHR& value) {
    return value != InvalidValue();
  }

  static void Free(EGLSyncKHR sync) {
    if (OHOSExternalTextureGL::eglDestroySyncKHR_) {
      EGLDisplay disp = eglGetCurrentDisplay();
      OHOSExternalTextureGL::eglDestroySyncKHR_(disp, sync);
    }
  }
};

struct GlResource {
  OHOSUniqueEGLImageKHR egl_image;
  impeller::UniqueGLTexture texture;
  UniqueEGLSync wait_sync;
  UniqueEGLSync signal_sync;
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_OHOS_OHOS_EXTERNAL_TEXTURE_GL_H_