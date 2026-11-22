// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// HDR color transform helpers for the unified PQ/HLG pipeline
//
// All functions operate on highp floats: PQ dark regions and specular
// highlights lose unacceptable precision in mediump.
//
// Reference conditions follow ITU-R BT.2100 / BT.2408:
//  - HLG reference white (graphics white) = 75% signal
//  - PQ nominal peak                      = 10000 cd/m^2
// The working space is linear BT.2020 with 1.0 == graphics white (203 nits)
// so SDR content maps 1:1 and HDR highlights may exceed 1.0. HLG's EOTF is
// peak-relative, therefore HLG values must be normalized at the boundary of
// this working space instead of scaling the non-linear HLG signal.

// -----------------------------------------------------------------------------
// Transfer functions: sRGB
// ----------------------------------------------------------------------------

const float kSrgbAlpha = 0.055;

highp float SRGBToLinearChannel(highp float channel) {
  highp float value = abs(channel);
  if (value <= 0.04045) {
    return channel / 12.92;
  }
  return sign(channel) * pow((value + kSrgbAlpha) / (1.0 + kSrgbAlpha), 2.4);
}

highp vec3 SRGBToLinear(highp vec3 color) {
  return vec3(SRGBToLinearChannel(color.r),  //
              SRGBToLinearChannel(color.g),  //
              SRGBToLinearChannel(color.b));
}

highp float LinearToSRGBChannel(highp float channel) {
  highp float value = abs(channel);
  if (value <= 0.0031308) {
    return channel * 12.92;
  }
  return sign(channel) *
         ((1.0 + kSrgbAlpha) * pow(value, 1.0 / 2.4) - kSrgbAlpha);
}

highp vec3 LinearToSRGB(highp vec3 color) {
  return vec3(LinearToSRGBChannel(color.r),  //
              LinearToSRGBChannel(color.g),  //
              LinearToSRGBChannel(color.b));
}

// -----------------------------------------------------------------------------
// Transfer functions: HLG (BT.2100), normalized signal [0, 1]
// ----------------------------------------------------------------------------

const float kHlgA = 0.17883277;
const float kHlgB = 0.28466892;  // 1 - 4 * kHlgA
const float kHlgC = 0.55991073;  // 0.5 - kHlgA * ln(4 * kHlgA)
// Neutral HLG signal 0.75 under the 1.2 reference system gamma.
// In other words, 203-nit graphics white occupies this fraction of the HLG
// peak-relative linear range.
const float kHlgGraphicsWhiteLinear = 0.203152145;

// HLG inverse OETF: signal -> relative scene light [0, 1].
highp float HLGInverseOETFChannel(highp float channel) {
  if (channel <= 0.5) {
    return (channel * channel) / 3.0;
  }
  return (exp((channel - kHlgC) / kHlgA) + kHlgB) / 12.0;
}

highp vec3 HLGInverseOETF(highp vec3 color) {
  return vec3(HLGInverseOETFChannel(color.r),  //
              HLGInverseOETFChannel(color.g),  //
              HLGInverseOETFChannel(color.b));
}

// HLG OETF: relative scene light [0, 1] -> signal.
highp float HLGOETFChannel(highp float scene) {
  if (scene <= 0.0) {
    return 0.0;
  }
  if (scene <= (1.0 / 12.0)) {
    return sqrt(3.0 * scene);
  }
  return kHlgA * log(12.0 * scene - kHlgB) + kHlgC;
}

highp vec3 HLGOETF(highp vec3 scene) {
  return vec3(HLGOETFChannel(scene.r),  //
              HLGOETFChannel(scene.g),  //
              HLGOETFChannel(scene.b));
}

// BT.2100 HLG OOTF uses scene luminance, not separate channel powers.
const highp vec3 kBT2020Luma = vec3(0.2627, 0.6780, 0.0593);

// Linear BT.2020 scene light -> display light in units of 203 nits.
highp vec3 HLGSceneToWorking203(highp vec3 scene) {
  highp float luminance = max(dot(scene, kBT2020Luma), 0.0);
  return scene * pow(luminance, 0.2) / kHlgGraphicsWhiteLinear;
}

// Inverse HLG OOTF and OETF (peak-relative BT.2020 display light -> signal).
highp vec3 LinearToHLG(highp vec3 working) {
  highp vec3 display = max(working, 0.0);
  highp float luminance = dot(display, kBT2020Luma);
  if (luminance <= 0.0) {
    return vec3(0.0);
  }
  return HLGOETF(display * pow(luminance, (1.0 / 1.2) - 1.0));
}

// Linear BT.2020 working space where 1.0 is 203 nits -> HLG signal. Scaling
// before the inverse EOTF preserves the SDR tone curve; multiplying the
// encoded result by 0.75 incorrectly lifts shadows and midtones.
highp vec3 Working203ToHLG(highp vec3 working) {
  return LinearToHLG(working * kHlgGraphicsWhiteLinear);
}

// -----------------------------------------------------------------------------
// Transfer functions: PQ / ST 2084 (BT.2100), absolute light
// ----------------------------------------------------------------------------

const float kPqM1 = 0.1593017578125;
const float kPqM2 = 78.84375;
const float kPqC1 = 0.8359375;
const float kPqC2 = 18.8515625;
const float kPqC3 = 18.6875;
const float kPqMaxLuminance = 10000.0;

// PQ inverse EOTF: signal [0, 1] -> absolute light in cd/m^2.
highp float PQToNitsChannel(highp float channel) {
  highp float c = pow(channel, 1.0 / kPqM2);
  highp float numerator = max(c - kPqC1, 0.0);
  highp float denominator = kPqC2 - kPqC3 * c;
  return kPqMaxLuminance * pow(numerator / denominator, 1.0 / kPqM1);
}

highp vec3 PQToNits(highp vec3 color) {
  return vec3(PQToNitsChannel(color.r),  //
              PQToNitsChannel(color.g),  //
              PQToNitsChannel(color.b));
}

// PQ EOTF: absolute light in cd/m^2 -> signal [0, 1].
// PQ EOTF (signal -> linear working space in units of reference white).
// 203 cd/m^2 == 1.0.
highp vec3 PQToLinear(highp vec3 color) {
  return PQToNits(color) / 203.0;
}

// -----------------------------------------------------------------------------
// Primaries: linear conversion into BT.2020 (the working space primaries)
// ----------------------------------------------------------------------------

// Linear BT.709 (== sRGB primaries) -> linear BT.2020.
highp vec3 BT709ToBT2020(highp vec3 color) {
  // Rows: [0.6274, 0.3293, 0.0433], [0.0691, 0.9195, 0.0114],
  //       [0.0164, 0.0880, 0.8956]
  return mat3(0.627404, 0.069097, 0.016391,  //
              0.329282, 0.919540, 0.088013,  //
              0.043313, 0.011362, 0.895595) *
         color;
}

// Linear BT.2020 -> linear BT.709 (== sRGB primaries).
highp vec3 BT2020ToBT709(highp vec3 color) {
  // Rows: [1.6605, -0.5876, -0.0728], [-0.1246, 1.1329, -0.0083],
  //       [-0.0182, -0.1006, 1.1187]
  return mat3(1.660491, -0.124550, -0.018151,  //
              -0.587641, 1.132900, -0.100579,  //
              -0.072838, -0.008349, 1.118730) *
         color;
}

// Linear Display P3 (D65) -> linear BT.2020.
highp vec3 DisplayP3ToBT2020(highp vec3 color) {
  // D65 primaries, equivalent to the SkNamedGamut matrices in Skia.
  return mat3(0.75383303, 0.04574385, -0.00121034,  //
              0.19859737, 0.94177722, 0.01760172,   //
              0.04756960, 0.01247893, 0.98360862) *
         color;
}

// Linear BT.2020 -> linear Display P3 (D65).
highp vec3 BT2020ToDisplayP3(highp vec3 color) {
  return mat3(1.34357825, -0.06529745, 0.00282179,   //
              -0.28217967, 1.07578792, -0.01959849,  //
              -0.06139858, -0.01049046, 1.01677671) *
         color;
}

// Convert premultiplied image pixels to either the HLG presentation encoding
// or the extended-sRGB/P3 working encoding used by offscreen passes.
highp vec4 ConvertHdrTexture(highp vec4 sampled,
                             highp float opacity,
                             int transfer,
                             int primaries,
                             int limited_range,
                             int output_mode,
                             int target_color_space) {
  highp float alpha = sampled.a;
  if (alpha <= 0.0) {
    return vec4(0.0);
  }
  highp vec3 color = sampled.rgb / alpha;
  if (limited_range == 1) {
    color = clamp((color * 1023.0 - 64.0) / 876.0, 0.0, 1.0);
  }

  highp vec3 working;
  if (transfer == 1) {
    highp vec3 scene = HLGInverseOETF(clamp(color, 0.0, 1.0));
    working =
        HLGSceneToWorking203(primaries == 1 ? DisplayP3ToBT2020(scene) : scene);
  } else {
    working = transfer == 2 ? PQToLinear(clamp(color, 0.0, 1.0))
                            : SRGBToLinear(color);
    if (primaries == 1) {
      working = DisplayP3ToBT2020(working);
    } else if (transfer == 0) {
      working = BT709ToBT2020(working);
    }
  }

  if (output_mode == 1) {
    color = Working203ToHLG(working);
  } else {
    highp vec3 linear_color = target_color_space == 2
                                  ? BT2020ToDisplayP3(working)
                                  : BT2020ToBT709(working);
    color = LinearToSRGB(linear_color);
  }
  alpha *= opacity;
  return vec4(color * alpha, alpha);
}
