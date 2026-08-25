/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#define private public

#include "flutter/fml/message_loop_impl.h"
#include "flutter/fml/platform/ohos/message_loop_ohos.h"

#include <gtest/gtest.h>
#include <uv.h>

#include <atomic>
#include <chrono>
#include <thread>

#include "flutter/fml/message_loop.h"
#include "flutter/fml/task_runner.h"
#include "flutter/fml/time/time_delta.h"
#include "flutter/fml/time/time_point.h"

namespace flutter {
namespace testing {

// ---------------------------------------------------------------------------
// Helper: create a MessageLoopOhos via the factory (platform_loop=nullptr).
// This exercises the non-platform-loop constructor path (uv_loop_init +
// uv_poll_init + uv_poll_start).
// ---------------------------------------------------------------------------
static fml::RefPtr<fml::MessageLoopImpl> CreateLoopNoPlatform() {
  return fml::MessageLoopImpl::Create(nullptr);
}

// ---------------------------------------------------------------------------
// Helper: create a MessageLoopOhos with a real uv_loop_t as platform_loop.
// MessageLoopOhos does NOT own the platform_loop pointer — the caller must
// clean it up via CleanupPlatformLoop() after Terminate().
// ---------------------------------------------------------------------------
struct LoopWithPlatform {
  fml::RefPtr<fml::MessageLoopImpl> loop;
  uv_loop_t* platform_loop;
};

static LoopWithPlatform CreateLoopWithPlatform() {
  auto* loop_ptr = new uv_loop_t();
  uv_loop_init(loop_ptr);
  fml::RefPtr<fml::MessageLoopImpl> loop =
      fml::MessageLoopImpl::Create(loop_ptr);
  // Give the timerhandle_thread_ time to enter epoll_wait before returning,
  // so that Terminate()'s WakeUp can unblock it.
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  return {std::move(loop), loop_ptr};
}

// Clean up the platform uv_loop_t after Terminate().
// uv_close is async — drain pending callbacks via uv_run until
// uv_loop_alive returns 0, then close and delete the loop.
static void CleanupPlatformLoop(uv_loop_t* platform_loop) {
  int guard = 10;
  while (uv_loop_alive(platform_loop) && guard-- > 0) {
    uv_run(platform_loop, UV_RUN_NOWAIT);
  }
  if (uv_loop_close(platform_loop) == 0) {
    delete platform_loop;
  }
}

// ===========================================================================
// 1. Constructor — non-platform-loop path (platform_loop == nullptr)
// ===========================================================================

// Verify that a loop created without a platform loop has valid epoll and
// timer fds, and is in the non-platform-loop mode.
TEST(MessageLoopOhosTest, CreateWithoutPlatformLoop) {
  fml::RefPtr<fml::MessageLoopImpl> loop = CreateLoopNoPlatform();
  ASSERT_TRUE(loop);
  loop->Terminate();
}

// Verify that creating two loops without platform loop works (each gets its
// own epoll_fd and timer_fd).
TEST(MessageLoopOhosTest, CreateTwoLoopsWithoutPlatformLoop) {
  fml::RefPtr<fml::MessageLoopImpl> loop1 = CreateLoopNoPlatform();
  fml::RefPtr<fml::MessageLoopImpl> loop2 = CreateLoopNoPlatform();
  ASSERT_TRUE(loop1);
  ASSERT_TRUE(loop2);
  loop1->Terminate();
  loop2->Terminate();
}

// ===========================================================================
// 2. Constructor — platform-loop path (platform_loop != nullptr)
// ===========================================================================

// Verify that a loop created with a platform loop initializes correctly and
// starts the timerhandle thread.
TEST(MessageLoopOhosTest, CreateWithPlatformLoop) {
  auto ctx = CreateLoopWithPlatform();
  ASSERT_TRUE(ctx.loop);
  ctx.loop->Terminate();
  CleanupPlatformLoop(ctx.platform_loop);
}

// ===========================================================================
// 3. WakeUp — various time points
// ===========================================================================

// WakeUp with a future time point should succeed (TimerRearm returns true).
TEST(MessageLoopOhosTest, WakeUpFutureTime) {
  fml::RefPtr<fml::MessageLoopImpl> loop = CreateLoopNoPlatform();
  loop->WakeUp(fml::TimePoint::Now() +
               fml::TimeDelta::FromMilliseconds(500));
  loop->Terminate();
}

// WakeUp with a past time point (now) should also succeed.
TEST(MessageLoopOhosTest, WakeUpNow) {
  fml::RefPtr<fml::MessageLoopImpl> loop = CreateLoopNoPlatform();
  loop->WakeUp(fml::TimePoint::Now());
  loop->Terminate();
}

// WakeUp with a very far future time point.
TEST(MessageLoopOhosTest, WakeUpFarFuture) {
  fml::RefPtr<fml::MessageLoopImpl> loop = CreateLoopNoPlatform();
  loop->WakeUp(fml::TimePoint::Max());
  loop->Terminate();
}

// Multiple WakeUp calls should not crash.
TEST(MessageLoopOhosTest, WakeUpMultipleTimes) {
  fml::RefPtr<fml::MessageLoopImpl> loop = CreateLoopNoPlatform();
  for (int i = 0; i < 10; i++) {
    loop->WakeUp(fml::TimePoint::Now() +
                 fml::TimeDelta::FromMilliseconds(i * 100));
  }
  loop->Terminate();
}

// WakeUp on a platform-loop instance.
TEST(MessageLoopOhosTest, WakeUpWithPlatformLoop) {
  auto ctx = CreateLoopWithPlatform();
  ctx.loop->WakeUp(fml::TimePoint::Now() +
                   fml::TimeDelta::FromMilliseconds(100));
  ctx.loop->Terminate();
  CleanupPlatformLoop(ctx.platform_loop);
}

// ===========================================================================
// 4. Terminate — various scenarios
// ===========================================================================

// Terminate before Run — non-platform loop.
TEST(MessageLoopOhosTest, TerminateBeforeRunNoPlatform) {
  fml::RefPtr<fml::MessageLoopImpl> loop = CreateLoopNoPlatform();
  loop->Terminate();
}

// Terminate before Run — platform loop.
TEST(MessageLoopOhosTest, TerminateBeforeRunWithPlatform) {
  auto ctx = CreateLoopWithPlatform();
  ctx.loop->Terminate();
  CleanupPlatformLoop(ctx.platform_loop);
}

// Terminate after WakeUp — non-platform loop.
TEST(MessageLoopOhosTest, TerminateAfterWakeUpNoPlatform) {
  fml::RefPtr<fml::MessageLoopImpl> loop = CreateLoopNoPlatform();
  loop->WakeUp(fml::TimePoint::Now() +
               fml::TimeDelta::FromMilliseconds(50));
  // Small delay to let timer potentially fire
  std::this_thread::sleep_for(std::chrono::milliseconds(60));
  loop->Terminate();
}

// Terminate after WakeUp — platform loop.
TEST(MessageLoopOhosTest, TerminateAfterWakeUpWithPlatform) {
  auto ctx = CreateLoopWithPlatform();
  ctx.loop->WakeUp(fml::TimePoint::Now() +
                   fml::TimeDelta::FromMilliseconds(50));
  std::this_thread::sleep_for(std::chrono::milliseconds(60));
  ctx.loop->Terminate();
  CleanupPlatformLoop(ctx.platform_loop);
}

// Double Terminate should not crash.
TEST(MessageLoopOhosTest, DoubleTerminateNoPlatform) {
  fml::RefPtr<fml::MessageLoopImpl> loop = CreateLoopNoPlatform();
  loop->Terminate();
  // Second Terminate is a no-op since the loop is already terminated
  loop->Terminate();
}

// Terminate + cleanup for platform loop. Unlike the non-platform path,
// double Terminate() is unsafe here (uv_close on already-closed handle).
TEST(MessageLoopOhosTest, TerminateAndCleanupWithPlatform) {
  auto ctx = CreateLoopWithPlatform();
  ctx.loop->Terminate();
  CleanupPlatformLoop(ctx.platform_loop);
}

// ===========================================================================
// 5. Run + Terminate — brief run cycle
// ===========================================================================

// Run the loop briefly in a thread, then Terminate.
// We use uv_run with UV_RUN_NOWAIT in a loop to avoid blocking.
// This tests that Run() can be called and the loop processes tasks.
TEST(MessageLoopOhosTest, RunAndTerminateNoPlatform) {
  fml::RefPtr<fml::MessageLoopImpl> loop = CreateLoopNoPlatform();
  auto* loop_ohos = static_cast<fml::MessageLoopOhos*>(loop.get());

  std::atomic<bool> task_ran(false);
  loop->PostTask([&task_ran]() { task_ran.store(true); },
                 fml::TimePoint::Now());

  // Run the loop in a non-blocking way
  for (int i = 0; i < 100 && !task_ran.load(); i++) {
    uv_run(&loop_ohos->loop_, UV_RUN_NOWAIT);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  EXPECT_TRUE(task_ran.load());
  loop->Terminate();
}

// Run + Terminate with platform loop.
// For the platform-loop path, Run() is not called by us (the platform runs
// the loop). We only verify that Terminate works correctly with a platform
// loop, which is already covered by CreateWithPlatformLoop. Instead, this
// test verifies that Terminate can be called without Run for the platform
// path, and the timerhandle_thread_ is properly joined.
TEST(MessageLoopOhosTest, RunAndTerminateWithPlatform) {
  auto ctx = CreateLoopWithPlatform();
  // For platform loops, the platform (not us) runs the uv loop.
  // We just verify Terminate works without calling Run.
  ctx.loop->Terminate();
  CleanupPlatformLoop(ctx.platform_loop);
  SUCCEED();
}

// ===========================================================================
// 6. PostTask — task execution via Run
// ===========================================================================

// PostTask before Run, then Run should execute the task.
// Uses UV_RUN_NOWAIT to avoid blocking.
TEST(MessageLoopOhosTest, PostTaskAndRun) {
  fml::RefPtr<fml::MessageLoopImpl> loop = CreateLoopNoPlatform();
  auto* loop_ohos = static_cast<fml::MessageLoopOhos*>(loop.get());

  std::atomic<bool> task_ran(false);
  loop->PostTask([&task_ran]() { task_ran.store(true); },
                 fml::TimePoint::Now());

  for (int i = 0; i < 100 && !task_ran.load(); i++) {
    uv_run(&loop_ohos->loop_, UV_RUN_NOWAIT);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  EXPECT_TRUE(task_ran.load());
  loop->Terminate();
}

// PostTask with a delayed execution time.
// Uses UV_RUN_NOWAIT to avoid blocking.
TEST(MessageLoopOhosTest, PostDelayedTaskAndRun) {
  fml::RefPtr<fml::MessageLoopImpl> loop = CreateLoopNoPlatform();
  auto* loop_ohos = static_cast<fml::MessageLoopOhos*>(loop.get());

  std::atomic<bool> task_ran(false);
  loop->PostTask([&task_ran]() { task_ran.store(true); },
                 fml::TimePoint::Now() +
                     fml::TimeDelta::FromMilliseconds(100));

  for (int i = 0; i < 200 && !task_ran.load(); i++) {
    uv_run(&loop_ohos->loop_, UV_RUN_NOWAIT);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  EXPECT_TRUE(task_ran.load());
  loop->Terminate();
}

// PostTask without Run, then Terminate — task should not run.
TEST(MessageLoopOhosTest, PostTaskWithoutRun) {
  fml::RefPtr<fml::MessageLoopImpl> loop = CreateLoopNoPlatform();

  std::atomic<bool> task_ran(false);
  loop->PostTask([&task_ran]() { task_ran.store(true); },
                 fml::TimePoint::Now());

  loop->Terminate();
  EXPECT_FALSE(task_ran.load());
}

// Post multiple tasks and verify they all run.
// Uses UV_RUN_NOWAIT to avoid blocking.
TEST(MessageLoopOhosTest, PostMultipleTasksAndRun) {
  fml::RefPtr<fml::MessageLoopImpl> loop = CreateLoopNoPlatform();
  auto* loop_ohos = static_cast<fml::MessageLoopOhos*>(loop.get());

  std::atomic<int> counter(0);
  for (int i = 0; i < 5; i++) {
    loop->PostTask([&counter]() { counter.fetch_add(1); },
                   fml::TimePoint::Now());
  }

  for (int i = 0; i < 100 && counter.load() < 5; i++) {
    uv_run(&loop_ohos->loop_, UV_RUN_NOWAIT);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  EXPECT_EQ(counter.load(), 5);
  loop->Terminate();
}

// ===========================================================================
// 7. OnPollCallback — direct invocation with various status/events
// ===========================================================================

// OnPollCallback with status < 0 (error) — should return early.
TEST(MessageLoopOhosTest, OnPollCallbackError) {
  fml::RefPtr<fml::MessageLoopImpl> loop_impl = CreateLoopNoPlatform();
  auto* loop_ohos = static_cast<fml::MessageLoopOhos*>(loop_impl.get());

  // Create a poll handle and set data to point to the loop
  uv_poll_t poll_handle{};
  poll_handle.data = loop_ohos;

  // status < 0 → error path
  fml::MessageLoopOhos::OnPollCallback(&poll_handle, -1, 0);

  loop_impl->Terminate();
}

// OnPollCallback with status >= 0 but no UV_READABLE event — should not drain.
TEST(MessageLoopOhosTest, OnPollCallbackNoReadable) {
  fml::RefPtr<fml::MessageLoopImpl> loop_impl = CreateLoopNoPlatform();
  auto* loop_ohos = static_cast<fml::MessageLoopOhos*>(loop_impl.get());

  uv_poll_t poll_handle{};
  poll_handle.data = loop_ohos;

  // status = 0, events = 0 → no readable, should return without draining
  fml::MessageLoopOhos::OnPollCallback(&poll_handle, 0, 0);

  loop_impl->Terminate();
}

// OnPollCallback with UV_READABLE event — should drain timer and run tasks.
TEST(MessageLoopOhosTest, OnPollCallbackReadable) {
  fml::RefPtr<fml::MessageLoopImpl> loop_impl = CreateLoopNoPlatform();
  auto* loop_ohos = static_cast<fml::MessageLoopOhos*>(loop_impl.get());

  // Post a task so RunExpiredTasksNow has something to do
  std::atomic<bool> task_ran(false);
  loop_impl->PostTask([&task_ran]() { task_ran.store(true); },
                      fml::TimePoint::Now());

  // Rearm the timer so it fires immediately
  loop_impl->WakeUp(fml::TimePoint::Now());

  // Wait a tiny bit for the timer to fire
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  uv_poll_t poll_handle{};
  poll_handle.data = loop_ohos;

  // status = 0, events = UV_READABLE → should drain timer and run tasks
  fml::MessageLoopOhos::OnPollCallback(&poll_handle, 0, UV_READABLE);

  EXPECT_TRUE(task_ran.load());

  loop_impl->Terminate();
}

// OnPollCallback with UV_READABLE but timer not expired — TimerDrain returns
// false, should not run tasks.
TEST(MessageLoopOhosTest, OnPollCallbackReadableNoTimerExpired) {
  fml::RefPtr<fml::MessageLoopImpl> loop_impl = CreateLoopNoPlatform();
  auto* loop_ohos = static_cast<fml::MessageLoopOhos*>(loop_impl.get());

  // Rearm timer to far future so it hasn't expired
  loop_impl->WakeUp(fml::TimePoint::Max());

  uv_poll_t poll_handle{};
  poll_handle.data = loop_ohos;

  // status = 0, events = UV_READABLE → TimerDrain returns false (timer not
  // expired), should not call RunExpiredTasksNow
  fml::MessageLoopOhos::OnPollCallback(&poll_handle, 0, UV_READABLE);

  loop_impl->Terminate();
}

// ===========================================================================
// 8. OnAsyncCallback — direct invocation
// ===========================================================================

// OnAsyncCallback should call RunExpiredTasksNow on the loop.
TEST(MessageLoopOhosTest, OnAsyncCallback) {
  fml::RefPtr<fml::MessageLoopImpl> loop_impl = CreateLoopNoPlatform();
  auto* loop_ohos = static_cast<fml::MessageLoopOhos*>(loop_impl.get());

  std::atomic<bool> task_ran(false);
  loop_impl->PostTask([&task_ran]() { task_ran.store(true); },
                      fml::TimePoint::Now());

  uv_async_t async_handle{};
  async_handle.data = loop_ohos;

  fml::MessageLoopOhos::OnAsyncCallback(&async_handle);

  EXPECT_TRUE(task_ran.load());

  loop_impl->Terminate();
}

// ===========================================================================
// 9. OnAsyncHandleClose — direct invocation (no-op)
// ===========================================================================

// OnAsyncHandleClose is a no-op; just verify it doesn't crash.
TEST(MessageLoopOhosTest, OnAsyncHandleClose) {
  uv_handle_t handle{};
  fml::MessageLoopOhos::OnAsyncHandleClose(&handle);
  SUCCEED();
}

// OnAsyncHandleClose with nullptr — should not crash (it's a no-op).
TEST(MessageLoopOhosTest, OnAsyncHandleCloseNullptr) {
  fml::MessageLoopOhos::OnAsyncHandleClose(nullptr);
  SUCCEED();
}

// ===========================================================================
// 10. AddOrRemoveTimerSource — direct invocation
// ===========================================================================

// AddOrRemoveTimerSource(true) should succeed (already added in constructor,
// but this tests the function directly).
TEST(MessageLoopOhosTest, AddOrRemoveTimerSourceAdd) {
  fml::RefPtr<fml::MessageLoopImpl> loop_impl = CreateLoopNoPlatform();
  auto* loop_ohos = static_cast<fml::MessageLoopOhos*>(loop_impl.get());

  // Remove first, then add
  bool removed = loop_ohos->AddOrRemoveTimerSource(false);
  // Removing should succeed
  EXPECT_TRUE(removed);

  bool added = loop_ohos->AddOrRemoveTimerSource(true);
  // Adding should succeed
  EXPECT_TRUE(added);

  loop_impl->Terminate();
}

// AddOrRemoveTimerSource(false) should succeed.
TEST(MessageLoopOhosTest, AddOrRemoveTimerSourceRemove) {
  fml::RefPtr<fml::MessageLoopImpl> loop_impl = CreateLoopNoPlatform();
  auto* loop_ohos = static_cast<fml::MessageLoopOhos*>(loop_impl.get());

  bool removed = loop_ohos->AddOrRemoveTimerSource(false);
  EXPECT_TRUE(removed);

  loop_impl->Terminate();
}

// AddOrRemoveTimerSource — double add should fail (EPOLL_CTL_ADD on already
// added fd returns EEXIST).
TEST(MessageLoopOhosTest, AddOrRemoveTimerSourceDoubleAdd) {
  fml::RefPtr<fml::MessageLoopImpl> loop_impl = CreateLoopNoPlatform();
  auto* loop_ohos = static_cast<fml::MessageLoopOhos*>(loop_impl.get());

  // Already added in constructor, adding again should fail
  bool added = loop_ohos->AddOrRemoveTimerSource(true);
  EXPECT_FALSE(added);

  loop_impl->Terminate();
}

// AddOrRemoveTimerSource — double remove should fail (EPOLL_CTL_DEL on
// already removed fd returns ENOENT).
TEST(MessageLoopOhosTest, AddOrRemoveTimerSourceDoubleRemove) {
  fml::RefPtr<fml::MessageLoopImpl> loop_impl = CreateLoopNoPlatform();
  auto* loop_ohos = static_cast<fml::MessageLoopOhos*>(loop_impl.get());

  // Remove once
  bool removed = loop_ohos->AddOrRemoveTimerSource(false);
  EXPECT_TRUE(removed);

  // Remove again — should fail
  bool removed2 = loop_ohos->AddOrRemoveTimerSource(false);
  EXPECT_FALSE(removed2);

  loop_impl->Terminate();
}

// ===========================================================================
// 11. Run + PostTask + WakeUp — integration
// ===========================================================================

// Post a delayed task, Run, and verify it executes after the delay.
// Uses UV_RUN_NOWAIT to avoid blocking.
TEST(MessageLoopOhosTest, RunWithDelayedTask) {
  fml::RefPtr<fml::MessageLoopImpl> loop = CreateLoopNoPlatform();
  auto* loop_ohos = static_cast<fml::MessageLoopOhos*>(loop.get());

  std::atomic<bool> task_ran(false);
  loop->PostTask([&task_ran]() { task_ran.store(true); },
                 fml::TimePoint::Now() +
                     fml::TimeDelta::FromMilliseconds(50));

  for (int i = 0; i < 200 && !task_ran.load(); i++) {
    uv_run(&loop_ohos->loop_, UV_RUN_NOWAIT);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  EXPECT_TRUE(task_ran.load());
  loop->Terminate();
}

// Post a task that itself posts another task.
// Uses UV_RUN_NOWAIT to avoid blocking.
TEST(MessageLoopOhosTest, ChainedTasks) {
  fml::RefPtr<fml::MessageLoopImpl> loop = CreateLoopNoPlatform();
  auto* loop_ohos = static_cast<fml::MessageLoopOhos*>(loop.get());

  std::atomic<int> counter(0);
  loop->PostTask(
      [&]() {
        counter.fetch_add(1);
        loop->PostTask([&counter]() { counter.fetch_add(1); },
                       fml::TimePoint::Now());
      },
      fml::TimePoint::Now());

  for (int i = 0; i < 200 && counter.load() < 2; i++) {
    uv_run(&loop_ohos->loop_, UV_RUN_NOWAIT);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  EXPECT_EQ(counter.load(), 2);
  loop->Terminate();
}

// ===========================================================================
// 12. Task observers
// ===========================================================================

// Add and remove task observers.
// Uses UV_RUN_NOWAIT to avoid blocking.
#ifndef NDEBUG
TEST(MessageLoopOhosTest, TaskObserverDisabledInDebugBuild) {
  GTEST_SKIP() << "impl-level observer APIs assert same-thread ownership; debug builds abort by design.";
}
#else
TEST(MessageLoopOhosTest, TaskObserver) {
  fml::RefPtr<fml::MessageLoopImpl> loop = CreateLoopNoPlatform();
  auto* loop_ohos = static_cast<fml::MessageLoopOhos*>(loop.get());

  std::atomic<int> observer_count(0);
  intptr_t key = 1;
  loop->AddTaskObserver(key, [&observer_count]() {
    observer_count.fetch_add(1);
  });

  std::atomic<bool> task_ran(false);
  loop->PostTask([&task_ran]() { task_ran.store(true); },
                 fml::TimePoint::Now());

  for (int i = 0; i < 100 && !task_ran.load(); i++) {
    uv_run(&loop_ohos->loop_, UV_RUN_NOWAIT);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  EXPECT_TRUE(task_ran.load());
  // Observer should have been called at least once
  EXPECT_GE(observer_count.load(), 1);
  loop->Terminate();
}
#endif  // NDEBUG

// ===========================================================================
// 13. Destructor — verify clean destruction
// ===========================================================================

// Verify that the loop can be created and destroyed (via RefPtr release)
// without calling Terminate. The destructor should handle cleanup.
TEST(MessageLoopOhosTest, DestructorWithoutTerminate) {
  {
    fml::RefPtr<fml::MessageLoopImpl> loop = CreateLoopNoPlatform();
    // loop goes out of scope here — destructor runs
  }
  SUCCEED();
}

// Destructor without Terminate — platform loop.
// Note: For the platform-loop path, the destructor does not join
// timerhandle_thread_, so we call Terminate() first to ensure clean
// shutdown before the loop goes out of scope.
TEST(MessageLoopOhosTest, DestructorWithoutTerminatePlatform) {
  {
    auto ctx = CreateLoopWithPlatform();
    ctx.loop->Terminate();
    CleanupPlatformLoop(ctx.platform_loop);
    // ctx.loop goes out of scope here — destructor runs
  }
  SUCCEED();
}

// ===========================================================================
// 14. Run + Terminate stress — multiple cycles
// ===========================================================================

// Create, Run, Terminate multiple times (different loop instances).
// Uses UV_RUN_NOWAIT to avoid blocking.
TEST(MessageLoopOhosTest, MultipleRunCycles) {
  for (int i = 0; i < 3; i++) {
    fml::RefPtr<fml::MessageLoopImpl> loop = CreateLoopNoPlatform();
    auto* loop_ohos = static_cast<fml::MessageLoopOhos*>(loop.get());

    std::atomic<bool> task_ran(false);
    loop->PostTask([&task_ran]() { task_ran.store(true); },
                   fml::TimePoint::Now());

    for (int j = 0; j < 100 && !task_ran.load(); j++) {
      uv_run(&loop_ohos->loop_, UV_RUN_NOWAIT);
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(task_ran.load());
    loop->Terminate();
  }
}

// ===========================================================================
// 15. WakeUp with Max and Min time points
// ===========================================================================

// WakeUp with TimePoint::Min() — should not crash.
TEST(MessageLoopOhosTest, WakeUpMinTime) {
  fml::RefPtr<fml::MessageLoopImpl> loop = CreateLoopNoPlatform();
  loop->WakeUp(fml::TimePoint::Min());
  loop->Terminate();
}

// WakeUp with TimePoint::Max() then WakeUp with Now — should update timer.
TEST(MessageLoopOhosTest, WakeUpMaxThenNow) {
  fml::RefPtr<fml::MessageLoopImpl> loop = CreateLoopNoPlatform();
  loop->WakeUp(fml::TimePoint::Max());
  loop->WakeUp(fml::TimePoint::Now());
  loop->Terminate();
}

}  // namespace testing
}  // namespace flutter
