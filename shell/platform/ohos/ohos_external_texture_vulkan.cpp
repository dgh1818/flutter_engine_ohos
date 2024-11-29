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
#include "impeller/renderer/backend/vulkan/fence_waiter_vk.h"
#include "vulkan/vulkan_core.h"

namespace flutter {

OHOSExternalTextureVulkan::OHOSExternalTextureVulkan(
    const std::shared_ptr<impeller::ContextVK>& impeller_context,
    int64_t id,
    OH_OnFrameAvailableListener listener)
    : OHOSExternalTexture(id, listener), impeller_context_(impeller_context) {}

OHOSExternalTextureVulkan::~OHOSExternalTextureVulkan() {}

void OHOSExternalTextureVulkan::SetGPUFence(OHNativeWindowBuffer* window_buffer,
                                            int* fence_fd) {
  /// We cannot move the logic for generating fence_fd into the fence_callback
  /// of QueueSubmit because Vulkan's task reordering means that
  /// submitting a rendering task doesn’t guarantee its execution before the
  /// semaphore specified by QueueSignalReleaseImageOHOS. This could cause
  /// synchronization issues. To address this, the SetGPUFence call is adjusted
  /// to trigger only when the fence_fd is actually needed—specifically, after
  /// presenting the frame that uses the buffer. This ensures that the semaphore
  /// is triggered after its corresponding rendering task.

  if (fence_fd == nullptr || window_buffer == nullptr) {
    return;
  }
  if (FdIsValid(*fence_fd)) {
    close(*fence_fd);
  }
  *fence_fd = -1;

  OH_NativeBuffer* native_buffer = nullptr;
  int ret =
      OH_NativeBuffer_FromNativeWindowBuffer(window_buffer, &native_buffer);
  if (ret != 0 || native_buffer == nullptr) {
    FML_LOG(ERROR) << "OHOSExternalTextureVulkan get OH_NativeBuffer error:"
                   << ret;
    return;
  }

  // ensure buffer_id > 0 (may get seqNum = 0)
  uint32_t buffer_id = OH_NativeBuffer_GetSeqNum(native_buffer) + 1;
  auto texture = vk_resources_[buffer_id].texture;
  auto device = impeller_context_->GetDevice();
  if (texture && device) {
    impeller::vk::ExportSemaphoreCreateInfo export_info;
    export_info.setHandleTypes(
        impeller::vk::ExternalSemaphoreHandleTypeFlagBits::eSyncFd);

    impeller::vk::SemaphoreCreateInfo create_info;
    create_info.setPNext(&export_info);
    auto ret = device.createSemaphoreUnique(create_info);
    if (ret.result != impeller::vk::Result::eSuccess || !ret.value) {
      FML_LOG(ERROR) << "createSemaphoreUnique in SetGPUFence failed";
      return;
    }

    // Use it to ensure that when the window_buffer is held by the producer, the
    // corresponding vksemaphore associated with the fence_fd will not be
    // destroyed.
    vk_resources_[buffer_id].signal_semaphore = std::move(ret.value);

    std::vector<impeller::vk::Semaphore> semaphore_vector;
    semaphore_vector.push_back(vk_resources_[buffer_id].signal_semaphore.get());

    // The logic of vkQueueSignalReleaseImageOHOS includes adding the
    // vkSemaphore to the GraphicQueue, so this vkSemaphore does not require an
    // additional submission through vkQueueSubmit. When the GraphicQueue
    // reaches this point, the vkSemaphore and fence_fd will be signaled.
    auto result =
        impeller_context_->GetGraphicsQueue()->QueueSignalReleaseImageOHOS(
            semaphore_vector,
            vk_resources_[buffer_id].texture->GetTextureSource()->GetImage(),
            fence_fd);
    if (result != impeller::vk::Result::eSuccess) {
      FML_LOG(ERROR) << "Could not QueueSignalReleaseImageOHOS: "
                     << impeller::vk::to_string(result) << " fence_fd "
                     << *fence_fd;
      // Sometimes, it may return a valid file descriptor even if the call
      // fails.
      if (*fence_fd != -1 && FdIsValid(*fence_fd)) {
        close(*fence_fd);
      }
      *fence_fd = -1;
      return;
    }
  }

  bool is_signal = FenceIsSignal(*fence_fd);
  bool fence_ok = FdIsValid(*fence_fd);

  // If the fd has already signaled, there is no need to send the fd to the
  // producer, as the data has already been consumed.
  if (fence_ok && is_signal) {
    close(*fence_fd);
    *fence_fd = -1;
  }

  return;
}

impeller::vk::UniqueSemaphore OHOSExternalTextureVulkan::CreateVkSemaphore(
    int fence_fd) {
  impeller::vk::SemaphoreCreateInfo semaphore_info;
  impeller::vk::Device device = impeller_context_->GetDevice();
  auto semaphore_result = device.createSemaphoreUnique(semaphore_info, nullptr);
  if (semaphore_result.result != impeller::vk::Result::eSuccess) {
    FML_LOG(ERROR) << "vkCreateSemaphore failed";
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
    if (FdIsValid(fence_fd)) {
      close(fence_fd);
    }
    return impeller::vk::UniqueSemaphore();
  }
  return semaphore;
}

void OHOSExternalTextureVulkan::WaitGPUFence(int fence_fd) {
  auto texture = vk_resources_[now_key_].texture;
  impeller::vk::SubmitInfo submit_info;
  impeller::BarrierVK barrier;
  std::shared_ptr<impeller::CommandBuffer> cmd_buffer =
      impeller_context_->CreateCommandBuffer();

  std::function<void()> fence_callback = [fence_fd, cmd_buffer]() {
    if (FdIsValid(fence_fd)) {
      close(fence_fd);
    }
  };
  // auto close fd when error return.
  fml::ScopedCleanupClosure auto_close(fence_callback);

  auto [fence_result, complete_fence] =
      impeller_context_->GetDevice().createFenceUnique({});
  if (fence_result != impeller::vk::Result::eSuccess) {
    FML_LOG(ERROR) << "Failed to create fence: "
                   << impeller::vk::to_string(fence_result);
    return;
  }

  if (texture) {
    // Transition the layout to shader read.
    impeller::CommandBufferVK& buffer_vk =
        impeller::CommandBufferVK::Cast(*cmd_buffer);
    auto encoder = buffer_vk.GetEncoder();

    barrier.cmd_buffer = encoder->GetCommandBuffer();
    barrier.src_access = impeller::vk::AccessFlagBits::eColorAttachmentWrite |
                         impeller::vk::AccessFlagBits::eTransferWrite;
    barrier.src_stage =
        impeller::vk::PipelineStageFlagBits::eColorAttachmentOutput |
        impeller::vk::PipelineStageFlagBits::eTransfer;
    barrier.dst_access = impeller::vk::AccessFlagBits::eShaderRead;
    barrier.dst_stage = impeller::vk::PipelineStageFlagBits::eFragmentShader;

    barrier.new_layout = impeller::vk::ImageLayout::eShaderReadOnlyOptimal;

    if (!texture->SetLayout(barrier)) {
      FML_LOG(ERROR) << "External texture SetLayout failed";
      return;
    }

    if (!encoder->EndCommandBuffer()) {
      FML_LOG(ERROR) << "Failed to end command buffer";
      return;
    }

    submit_info.setCommandBuffers(barrier.cmd_buffer);
  }

  if (fence_fd > 0 && FdIsValid(fence_fd)) {
    // If the fence_fd is already signaled, it means the related data has
    // already been produced, so there's no need to import it into Vulkan.
    if (!FenceIsSignal(fence_fd)) {
      // The ownership of the fence_fd will be transferred to Vulkan. Once the
      // corresponding wait signal is received, Vulkan will automatically
      // release this fd. However, if this semaphore is not correctly submitted
      // to the GraphicQueue, the fence_fd will not be triggered and will need
      // to be released manually.
      auto wait_semaphore = CreateVkSemaphore(fence_fd);
      if (wait_semaphore.get() != VK_NULL_HANDLE) {
        // We use shared_ptr to ensure that the semaphore can only be destroyed
        // after Vulkan has finished using it (i.e., after the wait is
        // complete).
        auto shared_wait_semaphore =
            std::make_shared<impeller::vk::UniqueSemaphore>(
                std::move(wait_semaphore));
        impeller::vk::PipelineStageFlags wait_stage =
            impeller::vk::PipelineStageFlagBits::eAllCommands;
        submit_info.setWaitSemaphores(shared_wait_semaphore->get());
        submit_info.setWaitDstStageMask(wait_stage);
        fence_callback = [shared_wait_semaphore, cmd_buffer]() {
          // This lambda function will hold the semaphore and cmd_buffer until
          // the signal is triggered, ensuring that the semaphore is destroyed
          // only after Vulkan has finished using it. It is called on the
          // fence-wait thread.
        };
      }
    }
  }

  if (submit_info.waitSemaphoreCount == 0 &&
      submit_info.commandBufferCount == 0) {
    FML_LOG(ERROR) << "Texture is null with fence fd " << fence_fd
                   << " :no need submit";
    return;
  }

  auto status = impeller_context_->GetGraphicsQueue()->Submit(submit_info,
                                                              *complete_fence);
  if (status != impeller::vk::Result::eSuccess) {
    FML_LOG(ERROR) << "Failed to submit queue: "
                   << impeller::vk::to_string(status);
    return;
  }

  auto added_fence = impeller_context_->GetFenceWaiter()->AddFence(
      std::move(complete_fence), fence_callback);
  if (!added_fence) {
    // only happen when the FenceWaiter thread is terminated.
    FML_LOG(ERROR) << "failed to add Fence";
    return;
  }

  auto_close.Release();
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
