// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

precision mediump float;

#include <impeller/color_space.glsl>
#include <impeller/color_transform.glsl>
#include <impeller/constants.glsl>
#include <impeller/types.glsl>

uniform f16sampler2D texture_sampler;

uniform FragInfo {
  float alpha;
  // Source transfer of the texture (ImageColorMetadata.transfer):
  // 0 = sRGB (SDR_DisplayP3 default), 1 = HLG, 2 = PQ.
  int source_transfer;
  // Output composition mode (per-surface OutputColorProfile.mode):
  // 0 = passthrough (SDR_DisplayP3 default), 1 = encode to HLG for the
  // fixed 10-bit HLG swapchain (HLG10_Unified).
  int output_mode;
  // Source primaries: 0 = BT.709, 1 = Display P3 (D65). Picks the correct
  // linear -> BT.2020 gamut matrix.
  int source_primaries;
  // Limited-range RGB samples have nominal 10-bit code values 64..940.
  int source_limited_10bit;
  float source_color_space;
  float target_color_space;
}
frag_info;

in highp vec2 v_texture_coords;

out f16vec4 frag_color;

// Source texture color -> linear BT.2020 working space (203-nit ref white).
highp vec3 ToLinearWorking(highp vec3 color) {
  highp vec3 linear_color;
  if (frag_info.source_transfer == 1) {
    linear_color = HLGToWorking203(color);
  } else if (frag_info.source_transfer == 2) {
    linear_color = PQToLinear(color);
  } else {
    linear_color = SRGBToLinear(color);
  }
  if (frag_info.source_primaries == 1) {
    return DisplayP3ToBT2020(linear_color);
  }
  if (frag_info.source_transfer != 0) {
    return linear_color;
  }
  return BT709ToBT2020(linear_color);
}

// Linear BT.2020 working space -> output surface encoding.
highp vec3 FromLinearWorking(highp vec3 working) {
  if (frag_info.output_mode == 1) {
    return Working203ToHLG(working);
  }
  return BT2020ToBT709(working);
}

void main() {
  f16vec4 sampled =
      texture(texture_sampler, v_texture_coords, float16_t(kDefaultMipBias));
  if (frag_info.output_mode == 1) {
    highp vec3 color = vec3(sampled.rgb);
    if (frag_info.source_limited_10bit == 1) {
      color = clamp((color * 1023.0 - 64.0) / 876.0, 0.0, 1.0);
    }
    highp vec3 working = ToLinearWorking(color);
    color = FromLinearWorking(working);
    frag_color =
        f16vec4(f16vec3(color), sampled.a * float16_t(frag_info.alpha));
  } else {
    frag_color = f16vec4(IPConvertColorSpace(vec4(sampled) * frag_info.alpha,
                                             frag_info.source_color_space,
                                             frag_info.target_color_space));
  }
}
