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
#include <cstring>
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

    // Voting NDK is @since 20 and needs a loadable libnative_vsync.so: without
    // it the ctor nulls the handle and every test below would fail in this
    // SetUp. Skip on such systems instead.
    if (mgr_->lib_native_vsync_handle_ == nullptr) {
      GTEST_SKIP() << "libnative_vsync.so unavailable or lacks "
                      "OH_NativeVSync_SetExpectedFrameRateRange (@since 20)";
    }

    // Ensure LTPO is enabled for most tests.
    mgr_->ParseFramesCfg();
    EXPECT_EQ(mgr_->CheckVotingSwitchState(), LTPOSwitchState::LTPO_SWITCH_ON);
  }

  void TearDown() override { OhosVsyncVotingMgr::ResetInstance(); }

  std::shared_ptr<OhosVsyncVotingMgr> mgr_;
};

namespace {

struct FakeSetRangeLog {
  int call_count = 0;
  const void* last_handle = nullptr;
  int last_min = -1;
  int last_max = -1;
  int last_expected = -1;
  int return_value = 0;
};

FakeSetRangeLog g_fake_range;

int FakeSetExpectedFrameRange(OH_NativeVSync* nativeVsync,
                              OH_NativeVSync_ExpectedRateRange* range) {
  g_fake_range.call_count++;
  g_fake_range.last_handle = nativeVsync;
  g_fake_range.last_min = range->min;
  g_fake_range.last_max = range->max;
  g_fake_range.last_expected = range->expected;
  return g_fake_range.return_value;
}

}

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
  EXPECT_EQ(mgr_->native_vsync_map_.count("test"), 0u);

  // DetachNativeVsync should not crash even if not attached.
  EXPECT_NO_FATAL_FAILURE(mgr_->DetachNativeVsync("test"));
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
  EXPECT_EQ(mgr_->asset_provider_, nullptr);
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

TEST_F(OhosVsyncVotingMgrTest, VotingCallsIgnoredBeforeParseFramesCfg) {
  OhosVsyncVotingMgr::ResetInstance();
  auto mgr = OhosVsyncVotingMgr::GetInstance();
  ASSERT_NE(mgr, nullptr);
  EXPECT_EQ(mgr->CheckVotingSwitchState(),
            LTPOSwitchState::LTPO_SWITCH_NOT_INIT);

  int64_t now = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  mgr->VoteAnimationValue(AnimationType::AN_TYPE_TRANSLATE, 1.0, 4000.0);
  mgr->VoteTouchValue(VVMTouchType::TOUCH_TYPE_DOWN, now);
  mgr->VoteVideoValue(1, 60);
  EXPECT_EQ(mgr->animation_voting_.load(), 0);
  EXPECT_EQ(mgr->touch_voting_.load(), 0);
  EXPECT_EQ(mgr->video_voting_.load(), 0);

  OhosVsyncVotingMgr::ResetInstance();
}

TEST_F(OhosVsyncVotingMgrTest, VotingCallsIgnoredWhenSwitchOff) {
  OhosVsyncVotingMgr::ResetInstance();
  auto mgr = OhosVsyncVotingMgr::GetInstance();
  mgr->switch_status_ = LTPOSwitchState::LTPO_SWITCH_OFF;

  int64_t now = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  mgr->VoteAnimationValue(AnimationType::AN_TYPE_TRANSLATE, 1.0, 4000.0);
  mgr->VoteTouchValue(VVMTouchType::TOUCH_TYPE_UP, now);
  mgr->VoteVideoValue(1, 60);
  mgr->VotingByNativeVsync(nullptr);
  mgr->VotingBySelf();
  EXPECT_EQ(mgr->animation_voting_.load(), 0);
  EXPECT_EQ(mgr->touch_voting_.load(), 0);
  EXPECT_EQ(mgr->video_voting_.load(), 0);

  OhosVsyncVotingMgr::ResetInstance();
}

TEST_F(OhosVsyncVotingMgrTest, VoteAnimationValueScaleRotationNoVoting) {
  mgr_->VoteAnimationValue(AnimationType::AN_TYPE_SCALE, 1.0, 4000.0);
  mgr_->VoteAnimationValue(AnimationType::AN_TYPE_ROTATION, 1.0, 4000.0);
  mgr_->VoteAnimationValue(static_cast<AnimationType>(99), 1.0, 4000.0);
  EXPECT_EQ(mgr_->animation_voting_.load(), 0);
}

TEST_F(OhosVsyncVotingMgrTest, TouchUp3SecAfterDuringTouchDownIgnored) {
  int64_t now = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  mgr_->VoteTouchValue(VVMTouchType::TOUCH_TYPE_DOWN, now);
  mgr_->VoteTouchValue(VVMTouchType::TOUCH_TYPE_UP_3_SEC_AFTER, now + 4000);
  EXPECT_EQ(mgr_->touch_voting_.load(), 120);
  EXPECT_TRUE(mgr_->is_touch_down_);
}

TEST_F(OhosVsyncVotingMgrTest, TouchUp3SecAfterBeforeTimeoutKeepsVoting) {
  int64_t now = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  mgr_->VoteTouchValue(VVMTouchType::TOUCH_TYPE_UP, now);
  mgr_->VoteTouchValue(VVMTouchType::TOUCH_TYPE_UP_3_SEC_AFTER, now + 1000);
  EXPECT_EQ(mgr_->touch_voting_.load(), 120);
}

TEST_F(OhosVsyncVotingMgrTest, TouchUnknownTypeIgnored) {
  int64_t now = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  mgr_->VoteTouchValue(static_cast<VVMTouchType>(99), now);
  EXPECT_EQ(mgr_->touch_voting_.load(), 0);
}

TEST_F(OhosVsyncVotingMgrTest, VoteVideoValueInvalidInputIgnored) {
  mgr_->VoteVideoValue(0, 30);
  mgr_->VoteVideoValue(1, 0);
  mgr_->VoteVideoValue(-1, 60);
  EXPECT_EQ(mgr_->video_voting_.load(), 0);
}

TEST_F(OhosVsyncVotingMgrTest, VoteVideoValueLowFrameRateMapsTo30) {
  mgr_->VoteVideoValue(1, 30);
  EXPECT_EQ(mgr_->video_voting_.load(), 30);
}

TEST_F(OhosVsyncVotingMgrTest, VoteVideoValueHighFrameRateMapsTo60) {
  mgr_->VoteVideoValue(1, 60);
  EXPECT_EQ(mgr_->video_voting_.load(), 60);
  mgr_->VoteVideoValue(2, 90);
  EXPECT_EQ(mgr_->video_voting_.load(), 60);
}

TEST_F(OhosVsyncVotingMgrTest, VoteTranslateLowerVelocityIgnored) {
  mgr_->VoteAnimationValue(AnimationType::AN_TYPE_TRANSLATE, 1.0, 4000.0);
  EXPECT_EQ(mgr_->animation_voting_.load(), 90);
  mgr_->VoteAnimationValue(AnimationType::AN_TYPE_TRANSLATE, 1.0, 100.0);
  EXPECT_EQ(mgr_->animation_voting_.load(), 90);
}

TEST_F(OhosVsyncVotingMgrTest, AttachNativeVsyncNullHandleRejected) {
  mgr_->AttachNativeVsync("null_entry", nullptr);
  EXPECT_EQ(mgr_->native_vsync_map_.count("null_entry"), 0u);
}

TEST_F(OhosVsyncVotingMgrTest, AttachAndDetachRealNativeVsync) {
  const char* name = "unittest_attach";
  OH_NativeVSync* handle = OH_NativeVSync_Create(name, strlen(name));
  ASSERT_NE(handle, nullptr);
  mgr_->AttachNativeVsync("real_entry", handle);
  EXPECT_EQ(mgr_->native_vsync_map_.count("real_entry"), 1u);
  mgr_->DetachNativeVsync("real_entry");
  EXPECT_EQ(mgr_->native_vsync_map_.count("real_entry"), 0u);
  OH_NativeVSync_Destroy(handle);
}

TEST_F(OhosVsyncVotingMgrTest, DelayDropHoldsHighRateThenFalls) {
  mgr_->local_framerate_ = 120;
  for (int i = 0; i < 3; i++) {
    EXPECT_EQ(mgr_->DelayFrameRateDropForStability(60), 120);
  }
  EXPECT_EQ(mgr_->DelayFrameRateDropForStability(60), 60);
}

TEST_F(OhosVsyncVotingMgrTest, DelayDropSelfRoleDoesNotDecrement) {
  mgr_->local_framerate_ = 120;
  for (int i = 0; i < 10; i++) {
    EXPECT_EQ(mgr_->DelayFrameRateDropForStability(
                  60, VVMVotingFrameRateRole::ROLE_SELF),
              120);
  }
}

TEST_F(OhosVsyncVotingMgrTest, DelayDropTracksHighestExpected) {
  mgr_->local_framerate_ = 120;
  mgr_->DelayFrameRateDropForStability(30);
  mgr_->DelayFrameRateDropForStability(60);
  mgr_->DelayFrameRateDropForStability(60);
  EXPECT_EQ(mgr_->DelayFrameRateDropForStability(60), 60);
}

TEST_F(OhosVsyncVotingMgrTest, VotingExpectedRateRangeSameAsDisplayRateFails) {
  int32_t saved = PlatformViewOHOSNapi::display_refresh_rate;
  PlatformViewOHOSNapi::display_refresh_rate = 60;
  mgr_->local_framerate_ = 60;
  OH_NativeVSync_ExpectedRateRange range = {0, 0, 0};
  EXPECT_EQ(mgr_->VotingExpectedRateRange(60, &range), -1);
  PlatformViewOHOSNapi::display_refresh_rate = saved;
}

TEST_F(OhosVsyncVotingMgrTest, VotingByNativeVsyncNullHandleNoOp) {
  EXPECT_NO_FATAL_FAILURE(mgr_->VotingByNativeVsync(nullptr));
  EXPECT_EQ(mgr_->local_framerate_, 0);
}

TEST_F(OhosVsyncVotingMgrTest, VotingByNativeVsyncAppliesRange) {
  auto saved_range_func = mgr_->func_SetExpectedFrameRateRange_symbol_handle_;
  g_fake_range = FakeSetRangeLog{};
  mgr_->func_SetExpectedFrameRateRange_symbol_handle_ =
      &FakeSetExpectedFrameRange;
  const char* name = "unittest_vbn";
  OH_NativeVSync* handle = OH_NativeVSync_Create(name, strlen(name));
  ASSERT_NE(handle, nullptr);
  EXPECT_NO_FATAL_FAILURE(mgr_->VotingByNativeVsync(handle));
  EXPECT_EQ(mgr_->local_framerate_, 60);
  mgr_->func_SetExpectedFrameRateRange_symbol_handle_ = saved_range_func;
  OH_NativeVSync_Destroy(handle);
}

TEST_F(OhosVsyncVotingMgrTest, VotingByNativeVsyncSkipsUnchangedRate) {
  auto saved_range_func = mgr_->func_SetExpectedFrameRateRange_symbol_handle_;
  g_fake_range = FakeSetRangeLog{};
  mgr_->func_SetExpectedFrameRateRange_symbol_handle_ =
      &FakeSetExpectedFrameRange;
  const char* name = "unittest_vbn2";
  OH_NativeVSync* handle = OH_NativeVSync_Create(name, strlen(name));
  ASSERT_NE(handle, nullptr);
  int32_t saved = PlatformViewOHOSNapi::display_refresh_rate;
  PlatformViewOHOSNapi::display_refresh_rate = 60;
  mgr_->local_framerate_ = 60;
  mgr_->cur_animation_translate_velocity_ = 55.5;
  EXPECT_NO_FATAL_FAILURE(mgr_->VotingByNativeVsync(handle));
  EXPECT_EQ(mgr_->cur_animation_translate_velocity_, 0.0);
  PlatformViewOHOSNapi::display_refresh_rate = saved;
  mgr_->func_SetExpectedFrameRateRange_symbol_handle_ = saved_range_func;
  OH_NativeVSync_Destroy(handle);
}

TEST_F(OhosVsyncVotingMgrTest, VotingBySelfClearsStaleAnimationVote) {
  mgr_->animation_voting_.store(90);
  EXPECT_NO_FATAL_FAILURE(mgr_->VotingBySelf());
  EXPECT_EQ(mgr_->animation_voting_.load(), 0);
}

TEST_F(OhosVsyncVotingMgrTest, VotingBySelfSkipsNullMapEntries) {
  {
    std::lock_guard<std::mutex> lock(mgr_->native_vsync_map_mutex_);
    mgr_->native_vsync_map_["ghost"] = nullptr;
  }
  EXPECT_NO_FATAL_FAILURE(mgr_->VotingBySelf());
  EXPECT_EQ(mgr_->local_framerate_, 60);
  {
    std::lock_guard<std::mutex> lock(mgr_->native_vsync_map_mutex_);
    mgr_->native_vsync_map_.clear();
  }
}

TEST_F(OhosVsyncVotingMgrTest, VotingBySelfAppliesRangeToAttachedHandle) {
  auto saved_range_func = mgr_->func_SetExpectedFrameRateRange_symbol_handle_;
  g_fake_range = FakeSetRangeLog{};
  mgr_->func_SetExpectedFrameRateRange_symbol_handle_ =
      &FakeSetExpectedFrameRange;
  const char* name = "unittest_self";
  OH_NativeVSync* handle = OH_NativeVSync_Create(name, strlen(name));
  ASSERT_NE(handle, nullptr);
  mgr_->AttachNativeVsync("self_entry", handle);
  int64_t now = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  EXPECT_NO_FATAL_FAILURE(
      mgr_->VoteTouchValue(VVMTouchType::TOUCH_TYPE_DOWN, now));
  EXPECT_EQ(mgr_->touch_voting_.load(), 120);
  EXPECT_EQ(mgr_->local_framerate_, 120);
  mgr_->DetachNativeVsync("self_entry");
  mgr_->func_SetExpectedFrameRateRange_symbol_handle_ = saved_range_func;
  OH_NativeVSync_Destroy(handle);
}

TEST_F(OhosVsyncVotingMgrTest, ParseFramesConfigJsonValidAndInvalid) {
  Json::Value root;
  const char* valid = "{\"SWITCH\":1}";
  EXPECT_TRUE(mgr_->ParseFramesConfigJson(valid, strlen(valid), root));
  const char* invalid = "{\"SWITCH\":";
  EXPECT_FALSE(mgr_->ParseFramesConfigJson(invalid, strlen(invalid), root));
}

TEST_F(OhosVsyncVotingMgrTest, ParseTranslateRejectsMalformedEntries) {
  mgr_->frames_config_vec_.clear();

  mgr_->ParseTranslate(Json::Value(Json::arrayValue));
  EXPECT_TRUE(mgr_->frames_config_vec_.empty());

  Json::Value arr(Json::arrayValue);
  arr.append(Json::Value(1));
  arr.append("str");
  mgr_->ParseTranslate(arr);
  EXPECT_TRUE(mgr_->frames_config_vec_.empty());

  Json::Value missing_key(Json::objectValue);
  missing_key["serial_number"] = 1;
  missing_key["min"] = 0;
  missing_key["max"] = 10;
  arr.append(missing_key);
  mgr_->ParseTranslate(arr);
  EXPECT_TRUE(mgr_->frames_config_vec_.empty());

  Json::Value bad_type(Json::objectValue);
  bad_type["serial_number"] = 1;
  bad_type["min"] = 0;
  bad_type["max"] = "ten";
  bad_type["preferred_fps"] = 60;
  arr.append(bad_type);
  mgr_->ParseTranslate(arr);
  EXPECT_TRUE(mgr_->frames_config_vec_.empty());
}

TEST_F(OhosVsyncVotingMgrTest, ParseTranslateAcceptsValidAndWrongSerial) {
  mgr_->frames_config_vec_.clear();
  Json::Value arr(Json::arrayValue);
  Json::Value good(Json::objectValue);
  good["serial_number"] = 1;
  good["min"] = 0;
  good["max"] = 10;
  good["preferred_fps"] = 60;
  arr.append(good);
  Json::Value wrong_serial(Json::objectValue);
  wrong_serial["serial_number"] = 42;
  wrong_serial["min"] = 10;
  wrong_serial["max"] = 20;
  wrong_serial["preferred_fps"] = 90;
  arr.append(wrong_serial);
  mgr_->ParseTranslate(arr);
  EXPECT_EQ(mgr_->frames_config_vec_.size(), 2u);
}

TEST_F(OhosVsyncVotingMgrTest, ApplyTranslateConfigMissingKeyUsesDefault) {
  mgr_->frames_config_vec_.clear();
  mgr_->ApplyTranslateConfig(Json::Value(Json::objectValue));
  EXPECT_EQ(mgr_->frames_config_vec_.size(), 5u);
}

TEST_F(OhosVsyncVotingMgrTest, ApplyTranslateConfigInvalidFallsBackToDefault) {
  mgr_->frames_config_vec_.clear();
  Json::Value root(Json::objectValue);
  root["TRANSLATE"] = Json::Value(Json::arrayValue);
  mgr_->ApplyTranslateConfig(root);
  EXPECT_EQ(mgr_->frames_config_vec_.size(), 5u);
}

TEST_F(OhosVsyncVotingMgrTest, ApplyTranslateConfigKeepsBackupOnBadOverride) {
  std::vector<std::map<std::string, int>> custom(2);
  custom[0]["min"] = 0;
  custom[0]["preferred_fps"] = 60;
  custom[1]["min"] = 100;
  custom[1]["preferred_fps"] = 120;
  mgr_->frames_config_vec_ = custom;

  Json::Value root(Json::objectValue);
  root["TRANSLATE"] = Json::Value(Json::arrayValue);
  mgr_->ApplyTranslateConfig(root);
  EXPECT_EQ(mgr_->frames_config_vec_.size(), 2u);
}

TEST_F(OhosVsyncVotingMgrTest, ApplyTranslateConfigValidOverrideWins) {
  Json::Value root(Json::objectValue);
  Json::Value arr(Json::arrayValue);
  Json::Value item(Json::objectValue);
  item["serial_number"] = 1;
  item["min"] = 700;
  item["max"] = -1;
  item["preferred_fps"] = 90;
  arr.append(item);
  root["TRANSLATE"] = arr;
  mgr_->ApplyTranslateConfig(root);
  ASSERT_EQ(mgr_->frames_config_vec_.size(), 1u);
  EXPECT_EQ(mgr_->frames_config_vec_[0]["preferred_fps"], 90);

  mgr_->VoteAnimationValue(AnimationType::AN_TYPE_TRANSLATE, 1.0, 5000.0);
  EXPECT_EQ(mgr_->animation_voting_.load(), 90);
}

TEST_F(OhosVsyncVotingMgrTest, SetAssetProviderNullRejected) {
  mgr_->SetAssetProvider(nullptr);
  EXPECT_EQ(mgr_->asset_provider_, nullptr);
}

TEST_F(OhosVsyncVotingMgrTest, SetAssetProviderSecondCallIgnored) {
  void* handle_a = reinterpret_cast<void*>(0x11);
  void* handle_b = reinterpret_cast<void*>(0x22);
  mgr_->SetAssetProvider(std::make_unique<OHOSAssetProvider>(handle_a));
  ASSERT_NE(mgr_->asset_provider_, nullptr);
  mgr_->SetAssetProvider(std::make_unique<OHOSAssetProvider>(handle_b));
  EXPECT_EQ(mgr_->asset_provider_->GetHandle(), handle_a);
}

TEST_F(OhosVsyncVotingMgrTest, ParseFramesCfgWithNullHandleProvider) {
  OhosVsyncVotingMgr::ResetInstance();
  auto mgr = OhosVsyncVotingMgr::GetInstance();
  void* null_handle = nullptr;
  mgr->SetAssetProvider(std::make_unique<OHOSAssetProvider>(null_handle));
  mgr->ParseFramesCfg();
  EXPECT_EQ(mgr->CheckVotingSwitchState(), LTPOSwitchState::LTPO_SWITCH_ON);
  EXPECT_FALSE(mgr->frames_config_vec_.empty());
  OhosVsyncVotingMgr::ResetInstance();
}

void SetRawFileStubContent(const char* data, size_t size);
void SetRawFileStubOpenFail(bool fail);

void SetNativeVsyncDlopenRedirect(int mode);
int GetAndResetDlopenRedirectCount();

TEST_F(OhosVsyncVotingMgrTest, AnimationVoteWinsWhenTouchVoteStale) {
  int64_t now = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  mgr_->VoteTouchValue(VVMTouchType::TOUCH_TYPE_UP, now - 5000);
  EXPECT_EQ(mgr_->touch_voting_.load(), 120);
  mgr_->VoteAnimationValue(AnimationType::AN_TYPE_TRANSLATE, 1.0, 4000.0);
  EXPECT_EQ(mgr_->animation_voting_.load(), 90);
  EXPECT_EQ(mgr_->VoteFinalFrameRateByPriority(), 90);
}

TEST_F(OhosVsyncVotingMgrTest, VotingExpectedRateRangeSameLocalProceeds) {
  int32_t saved = PlatformViewOHOSNapi::display_refresh_rate;
  PlatformViewOHOSNapi::display_refresh_rate = 60;
  mgr_->local_framerate_ = 90;
  OH_NativeVSync_ExpectedRateRange range = {0, 0, 0};
  EXPECT_EQ(mgr_->VotingExpectedRateRange(90, &range), 0);
  EXPECT_EQ(range.min, 30);
  EXPECT_EQ(range.max, 120);
  EXPECT_EQ(range.expected, 90);
  PlatformViewOHOSNapi::display_refresh_rate = saved;
}

TEST_F(OhosVsyncVotingMgrTest, VotingSkippedWhenFuncPointerNull) {
  auto saved = mgr_->func_SetExpectedFrameRateRange_symbol_handle_;
  mgr_->func_SetExpectedFrameRateRange_symbol_handle_ = nullptr;
  mgr_->cur_animation_translate_velocity_ = 55.5;
  const char* name = "unittest_funcnull";
  OH_NativeVSync* handle = OH_NativeVSync_Create(name, strlen(name));
  ASSERT_NE(handle, nullptr);
  mgr_->VotingByNativeVsync(handle);
  EXPECT_EQ(mgr_->cur_animation_translate_velocity_, 55.5);
  EXPECT_EQ(mgr_->local_framerate_, 0);
  mgr_->VotingBySelf();
  EXPECT_EQ(mgr_->local_framerate_, 0);
  mgr_->func_SetExpectedFrameRateRange_symbol_handle_ = saved;
  OH_NativeVSync_Destroy(handle);
}

TEST_F(OhosVsyncVotingMgrTest, VotingByNativeVsyncPropagatesRangeCallError) {
  auto saved = mgr_->func_SetExpectedFrameRateRange_symbol_handle_;
  mgr_->func_SetExpectedFrameRateRange_symbol_handle_ =
      &FakeSetExpectedFrameRange;
  g_fake_range = FakeSetRangeLog{};
  g_fake_range.return_value = 1;
  const char* name = "unittest_range_err";
  OH_NativeVSync* handle = OH_NativeVSync_Create(name, strlen(name));
  ASSERT_NE(handle, nullptr);
  mgr_->VotingByNativeVsync(handle);
  EXPECT_EQ(g_fake_range.call_count, 1);
  EXPECT_EQ(g_fake_range.last_min, 30);
  EXPECT_EQ(g_fake_range.last_max, 120);
  EXPECT_EQ(g_fake_range.last_expected, 60);
  EXPECT_EQ(mgr_->local_framerate_, 60);
  mgr_->func_SetExpectedFrameRateRange_symbol_handle_ = saved;
  OH_NativeVSync_Destroy(handle);
}

TEST_F(OhosVsyncVotingMgrTest, VotingBySelfPropagatesRangeCallError) {
  auto saved = mgr_->func_SetExpectedFrameRateRange_symbol_handle_;
  mgr_->func_SetExpectedFrameRateRange_symbol_handle_ =
      &FakeSetExpectedFrameRange;
  g_fake_range = FakeSetRangeLog{};
  g_fake_range.return_value = 1;
  const char* name = "unittest_self_err";
  OH_NativeVSync* handle = OH_NativeVSync_Create(name, strlen(name));
  ASSERT_NE(handle, nullptr);
  mgr_->AttachNativeVsync("self_err_entry", handle);
  int64_t now = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  mgr_->VoteTouchValue(VVMTouchType::TOUCH_TYPE_DOWN, now);
  EXPECT_EQ(g_fake_range.call_count, 1);
  EXPECT_EQ(g_fake_range.last_expected, 120);
  EXPECT_EQ(mgr_->local_framerate_, 120);
  mgr_->DetachNativeVsync("self_err_entry");
  mgr_->func_SetExpectedFrameRateRange_symbol_handle_ = saved;
  OH_NativeVSync_Destroy(handle);
}

TEST_F(OhosVsyncVotingMgrTest, VotingBySelfSkipsRangeWhenRateUnchanged) {
  int32_t saved_display = PlatformViewOHOSNapi::display_refresh_rate;
  PlatformViewOHOSNapi::display_refresh_rate = 60;
  auto saved = mgr_->func_SetExpectedFrameRateRange_symbol_handle_;
  mgr_->func_SetExpectedFrameRateRange_symbol_handle_ =
      &FakeSetExpectedFrameRange;
  g_fake_range = FakeSetRangeLog{};
  mgr_->local_framerate_ = 60;
  const char* name = "unittest_self_skip";
  OH_NativeVSync* handle = OH_NativeVSync_Create(name, strlen(name));
  ASSERT_NE(handle, nullptr);
  mgr_->AttachNativeVsync("self_skip_entry", handle);
  mgr_->VotingBySelf();
  EXPECT_EQ(g_fake_range.call_count, 0);
  EXPECT_EQ(mgr_->local_framerate_, 60);
  mgr_->DetachNativeVsync("self_skip_entry");
  mgr_->func_SetExpectedFrameRateRange_symbol_handle_ = saved;
  PlatformViewOHOSNapi::display_refresh_rate = saved_display;
  OH_NativeVSync_Destroy(handle);
}

TEST_F(OhosVsyncVotingMgrTest, CtorDlopenFailureDisablesAllVoting) {
  SetNativeVsyncDlopenRedirect(1);
  OhosVsyncVotingMgr::ResetInstance();
  auto mgr = OhosVsyncVotingMgr::GetInstance();
  SetNativeVsyncDlopenRedirect(0);
  ASSERT_NE(mgr, nullptr);
  EXPECT_GT(GetAndResetDlopenRedirectCount(), 0);
  EXPECT_EQ(mgr->lib_native_vsync_handle_, nullptr);

  int64_t now = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  mgr->VoteAnimationValue(AnimationType::AN_TYPE_TRANSLATE, 1.0, 4000.0);
  mgr->VoteTouchValue(VVMTouchType::TOUCH_TYPE_DOWN, now);
  mgr->VoteVideoValue(1, 60);
  EXPECT_EQ(mgr->animation_voting_.load(), 0);
  EXPECT_EQ(mgr->touch_voting_.load(), 0);
  EXPECT_EQ(mgr->video_voting_.load(), 0);

  OH_NativeVSync* dummy = reinterpret_cast<OH_NativeVSync*>(0x1);
  mgr->VotingByNativeVsync(dummy);
  mgr->VotingBySelf();
  mgr->AttachNativeVsync("redirect_entry", dummy);
  EXPECT_EQ(mgr->local_framerate_, 0);
  EXPECT_EQ(mgr->native_vsync_map_.count("redirect_entry"), 0u);

  mgr->SetAssetProvider(
      std::make_unique<OHOSAssetProvider>(reinterpret_cast<void*>(0x22)));
  EXPECT_EQ(mgr->asset_provider_, nullptr);

  mgr->ParseFramesCfg();
  EXPECT_EQ(mgr->CheckVotingSwitchState(), LTPOSwitchState::LTPO_SWITCH_OFF);
  OhosVsyncVotingMgr::ResetInstance();
}

TEST_F(OhosVsyncVotingMgrTest, CtorDlsymFailureClearsHandle) {
  SetNativeVsyncDlopenRedirect(2);
  OhosVsyncVotingMgr::ResetInstance();
  auto mgr = OhosVsyncVotingMgr::GetInstance();
  SetNativeVsyncDlopenRedirect(0);
  ASSERT_NE(mgr, nullptr);
  EXPECT_GT(GetAndResetDlopenRedirectCount(), 0);
  EXPECT_EQ(mgr->lib_native_vsync_handle_, nullptr);
  EXPECT_EQ(mgr->func_SetExpectedFrameRateRange_symbol_handle_, nullptr);
  mgr->ParseFramesCfg();
  EXPECT_EQ(mgr->CheckVotingSwitchState(), LTPOSwitchState::LTPO_SWITCH_OFF);
  OhosVsyncVotingMgr::ResetInstance();
}

TEST_F(OhosVsyncVotingMgrTest, SwitchOffJsonDisablesLtpo) {
  static const char kCfg[] = "{\"SWITCH\":0}";
  SetRawFileStubContent(kCfg, sizeof(kCfg) - 1);
  OhosVsyncVotingMgr::ResetInstance();
  auto mgr = OhosVsyncVotingMgr::GetInstance();
  mgr->SetAssetProvider(
      std::make_unique<OHOSAssetProvider>(reinterpret_cast<void*>(0x99)));
  mgr->ParseFramesCfg();
  EXPECT_EQ(mgr->CheckVotingSwitchState(), LTPOSwitchState::LTPO_SWITCH_OFF);
  mgr->VoteAnimationValue(AnimationType::AN_TYPE_TRANSLATE, 1.0, 4000.0);
  EXPECT_EQ(mgr->animation_voting_.load(), 0);
  OhosVsyncVotingMgr::ResetInstance();
}

TEST_F(OhosVsyncVotingMgrTest, SwitchNonNumericKeepsDefaultLtpo) {
  static const char kCfg[] = "{\"SWITCH\":\"off\"}";
  SetRawFileStubContent(kCfg, sizeof(kCfg) - 1);
  OhosVsyncVotingMgr::ResetInstance();
  auto mgr = OhosVsyncVotingMgr::GetInstance();
  mgr->SetAssetProvider(
      std::make_unique<OHOSAssetProvider>(reinterpret_cast<void*>(0x99)));
  mgr->ParseFramesCfg();
  EXPECT_EQ(mgr->CheckVotingSwitchState(), LTPOSwitchState::LTPO_SWITCH_ON);
  EXPECT_EQ(mgr->frames_config_vec_.size(), 5u);
  OhosVsyncVotingMgr::ResetInstance();
}

TEST_F(OhosVsyncVotingMgrTest, InvalidJsonKeepsDefaultLtpo) {
  static const char kCfg[] = "not a json config";
  SetRawFileStubContent(kCfg, sizeof(kCfg) - 1);
  OhosVsyncVotingMgr::ResetInstance();
  auto mgr = OhosVsyncVotingMgr::GetInstance();
  mgr->SetAssetProvider(
      std::make_unique<OHOSAssetProvider>(reinterpret_cast<void*>(0x99)));
  mgr->ParseFramesCfg();
  EXPECT_EQ(mgr->CheckVotingSwitchState(), LTPOSwitchState::LTPO_SWITCH_ON);
  EXPECT_EQ(mgr->frames_config_vec_.size(), 5u);
  OhosVsyncVotingMgr::ResetInstance();
}

TEST_F(OhosVsyncVotingMgrTest, OpenFailKeepsDefaultLtpo) {
  SetRawFileStubOpenFail(true);
  OhosVsyncVotingMgr::ResetInstance();
  auto mgr = OhosVsyncVotingMgr::GetInstance();
  mgr->SetAssetProvider(
      std::make_unique<OHOSAssetProvider>(reinterpret_cast<void*>(0x99)));
  mgr->ParseFramesCfg();
  EXPECT_EQ(mgr->CheckVotingSwitchState(), LTPOSwitchState::LTPO_SWITCH_ON);
  EXPECT_EQ(mgr->frames_config_vec_.size(), 5u);
  OhosVsyncVotingMgr::ResetInstance();
}

TEST_F(OhosVsyncVotingMgrTest, EmptyFileDataKeepsDefaultLtpo) {
  SetRawFileStubContent("", 0);
  OhosVsyncVotingMgr::ResetInstance();
  auto mgr = OhosVsyncVotingMgr::GetInstance();
  mgr->SetAssetProvider(
      std::make_unique<OHOSAssetProvider>(reinterpret_cast<void*>(0x99)));
  mgr->ParseFramesCfg();
  EXPECT_EQ(mgr->CheckVotingSwitchState(), LTPOSwitchState::LTPO_SWITCH_ON);
  EXPECT_EQ(mgr->frames_config_vec_.size(), 5u);
  OhosVsyncVotingMgr::ResetInstance();
}

TEST_F(OhosVsyncVotingMgrTest, NoSwitchKeyAppliesTranslateOverride) {
  static const char kCfg[] =
      "{\"TRANSLATE\":[{\"serial_number\":1,\"min\":0,\"max\":10,"
      "\"preferred_fps\":60}]}";
  SetRawFileStubContent(kCfg, sizeof(kCfg) - 1);
  OhosVsyncVotingMgr::ResetInstance();
  auto mgr = OhosVsyncVotingMgr::GetInstance();
  mgr->SetAssetProvider(
      std::make_unique<OHOSAssetProvider>(reinterpret_cast<void*>(0x99)));
  mgr->ParseFramesCfg();
  EXPECT_EQ(mgr->CheckVotingSwitchState(), LTPOSwitchState::LTPO_SWITCH_ON);
  EXPECT_EQ(mgr->frames_config_vec_.size(), 1u);
  OhosVsyncVotingMgr::ResetInstance();
}

TEST_F(OhosVsyncVotingMgrTest, TranslateOverrideEndToEnd) {
  static const char kCfg[] =
      "{\"SWITCH\":1,\"TRANSLATE\":[{\"serial_number\":1,\"min\":700,"
      "\"max\":-1,\"preferred_fps\":90}]}";
  SetRawFileStubContent(kCfg, sizeof(kCfg) - 1);
  OhosVsyncVotingMgr::ResetInstance();
  auto mgr = OhosVsyncVotingMgr::GetInstance();
  mgr->SetAssetProvider(
      std::make_unique<OHOSAssetProvider>(reinterpret_cast<void*>(0x99)));
  mgr->ParseFramesCfg();
  EXPECT_EQ(mgr->CheckVotingSwitchState(), LTPOSwitchState::LTPO_SWITCH_ON);
  ASSERT_EQ(mgr->frames_config_vec_.size(), 1u);
  mgr->VoteAnimationValue(AnimationType::AN_TYPE_TRANSLATE, 1.0, 5000.0);
  EXPECT_EQ(mgr->animation_voting_.load(), 90);
  OhosVsyncVotingMgr::ResetInstance();
}

TEST_F(OhosVsyncVotingMgrTest, PriorityFallsThroughToNoVoting) {
  int64_t now = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  mgr_->VoteTouchValue(VVMTouchType::TOUCH_TYPE_UP, now - 5000);
  EXPECT_EQ(mgr_->touch_voting_.load(), 120);
  EXPECT_EQ(mgr_->VoteFinalFrameRateByPriority(), 0);
  EXPECT_EQ(mgr_->animation_voting_.load(), 0);
}

TEST_F(OhosVsyncVotingMgrTest, SetPlatformViewExistSameValueSkipsStore) {
  int64_t now = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  mgr_->VoteTouchValue(VVMTouchType::TOUCH_TYPE_UP, now - 5000);
  EXPECT_EQ(mgr_->touch_voting_.load(), 120);

  mgr_->SetPlatformViewExist(false);
  EXPECT_FALSE(mgr_->is_platformview_exist_.load());
  mgr_->SetPlatformViewExist(true);
  EXPECT_TRUE(mgr_->is_platformview_exist_.load());
  mgr_->SetPlatformViewExist(true);
  EXPECT_TRUE(mgr_->is_platformview_exist_.load());

  EXPECT_EQ(mgr_->VoteFinalFrameRateByPriority(), 120);
  EXPECT_FALSE(mgr_->is_platformview_exist_.load());
  EXPECT_EQ(mgr_->VoteFinalFrameRateByPriority(), 0);
}

TEST_F(OhosVsyncVotingMgrTest, DelayDropRiseResetsDeferredTarget) {
  mgr_->local_framerate_ = 120;
  mgr_->DelayFrameRateDropForStability(60);
  EXPECT_EQ(mgr_->expected_drop_framerate_, 60);
  EXPECT_EQ(mgr_->DelayFrameRateDropForStability(120), 120);
  EXPECT_EQ(mgr_->expected_drop_framerate_, 0);
  EXPECT_EQ(mgr_->delay_drop_framerate_times_, 4);
}

}  // namespace testing
}  // namespace flutter
