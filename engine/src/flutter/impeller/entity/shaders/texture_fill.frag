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

void main() {
  f16vec4 sampled =
      texture(texture_sampler, v_texture_coords, float16_t(kDefaultMipBias));
  if (frag_info.output_mode == 1 || frag_info.source_transfer != 0) {
    frag_color = f16vec4(ConvertHdrTexture(
        vec4(sampled), frag_info.alpha, frag_info.source_transfer,
        frag_info.source_primaries, frag_info.source_limited_10bit,
        frag_info.output_mode, int(frag_info.target_color_space + 0.5)));
  } else {
    frag_color = f16vec4(IPConvertColorSpace(vec4(sampled) * frag_info.alpha,
                                             frag_info.source_color_space,
                                             frag_info.target_color_space));
  }
}
