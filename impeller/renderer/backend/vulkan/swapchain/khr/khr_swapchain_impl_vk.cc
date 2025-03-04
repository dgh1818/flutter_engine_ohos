// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/renderer/backend/vulkan/swapchain/khr/khr_swapchain_impl_vk.h"

#include "fml/synchronization/semaphore.h"
#include "impeller/base/validation.h"
#include "impeller/core/formats.h"
#include "impeller/renderer/backend/vulkan/command_buffer_vk.h"
#include "impeller/renderer/backend/vulkan/command_encoder_vk.h"
#include "impeller/renderer/backend/vulkan/context_vk.h"
#include "impeller/renderer/backend/vulkan/formats_vk.h"
#include "impeller/renderer/backend/vulkan/gpu_tracer_vk.h"
#include "impeller/renderer/backend/vulkan/swapchain/khr/khr_surface_vk.h"
#include "impeller/renderer/backend/vulkan/swapchain/khr/khr_swapchain_image_vk.h"
#include "impeller/renderer/context.h"
#include "vulkan/vulkan_structs.hpp"

namespace impeller {

static constexpr size_t kMaxFramesInFlight = 3u;

// Number of frames to poll for orientation changes. For example `1u` means
// that the orientation will be polled every frame, while `2u` means that the
// orientation will be polled every other frame.
static constexpr size_t kPollFramesForOrientation = 1u;

struct KHRFrameSynchronizerVK {
  vk::UniqueFence acquire;
  vk::UniqueSemaphore render_ready;
  vk::UniqueSemaphore present_ready;
  std::shared_ptr<CommandBuffer> final_cmd_buffer;
  std::shared_ptr<CommandBuffer> ready_cmd_buffer;
  bool is_valid = false;

  explicit KHRFrameSynchronizerVK(const vk::Device& device) {
    auto acquire_res = device.createFenceUnique(
        vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled});
    auto render_res = device.createSemaphoreUnique({});
    auto present_res = device.createSemaphoreUnique({});
    if (acquire_res.result != vk::Result::eSuccess ||
        render_res.result != vk::Result::eSuccess ||
        present_res.result != vk::Result::eSuccess) {
      VALIDATION_LOG << "Could not create synchronizer.";
      return;
    }
    acquire = std::move(acquire_res.value);
    render_ready = std::move(render_res.value);
    present_ready = std::move(present_res.value);
    is_valid = true;
  }

  ~KHRFrameSynchronizerVK() = default;

  bool WaitForFence(const vk::Device& device) {
    if (auto result = device.waitForFences(
            *acquire,                             // fence
            true,                                 // wait all
            std::numeric_limits<uint64_t>::max()  // timeout (ns)
        );
        result != vk::Result::eSuccess) {
      VALIDATION_LOG << "Fence wait failed: " << vk::to_string(result);
      return false;
    }
    if (auto result = device.resetFences(*acquire);
        result != vk::Result::eSuccess) {
      VALIDATION_LOG << "Could not reset fence: " << vk::to_string(result);
      return false;
    }
    return true;
  }
};

static bool ContainsFormat(const std::vector<vk::SurfaceFormatKHR>& formats,
                           vk::SurfaceFormatKHR format) {
  return std::find(formats.begin(), formats.end(), format) != formats.end();
}

static std::optional<vk::SurfaceFormatKHR> ChooseSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR>& formats,
    PixelFormat preference) {
  if (impeller::Context::hdr_ == 2) {  // video PQ
    const auto colorspace = vk::ColorSpaceKHR::eHdr10St2084EXT;
    const auto vk_preference =
        vk::SurfaceFormatKHR{vk::Format::eA2B10G10R10UnormPack32, colorspace};
    FML_DLOG(WARNING) << "enter eHdr10St2084EXT!";
    return vk_preference;
  }

  if (ToVKImageFormat(preference) == vk::Format::eA2B10G10R10UnormPack32) {
    const auto colorspace = vk::ColorSpaceKHR::eHdr10HlgEXT;
    const auto vk_preference =
        vk::SurfaceFormatKHR{vk::Format::eA2B10G10R10UnormPack32, colorspace};
    FML_DLOG(WARNING) << "enter eHdr10HlgEXT!";
    return vk_preference;
  }

  const auto colorspace = vk::ColorSpaceKHR::eSrgbNonlinear;
  const auto vk_preference =
      vk::SurfaceFormatKHR{ToVKImageFormat(preference), colorspace};
  if (ContainsFormat(formats, vk_preference)) {
    return vk_preference;
  }

  std::vector<vk::SurfaceFormatKHR> options = {
      {vk::Format::eB8G8R8A8Unorm, colorspace},
      {vk::Format::eR8G8B8A8Unorm, colorspace}};
  for (const auto& format : options) {
    if (ContainsFormat(formats, format)) {
      return format;
    }
  }

  return std::nullopt;
}

static std::optional<vk::CompositeAlphaFlagBitsKHR> ChooseAlphaCompositionMode(
    vk::CompositeAlphaFlagsKHR flags) {
  if (flags & vk::CompositeAlphaFlagBitsKHR::eInherit) {
    return vk::CompositeAlphaFlagBitsKHR::eInherit;
  }
  if (flags & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied) {
    return vk::CompositeAlphaFlagBitsKHR::ePreMultiplied;
  }
  if (flags & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied) {
    return vk::CompositeAlphaFlagBitsKHR::ePostMultiplied;
  }
  if (flags & vk::CompositeAlphaFlagBitsKHR::eOpaque) {
    return vk::CompositeAlphaFlagBitsKHR::eOpaque;
  }

  return std::nullopt;
}

std::shared_ptr<KHRSwapchainImplVK> KHRSwapchainImplVK::Create(
    const std::shared_ptr<Context>& context,
    vk::UniqueSurfaceKHR surface,
    const ISize& size,
    bool enable_msaa,
    vk::SwapchainKHR old_swapchain) {
  return std::shared_ptr<KHRSwapchainImplVK>(new KHRSwapchainImplVK(
      context, std::move(surface), size, enable_msaa, old_swapchain));
}

KHRSwapchainImplVK::KHRSwapchainImplVK(const std::shared_ptr<Context>& context,
                                       vk::UniqueSurfaceKHR surface,
                                       const ISize& size,
                                       bool enable_msaa,
                                       vk::SwapchainKHR old_swapchain) {
  if (!context) {
    VALIDATION_LOG << "Cannot create a swapchain without a context.";
    return;
  }

  auto& vk_context = ContextVK::Cast(*context);

  const auto [caps_result, surface_caps] =
      vk_context.GetPhysicalDevice().getSurfaceCapabilitiesKHR(*surface);
  if (caps_result != vk::Result::eSuccess) {
    VALIDATION_LOG << "Could not get surface capabilities: "
                   << vk::to_string(caps_result);
    return;
  }

  auto [formats_result, formats] =
      vk_context.GetPhysicalDevice().getSurfaceFormatsKHR(*surface);
  if (formats_result != vk::Result::eSuccess) {
    VALIDATION_LOG << "Could not get surface formats: "
                   << vk::to_string(formats_result);
    return;
  }

  auto format = ChooseSurfaceFormat(
      formats, vk_context.GetCapabilities()->GetDefaultColorFormat());
  hdr_ = vk_context.GetContextHdr();

  if (vk_context.GetContextHdr() > 0) {
    format = ChooseSurfaceFormat(formats, PixelFormat::kR10G10B10A2);
    FML_DLOG(INFO) << "impl choose hdr";
  } else {
    format = ChooseSurfaceFormat(formats, PixelFormat::kR8G8B8A8UNormInt);
    FML_DLOG(INFO) << "impl not choose hdr";
  }

  if (!format.has_value()) {
    VALIDATION_LOG << "Swapchain has no supported formats.";
    return;
  }
  vk_context.SetOffscreenFormat(ToPixelFormat(format.value().format));

  const auto composite =
      ChooseAlphaCompositionMode(surface_caps.supportedCompositeAlpha);
  if (!composite.has_value()) {
    VALIDATION_LOG << "No composition mode supported.";
    return;
  }

  vk::SwapchainCreateInfoKHR swapchain_info;
  swapchain_info.surface = *surface;
  swapchain_info.imageFormat = format.value().format;
  swapchain_info.imageColorSpace = format.value().colorSpace;
  swapchain_info.presentMode = vk::PresentModeKHR::eFifo;
  swapchain_info.imageExtent = vk::Extent2D{
      std::clamp(static_cast<uint32_t>(size.width),
                 surface_caps.minImageExtent.width,
                 surface_caps.maxImageExtent.width),
      std::clamp(static_cast<uint32_t>(size.height),
                 surface_caps.minImageExtent.height,
                 surface_caps.maxImageExtent.height),
  };
  swapchain_info.minImageCount =
      std::clamp(surface_caps.minImageCount + 1u,  // preferred image count
                 surface_caps.minImageCount,       // min count cannot be zero
                 surface_caps.maxImageCount == 0u
                     ? surface_caps.minImageCount + 1u
                     : surface_caps.maxImageCount  // max zero means no limit
      );
  swapchain_info.imageArrayLayers = 1u;
  // Swapchain images are primarily used as color attachments (via resolve),
  // blit targets, or input attachments.
  swapchain_info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment |
                              vk::ImageUsageFlagBits::eTransferDst |
                              vk::ImageUsageFlagBits::eInputAttachment;
  swapchain_info.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
  swapchain_info.compositeAlpha = composite.value();
  // If we set the clipped value to true, Vulkan expects we will never read back
  // from the buffer. This is analogous to [CAMetalLayer framebufferOnly] in
  // Metal.
  swapchain_info.clipped = true;
  // Setting queue family indices is irrelevant since the present mode is
  // exclusive.
  swapchain_info.imageSharingMode = vk::SharingMode::eExclusive;
  swapchain_info.oldSwapchain = old_swapchain;

  auto [swapchain_result, swapchain] =
      vk_context.GetDevice().createSwapchainKHRUnique(swapchain_info);
  if (swapchain_result != vk::Result::eSuccess) {
    VALIDATION_LOG << "Could not create swapchain: "
                   << vk::to_string(swapchain_result);
    return;
  }

  auto [images_result, images] =
      vk_context.GetDevice().getSwapchainImagesKHR(*swapchain);
  if (images_result != vk::Result::eSuccess) {
    VALIDATION_LOG << "Could not get swapchain images.";
    return;
  }

  TextureDescriptor texture_desc;
  texture_desc.usage = TextureUsage::kRenderTarget;
  texture_desc.storage_mode = StorageMode::kDevicePrivate;
  texture_desc.format = ToPixelFormat(swapchain_info.imageFormat);
  texture_desc.size = ISize::MakeWH(swapchain_info.imageExtent.width,
                                    swapchain_info.imageExtent.height);

  // Allocate a single onscreen MSAA texture and Depth+Stencil Texture to
  // be shared by all swapchain images.
  TextureDescriptor msaa_desc;
  msaa_desc.storage_mode = StorageMode::kDeviceTransient;
  msaa_desc.type = TextureType::kTexture2DMultisample;
#ifdef OHOS_PLATFORM
  msaa_desc.sample_count = SampleCount::kCount2;
#else
  msaa_desc.sample_count = SampleCount::kCount4;
#endif
  msaa_desc.format = texture_desc.format;
  msaa_desc.size = texture_desc.size;
  msaa_desc.usage = TextureUsage::kRenderTarget;

  // The depth+stencil configuration matches the configuration used by
  // RenderTarget::SetupDepthStencilAttachments and matching the swapchain
  // image dimensions and sample count.
  TextureDescriptor depth_stencil_desc;
  depth_stencil_desc.storage_mode = StorageMode::kDeviceTransient;
  if (enable_msaa) {
    depth_stencil_desc.type = TextureType::kTexture2DMultisample;
#ifdef OHOS_PLATFORM
    depth_stencil_desc.sample_count = SampleCount::kCount2;
#else
    depth_stencil_desc.sample_count = SampleCount::kCount4;
#endif
  } else {
    depth_stencil_desc.type = TextureType::kTexture2D;
    depth_stencil_desc.sample_count = SampleCount::kCount1;
  }
  depth_stencil_desc.format =
      context->GetCapabilities()->GetDefaultDepthStencilFormat();
  depth_stencil_desc.size = texture_desc.size;
  depth_stencil_desc.usage = TextureUsage::kRenderTarget;

  std::shared_ptr<Texture> msaa_texture;
  if (enable_msaa) {
    msaa_texture = context->GetResourceAllocator()->CreateTexture(msaa_desc);
  }
  std::shared_ptr<Texture> depth_stencil_texture =
      context->GetResourceAllocator()->CreateTexture(depth_stencil_desc);

  std::vector<std::shared_ptr<KHRSwapchainImageVK>> swapchain_images;
  for (const auto& image : images) {
    auto swapchain_image = std::make_shared<KHRSwapchainImageVK>(
        texture_desc,            // texture descriptor
        vk_context.GetDevice(),  // device
        image                    // image
    );
    if (!swapchain_image->IsValid()) {
      VALIDATION_LOG << "Could not create swapchain image.";
      return;
    }
    swapchain_image->SetMSAATexture(msaa_texture);
    swapchain_image->SetDepthStencilTexture(depth_stencil_texture);

    ContextVK::SetDebugName(
        vk_context.GetDevice(), swapchain_image->GetImage(),
        "SwapchainImage" + std::to_string(swapchain_images.size()));
    ContextVK::SetDebugName(
        vk_context.GetDevice(), swapchain_image->GetImageView(),
        "SwapchainImageView" + std::to_string(swapchain_images.size()));

    swapchain_images.emplace_back(swapchain_image);
  }

  std::vector<std::unique_ptr<KHRFrameSynchronizerVK>> synchronizers;
  for (size_t i = 0u; i < kMaxFramesInFlight; i++) {
    auto sync =
        std::make_unique<KHRFrameSynchronizerVK>(vk_context.GetDevice());
    if (!sync->is_valid) {
      VALIDATION_LOG << "Could not create frame synchronizers.";
      return;
    }
    synchronizers.emplace_back(std::move(sync));
  }
  FML_DCHECK(!synchronizers.empty());

  context_ = context;
  surface_ = std::move(surface);
  surface_format_ = swapchain_info.imageFormat;
  swapchain_ = std::move(swapchain);
  images_ = std::move(swapchain_images);
  synchronizers_ = std::move(synchronizers);
  current_frame_ = synchronizers_.size() - 1u;
  size_ = size;
  enable_msaa_ = enable_msaa;
  is_valid_ = true;
}

KHRSwapchainImplVK::~KHRSwapchainImplVK() {
  DestroySwapchain();
}

const ISize& KHRSwapchainImplVK::GetSize() const {
  return size_;
}

int KHRSwapchainImplVK::GetHdr() const {
  return hdr_;
}

void KHRSwapchainImplVK::SetHdr(int hdr) {
  hdr_ = hdr;
}

bool KHRSwapchainImplVK::IsValid() const {
  return is_valid_;
}

void KHRSwapchainImplVK::WaitIdle() const {
  if (auto context = context_.lock()) {
    // vkDeviceWaitIdle is equivalent to calling vkQueueWaitIdle on all queues.
    ContextVK::Cast(*context).WaitIdle();
  }
}

std::pair<vk::UniqueSurfaceKHR, vk::UniqueSwapchainKHR>
KHRSwapchainImplVK::DestroySwapchain() {
  WaitIdle();
  is_valid_ = false;
  synchronizers_.clear();
  images_.clear();
  context_.reset();
  return {std::move(surface_), std::move(swapchain_)};
}

vk::Format KHRSwapchainImplVK::GetSurfaceFormat() const {
  return surface_format_;
}

std::shared_ptr<Context> KHRSwapchainImplVK::GetContext() const {
  return context_.lock();
}

KHRSwapchainImplVK::AcquireResult KHRSwapchainImplVK::AcquireNextDrawable() {
  auto context_strong = context_.lock();
  if (!context_strong) {
    return KHRSwapchainImplVK::AcquireResult{};
  }

  const auto& context = ContextVK::Cast(*context_strong);

  current_frame_ = (current_frame_ + 1u) % synchronizers_.size();

  const auto& sync = synchronizers_[current_frame_];

  //----------------------------------------------------------------------------
  /// Wait on the host for the synchronizer fence.
  ///
  if (!sync->WaitForFence(context.GetDevice())) {
    VALIDATION_LOG << "Could not wait for fence.";
    return KHRSwapchainImplVK::AcquireResult{};
  }

  //----------------------------------------------------------------------------
  /// Get the next image index.
  ///
  auto [acq_result, index] = context.GetDevice().acquireNextImageKHR(
      *swapchain_,          // swapchain
      1'000'000'000,        // timeout (ns) 1000ms
      *sync->render_ready,  // signal semaphore
      nullptr               // fence
  );

  switch (acq_result) {
    case vk::Result::eSuccess:
      // Keep going.
      break;
    case vk::Result::eSuboptimalKHR:
    case vk::Result::eErrorOutOfDateKHR:
      // A recoverable error. Just say we are out of date.
      return AcquireResult{true /* out of date */};
      break;
    default:
      // An unrecoverable error.
      VALIDATION_LOG << "Could not acquire next swapchain image: "
                     << vk::to_string(acq_result);
      return AcquireResult{false /* out of date */};
  }

  if (index >= images_.size()) {
    VALIDATION_LOG << "Swapchain returned an invalid image index.";
    return KHRSwapchainImplVK::AcquireResult{};
  }

  /// Record all subsequent cmd buffers as part of the current frame.
  context.GetGPUTracer()->MarkFrameStart();

  auto image = images_[index % images_.size()];

  /// The GPU's write operations to the image must wait for the
  /// sync->render_ready semaphore (i.e., wait for the GPU or hardware composer
  /// to complete reading the image);
  /// otherwise, screen tearing or other visual artifacts may occur.
  /// However, the current function does not provide the render_ready semaphore
  /// upon return, meaning subsequent write operations to the image will not
  /// wait for the semaphore to signal, potentially leading to visual anomalies.

  /// To address this issue, a write barrier is added here for the image,
  /// along with a wait for the corresponding semaphore,
  /// ensuring correct rendering.
  /// Note: vkWaitSemaphores might not function correctly when the semaphore is
  /// imported from a sync FD.
  sync->ready_cmd_buffer = context.CreateCommandBuffer();
  if (sync->ready_cmd_buffer) {
    auto vk_cmd_buffer = CommandBufferVK::Cast(*sync->ready_cmd_buffer)
                             .GetEncoder()
                             ->GetCommandBuffer();
    BarrierVK barrier;
    barrier.new_layout = vk::ImageLayout::eColorAttachmentOptimal;
    barrier.cmd_buffer = vk_cmd_buffer;
    barrier.src_access = {};
    barrier.src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
    barrier.dst_access = vk::AccessFlagBits::eColorAttachmentWrite;
    barrier.dst_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    image->SetLayout(barrier);

    auto end_ret = vk_cmd_buffer.end();
    if (end_ret == vk::Result::eSuccess) {
      vk::SubmitInfo submit_info;
      vk::PipelineStageFlags wait_stage =
          vk::PipelineStageFlagBits::eColorAttachmentOutput;
      submit_info.setWaitDstStageMask(wait_stage);
      submit_info.setWaitSemaphores(*sync->render_ready);
      submit_info.setCommandBuffers(vk_cmd_buffer);
      auto result = context.GetGraphicsQueue()->Submit(submit_info, nullptr);
      if (result != vk::Result::eSuccess) {
        VALIDATION_LOG << "Submit Swapchain Image Write Barrier Failed: "
                       << vk::to_string(result);
      }
    } else {
      VALIDATION_LOG << "Command Buffer End Failed: " << vk::to_string(end_ret);
    }
  } else {
    VALIDATION_LOG << "Create Command Buffer Failed";
  }

  uint32_t image_index = index;
  return AcquireResult{KHRSurfaceVK::WrapSwapchainImage(
      context_strong,  // context
      image,           // swapchain image
      [weak_swapchain = weak_from_this(), image, image_index]() -> bool {
        auto swapchain = weak_swapchain.lock();
        if (!swapchain) {
          return false;
        }
        return swapchain->Present(image, image_index);
      },            // swap callback
      enable_msaa_  //
      )};
}

bool KHRSwapchainImplVK::Present(
    const std::shared_ptr<KHRSwapchainImageVK>& image,
    uint32_t index) {
  auto context_strong = context_.lock();
  if (!context_strong) {
    return false;
  }

  const auto& context = ContextVK::Cast(*context_strong);
  const auto& sync = synchronizers_[current_frame_];
  context.GetGPUTracer()->MarkFrameEnd();

  //----------------------------------------------------------------------------
  /// Transition the image to color-attachment-optimal.
  ///
  sync->final_cmd_buffer = context.CreateCommandBuffer();
  if (!sync->final_cmd_buffer) {
    return false;
  }

  auto vk_final_cmd_buffer = CommandBufferVK::Cast(*sync->final_cmd_buffer)
                                 .GetEncoder()
                                 ->GetCommandBuffer();
  {
    BarrierVK barrier;
    barrier.new_layout = vk::ImageLayout::ePresentSrcKHR;
    barrier.cmd_buffer = vk_final_cmd_buffer;
    barrier.src_access = vk::AccessFlagBits::eColorAttachmentWrite;
    barrier.src_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    barrier.dst_access = {};
    barrier.dst_stage = vk::PipelineStageFlagBits::eBottomOfPipe;

    if (!image->SetLayout(barrier).ok()) {
      return false;
    }

    if (vk_final_cmd_buffer.end() != vk::Result::eSuccess) {
      return false;
    }
  }

  //----------------------------------------------------------------------------
  /// Signal that the presentation semaphore is ready.
  ///
  {
    vk::SubmitInfo submit_info;
    vk::PipelineStageFlags wait_stage =
        vk::PipelineStageFlagBits::eColorAttachmentOutput;
    submit_info.setWaitDstStageMask(wait_stage);
    submit_info.setWaitSemaphores(*sync->render_ready);
    submit_info.setSignalSemaphores(*sync->present_ready);
    submit_info.setCommandBuffers(vk_final_cmd_buffer);
    auto result =
        context.GetGraphicsQueue()->Submit(submit_info, *sync->acquire);
    if (result != vk::Result::eSuccess) {
      VALIDATION_LOG << "Could not wait on render semaphore: "
                     << vk::to_string(result);
      return false;
    }
  }

  //----------------------------------------------------------------------------
  /// Present the image.
  ///
  uint32_t indices[] = {static_cast<uint32_t>(index)};

  vk::PresentInfoKHR present_info;
  present_info.setSwapchains(*swapchain_);
  present_info.setImageIndices(indices);
  present_info.setWaitSemaphores(*sync->present_ready);

  auto result = context.GetGraphicsQueue()->Present(present_info);

  switch (result) {
    case vk::Result::eErrorOutOfDateKHR:
      // Caller will recreate the impl on acquisition, not submission.
      [[fallthrough]];
    case vk::Result::eErrorSurfaceLostKHR:
      // Vulkan guarantees that the set of queue operations will still
      // complete successfully.
      [[fallthrough]];
    case vk::Result::eSuboptimalKHR:
      // Even though we're handling rotation changes via polling, we
      // still need to handle the case where the swapchain signals that
      // it's suboptimal (i.e. every frame when we are rotated given we
      // aren't doing Vulkan pre-rotation).
      [[fallthrough]];
    case vk::Result::eSuccess:
      break;
    default:
      VALIDATION_LOG << "Could not present queue: " << vk::to_string(result);
      break;
  }

  return true;
}

}  // namespace impeller
