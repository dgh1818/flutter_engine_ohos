#include "ohos_external_texture_vulkan.h"
#include <fcntl.h>
#include <native_buffer/native_buffer.h>
#include <native_window/external_window.h>
#include <unistd.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_ohos.h>
#include <vector>

#include "flutter/impeller/core/formats.h"
#include "flutter/impeller/core/texture_descriptor.h"
#include "flutter/impeller/display_list/dl_image_impeller.h"

#include "flutter/impeller/renderer/backend/vulkan/command_buffer_vk.h"
#include "flutter/impeller/renderer/backend/vulkan/command_encoder_vk.h"
#include "flutter/impeller/renderer/backend/vulkan/ohos/ohb_texture_source_vk.h"
#include "flutter/impeller/renderer/backend/vulkan/texture_vk.h"
#include "fml/logging.h"
#include "vulkan/vulkan_core.h"

namespace flutter {

OHOSExternalTextureVulkan::OHOSExternalTextureVulkan(
    const std::shared_ptr<impeller::ContextVK>& impeller_context,
    int64_t id,
    OH_OnFrameAvailableListener listener)
    : OHOSExternalTexture(id, listener), impeller_context_(impeller_context) {}

OHOSExternalTextureVulkan::~OHOSExternalTextureVulkan() {}

void OHOSExternalTextureVulkan::SetGPUFence(int* fence_fd) {
  *fence_fd = -1;
  // need create vkSemaphore with fence_fd in the future.
  return;
}

impeller::vk::UniqueSemaphore OHOSExternalTextureVulkan::CreateVkSemaphore(
    int fence_fd) {
  // 创建semaphore info
  impeller::vk::SemaphoreCreateInfo semaphore_info;
  impeller::vk::Device device = impeller_context_->GetDevice();
  auto semaphore_result = device.createSemaphoreUnique(semaphore_info, nullptr);
  if (semaphore_result.result != impeller::vk::Result::eSuccess) {
    FML_LOG(ERROR) << "vkCreateSemaphore failed";
    close(fence_fd);
    return impeller::vk::UniqueSemaphore();
  }

  auto semaphore = std::move(semaphore_result.value);

  impeller::vk::ImportSemaphoreFdInfoKHR import_info;
  import_info.setSemaphore(semaphore.get());
  import_info.setFlags(impeller::vk::SemaphoreImportFlagBits::eTemporary);
  import_info.setFd(fence_fd);
  import_info.setHandleType(
      impeller::vk::ExternalSemaphoreHandleTypeFlagBits::eSyncFd);

  auto import_result = device.importSemaphoreFdKHR(import_info);
  if (import_result != impeller::vk::Result::eSuccess) {
    FML_LOG(ERROR) << "importSemaphoreFdKHR failed";
    close(fence_fd);
    return impeller::vk::UniqueSemaphore();
  }
  return semaphore;
}

void OHOSExternalTextureVulkan::WaitGPUFence(int fence_fd) {
  if (fence_fd > 0) {
    auto semaphore = CreateVkSemaphore(fence_fd);

    if (semaphore.get() == VK_NULL_HANDLE) {
      return;
    }

    impeller::vk::SubmitInfo submit_info;
    submit_info.setCommandBufferCount(0);
    submit_info.setWaitSemaphoreCount(1);
    submit_info.setWaitSemaphores(semaphore.get());
    impeller::vk::PipelineStageFlags wait_stage =
        impeller::vk::PipelineStageFlagBits::eFragmentShader;
    submit_info.setWaitDstStageMask(wait_stage);

    auto result = impeller_context_->GetGraphicsQueue()->Submit(
        submit_info, impeller::vk::Fence());

    if (result != impeller::vk::Result::eSuccess) {
      FML_LOG(ERROR) << "Could not wait on render semaphore: "
                     << impeller::vk::to_string(result);
      return;
    }
    // we cannot destroy semaphore until it is signal in vulkan.
    vk_resources_[now_key_].wait_semaphore = std::move(semaphore);
  }

  auto texture = vk_resources_[now_key_].texture;
  if (texture) {
    // Transition the layout to shader read.
    auto buffer = impeller_context_->CreateCommandBuffer();
    impeller::CommandBufferVK& buffer_vk =
        impeller::CommandBufferVK::Cast(*buffer);

    impeller::BarrierVK barrier;
    barrier.cmd_buffer = buffer_vk.GetEncoder()->GetCommandBuffer();
    barrier.src_access = impeller::vk::AccessFlagBits::eColorAttachmentWrite |
                         impeller::vk::AccessFlagBits::eTransferWrite;
    barrier.src_stage =
        impeller::vk::PipelineStageFlagBits::eColorAttachmentOutput |
        impeller::vk::PipelineStageFlagBits::eTransfer;
    barrier.dst_access = impeller::vk::AccessFlagBits::eShaderRead;
    barrier.dst_stage = impeller::vk::PipelineStageFlagBits::eFragmentShader;

    barrier.new_layout = impeller::vk::ImageLayout::eShaderReadOnlyOptimal;

    if (!texture->SetLayout(barrier)) {
      return;
    }
    if (!impeller_context_->GetCommandQueue()->Submit({buffer}).ok()) {
      return;
    }
  }

  return;
}

void OHOSExternalTextureVulkan::GPUResourceDestroy() {
  vk_resources_.clear();
}

sk_sp<flutter::DlImage> OHOSExternalTextureVulkan::CreateDlImage(
    PaintContext& context,
    const SkRect& bounds,
    NativeBufferKey key,
    OHNativeWindowBuffer* nw_buffer) {
  auto texture_source = std::make_shared<impeller::OHBTextureSourceVK>(
      impeller_context_, nw_buffer);
  if (!texture_source->IsValid()) {
    FML_LOG(INFO) << " OHOSExternalTexture::CreateDlImage is null";
    return nullptr;
  }

  auto texture =
      std::make_shared<impeller::TextureVK>(impeller_context_, texture_source);

  auto dl_image = impeller::DlImageImpeller::Make(texture);

  vk_resources_[key] = {.texture = texture};
  now_key_ = key;
  vk_resources_.erase(image_lru_.AddImage(dl_image, key));
  return dl_image;
}

}  // namespace flutter
