/*
 * Copyright 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#ifndef FLUTTER_IMPELLER_TYPOGRAPHER_BACKENDS_SKIA_GLYPH_RASTER_WORKER_H_
#define FLUTTER_IMPELLER_TYPOGRAPHER_BACKENDS_SKIA_GLYPH_RASTER_WORKER_H_

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#ifdef FML_OS_OHOS
#include <qos/qos.h>
#endif

#include "flutter/fml/closure.h"
#include "flutter/fml/logging.h"
#include "flutter/fml/synchronization/count_down_latch.h"
#include "flutter/fml/thread.h"
#include "flutter/fml/trace_event.h"
#include "flutter/third_party/skia/src/ports/SkGlyphRasterWorkerSlot.h"

namespace impeller {

constexpr size_t kGlyphRasterWorkerCount = 2;
constexpr size_t kMinParallelGlyphCount = 16u;

constexpr double kRasterLaneWeightRatio = 5.0;
constexpr double kWorkerLaneWeightRatio = 2.0;

constexpr size_t kOverflowRatioPercent = 30u;

#ifdef FML_OS_OHOS
inline void OHOSPlatformThreadConfigSetter(
    const fml::Thread::ThreadConfig& config) {
  fml::Thread::SetCurrentThreadName(config);
  switch (config.priority) {
    case fml::Thread::ThreadPriority::kBackground: {
      int ret = OH_QoS_SetThreadQoS(QoS_Level::QOS_BACKGROUND);
      if (ret != 0) {
        FML_LOG(WARNING) << "qos set background failed:" << ret
                         << ", fallback to QOS_DEFAULT, tid:" << gettid();
        OH_QoS_SetThreadQoS(QoS_Level::QOS_DEFAULT);
      } else {
        FML_LOG(INFO) << "qos set background ok, tid:" << gettid();
      }
      break;
    }
    case fml::Thread::ThreadPriority::kDisplay: {
      int ret = OH_QoS_SetThreadQoS(QoS_Level::QOS_USER_INTERACTIVE);
      if (ret != 0) {
        FML_LOG(WARNING) << "qos set display failed:" << ret
                         << ", fallback to QOS_USER_INITIATED, tid:"
                         << gettid();
        OH_QoS_SetThreadQoS(QoS_Level::QOS_USER_INITIATED);
      } else {
        FML_LOG(INFO) << "qos set display ok, tid:" << gettid();
      }
      break;
    }
    case fml::Thread::ThreadPriority::kRaster: {
      int ret = OH_QoS_SetThreadQoS(QoS_Level::QOS_USER_INTERACTIVE);
      if (ret != 0) {
        FML_LOG(WARNING) << "qos set raster failed:" << ret
                         << ", fallback to QOS_USER_INITIATED, tid:"
                         << gettid();
        OH_QoS_SetThreadQoS(QoS_Level::QOS_USER_INITIATED);
      } else {
        FML_LOG(INFO) << "qos set raster ok, tid:" << gettid();
      }
      break;
    }
    default: {
      int ret = OH_QoS_SetThreadQoS(QoS_Level::QOS_DEFAULT);
      FML_LOG(INFO) << "qos set default result:" << ret << ", tid:" << gettid();
    }
  }
}
#endif

class ScopedGlyphRasterWorkerSlot {
 public:
  explicit ScopedGlyphRasterWorkerSlot(int slot)
      : previous_(SkGetCurrentGlyphRasterWorkerSlot()) {
    SkSetCurrentGlyphRasterWorkerSlot(slot);
  }

  ~ScopedGlyphRasterWorkerSlot() {
    SkSetCurrentGlyphRasterWorkerSlot(previous_);
  }

 private:
  int previous_;
};

class GlyphRasterWorkers {
 public:
  static GlyphRasterWorkers& Get() {
    static GlyphRasterWorkers instance;
    return instance;
  }

  size_t GetWorkerCount() const { return workers_.size(); }

  void Execute(const fml::closure& raster_thread_task,
               const std::vector<fml::closure>& worker_tasks) {
    size_t worker_count = workers_.size();
    FML_DCHECK(worker_tasks.size() == worker_count);

    if (worker_count == 0) {
      ScopedGlyphRasterWorkerSlot slot_guard(kSkGlyphRasterWorkerSlotNone);
      if (raster_thread_task) {
        raster_thread_task();
      }
      return;
    }

    const auto worker_count_str = std::to_string(worker_count);
    TRACE_EVENT1("impeller", "GlyphRasterWorkers::Execute", "worker_count",
                worker_count_str.c_str());

    fml::CountDownLatch latch(worker_count);
    for (size_t i = 0; i < worker_count; i++) {
      workers_[i]->GetTaskRunner()->PostTask([&, i]() {
        ScopedGlyphRasterWorkerSlot slot_guard(worker_slots_[i]);
        fml::ScopedCleanupClosure count_down([&latch]() { latch.CountDown(); });
        if (worker_tasks[i]) {
          worker_tasks[i]();
        }
      });
    }

    ScopedGlyphRasterWorkerSlot slot_guard(kSkGlyphRasterWorkerSlotNone);
    if (raster_thread_task) {
      raster_thread_task();
    }
    TRACE_EVENT0("impeller", "GlyphRasterWorkers::WaitForWorkers");
    latch.Wait();
  }

 private:
  GlyphRasterWorkers() {
    size_t worker_count = kGlyphRasterWorkerCount;
    workers_.reserve(worker_count);
    worker_slots_.reserve(worker_count);
    for (size_t i = 0; i < worker_count; i++) {
      int slot = SkRegisterGlyphRasterWorkerSlot();
      if (slot < 0) {
        FML_LOG(WARNING) << "Max glyph raster worker slots reached ("
                         << kSkGlyphRasterWorkerSlotCount
                         << "), remaining workers fall back to raster thread";
        break;
      }
      worker_slots_.push_back(slot);
      std::string thread_name = "GlyphRaster" + std::to_string(slot);
#ifdef FML_OS_OHOS
      workers_.emplace_back(std::make_unique<fml::Thread>(
          OHOSPlatformThreadConfigSetter,
          fml::Thread::ThreadConfig(thread_name,
                                    fml::Thread::ThreadPriority::kRaster)));
#else
      workers_.emplace_back(std::make_unique<fml::Thread>(
          fml::Thread::SetCurrentThreadName,
          fml::Thread::ThreadConfig(thread_name,
                                    fml::Thread::ThreadPriority::kRaster)));
#endif
    }
  }

  std::vector<std::unique_ptr<fml::Thread>> workers_;
  std::vector<int> worker_slots_;
};

struct GlyphWorkItem {
  size_t index;
  size_t cost;
};

template <typename T>
std::vector<std::vector<T>> LPTBalance(std::vector<T>& items,
                                       size_t lane_count) {
  std::vector<std::vector<T>> lanes(lane_count);
  if (lane_count == 0) {
    return lanes;
  }
  for (auto& lane : lanes) {
    lane.reserve(items.size() / lane_count + 1u);
  }

  if (lane_count == 1) {
    lanes[0] = std::move(items);
    return lanes;
  }

  std::sort(items.begin(), items.end(),
            [](const T& a, const T& b) { return a.cost > b.cost; });

  std::vector<size_t> lane_costs(lane_count, 0u);
  for (const auto& item : items) {
    size_t min_lane = 0;
    for (size_t lane = 1; lane < lane_count; lane++) {
      if (lane_costs[lane] < lane_costs[min_lane]) {
        min_lane = lane;
      }
    }
    lanes[min_lane].push_back(item);
    lane_costs[min_lane] += item.cost;
  }
  return lanes;
}

template <typename T>
std::vector<std::vector<T>> WeightedLPTBalance(
    std::vector<T>& items,
    size_t lane_count,
    const std::vector<double>& weights) {
  std::vector<std::vector<T>> lanes(lane_count);
  if (lane_count == 0 || items.empty()) {
    return lanes;
  }
  FML_DCHECK(weights.size() == lane_count);

  for (auto& lane : lanes) {
    lane.reserve(items.size() / lane_count + 1u);
  }

  if (lane_count == 1) {
    lanes[0] = std::move(items);
    return lanes;
  }

  std::sort(items.begin(), items.end(),
            [](const T& a, const T& b) { return a.cost > b.cost; });

  std::vector<size_t> lane_costs(lane_count, 0u);
  for (const auto& item : items) {
    size_t min_lane = 0;
    double min_weighted_cost = static_cast<double>(lane_costs[0]) / weights[0];
    for (size_t lane = 1; lane < lane_count; lane++) {
      double weighted_cost =
          static_cast<double>(lane_costs[lane]) / weights[lane];
      if (weighted_cost < min_weighted_cost) {
        min_lane = lane;
        min_weighted_cost = weighted_cost;
      }
    }
    lanes[min_lane].push_back(item);
    lane_costs[min_lane] += item.cost;
  }
  return lanes;
}

template <typename T>
size_t ApplyLaneOverflow(std::vector<std::vector<T>>& lanes,
                         size_t overflow_ratio_percent) {
  if (overflow_ratio_percent == 0u || lanes.size() <= 1u) {
    return 0u;
  }
  size_t total_overflow = 0u;
  for (size_t l = 1; l < lanes.size(); l++) {
    if (lanes[l].empty()) {
      continue;
    }
    size_t overflow_count =
        (lanes[l].size() * overflow_ratio_percent + 99u) / 100u;
    overflow_count = std::min(overflow_count, lanes[l].size() - 1u);
    if (overflow_count > 0u) {
      auto& worker_lane = lanes[l];
      auto overflow_begin = worker_lane.end() - overflow_count;
      lanes[0].insert(lanes[0].end(), overflow_begin, worker_lane.end());
      worker_lane.erase(overflow_begin, worker_lane.end());
      total_overflow += overflow_count;
    }
  }
  return total_overflow;
}

inline std::vector<double> BuildDefaultLaneWeights(size_t lane_count) {
  std::vector<double> weights(lane_count);
  if (lane_count == 0) {
    return weights;
  }
  weights[0] = kRasterLaneWeightRatio;
  for (size_t i = 1; i < lane_count; i++) {
    weights[i] = kWorkerLaneWeightRatio;
  }
  return weights;
}

}  // namespace impeller

#endif  // FLUTTER_IMPELLER_TYPOGRAPHER_BACKENDS_SKIA_GLYPH_RASTER_WORKER_H_
