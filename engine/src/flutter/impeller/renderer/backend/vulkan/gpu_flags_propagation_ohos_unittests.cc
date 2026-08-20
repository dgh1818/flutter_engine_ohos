/*
 * Copyright 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/fml/build_config.h"

#if defined(FML_OS_OHOS)

#include "gtest/gtest.h"
#include "impeller/base/flags.h"
#include "impeller/renderer/backend/vulkan/test/mock_vulkan.h"
#include "impeller/typographer/backends/skia/typographer_context_skia.h"

namespace impeller {
namespace testing {

TEST(GPUFlagsPropagationOhosTest, TypographerContextCreatedWithDifferentFlags) {
  Flags flags_disabled;
  flags_disabled.glyph_raster_parallelization = false;
  auto typographer_disabled = TypographerContextSkia::Make(flags_disabled);
  EXPECT_NE(typographer_disabled, nullptr);

  Flags flags_enabled;
  flags_enabled.glyph_raster_parallelization = true;
  auto typographer_enabled = TypographerContextSkia::Make(flags_enabled);
  EXPECT_NE(typographer_enabled, nullptr);
}

TEST(GPUFlagsPropagationOhosTest, MockContextAndTypographerCoexist) {
  auto const vk_context = MockVulkanContextBuilder().Build();
  ASSERT_NE(vk_context, nullptr);

  Flags flags;
  flags.glyph_raster_parallelization = true;
  auto typographer = TypographerContextSkia::Make(flags);
  ASSERT_NE(typographer, nullptr);

  EXPECT_NE(vk_context, nullptr);
  EXPECT_NE(typographer, nullptr);
}

}  // namespace testing
}  // namespace impeller

#endif  // defined(FML_OS_OHOS)
