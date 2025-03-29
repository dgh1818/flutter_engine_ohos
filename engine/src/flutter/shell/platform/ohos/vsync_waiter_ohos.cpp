/*
 * Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "flutter/shell/platform/ohos/vsync_waiter_ohos.h"
#include <qos/qos.h>
#include <atomic>
#include <dlfcn.h>
#include "fml/trace_event.h"
#include "napi_common.h"
#include "ohos_logging.h"

namespace flutter {

static constexpr int32_t SUPPORT_API_VERSION = 14;
const char* flutterSyncName = "flutter_connect";
const char* NATIVE_DVSYNC_SO = "libnative_vsync.so";


thread_local bool VsyncWaiterOHOS::firstCall = true;

VsyncWaiterOHOS::VsyncWaiterOHOS(const flutter::TaskRunners& task_runners,
                                 std::shared_ptr<bool>& enable_frame_cache)
    : VsyncWaiter(task_runners), enable_frame_cache_(enable_frame_cache) {
  vsync_handle_ =
      OH_NativeVSync_Create("flutterSyncName", strlen(flutterSyncName));
}

VsyncWaiterOHOS::~VsyncWaiterOHOS() {
  OH_NativeVSync_Destroy(vsync_handle_);
  vsync_handle_ = nullptr;
}

void VsyncWaiterOHOS::UpdateDisplayRefreshRate(int64_t period) {
  if (period != 0) {
    int refresh_rate = 1000000000 / period;
    auto rates = std::atomic_load(&PlatformViewOHOSNapi::all_refresh_rates);
    if (!rates) {
      return;
    }
    auto big_it = rates->upper_bound(refresh_rate);
    auto small_it =
        (big_it == rates->begin()) ? rates->end() : std::prev(big_it);

    int big_rate = 1000000000;
    int small_rate = 1;
    if (big_it != rates->end()) {
      big_rate = *big_it;
    }
    if (small_it != rates->end()) {
      small_rate = *small_it;
    }
    if (refresh_rate >= (small_rate + big_rate) / 2) {
      refresh_rate = big_rate;
    } else {
      refresh_rate = small_rate;
    }

    if (PlatformViewOHOSNapi::display_refresh_rate != refresh_rate) {
      TRACE_EVENT0("flutter", "ChangeRefreshRate");
      FML_DLOG(INFO) << "refresh_rate change:"
                     << PlatformViewOHOSNapi::display_refresh_rate << "->"
                     << refresh_rate;
      PlatformViewOHOSNapi::display_refresh_rate = refresh_rate;
    }
  }
}

int64_t VsyncWaiterOHOS::GetVsyncPeriod() {
  long long period = 0;
  if (vsync_handle_) {
    OH_NativeVSync_GetPeriod(vsync_handle_, &period);
    UpdateDisplayRefreshRate(period);
  }
  return period;
}

void VsyncWaiterOHOS::AwaitVSync() {
  TRACE_EVENT0("flutter", "VsyncWaiterOHOS::AwaitVSync");
  if (vsync_handle_ == nullptr) {
    LOGE("AwaitVSync vsync_handle_ is nullptr");
    return;
  }
  auto* weak_this = new std::weak_ptr<VsyncWaiter>(shared_from_this());
  OH_NativeVSync* handle = vsync_handle_;

  fml::TaskRunner::RunNowOrPostTask(
      task_runners_.GetUITaskRunner(), [weak_this, handle]() {
        int32_t ret = 0;
        if (0 != (ret = OH_NativeVSync_RequestFrameWithMultiCallback(
                      handle, &OnVsyncFromOHOS, weak_this))) {
          FML_DLOG(ERROR) << "AwaitVSync...failed:" << ret;
        }
      });
}

void VsyncWaiterOHOS::OnVsyncFromOHOS(long long timestamp, void* data) {
  if (data == nullptr) {
    FML_LOG(ERROR) << "VsyncWaiterOHOS::OnVsyncFromOHOS, data is nullptr.";
    return;
  }
  if (VsyncWaiterOHOS::firstCall) {
    int ret = OH_QoS_SetThreadQoS(QoS_Level::QOS_USER_INTERACTIVE);
    FML_DLOG(INFO) << "qos set VsyncWaiterOHOS result:" << ret
                   << ",tid:" << gettid();
    VsyncWaiterOHOS::firstCall = false;
  }
  int64_t frame_nanos = static_cast<int64_t>(timestamp);
  auto frame_time = fml::TimePoint::FromEpochDelta(
      fml::TimeDelta::FromNanoseconds(frame_nanos));

  auto* weak_this = reinterpret_cast<std::weak_ptr<VsyncWaiter>*>(data);
  uint64_t vsync_period = 0;
  auto shared_this = weak_this->lock();
  if (shared_this) {
    auto ohos_vsync_waiter = static_cast<VsyncWaiterOHOS*>(shared_this.get());
    vsync_period = ohos_vsync_waiter->GetVsyncPeriod();
    // To avoid excessive response latency, frames will not be cached when the
    // refresh rate is 60 Hz.
    if (*ohos_vsync_waiter->enable_frame_cache_ && vsync_period < 15000000) {
      // When the frame cache is enabled, one frame will be cached, sacrificing
      // one frame of latency in exchange for smoothness.
      vsync_period += vsync_period - 1000000;
    }
  }

  // [-1ms] is to avoid this situation:
  // ui_timestamp(xxx8.334ms) > now_time(xxx8.332ms) => skip this frame
  // update [-2ms]: vsync may get a perid of 7.1 ms when 120hz.
  auto target_time = frame_time +
                     fml::TimeDelta::FromNanoseconds(vsync_period) -
                     fml::TimeDelta::FromMilliseconds(2);
  std::string trace_str =
      std::to_string(timestamp) + "-" + std::to_string(vsync_period) + "-" +
      std::to_string(target_time.ToEpochDelta().ToNanoseconds());
  TRACE_EVENT1("flutter", "OHOSVsync", "timestamp-period", trace_str.c_str());
  ConsumePendingCallback(weak_this, frame_time, target_time);
}

void VsyncWaiterOHOS::ConsumePendingCallback(
    std::weak_ptr<VsyncWaiter>* weak_this,
    fml::TimePoint frame_start_time,
    fml::TimePoint frame_target_time) {
  std::shared_ptr<VsyncWaiter> shared_this = weak_this->lock();
  delete weak_this;

  if (shared_this) {
    shared_this->FireCallback(frame_start_time, frame_target_time);
  }
}

void VsyncWaiterOHOS::SetDvsyncSwitch(bool enableDvsync) {
  if (apiVersion_ == 0) {
    apiVersion_ = OH_GetSdkApiVersion();
  }
  if (apiVersion_ < SUPPORT_API_VERSION) {
    LOGI("current api version not support native dvsync!");
    return;
  }
  if (!handle_) {
    handle_ = dlopen(NATIVE_DVSYNC_SO, RTLD_NOW);
  }
  if (!handle_) {
    LOGE("SetDvsyncSwitch load %{public}s failed!", NATIVE_DVSYNC_SO);
    return;
  }

  if (!nativeDvsyncFunc_) {
    nativeDvsyncFunc_ = reinterpret_cast<NativeDvsyncFunc>(dlsym(handle_, "OH_NativeVSync_DVSyncSwitch"));
  }
  if (!nativeDvsyncFunc_) {
    LOGE("SetDvsyncSwitch load OH_NativeVSync_DVSyncSwitch failed!");
    dlclose(handle_);
    handle_ = nullptr;
    return;
  }
  nativeDvsyncFunc_(vsync_handle_, enableDvsync);
}

}  // namespace flutter
