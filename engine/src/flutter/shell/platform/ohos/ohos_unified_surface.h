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

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_OHOS_UNIFIED_SURFACE_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_OHOS_UNIFIED_SURFACE_H_

#include <memory>
#include "flutter/flow/surface.h"
#include "flutter/shell/platform/ohos/context/ohos_context.h"
#include "flutter/shell/platform/ohos/ohos_surface_gl_skia.h"
#include "flutter/shell/platform/ohos/surface/ohos_native_window.h"
#include "flutter/shell/platform/ohos/surface/ohos_surface.h"
#include "third_party/skia/include/core/SkSize.h"

namespace flutter {

class OHOSUnifiedSurface : public GPUSurfaceGLDelegate, public OHOSSurface {
 public:
  ~OHOSUnifiedSurface() {}
  bool IsValid() {}
  void TeardownOnScreenContext() {}

  bool OnScreenSurfaceResize(const SkISize& size) {}

  bool ResourceContextMakeCurrent() {}

  bool ResourceContextClearCurrent() {}

  bool SetNativeWindow(fml::RefPtr<OHOSNativeWindow> window) {}

  std::unique_ptr<Surface> CreateSnapshotSurface()

      std::unique_ptr<Surface> CreateGPUSurface(
          GrDirectContext* gr_context = nullptr) {}

  std::shared_ptr<impeller::Context> GetImpellerContext() {}

  std::unique_ptr<GLContextResult> GLContextMakeCurrent() {}

  bool GLContextClearCurrent() {}

  void GLContextSetDamageRegion(const std::optional<SkIRect>& region) {}

  bool GLContextPresent(const GLPresentInfo& present_info) {}

  GLFBOInfo GLContextFBO(GLFrameInfo frame_info) {}

  bool GLContextFBOResetAfterPresent() {}

  SurfaceFrame::FramebufferInfo GLContextFramebufferInfo() {}

  SkMatrix GLContextSurfaceTransformation() {}

  sk_sp<const GrGLInterface> GetGLInterface() {}

  static sk_sp<const GrGLInterface> GetDefaultPlatformGLInterface() {}

  using GLProcResolver = std::function<void*(const char*)>;

  GLProcResolver GetGLProcResolver() {}

  bool AllowsDrawingWhenGpuDisabled() {}
};
}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_OHOS_OHOS_UNIFIED_SURFACE_H_