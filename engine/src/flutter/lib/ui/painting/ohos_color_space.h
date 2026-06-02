// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_LIB_UI_PAINTING_OHOS_COLOR_SPACE_H_
#define FLUTTER_LIB_UI_PAINTING_OHOS_COLOR_SPACE_H_

#include <cstdint>

#include "flutter/fml/build_config.h"
#include "impeller/core/texture_descriptor.h"

namespace flutter {

inline impeller::TextureColorSpace OhosColorSpaceToTextureColorSpace(
    uint32_t color_space) {
#ifdef FML_OS_OHOS
  // Keep these values in sync with native_color_space_manager.h.
  constexpr uint32_t kNone = 0;
  constexpr uint32_t kAdobeRGB = 1;
  constexpr uint32_t kDciP3 = 2;
  constexpr uint32_t kDisplayP3 = 3;
  constexpr uint32_t kSRGB = 4;
  constexpr uint32_t kCustom = 5;
  constexpr uint32_t kBT709 = 6;
  constexpr uint32_t kBT601Ebu = 7;
  constexpr uint32_t kBT601SmpteC = 8;
  constexpr uint32_t kBT2020HLG = 9;
  constexpr uint32_t kBT2020PQ = 10;
  constexpr uint32_t kP3HLG = 11;
  constexpr uint32_t kP3PQ = 12;
  constexpr uint32_t kAdobeRGBLimit = 13;
  constexpr uint32_t kDisplayP3Limit = 14;
  constexpr uint32_t kSRGBLimit = 15;
  constexpr uint32_t kBT709Limit = 16;
  constexpr uint32_t kBT601EbuLimit = 17;
  constexpr uint32_t kBT601SmpteCLimit = 18;
  constexpr uint32_t kBT2020HLGLimit = 19;
  constexpr uint32_t kBT2020PQLimit = 20;
  constexpr uint32_t kP3HLGLimit = 21;
  constexpr uint32_t kP3PQLimit = 22;
  constexpr uint32_t kLinearP3 = 23;
  constexpr uint32_t kLinearSRGB = 24;
  constexpr uint32_t kLinearBT2020 = 25;

  switch (color_space) {
    case kDisplayP3:
    case kDisplayP3Limit:
    case kLinearP3:
      return impeller::TextureColorSpace::kDisplayP3;
    case kAdobeRGB:
    case kDciP3:
    case kBT2020HLG:
    case kBT2020PQ:
    case kP3HLG:
    case kP3PQ:
    case kAdobeRGBLimit:
    case kBT2020HLGLimit:
    case kBT2020PQLimit:
    case kP3HLGLimit:
    case kP3PQLimit:
    case kLinearBT2020:
      return impeller::TextureColorSpace::kExtendedSRGB;
    case kNone:
    case kSRGB:
    case kCustom:
    case kBT709:
    case kBT601Ebu:
    case kBT601SmpteC:
    case kSRGBLimit:
    case kBT709Limit:
    case kBT601EbuLimit:
    case kBT601SmpteCLimit:
    case kLinearSRGB:
    default:
      return impeller::TextureColorSpace::kSRGB;
  }
#else
  return impeller::TextureColorSpace::kSRGB;
#endif
}

}  // namespace flutter

#endif  // FLUTTER_LIB_UI_PAINTING_OHOS_COLOR_SPACE_H_
