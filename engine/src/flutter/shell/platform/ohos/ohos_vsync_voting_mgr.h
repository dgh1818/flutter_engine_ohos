/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */
#ifndef FLUTTER_SHELL_PLATFORM_OHOS_OHOS_VSYNC_VOTING_MGR_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_OHOS_VSYNC_VOTING_MGR_H_

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>

#include <json/json.h>

#include <native_vsync/native_vsync.h>

#include "flutter/fml/time/time_point.h"
#include "flutter/shell/platform/ohos/ohos_asset_provider.h"

namespace flutter {
using namespace std;
using SetExpectedFrameRateRangeFunc_ =
  int (*)(OH_NativeVSync* nativeVsync,
          OH_NativeVSync_ExpectedRateRange* range);

enum class LTPOSwitchState {
  LTPO_SWITCH_OFF = 0,
  LTPO_SWITCH_ON = 1,
  LTPO_SWITCH_NOT_INIT = 2,
};

enum class AnimationType {
  AN_TYPE_TRANSLATE = 0,
  AN_TYPE_SCALE,
  AN_TYPE_ROTATION,
};

enum class VVMTouchType {
  TOUCH_TYPE_DOWN,
  TOUCH_TYPE_UP,
  TOUCH_TYPE_UP_3_SEC_AFTER,
};

enum class VVMVotingType {
  VOTING_TYPE_NOTHING,
  VOTING_TYPE_TOUCH_DOWN_FPS_120,
  VOTING_TYPE_TOUCH_UP_FPS_60,
  VOTING_TYPE_TOUCH_UP_FPS_120,
  VOTING_TYPE_COMMON_PLATFORMVIEW_FPS_120,
  VOTING_TYPE_ANIMATION,
};

enum class VVMVotingFrameRateRole {
  ROLE_SELF,
  ROLE_EX_MODULE,
};

class OhosVsyncVotingMgr {
public:
  OhosVsyncVotingMgr();

  ~OhosVsyncVotingMgr();

  OhosVsyncVotingMgr(OhosVsyncVotingMgr&) = delete;

  OhosVsyncVotingMgr& operator=(const OhosVsyncVotingMgr&) = delete;

  static shared_ptr<OhosVsyncVotingMgr> GetInstance();

  void VoteAnimationValue(AnimationType AN_type,
                          double device_pixel_ratio,
                          double velocity);

  void VoteTouchValue(VVMTouchType type, int64_t timestamp);

  void VoteVideoValue(int second, int frame_count);

  void AttachNativeVsync(string handle_name, OH_NativeVSync* handle);

  void DetachNativeVsync(string handle_name);

  void VotingByNativeVsync(OH_NativeVSync* handle);

  void ParseFramesCfg();

  void SetAssetProvider(std::unique_ptr<OHOSAssetProvider> hap_asset_provider);

  void SetPlatformViewExist(bool is_exist);

  LTPOSwitchState CheckVotingSwitchState();

private:
  int ParseFramesCfgImpl();

  void VoteANTranslate(double velocity);

  void VotingBySelf();

  void ParseTranslate(const Json::Value& arr);

  int VoteFinalFrameRateByPriority();

  int DelayFrameRateDropForStability(
    int next_framerate,
    VVMVotingFrameRateRole type = VVMVotingFrameRateRole::ROLE_EX_MODULE
  );

  int VotingExpectedRateRange(
    int result_frameRate,
    OH_NativeVSync_ExpectedRateRange* range
  );

private:
  // The expected voting frame rate from animation.
  atomic<int> animation_voting_ = 0;

  // The expected voting frame rate from touch event.
  atomic<int> touch_voting_ = 0;

  // The expected voting frame rate from video.
  atomic<int> video_voting_ = 0;

  atomic<bool> is_platformview_exist_ = false;

  atomic<int64_t> touch_timestamp_ = 0;

  int local_framerate_ = 0;

  LTPOSwitchState switch_status_ = LTPOSwitchState::LTPO_SWITCH_NOT_INIT;

  bool is_touch_down_ = false;

  bool is_frames_config_file_init_ = false;

  double cur_animation_translate_velocity_ = 0.0;

  map<string, OH_NativeVSync*> native_vsync_map_;

  unique_ptr<OHOSAssetProvider> asset_provider_;

  vector<map<string, int> > frames_config_vec_;

  // pointing to the lib of OH_NativeVSync_SetExpectedFrameRateRange
  void* lib_native_vsync_handle_;

  // call the OH_NativeVSync_SetExpectedFrameRateRange function
  SetExpectedFrameRateRangeFunc_ func_SetExpectedFrameRateRange_symbol_handle_ = nullptr;

  int delay_drop_framerate_times_ = 0;

  int expected_drop_framerate_ = 0;

  std::mutex native_vsync_map_mutex_;
};  // class OhosVsyncVotingMgr

}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_OHOS_VSYNC_VOTING_MGR_H_
