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

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_OHOS_EXTERNAL_TEXTURE_VULKAN_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_OHOS_EXTERNAL_TEXTURE_VULKAN_H_

#include <native_window/external_window.h>
#include "flutter/impeller/renderer/backend/vulkan/context_vk.h"
#include "impeller/renderer/backend/vulkan/texture_vk.h"
#include "ohos_external_texture.h"

namespace flutter {

struct VkResource;

class OHOSExternalTextureVulkan : public OHOSExternalTexture {
 public:
  explicit OHOSExternalTextureVulkan(
      const std::shared_ptr<impeller::ContextVK>& impeller_context,
      int64_t id,
      OH_OnFrameAvailableListener listener);

  ~OHOSExternalTextureVulkan() override;

 protected:
  std::unordered_map<NativeBufferKey, VkResource> vk_resources_;
  NativeBufferKey now_key_;

  void SetGPUFence(OHNativeWindowBuffer* window_buffer, int* fence_fd) override;
  void WaitGPUFence(int fence_fd) override;
  void GPUResourceDestroy() override;

  sk_sp<flutter::DlImage> CreateDlImage(
      PaintContext& context,
      const SkRect& bounds,
      NativeBufferKey key,
      OHNativeWindowBuffer* nw_buffer) override;

 private:
  const std::shared_ptr<impeller::ContextVK> impeller_context_;
  impeller::vk::UniqueSemaphore CreateVkSemaphore(int fence_fd);

  FML_DISALLOW_COPY_AND_ASSIGN(OHOSExternalTextureVulkan);
};

struct VkResource {
  std::shared_ptr<impeller::TextureVK> texture;
  // This is used to ensure that when the window_buffer is held by the producer,
  // the corresponding vksemaphore associated with the fence_fd will not be
  // destroyed.
  impeller::vk::UniqueSemaphore signal_semaphore;
};

}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_OHOS_EXTERNAL_TEXTURE_VULKAN_H_