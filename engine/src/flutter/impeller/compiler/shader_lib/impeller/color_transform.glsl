// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// HDR color transform helpers for the unified PQ/HLG pipeline
// (PLAN.md: 统一颜色管线 / Phase 3 统一 shader).
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
  if (channel <= 0.04045) {
    return channel / 12.92;
  }
  return pow((channel + kSrgbAlpha) / (1.0 + kSrgbAlpha), 2.4);
}

highp vec3 SRGBToLinear(highp vec3 color) {
  return vec3(SRGBToLinearChannel(color.r),  //
              SRGBToLinearChannel(color.g),  //
              SRGBToLinearChannel(color.b));
}

highp float LinearToSRGBChannel(highp float channel) {
  if (channel <= 0.0031308) {
    return channel * 12.92;
  }
  return (1.0 + kSrgbAlpha) * pow(channel, 1.0 / 2.4) - kSrgbAlpha;
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
const float kHlgGraphicsWhiteSignal = 0.75;
// HLGToLinear(kHlgGraphicsWhiteSignal) for the 1.2 reference system gamma.
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

// HLG inverse OOTF (per-channel practical form, BT.2408 system gamma 1.2):
// relative scene light -> display light normalized so that scene 1.0 maps to
// the HLG reference white (203 cd/m^2). Highlights keep linear headroom > 1.0.
highp vec3 HLGInverseOOTF(highp vec3 scene) {
  return pow(max(scene, 0.0), vec3(1.2));
}

// HLG EOTF (signal -> peak-relative display light).
highp vec3 HLGToLinear(highp vec3 color) {
  return HLGInverseOOTF(HLGInverseOETF(color));
}

// HLG inverse EOTF (peak-relative display light -> signal).
highp vec3 LinearToHLG(highp vec3 working) {
  return HLGOETF(pow(max(working, 0.0), vec3(1.0 / 1.2)));
}

// HLG signal -> linear BT.2020 working space where 1.0 is 203 nits.
highp vec3 HLGToWorking203(highp vec3 color) {
  return HLGToLinear(color) / kHlgGraphicsWhiteLinear;
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
highp float NitsToPQChannel(highp float nits) {
  highp float y = max(nits, 0.0) / kPqMaxLuminance;
  highp float ym1 = pow(y, kPqM1);
  return pow((kPqC1 + kPqC2 * ym1) / (1.0 + kPqC3 * ym1), kPqM2);
}

highp vec3 NitsToPQ(highp vec3 nits) {
  return vec3(NitsToPQChannel(nits.r),  //
              NitsToPQChannel(nits.g),  //
              NitsToPQChannel(nits.b));
}

// PQ EOTF (signal -> linear working space in units of reference white).
// 203 cd/m^2 == 1.0.
highp vec3 PQToLinear(highp vec3 color) {
  return PQToNits(color) / 203.0;
}

// PQ inverse EOTF (linear working space -> signal).
highp vec3 LinearToPQ(highp vec3 working) {
  return NitsToPQ(working * 203.0);
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
  // Rows: [0.7538, 0.1985, 0.0477], [0.0457, 0.9278, 0.0264],
  //       [-0.0011, 0.0240, 0.9771]
  return mat3(0.753845, 0.045742, -0.001111,  //
              0.198453, 0.927774, 0.024014,   //
              0.047702, 0.026483, 0.977097) *
         color;
}

// Linear BT.2020 -> linear Display P3 (D65).
highp vec3 BT2020ToDisplayP3(highp vec3 color) {
  // Rows: [1.4136, -0.3024, -0.1112], [-0.0685, 1.0758, -0.0073],
  //       [0.0165, -0.0449, 1.0284]
  return mat3(1.413578, -0.068533, 0.016452,   //
              -0.302412, 1.075818, -0.044919,  //
              -0.111167, -0.007286, 1.028467) *
         color;
}
