/*
 * Copyright 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/shell/platform/ohos/context/ohos_context.h"

#include "gtest/gtest.h"
#include "impeller/base/flags.h"

namespace flutter {
namespace testing {

TEST(OHOSContextTest, DefaultImpellerFlagsAreDefaultConstructed) {
  OHOSContext context(OHOSRenderingAPI::kSoftware);
  const auto& flags = context.GetImpellerFlags();
  EXPECT_FALSE(flags.glyph_raster_parallelization);
  EXPECT_FALSE(flags.antialiased_lines);
}

TEST(OHOSContextTest, SetAndGetImpellerFlags) {
  OHOSContext context(OHOSRenderingAPI::kSoftware);

  impeller::Flags flags;
  flags.glyph_raster_parallelization = true;
  context.SetImpellerFlags(flags);

  const auto& retrieved = context.GetImpellerFlags();
  EXPECT_TRUE(retrieved.glyph_raster_parallelization);
}

TEST(OHOSContextTest, SetImpellerFlagsOverwritesPrevious) {
  OHOSContext context(OHOSRenderingAPI::kSoftware);

  impeller::Flags flags1;
  flags1.glyph_raster_parallelization = true;
  context.SetImpellerFlags(flags1);
  EXPECT_TRUE(context.GetImpellerFlags().glyph_raster_parallelization);

  impeller::Flags flags2;
  flags2.glyph_raster_parallelization = false;
  context.SetImpellerFlags(flags2);
  EXPECT_FALSE(context.GetImpellerFlags().glyph_raster_parallelization);
}

TEST(OHOSContextTest, GetImpellerFlagsReturnsReference) {
  OHOSContext context(OHOSRenderingAPI::kSoftware);

  impeller::Flags flags;
  flags.glyph_raster_parallelization = true;
  context.SetImpellerFlags(flags);

  const impeller::Flags& ref1 = context.GetImpellerFlags();
  const impeller::Flags& ref2 = context.GetImpellerFlags();
  EXPECT_EQ(ref1.glyph_raster_parallelization,
            ref2.glyph_raster_parallelization);
}

TEST(OHOSContextTest, AllFlagsFieldsPreserved) {
  OHOSContext context(OHOSRenderingAPI::kSoftware);

  impeller::Flags flags;
  flags.glyph_raster_parallelization = true;
  flags.antialiased_lines = true;
  context.SetImpellerFlags(flags);

  const auto& retrieved = context.GetImpellerFlags();
  EXPECT_TRUE(retrieved.glyph_raster_parallelization);
  EXPECT_TRUE(retrieved.antialiased_lines);
}

}  // namespace testing
}  // namespace flutter
