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
  vec4 source_rect;
  float alpha;
  int source_transfer;
  int output_mode;
  int source_primaries;
  int source_limited_10bit;
  float source_color_space;
  float target_color_space;
}
frag_info;

in highp vec2 v_texture_coords;

out f16vec4 frag_color;

void main() {
  vec2 texture_coords = vec2(clamp(v_texture_coords.x, frag_info.source_rect.x,
                                   frag_info.source_rect.z),
                             clamp(v_texture_coords.y, frag_info.source_rect.y,
                                   frag_info.source_rect.w));
  f16vec4 sampled =
      texture(texture_sampler, texture_coords, float16_t(kDefaultMipBias));
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
