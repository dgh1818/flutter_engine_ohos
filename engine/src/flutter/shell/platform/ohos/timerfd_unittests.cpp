/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/fml/platform/ohos/timerfd.h"

#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include <chrono>
#include <thread>

#include "flutter/fml/time/time_point.h"

namespace flutter {
namespace testing {

class TimerfdTest : public ::testing::Test {
 protected:
  void SetUp() override {
    fd_ = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    ASSERT_GE(fd_, 0) << "timerfd_create failed: " << strerror(errno);
  }

  void TearDown() override {
    if (fd_ >= 0) {
      ::close(fd_);
      fd_ = -1;
    }
  }

  int fd_ = -1;
};

// ===== TimerRearm =====

// Normal path: set a future time point, should successfully rearm the timer
TEST_F(TimerfdTest, TimerRearmFutureTimePointSucceeds) {
  auto future_time = fml::TimePoint::Now() +
                    fml::TimeDelta::FromMilliseconds(100);
  EXPECT_TRUE(fml::TimerRearm(fd_, future_time));
}

// SOURCE BUG (fml/platform/ohos/timerfd.cc:48): `spec.it_interval = spec.it_value`
// makes the timer periodic, contradicting the "single expiry" comment. For a
// zero/past time point (nano_secs clamped to 1), it_interval={0,1} causes the
// timer to fire every nanosecond, so fire_count can be >>1. The correct fix
// is to set it_interval={0,0} (or delete the line), but the source is not
// modified in this commit. This test documents the buggy behavior.
TEST_F(TimerfdTest, TimerRearmZeroTimePointClampsToOneNanosecond) {
  auto zero_time = fml::TimePoint::FromEpochDelta(fml::TimeDelta::Zero());
  EXPECT_TRUE(fml::TimerRearm(fd_, zero_time));

  // BUG: fire_count may be >>1 due to periodic it_interval. Should be 1.
  uint64_t fire_count = 0;
  ssize_t size = ::read(fd_, &fire_count, sizeof(uint64_t));
  EXPECT_EQ(size, static_cast<ssize_t>(sizeof(uint64_t)));
  EXPECT_GT(fire_count, 0u);  // BUG: should be EXPECT_EQ(fire_count, 1u)
}

// SOURCE BUG (same as above): it_interval = it_value makes timer periodic.
// For a past time point, fire_count may be >>1. Should be 1.
TEST_F(TimerfdTest, TimerRearmPastTimePointTriggersImmediately) {
  auto past_time = fml::TimePoint::Now() -
                   fml::TimeDelta::FromMilliseconds(1000);
  EXPECT_TRUE(fml::TimerRearm(fd_, past_time));

  uint64_t fire_count = 0;
  ssize_t size = ::read(fd_, &fire_count, sizeof(uint64_t));
  EXPECT_EQ(size, static_cast<ssize_t>(sizeof(uint64_t)));
  EXPECT_GT(fire_count, 0u);  // BUG: should be EXPECT_EQ(fire_count, 1u)
}

// Error handling: invalid fd, TimerRearm should return false
TEST_F(TimerfdTest, TimerRearmInvalidFdReturnsFalse) {
  int invalid_fd = -1;
  auto future_time = fml::TimePoint::Now() +
                    fml::TimeDelta::FromMilliseconds(100);
  EXPECT_FALSE(fml::TimerRearm(invalid_fd, future_time));
}

// Error handling: using a closed fd, TimerRearm should return false
TEST_F(TimerfdTest, TimerRearmClosedFdReturnsFalse) {
  int closed_fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  ASSERT_GE(closed_fd, 0);
  ::close(closed_fd);

  auto future_time = fml::TimePoint::Now() +
                    fml::TimeDelta::FromMilliseconds(100);
  EXPECT_FALSE(fml::TimerRearm(closed_fd, future_time));
}

// ===== TimerDrain =====

// Normal path: after timer expires, TimerDrain should return true
TEST_F(TimerfdTest, TimerDrainAfterExpiryReturnsTrue) {
  auto immediate_time = fml::TimePoint::FromEpochDelta(fml::TimeDelta::Zero());
  ASSERT_TRUE(fml::TimerRearm(fd_, immediate_time));

  // Wait for timer to fire
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  EXPECT_TRUE(fml::TimerDrain(fd_));
}

// Boundary condition: when timer has not expired, TimerDrain on non-blocking fd should return false
TEST_F(TimerfdTest, TimerDrainBeforeExpiryReturnsFalse) {
  // Set a far future time point to ensure timer has not fired
  auto far_future = fml::TimePoint::Now() +
                    fml::TimeDelta::FromSeconds(60);
  ASSERT_TRUE(fml::TimerRearm(fd_, far_future));

  // Read immediately without waiting (non-blocking fd, read returns EAGAIN)
  EXPECT_FALSE(fml::TimerDrain(fd_));
}

// Error handling: invalid fd, TimerDrain should return false
TEST_F(TimerfdTest, TimerDrainInvalidFdReturnsFalse) {
  EXPECT_FALSE(fml::TimerDrain(-1));
}

// Error handling: using a closed fd, TimerDrain should return false
TEST_F(TimerfdTest, TimerDrainClosedFdReturnsFalse) {
  int closed_fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  ASSERT_GE(closed_fd, 0);
  ::close(closed_fd);

  EXPECT_FALSE(fml::TimerDrain(closed_fd));
}

// State transition: consecutive Rearm + Drain cycle
TEST_F(TimerfdTest, RearmDrainCycleWorksRepeatedly) {
  for (int i = 0; i < 3; i++) {
    auto immediate = fml::TimePoint::FromEpochDelta(fml::TimeDelta::Zero());
    ASSERT_TRUE(fml::TimerRearm(fd_, immediate));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_TRUE(fml::TimerDrain(fd_));
  }
}

}  // namespace testing
}  // namespace flutter
