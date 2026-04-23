/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#ifndef FLUTTER_FML_PLATFORM_OHOS_HIAPPEVENT_OHOS_HIAPPEVENT_H_
#define FLUTTER_FML_PLATFORM_OHOS_HIAPPEVENT_OHOS_HIAPPEVENT_H_

#include <hiappevent/hiappevent.h>
#include <vector>
#include <atomic>
#include "flutter/fml/platform/ohos/dynamic_library_loader.h"

namespace fml {

namespace hiappevent {

using CreateProcessorFunc = HiAppEvent_Processor* (*)(const char* name);
using SetReportRouteFunc = int (*)(HiAppEvent_Processor* processor,
                                   const char* appId,
                                   const char* routeInfo);
using SetReportPoliceFunc = int (*)(HiAppEvent_Processor* processor,
                                    int periodReport,
                                    int batchReport,
                                    bool onStartReport,
                                    bool onBackgroundReport);
using SetReportEventFunc = int (*)(HiAppEvent_Processor* processor,
                                   const char* domain,
                                   const char* name,
                                   bool isRealTime);
using AddFunc = int64_t (*)(HiAppEvent_Processor* processor);

using DestroyProcessor = void (*)(HiAppEvent_Processor* processor);

typedef struct MissedFrameInfo {
  int64_t utc_time_stamp_millis;
  int64_t vsync_start_time_micros;
  int64_t vsync_target_time_micros;
  int64_t latest_vsync_target_time_micros;
  int64_t frame_duration_micros;
  int64_t raster_finish_time_micros;
  int64_t frame_budget_time_micros;
  uint64_t frame_number;
  int vsync_transitions_missed;
 } MissedFrameInfo;

enum class OhosHiappEventFlag {
  kSingleFlag,
  kStaticFlag,
  kScrolledFlag,
};

// ===== Scroll semantic =====
enum class ScrollingStatus : int32_t {
  kScrollStart = 0,
  kScrollEnd   = 1,
};

// Cross-thread visible scroll state
extern std::atomic<int> ScrollStatus;

// Cross-thread scroll start frame ID and end frame ID
extern std::atomic<uint64_t> scroll_start_frame_;
extern std::atomic<uint64_t> scroll_end_frame_;

// Last frame number observed by rasterizer (atomic)
extern std::atomic<uint64_t> last_frame_number_;

class OhosHiappEventDDL {
 public:
  OhosHiappEventDDL(void);
  ~OhosHiappEventDDL();

  void Init(void);

  static std::shared_ptr<OhosHiappEventDDL> GetInstance(void);

  void ReportJANKEvent(const MissedFrameInfo& missed_frame_info);

  void ReportScrollJANKEvent(const MissedFrameInfo& missed_frame_info);

  void Flush(void);

  void FlushScroll(void);

  void FlushAllIn(OhosHiappEventFlag type);

  std::unique_ptr<flutter::DynamicLibraryLoader> loader_;

  // Called every frame from Rasterizer: atomic store only.
  void UpdateLastFrameNumber(uint64_t frame_number);

  // Called from platform_view when scroll status changes.
  // These functions should be invoked on IO thread to keep ordering.
  void OnScrollStart();
  void OnScrollEndAndFlush();

 private:
  int WriteSingleFrame(void);

  int WriteStatisticFrame(void);

  int WriteScrolledFrame(void);

  CreateProcessorFunc createProcessorFunc_ = nullptr;
  SetReportRouteFunc setReportRouteFunc_ = nullptr;
  SetReportPoliceFunc setReportPoliceFunc_ = nullptr;
  SetReportEventFunc setReportEventFunc_ = nullptr;
  AddFunc addFunc_ = nullptr;
  DestroyProcessor destroyProcessor_ = nullptr;

  int apiVersion_ = 0;

  bool isValid_ = false;

  bool isInit_ = false;

  std::vector<MissedFrameInfo> missed_frame_infos;

  std::vector<MissedFrameInfo> missed_frame_infos_scroll;

};

};  // namespace hiappevent
};  // namespace fml

#endif // FLUTTER_FML_PLATFORM_OHOS_HIAPPEVENT_OHOS_HIAPPEVENT_H_
