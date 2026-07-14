// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "display_list/dl_blend_mode.h"
#include "display_list/dl_color.h"
#include "display_list/dl_tile_mode.h"
#include "flutter/testing/testing.h"
#include "impeller/core/formats.h"
#include "impeller/display_list/skia_conversions.h"
#include "impeller/geometry/color.h"
#include "impeller/geometry/scalar.h"

#include <cmath>

namespace impeller {
namespace testing {

TEST(SkiaConversionTest, ToSamplerDescriptor) {
  EXPECT_EQ(skia_conversions::ToSamplerDescriptor(
                flutter::DlImageSampling::kNearestNeighbor)
                .min_filter,
            impeller::MinMagFilter::kNearest);
  EXPECT_EQ(skia_conversions::ToSamplerDescriptor(
                flutter::DlImageSampling::kNearestNeighbor)
                .mip_filter,
            impeller::MipFilter::kBase);

  EXPECT_EQ(
      skia_conversions::ToSamplerDescriptor(flutter::DlImageSampling::kLinear)
          .min_filter,
      impeller::MinMagFilter::kLinear);
  EXPECT_EQ(
      skia_conversions::ToSamplerDescriptor(flutter::DlImageSampling::kLinear)
          .mip_filter,
      impeller::MipFilter::kBase);

  EXPECT_EQ(skia_conversions::ToSamplerDescriptor(
                flutter::DlImageSampling::kMipmapLinear)
                .min_filter,
            impeller::MinMagFilter::kLinear);
  EXPECT_EQ(skia_conversions::ToSamplerDescriptor(
                flutter::DlImageSampling::kMipmapLinear)
                .mip_filter,
            impeller::MipFilter::kLinear);
}

TEST(SkiaConversionsTest, ToColor) {
  // Create a color with alpha, red, green, and blue values that are all
  // trivially divisible by 255 so that we can test the conversion results in
  // correct scalar values.
  //                                                AARRGGBB
  const flutter::DlColor color = flutter::DlColor(0x8040C020);
  auto converted_color = skia_conversions::ToColor(color).color;

  ASSERT_TRUE(ScalarNearlyEqual(converted_color.alpha, 0x80 * (1.0f / 255)));
  ASSERT_TRUE(ScalarNearlyEqual(converted_color.red, 0x40 * (1.0f / 255)));
  ASSERT_TRUE(ScalarNearlyEqual(converted_color.green, 0xC0 * (1.0f / 255)));
  ASSERT_TRUE(ScalarNearlyEqual(converted_color.blue, 0x20 * (1.0f / 255)));
}

// ============================================================================
// Color Space Conversion Tests
// Reference: IEC 61966-2-1 (sRGB), SMPTE EG 432-1 (Display P3)
// ============================================================================

// sRGB gamma transfer function (matches shader implementation)
static float SrgbToLinear(float c) {
  c = std::max(c, 0.0f);
  if (c <= 0.04045f) {
    return c / 12.92f;
  }
  return std::pow((c + 0.055f) / 1.055f, 2.4f);
}

// Inverse sRGB gamma transfer function (matches shader implementation)
static float LinearToSrgb(float c) {
  c = std::max(c, 0.0f);
  if (c <= 0.0031308f) {
    return c * 12.92f;
  }
  return std::clamp(1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f, 0.0f, 1.0f);
}

// sRGB to Display P3 conversion matrix (matches shader implementation)
static void SrgbToP3Linear(float r, float g, float b,
                           float& out_r, float& out_g, float& out_b) {
  out_r = 0.822461186547f * r + 0.177538013953f * g + 0.0f * b;
  out_g = 0.033191798446f * r + 0.966808201554f * g + 0.0f * b;
  out_b = 0.017055917378f * r + 0.072079544594f * g + 0.910864537928f * b;
}

// Display P3 to sRGB conversion matrix (matches shader implementation)
static void P3ToSrgbLinear(float r, float g, float b,
                           float& out_r, float& out_g, float& out_b) {
  out_r = 1.306671048092539f * r + (-0.298061942172353f) * g + 0.213228303487995f * b;
  out_g = (-0.117390025596251f) * r + 1.127722006101976f * g + 0.109727644608938f * b;
  out_b = 0.214813187718391f * r + 0.054268702864647f * g + 1.406898424029350f * b;
}

TEST(ColorSpaceConversionTest, SrgbGammaThresholds) {
  // Verify gamma thresholds match IEC 61966-2-1 specification
  // Threshold for sRGB to linear: 0.04045
  // Threshold for linear to sRGB: 0.0031308

  // At threshold, both branches should give approximately the same result
  constexpr float kSrgbThreshold = 0.04045f;
  constexpr float kLinearThreshold = 0.0031308f;

  // Verify srgb_to_linear continuity at threshold
  float linear_from_branch1 = kSrgbThreshold / 12.92f;
  float linear_from_branch2 = std::pow((kSrgbThreshold + 0.055f) / 1.055f, 2.4f);
  EXPECT_NEAR(linear_from_branch1, linear_from_branch2, 1e-6f);

  // Verify linear_to_srgb continuity at threshold
  float srgb_from_branch1 = kLinearThreshold * 12.92f;
  float srgb_from_branch2 = 1.055f * std::pow(kLinearThreshold, 1.0f / 2.4f) - 0.055f;
  EXPECT_NEAR(srgb_from_branch1, srgb_from_branch2, 1e-6f);
}

TEST(ColorSpaceConversionTest, SrgbGammaRoundTrip) {
  // Test that sRGB -> linear -> sRGB round-trip preserves values
  constexpr float kTestValues[] = {0.0f, 0.1f, 0.25f, 0.5f, 0.75f, 1.0f};
  for (float val : kTestValues) {
    float linear = SrgbToLinear(val);
    float round_trip = LinearToSrgb(linear);
    EXPECT_NEAR(val, round_trip, 1e-5f)
        << "Round-trip failed for value " << val;
  }
}

TEST(ColorSpaceConversionTest, SrgbGammaKnownValues) {
  // Verify known sRGB gamma values from IEC 61966-2-1
  // sRGB 0.0 -> linear 0.0
  EXPECT_FLOAT_EQ(SrgbToLinear(0.0f), 0.0f);
  // sRGB 1.0 -> linear 1.0
  EXPECT_FLOAT_EQ(SrgbToLinear(1.0f), 1.0f);
  // sRGB 0.5 -> linear ~0.2140
  EXPECT_NEAR(SrgbToLinear(0.5f), 0.2140f, 0.001f);
}

TEST(ColorSpaceConversionTest, SrgbToP3IdentityForWhite) {
  // White (1,1,1) should remain white after conversion
  // Both sRGB and Display P3 use D65 white point
  float r, g, b;
  SrgbToP3Linear(1.0f, 1.0f, 1.0f, r, g, b);
  EXPECT_NEAR(r, 1.0f, 1e-5f);
  EXPECT_NEAR(g, 1.0f, 1e-5f);
  EXPECT_NEAR(b, 1.0f, 1e-5f);
}

TEST(ColorSpaceConversionTest, P3ToSrgbIdentityForWhite) {
  // White (1,1,1) should remain white after conversion
  float r, g, b;
  P3ToSrgbLinear(1.0f, 1.0f, 1.0f, r, g, b);
  EXPECT_NEAR(r, 1.0f, 1e-5f);
  EXPECT_NEAR(g, 1.0f, 1e-5f);
  EXPECT_NEAR(b, 1.0f, 1e-5f);
}

TEST(ColorSpaceConversionTest, SrgbToP3RoundTrip) {
  // Test that sRGB -> P3 -> sRGB round-trip preserves values
  constexpr float kTestColors[][3] = {
      {1.0f, 0.0f, 0.0f},  // Pure red
      {0.0f, 1.0f, 0.0f},  // Pure green
      {0.0f, 0.0f, 1.0f},  // Pure blue
      {0.5f, 0.5f, 0.5f},  // Gray
      {1.0f, 1.0f, 0.0f},  // Yellow
      {0.0f, 1.0f, 1.0f},  // Cyan
  };

  for (const auto& color : kTestColors) {
    float r = color[0], g = color[1], b = color[2];

    // Convert to linear
    float linear_r = SrgbToLinear(r);
    float linear_g = SrgbToLinear(g);
    float linear_b = SrgbToLinear(b);

    // sRGB linear -> P3 linear
    float p3_r, p3_g, p3_b;
    SrgbToP3Linear(linear_r, linear_g, linear_b, p3_r, p3_g, p3_b);

    // P3 linear -> sRGB linear
    float back_r, back_g, back_b;
    P3ToSrgbLinear(p3_r, p3_g, p3_b, back_r, back_g, back_b);

    // Should round-trip to original linear values
    EXPECT_NEAR(linear_r, back_r, 1e-5f)
        << "Round-trip failed for R in color (" << r << ", " << g << ", " << b << ")";
    EXPECT_NEAR(linear_g, back_g, 1e-5f)
        << "Round-trip failed for G in color (" << r << ", " << g << ", " << b << ")";
    EXPECT_NEAR(linear_b, back_b, 1e-5f)
        << "Round-trip failed for B in color (" << r << ", " << g << ", " << b << ")";
  }
}

TEST(ColorSpaceConversionTest, SrgbRedToP3) {
  // sRGB pure red (1,0,0) in linear space
  // Should map to Display P3 red which has different chromaticity
  // sRGB red (0.64, 0.33) -> P3 red (0.680, 0.320)
  float linear_r = SrgbToLinear(1.0f);
  float linear_g = SrgbToLinear(0.0f);
  float linear_b = SrgbToLinear(0.0f);

  float p3_r, p3_g, p3_b;
  SrgbToP3Linear(linear_r, linear_g, linear_b, p3_r, p3_g, p3_b);

  // sRGB red in P3 space should have reduced red component and some green
  // because P3 red primary is more saturated than sRGB red
  EXPECT_GT(p3_r, 0.9f);  // Red component should be high
  EXPECT_GT(p3_g, 0.0f);  // Should have some green
  EXPECT_NEAR(p3_b, 0.0f, 1e-5f);  // Blue should be zero
}

TEST(ColorSpaceConversionTest, MatrixCoefficientsSum) {
  // Each row of the conversion matrix should sum to 1.0
  // because the matrices preserve white point (D65)

  // sRGB to P3 matrix row sums
  float row1_sum = 0.822461186547f + 0.177538013953f + 0.0f;
  float row2_sum = 0.033191798446f + 0.966808201554f + 0.0f;
  float row3_sum = 0.017055917378f + 0.072079544594f + 0.910864537928f;
  EXPECT_NEAR(row1_sum, 1.0f, 1e-5f);
  EXPECT_NEAR(row2_sum, 1.0f, 1e-5f);
  EXPECT_NEAR(row3_sum, 1.0f, 1e-5f);

  // P3 to sRGB matrix row sums
  float row1_sum_inv = 1.306671048092539f + (-0.298061942172353f) + 0.213228303487995f;
  float row2_sum_inv = (-0.117390025596251f) + 1.127722006101976f + 0.109727644608938f;
  float row3_sum_inv = 0.214813187718391f + 0.054268702864647f + 1.406898424029350f;
  EXPECT_NEAR(row1_sum_inv, 1.0f, 1e-5f);
  EXPECT_NEAR(row2_sum_inv, 1.0f, 1e-5f);
  EXPECT_NEAR(row3_sum_inv, 1.0f, 1e-5f);
}

}  // namespace testing
}  // namespace impeller
