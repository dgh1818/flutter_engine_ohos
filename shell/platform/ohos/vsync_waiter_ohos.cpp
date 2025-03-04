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
#include "fml/trace_event.h"
#include "napi_common.h"
#include "ohos_logging.h"

namespace flutter {

const char* flutterSyncName = "flutter_connect";

thread_local bool VsyncWaiterOHOS::firstCall = true;

VsyncWaiterOHOS::VsyncWaiterOHOS(const flutter::TaskRunners& task_runners,
                                 std::shared_ptr<bool>& enable_frame_cache)
    : VsyncWaiter(task_runners), enable_frame_cache_(enable_frame_cache) {
  vsyncHandle =
      OH_NativeVSync_Create("flutterSyncName", strlen(flutterSyncName));
}

VsyncWaiterOHOS::~VsyncWaiterOHOS() {
  OH_NativeVSync_Destroy(vsyncHandle);
  vsyncHandle = nullptr;
}

int64_t VsyncWaiterOHOS::GetVsyncPeriod() {
  long long period = 0;
  if (vsyncHandle) {
    OH_NativeVSync_GetPeriod(vsyncHandle, &period);
  }
  return period;
}

void VsyncWaiterOHOS::AwaitVSync() {
  TRACE_EVENT0("flutter", "VsyncWaiterOHOS::AwaitVSync");
  if (vsyncHandle == nullptr) {
    LOGE("AwaitVSync vsyncHandle is nullptr");
    return;
  }
  auto* weak_this = new std::weak_ptr<VsyncWaiter>(shared_from_this());
  OH_NativeVSync* handle = vsyncHandle;

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

}  // namespace flutter
