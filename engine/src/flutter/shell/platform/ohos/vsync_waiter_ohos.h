/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_VSYNC_WAITER_OHOS_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_VSYNC_WAITER_OHOS_H_
#include <memory>

#include <deviceinfo.h>
#include <native_vsync/native_vsync.h>
#include "flutter/fml/macros.h"
#include "flutter/shell/common/vsync_waiter.h"
#include "napi/platform_view_ohos_napi.h"

namespace flutter {
using NativeDvsyncFunc = int (*)(OH_NativeVSync* nativeVSync, bool enable);

class VsyncWaiterOHOS final : public VsyncWaiter {
 public:
  explicit VsyncWaiterOHOS(const flutter::TaskRunners& task_runners,
                           std::shared_ptr<bool>& enable_frame_cache);

  int64_t GetVsyncPeriod();

  ~VsyncWaiterOHOS() override;

 private:
  thread_local static bool firstCall;
  // |VsyncWaiter|
  void AwaitVSync() override;
  void UpdateDisplayRefreshRate(int64_t period);

  static void OnVsyncFromOHOS(long long timestamp, void* data);
  static void ConsumePendingCallback(std::weak_ptr<VsyncWaiter>* weak_this,
                                     fml::TimePoint frame_start_time,
                                     fml::TimePoint frame_target_time);

  void SetDvsyncSwitch(bool enableDvsync);
  OH_NativeVSync* vsync_handle_;
  NativeDvsyncFunc nativeDvsyncFunc_ = nullptr;
  std::shared_ptr<bool> enable_frame_cache_;
  void* handle_ = nullptr;
  int32_t apiVersion_ = 0;
  FML_DISALLOW_COPY_AND_ASSIGN(VsyncWaiterOHOS);
};
}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_VSYNC_WAITER_OHOS_H_