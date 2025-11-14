/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */
#include "ohos_vsync_voting_mgr.h"

#include <dlfcn.h>
#include <algorithm>

#include "flutter/fml/logging.h"
#include "flutter/fml/trace_event.h"
#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"

namespace flutter {

thread_local int64_t touchTimestamp = 0;

static std::shared_ptr<OhosVsyncVotingMgr> instance = nullptr;
static std::once_flag instanceFlag;

// 标准DPI = 160
static constexpr int32_t PHYSICAL_PIXEL_DENSITY = 160;
// 1 inch equals 25.4 millimeters
static constexpr double INCH_2_MILL = 25.4;

static constexpr int32_t FPS_120 = 120;
static constexpr int32_t FPS_90 = 90;
static constexpr int32_t FPS_72 = 72;
static constexpr int32_t FPS_60 = 60;
static constexpr int32_t FPS_30 = 30;

static constexpr int32_t RET_FAILED = -1;
static constexpr int32_t RET_SUCCEED = 0;

constexpr char LIB_NATIVE_VSYNC_NAME[] = "libnative_vsync.so";
constexpr char FUN_SET_FREAM_RATE_NAME[] =
    "OH_NativeVSync_SetExpectedFrameRateRange";

constexpr char FRAMES_CFG_JSON[] = "framesconfig.json";
constexpr char SWITCH_KEY[] = "SWITCH";
constexpr char TRANSLATE_KEY[] = "TRANSLATE";
constexpr char SCALE_KEY[] = "SCALE";
constexpr char ROTATION_KEY[] = "ROTATION";

static constexpr fml::TimeDelta TOUCH_3_SEC = fml::TimeDelta::FromSeconds(3);

std::shared_ptr<OhosVsyncVotingMgr> OhosVsyncVotingMgr::GetInstance(void) {
  std::call_once(instanceFlag, [&] {
    instance = std::make_shared<OhosVsyncVotingMgr>();
  });

  return instance;
}

OhosVsyncVotingMgr::OhosVsyncVotingMgr()
    : asset_provider_(nullptr), libHandle_(nullptr) {
  switchStatus_ = LTPOSwitchState::LTPO_SWITCH_NOT_INIT;
  libHandle_ = dlopen(LIB_NATIVE_VSYNC_NAME, RTLD_LAZY | RTLD_LOCAL);
  if (libHandle_ == nullptr) {
    FML_LOG(ERROR) << "Failed to dlopen libnative_vsync.so";
    return;
  }

  setExpectedFrameRateRangeFunc_ =
      reinterpret_cast<SetExpectedFrameRateRangeFunc_>(
          dlsym(libHandle_, FUN_SET_FREAM_RATE_NAME));
  if (setExpectedFrameRateRangeFunc_ == nullptr) {
    dlclose(libHandle_);
    libHandle_ = nullptr;
    FML_LOG(ERROR)
        << "Failed to dlsym OH_NativeVSync_SetExpectedFrameRateRange";
  }
}

OhosVsyncVotingMgr::~OhosVsyncVotingMgr() {
  if (libHandle_ != nullptr) {
    dlclose(libHandle_);
  }
}

void OhosVsyncVotingMgr::VoteAnimationValue(AnimationType ANType,
                                            double devicePixelRatio,
                                            double velocity) {
  // 接口不存在或ltpo未使能
  if (libHandle_ == nullptr || switchStatus_ != LTPOSwitchState::LTPO_SWITCH_ON) {
    return;
  }

  double velocityTmp = abs(velocity);
  if (devicePixelRatio != 0.0) {
    // V(millimeter) = V(pixel) * 25.4 / (devicePixelRatio * 160)
    velocityTmp = velocityTmp / (devicePixelRatio * PHYSICAL_PIXEL_DENSITY);
    velocityTmp = velocityTmp * INCH_2_MILL;
  }

  switch (ANType) {
    case AnimationType::AN_TYPE_TRANSLATE:
      VoteANTranslate(velocityTmp);
      break;
    case AnimationType::AN_TYPE_SCALE:
    case AnimationType::AN_TYPE_ROTATION:
    default:
      break;
  }
  return;
}

void OhosVsyncVotingMgr::VoteTouchValue(VVMTouchType type, int64_t timestamp) {
  // 接口不存在或ltpo未使能
  if (libHandle_ == nullptr || switchStatus_ != LTPOSwitchState::LTPO_SWITCH_ON) {
    return;
  }

  const int TouchTimeOut_ = 3000;

  switch (type) {
    case VVMTouchType::TOUCH_TYPE_DOWN:
      isTouchDown_ = true;
      touchVoting_.store(FPS_120);
      VotingBySelf();
      break;
    case VVMTouchType::TOUCH_TYPE_UP:
      isTouchDown_ = false;
      touchVoting_.store(FPS_120);
      touchTimestamp = timestamp;
      VotingBySelf();
      break;
    case VVMTouchType::TOUCH_TYPE_UP_3_SEC_AFTER:
      // 避免连续抛滑出现touch失效
      if (isTouchDown_) {
        break;
      }
      // 取最后一次手指抬起后的时间
      if (timestamp - touchTimestamp >= TouchTimeOut_) {
        touchVoting_.store(0);
        VotingBySelf();
      }
      break;
    default:
      break;
  }
  return;
}

void OhosVsyncVotingMgr::VoteVideoValue(int second, int frameCount) {
  // 接口不存在或ltpo未使能
  if (libHandle_ == nullptr || switchStatus_ != LTPOSwitchState::LTPO_SWITCH_ON) {
    return;
  }

  if (second <= 0 || frameCount <= 0 || second == 0) {
    return;
  }

  int frameRate = frameCount / second;
  if (frameRate <= FPS_30) {
    videoVoting_.store(FPS_30);
  } else {
    videoVoting_.store(FPS_60);
  }
  return;
}

void OhosVsyncVotingMgr::VoteANTranslate(double velocity) {
  // 一个vsync周期内取动画速率最大值进行帧率投票
  if (velocity < curAnimationTranslateVelocity_) {
    return;
  }

  curAnimationTranslateVelocity_ = velocity;

  // 认为帧率60是静置的，当速率为0或其他速率范围时，默认60刷新率
  int expectedRateTmp = FPS_60;
  size_t framesSetSize = framesSet.size();
  for (std::vector<std::map<string, int>>::iterator it = framesSet.begin();
       it != framesSet.end(); it++) {
    if (velocity > static_cast<double>((*it)["min"])) {
      expectedRateTmp = (*it)["preferred_fps"];
      break;
    }
  }

  if (expectedRateTmp == PlatformViewOHOSNapi::display_refresh_rate) {
    return;
  }

  animationVoting_.store(expectedRateTmp);
}

void OhosVsyncVotingMgr::AttachNativeVsync(string handleName,
                                           OH_NativeVSync* handle) {
  // 接口不存在
  if (libHandle_ == nullptr) {
    FML_LOG(ERROR) << "libHandle is null, or ltpo is not enabled";
    return;
  }

  if (handle == nullptr) {
    FML_LOG(ERROR) << "handle is null";
    return;
  }

  nativeVsyncMap_.insert(std::pair(handleName, handle));
  return;
}

void OhosVsyncVotingMgr::DettachNativeVsync(string handleName) {
  nativeVsyncMap_.erase(handleName);
  return;
}

void OhosVsyncVotingMgr::VotingByNativeVsync(OH_NativeVSync* handle) {
  // 接口不存在或ltpo未使能
  if (libHandle_ == nullptr || switchStatus_ != LTPOSwitchState::LTPO_SWITCH_ON ||
      setExpectedFrameRateRangeFunc_ == nullptr) {
    return;
  }

  if (handle == nullptr) {
    return;
  }

  int resultFrameRate = 0;

  if (animationVoting_.load() != 0 && !isTouchDown_) {
    resultFrameRate = animationVoting_.load();
  } else if (touchVoting_.load() != 0) {
    resultFrameRate = touchVoting_.load();
  } else if (videoVoting_.load() != 0) {
    resultFrameRate = videoVoting_.load();
  }

  // 清空缓存
  curAnimationTranslateVelocity_ = 0.0;

  // ltpo投票的情况下，判断platformview是否存在，需要在此清除isPlatformViewExist_存在的标识
  if (isPlatformViewExist_.load()) {
    // 存在则强制拉高到120帧率
    if (resultFrameRate != 0) {
      resultFrameRate = FPS_120;
    }
    // 只在每个vsync信号期间，去清除isPlatformViewExist_标识
    isPlatformViewExist_.store(false);
  }

  if (resultFrameRate == 0 && resultFrameRate == localFrameRate_) {
    return;
  }

  if (resultFrameRate == PlatformViewOHOSNapi::display_refresh_rate &&
      resultFrameRate == localFrameRate_) {
    return;
  }

  localFrameRate_ = resultFrameRate;

  int min = 0;
  int max = 0;
  if (resultFrameRate != 0) {
    min = FPS_30;
    max = FPS_120;
  }

  std::ostringstream oss;
  oss << "{" << min << "," << max << "," << resultFrameRate << "}";
  std::string rangeStr = oss.str();
  FML_DLOG(INFO) << "SetExpectedFrameRateRange : " << rangeStr.c_str();
  TRACE_EVENT1("flutter", "SetExpectedFrameRateRange", "range",
               rangeStr.c_str());

  OH_NativeVSync_ExpectedRateRange range = {min, max, resultFrameRate};

  int ret = setExpectedFrameRateRangeFunc_(handle, &range);
  if (ret != 0) {
    FML_LOG(ERROR) << "SetExpectedFrameRateRange failed, ret = " << ret;
  }

  return;
}

void OhosVsyncVotingMgr::VotingBySelf() {
  // 接口不存在或ltpo未使能
  if (libHandle_ == nullptr || switchStatus_ != LTPOSwitchState::LTPO_SWITCH_ON ||
      setExpectedFrameRateRangeFunc_ == nullptr) {
    return;
  }

  if (nativeVsyncMap_.size() == 0) {
    return;
  }

  int resultFrameRate = 0;

  if (animationVoting_.load() != 0 && !isTouchDown_) {
    resultFrameRate = animationVoting_.load();
  } else if (touchVoting_.load() != 0) {
    resultFrameRate = touchVoting_.load();
  } else if (videoVoting_.load() != 0) {
    resultFrameRate = videoVoting_.load();
  }

  // 清空缓存
  curAnimationTranslateVelocity_ = 0.0;
  if (touchVoting_.load() == 0) {
    // 触摸事件触发时，需要判断是否需要清空动画的帧率投票
    animationVoting_.store(0);
  }

  // ltpo投票的情况下，判断platformview是否存在
  if (resultFrameRate != 0 && isPlatformViewExist_.load()) {
    // 存在则强制拉高到120帧率
    resultFrameRate = 120;
  }

  if (resultFrameRate == 0 && resultFrameRate == localFrameRate_) {
    return;
  }

  if (resultFrameRate == PlatformViewOHOSNapi::display_refresh_rate &&
      resultFrameRate == localFrameRate_) {
    return;
  }

  localFrameRate_ = resultFrameRate;

  int min = 0;
  int max = 0;
  if (resultFrameRate != 0) {
    min = FPS_30;
    max = FPS_120;
  }

  std::ostringstream oss;
  oss << "{" << min << "," << max << "," << resultFrameRate << "}";
  std::string rangeStr = oss.str();
  FML_LOG(INFO) << "BySelf SetExpectedFrameRateRange : " << rangeStr.c_str();
  TRACE_EVENT1("flutter", "BySelf SetExpectedFrameRateRange", "range",
               rangeStr.c_str());

  OH_NativeVSync_ExpectedRateRange range = {min, max, resultFrameRate};

  int ret = 0;
  for (auto it = nativeVsyncMap_.begin(); it != nativeVsyncMap_.end(); it++) {
    OH_NativeVSync* handleTmp = it->second;
    ret = setExpectedFrameRateRangeFunc_(handleTmp, &range);
    if (ret != 0) {
      FML_LOG(ERROR) << "BySelf SetExpectedFrameRateRange failed, ret = "
                     << ret;
    }
  }
  return;
}

void OhosVsyncVotingMgr::ParseTranslate(const Json::Value& arr) {
  if (arr.empty()) {
    FML_LOG(ERROR) << "The array is empty";
  }

  const char* tags[] = {"serial_number", "min", "max", "preferred_fps"};
  size_t num = sizeof(tags) / sizeof(char*);

  int number = 1;
  size_t size = arr.size();
  for (unsigned int i = 0; i < size; i++) {
    std::map<string, int> mapTmp;
    if (arr[i].isObject()) {
      for (unsigned int j = 0; j < num; j++) {
        const char* key = tags[j];
        if (!arr[i].isMember(key)) {
          FML_LOG(ERROR) << "config tag missed, key = " << key;
          return;
        } else if (!arr[i][key].isInt()) {
          FML_LOG(ERROR) << "config value invalid, key = " << key;
          return;
        } else {
          int valueTmp = arr[i][key].asInt();
          if ((j == 0) && (valueTmp != number)) {
            FML_LOG(ERROR) << "config value serial_number is wrong";
          }
          mapTmp.insert(std::pair<string, int>(string(key), valueTmp));
        }
      }
    }

    framesSet.push_back(mapTmp);
    number++;
  }
}

void OhosVsyncVotingMgr::ParseFramesCfg(void) {
  // 接口不存在
  if (libHandle_ == nullptr) {
    FML_LOG(ERROR) << "libHandle is null";
    switchStatus_ = LTPOSwitchState::LTPO_SWITCH_OFF;
    return;
  }

  if (asset_provider_ == nullptr) {
    FML_LOG(ERROR) << "asset_provider is null";
    switchStatus_ = LTPOSwitchState::LTPO_SWITCH_OFF;
    return;
  }

  if (isCfgFileInit_) {
    FML_LOG(ERROR) << "framesconfig file has been initiallized";
    switchStatus_ = LTPOSwitchState::LTPO_SWITCH_OFF;
    return;
  }

  isCfgFileInit_ = true;

  if (ParseFramesCfgImpl() != RET_SUCCEED) {
    FML_LOG(ERROR) << "Failed to parse file frameconfig";
    switchStatus_ = LTPOSwitchState::LTPO_SWITCH_OFF;
  }

  return;
}

int OhosVsyncVotingMgr::ParseFramesCfgImpl(void) {
  std::unique_ptr<fml::Mapping> framesCfgMapping =
      asset_provider_->GetAsMapping(std::string(FRAMES_CFG_JSON));
  if (framesCfgMapping == nullptr) {
    FML_LOG(ERROR) << "Failed to GetAsMapping";
    return RET_FAILED;
  }

  const char* data =
      reinterpret_cast<const char*>(framesCfgMapping->GetMapping());
  if (data == nullptr) {
    FML_LOG(ERROR) << "Failed to GetBuffer";
    return RET_FAILED;
  }

  int size = static_cast<int>(framesCfgMapping->GetSize());

  Json::Value root;
  Json::CharReaderBuilder charReaderBuilder;
  std::string errs;
  std::unique_ptr<Json::CharReader> jsonReader(
      charReaderBuilder.newCharReader());
  bool isJson = jsonReader->parse(data, data + size, &root, &errs);
  if (!isJson || !errs.empty()) {
    FML_LOG(ERROR) << "Failed to parse frameconfig.json, err = " << errs;
    return RET_FAILED;
  }

  uint32_t switchValue = 0;
  if (root.isMember(SWITCH_KEY)) {
    if (root[SWITCH_KEY].isNumeric()) {
      switchValue = root[SWITCH_KEY].asUInt();
      FML_LOG(INFO) << "vsync_voting_mgr switchValue = " << switchValue;
    } else {
      FML_LOG(ERROR) << "Failed to parse key of SWITCH";
      return RET_FAILED;
    }
  }

  if (switchValue != static_cast<uint32_t>(LTPOSwitchState::LTPO_SWITCH_ON)) {
    FML_LOG(WARNING) << "ltpo is not enabled";
    return RET_FAILED;
  }

  if (root.isMember(TRANSLATE_KEY)) {
    ParseTranslate(root[TRANSLATE_KEY]);
  } else {
    FML_LOG(ERROR) << "Failed to parse key of TRANSLATE";
    return RET_FAILED;
  }

  switchStatus_ = LTPOSwitchState::LTPO_SWITCH_ON;
  return RET_SUCCEED;
}

void OhosVsyncVotingMgr::SetAssetProvider(
    std::unique_ptr<OHOSAssetProvider> hap_asset_provider) {
  if (libHandle_ == nullptr) {
    return;
  }

  if (hap_asset_provider == nullptr) {
    FML_LOG(ERROR) << "hap_asset_provider is null";
    return;
  }

  if (asset_provider_ != nullptr) {
    FML_LOG(ERROR) << "asset_provider is not null";
    return;
  }

  asset_provider_ = std::move(hap_asset_provider);
  return;
}

void OhosVsyncVotingMgr::SetPlatformViewExist(bool isExist) {
  if (isPlatformViewExist_.load() != isExist) {
    isPlatformViewExist_.store(isExist);
  }
  return;
}

LTPOSwitchState OhosVsyncVotingMgr::CheckVotingSwitchState(void) {
  return switchStatus_;
}
}  // namespace flutter
