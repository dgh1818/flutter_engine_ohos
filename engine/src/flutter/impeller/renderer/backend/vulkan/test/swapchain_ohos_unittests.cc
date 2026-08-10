/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

// OHOS-only assertions for the __OHOS__ branches in the Vulkan swapchain:
//  - SwapchainTransientsVK creates the MSAA and depth-stencil transients with
//    SampleCount::kCount2 instead of kCount4 (swapchain_transients_vk.cc).
//  - KHRSwapchainImplVK consumes the preload flag when creating the swapchain
//    (khr_swapchain_impl_vk.cc).
// Registered only in flutter_ohos_unittests; the guard keeps this file an
// empty translation unit on non-OHOS builds.
#include "flutter/fml/build_config.h"

#include "gtest/gtest.h"
#include "impeller/core/formats.h"
#include "impeller/core/texture.h"
#include "impeller/core/texture_descriptor.h"
#include "impeller/renderer/backend/vulkan/swapchain/khr/khr_swapchain_vk.h"
#include "impeller/renderer/backend/vulkan/swapchain/swapchain_transients_vk.h"
#include "impeller/renderer/backend/vulkan/test/mock_vulkan.h"

#if defined(FML_OS_OHOS)

namespace impeller {
namespace testing {

TEST(SwapchainOhosTest, MSAAUsesSampleCountTwoOnOhos) {
  auto const context = MockVulkanContextBuilder().Build();
  ASSERT_NE(context, nullptr);

  TextureDescriptor desc;
  desc.format = PixelFormat::kR8G8B8A8UNormInt;
  desc.size = ISize{100, 100};

  SwapchainTransientsVK transients(context, desc, /*enable_msaa=*/true);

  // On OHOS both transients are created with 2 samples instead of 4.
  const auto& msaa_texture = transients.GetMSAATexture();
  ASSERT_NE(msaa_texture, nullptr);
  EXPECT_EQ(msaa_texture->GetTextureDescriptor().sample_count,
            SampleCount::kCount2);

  const auto& depth_stencil_texture = transients.GetDepthStencilTexture();
  ASSERT_NE(depth_stencil_texture, nullptr);
  EXPECT_EQ(depth_stencil_texture->GetTextureDescriptor().sample_count,
            SampleCount::kCount2);
}

TEST(SwapchainOhosTest, PreloadResetAfterSwapchainCreate) {
  auto const context = MockVulkanContextBuilder().Build();
  ASSERT_NE(context, nullptr);
  context->SetIsPreload(true);

  vk::UniqueSurfaceKHR surface{};
  SetSwapchainImageSize(ISize{100, 100});
  auto swapchain =
      KHRSwapchainVK::Create(context, std::move(surface), ISize{100, 100},
                             /*enable_msaa=*/false);

  ASSERT_NE(swapchain, nullptr);
  ASSERT_TRUE(swapchain->IsValid());
  // The OHOS branch of KHRSwapchainImplVK::Create forces minImageCount to 2
  // while preloading and resets the flag on the context as a side effect.
  EXPECT_FALSE(context->IsPreload());
}

}  // namespace testing
}  // namespace impeller

#endif  // defined(FML_OS_OHOS)
