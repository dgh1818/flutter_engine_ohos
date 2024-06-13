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

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

#include <multimedia/image_framework/image_mdk.h>
#include <multimedia/image_framework/image_pixel_map_mdk.h>
#include <native_buffer/native_buffer.h>
#include <native_image/native_image.h>
#include <native_window/external_window.h>

#include "flutter/common/graphics/texture.h"
#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"
#include "flutter/shell/platform/ohos/ohos_surface_gl_skia.h"
#include "flutter/shell/platform/ohos/surface/ohos_surface.h"

// maybe now unused
namespace flutter {

class OHOSExternalTextureGL : public flutter::Texture {
 public:
  explicit OHOSExternalTextureGL(
      int64_t id,
      const std::shared_ptr<OHOSSurface>& ohos_surface);

  ~OHOSExternalTextureGL() override;

  OH_NativeImage* nativeImage_;

  void Paint(PaintContext& context,
             const SkRect& bounds,
             bool freeze,
             DlImageSampling sampling) override;

  void OnGrContextCreated() override;

  void OnGrContextDestroyed() override;

  void MarkNewFrameAvailable() override;

  void OnTextureUnregistered() override;

  void DispatchImage(ImageNative* image);

  void setBackground(int32_t width, int32_t height);

  void DispatchPixelMap(NativePixelMap* pixelMap);

 private:
  void Attach();

  void Update();

  void Detach();

  void UpdateTransform();

  EGLDisplay GetPlatformEglDisplay(EGLenum platform,
                                   void* native_display,
                                   const EGLint* attrib_list);

  bool CheckEglExtension(const char* extensions, const char* extension);

  void HandlePixelMapBuffer();

  void ProducePixelMapToNativeImage();

  enum class AttachmentState { uninitialized, attached, detached };

  AttachmentState state_ = AttachmentState::uninitialized;

  bool new_frame_ready_ = false;

  bool first_update_ = false;

  GLuint texture_name_ = 0;

  std::shared_ptr<OHOSSurface> ohos_surface_;

  SkMatrix transform_;

  OHNativeWindow* nativeWindow_;

  OHNativeWindowBuffer* buffer_;

  NativePixelMap* pixelMap_;

  ImageNative* lastImage_;

  bool isEmulator_;

  OhosPixelMapInfos pixelMapInfo;

  int fenceFd = -1;

  EGLContext eglContext_;
  EGLDisplay eglDisplay_;

  FML_DISALLOW_COPY_AND_ASSIGN(OHOSExternalTextureGL);
};
}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_OHOS_EXTERNAL_TEXTURE_GL_H_