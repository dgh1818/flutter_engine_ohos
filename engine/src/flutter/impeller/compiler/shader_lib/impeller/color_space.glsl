// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COLOR_SPACE_GLSL_
#define COLOR_SPACE_GLSL_

#include <impeller/types.glsl>

// Must match impeller::ColorSpace in impeller/geometry/color.h.
const int kColorSpaceSRGB = 0;
const int kColorSpaceExtendedSRGB = 1;
const int kColorSpaceDisplayP3 = 2;

float IPSrgbChannelToLinear(float c) {
  float value = abs(c);
  if (value <= 0.04045) {
    return c / 12.92;
  }
  return sign(c) * pow((value + 0.055) / 1.055, 2.4);
}

float IPLinearChannelToSrgb(float c) {
  float value = abs(c);
  if (value <= 0.0031308) {
    return c * 12.92;
  }
  return sign(c) * (1.055 * pow(value, 1.0 / 2.4) - 0.055);
}

vec3 IPSrgbToLinear(vec3 color) {
  return vec3(IPSrgbChannelToLinear(color.r), IPSrgbChannelToLinear(color.g),
              IPSrgbChannelToLinear(color.b));
}

/// Applies the sRGB transfer function without clamping, so values outside
/// [0, 1] are preserved for extended range targets.
vec3 IPLinearToSrgb(vec3 color) {
  return vec3(IPLinearChannelToSrgb(color.r), IPLinearChannelToSrgb(color.g),
              IPLinearChannelToSrgb(color.b));
}

// https://www.w3.org/TR/css-color-4/
vec3 IPLinearSrgbToLinearP3(vec3 color) {
  return vec3(0.822461968714 * color.r + 0.177538031286 * color.g,
              0.033194198851 * color.r + 0.966805801149 * color.g,
              0.017082630721 * color.r + 0.072397440664 * color.g +
                  0.910519928615 * color.b);
}

vec3 IPLinearP3ToLinearSrgb(vec3 color) {
  return vec3(1.224940176281 * color.r - 0.224940176281 * color.g,
              -0.042056954710 * color.r + 1.042056954710 * color.g,
              -0.019637554590 * color.r - 0.078636045551 * color.g +
                  1.098273600141 * color.b);
}

/// Converts `color` from `source_color_space` to `target_color_space`.
///
/// The color space arguments carry impeller::ColorSpace values as floats.
vec4 IPConvertColorSpace(vec4 color,
                         float source_color_space,
                         float target_color_space) {
  int src = int(source_color_space + 0.5);
  int dst = int(target_color_space + 0.5);
  if (src == dst) {
    return color;
  }
  if (color.a <= 0.0) {
    return vec4(0.0);
  }
  color.rgb /= color.a;
  if (src != kColorSpaceDisplayP3 && dst == kColorSpaceDisplayP3) {
    color.rgb =
        IPLinearToSrgb(IPLinearSrgbToLinearP3(IPSrgbToLinear(color.rgb)));
  } else if (src == kColorSpaceDisplayP3 && dst != kColorSpaceDisplayP3) {
    color.rgb =
        IPLinearToSrgb(IPLinearP3ToLinearSrgb(IPSrgbToLinear(color.rgb)));
    if (dst == kColorSpaceSRGB) {
      color.rgb = clamp(color.rgb, 0.0, 1.0);
    }
  } else if (src == kColorSpaceExtendedSRGB && dst == kColorSpaceSRGB) {
    color.rgb = clamp(color.rgb, 0.0, 1.0);
  }
  color.rgb *= color.a;
  return color;
}

#endif  // COLOR_SPACE_GLSL_
