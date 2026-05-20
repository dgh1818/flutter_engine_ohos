// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// can be found in the LICENSE file.

#include "flutter/display_list/dl_color.h"

#include <algorithm>
#include <cmath>

#include "flutter/fml/build_config.h"

namespace flutter {

DlColor DlColor::withColorSpace(DlColorSpace color_space) const {
  // If same color space, return as-is
  if (color_space_ == color_space) {
    return *this;
  }

  // Color space conversion is handled in shader for OHOS wide gamut
  // Just return the color with the new color space identifier
  // Preserve the original color values
  switch (color_space) {
    case DlColorSpace::kSRGB:
      // When converting to sRGB, clamp values to [0,1]
      return DlColor(alpha_, std::clamp(red_, 0.0f, 1.0f),
                     std::clamp(green_, 0.0f, 1.0f),
                     std::clamp(blue_, 0.0f, 1.0f), DlColorSpace::kSRGB);
    case DlColorSpace::kExtendedSRGB:
    case DlColorSpace::kDisplayP3:
      // For ExtendedSRGB and DisplayP3, keep the values as-is
      // Conversion will happen in shader
      return DlColor(alpha_, red_, green_, blue_, color_space);
    default:
      // Unknown color space, return as-is
      return *this;
  }
}

}  // namespace flutter
