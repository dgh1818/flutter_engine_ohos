#include "ohos_external_texture_vulkan.h"

#include "flutter/impeller/core/formats.h"
#include "flutter/impeller/core/texture_descriptor.h"
#include "flutter/impeller/display_list/dl_image_impeller.h"

#include "flutter/impeller/renderer/backend/vulkan/command_buffer_vk.h"
#include "flutter/impeller/renderer/backend/vulkan/command_encoder_vk.h"
#include "flutter/impeller/renderer/backend/vulkan/ohos/ohb_texture_source_vk.h"
#include "flutter/impeller/renderer/backend/vulkan/texture_vk.h"

namespace flutter {

OHOSExternalTextureVulkan::OHOSExternalTextureVulkan(
    const std::shared_ptr<impeller::ContextVK>& impeller_context,
    int64_t id,
    OH_OnFrameAvailableListener listener)
    : OHOSExternalTexture(id, listener), impeller_context_(impeller_context) {}

OHOSExternalTextureVulkan::~OHOSExternalTextureVulkan() {}

void OHOSExternalTextureVulkan::SetGPUFence(int* fence_fd) {
  *fence_fd = -1;
}

void OHOSExternalTextureVulkan::WaitGPUFence(int fence_fd) {
  close(fence_fd);
}

void OHOSExternalTextureVulkan::GPUResourceDestroy() {}

sk_sp<flutter::DlImage> OHOSExternalTextureVulkan::CreateDlImage(
    PaintContext& context,
    const SkRect& bounds,
    NativeBufferKey key,
    OHNativeWindowBuffer* nw_buffer) {
  FML_LOG(ERROR) << " OHOSExternalTexture::CreateDlImage";
  auto texture_source = std::make_shared<impeller::OHBTextureSourceVK>(
      impeller_context_, nw_buffer);
  if (!texture_source->IsValid()) {
    FML_LOG(INFO) << " OHOSExternalTexture::CreateDlImage is null";
    return nullptr;
  }

  auto texture =
      std::make_shared<impeller::TextureVK>(impeller_context_, texture_source);
  // Transition the layout to shader read.
  {
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
      return nullptr;
    }
    if (!impeller_context_->GetCommandQueue()->Submit({buffer}).ok()) {
      return nullptr;
    }
  }

  auto dl_image = impeller::DlImageImpeller::Make(texture);

  image_lru_.AddImage(dl_image, key);
  return dl_image;
}

}  // namespace flutter
