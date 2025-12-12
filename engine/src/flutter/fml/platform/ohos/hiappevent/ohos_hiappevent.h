/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#ifndef FLUTTER_FML_PLATFORM_OHOS_HIAPPEVENT_OHOS_HIAPPEVENT_H_
#define FLUTTER_FML_PLATFORM_OHOS_HIAPPEVENT_OHOS_HIAPPEVENT_H_

#include <hiappevent/hiappevent.h>
#include <vector>
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
  int64_t endTimeMicros;      // unit: us
  int64_t targetTime;         // unit: ns
  int64_t lastestTargetTime;  // unit: ns
  int missedFrame;
} MissedFrameInfo;

enum class OhosHiappEventFlag {
  kSingleFlag,
  kStaticFlag,
  kScrolledFlag,
};

class OhosHiappEventDDL {
 public:
  OhosHiappEventDDL(void);
  ~OhosHiappEventDDL();

  void Init(void);

  static std::shared_ptr<OhosHiappEventDDL> GetInstance(void);

  void ReportJANKEvent(int64_t endTimeMicros,
                       const char** argumentValues,
                       int argumentCount);

  void ReportScrollJANKEvent(int64_t endTimeMicros,
                              const char** argumentValues,
                              int argumentCount);

  void Flush(void);

  void FlushScroll(void);

  void FlushAllIn(OhosHiappEventFlag type);

  std::unique_ptr<flutter::DynamicLibraryLoader> loader_;

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

  std::vector<MissedFrameInfo> MissedFrameInfos;

 std::vector<MissedFrameInfo> MissedFrameInfosScroll;
};

};  // namespace hiappevent
};  // namespace fml

#endif // FLUTTER_FML_PLATFORM_OHOS_HIAPPEVENT_OHOS_HIAPPEVENT_H_
