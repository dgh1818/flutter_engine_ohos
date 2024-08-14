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
#include "fml/trace_event.h"
#include "shell/platform/ohos/surface/ohos_surface.h"

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

  std::lock_guard<std::mutex> lock(surface_preload_mutex_);

  if (preload_gpu_surface_ && preload_gpu_surface_->IsValid()) {
    return std::move(preload_gpu_surface_);
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
  if (!window) {
    native_window_ = nullptr;
    return false;
  }
  TRACE_EVENT0("flutter", "OHOSSurfaceVulkanImpeller-SetNativeWindow");

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

bool OHOSSurfaceVulkanImpeller::PrepareOffscreenWindow(int32_t width,
                                                       int32_t height) {
  // Currently, when creating a Vulkan swapchain, a completely clean
  // NativeWindow is required. Any flushBuffer actions on this window will cause
  // the swapchain creation to stall (creation involves requesting all buffers
  // within the Vulkan implementation, and flushing will occupy one buffer
  // position, causing the buffer queue to fill up prematurely and preventing
  // Vulkan from obtaining enough free buffers). Therefore, we cannot directly
  // pass the off-screen rendered buffer to RS. Additionally, swapchain creation
  // requires requesting all buffers, making it time-consuming and costly.
  // Rendering the buffer to the screen buffer is also challenging: we need to
  // obtain various contexts wrapped by Impeller and invoke them according to
  // its conventions. Hence, we opt to only perform the time-consuming surface
  // creation work here without using NativeImage for off-screen rendering.
  TRACE_EVENT0("flutter", "impeller-PrepareContext");
  std::lock_guard<std::mutex> lock(surface_preload_mutex_);
  preload_gpu_surface_ =
      std::make_unique<GPUSurfaceVulkanImpeller>(surface_context_vk_);
  // return false means that it will not invoke PlatformView::NotifyCreated().
  // return false;
  // If we want to skip time-consuming tasks during the first frame, we can
  // render it to offscreen window. However, the result of the offscreen
  // rendering will not be drawn to the onscreen window.
  OHOSSurface::PrepareOffscreenWindow(width, height);
  return true;
}

std::shared_ptr<impeller::Context>
OHOSSurfaceVulkanImpeller::GetImpellerContext() {
  return surface_context_vk_;
}

}  // namespace flutter