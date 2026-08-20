/*
 * Copyright 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "impeller/base/flags.h"
#include "impeller/typographer/backends/skia/glyph_atlas_parallelizer.h"
#include "impeller/typographer/backends/skia/glyph_raster_worker.h"

#include <atomic>
#include <chrono>
#include <set>
#include <thread>
#include <vector>

#include "flutter/fml/thread.h"
#include "flutter/testing/testing.h"
#include "gtest/gtest.h"
#include "third_party/skia/src/ports/SkGlyphRasterWorkerSlot.h"

namespace impeller {
namespace testing {

TEST(SkGlyphRasterWorkerSlotTest, DefaultSlotIsNone) {
  EXPECT_EQ(SkGetCurrentGlyphRasterWorkerSlot(), kSkGlyphRasterWorkerSlotNone);
}

TEST(SkGlyphRasterWorkerSlotTest, SetAndGetSlot) {
  SkSetCurrentGlyphRasterWorkerSlot(0);
  EXPECT_EQ(SkGetCurrentGlyphRasterWorkerSlot(), 0);

  SkSetCurrentGlyphRasterWorkerSlot(1);
  EXPECT_EQ(SkGetCurrentGlyphRasterWorkerSlot(), 1);

  SkSetCurrentGlyphRasterWorkerSlot(2);
  EXPECT_EQ(SkGetCurrentGlyphRasterWorkerSlot(), 2);

  SkSetCurrentGlyphRasterWorkerSlot(kSkGlyphRasterWorkerSlotNone);
  EXPECT_EQ(SkGetCurrentGlyphRasterWorkerSlot(), kSkGlyphRasterWorkerSlotNone);
}

TEST(SkGlyphRasterWorkerSlotTest, SlotCountIsThree) {
  EXPECT_EQ(kSkGlyphRasterWorkerSlotCount, 3);
}

TEST(SkGlyphRasterWorkerSlotTest, RegisterReturnsAscendingSlots) {
  auto& counter = SkGlyphRasterWorkerSlotCounter();
  int saved = counter.load();

  int first = SkRegisterGlyphRasterWorkerSlot();
  EXPECT_GE(first, 0);
  EXPECT_LT(first, kSkGlyphRasterWorkerSlotCount);

  int second = SkRegisterGlyphRasterWorkerSlot();
  if (second >= 0) {
    EXPECT_EQ(second, first + 1);
  }

  while (counter.load() < kSkGlyphRasterWorkerSlotCount) {
    SkRegisterGlyphRasterWorkerSlot();
  }
  EXPECT_EQ(SkRegisterGlyphRasterWorkerSlot(), kSkGlyphRasterWorkerSlotNone);

  counter.store(saved);
}

TEST(SkGlyphRasterWorkerSlotTest, ThreadLocalIsolation) {
  SkSetCurrentGlyphRasterWorkerSlot(0);
  EXPECT_EQ(SkGetCurrentGlyphRasterWorkerSlot(), 0);

  std::atomic_bool worker_checked = false;
  fml::Thread thread("slot_test_thread");
  thread.GetTaskRunner()->PostTask([&]() {
    EXPECT_EQ(SkGetCurrentGlyphRasterWorkerSlot(),
              kSkGlyphRasterWorkerSlotNone);
    SkSetCurrentGlyphRasterWorkerSlot(1);
    EXPECT_EQ(SkGetCurrentGlyphRasterWorkerSlot(), 1);
    worker_checked.store(true);
  });

  while (!worker_checked.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  EXPECT_EQ(SkGetCurrentGlyphRasterWorkerSlot(), 0);

  SkSetCurrentGlyphRasterWorkerSlot(kSkGlyphRasterWorkerSlotNone);
}

TEST(ScopedGlyphRasterWorkerSlotTest, RestoresPreviousSlot) {
  SkSetCurrentGlyphRasterWorkerSlot(kSkGlyphRasterWorkerSlotNone);
  EXPECT_EQ(SkGetCurrentGlyphRasterWorkerSlot(), kSkGlyphRasterWorkerSlotNone);

  {
    ScopedGlyphRasterWorkerSlot guard(0);
    EXPECT_EQ(SkGetCurrentGlyphRasterWorkerSlot(), 0);
  }

  EXPECT_EQ(SkGetCurrentGlyphRasterWorkerSlot(), kSkGlyphRasterWorkerSlotNone);
}

TEST(ScopedGlyphRasterWorkerSlotTest, NestedRestores) {
  SkSetCurrentGlyphRasterWorkerSlot(kSkGlyphRasterWorkerSlotNone);

  {
    ScopedGlyphRasterWorkerSlot guard1(0);
    EXPECT_EQ(SkGetCurrentGlyphRasterWorkerSlot(), 0);

    {
      ScopedGlyphRasterWorkerSlot guard2(1);
      EXPECT_EQ(SkGetCurrentGlyphRasterWorkerSlot(), 1);
    }

    EXPECT_EQ(SkGetCurrentGlyphRasterWorkerSlot(), 0);
  }

  EXPECT_EQ(SkGetCurrentGlyphRasterWorkerSlot(), kSkGlyphRasterWorkerSlotNone);
}

TEST(ScopedGlyphRasterWorkerSlotTest, RestoreFromWorkerSlot) {
  SkSetCurrentGlyphRasterWorkerSlot(2);

  {
    ScopedGlyphRasterWorkerSlot guard(0);
    EXPECT_EQ(SkGetCurrentGlyphRasterWorkerSlot(), 0);
  }

  EXPECT_EQ(SkGetCurrentGlyphRasterWorkerSlot(), 2);
  SkSetCurrentGlyphRasterWorkerSlot(kSkGlyphRasterWorkerSlotNone);
}

TEST(GlyphRasterWorkersTest, GetWorkerCountReturnsTwo) {
  EXPECT_EQ(GlyphRasterWorkers::Get().GetWorkerCount(), 2u);
}

TEST(GlyphRasterWorkersTest, ExecuteRunsRasterTaskAndWorkerTasks) {
  std::atomic_bool raster_task_ran = false;
  std::atomic_bool worker0_task_ran = false;
  std::atomic_bool worker1_task_ran = false;

  size_t worker_count = GlyphRasterWorkers::Get().GetWorkerCount();
  std::vector<fml::closure> worker_tasks;
  worker_tasks.emplace_back([&]() { worker0_task_ran.store(true); });
  if (worker_count > 1) {
    worker_tasks.emplace_back([&]() { worker1_task_ran.store(true); });
  }

  GlyphRasterWorkers::Get().Execute([&]() { raster_task_ran.store(true); },
                                    worker_tasks);

  EXPECT_TRUE(raster_task_ran.load());
  EXPECT_TRUE(worker0_task_ran.load());
  if (worker_count > 1) {
    EXPECT_TRUE(worker1_task_ran.load());
  }
}

TEST(GlyphRasterWorkersTest, ExecuteRunsAllTasks) {
  std::atomic_size_t raster_count = 0;
  std::atomic_size_t worker0_count = 0;
  std::atomic_size_t worker1_count = 0;

  size_t worker_count = GlyphRasterWorkers::Get().GetWorkerCount();
  std::vector<fml::closure> worker_tasks;
  worker_tasks.emplace_back([&]() { worker0_count.fetch_add(1); });
  if (worker_count > 1) {
    worker_tasks.emplace_back([&]() { worker1_count.fetch_add(1); });
  }

  GlyphRasterWorkers::Get().Execute([&]() { raster_count.fetch_add(1); },
                                    worker_tasks);

  EXPECT_EQ(raster_count.load(), 1u);
  EXPECT_EQ(worker0_count.load(), 1u);
  if (worker_count > 1) {
    EXPECT_EQ(worker1_count.load(), 1u);
  }
}

TEST(GlyphRasterWorkersTest, ExecuteSetsCorrectWorkerSlot) {
  std::atomic<int> raster_slot = kSkGlyphRasterWorkerSlotNone;
  std::atomic<int> worker0_slot = kSkGlyphRasterWorkerSlotNone;
  std::atomic<int> worker1_slot = kSkGlyphRasterWorkerSlotNone;

  size_t worker_count = GlyphRasterWorkers::Get().GetWorkerCount();
  std::vector<fml::closure> worker_tasks;
  worker_tasks.emplace_back(
      [&]() { worker0_slot.store(SkGetCurrentGlyphRasterWorkerSlot()); });
  if (worker_count > 1) {
    worker_tasks.emplace_back(
        [&]() { worker1_slot.store(SkGetCurrentGlyphRasterWorkerSlot()); });
  }

  GlyphRasterWorkers::Get().Execute(
      [&]() { raster_slot.store(SkGetCurrentGlyphRasterWorkerSlot()); },
      worker_tasks);

  EXPECT_EQ(raster_slot.load(), kSkGlyphRasterWorkerSlotNone);
  EXPECT_GE(worker0_slot.load(), 0);
  EXPECT_LT(worker0_slot.load(), kSkGlyphRasterWorkerSlotCount);
  if (worker_count > 1) {
    EXPECT_GE(worker1_slot.load(), 0);
    EXPECT_LT(worker1_slot.load(), kSkGlyphRasterWorkerSlotCount);
    EXPECT_NE(worker0_slot.load(), worker1_slot.load());
  }
}

TEST(GlyphRasterWorkersTest, ExecuteWithCallableButNoOpTasksDoesNotCrash) {
  size_t worker_count = GlyphRasterWorkers::Get().GetWorkerCount();
  std::vector<fml::closure> worker_tasks;
  for (size_t i = 0; i < worker_count; i++) {
    worker_tasks.emplace_back([]() {});
  }
  GlyphRasterWorkers::Get().Execute([]() {}, worker_tasks);
}

TEST(LPTBalanceTest, EmptyItems) {
  std::vector<GlyphWorkItem> items;
  auto lanes = LPTBalance(items, 3u);
  EXPECT_EQ(lanes.size(), 3u);
  for (const auto& lane : lanes) {
    EXPECT_EQ(lane.size(), 0u);
  }
}

TEST(LPTBalanceTest, ZeroLaneCount) {
  std::vector<GlyphWorkItem> items = {{0, 10}};
  auto lanes = LPTBalance(items, 0u);
  EXPECT_EQ(lanes.size(), 0u);
}

TEST(LPTBalanceTest, SingleLane) {
  std::vector<GlyphWorkItem> items = {{0, 10}, {1, 20}, {2, 30}};
  auto lanes = LPTBalance(items, 1u);
  EXPECT_EQ(lanes.size(), 1u);
  EXPECT_EQ(lanes[0].size(), 3u);
  EXPECT_EQ(lanes[0][0].index, 0u);
  EXPECT_EQ(lanes[0][1].index, 1u);
  EXPECT_EQ(lanes[0][2].index, 2u);
}

TEST(LPTBalanceTest, TwoLanesEvenDistribution) {
  std::vector<GlyphWorkItem> items = {{0, 10}, {1, 10}, {2, 10}, {3, 10}};
  auto lanes = LPTBalance(items, 2u);
  EXPECT_EQ(lanes.size(), 2u);

  size_t total = lanes[0].size() + lanes[1].size();
  EXPECT_EQ(total, 4u);

  size_t cost0 = 0, cost1 = 0;
  for (const auto& item : lanes[0])
    cost0 += item.cost;
  for (const auto& item : lanes[1])
    cost1 += item.cost;
  EXPECT_EQ(cost0, 20u);
  EXPECT_EQ(cost1, 20u);
}

TEST(LPTBalanceTest, LPTSortsByDescendingCost) {
  std::vector<GlyphWorkItem> items = {{0, 5}, {1, 100}, {2, 10}, {3, 50}};
  auto lanes = LPTBalance(items, 2u);
  EXPECT_EQ(lanes.size(), 2u);

  size_t total = lanes[0].size() + lanes[1].size();
  EXPECT_EQ(total, 4u);

  size_t cost0 = 0, cost1 = 0;
  for (const auto& item : lanes[0])
    cost0 += item.cost;
  for (const auto& item : lanes[1])
    cost1 += item.cost;

  int64_t diff = static_cast<int64_t>(cost0) - static_cast<int64_t>(cost1);
  EXPECT_LE(std::abs(diff), static_cast<int64_t>(cost0 + cost1) / 2);
}

TEST(LPTBalanceTest, ThreeLanesWithVaryingCosts) {
  std::vector<GlyphWorkItem> items = {
      {0, 100}, {1, 90}, {2, 80}, {3, 70}, {4, 60},
      {5, 50},  {6, 40}, {7, 30}, {8, 20}, {9, 10},
  };
  auto lanes = LPTBalance(items, 3u);
  EXPECT_EQ(lanes.size(), 3u);

  size_t total = 0;
  for (const auto& lane : lanes) {
    total += lane.size();
  }
  EXPECT_EQ(total, 10u);

  std::vector<size_t> costs(3, 0);
  for (size_t i = 0; i < 3; i++) {
    for (const auto& item : lanes[i]) {
      costs[i] += item.cost;
    }
  }
  EXPECT_EQ(costs[0] + costs[1] + costs[2], 550u);
  size_t max_cost = *std::max_element(costs.begin(), costs.end());
  size_t min_cost = *std::min_element(costs.begin(), costs.end());
  EXPECT_LE(max_cost - min_cost, 100u);
}

TEST(LPTBalanceTest, SingleItem) {
  std::vector<GlyphWorkItem> items = {{0, 42}};
  auto lanes = LPTBalance(items, 3u);
  EXPECT_EQ(lanes.size(), 3u);
  EXPECT_EQ(lanes[0].size(), 1u);
  EXPECT_EQ(lanes[0][0].cost, 42u);
  EXPECT_EQ(lanes[1].size(), 0u);
  EXPECT_EQ(lanes[2].size(), 0u);
}

TEST(LPTBalanceTest, AllItemsSameCost) {
  std::vector<GlyphWorkItem> items = {
      {0, 10}, {1, 10}, {2, 10}, {3, 10}, {4, 10}, {5, 10},
  };
  auto lanes = LPTBalance(items, 3u);
  EXPECT_EQ(lanes.size(), 3u);

  size_t total = 0;
  for (const auto& lane : lanes) {
    total += lane.size();
  }
  EXPECT_EQ(total, 6u);

  std::vector<size_t> costs(3, 0);
  for (size_t i = 0; i < 3; i++) {
    for (const auto& item : lanes[i]) {
      costs[i] += item.cost;
    }
  }
  EXPECT_EQ(costs[0], 20u);
  EXPECT_EQ(costs[1], 20u);
  EXPECT_EQ(costs[2], 20u);
}

TEST(LPTBalanceTest, OneLargeItemDominates) {
  std::vector<GlyphWorkItem> items = {
      {0, 1000},
      {1, 1},
      {2, 1},
      {3, 1},
  };
  auto lanes = LPTBalance(items, 2u);
  EXPECT_EQ(lanes.size(), 2u);

  std::vector<size_t> costs(2, 0);
  for (size_t i = 0; i < 2; i++) {
    for (const auto& item : lanes[i]) {
      costs[i] += item.cost;
    }
  }
  EXPECT_EQ(costs[0] + costs[1], 1003u);
  EXPECT_GE(costs[0], 1000u);
}

TEST(ConstantsTest, DefaultWorkerCountIsTwo) {
  EXPECT_EQ(kGlyphRasterWorkerCount, 2u);
}

TEST(ConstantsTest, DefaultMinParallelGlyphCountIsSixteen) {
  EXPECT_EQ(kMinParallelGlyphCount, 16u);
}

TEST(ConstantsTest, DefaultRasterLaneWeightRatioIsFive) {
  EXPECT_EQ(kRasterLaneWeightRatio, 5.0);
}

TEST(ConstantsTest, DefaultWorkerLaneWeightRatioIsTwo) {
  EXPECT_EQ(kWorkerLaneWeightRatio, 2.0);
}

TEST(ConstantsTest, DefaultOverflowRatioIsThirty) {
  EXPECT_EQ(kOverflowRatioPercent, 30u);
}

TEST(CostConstantsTest, OutlineGlyphBaseCostIsOne) {
  EXPECT_EQ(kOutlineGlyphBaseCost, 1u);
}

TEST(CostConstantsTest, ColorGlyphBaseCostIsTen) {
  EXPECT_EQ(kColorGlyphBaseCost, 10u);
}

TEST(CostConstantsTest, ColorCostGreaterThanOutlineCost) {
  EXPECT_GT(kColorGlyphBaseCost, kOutlineGlyphBaseCost);
}

TEST(FlagsTest, DefaultGlyphRasterParallelizationIsFalse) {
  Flags flags;
  EXPECT_FALSE(flags.glyph_raster_parallelization);
}

TEST(FlagsTest, CanEnableGlyphRasterParallelization) {
  Flags flags;
  flags.glyph_raster_parallelization = true;
  EXPECT_TRUE(flags.glyph_raster_parallelization);
}

TEST(BuildDefaultLaneWeightsTest, ZeroLaneCount) {
  auto weights = BuildDefaultLaneWeights(0u);
  EXPECT_EQ(weights.size(), 0u);
}

TEST(BuildDefaultLaneWeightsTest, SingleLane) {
  auto weights = BuildDefaultLaneWeights(1u);
  EXPECT_EQ(weights.size(), 1u);
  EXPECT_DOUBLE_EQ(weights[0], 5.0);
}

TEST(BuildDefaultLaneWeightsTest, ThreeLanes) {
  auto weights = BuildDefaultLaneWeights(3u);
  EXPECT_EQ(weights.size(), 3u);
  EXPECT_DOUBLE_EQ(weights[0], 5.0);
  EXPECT_DOUBLE_EQ(weights[1], 2.0);
  EXPECT_DOUBLE_EQ(weights[2], 2.0);
}

TEST(BuildDefaultLaneWeightsTest, FourLanes) {
  auto weights = BuildDefaultLaneWeights(4u);
  EXPECT_EQ(weights.size(), 4u);
  EXPECT_DOUBLE_EQ(weights[0], 5.0);
  EXPECT_DOUBLE_EQ(weights[1], 2.0);
  EXPECT_DOUBLE_EQ(weights[2], 2.0);
  EXPECT_DOUBLE_EQ(weights[3], 2.0);
}

TEST(WeightedLPTBalanceTest, EmptyItems) {
  std::vector<GlyphWorkItem> items;
  std::vector<double> weights = {5.0, 2.0, 2.0};
  auto lanes = WeightedLPTBalance(items, 3u, weights);
  EXPECT_EQ(lanes.size(), 3u);
  for (const auto& lane : lanes) {
    EXPECT_EQ(lane.size(), 0u);
  }
}

TEST(WeightedLPTBalanceTest, ZeroLaneCount) {
  std::vector<GlyphWorkItem> items = {{0, 10}};
  std::vector<double> weights;
  auto lanes = WeightedLPTBalance(items, 0u, weights);
  EXPECT_EQ(lanes.size(), 0u);
}

TEST(WeightedLPTBalanceTest, SingleLane) {
  std::vector<GlyphWorkItem> items = {{0, 10}, {1, 20}, {2, 30}};
  std::vector<double> weights = {5.0};
  auto lanes = WeightedLPTBalance(items, 1u, weights);
  EXPECT_EQ(lanes.size(), 1u);
  EXPECT_EQ(lanes[0].size(), 3u);
}

TEST(WeightedLPTBalanceTest, EqualWeightsBehaveLikeLPT) {
  std::vector<GlyphWorkItem> items = {{0, 10}, {1, 10}, {2, 10}, {3, 10}};
  std::vector<double> weights = {1.0, 1.0};
  auto lanes = WeightedLPTBalance(items, 2u, weights);
  EXPECT_EQ(lanes.size(), 2u);

  size_t cost0 = 0, cost1 = 0;
  for (const auto& item : lanes[0])
    cost0 += item.cost;
  for (const auto& item : lanes[1])
    cost1 += item.cost;
  EXPECT_EQ(cost0, 20u);
  EXPECT_EQ(cost1, 20u);
}

TEST(WeightedLPTBalanceTest, RasterLaneGetsMoreWorkWith522) {
  std::vector<GlyphWorkItem> items = {
      {0, 100}, {1, 90}, {2, 80}, {3, 70}, {4, 60},
      {5, 50},  {6, 40}, {7, 30}, {8, 20}, {9, 10},
  };
  std::vector<double> weights = {5.0, 2.0, 2.0};
  auto lanes = WeightedLPTBalance(items, 3u, weights);
  EXPECT_EQ(lanes.size(), 3u);

  std::vector<size_t> costs(3, 0);
  for (size_t i = 0; i < 3; i++) {
    for (const auto& item : lanes[i]) {
      costs[i] += item.cost;
    }
  }

  EXPECT_EQ(costs[0] + costs[1] + costs[2], 550u);
  EXPECT_GT(costs[0], costs[1]);
  EXPECT_GT(costs[0], costs[2]);
  EXPECT_EQ(costs[1], costs[2]);
}

TEST(WeightedLPTBalanceTest, RasterLaneWeightedCostSlightlyAboveWorkers) {
  std::vector<GlyphWorkItem> items = {
      {0, 10}, {1, 10}, {2, 10}, {3, 10}, {4, 10}, {5, 10},
  };
  std::vector<double> weights = {5.0, 2.0, 2.0};
  auto lanes = WeightedLPTBalance(items, 3u, weights);
  EXPECT_EQ(lanes.size(), 3u);

  std::vector<size_t> costs(3, 0);
  for (size_t i = 0; i < 3; i++) {
    for (const auto& item : lanes[i]) {
      costs[i] += item.cost;
    }
  }

  EXPECT_EQ(costs[0] + costs[1] + costs[2], 60u);
  EXPECT_GT(costs[0], costs[1]);
  EXPECT_GT(costs[0], costs[2]);
}

TEST(WeightedLPTBalanceTest, WorkerLanesStayBalanced) {
  std::vector<GlyphWorkItem> items = {
      {0, 100}, {1, 90}, {2, 80}, {3, 70}, {4, 60},
      {5, 50},  {6, 40}, {7, 30}, {8, 20}, {9, 10},
  };
  std::vector<double> weights = {5.0, 2.0, 2.0};
  auto lanes = WeightedLPTBalance(items, 3u, weights);

  std::vector<size_t> costs(3, 0);
  for (size_t i = 0; i < 3; i++) {
    for (const auto& item : lanes[i]) {
      costs[i] += item.cost;
    }
  }
  EXPECT_EQ(costs[1], costs[2]);
}

TEST(WeightedLPTBalanceTest, SingleItem) {
  std::vector<GlyphWorkItem> items = {{0, 42}};
  std::vector<double> weights = {5.0, 2.0, 2.0};
  auto lanes = WeightedLPTBalance(items, 3u, weights);
  EXPECT_EQ(lanes[0].size(), 1u);
  EXPECT_EQ(lanes[0][0].cost, 42u);
  EXPECT_EQ(lanes[1].size(), 0u);
  EXPECT_EQ(lanes[2].size(), 0u);
}

TEST(ApplyLaneOverflowTest, ZeroRatioDoesNothing) {
  std::vector<std::vector<GlyphWorkItem>> lanes = {
      {{0, 10}, {1, 10}},
      {{2, 10}, {3, 10}, {4, 10}},
      {{5, 10}, {6, 10}, {7, 10}},
  };
  size_t total = 0;
  for (const auto& l : lanes)
    total += l.size();
  size_t overflow = ApplyLaneOverflow(lanes, 0u);
  EXPECT_EQ(overflow, 0u);
  size_t total_after = 0;
  for (const auto& l : lanes)
    total_after += l.size();
  EXPECT_EQ(total_after, total);
}

TEST(ApplyLaneOverflowTest, SingleLaneDoesNothing) {
  std::vector<std::vector<GlyphWorkItem>> lanes = {{{0, 10}, {1, 10}}};
  size_t overflow = ApplyLaneOverflow(lanes, 30u);
  EXPECT_EQ(overflow, 0u);
  EXPECT_EQ(lanes[0].size(), 2u);
}

TEST(ApplyLaneOverflowTest, EmptyWorkerLanesSkipped) {
  std::vector<std::vector<GlyphWorkItem>> lanes = {
      {{0, 10}},
      {},
      {},
  };
  size_t overflow = ApplyLaneOverflow(lanes, 30u);
  EXPECT_EQ(overflow, 0u);
  EXPECT_EQ(lanes[0].size(), 1u);
}

TEST(ApplyLaneOverflowTest, MovesItemsFromWorkersToLane0) {
  std::vector<std::vector<GlyphWorkItem>> lanes = {
      {{0, 10}},
      {{1, 10},
       {2, 10},
       {3, 10},
       {4, 10},
       {5, 10},
       {6, 10},
       {7, 10},
       {8, 10},
       {9, 10},
       {10, 10}},
      {{11, 10},
       {12, 10},
       {13, 10},
       {14, 10},
       {15, 10},
       {16, 10},
       {17, 10},
       {18, 10},
       {19, 10},
       {20, 10}},
  };
  size_t total_before = 0;
  for (const auto& l : lanes)
    total_before += l.size();
  size_t overflow = ApplyLaneOverflow(lanes, 30u);
  EXPECT_EQ(overflow, 6u);
  EXPECT_EQ(lanes[0].size(), 1u + 3u + 3u);
  EXPECT_EQ(lanes[1].size(), 7u);
  EXPECT_EQ(lanes[2].size(), 7u);
  size_t total_after = 0;
  for (const auto& l : lanes)
    total_after += l.size();
  EXPECT_EQ(total_after, total_before);
}

TEST(ApplyLaneOverflowTest, MovedItemsAreLastInWorkerLane) {
  std::vector<std::vector<GlyphWorkItem>> lanes = {
      {},
      {{1, 10}, {2, 10}, {3, 10}, {4, 10}, {5, 10}},
  };
  ApplyLaneOverflow(lanes, 30u);
  EXPECT_EQ(lanes[0].size(), 2u);
  EXPECT_EQ(lanes[1].size(), 3u);
  EXPECT_EQ(lanes[0][0].index, 4u);
  EXPECT_EQ(lanes[0][1].index, 5u);
}

TEST(ApplyLaneOverflowTest, TotalItemCountPreserved) {
  std::vector<GlyphWorkItem> items = {
      {0, 100}, {1, 90}, {2, 80}, {3, 70}, {4, 60},
      {5, 50},  {6, 40}, {7, 30}, {8, 20}, {9, 10},
  };
  std::vector<double> weights = {5.0, 2.0, 2.0};
  auto lanes = WeightedLPTBalance(items, 3u, weights);
  size_t total_before = 0;
  for (const auto& l : lanes)
    total_before += l.size();
  ApplyLaneOverflow(lanes, 30u);
  size_t total_after = 0;
  for (const auto& l : lanes)
    total_after += l.size();
  EXPECT_EQ(total_after, total_before);
}

TEST(ApplyLaneOverflowTest, NoDuplicateItems) {
  std::vector<GlyphWorkItem> items = {
      {0, 100}, {1, 90}, {2, 80}, {3, 70}, {4, 60},
      {5, 50},  {6, 40}, {7, 30}, {8, 20}, {9, 10},
  };
  std::vector<double> weights = {5.0, 2.0, 2.0};
  auto lanes = WeightedLPTBalance(items, 3u, weights);
  ApplyLaneOverflow(lanes, 30u);
  std::set<size_t> seen;
  for (const auto& lane : lanes) {
    for (const auto& item : lane) {
      EXPECT_EQ(seen.count(item.index), 0u);
      seen.insert(item.index);
    }
  }
  EXPECT_EQ(seen.size(), items.size());
}

TEST(ApplyLaneOverflowTest, WorkerLaneNotDrained) {
  std::vector<std::vector<GlyphWorkItem>> lanes = {
      {},
      {{1, 10}},
      {{2, 10}, {3, 10}},
  };
  ApplyLaneOverflow(lanes, 50u);
  EXPECT_GE(lanes[1].size(), 1u);
  EXPECT_GE(lanes[2].size(), 1u);
}

TEST(ApplyLaneOverflowTest, SmallWorkerLaneRoundsUp) {
  std::vector<std::vector<GlyphWorkItem>> lanes = {
      {},
      {{1, 10}, {2, 10}},
      {{3, 10}, {4, 10}},
  };
  size_t overflow = ApplyLaneOverflow(lanes, 30u);
  EXPECT_EQ(overflow, 2u);
  EXPECT_EQ(lanes[1].size(), 1u);
  EXPECT_EQ(lanes[2].size(), 1u);
}

TEST(ApplyLaneOverflowTest, SingleItemWorkerLaneNotDrained) {
  std::vector<std::vector<GlyphWorkItem>> lanes = {
      {},
      {{1, 10}},
  };
  size_t overflow = ApplyLaneOverflow(lanes, 30u);
  EXPECT_EQ(overflow, 0u);
  EXPECT_EQ(lanes[1].size(), 1u);
}

class RasterizeTest : public ::testing::Test {
 protected:
  static ParallelGlyphWorkItem MakeItem(size_t index,
                                        size_t cost = 1u,
                                        IRect destination = IRect()) {
    return ParallelGlyphWorkItem{
        .index = index,
        .pos = Rect::MakeLTRB(0, 0, 10, 10),
        .bounds = Rect::MakeLTRB(0, 0, 8, 8),
        .size = Size(10, 10),
        .cost = cost,
    };
  }
};

TEST_F(RasterizeTest, SerialPathWhenParallelizationDisabled) {
  Flags flags;
  flags.glyph_raster_parallelization = false;

  size_t count = 20;
  auto scan_fn = [&](size_t i) -> std::optional<ParallelGlyphWorkItem> {
    return MakeItem(i);
  };
  auto rasterize_fn = [&](const ParallelGlyphWorkItem& item,
                          PendingAtlasUpload& upload) -> bool {
    upload.destination = IRect::MakeXYWH(0, 0, 10, 10);
    upload.size = Size(10, 10);
    return true;
  };

  std::vector<std::optional<PendingAtlasUpload>> uploads(count);
  EXPECT_TRUE(GlyphAtlasParallelizer::Rasterize(flags, 0, count, scan_fn,
                                                rasterize_fn, uploads));

  size_t filled = 0;
  for (const auto& u : uploads) {
    if (u.has_value()) {
      filled++;
    }
  }
  EXPECT_EQ(filled, count);
}

TEST_F(RasterizeTest, ParallelPathWhenEnabledAndAboveThreshold) {
  Flags flags;
  flags.glyph_raster_parallelization = true;

  size_t count = kMinParallelGlyphCount + 4;
  auto scan_fn = [&](size_t i) -> std::optional<ParallelGlyphWorkItem> {
    return MakeItem(i, kOutlineGlyphBaseCost);
  };
  auto rasterize_fn = [&](const ParallelGlyphWorkItem& item,
                          PendingAtlasUpload& upload) -> bool {
    upload.destination = IRect::MakeXYWH(0, 0, 10, 10);
    upload.size = Size(10, 10);
    return true;
  };

  std::vector<std::optional<PendingAtlasUpload>> uploads(count);
  EXPECT_TRUE(GlyphAtlasParallelizer::Rasterize(flags, 0, count, scan_fn,
                                                rasterize_fn, uploads));

  size_t filled = 0;
  for (const auto& u : uploads) {
    if (u.has_value()) {
      filled++;
    }
  }
  EXPECT_EQ(filled, count);
}

TEST_F(RasterizeTest, FallbackToSerialBelowThreshold) {
  Flags flags;
  flags.glyph_raster_parallelization = true;

  size_t count = kMinParallelGlyphCount - 1;
  auto scan_fn = [&](size_t i) -> std::optional<ParallelGlyphWorkItem> {
    return MakeItem(i, kOutlineGlyphBaseCost);
  };

  std::atomic_size_t rasterize_call_count = 0;
  auto rasterize_fn = [&](const ParallelGlyphWorkItem& item,
                          PendingAtlasUpload& upload) -> bool {
    rasterize_call_count.fetch_add(1);
    upload.destination = IRect::MakeXYWH(0, 0, 10, 10);
    upload.size = Size(10, 10);
    return true;
  };

  std::vector<std::optional<PendingAtlasUpload>> uploads(count);
  EXPECT_TRUE(GlyphAtlasParallelizer::Rasterize(flags, 0, count, scan_fn,
                                                rasterize_fn, uploads));
  EXPECT_EQ(rasterize_call_count.load(), count);
}

TEST_F(RasterizeTest, RasterizeFailedStopsEarly) {
  Flags flags;
  flags.glyph_raster_parallelization = false;

  size_t count = 10;
  size_t fail_at = 3;
  std::atomic_size_t rasterize_call_count = 0;

  auto scan_fn = [&](size_t i) -> std::optional<ParallelGlyphWorkItem> {
    return MakeItem(i);
  };
  auto rasterize_fn = [&](const ParallelGlyphWorkItem& item,
                          PendingAtlasUpload& upload) -> bool {
    rasterize_call_count.fetch_add(1);
    if (item.index == fail_at) {
      return false;
    }
    upload.destination = IRect::MakeXYWH(0, 0, 10, 10);
    upload.size = Size(10, 10);
    return true;
  };

  std::vector<std::optional<PendingAtlasUpload>> uploads(count);
  EXPECT_FALSE(GlyphAtlasParallelizer::Rasterize(flags, 0, count, scan_fn,
                                                 rasterize_fn, uploads));
  EXPECT_LE(rasterize_call_count.load(), count);
}

TEST_F(RasterizeTest, EmptyRange) {
  Flags flags;
  flags.glyph_raster_parallelization = false;

  auto scan_fn = [&](size_t i) -> std::optional<ParallelGlyphWorkItem> {
    return MakeItem(i);
  };
  auto rasterize_fn = [&](const ParallelGlyphWorkItem& item,
                          PendingAtlasUpload& upload) -> bool {
    upload.destination = IRect::MakeXYWH(0, 0, 10, 10);
    upload.size = Size(10, 10);
    return true;
  };

  std::vector<std::optional<PendingAtlasUpload>> uploads;
  EXPECT_TRUE(GlyphAtlasParallelizer::Rasterize(flags, 0, 0, scan_fn,
                                                rasterize_fn, uploads));
}

TEST_F(RasterizeTest, ScanFnReturnsNulloptForSomeItems) {
  Flags flags;
  flags.glyph_raster_parallelization = false;

  size_t count = 10;
  auto scan_fn = [&](size_t i) -> std::optional<ParallelGlyphWorkItem> {
    if (i % 2 == 0) {
      return MakeItem(i);
    }
    return std::nullopt;
  };
  auto rasterize_fn = [&](const ParallelGlyphWorkItem& item,
                          PendingAtlasUpload& upload) -> bool {
    upload.destination = IRect::MakeXYWH(0, 0, 10, 10);
    upload.size = Size(10, 10);
    return true;
  };

  std::vector<std::optional<PendingAtlasUpload>> uploads(count);
  EXPECT_TRUE(GlyphAtlasParallelizer::Rasterize(flags, 0, count, scan_fn,
                                                rasterize_fn, uploads));

  size_t filled = 0;
  for (size_t i = 0; i < uploads.size(); i++) {
    if (uploads[i].has_value()) {
      filled++;
    }
  }
  EXPECT_EQ(filled, count / 2);
}

TEST_F(RasterizeTest, UploadsPlacedAtCorrectIndex) {
  Flags flags;
  flags.glyph_raster_parallelization = false;

  size_t start = 5;
  size_t end = 15;
  auto scan_fn = [&](size_t i) -> std::optional<ParallelGlyphWorkItem> {
    return MakeItem(i);
  };
  auto rasterize_fn = [&](const ParallelGlyphWorkItem& item,
                          PendingAtlasUpload& upload) -> bool {
    upload.destination =
        IRect::MakeXYWH(static_cast<int>(item.index), 0, 10, 10);
    upload.size = Size(10, 10);
    return true;
  };

  std::vector<std::optional<PendingAtlasUpload>> uploads(end - start);
  EXPECT_TRUE(GlyphAtlasParallelizer::Rasterize(flags, start, end, scan_fn,
                                                rasterize_fn, uploads));

  for (size_t i = 0; i < uploads.size(); i++) {
    ASSERT_TRUE(uploads[i].has_value());
    EXPECT_EQ(uploads[i]->destination.GetX(), static_cast<int>(start + i));
  }
}

TEST_F(RasterizeTest, ParallelPathFailedStopsEarly) {
  Flags flags;
  flags.glyph_raster_parallelization = true;

  size_t count = kMinParallelGlyphCount + 4;
  auto scan_fn = [&](size_t i) -> std::optional<ParallelGlyphWorkItem> {
    return MakeItem(i, kOutlineGlyphBaseCost);
  };

  std::atomic_bool failed_written = false;
  std::atomic_size_t rasterize_call_count = 0;
  auto rasterize_fn = [&](const ParallelGlyphWorkItem& item,
                          PendingAtlasUpload& upload) -> bool {
    rasterize_call_count.fetch_add(1);
    if (item.index == kMinParallelGlyphCount / 2 && !failed_written.load()) {
      failed_written.store(true);
      return false;
    }
    upload.destination = IRect::MakeXYWH(0, 0, 10, 10);
    upload.size = Size(10, 10);
    return true;
  };

  std::vector<std::optional<PendingAtlasUpload>> uploads(count);
  EXPECT_FALSE(GlyphAtlasParallelizer::Rasterize(flags, 0, count, scan_fn,
                                                 rasterize_fn, uploads));
}

TEST_F(RasterizeTest, ParallelPathWithColorGlyphs) {
  Flags flags;
  flags.glyph_raster_parallelization = true;

  size_t count = kMinParallelGlyphCount;
  auto scan_fn = [&](size_t i) -> std::optional<ParallelGlyphWorkItem> {
    return MakeItem(i, kColorGlyphBaseCost);
  };
  auto rasterize_fn = [&](const ParallelGlyphWorkItem& item,
                          PendingAtlasUpload& upload) -> bool {
    upload.destination = IRect::MakeXYWH(0, 0, 10, 10);
    upload.size = Size(10, 10);
    return true;
  };

  std::vector<std::optional<PendingAtlasUpload>> uploads(count);
  EXPECT_TRUE(GlyphAtlasParallelizer::Rasterize(flags, 0, count, scan_fn,
                                                rasterize_fn, uploads));

  size_t filled = 0;
  for (const auto& u : uploads) {
    if (u.has_value()) {
      filled++;
    }
  }
  EXPECT_EQ(filled, count);
}

TEST_F(RasterizeTest, ParallelPathWithMixedCosts) {
  Flags flags;
  flags.glyph_raster_parallelization = true;

  size_t count = kMinParallelGlyphCount + 8;
  auto scan_fn = [&](size_t i) -> std::optional<ParallelGlyphWorkItem> {
    size_t cost = (i % 3 == 0) ? kColorGlyphBaseCost : kOutlineGlyphBaseCost;
    return MakeItem(i, cost);
  };
  auto rasterize_fn = [&](const ParallelGlyphWorkItem& item,
                          PendingAtlasUpload& upload) -> bool {
    upload.destination = IRect::MakeXYWH(0, 0, 10, 10);
    upload.size = Size(10, 10);
    return true;
  };

  std::vector<std::optional<PendingAtlasUpload>> uploads(count);
  EXPECT_TRUE(GlyphAtlasParallelizer::Rasterize(flags, 0, count, scan_fn,
                                                rasterize_fn, uploads));

  size_t filled = 0;
  for (const auto& u : uploads) {
    if (u.has_value()) {
      filled++;
    }
  }
  EXPECT_EQ(filled, count);
}

}  // namespace testing
}  // namespace impeller
