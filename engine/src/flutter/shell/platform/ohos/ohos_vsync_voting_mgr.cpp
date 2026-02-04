/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */
#include "ohos_vsync_voting_mgr.h"

#include <dlfcn.h>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>

#include "flutter/fml/logging.h"
#include "flutter/fml/trace_event.h"
#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"

namespace flutter {

static std::shared_ptr<OhosVsyncVotingMgr> instance = nullptr;
static std::once_flag instance_flag;

// default is 160 DPI
static constexpr int32_t PHYSICAL_PIXEL_DENSITY = 160;
// 1 inch equals 25.4 millimeters
static constexpr double INCH_2_MILL = 25.4;

// After the touch up event, the frame rate drops to 60 after some time.
static constexpr int32_t TOUCH_UP_MILLIS_TIME_OUT_FPS_60 = 3000;
// After the touch up event, the frame rate remains at 120 for a period of time.
static constexpr int32_t TOUCH_UP_MILLIS_TIME_OUT_FPS_120 = 100;

// When switching from a high frame rate to a low frame rate,
// a certain number of frames need to be waited.
static constexpr int32_t HIGH_FPS_MAINTAINED_TIMES = 4;

static constexpr int32_t FPS_120 = 120;
static constexpr int32_t FPS_90 = 90;
static constexpr int32_t FPS_72 = 72;
static constexpr int32_t FPS_60 = 60;
static constexpr int32_t FPS_30 = 30;
static constexpr int32_t FPS_NO_VOTING = 0;

static constexpr int32_t DEFAULT_FPS = FPS_120;

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

static unordered_map<VVMVotingType, std::string> voting_type_unordered_map = {
  {VVMVotingType::VOTING_TYPE_NOTHING, std::string("NOTHING")},
  {VVMVotingType::VOTING_TYPE_TOUCH_DOWN_FPS_120, std::string("TOUCH_DOWN_FPS_120")},
  {VVMVotingType::VOTING_TYPE_TOUCH_UP_FPS_60, std::string("TOUCH_UP_FPS_60")},
  {VVMVotingType::VOTING_TYPE_TOUCH_UP_FPS_120, std::string("TOUCH_UP_FPS_120")},
  {VVMVotingType::VOTING_TYPE_COMMON_PLATFORMVIEW_FPS_120, std::string("COMMON_PLATFORMVIEW_FPS_120")},
  {VVMVotingType::VOTING_TYPE_ANIMATION, std::string("ANIMATION")},
};

std::shared_ptr<OhosVsyncVotingMgr> OhosVsyncVotingMgr::GetInstance() {
  std::call_once(instance_flag, [&] {
    instance = std::make_shared<OhosVsyncVotingMgr>();
  });

  return instance;
}

OhosVsyncVotingMgr::OhosVsyncVotingMgr()
    : asset_provider_(nullptr), lib_native_vsync_handle_(nullptr) {
  switch_status_ = LTPOSwitchState::LTPO_SWITCH_NOT_INIT;
  delay_drop_framerate_times_ = HIGH_FPS_MAINTAINED_TIMES;

  lib_native_vsync_handle_ = dlopen(LIB_NATIVE_VSYNC_NAME, RTLD_LAZY | RTLD_LOCAL);
  if (lib_native_vsync_handle_ == nullptr) {
    FML_LOG(ERROR) << "Failed to dlopen libnative_vsync.so";
    return;
  }

  func_SetExpectedFrameRateRange_symbol_handle_ =
      reinterpret_cast<SetExpectedFrameRateRangeFunc_>(
          dlsym(lib_native_vsync_handle_, FUN_SET_FREAM_RATE_NAME));
  if (func_SetExpectedFrameRateRange_symbol_handle_ == nullptr) {
    dlclose(lib_native_vsync_handle_);
    lib_native_vsync_handle_ = nullptr;
    FML_LOG(ERROR)
        << "Failed to dlsym OH_NativeVSync_SetExpectedFrameRateRange";
  }
}

OhosVsyncVotingMgr::~OhosVsyncVotingMgr() {
  if (lib_native_vsync_handle_ != nullptr) {
    dlclose(lib_native_vsync_handle_);
  }
}

void OhosVsyncVotingMgr::VoteAnimationValue(AnimationType AN_type,
                                            double device_pixel_ratio,
                                            double velocity) {
  if (lib_native_vsync_handle_ == nullptr || switch_status_ != LTPOSwitchState::LTPO_SWITCH_ON) {
    return;
  }

  double velocity_tmp = std::abs(velocity);
  if (device_pixel_ratio != 0.0) {
    // V(millimeter) = V(pixel) * 25.4 / (device_pixel_ratio * 160)
    velocity_tmp = velocity_tmp / (device_pixel_ratio * PHYSICAL_PIXEL_DENSITY);
    velocity_tmp = velocity_tmp * INCH_2_MILL;
  }

  switch (AN_type) {
    case AnimationType::AN_TYPE_TRANSLATE:
      VoteANTranslate(velocity_tmp);
      break;
    case AnimationType::AN_TYPE_SCALE:
    case AnimationType::AN_TYPE_ROTATION:
    default:
      break;
  }
  return;
}

void OhosVsyncVotingMgr::VoteTouchValue(VVMTouchType type, int64_t timestamp) {
  if (lib_native_vsync_handle_ == nullptr || switch_status_ != LTPOSwitchState::LTPO_SWITCH_ON) {
    return;
  }

  switch (type) {
    case VVMTouchType::TOUCH_TYPE_DOWN:
      is_touch_down_ = true;
      touch_voting_.store(FPS_120);
      VotingBySelf();
      break;
    case VVMTouchType::TOUCH_TYPE_UP:
      is_touch_down_ = false;
      touch_voting_.store(FPS_120);
      touch_timestamp_.store(timestamp);
      VotingBySelf();
      break;
    case VVMTouchType::TOUCH_TYPE_UP_3_SEC_AFTER:
      // During the continuous swiping process, maintain a stable frame rate(FPS).
      if (is_touch_down_) {
        break;
      }
      if (timestamp - touch_timestamp_.load() >= TOUCH_UP_MILLIS_TIME_OUT_FPS_60) {
        touch_voting_.store(FPS_NO_VOTING);
        VotingBySelf();
      }
      break;
    default:
      break;
  }
  return;
}

void OhosVsyncVotingMgr::VoteVideoValue(int second, int frame_count) {
  if (lib_native_vsync_handle_ == nullptr || switch_status_ != LTPOSwitchState::LTPO_SWITCH_ON) {
    return;
  }

  if (second <= 0 || frame_count <= 0) {
    return;
  }

  int framerate = frame_count / second;
  if (framerate <= FPS_30) {
    video_voting_.store(FPS_30);
  } else {
    video_voting_.store(FPS_60);
  }
  return;
}

void OhosVsyncVotingMgr::VoteANTranslate(double velocity) {
  // Determine the maximum velocity withthin one vsync. 
  if (velocity < cur_animation_translate_velocity_) {
    return;
  }

  cur_animation_translate_velocity_ = velocity;

  int expected_framerate_tmp = DEFAULT_FPS;
  for (std::vector<std::map<string, int>>::iterator it = frames_config_vec_.begin();
       it != frames_config_vec_.end(); it++) {
    if (velocity > static_cast<double>((*it)["min"])) {
      expected_framerate_tmp = (*it)["preferred_fps"];
      break;
    }
  }

  animation_voting_.store(expected_framerate_tmp);
}

void OhosVsyncVotingMgr::AttachNativeVsync(string handle_name,
                                           OH_NativeVSync* handle) {
  if (lib_native_vsync_handle_ == nullptr) {
    FML_LOG(ERROR) << "libHandle is null, or ltpo is not enabled";
    return;
  }

  if (handle == nullptr) {
    FML_LOG(ERROR) << "handle is null";
    return;
  }

  {
    std::lock_guard<std::mutex> lock(native_vsync_map_mutex_);
    native_vsync_map_.insert(std::pair(handle_name, handle));
  }
  return;
}

void OhosVsyncVotingMgr::DetachNativeVsync(string handle_name) {
  std::lock_guard<std::mutex> lock(native_vsync_map_mutex_);
  native_vsync_map_.erase(handle_name);
  return;
}

int OhosVsyncVotingMgr::VoteFinalFrameRateByPriority() {
  int final_framerate = FPS_NO_VOTING;
  VVMVotingType voting_type = VVMVotingType::VOTING_TYPE_NOTHING;
  do {
    if (is_touch_down_) {
      final_framerate = FPS_120;
      voting_type = VVMVotingType::VOTING_TYPE_TOUCH_DOWN_FPS_120;
      break;
    }

    // When there is no touch event on the screen for 3 seconds,
    // the frame rate changes to 60.
    if (touch_voting_.load() == FPS_NO_VOTING) {
      final_framerate = FPS_60;
      voting_type = VVMVotingType::VOTING_TYPE_TOUCH_UP_FPS_60;
      break;
    }

    int64_t now_timestamp = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
    if (now_timestamp - touch_timestamp_.load() < TOUCH_UP_MILLIS_TIME_OUT_FPS_120) {
      final_framerate = FPS_120;
      voting_type = VVMVotingType::VOTING_TYPE_TOUCH_UP_FPS_120;
      break;
    }

    // When PlatformView exists, either do not vote on the frame rate
    // or only vote for a frame rate of 120.
    // The flag is_platformview_exist_ should be reset by NativeVsync.
    if (is_platformview_exist_.load()) {
      is_platformview_exist_.store(false);
      final_framerate = FPS_120;
      voting_type = VVMVotingType::VOTING_TYPE_COMMON_PLATFORMVIEW_FPS_120;
      break;
    }

    int animation_voting = animation_voting_.load();
    if (animation_voting != FPS_NO_VOTING) {
      final_framerate = animation_voting;
      voting_type = VVMVotingType::VOTING_TYPE_ANIMATION;
      break;
    }
  } while (0);

  TRACE_EVENT2("flutter", "VoteFinalFrameRateByPriority",
    "FPS", std::to_string(final_framerate).c_str(),
    "type", voting_type_unordered_map[voting_type].c_str());
  return final_framerate;
}

int OhosVsyncVotingMgr::DelayFrameRateDropForStability(
    int next_framerate,
    VVMVotingFrameRateRole type) {
  std::ostringstream oss;
  oss << "DelayFrameRateDropForStability " << local_framerate_ << "->" << next_framerate;
  std::string resultStr = oss.str();
  TRACE_EVENT0("flutter", resultStr.c_str());

  if (next_framerate >= local_framerate_) {
    delay_drop_framerate_times_ = HIGH_FPS_MAINTAINED_TIMES;
    expected_drop_framerate_ = FPS_NO_VOTING;
    return next_framerate;
  }

  TRACE_EVENT1("flutter", "DelayFrameRateDropForStability",
    "remain", std::to_string(delay_drop_framerate_times_).c_str());

  // The high frame rate transitions to low frame rate
  // using a slowly decreasing method.
  // HIGH_FPS_MAINTAINED_TIMES represents the number of vsync during
  if (type == VVMVotingFrameRateRole::ROLE_EX_MODULE) {
    delay_drop_framerate_times_--;
    if (next_framerate > expected_drop_framerate_) {
      expected_drop_framerate_ = next_framerate;
    }
  }

  if (delay_drop_framerate_times_ <= 0) {
    delay_drop_framerate_times_ = HIGH_FPS_MAINTAINED_TIMES;
    return expected_drop_framerate_;
  }

  return local_framerate_;
}

int OhosVsyncVotingMgr::VotingExpectedRateRange(
    int result_frameRate,
    OH_NativeVSync_ExpectedRateRange* range) {
  if ((result_frameRate == local_framerate_) &&
      (result_frameRate == FPS_NO_VOTING || result_frameRate == PlatformViewOHOSNapi::display_refresh_rate)) {
    // Vote results match the local frame rate or the display refresh rate.
    return RET_FAILED;
  }
  local_framerate_ = result_frameRate;

  if (result_frameRate != FPS_NO_VOTING) {
    range->min = FPS_30;
    range->max = FPS_120;
  } else {
    // If range is {0, 0, 0}
    // represents native_vsync without managing framerate.
    // Current framerate is managed by the system.
    range->min = 0;
    range->max = 0;
  }
  range->expected = result_frameRate;

  return RET_SUCCEED;
}

void OhosVsyncVotingMgr::VotingByNativeVsync(OH_NativeVSync* handle) {
  if (lib_native_vsync_handle_ == nullptr || switch_status_ != LTPOSwitchState::LTPO_SWITCH_ON ||
      func_SetExpectedFrameRateRange_symbol_handle_ == nullptr) {
    return;
  }

  if (handle == nullptr) {
    return;
  }

  int resultFrameRate = VoteFinalFrameRateByPriority();
  resultFrameRate = DelayFrameRateDropForStability(resultFrameRate);

  // Reset some states
  cur_animation_translate_velocity_ = 0.0;

  OH_NativeVSync_ExpectedRateRange range = {0, 0, 0};
  int ret = VotingExpectedRateRange(resultFrameRate, &range);
  if (ret != RET_SUCCEED) {
    return;
  }

  std::ostringstream oss;
  oss << "{" << range.min << "," << range.max << "," << range.expected << "}";
  std::string rangeStr = oss.str();
  FML_DLOG(INFO) << "SetExpectedFrameRateRange : " << rangeStr.c_str();
  TRACE_EVENT1("flutter", "SetExpectedFrameRateRange",
    "range", rangeStr.c_str());

  ret = func_SetExpectedFrameRateRange_symbol_handle_(handle, &range);
  if (ret != 0) {
    FML_LOG(ERROR) << "SetExpectedFrameRateRange failed, ret = " << ret;
  }

  return;
}

void OhosVsyncVotingMgr::VotingBySelf() {
  if (lib_native_vsync_handle_ == nullptr || switch_status_ != LTPOSwitchState::LTPO_SWITCH_ON ||
      func_SetExpectedFrameRateRange_symbol_handle_ == nullptr) {
    return;
  }

  int result_framerate = VoteFinalFrameRateByPriority();
  result_framerate = DelayFrameRateDropForStability(result_framerate, VVMVotingFrameRateRole::ROLE_SELF);

  // Reset some states
  cur_animation_translate_velocity_ = 0.0;
  if (touch_voting_.load() == FPS_NO_VOTING) {
    // After the voting event triggered by a touch event ends,
    // the voting event for the animation event should be canceled.
    animation_voting_.store(FPS_NO_VOTING);
  }

  OH_NativeVSync_ExpectedRateRange range = {0, 0, 0};
  int ret = VotingExpectedRateRange(result_framerate, &range);
  if (ret != RET_SUCCEED) {
    return;
  }

  std::ostringstream oss;
  oss << "{" << range.min << "," << range.max << "," << range.expected << "}";
  std::string range_str = oss.str();
  FML_LOG(INFO) << "BySelf SetExpectedFrameRateRange : " << range_str.c_str();
  TRACE_EVENT1("flutter", "BySelf SetExpectedFrameRateRange", "range",
               range_str.c_str());

  // Safe copy of the current handle list
  std::vector<OH_NativeVSync*> native_vsync_handle_vec;
  {
    std::lock_guard<std::mutex> lock(native_vsync_map_mutex_);
    if (native_vsync_map_.empty()) {
      return;
    }
    native_vsync_handle_vec.reserve(native_vsync_map_.size());
    for (auto it = native_vsync_map_.begin(); it != native_vsync_map_.end(); ++it) {
      native_vsync_handle_vec.push_back(it->second);
    }
  }

  for (auto* handle : native_vsync_handle_vec) {
    if (handle == nullptr) {
      continue;
    }
    ret = func_SetExpectedFrameRateRange_symbol_handle_(handle, &range);
    if (ret != 0) {
      FML_LOG(ERROR) << "BySelf SetExpectedFrameRateRange failed, ret = " << ret;
    }
  }

  return;
}

void OhosVsyncVotingMgr::ParseTranslate(const Json::Value& arr) {
  if (arr.empty()) {
    FML_LOG(ERROR) << "The array is empty";
    return;
  }

  const char* tags[] = {"serial_number", "min", "max", "preferred_fps"};
  size_t num = sizeof(tags) / sizeof(char*);

  int number = 1;
  size_t size = arr.size();
  for (unsigned int i = 0; i < size; i++) {
    if (!arr[i].isObject()) {
      FML_LOG(ERROR) << "config item is not object at index = " << i;
      continue;
    }

    std::map<string, int> map_tmp;
    bool valid = true;
    for (unsigned int j = 0; j < num; j++) {
      const char* key = tags[j];
      if (!arr[i].isMember(key)) {
        FML_LOG(ERROR) << "config tag missed, key = " << key;
        valid = false;
        break;
      }
      if (!arr[i][key].isInt()) {
        FML_LOG(ERROR) << "config value invalid, key = " << key;
        valid = false;
        break;
      }
      int value_tmp = arr[i][key].asInt();
      if ((j == 0) && (value_tmp != number)) {
        FML_LOG(ERROR) << "config value serial_number is wrong";
      }
      map_tmp.insert(std::pair<string, int>(string(key), value_tmp));
    }
    if (valid) {
      frames_config_vec_.push_back(map_tmp);
      number++;
    }
  }
}

void OhosVsyncVotingMgr::ParseFramesCfg() {
  if (lib_native_vsync_handle_ == nullptr) {
    FML_LOG(ERROR) << "libHandle is null";
    switch_status_ = LTPOSwitchState::LTPO_SWITCH_OFF;
    return;
  }

  if (asset_provider_ == nullptr) {
    FML_LOG(ERROR) << "asset_provider is null";
    switch_status_ = LTPOSwitchState::LTPO_SWITCH_OFF;
    return;
  }

  if (is_frames_config_file_init_) {
    FML_LOG(ERROR) << "framesconfig file has been initiallized";
    // switch_status_ has already been set once; there’s no need to set it again.
    return;
  }

  is_frames_config_file_init_ = true;

  if (ParseFramesCfgImpl() != RET_SUCCEED) {
    FML_LOG(ERROR) << "Failed to parse file frameconfig";
    switch_status_ = LTPOSwitchState::LTPO_SWITCH_OFF;
  }

  return;
}

int OhosVsyncVotingMgr::ParseFramesCfgImpl() {
  std::unique_ptr<fml::Mapping> frames_config_mapping =
      asset_provider_->GetAsMapping(std::string(FRAMES_CFG_JSON));
  if (frames_config_mapping == nullptr) {
    FML_LOG(ERROR) << "Failed to GetAsMapping";
    return RET_FAILED;
  }

  const char* data =
      reinterpret_cast<const char*>(frames_config_mapping->GetMapping());
  if (data == nullptr) {
    FML_LOG(ERROR) << "Failed to GetBuffer";
    return RET_FAILED;
  }

  int size = static_cast<int>(frames_config_mapping->GetSize());

  Json::Value root;
  Json::CharReaderBuilder char_reader_builder;
  std::string errs;
  std::unique_ptr<Json::CharReader> json_reader(
      char_reader_builder.newCharReader());
  bool is_json = json_reader->parse(data, data + size, &root, &errs);
  if (!is_json || !errs.empty()) {
    FML_LOG(ERROR) << "Failed to parse frameconfig.json, err = " << errs;
    return RET_FAILED;
  }

  uint32_t switch_status = 0;
  if (root.isMember(SWITCH_KEY)) {
    if (root[SWITCH_KEY].isNumeric()) {
      switch_status = root[SWITCH_KEY].asUInt();
      // for DFX
      FML_LOG(WARNING) << "vsync_voting_mgr switch_status = " << switch_status;
    } else {
      FML_LOG(ERROR) << "Failed to parse key of SWITCH";
      return RET_FAILED;
    }
  }

  if (switch_status != static_cast<uint32_t>(LTPOSwitchState::LTPO_SWITCH_ON)) {
    FML_LOG(WARNING) << "ltpo is not enabled";
    return RET_FAILED;
  }

  if (root.isMember(TRANSLATE_KEY)) {
    ParseTranslate(root[TRANSLATE_KEY]);
  } else {
    FML_LOG(ERROR) << "Failed to parse key of TRANSLATE";
    return RET_FAILED;
  }

  switch_status_ = LTPOSwitchState::LTPO_SWITCH_ON;
  return RET_SUCCEED;
}

void OhosVsyncVotingMgr::SetAssetProvider(
    std::unique_ptr<OHOSAssetProvider> hap_asset_provider) {
  if (lib_native_vsync_handle_ == nullptr) {
    return;
  }

  if (hap_asset_provider == nullptr) {
    FML_LOG(ERROR) << "hap_asset_provider is null";
    return;
  }

  if (asset_provider_ != nullptr) {
    FML_LOG(WARNING) << "asset_provider already init";
    return;
  }

  asset_provider_ = std::move(hap_asset_provider);
  return;
}

void OhosVsyncVotingMgr::SetPlatformViewExist(bool is_exist) {
  if (is_platformview_exist_.load() != is_exist) {
    is_platformview_exist_.store(is_exist);
  }
  return;
}

LTPOSwitchState OhosVsyncVotingMgr::CheckVotingSwitchState() {
  // for DFX
  if (switch_status_ == LTPOSwitchState::LTPO_SWITCH_ON) {
    FML_LOG(WARNING) << "VotingSwitchState is on.";
  }
  return switch_status_;
}
}  // namespace flutter
