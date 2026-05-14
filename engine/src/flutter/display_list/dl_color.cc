// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// can be found in the LICENSE file.

#include "flutter/display_list/dl_color.h"

#include <algorithm>
#include <cmath>

#include "flutter/fml/build_config.h"

namespace flutter {

namespace {
#ifdef FML_OS_OHOS
// OHOS: P3 to SRGB conversion matrix (linear space)
const std::array<DlScalar, 12> kP3ToSrgb = {
    1.306671048092539,  -0.298061942172353,
    0.213228303487995,  -0.213580156254466,
    -0.117390025596251, 1.127722006101976,
    0.109727644608938,  -0.109450321455370,
    0.214813187718391,  0.054268702864647,
    1.406898424029350,  -0.364892765879631};

// OHOS: SRGB to P3 conversion matrix (linear space)
const std::array<DlScalar, 12> kSrgbToP3 = {
    0.822461186547,   0.177538013953,   0.000000000000,   0.000000000000,
    0.033191798446,   0.966808201554,   0.000000000000,   0.000000000000,
    0.017055917378,   0.072079544594,   0.910864537928,   0.000000000000};

DlScalar srgb_to_linear(DlScalar c) {
  c = std::clamp(c, 0.0f, 1.0f);
  if (c <= 0.04045f) {
    return c / 12.92f;
  }
  return std::pow((c + 0.055f) / 1.055f, 2.4f);
}

DlScalar linear_to_srgb(DlScalar c) {
  c = std::max(c, 0.0f);
  if (c <= 0.0031308f) {
    return c * 12.92f;
  }
  return std::clamp(1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f, 0.0f, 1.0f);
}

DlColor transformLinear(const DlColor& color,
                        const std::array<DlScalar, 12>& matrix,
                        DlColorSpace color_space) {
  DlScalar r_lin = srgb_to_linear(color.getRedF());
  DlScalar g_lin = srgb_to_linear(color.getGreenF());
  DlScalar b_lin = srgb_to_linear(color.getBlueF());
  
  DlScalar r_out = matrix[0] * r_lin + matrix[1] * g_lin + matrix[2] * b_lin + matrix[3];
  DlScalar g_out = matrix[4] * r_lin + matrix[5] * g_lin + matrix[6] * b_lin + matrix[7];
  DlScalar b_out = matrix[8] * r_lin + matrix[9] * g_lin + matrix[10] * b_lin + matrix[11];
  
  r_out = linear_to_srgb(r_out);
  g_out = linear_to_srgb(g_out);
  b_out = linear_to_srgb(b_out);
  
  return DlColor(color.getAlphaF(), r_out, g_out, b_out, color_space);
}
#endif  // FML_OS_OHOS
}  // namespace

DlColor DlColor::withColorSpace(DlColorSpace color_space) const {
  switch (color_space_) {
    case DlColorSpace::kSRGB:
      switch (color_space) {
        case DlColorSpace::kSRGB:
          return *this;
        case DlColorSpace::kExtendedSRGB:
          return DlColor(alpha_, red_, green_, blue_,
                         DlColorSpace::kExtendedSRGB);
        case DlColorSpace::kDisplayP3:
#ifdef FML_OS_OHOS
          return transformLinear(*this, kSrgbToP3, DlColorSpace::kDisplayP3);
#else
          // Non-OHOS: DisplayP3 not supported, return as ExtendedSRGB
          return DlColor(alpha_, red_, green_, blue_,
                         DlColorSpace::kExtendedSRGB);
#endif
      }
    case DlColorSpace::kExtendedSRGB:
      switch (color_space) {
        case DlColorSpace::kSRGB:
          return DlColor(alpha_, std::clamp(red_, 0.0f, 1.0f),
                         std::clamp(green_, 0.0f, 1.0f),
                         std::clamp(blue_, 0.0f, 1.0f), DlColorSpace::kSRGB);
        case DlColorSpace::kExtendedSRGB:
          return *this;
        case DlColorSpace::kDisplayP3:
#ifdef FML_OS_OHOS
          return transformLinear(*this, kSrgbToP3, DlColorSpace::kDisplayP3);
#else
          // Non-OHOS: DisplayP3 not supported, return as ExtendedSRGB
          return DlColor(alpha_, red_, green_, blue_,
                         DlColorSpace::kExtendedSRGB);
#endif
      }
    case DlColorSpace::kDisplayP3:
      switch (color_space) {
        case DlColorSpace::kSRGB:
#ifdef FML_OS_OHOS
          return transformLinear(*this, kP3ToSrgb, DlColorSpace::kExtendedSRGB)
              .withColorSpace(DlColorSpace::kSRGB);
#else
          return DlColor(alpha_, std::clamp(red_, 0.0f, 1.0f),
                         std::clamp(green_, 0.0f, 1.0f),
                         std::clamp(blue_, 0.0f, 1.0f), DlColorSpace::kSRGB);
#endif
        case DlColorSpace::kExtendedSRGB:
#ifdef FML_OS_OHOS
          return transformLinear(*this, kP3ToSrgb, DlColorSpace::kExtendedSRGB);
#else
          return DlColor(alpha_, red_, green_, blue_,
                         DlColorSpace::kExtendedSRGB);
#endif
        case DlColorSpace::kDisplayP3:
          return *this;
      }
  }
}

}  // namespace flutter
