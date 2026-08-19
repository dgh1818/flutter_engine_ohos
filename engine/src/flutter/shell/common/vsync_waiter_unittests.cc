// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#define FML_USED_ON_EMBEDDER

#include <initializer_list>

#include "flutter/common/settings.h"
#include "flutter/common/task_runners.h"
#include "flutter/flow/frame_timings.h"
#include "flutter/fml/synchronization/waitable_event.h"
#include "flutter/fml/thread.h"
#include "flutter/fml/time/time_delta.h"
#include "flutter/fml/time/time_point.h"
#include "flutter/shell/common/switches.h"

#include "gtest/gtest.h"
#include "thread_host.h"
#include "vsync_waiter.h"

namespace flutter {
namespace testing {

class TestVsyncWaiter : public VsyncWaiter {
 public:
  explicit TestVsyncWaiter(const TaskRunners& task_runners)
      : VsyncWaiter(task_runners) {}

  int await_vsync_call_count_ = 0;

 protected:
  void AwaitVSync() override { await_vsync_call_count_++; }
};

TEST(VsyncWaiterTest, NoUnneededAwaitVsync) {
  using flutter::ThreadHost;
  std::string prefix = "vsync_waiter_test";

  fml::MessageLoop::EnsureInitializedForCurrentThread();
  auto task_runner = fml::MessageLoop::GetCurrent().GetTaskRunner();

  const flutter::TaskRunners task_runners(prefix, task_runner, task_runner,
                                          task_runner, task_runner);

  TestVsyncWaiter vsync_waiter(task_runners);

  vsync_waiter.ScheduleSecondaryCallback(1, [] {});
  EXPECT_EQ(vsync_waiter.await_vsync_call_count_, 1);

  vsync_waiter.ScheduleSecondaryCallback(2, [] {});
  EXPECT_EQ(vsync_waiter.await_vsync_call_count_, 1);
}

class FireCallbackVsyncWaiter : public VsyncWaiter {
 public:
  explicit FireCallbackVsyncWaiter(const TaskRunners& task_runners)
      : VsyncWaiter(task_runners) {}

  using VsyncWaiter::FireCallback;

 protected:
  void AwaitVSync() override {}
};

TEST(VsyncWaiterTest, FireCallbackThreeArgDefaultsDeadlineToTarget) {
  fml::MessageLoop::EnsureInitializedForCurrentThread();
  auto task_runner = fml::MessageLoop::GetCurrent().GetTaskRunner();

  fml::Thread ui_thread("vsync_test_ui");
  const flutter::TaskRunners task_runners(
      "test", task_runner, ui_thread.GetTaskRunner(), task_runner, task_runner);

  FireCallbackVsyncWaiter waiter(task_runners);

  fml::TimePoint frame_start = fml::TimePoint::Now();
  fml::TimePoint frame_target =
      frame_start + fml::TimeDelta::FromMilliseconds(16);

  std::unique_ptr<FrameTimingsRecorder> captured_recorder;
  fml::AutoResetWaitableEvent latch;

  waiter.AsyncWaitForVsync([&](std::unique_ptr<FrameTimingsRecorder> recorder) {
    captured_recorder = std::move(recorder);
    latch.Signal();
  });

  waiter.FireCallback(frame_start, frame_target);

  latch.Wait();

  ASSERT_TRUE(captured_recorder != nullptr);
  ASSERT_EQ(captured_recorder->GetVsyncStartTime(), frame_start);
  ASSERT_EQ(captured_recorder->GetVsyncTargetTime(), frame_target);
  ASSERT_EQ(captured_recorder->GetDartFrameDeadline(), frame_target);
}

TEST(VsyncWaiterTest, FireCallbackFourArgSeparateDeadline) {
  fml::MessageLoop::EnsureInitializedForCurrentThread();
  auto task_runner = fml::MessageLoop::GetCurrent().GetTaskRunner();

  fml::Thread ui_thread("vsync_test_ui2");
  const flutter::TaskRunners task_runners(
      "test", task_runner, ui_thread.GetTaskRunner(), task_runner, task_runner);

  FireCallbackVsyncWaiter waiter(task_runners);

  fml::TimePoint frame_start = fml::TimePoint::Now();
  fml::TimePoint frame_target =
      frame_start + fml::TimeDelta::FromMilliseconds(32);
  fml::TimePoint dart_frame_deadline =
      frame_start + fml::TimeDelta::FromMilliseconds(16);

  std::unique_ptr<FrameTimingsRecorder> captured_recorder;
  fml::AutoResetWaitableEvent latch;

  waiter.AsyncWaitForVsync([&](std::unique_ptr<FrameTimingsRecorder> recorder) {
    captured_recorder = std::move(recorder);
    latch.Signal();
  });

  waiter.FireCallback(frame_start, frame_target, dart_frame_deadline);

  latch.Wait();

  ASSERT_TRUE(captured_recorder != nullptr);
  ASSERT_EQ(captured_recorder->GetVsyncStartTime(), frame_start);
  ASSERT_EQ(captured_recorder->GetVsyncTargetTime(), frame_target);
  ASSERT_EQ(captured_recorder->GetDartFrameDeadline(), dart_frame_deadline);
}

}  // namespace testing
}  // namespace flutter
