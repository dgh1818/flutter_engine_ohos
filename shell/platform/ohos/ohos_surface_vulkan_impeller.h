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

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_OHOS_SURFACE_VULKAN_IMPELLER_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_OHOS_SURFACE_VULKAN_IMPELLER_H_

#include "flutter/fml/concurrent_message_loop.h"
#include "flutter/fml/macros.h"
#include "flutter/impeller/renderer/backend/vulkan/surface_context_vk.h"
#include "flutter/impeller/renderer/context.h"

#include "shell/platform/ohos/ohos_context_vulkan_impeller.h"
#include "surface/ohos_native_window.h"
#include "surface/ohos_surface.h"

namespace flutter {

class OHOSSurfaceVulkanImpeller : public OHOSSurface {
 public:
  explicit OHOSSurfaceVulkanImpeller(
      const std::shared_ptr<OHOSContext>& ohos_context);

  ~OHOSSurfaceVulkanImpeller() override;

  // |OHOSSurface|
  bool IsValid() const override;

  // |OHOSSurface|
  std::unique_ptr<Surface> CreateGPUSurface(
      GrDirectContext* gr_context) override;

  // |OHOSSurface|
  void TeardownOnScreenContext() override;

  // |OHOSSurface|
  bool OnScreenSurfaceResize(const SkISize& size) override;

  // |OHOSSurface|
  bool ResourceContextMakeCurrent() override;

  // |OHOSSurface|
  bool ResourceContextClearCurrent() override;

  // |OHOSSurface|
  std::shared_ptr<impeller::Context> GetImpellerContext() override;

  // |OHOSSurface|
  bool SetNativeWindow(fml::RefPtr<OHOSNativeWindow> window) override;

 private:
  fml::RefPtr<OHOSNativeWindow> native_window_;
  std::shared_ptr<impeller::SurfaceContextVK> surface_context_vk_;
  bool is_valid_ = false;

  FML_DISALLOW_COPY_AND_ASSIGN(OHOSSurfaceVulkanImpeller);
};
}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_OHOS_SURFACE_VULKAN_IMPELLER_H_