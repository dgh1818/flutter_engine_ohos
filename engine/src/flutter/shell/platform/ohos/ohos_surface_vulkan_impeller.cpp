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

#include "ohos_surface_vulkan_impeller.h"

#include <memory>
#include <utility>

#include "flutter/fml/logging.h"
#include "flutter/fml/memory/ref_ptr.h"
#include "flutter/impeller/renderer/backend/vulkan/context_vk.h"
#include "flutter/shell/gpu/gpu_surface_vulkan_impeller.h"

namespace flutter {

OHOSSurfaceVulkanImpeller::OHOSSurfaceVulkanImpeller(
    const std::shared_ptr<OHOSContext>& ohos_context)
    : OHOSSurface(ohos_context) {
  is_valid_ = ohos_context->IsValid();
  auto& context_vk =
      impeller::ContextVK::Cast(*ohos_context->GetImpellerContext());
  surface_context_vk_ = context_vk.CreateSurfaceContext();
}

OHOSSurfaceVulkanImpeller::~OHOSSurfaceVulkanImpeller() {}

// |OHOSSurface|
bool OHOSSurfaceVulkanImpeller::IsValid() const {
  return is_valid_;
}

// |OHOSSurface|
std::unique_ptr<Surface> OHOSSurfaceVulkanImpeller::CreateGPUSurface(
    GrDirectContext* gr_context) {
  if (!IsValid()) {
    return nullptr;
  }

  if (!native_window_ || !native_window_->IsValid()) {
    return nullptr;
  }

  std::unique_ptr<GPUSurfaceVulkanImpeller> gpu_surface =
      std::make_unique<GPUSurfaceVulkanImpeller>(surface_context_vk_);

  if (!gpu_surface->IsValid()) {
    return nullptr;
  }

  return gpu_surface;
}

// |OHOSSurface|
void OHOSSurfaceVulkanImpeller::TeardownOnScreenContext() {
  // do nothing
}

// |OHOSSurface|
bool OHOSSurfaceVulkanImpeller::OnScreenSurfaceResize(const SkISize& size) {
  surface_context_vk_->UpdateSurfaceSize(
      impeller::ISize{size.width(), size.height()});
  return true;
}

// |OHOSSurface|
bool OHOSSurfaceVulkanImpeller::ResourceContextMakeCurrent() {
  // do nothing (it is not opengl)
  return true;
}

// |OHOSSurface|
bool OHOSSurfaceVulkanImpeller::ResourceContextClearCurrent() {
  // do nothing (it is not opengl)
  return true;
}

// |OHOSSurface|
bool OHOSSurfaceVulkanImpeller::SetNativeWindow(
    fml::RefPtr<OHOSNativeWindow> window) {
  native_window_ = std::move(window);
  bool success = native_window_ && native_window_->IsValid();

  if (success) {
    auto surface =
        surface_context_vk_->CreateOHOSSurface(native_window_->Gethandle());

    if (!surface) {
      FML_LOG(ERROR) << "Could not create a vulkan surface.";
      return false;
    }
    auto size = native_window_->GetSize();
    return surface_context_vk_->SetWindowSurface(
        std::move(surface), impeller::ISize{size.width(), size.height()});
  }

  native_window_ = nullptr;
  return false;
}

std::shared_ptr<impeller::Context>
OHOSSurfaceVulkanImpeller::GetImpellerContext() {
  return surface_context_vk_;
}

}  // namespace flutter