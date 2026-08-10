/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

// OHOS-only assertions for the FML_OS_OHOS color-space API of
// SolidColorContents (solid_color_contents.h:24/56, .cc:26). Registered
// only in flutter_ohos_unittests; the guard keeps this file an empty
// translation unit on non-OHOS builds.
#include "flutter/fml/build_config.h"

#include "impeller/entity/contents/solid_color_contents.h"
#include "impeller/entity/geometry/geometry.h"

#include "gtest/gtest.h"

#if defined(FML_OS_OHOS)

namespace impeller {
namespace testing {

TEST(SolidColorContentsOhosTest, ColorSpaceDefaultsToSRGB) {
  auto geom = Geometry::MakeCover();
  SolidColorContents contents(geom.get());
  EXPECT_EQ(contents.GetSourceColorSpace(), ColorSpace::kSRGB);
  EXPECT_EQ(contents.GetTargetColorSpace(), ColorSpace::kSRGB);
}

TEST(SolidColorContentsOhosTest, SetColorWithSpaceUpdatesSourceSpace) {
  auto geom = Geometry::MakeCover();
  SolidColorContents contents(geom.get());
  contents.SetColorWithSpace(Color::Red(), ColorSpace::kDisplayP3);

  // Color is stored, source color space follows the argument, and the
  // target color space is left at its default.
  EXPECT_EQ(contents.GetColor(), Color::Red());
  EXPECT_EQ(contents.GetSourceColorSpace(), ColorSpace::kDisplayP3);
  EXPECT_EQ(contents.GetTargetColorSpace(), ColorSpace::kSRGB);
}

TEST(SolidColorContentsOhosTest, ColorSpaceSettersRoundTrip) {
  auto geom = Geometry::MakeCover();
  SolidColorContents contents(geom.get());
  contents.SetSourceColorSpace(ColorSpace::kExtendedSRGB);
  EXPECT_EQ(contents.GetSourceColorSpace(), ColorSpace::kExtendedSRGB);

  contents.SetTargetColorSpace(ColorSpace::kDisplayP3);
  EXPECT_EQ(contents.GetTargetColorSpace(), ColorSpace::kDisplayP3);
}

}  // namespace testing
}  // namespace impeller

#endif  // FML_OS_OHOS
