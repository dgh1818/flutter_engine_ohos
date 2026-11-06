// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

precision mediump float;

#include <impeller/color_space.glsl>
#include <impeller/texture.glsl>
#include <impeller/types.glsl>

layout(constant_id = 0) const float supports_decal = 1.0;

uniform f16sampler2D texture_sampler;

uniform FragInfo {
  float x_tile_mode;
  float y_tile_mode;
  float alpha;
  float source_color_space;
  float target_color_space;
}
frag_info;

in highp vec2 v_texture_coords;

out f16vec4 frag_color;

void main() {
  f16vec4 sampled;
  if (supports_decal == 1.0) {
    sampled = texture(texture_sampler,   // sampler
                      v_texture_coords,  // texture coordinates
                      float16_t(kDefaultMipBias));
  } else {
    sampled = IPHalfSampleWithTileMode(
        texture_sampler,                   // sampler
        v_texture_coords,                  // texture coordinates
        float16_t(frag_info.x_tile_mode),  // x tile mode
        float16_t(frag_info.y_tile_mode)   // y tile mode
    );
  }
  frag_color = f16vec4(IPConvertColorSpace(vec4(sampled) * frag_info.alpha,
                                           frag_info.source_color_space,
                                           frag_info.target_color_space));
}
