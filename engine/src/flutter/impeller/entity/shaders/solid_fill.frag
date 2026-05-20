// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// found in the LICENSE file.

precision mediump float;

#include <impeller/types.glsl>

uniform FragInfo {
  vec4 color;
  float source_color_space;
  float target_color_space;
}
frag_info;

out vec4 frag_color;

float srgb_to_linear(float c) {
  c = max(c, 0.0);
  if (c <= 0.04045) {
    return c / 12.92;
  }
  return pow((c + 0.055) / 1.055, 2.4);
}

float linear_to_srgb_clamped(float c) {
  c = max(c, 0.0);
  if (c <= 0.0031308) {
    return c * 12.92;
  }
  return clamp(1.055 * pow(c, 1.0 / 2.4) - 0.055, 0.0, 1.0);
}

float linear_to_srgb_extended(float c) {
  c = max(c, 0.0);
  if (c <= 0.0031308) {
    return c * 12.92;
  }
  return 1.055 * pow(c, 1.0 / 2.4) - 0.055;
}

vec3 srgb_to_linear_vec3(vec3 color) {
  return vec3(
    srgb_to_linear(color.r),
    srgb_to_linear(color.g),
    srgb_to_linear(color.b)
  );
}

vec3 linear_to_srgb_clamped_vec3(vec3 color) {
  return vec3(
    linear_to_srgb_clamped(color.r),
    linear_to_srgb_clamped(color.g),
    linear_to_srgb_clamped(color.b)
  );
}

vec3 linear_to_srgb_extended_vec3(vec3 color) {
  return vec3(
    linear_to_srgb_extended(color.r),
    linear_to_srgb_extended(color.g),
    linear_to_srgb_extended(color.b)
  );
}
// https://www.w3.org/TR/css-color-4/
vec3 srgb_to_p3_linear(vec3 color) {
  return vec3(
    0.822461186547 * color.r + 0.177538013953 * color.g,
    0.033191798446 * color.r + 0.966808201554 * color.g,
    0.017055917378 * color.r + 0.072079544594 * color.g + 0.910864537928 * color.b
  );
}

vec3 p3_to_srgb_linear(vec3 color) {
  return vec3(
    1.306671048092539 * color.r - 0.298061942172353 * color.g + 0.213228303487995 * color.b,
    -0.117390025596251 * color.r + 1.127722006101976 * color.g + 0.109727644608938 * color.b,
    0.214813187718391 * color.r + 0.054268702864647 * color.g + 1.406898424029350 * color.b
  );
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
  
  if (src_cs == dst_cs) {
    frag_color = color;
    return;
  }
  
  if (src_cs < 2 && dst_cs == 2) {
    color.rgb = convert_srgb_family_to_p3(color.rgb);
  } else if (src_cs == 2 && dst_cs < 2) {
    color.rgb = convert_p3_to_srgb_family(color.rgb, dst_cs == 0);
  } else if (src_cs == 1 && dst_cs == 0) {
    color.rgb = clamp(color.rgb, 0.0, 1.0);
  }
  
  frag_color = color;
}