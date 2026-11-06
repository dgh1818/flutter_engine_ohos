// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_LIB_UI_PAINTING_OHOS_COLOR_SPACE_H_
#define FLUTTER_LIB_UI_PAINTING_OHOS_COLOR_SPACE_H_

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "flutter/fml/build_config.h"
#include "impeller/core/texture_descriptor.h"

namespace flutter {
namespace ohos_color_space_internal {

#ifdef FML_OS_OHOS
// Keep these values in sync with native_color_space_manager.h.
constexpr uint32_t kAdobeRGB = 1;
constexpr uint32_t kDciP3 = 2;
constexpr uint32_t kDisplayP3 = 3;
constexpr uint32_t kBT2020HLG = 9;
constexpr uint32_t kBT2020PQ = 10;
constexpr uint32_t kP3HLG = 11;
constexpr uint32_t kP3PQ = 12;
constexpr uint32_t kAdobeRGBLimit = 13;
constexpr uint32_t kDisplayP3Limit = 14;
constexpr uint32_t kBT2020HLGLimit = 19;
constexpr uint32_t kBT2020PQLimit = 20;
constexpr uint32_t kP3HLGLimit = 21;
constexpr uint32_t kP3PQLimit = 22;
constexpr uint32_t kLinearP3 = 23;
constexpr uint32_t kLinearBT2020 = 25;
// Internal marker for generated HLG pixels when primaries are not known.
constexpr uint32_t kHlgUnknownPrimaries = 0x7fffff00u;

constexpr uint32_t kDisplayP3ColorSpaces[] = {
    kDisplayP3,
    kDisplayP3Limit,
    kLinearP3,
};

constexpr uint32_t kExtendedSRGBColorSpaces[] = {
    kAdobeRGB,
    kDciP3,
    kAdobeRGBLimit,
    kLinearBT2020,
};

template <std::size_t N>
inline bool ContainsColorSpace(uint32_t colorSpace,
                               const uint32_t (&colorSpaces)[N]) {
  return std::find(colorSpaces, colorSpaces + N, colorSpace) != colorSpaces + N;
}

inline impeller::TextureColorSpace TextureColorSpaceForOhos(
    uint32_t colorSpace) {
  if (ContainsColorSpace(colorSpace, kDisplayP3ColorSpaces)) {
    return impeller::TextureColorSpace::kDisplayP3;
  }
  // Keep the BT.2100 transfer function distinct so the HDR shader pipeline
  // picks the correct EOTF (HLG vs PQ); the 10-bit container cannot express
  // the difference on its own.
  constexpr uint32_t kBT2020PqColorSpaces[] = {kBT2020PQ, kBT2020PQLimit};
  constexpr uint32_t kBT2020HlgColorSpaces[] = {kBT2020HLG, kBT2020HLGLimit};
  constexpr uint32_t kP3PqColorSpaces[] = {kP3PQ, kP3PQLimit};
  constexpr uint32_t kP3HlgColorSpaces[] = {kP3HLG, kP3HLGLimit};
  if (ContainsColorSpace(colorSpace, kBT2020PqColorSpaces)) {
    return impeller::TextureColorSpace::kBT2020PQ;
  }
  if (ContainsColorSpace(colorSpace, kBT2020HlgColorSpaces)) {
    return impeller::TextureColorSpace::kBT2020HLG;
  }
  if (ContainsColorSpace(colorSpace, kP3PqColorSpaces)) {
    return impeller::TextureColorSpace::kDisplayP3PQ;
  }
  if (ContainsColorSpace(colorSpace, kP3HlgColorSpaces)) {
    return impeller::TextureColorSpace::kDisplayP3HLG;
  }
  if (colorSpace == kHlgUnknownPrimaries) {
    return impeller::TextureColorSpace::kHLG;
  }
  if (ContainsColorSpace(colorSpace, kExtendedSRGBColorSpaces)) {
    return impeller::TextureColorSpace::kExtendedSRGB;
  }
  return impeller::TextureColorSpace::kSRGB;
}
#endif  // FML_OS_OHOS

}  // namespace ohos_color_space_internal

inline bool OhosColorSpaceIsLimitedHdr(uint32_t colorSpace) {
#ifdef FML_OS_OHOS
  using namespace ohos_color_space_internal;
  return colorSpace == kBT2020HLGLimit || colorSpace == kBT2020PQLimit ||
         colorSpace == kP3HLGLimit || colorSpace == kP3PQLimit;
#else
  return false;
#endif
}

inline impeller::TextureColorSpace OhosColorSpaceToTextureColorSpace(
    uint32_t colorSpace) {
#ifdef FML_OS_OHOS
  return ohos_color_space_internal::TextureColorSpaceForOhos(colorSpace);
#else
  return impeller::TextureColorSpace::kSRGB;
#endif
}

}  // namespace flutter

#endif  // FLUTTER_LIB_UI_PAINTING_OHOS_COLOR_SPACE_H_
