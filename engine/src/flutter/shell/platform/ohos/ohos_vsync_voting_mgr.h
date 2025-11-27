/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */
#ifndef FLUTTER_SHELL_PLATFORM_OHOS_OHOS_VSYNC_VOTING_MGR_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_OHOS_VSYNC_VOTING_MGR_H_

#include <json/json.h>
#include <cstdint>
#include <map>
#include <memory>


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

class OhosVsyncVotingMgr {
 public:
  OhosVsyncVotingMgr();

  ~OhosVsyncVotingMgr();

  OhosVsyncVotingMgr(OhosVsyncVotingMgr&) = delete;

  OhosVsyncVotingMgr& operator=(const OhosVsyncVotingMgr&) = delete;

  static shared_ptr<OhosVsyncVotingMgr> GetInstance(void);

  void VoteAnimationValue(AnimationType ANType,
                          double devicePixelRatio,
                          double velocity);

  void VoteTouchValue(VVMTouchType type, int64_t timestamp);

  void VoteVideoValue(int second, int frameCount);

  void AttachNativeVsync(string handleName, OH_NativeVSync* handle);

  void DettachNativeVsync(string handleName);

  void VotingByNativeVsync(OH_NativeVSync* handle);

  void ParseFramesCfg(void);

  void SetAssetProvider(std::unique_ptr<OHOSAssetProvider> hap_asset_provider);

  void SetPlatformViewExist(bool isExist);

  LTPOSwitchState CheckVotingSwitchState(void);

 private:

  int ParseFramesCfgImpl(void);

  void VoteANTranslate(double velocity);

  void VotingBySelf();

  void ParseTranslate(const Json::Value& arr);

 private:
  // The expected voting frame rate from animation.
  atomic<int> animationVoting_ = 0;

  // The temporary voting frame rate from animation.
  // Needed to further decide on the final frame rate of the animation.
  atomic<int> animationVotingTemp_ = 0;

  // The expected voting frame rate from touch event.
  atomic<int> touchVoting_ = 0;

  // The expected voting frame rate from video.
  atomic<int> videoVoting_ = 0;

  atomic<bool> isPlatformViewExist_ = false;

  int localFrameRate_ = 0;

  LTPOSwitchState switchStatus_ = LTPOSwitchState::LTPO_SWITCH_NOT_INIT;

  bool isTouchDown_ = false;

  bool isCfgFileInit_ = false;

  double curAnimationTranslateVelocity_ = 0.0;

  map<string, OH_NativeVSync*> nativeVsyncMap_;

  unique_ptr<OHOSAssetProvider> asset_provider_;

  vector<map<string, int>> framesSet;

  int animationVotingVsyncTimes_ = 0;

  // pointing to the lib of OH_NativeVSync_SetExpectedFrameRateRange
  void* libHandle_;

  // call the OH_NativeVSync_SetExpectedFrameRateRange function
  SetExpectedFrameRateRangeFunc_ setExpectedFrameRateRangeFunc_ = nullptr;
};  // class OhosVsyncVotingMgr

}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_OHOS_VSYNC_VOTING_MGR_H_
