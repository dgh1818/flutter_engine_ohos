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

constexpr uint32_t kDisplayP3ColorSpaces[] = {
    kDisplayP3,
    kDisplayP3Limit,
    kLinearP3,
};

constexpr uint32_t kExtendedSRGBColorSpaces[] = {
    kAdobeRGB,      kDciP3,      kBT2020HLG,     kBT2020PQ,
    kP3HLG,         kP3PQ,       kAdobeRGBLimit, kBT2020HLGLimit,
    kBT2020PQLimit, kP3HLGLimit, kP3PQLimit,     kLinearBT2020,
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
  if (ContainsColorSpace(colorSpace, kExtendedSRGBColorSpaces)) {
    return impeller::TextureColorSpace::kExtendedSRGB;
  }
  return impeller::TextureColorSpace::kSRGB;
}
#endif  // FML_OS_OHOS

}  // namespace ohos_color_space_internal

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
