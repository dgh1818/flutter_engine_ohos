// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// found in the LICENSE file.

precision mediump float;

#include <impeller/color_transform.glsl>
#include <impeller/types.glsl>

uniform FragInfo {
  vec4 color;
  float source_color_space;
  float target_color_space;
  // 0 = regular SDR/wide-gamut output, 1 = final 10-bit HLG swapchain.
  int output_mode;
}
frag_info;

out vec4 frag_color;

float srgb_to_linear(float c) {
  float value = abs(c);
  if (value <= 0.04045) {
    return c / 12.92;
  }
  return sign(c) * pow((value + 0.055) / 1.055, 2.4);
}

float linear_to_srgb_clamped(float c) {
  c = max(c, 0.0);
  if (c <= 0.0031308) {
    return c * 12.92;
  }
  return clamp(1.055 * pow(c, 1.0 / 2.4) - 0.055, 0.0, 1.0);
}

float linear_to_srgb_extended(float c) {
  float value = abs(c);
  if (value <= 0.0031308) {
    return c * 12.92;
  }
  return sign(c) * (1.055 * pow(value, 1.0 / 2.4) - 0.055);
}

vec3 srgb_to_linear_vec3(vec3 color) {
  return vec3(srgb_to_linear(color.r), srgb_to_linear(color.g),
              srgb_to_linear(color.b));
}

vec3 linear_to_srgb_clamped_vec3(vec3 color) {
  return vec3(linear_to_srgb_clamped(color.r), linear_to_srgb_clamped(color.g),
              linear_to_srgb_clamped(color.b));
}

vec3 linear_to_srgb_extended_vec3(vec3 color) {
  return vec3(linear_to_srgb_extended(color.r),
              linear_to_srgb_extended(color.g),
              linear_to_srgb_extended(color.b));
}
// https://www.w3.org/TR/css-color-4/
vec3 srgb_to_p3_linear(vec3 color) {
  return vec3(0.822461968714 * color.r + 0.177538031286 * color.g,
              0.033194198851 * color.r + 0.966805801149 * color.g,
              0.017082630721 * color.r + 0.072397440664 * color.g +
                  0.910519928615 * color.b);
}

vec3 p3_to_srgb_linear(vec3 color) {
  return vec3(1.224940176281 * color.r - 0.224940176281 * color.g,
              -0.042056954710 * color.r + 1.042056954710 * color.g,
              -0.019637554590 * color.r - 0.078636045551 * color.g +
                  1.098273600141 * color.b);
}

vec3 convert_srgb_family_to_p3(vec3 color) {
  vec3 linear = srgb_to_linear_vec3(color);
  vec3 p3_linear = srgb_to_p3_linear(linear);
  return linear_to_srgb_extended_vec3(p3_linear);
}

vec3 convert_p3_to_srgb_family(vec3 color, bool clamp_result) {
  vec3 linear = srgb_to_linear_vec3(color);
  vec3 srgb_linear = p3_to_srgb_linear(linear);
  if (clamp_result) {
    return linear_to_srgb_clamped_vec3(srgb_linear);
  } else {
    return linear_to_srgb_extended_vec3(srgb_linear);
  }
}

void main() {
  vec4 color = frag_info.color;
  int src_cs = int(frag_info.source_color_space + 0.5);
  int dst_cs = int(frag_info.target_color_space + 0.5);

  if (frag_info.output_mode == 1) {
    // Flutter colors arrive premultiplied. Apply nonlinear transfer functions
    // to the straight color, then premultiply again for the render pipeline.
    highp float alpha = color.a;
    highp vec3 straight = alpha > 0.0 ? vec3(color.rgb) / alpha : vec3(0.0);
    highp vec3 working = src_cs == 2 ? DisplayP3ToBT2020(SRGBToLinear(straight))
                                     : BT709ToBT2020(SRGBToLinear(straight));
    color.rgb = vec3(Working203ToHLG(working) * alpha);
    frag_color = color;
    return;
  }

  if (src_cs == dst_cs) {
    frag_color = color;
    return;
  }

  if (color.a <= 0.0) {
    frag_color = vec4(0.0);
    return;
  }
  color.rgb /= color.a;

  if (src_cs < 2 && dst_cs == 2) {
    color.rgb = convert_srgb_family_to_p3(color.rgb);
  } else if (src_cs == 2 && dst_cs < 2) {
    color.rgb = convert_p3_to_srgb_family(color.rgb, dst_cs == 0);
  } else if (src_cs == 1 && dst_cs == 0) {
    color.rgb = clamp(color.rgb, 0.0, 1.0);
  }

  color.rgb *= color.a;
  frag_color = color;
}
