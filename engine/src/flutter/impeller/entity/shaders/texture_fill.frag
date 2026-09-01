// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

precision mediump float;

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
}
frag_info;

in highp vec2 v_texture_coords;

out f16vec4 frag_color;

// Source texture color -> linear BT.2020 working space (203-nit ref white).
highp vec3 ToLinearWorking(highp vec3 color) {
  if (frag_info.source_transfer == 1) {
    return HLGToWorking203(color);
  }
  if (frag_info.source_transfer == 2) {
    return PQToLinear(color);
  }
  if (frag_info.source_primaries == 1) {
    return DisplayP3ToBT2020(SRGBToLinear(color));
  }
  return BT709ToBT2020(SRGBToLinear(color));
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
  highp vec3 color = vec3(sampled.rgb);
  if (frag_info.output_mode == 1) {
    highp vec3 working = ToLinearWorking(color);
    // Working-space 1.0 (203-nit SDR/graphics white) maps to HLG signal 0.75.
    // The scaling is performed before the inverse EOTF so shadows and
    // midtones retain the SDR appearance inside the HLG presentation.
    color = FromLinearWorking(working);
  }
  frag_color = f16vec4(f16vec3(color), sampled.a * float16_t(frag_info.alpha));
}
