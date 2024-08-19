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

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_OHOS_EXTERNAL_TEXTURE_GL_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_OHOS_EXTERNAL_TEXTURE_GL_H_

#include "ohos_external_texture.h"

#include "flutter/impeller/toolkit/egl/image.h"
#include "flutter/impeller/toolkit/gles/texture.h"

namespace impeller {
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
    static PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR =
        (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
    if (eglDestroyImageKHR == nullptr) {
      FML_LOG(ERROR) << "get null eglDestroyImageKHR";
      return;
    }
    eglDestroyImageKHR(image.display, image.image);
  }
};

using OHOSUniqueEGLImageKHR =
    fml::UniqueObject<impeller::EGLImageKHRWithDisplay,
                      OHOSEGLImageKHRWithDisplayTraits>;
}  // namespace impeller

namespace flutter {

class OHOSExternalTextureGL : public OHOSExternalTexture {
 public:
  explicit OHOSExternalTextureGL(int64_t id,
                                 OH_OnFrameAvailableListener listener);

  ~OHOSExternalTextureGL() override;

 protected:
  void SetGPUFence(int* fence_fd) override;
  void WaitGPUFence(int fence_fd) override;
  void GPUResourceDestroy() override;

  sk_sp<flutter::DlImage> CreateDlImage(
      PaintContext& context,
      const SkRect& bounds,
      NativeBufferKey key,
      OHNativeWindowBuffer* nw_buffer) override;

 private:
  struct GlResource {
    impeller::OHOSUniqueEGLImageKHR egl_image;
    impeller::UniqueGLTexture texture;
  };

  std::unordered_map<NativeBufferKey, GlResource> gl_resources_;

  // void UpdateTransform();
  impeller::OHOSUniqueEGLImageKHR CreateEGLImage(
      OHNativeWindowBuffer* nw_buffer);

  static PFNEGLCREATESYNCKHRPROC eglCreateSyncKHR_;
  static PFNEGLDUPNATIVEFENCEFDANDROIDPROC eglDupNativeFenceFDANDROID_;
  static PFNEGLDESTROYSYNCKHRPROC eglDestroySyncKHR_;
  static PFNEGLWAITSYNCKHRPROC eglWaitSyncKHR_;
  static PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR_;
  static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES_;

  static void InitEGLFunPtr();

  FML_DISALLOW_COPY_AND_ASSIGN(OHOSExternalTextureGL);
};

}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_OHOS_EXTERNAL_TEXTURE_GL_H_