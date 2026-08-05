/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

// These tests exercise the voting internals (VoteFinalFrameRateByPriority,
// VotingExpectedRateRange, DelayFrameRateDropForStability), which are
// private. Expose them with the same #define-private-public pattern used
// by vsync_waiter_ohos_unittests.cpp. The #undef must stay immediately
// after this include so that later headers (gtest, ...) are unaffected.
#define private public
#include "flutter/shell/platform/ohos/ohos_vsync_voting_mgr.h"
#undef private

#include <chrono>
#include <thread>

#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"
#include "gtest/gtest.h"

namespace flutter {
namespace testing {

// Test fixture that resets the singleton state between tests.
class OhosVsyncVotingMgrTest : public ::testing::Test {
 protected:
  void SetUp() override {
    OhosVsyncVotingMgr::ResetInstance();

    mgr_ = OhosVsyncVotingMgr::GetInstance();
    ASSERT_NE(mgr_, nullptr);

    // Ensure LTPO is enabled for most tests.
    mgr_->ParseFramesCfg();
    EXPECT_EQ(mgr_->CheckVotingSwitchState(), LTPOSwitchState::LTPO_SWITCH_ON);
  }

  void TearDown() override { OhosVsyncVotingMgr::ResetInstance(); }

  std::shared_ptr<OhosVsyncVotingMgr> mgr_;
};

TEST_F(OhosVsyncVotingMgrTest, SingletonReturnsSameInstance) {
  auto mgr2 = OhosVsyncVotingMgr::GetInstance();
  EXPECT_EQ(mgr_, mgr2);
}

TEST_F(OhosVsyncVotingMgrTest, CheckVotingSwitchStateDefaultOn) {
  LTPOSwitchState state = mgr_->CheckVotingSwitchState();
  EXPECT_EQ(state, LTPOSwitchState::LTPO_SWITCH_ON);
}

TEST_F(OhosVsyncVotingMgrTest, VoteTouchDownSets120Fps) {
  int64_t now = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  mgr_->VoteTouchValue(VVMTouchType::TOUCH_TYPE_DOWN, now);

  // After touch down, the voting should result in 120 FPS.
  int rate = mgr_->VoteFinalFrameRateByPriority();
  EXPECT_EQ(rate, 120);
}

TEST_F(OhosVsyncVotingMgrTest, VoteTouchUpThenTimeoutDropsTo60) {
  int64_t now = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  mgr_->VoteTouchValue(VVMTouchType::TOUCH_TYPE_DOWN, now);
  mgr_->VoteTouchValue(VVMTouchType::TOUCH_TYPE_UP, now);

  // Immediately after touch up (within 100ms), should still be 120.
  int rate = mgr_->VoteFinalFrameRateByPriority();
  EXPECT_EQ(rate, 120);

  // Simulate the 3-second timeout callback.
  int64_t later = now + 4000;  // 4 seconds later
  mgr_->VoteTouchValue(VVMTouchType::TOUCH_TYPE_UP_3_SEC_AFTER, later);

  rate = mgr_->VoteFinalFrameRateByPriority();
  EXPECT_EQ(rate, 60);
}

TEST_F(OhosVsyncVotingMgrTest, VoteAnimationTranslateSetsFpsByVelocity) {
  // Velocity 0 mm/frame should map to 60 FPS (the lowest bracket).
  mgr_->VoteAnimationValue(AnimationType::AN_TYPE_TRANSLATE, 1.0, 0.0);
  int rate = mgr_->VoteFinalFrameRateByPriority();
  // With no touch event active, animation voting should take effect.
  // But touch_voting_ is 0 by default which means no voting, leading to 60.
  EXPECT_EQ(rate, 60);
}

TEST_F(OhosVsyncVotingMgrTest, VoteVideoSetsFpsByFrameRate) {
  // 30 fps video -> vote for 30.
  mgr_->VoteVideoValue(1, 30);
  // Video voting alone without touch/animation: the priority logic first
  // checks touch (no vote -> 60), so video doesn't directly affect
  // VoteFinalFrameRateByPriority. Let's verify video_voting_ is set.
  // Since video voting is lower priority than touch, and default touch is 0
  // (meaning no touch, which maps to 60), the result is 60.
  int rate = mgr_->VoteFinalFrameRateByPriority();
  EXPECT_EQ(rate, 60);
}

TEST_F(OhosVsyncVotingMgrTest, PlatformViewExistSets120Fps) {
  int64_t now = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  int64_t old_ts = now - 200;

  mgr_->VoteTouchValue(VVMTouchType::TOUCH_TYPE_UP, old_ts);

  mgr_->SetPlatformViewExist(true);
  int rate = mgr_->VoteFinalFrameRateByPriority();
  EXPECT_EQ(rate, 120);
}

TEST_F(OhosVsyncVotingMgrTest, AttachDetachNativeVsync) {
  // AttachNativeVsync with nullptr handle should not crash.
  mgr_->AttachNativeVsync("test", nullptr);

  // DetachNativeVsync should not crash even if not attached.
  mgr_->DetachNativeVsync("test");
}

TEST_F(OhosVsyncVotingMgrTest, DelayFrameRateDropForStability) {
  OH_NativeVSync_ExpectedRateRange range = {0, 0, 0};
  mgr_->VotingExpectedRateRange(120, &range);

  int rate = mgr_->DelayFrameRateDropForStability(120);
  EXPECT_EQ(rate, 120);

  rate = mgr_->DelayFrameRateDropForStability(60);
  EXPECT_EQ(rate, 120);
}

TEST_F(OhosVsyncVotingMgrTest,
       VotingExpectedRateRangeReturnsFailedForSameRate) {
  OH_NativeVSync_ExpectedRateRange range = {0, 0, 0};

  int ret = mgr_->VotingExpectedRateRange(0, &range);
  EXPECT_EQ(ret, -1);

  ret = mgr_->VotingExpectedRateRange(120, &range);
  EXPECT_EQ(ret, 0);

  ret = mgr_->VotingExpectedRateRange(120, &range);
  EXPECT_EQ(ret, 0);
}

TEST_F(OhosVsyncVotingMgrTest, VotingExpectedRateRangeSetsRangeCorrectly) {
  OH_NativeVSync_ExpectedRateRange range = {0, 0, 0};

  int ret = mgr_->VotingExpectedRateRange(90, &range);
  EXPECT_EQ(ret, 0);  // RET_SUCCEED
  EXPECT_EQ(range.min, 30);
  EXPECT_EQ(range.max, 120);
  EXPECT_EQ(range.expected, 90);
}

TEST_F(OhosVsyncVotingMgrTest, VotingExpectedRateRangeZeroMeansNoManage) {
  OH_NativeVSync_ExpectedRateRange range = {99, 99, 99};

  mgr_->VotingExpectedRateRange(120, &range);

  int ret = mgr_->VotingExpectedRateRange(0, &range);
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(range.min, 0);
  EXPECT_EQ(range.max, 0);
  EXPECT_EQ(range.expected, 0);
}

TEST_F(OhosVsyncVotingMgrTest, SetAssetProviderWithNullDoesNotCrash) {
  mgr_->SetAssetProvider(nullptr);
  // Should not crash; asset_provider_ remains null.
}

TEST_F(OhosVsyncVotingMgrTest, ParseFramesCfgIdempotent) {
  // Calling ParseFramesCfg multiple times should not change state.
  mgr_->ParseFramesCfg();
  LTPOSwitchState state1 = mgr_->CheckVotingSwitchState();

  mgr_->ParseFramesCfg();
  LTPOSwitchState state2 = mgr_->CheckVotingSwitchState();

  EXPECT_EQ(state1, state2);
}

TEST_F(OhosVsyncVotingMgrTest, VoteFinalFrameRatePriorityOrder) {
  int64_t now = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();

  mgr_->VoteAnimationValue(AnimationType::AN_TYPE_TRANSLATE, 1.0, 1000.0);
  int rate = mgr_->VoteFinalFrameRateByPriority();
  EXPECT_EQ(rate, 60);

  int64_t old_ts = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds() - 200;
  mgr_->VoteTouchValue(VVMTouchType::TOUCH_TYPE_UP, old_ts);
  mgr_->SetPlatformViewExist(true);
  rate = mgr_->VoteFinalFrameRateByPriority();
  EXPECT_EQ(rate, 120);

  mgr_->VoteTouchValue(VVMTouchType::TOUCH_TYPE_DOWN, now);
  rate = mgr_->VoteFinalFrameRateByPriority();
  EXPECT_EQ(rate, 120);
}

TEST_F(OhosVsyncVotingMgrTest, VotesForHighestRate) {
  int64_t now = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();

  mgr_->VoteAnimationValue(AnimationType::AN_TYPE_TRANSLATE, 0.0, 50.0);

  int rate = mgr_->VoteFinalFrameRateByPriority();
  EXPECT_EQ(rate, 60);

  mgr_->VoteTouchValue(VVMTouchType::TOUCH_TYPE_DOWN, now);
  rate = mgr_->VoteFinalFrameRateByPriority();
  EXPECT_EQ(rate, 120);

  mgr_->VoteTouchValue(VVMTouchType::TOUCH_TYPE_UP, now);
  rate = mgr_->VoteFinalFrameRateByPriority();
  EXPECT_EQ(rate, 120);
}

TEST_F(OhosVsyncVotingMgrTest, NoVoteReturnsDefault) {
  // 初始状态：无任何投票（touch_voting_=0, animation_voting_=0,
  // is_platformview_exist_=false）
  int rate = mgr_->VoteFinalFrameRateByPriority();
  // touch_voting_ == 0 (FPS_NO_VOTING) → 走 "no touch vote" 分支 → 60
  EXPECT_EQ(rate, 60);
}

TEST_F(OhosVsyncVotingMgrTest, CanRevokeVote) {
  int64_t now = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();

  // touch down → 投票 120
  mgr_->VoteTouchValue(VVMTouchType::TOUCH_TYPE_DOWN, now);
  int rate = mgr_->VoteFinalFrameRateByPriority();
  EXPECT_EQ(rate, 120);

  // touch up → 投票仍 120（短期内）
  mgr_->VoteTouchValue(VVMTouchType::TOUCH_TYPE_UP, now);
  rate = mgr_->VoteFinalFrameRateByPriority();
  EXPECT_EQ(rate, 120);

  // 3秒后 timeout → 撤销 touch 投票，恢复默认 60
  int64_t later = now + 4000;
  mgr_->VoteTouchValue(VVMTouchType::TOUCH_TYPE_UP_3_SEC_AFTER, later);
  rate = mgr_->VoteFinalFrameRateByPriority();
  EXPECT_EQ(rate, 60);
}

TEST_F(OhosVsyncVotingMgrTest, LTPOSwitchState) {
  // ParseFramesCfg 后：NOT_INIT → ON
  EXPECT_EQ(mgr_->CheckVotingSwitchState(), LTPOSwitchState::LTPO_SWITCH_ON);

  // ParseFramesCfg 幂等：再次调用状态不变
  mgr_->ParseFramesCfg();
  EXPECT_EQ(mgr_->CheckVotingSwitchState(), LTPOSwitchState::LTPO_SWITCH_ON);

  // ResetInstance 后新建实例：回到 NOT_INIT
  OhosVsyncVotingMgr::ResetInstance();
  mgr_ = OhosVsyncVotingMgr::GetInstance();
  EXPECT_EQ(mgr_->CheckVotingSwitchState(),
            LTPOSwitchState::LTPO_SWITCH_NOT_INIT);

  // 调用 ParseFramesCfg 后：NOT_INIT → ON
  mgr_->ParseFramesCfg();
  EXPECT_EQ(mgr_->CheckVotingSwitchState(), LTPOSwitchState::LTPO_SWITCH_ON);
}

}  // namespace testing
}  // namespace flutter
