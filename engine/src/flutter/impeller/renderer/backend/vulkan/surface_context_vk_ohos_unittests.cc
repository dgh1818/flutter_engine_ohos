/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

// OHOS-only assertions for the FML_OS_OHOS target color space handling in
// SurfaceContextVK:
//  - The target color space defaults to sRGB (surface_context_vk.h).
//  - SetWindowSurface re-derives it from the swapchain surface format; a
//    non-wide-gamut format takes the else path and keeps sRGB
//    (surface_context_vk.cc).
//  - SetTargetColorSpace/GetTargetColorSpace round-trip.
// Registered only in flutter_ohos_unittests; the guard keeps this file an
// empty translation unit on non-OHOS builds.
#include "flutter/fml/build_config.h"

#include "gtest/gtest.h"
#include "impeller/renderer/backend/vulkan/surface_context_vk.h"
#include "impeller/renderer/backend/vulkan/test/mock_vulkan.h"

#if defined(FML_OS_OHOS)

namespace impeller {
namespace testing {

TEST(SurfaceContextOhosTest, DefaultTargetColorSpaceIsSRGB) {
  auto const context = MockVulkanContextBuilder().Build();
  ASSERT_NE(context, nullptr);

  SurfaceContextVK surface_context(context);

  EXPECT_EQ(surface_context.GetTargetColorSpace(), ColorSpace::kSRGB);
}

TEST(SurfaceContextOhosTest, WindowSurfaceKeepsSRGBForNonWideGamutFormat) {
  SetSwapchainImageSize({100, 100});
  std::shared_ptr<ContextVK> context = MockVulkanContextBuilder().Build();
  ASSERT_NE(context, nullptr);

  vk::UniqueSurfaceKHR surface{};
  SurfaceContextVK surface_context(context);

  ASSERT_TRUE(surface_context.SetWindowSurface(std::move(surface), {100, 100}));
  // The mock reports VK_FORMAT_R8G8B8A8_UNORM (not A2B10G10R10), so the OHOS
  // branch of SetWindowSurface takes the else path and keeps sRGB.
  EXPECT_EQ(surface_context.GetTargetColorSpace(), ColorSpace::kSRGB);

  surface_context.SetTargetColorSpace(ColorSpace::kDisplayP3);
  EXPECT_EQ(surface_context.GetTargetColorSpace(), ColorSpace::kDisplayP3);
}

}  // namespace testing
}  // namespace impeller

#endif  // defined(FML_OS_OHOS)
