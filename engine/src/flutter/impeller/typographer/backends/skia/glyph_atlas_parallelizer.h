/*
 * Copyright 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#ifndef FLUTTER_IMPELLER_TYPOGRAPHER_BACKENDS_SKIA_GLYPH_ATLAS_PARALLELIZER_H_
#define FLUTTER_IMPELLER_TYPOGRAPHER_BACKENDS_SKIA_GLYPH_ATLAS_PARALLELIZER_H_

#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

#include "third_party/skia/include/core/SkBitmap.h"

#include "impeller/geometry/rect.h"
#include "impeller/geometry/size.h"

namespace impeller {

struct Flags;

constexpr size_t kOutlineGlyphBaseCost = 1u;
constexpr size_t kColorGlyphBaseCost = 10u;

struct ParallelGlyphWorkItem {
  size_t index;
  Rect pos;
  Rect bounds;
  Size size;
  size_t cost;
};

struct PendingAtlasUpload {
  IRect destination;
  Size size;
  SkBitmap bitmap;
};

class GlyphAtlasParallelizer {
 public:
  using ScanOneFn = std::function<std::optional<ParallelGlyphWorkItem>(size_t)>;

  using RasterizeOneFn =
      std::function<bool(const ParallelGlyphWorkItem&, PendingAtlasUpload&)>;

  static bool Rasterize(
      const Flags& flags,
      size_t start_index,
      size_t end_index,
      const ScanOneFn& scan_fn,
      const RasterizeOneFn& rasterize_fn,
      std::vector<std::optional<PendingAtlasUpload>>& uploads);
};

}  // namespace impeller

#endif  // FLUTTER_IMPELLER_TYPOGRAPHER_BACKENDS_SKIA_GLYPH_ATLAS_PARALLELIZER_H_
