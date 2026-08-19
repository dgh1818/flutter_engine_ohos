/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#define FML_USED_ON_EMBEDDER

#define private public
#include "flutter/shell/platform/ohos/vsync_waiter_ohos.h"
#undef private

#include <atomic>
#include <set>

#include "flutter/common/task_runners.h"
#include "flutter/flow/frame_timings.h"
#include "flutter/fml/message_loop.h"
#include "flutter/fml/synchronization/waitable_event.h"
#include "flutter/fml/thread.h"
#include "flutter/fml/time/time_delta.h"
#include "flutter/fml/time/time_point.h"
#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"
#include "flutter/shell/platform/ohos/ohos_vsync_voting_mgr.h"
#include "gtest/gtest.h"

namespace flutter {
namespace testing {

class VsyncWaiterOhosTest : public ::testing::Test {
 protected:
  void SetUp() override {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    auto task_runner = fml::MessageLoop::GetCurrent().GetTaskRunner();

    const TaskRunners task_runners("test_vsync", task_runner, task_runner,
                                   task_runner, task_runner);

    enable_frame_cache_ = std::make_shared<bool>(false);

    OhosVsyncVotingMgr::ResetInstance();

    original_refresh_rate_ = PlatformViewOHOSNapi::display_refresh_rate;
    original_all_rates_ = PlatformViewOHOSNapi::all_refresh_rates;

    PlatformViewOHOSNapi::display_refresh_rate = 60;
    PlatformViewOHOSNapi::all_refresh_rates =
        std::make_shared<std::set<int>>(std::set<int>{60, 90, 120});

    waiter_ =
        std::make_shared<VsyncWaiterOHOS>(task_runners, enable_frame_cache_);
  }

  void TearDown() override {
    PlatformViewOHOSNapi::display_refresh_rate = original_refresh_rate_;
    PlatformViewOHOSNapi::all_refresh_rates = original_all_rates_;
    waiter_.reset();
    OhosVsyncVotingMgr::ResetInstance();
  }

  std::shared_ptr<bool> enable_frame_cache_;
  std::shared_ptr<VsyncWaiterOHOS> waiter_;

  int32_t original_refresh_rate_;
  std::shared_ptr<std::set<int>> original_all_rates_;
};

TEST_F(VsyncWaiterOhosTest, ConstructorCreatesValidHandle) {
  EXPECT_NE(waiter_->vsync_handle_, nullptr);
}

TEST_F(VsyncWaiterOhosTest, DestructorDetachesFromVotingMgr) {
  auto mgr = OhosVsyncVotingMgr::GetInstance();
  EXPECT_NE(mgr, nullptr);

  waiter_.reset();

  auto mgr2 = OhosVsyncVotingMgr::GetInstance();
  EXPECT_EQ(mgr, mgr2);
}

TEST_F(VsyncWaiterOhosTest, UpdateDisplayRefreshRateNullRates) {
  PlatformViewOHOSNapi::all_refresh_rates = nullptr;
  PlatformViewOHOSNapi::display_refresh_rate = 60;

  waiter_->UpdateDisplayRefreshRate(1000000000 / 90);

  EXPECT_EQ(PlatformViewOHOSNapi::display_refresh_rate, 60);
}

TEST_F(VsyncWaiterOhosTest, UpdateDisplayRefreshRateExactMatch) {
  PlatformViewOHOSNapi::display_refresh_rate = 30;

  waiter_->UpdateDisplayRefreshRate(1000000000 / 60);

  EXPECT_EQ(PlatformViewOHOSNapi::display_refresh_rate, 60);
}

TEST_F(VsyncWaiterOhosTest, UpdateDisplayRefreshRateClosestLow) {
  PlatformViewOHOSNapi::display_refresh_rate = 30;

  waiter_->UpdateDisplayRefreshRate(1000000000 / 100);

  EXPECT_EQ(PlatformViewOHOSNapi::display_refresh_rate, 90);
}

TEST_F(VsyncWaiterOhosTest, UpdateDisplayRefreshRateClosestHigh) {
  PlatformViewOHOSNapi::display_refresh_rate = 30;

  waiter_->UpdateDisplayRefreshRate(1000000000 / 110);

  EXPECT_EQ(PlatformViewOHOSNapi::display_refresh_rate, 120);
}

TEST_F(VsyncWaiterOhosTest, UpdateDisplayRefreshRateBelowMin) {
  PlatformViewOHOSNapi::display_refresh_rate = 30;

  waiter_->UpdateDisplayRefreshRate(1000000000 / 30);

  EXPECT_EQ(PlatformViewOHOSNapi::display_refresh_rate, 60);
}

TEST_F(VsyncWaiterOhosTest, UpdateDisplayRefreshRateAboveMax) {
  PlatformViewOHOSNapi::display_refresh_rate = 30;

  waiter_->UpdateDisplayRefreshRate(1000000000 / 150);

  EXPECT_EQ(PlatformViewOHOSNapi::display_refresh_rate, 120);
}

TEST_F(VsyncWaiterOhosTest, UpdateDisplayRefreshRatePeriodZero) {
  PlatformViewOHOSNapi::display_refresh_rate = 30;

  waiter_->UpdateDisplayRefreshRate(0);

  EXPECT_EQ(PlatformViewOHOSNapi::display_refresh_rate, 30);
}

TEST_F(VsyncWaiterOhosTest, UpdateDisplayRefreshRateNoChangeWhenSame) {
  PlatformViewOHOSNapi::display_refresh_rate = 60;

  waiter_->UpdateDisplayRefreshRate(1000000000 / 60);

  EXPECT_EQ(PlatformViewOHOSNapi::display_refresh_rate, 60);
}

TEST_F(VsyncWaiterOhosTest, UpdateDisplayRefreshRateMidpointBoundary) {
  PlatformViewOHOSNapi::all_refresh_rates =
      std::make_shared<std::set<int>>(std::set<int>{60, 120});
  PlatformViewOHOSNapi::display_refresh_rate = 30;

  waiter_->UpdateDisplayRefreshRate(1000000000 / 90);

  EXPECT_EQ(PlatformViewOHOSNapi::display_refresh_rate, 120);
}

TEST_F(VsyncWaiterOhosTest, SetDvsyncSwitchLowApiVersion) {
  waiter_->apiVersion_ = 10;

  waiter_->SetDvsyncSwitch(true);

  EXPECT_EQ(waiter_->handle_, nullptr);
  EXPECT_EQ(waiter_->nativeDvsyncFunc_, nullptr);
}

TEST_F(VsyncWaiterOhosTest, GetVsyncPeriodReturnsNonNegative) {
  int64_t period = waiter_->GetVsyncPeriod();
  EXPECT_GE(period, 0);
}

TEST_F(VsyncWaiterOhosTest, AwaitVSyncDoesNotCrash) {
  waiter_->AwaitVSync();
}

TEST_F(VsyncWaiterOhosTest, EnableFrameCacheFlag) {
  *enable_frame_cache_ = true;
  EXPECT_TRUE(*waiter_->enable_frame_cache_);
}

TEST_F(VsyncWaiterOhosTest, ConsumePendingCallbackExpiredWeakPtrNoCrash) {
  fml::TimePoint frame_start = fml::TimePoint::Now();
  fml::TimePoint frame_target =
      frame_start + fml::TimeDelta::FromMilliseconds(32);
  fml::TimePoint dart_frame_deadline =
      frame_start + fml::TimeDelta::FromMilliseconds(16);

  auto* weak_this = new std::weak_ptr<VsyncWaiter>();

  EXPECT_NO_FATAL_FAILURE(VsyncWaiterOHOS::ConsumePendingCallback(
      weak_this, frame_start, frame_target, dart_frame_deadline));
}

TEST_F(VsyncWaiterOhosTest, OnVsyncFromOHOSNullDataNoCrash) {
  EXPECT_NO_FATAL_FAILURE(VsyncWaiterOHOS::OnVsyncFromOHOS(0, nullptr));
}

TEST_F(VsyncWaiterOhosTest, OnVsyncFromOHOSEndToEndDartFrameDeadline) {
  fml::MessageLoop::EnsureInitializedForCurrentThread();
  auto task_runner = fml::MessageLoop::GetCurrent().GetTaskRunner();

  fml::Thread ui_thread("test_e2e_ui");
  const TaskRunners task_runners("test_e2e", task_runner,
                                 ui_thread.GetTaskRunner(), task_runner,
                                 task_runner);

  std::shared_ptr<bool> cache_flag = std::make_shared<bool>(false);

  OhosVsyncVotingMgr::ResetInstance();

  auto original_rate = PlatformViewOHOSNapi::display_refresh_rate;
  auto original_rates = PlatformViewOHOSNapi::all_refresh_rates;
  PlatformViewOHOSNapi::display_refresh_rate = 60;
  PlatformViewOHOSNapi::all_refresh_rates =
      std::make_shared<std::set<int>>(std::set<int>{60, 90, 120});

  auto ohos_waiter =
      std::make_shared<VsyncWaiterOHOS>(task_runners, cache_flag);

  std::unique_ptr<FrameTimingsRecorder> captured_recorder;
  fml::AutoResetWaitableEvent latch;

  ohos_waiter->AsyncWaitForVsync(
      [&](std::unique_ptr<FrameTimingsRecorder> recorder) {
        captured_recorder = std::move(recorder);
        latch.Signal();
      });

  auto* weak_this = new std::weak_ptr<VsyncWaiter>(ohos_waiter);

  int64_t timestamp_ns = 1'000'000'000;

  VsyncWaiterOHOS::firstCall = false;
  VsyncWaiterOHOS::OnVsyncFromOHOS(timestamp_ns, weak_this);

  latch.Wait();

  ASSERT_TRUE(captured_recorder != nullptr);

  auto frame_start = fml::TimePoint::FromEpochDelta(
      fml::TimeDelta::FromNanoseconds(timestamp_ns));
  EXPECT_EQ(captured_recorder->GetVsyncStartTime(), frame_start);

  EXPECT_EQ(captured_recorder->GetDartFrameDeadline(),
            captured_recorder->GetVsyncTargetTime());

  PlatformViewOHOSNapi::display_refresh_rate = original_rate;
  PlatformViewOHOSNapi::all_refresh_rates = original_rates;
}

TEST_F(VsyncWaiterOhosTest, OnVsyncFromOHOSFrameCacheOffsetsTargetNotDeadline) {
  fml::MessageLoop::EnsureInitializedForCurrentThread();
  auto task_runner = fml::MessageLoop::GetCurrent().GetTaskRunner();

  fml::Thread ui_thread("test_cache_ui");
  const TaskRunners task_runners("test_cache", task_runner,
                                 ui_thread.GetTaskRunner(), task_runner,
                                 task_runner);

  std::shared_ptr<bool> cache_flag = std::make_shared<bool>(true);

  OhosVsyncVotingMgr::ResetInstance();

  auto original_rate = PlatformViewOHOSNapi::display_refresh_rate;
  auto original_rates = PlatformViewOHOSNapi::all_refresh_rates;
  PlatformViewOHOSNapi::display_refresh_rate = 60;
  PlatformViewOHOSNapi::all_refresh_rates =
      std::make_shared<std::set<int>>(std::set<int>{60, 90, 120});

  auto ohos_waiter =
      std::make_shared<VsyncWaiterOHOS>(task_runners, cache_flag);

  std::unique_ptr<FrameTimingsRecorder> captured_recorder;
  fml::AutoResetWaitableEvent latch;

  ohos_waiter->AsyncWaitForVsync(
      [&](std::unique_ptr<FrameTimingsRecorder> recorder) {
        captured_recorder = std::move(recorder);
        latch.Signal();
      });

  auto* weak_this = new std::weak_ptr<VsyncWaiter>(ohos_waiter);

  int64_t timestamp_ns = 2'000'000'000;

  VsyncWaiterOHOS::firstCall = false;
  VsyncWaiterOHOS::OnVsyncFromOHOS(timestamp_ns, weak_this);

  latch.Wait();

  ASSERT_TRUE(captured_recorder != nullptr);

  EXPECT_NE(captured_recorder->GetDartFrameDeadline(),
            captured_recorder->GetVsyncTargetTime());

  EXPECT_LT(captured_recorder->GetDartFrameDeadline(),
            captured_recorder->GetVsyncTargetTime());

  PlatformViewOHOSNapi::display_refresh_rate = original_rate;
  PlatformViewOHOSNapi::all_refresh_rates = original_rates;
}

}  // namespace testing
}  // namespace flutter
