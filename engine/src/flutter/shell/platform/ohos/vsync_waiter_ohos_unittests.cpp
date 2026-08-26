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
#include <dlfcn.h>
#include <set>
#include "flutter/common/task_runners.h"
#include "flutter/flow/frame_timings.h"
#include "flutter/fml/message_loop.h"
#include "flutter/fml/time/time_delta.h"
#include "flutter/fml/time/time_point.h"
#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"
#include "flutter/shell/platform/ohos/ohos_vsync_voting_mgr.h"
#include "gtest/gtest.h"

extern "C" int g_stub_sdk_api_version;

namespace flutter {
namespace testing {

struct FakeDvsyncRecorder {
  static int call_count;
  static OH_NativeVSync* last_handle;
  static bool last_enable;
};

int FakeDvsyncRecorder::call_count = 0;
OH_NativeVSync* FakeDvsyncRecorder::last_handle = nullptr;
bool FakeDvsyncRecorder::last_enable = false;

int FakeDvsyncFunc(OH_NativeVSync* nativeVsync, bool enable) {
  FakeDvsyncRecorder::call_count++;
  FakeDvsyncRecorder::last_handle = nativeVsync;
  FakeDvsyncRecorder::last_enable = enable;
  return 0;
}

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

    FakeDvsyncRecorder::call_count = 0;
    FakeDvsyncRecorder::last_handle = nullptr;
    FakeDvsyncRecorder::last_enable = false;
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
  EXPECT_NO_FATAL_FAILURE(waiter_->AwaitVSync());
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

TEST_F(VsyncWaiterOhosTest, OnVsyncFromOHOSFrameCacheOffsetsTargetNotDeadline) {
  auto task_runner = fml::MessageLoop::GetCurrent().GetTaskRunner();
  const TaskRunners task_runners("test_cache", task_runner, task_runner,
                                 task_runner, task_runner);

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

  {
    std::lock_guard<std::mutex> lock(ohos_waiter->callback_mutex_);
    ohos_waiter->callback_ =
        [&](std::unique_ptr<FrameTimingsRecorder> recorder) {
          captured_recorder = std::move(recorder);
        };
  }

  auto* weak_this = new std::weak_ptr<VsyncWaiter>(ohos_waiter);

  int64_t timestamp_ns = 2'000'000'000;

  auto saved_handle = ohos_waiter->vsync_handle_;
  ohos_waiter->vsync_handle_ = nullptr;

  VsyncWaiterOHOS::firstCall = false;
  VsyncWaiterOHOS::OnVsyncFromOHOS(timestamp_ns, weak_this);

  fml::MessageLoop::GetCurrent().RunExpiredTasksNow();

  ohos_waiter->vsync_handle_ = saved_handle;

  ASSERT_TRUE(captured_recorder != nullptr);

  EXPECT_NE(captured_recorder->GetDartFrameDeadline(),
            captured_recorder->GetVsyncTargetTime());

  EXPECT_LT(captured_recorder->GetDartFrameDeadline(),
            captured_recorder->GetVsyncTargetTime());

  PlatformViewOHOSNapi::display_refresh_rate = original_rate;
  PlatformViewOHOSNapi::all_refresh_rates = original_rates;
}

TEST_F(VsyncWaiterOhosTest, AwaitVSyncNullHandleReturnsEarly) {
  auto saved = waiter_->vsync_handle_;
  waiter_->vsync_handle_ = nullptr;
  EXPECT_NO_FATAL_FAILURE(waiter_->AwaitVSync());
  waiter_->vsync_handle_ = saved;
}

TEST_F(VsyncWaiterOhosTest, OnVsyncFromOHOSFirstCallSetsQos) {
  VsyncWaiterOHOS::firstCall = true;
  auto* weak_this = new std::weak_ptr<VsyncWaiter>();
  EXPECT_NO_FATAL_FAILURE(
      VsyncWaiterOHOS::OnVsyncFromOHOS(1'000'000'000, weak_this));
  EXPECT_FALSE(VsyncWaiterOHOS::firstCall);
  VsyncWaiterOHOS::firstCall = false;
}

TEST_F(VsyncWaiterOhosTest, OnVsyncFromOHOSNoCacheKeepsDeadlineEqualToTarget) {
  VsyncWaiterOHOS::firstCall = false;
  std::unique_ptr<FrameTimingsRecorder> captured_recorder;
  {
    std::lock_guard<std::mutex> lock(waiter_->callback_mutex_);
    waiter_->callback_ = [&](std::unique_ptr<FrameTimingsRecorder> recorder) {
      captured_recorder = std::move(recorder);
    };
  }
  auto* weak_this = new std::weak_ptr<VsyncWaiter>(waiter_);

  auto saved_handle = waiter_->vsync_handle_;
  waiter_->vsync_handle_ = nullptr;

  VsyncWaiterOHOS::OnVsyncFromOHOS(2'000'000'000, weak_this);

  fml::MessageLoop::GetCurrent().RunExpiredTasksNow();
  waiter_->vsync_handle_ = saved_handle;

  ASSERT_NE(captured_recorder, nullptr);
  EXPECT_EQ(captured_recorder->GetDartFrameDeadline(),
            captured_recorder->GetVsyncTargetTime());
}

TEST_F(VsyncWaiterOhosTest, SetDvsyncSwitchApiVersionZeroQueriesSdk) {
  waiter_->nativeDvsyncFunc_ = &FakeDvsyncFunc;
  waiter_->handle_ = reinterpret_cast<void*>(0x1);
  int saved_version = g_stub_sdk_api_version;
  g_stub_sdk_api_version = 20;
  waiter_->apiVersion_ = 0;
  EXPECT_NO_FATAL_FAILURE(waiter_->SetDvsyncSwitch(true));
  EXPECT_EQ(waiter_->apiVersion_, 20);
  g_stub_sdk_api_version = saved_version;
  if (waiter_->apiVersion_ >= 14) {
    EXPECT_EQ(FakeDvsyncRecorder::call_count, 1);
    EXPECT_EQ(FakeDvsyncRecorder::last_handle, waiter_->vsync_handle_);
    EXPECT_TRUE(FakeDvsyncRecorder::last_enable);
  }
}

TEST_F(VsyncWaiterOhosTest, SetDvsyncSwitchWithApi14LoadsLibrary) {
  waiter_->nativeDvsyncFunc_ = &FakeDvsyncFunc;
  waiter_->handle_ = nullptr;
  waiter_->apiVersion_ = 14;
  EXPECT_NO_FATAL_FAILURE(waiter_->SetDvsyncSwitch(true));
  EXPECT_NE(waiter_->handle_, nullptr);
  EXPECT_EQ(FakeDvsyncRecorder::call_count, 1);
  EXPECT_NO_FATAL_FAILURE(waiter_->SetDvsyncSwitch(false));
  EXPECT_EQ(FakeDvsyncRecorder::call_count, 2);
  EXPECT_FALSE(FakeDvsyncRecorder::last_enable);
  dlclose(waiter_->handle_);
  waiter_->handle_ = nullptr;
}

TEST_F(VsyncWaiterOhosTest, SetDvsyncSwitchDlsymFailureReleasesHandle) {
  void* libc_handle = dlopen("libc.so", RTLD_NOW);
  ASSERT_NE(libc_handle, nullptr);

  waiter_->handle_ = libc_handle;
  waiter_->nativeDvsyncFunc_ = nullptr;
  waiter_->apiVersion_ = 14;
  EXPECT_NO_FATAL_FAILURE(waiter_->SetDvsyncSwitch(true));
  EXPECT_EQ(waiter_->nativeDvsyncFunc_, nullptr);
  EXPECT_EQ(waiter_->handle_, nullptr);
  EXPECT_EQ(FakeDvsyncRecorder::call_count, 0);
}

}  // namespace testing
}  // namespace flutter
