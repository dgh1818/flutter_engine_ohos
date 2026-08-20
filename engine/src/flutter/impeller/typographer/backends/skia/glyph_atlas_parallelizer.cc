/*
 * Copyright 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "impeller/typographer/backends/skia/glyph_atlas_parallelizer.h"

#include <atomic>
#include <cstring>
#include <string>
#include <utility>

#include "flutter/fml/trace_event.h"

#include "impeller/base/flags.h"
#include "impeller/typographer/backends/skia/glyph_raster_worker.h"

namespace impeller {

// Reserve hint for the lane diagnostics string written to the trace buffer.
constexpr size_t kLaneDiagStringReserve = 256u;

bool GlyphAtlasParallelizer::Rasterize(
    const Flags& flags,
    size_t start_index,
    size_t end_index,
    const ScanOneFn& scan_fn,
    const RasterizeOneFn& rasterize_fn,
    std::vector<std::optional<PendingAtlasUpload>>& uploads) {
  std::atomic_bool raster_failed = false;

  std::vector<ParallelGlyphWorkItem> items;
  items.reserve(end_index - start_index);

  for (size_t i = start_index; i < end_index; i++) {
    auto item = scan_fn(i);
    if (item.has_value()) {
      items.push_back(std::move(*item));
    }
  }

  const size_t worker_count = flags.glyph_raster_parallelization
                                  ? GlyphRasterWorkers::Get().GetWorkerCount()
                                  : 0u;
  const bool use_workers = flags.glyph_raster_parallelization &&
                           worker_count > 0u &&
                           items.size() >= kMinParallelGlyphCount;
  const size_t lane_count = use_workers ? worker_count + 1u : 1u;

  if (use_workers) {
    TRACE_EVENT0("impeller", "UpdateAtlasBitmap::ParallelPath");
  } else {
    TRACE_EVENT0("impeller", "UpdateAtlasBitmap::SerialPath");
  }

  std::vector<std::vector<ParallelGlyphWorkItem>> lanes;

  if (lane_count == 1) {
    lanes.resize(1);
    lanes[0] = std::move(items);
  } else {
    auto weights = BuildDefaultLaneWeights(lane_count);
    lanes = WeightedLPTBalance(items, lane_count, weights);

    {
      std::vector<size_t> lane_costs(lane_count, 0u);
      std::vector<size_t> lane_glyph_counts(lane_count, 0u);
      for (size_t l = 0; l < lane_count; l++) {
        lane_glyph_counts[l] = lanes[l].size();
        for (const auto& it : lanes[l]) {
          lane_costs[l] += it.cost;
        }
      }
      std::string diag;
      diag.reserve(kLaneDiagStringReserve);
      for (size_t l = 0; l < lane_count; l++) {
        if (l > 0)
          diag += ",";
        diag += "L" + std::to_string(l) +
                ":n=" + std::to_string(lane_glyph_counts[l]) +
                ",c=" + std::to_string(lane_costs[l]);
      }
      TRACE_EVENT1("impeller", "UpdateAtlasBitmap::LPTBalance", "lanes",
                   diag.c_str());
    }

    size_t total_overflow = ApplyLaneOverflow(lanes, kOverflowRatioPercent);
    if (total_overflow > 0u) {
      const auto overflow_str = std::to_string(total_overflow);
      TRACE_EVENT1("impeller", "UpdateAtlasBitmap::Lane0Overflow", "info",
                   overflow_str.c_str());
    }
  }

  auto rasterize_lane =
      [&](size_t lane_idx,
          const std::vector<ParallelGlyphWorkItem>& lane_items) {
        for (const auto& item : lane_items) {
          if (raster_failed.load()) {
            return;
          }
          PendingAtlasUpload upload;
          if (!rasterize_fn(item, upload)) {
            raster_failed.store(true);
            return;
          }
          uploads[item.index - start_index] = std::move(upload);
        }
      };

  if (lane_count == 1) {
    rasterize_lane(0, lanes[0]);
  } else {
    std::vector<fml::closure> worker_tasks;
    worker_tasks.reserve(worker_count);
    for (size_t i = 0; i < worker_count; i++) {
      worker_tasks.emplace_back(
          [&, i]() { rasterize_lane(i + 1, lanes[i + 1]); });
    }

    GlyphRasterWorkers::Get().Execute([&]() { rasterize_lane(0, lanes[0]); },
                                      worker_tasks);
  }

  return !raster_failed.load();
}

}  // namespace impeller
