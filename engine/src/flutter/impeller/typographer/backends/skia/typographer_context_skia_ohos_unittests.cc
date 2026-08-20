/*
 * Copyright 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/fml/build_config.h"

#include "gtest/gtest.h"
#include "impeller/base/flags.h"
#include "impeller/typographer/backends/skia/typographer_context_skia.h"

#if defined(FML_OS_OHOS)

namespace impeller {
namespace testing {

TEST(TypographerContextSkiaOhosTest, MakeWithDefaultFlagsReturnsValid) {
  auto context = TypographerContextSkia::Make();
  EXPECT_NE(context, nullptr);
}

TEST(TypographerContextSkiaOhosTest, MakeWithParallelizationDisabled) {
  Flags flags;
  flags.glyph_raster_parallelization = false;
  auto context = TypographerContextSkia::Make(flags);
  EXPECT_NE(context, nullptr);
}

TEST(TypographerContextSkiaOhosTest, MakeWithParallelizationEnabled) {
  Flags flags;
  flags.glyph_raster_parallelization = true;
  auto context = TypographerContextSkia::Make(flags);
  EXPECT_NE(context, nullptr);
}

TEST(TypographerContextSkiaOhosTest, MakeWithAllFlagsEnabled) {
  Flags flags;
  flags.glyph_raster_parallelization = true;
  flags.antialiased_lines = true;
  auto context = TypographerContextSkia::Make(flags);
  EXPECT_NE(context, nullptr);
}

TEST(TypographerContextSkiaOhosTest, MakeWithDefaultFlagsEqualsNoArg) {
  auto context_no_arg = TypographerContextSkia::Make();
  auto context_default = TypographerContextSkia::Make(Flags{});
  EXPECT_NE(context_no_arg, nullptr);
  EXPECT_NE(context_default, nullptr);
}

}  // namespace testing
}  // namespace impeller

#endif  // defined(FML_OS_OHOS)
