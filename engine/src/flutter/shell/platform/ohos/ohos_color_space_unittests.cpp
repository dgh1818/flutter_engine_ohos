/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/lib/ui/painting/ohos_color_space.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "impeller/core/texture_descriptor.h"

namespace flutter {
namespace testing {

// The header under test is a pure inline header. On OHOS (FML_OS_OHOS defined)
// it maps OHOS color-space constants to impeller::TextureColorSpace values;
// on other platforms it always returns kSRGB. These tests run on the OHOS
// target build, so FML_OS_OHOS is defined and the mapping logic is exercised.

// ---------------------------------------------------------------------------
// OhosColorSpaceToTextureColorSpace - Display P3 family
// ---------------------------------------------------------------------------

// Each of the DisplayP3 family color spaces must map to kDisplayP3.
TEST(OhosColorSpaceTest, DisplayP3MapsToDisplayP3) {
  EXPECT_EQ(OhosColorSpaceToTextureColorSpace(
                ohos_color_space_internal::kDisplayP3),
            impeller::TextureColorSpace::kDisplayP3);
}

TEST(OhosColorSpaceTest, DisplayP3LimitMapsToDisplayP3) {
  EXPECT_EQ(OhosColorSpaceToTextureColorSpace(
                ohos_color_space_internal::kDisplayP3Limit),
            impeller::TextureColorSpace::kDisplayP3);
}

TEST(OhosColorSpaceTest, LinearP3MapsToDisplayP3) {
  EXPECT_EQ(OhosColorSpaceToTextureColorSpace(
                ohos_color_space_internal::kLinearP3),
            impeller::TextureColorSpace::kDisplayP3);
}

// ---------------------------------------------------------------------------
// OhosColorSpaceToTextureColorSpace - Extended SRGB family
// ---------------------------------------------------------------------------

TEST(OhosColorSpaceTest, AdobeRGBMapsToExtendedSRGB) {
  EXPECT_EQ(OhosColorSpaceToTextureColorSpace(
                ohos_color_space_internal::kAdobeRGB),
            impeller::TextureColorSpace::kExtendedSRGB);
}

TEST(OhosColorSpaceTest, DciP3MapsToExtendedSRGB) {
  EXPECT_EQ(OhosColorSpaceToTextureColorSpace(
                ohos_color_space_internal::kDciP3),
            impeller::TextureColorSpace::kExtendedSRGB);
}

TEST(OhosColorSpaceTest, BT2020HLGMapsToExtendedSRGB) {
  EXPECT_EQ(OhosColorSpaceToTextureColorSpace(
                ohos_color_space_internal::kBT2020HLG),
            impeller::TextureColorSpace::kExtendedSRGB);
}

TEST(OhosColorSpaceTest, BT2020PQMapsToExtendedSRGB) {
  EXPECT_EQ(OhosColorSpaceToTextureColorSpace(
                ohos_color_space_internal::kBT2020PQ),
            impeller::TextureColorSpace::kExtendedSRGB);
}

TEST(OhosColorSpaceTest, P3HLGMapsToExtendedSRGB) {
  EXPECT_EQ(OhosColorSpaceToTextureColorSpace(
                ohos_color_space_internal::kP3HLG),
            impeller::TextureColorSpace::kExtendedSRGB);
}

TEST(OhosColorSpaceTest, P3PQMapsToExtendedSRGB) {
  EXPECT_EQ(OhosColorSpaceToTextureColorSpace(
                ohos_color_space_internal::kP3PQ),
            impeller::TextureColorSpace::kExtendedSRGB);
}

TEST(OhosColorSpaceTest, AdobeRGBLimitMapsToExtendedSRGB) {
  EXPECT_EQ(OhosColorSpaceToTextureColorSpace(
                ohos_color_space_internal::kAdobeRGBLimit),
            impeller::TextureColorSpace::kExtendedSRGB);
}

TEST(OhosColorSpaceTest, BT2020HLGLimitMapsToExtendedSRGB) {
  EXPECT_EQ(OhosColorSpaceToTextureColorSpace(
                ohos_color_space_internal::kBT2020HLGLimit),
            impeller::TextureColorSpace::kExtendedSRGB);
}

TEST(OhosColorSpaceTest, BT2020PQLimitMapsToExtendedSRGB) {
  EXPECT_EQ(OhosColorSpaceToTextureColorSpace(
                ohos_color_space_internal::kBT2020PQLimit),
            impeller::TextureColorSpace::kExtendedSRGB);
}

TEST(OhosColorSpaceTest, P3HLGLimitMapsToExtendedSRGB) {
  EXPECT_EQ(OhosColorSpaceToTextureColorSpace(
                ohos_color_space_internal::kP3HLGLimit),
            impeller::TextureColorSpace::kExtendedSRGB);
}

TEST(OhosColorSpaceTest, P3PQLimitMapsToExtendedSRGB) {
  EXPECT_EQ(OhosColorSpaceToTextureColorSpace(
                ohos_color_space_internal::kP3PQLimit),
            impeller::TextureColorSpace::kExtendedSRGB);
}

TEST(OhosColorSpaceTest, LinearBT2020MapsToExtendedSRGB) {
  EXPECT_EQ(OhosColorSpaceToTextureColorSpace(
                ohos_color_space_internal::kLinearBT2020),
            impeller::TextureColorSpace::kExtendedSRGB);
}

// ---------------------------------------------------------------------------
// OhosColorSpaceToTextureColorSpace - fallback to sRGB
// ---------------------------------------------------------------------------

// An unrecognized color space value must fall back to kSRGB.
TEST(OhosColorSpaceTest, UnknownColorSpaceMapsToSRGB) {
  EXPECT_EQ(OhosColorSpaceToTextureColorSpace(0u),
            impeller::TextureColorSpace::kSRGB);
  EXPECT_EQ(OhosColorSpaceToTextureColorSpace(9999u),
            impeller::TextureColorSpace::kSRGB);
}

// A value that sits between the defined constants must fall back to kSRGB.
TEST(OhosColorSpaceTest, GapValueMapsToSRGB) {
  // Values 4-8 fall in the gap between kDisplayP3 (3) and kBT2020HLG (9),
  // so they are not defined as any known color space and must fall back to
  // kSRGB.
  EXPECT_EQ(OhosColorSpaceToTextureColorSpace(4u),
            impeller::TextureColorSpace::kSRGB);
  EXPECT_EQ(OhosColorSpaceToTextureColorSpace(8u),
            impeller::TextureColorSpace::kSRGB);
}

// ---------------------------------------------------------------------------
// ContainsColorSpace template helper (indirectly via the mapping function)
// ---------------------------------------------------------------------------

// The DisplayP3 family takes precedence over the ExtendedSRGB family: a value
// that appears in kDisplayP3ColorSpaces must map to kDisplayP3 even though
// some values (like kDisplayP3Limit) might look close to extended-sRGB
// constants.
TEST(OhosColorSpaceTest, DisplayP3FamilyPrecedence) {
  // kDisplayP3Limit (14) is in the DisplayP3 family, not the extended family.
  EXPECT_EQ(OhosColorSpaceToTextureColorSpace(14u),
            impeller::TextureColorSpace::kDisplayP3);
}

}  // namespace testing
}  // namespace flutter
