// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/renderer/backend/vulkan/swapchain/khr/khr_swapchain_vk.h"

#include "flutter/fml/trace_event.h"
#include "impeller/base/validation.h"
#include "impeller/renderer/backend/vulkan/swapchain/khr/khr_swapchain_impl_vk.h"

namespace impeller {

std::shared_ptr<KHRSwapchainVK> KHRSwapchainVK::Create(
    const std::shared_ptr<Context>& context,
    vk::UniqueSurfaceKHR surface,
    const ISize& size,
    bool enable_msaa) {
  auto impl = KHRSwapchainImplVK::Create(context, std::move(surface), size,
                                         enable_msaa);
  if (!impl || !impl->IsValid()) {
    VALIDATION_LOG << "Failed to create SwapchainVK implementation.";
    return nullptr;
  }
  return std::shared_ptr<KHRSwapchainVK>(
      new KHRSwapchainVK(std::move(impl), size, enable_msaa));
}

KHRSwapchainVK::KHRSwapchainVK(std::shared_ptr<KHRSwapchainImplVK> impl,
                               const ISize& size,
                               bool enable_msaa)
    : impl_(std::move(impl)), size_(size), enable_msaa_(enable_msaa) {}

KHRSwapchainVK::~KHRSwapchainVK() = default;

bool KHRSwapchainVK::IsValid() const {
  return impl_ ? impl_->IsValid() : false;
}

void KHRSwapchainVK::UpdateSurfaceSize(const ISize& size) {
  // Update the size of the swapchain. On the next acquired drawable,
  // the sizes may no longer match, forcing the swapchain to be recreated.
  size_ = size;
}

// void KHRSwapchainVK::UpdateSurfaceHdr(int hdr) {
//   hdr_ = hdr;
// }

std::unique_ptr<Surface> KHRSwapchainVK::AcquireNextDrawable() {
  if (!IsValid()) {
    return nullptr;
  }

  TRACE_EVENT0("impeller", __FUNCTION__);

  hdr_ = impeller::Context::hdr_;
  auto result = impl_->AcquireNextDrawable();
  if (!result.out_of_date && size_ == impl_->GetSize() &&
      hdr_ == impl_->GetHdr()) {
    return std::move(result.surface);
  }

  FML_DLOG(INFO) << "SWAPCHAIN Recreate-" << "HDR is:" << hdr_;

  TRACE_EVENT0("impeller", "RecreateSwapchain");

  // This swapchain implementation indicates that it is out of date. Tear it
  // down and make a new one.
  auto context = impl_->GetContext();
  auto [surface, old_swapchain] = impl_->DestroySwapchain();

  auto new_impl = KHRSwapchainImplVK::Create(context,             //
                                             std::move(surface),  //
                                             size_,               //
                                             enable_msaa_,        //
                                             *old_swapchain       //
  );
  if (!new_impl || !new_impl->IsValid()) {
    VALIDATION_LOG << "Could not update swapchain.";
    // The old swapchain is dead because we took its surface. This is
    // unrecoverable.
    impl_.reset();
    return nullptr;
  }
  impl_ = std::move(new_impl);

  //----------------------------------------------------------------------------
  /// We managed to recreate the swapchain in the new configuration. Try again.
  ///
  return AcquireNextDrawable();
}

vk::Format KHRSwapchainVK::GetSurfaceFormat() const {
  return IsValid() ? impl_->GetSurfaceFormat() : vk::Format::eUndefined;
}

}  // namespace impeller
