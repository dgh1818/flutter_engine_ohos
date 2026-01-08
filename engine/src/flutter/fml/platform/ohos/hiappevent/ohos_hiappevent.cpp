/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "ohos_hiappevent.h"

#include <deviceinfo.h>
#include <dlfcn.h>
#include <unistd.h>
#include <map>
#include <thread>
#include <typeinfo>
#include "flutter/fml/logging.h"
#include "flutter/fml/platform/ohos/dynamic_library_loader.h"

// Header files related to UTC time conversion
#include <ctime>
#include <chrono>

namespace fml {

namespace hiappevent {

static std::shared_ptr<OhosHiappEventDDL> instance_ = nullptr;
static std::once_flag instanceFlag_;

static constexpr char HIAPPEVENT_LIB_NAME[] = "libhiappevent_ndk.z.so";

static constexpr char HIAPPEVENT_OTHER_JANK[] = "OTHER_JANK";
static constexpr char HIAPPEVENT_OTHER_JANK_STAT[] = "OTHER_JANK_STAT";
static constexpr char HIAPPEVENT_OTHER_JANK_SCROLL[] = "OTHER_JANK_SCROLL";
static constexpr int64_t kScrollJankThresholdUs = 50 * 1000; // 丢帧超过50ms才上报
static constexpr int64_t SECOND_TO_MICROS_UNIT = 1 * 1000 * 1000; // Unit conversion: second to microsecond
static constexpr int64_t MICROS_TO_MILLIS_UNIT = 1 * 1000; // Unit conversion: microsecond to millisecond 

static const int MISSED_FRAME_INFOS_SIZE = 10;
static const int REQUIRED_API_VERSION = 18;

static int recentScrollCount = 0; // New member: number of scrolls since last scrolled frame jank report

std::atomic<int> ScrollStatus{-1}; // Cross-thread visible scroll state

std::shared_ptr<OhosHiappEventDDL> OhosHiappEventDDL::GetInstance() {
  std::call_once(instanceFlag_, [&] {
    instance_ = std::shared_ptr<OhosHiappEventDDL>(new OhosHiappEventDDL());
  });
  return instance_;
}

OhosHiappEventDDL::OhosHiappEventDDL(void)
    : loader_(std::make_unique<flutter::DynamicLibraryLoader>(HIAPPEVENT_LIB_NAME)) {
  apiVersion_ = flutter::DynamicLibraryLoader::GetApiVersion();
  return;
}

OhosHiappEventDDL::~OhosHiappEventDDL() {

}

void OhosHiappEventDDL::Init(void) {
  if (apiVersion_ < REQUIRED_API_VERSION) {
    return;
  }

  if (isInit_) {
    FML_LOG(INFO) << "Initialization has been completed";
    return;
  }

  std::vector<flutter::SymbolInfo> symbols = {
      {"OH_HiAppEvent_CreateProcessor",
       reinterpret_cast<void**>(&createProcessorFunc_), 18},
      {"OH_HiAppEvent_SetReportRoute",
       reinterpret_cast<void**>(&setReportRouteFunc_), 18},
      {"OH_HiAppEvent_SetReportPolicy",
       reinterpret_cast<void**>(&setReportPoliceFunc_), 18},
      {"OH_HiAppEvent_SetReportEvent",
       reinterpret_cast<void**>(&setReportEventFunc_), 18},
      {"OH_HiAppEvent_AddProcessor", reinterpret_cast<void**>(&addFunc_), 18},
      {"OH_HiAppEvent_DestroyProcessor",
       reinterpret_cast<void**>(&destroyProcessor_), 18},
  };

  isValid_ = loader_->LoadSymbols(symbols);

  isInit_ = true;
  return;
}

//  Record scroll jank event based on MissedFrameInfo struct
void OhosHiappEventDDL::ReportScrollJANKEvent(const MissedFrameInfo& missedFrameInfo) {
  // 50ms threshold
  if (missedFrameInfo.frameDurationMicros < kScrollJankThresholdUs) {
     FML_LOG(INFO)
         << "Ignore scroll jank: frameCost="
         << missedFrameInfo.frameDurationMicros << "us (<50ms)";
     return;
   }

  FML_LOG(INFO)
      << "MissedFrameInfosScroll pushes back.";
  MissedFrameInfosScroll.push_back(missedFrameInfo);
}

// Record jank event based on MissedFrameInfo struct
void OhosHiappEventDDL::ReportJANKEvent(const MissedFrameInfo& missedFrameInfo) {
  if (MissedFrameInfos.size() == MISSED_FRAME_INFOS_SIZE) {
    // MissedFrameInfos is full.
    FML_LOG(INFO) << "Vector stops push_back";
    return;
  } else if (MissedFrameInfos.size() > MISSED_FRAME_INFOS_SIZE) {
    return;
  }

  FML_LOG(INFO)
      << "MissedFrameInfos pushes back.";
  MissedFrameInfos.push_back(missedFrameInfo); 
}

// Report single jank event to HiAppEvent
int OhosHiappEventDDL::WriteSingleFrame(void) {
  if (MissedFrameInfos.size() == 0) {
    FML_LOG(INFO) << "Size of MissedFrameInfos is zero";
    return -1;
  }

  ParamList list = OH_HiAppEvent_CreateParamList();
  if (list == nullptr) {
    FML_LOG(ERROR) << "CreateParamList error";
    return -1;
  }

  int missedFrame = MissedFrameInfos.front().vsyncTransitionsMissed;

  // The first missed frame vsync start time
  int64_t vsyncStartTime = MissedFrameInfos.front().vsyncStartTimeMicros;
  // Convert to UTC time
  int64_t frontRasterFinishTimeMicros = MissedFrameInfos.front().rasterFinishTimeMicros;
  int64_t diff = (frontRasterFinishTimeMicros - vsyncStartTime) / MICROS_TO_MILLIS_UNIT;
  int64_t vsyncStartTimeUTC = MissedFrameInfos.front().UTCTimeStampMillis - diff;

  // The last missed frame's end time (converted to UTC)
  int64_t endTimeUTC = MissedFrameInfos.front().UTCTimeStampMillis;

  if (endTimeUTC < vsyncStartTimeUTC) {
    FML_LOG(ERROR) << "report error, endTime is less than targetTime";
    return -1;
  }

  OH_HiAppEvent_AddStringParam(list, "frameworkName", "FLUTTER");
  OH_HiAppEvent_AddInt32Param(list, "versionCode", 0);
  OH_HiAppEvent_AddInt32Param(list, "missedFrames", missedFrame);
  OH_HiAppEvent_AddInt64Param(list, "startTime", vsyncStartTimeUTC);
  OH_HiAppEvent_AddInt64Param(list, "endTime", endTimeUTC);
  OH_HiAppEvent_AddInt64Param(list, "pid", getpid());

  int ret = OH_HiAppEvent_Write("PERFORMANCE", "OTHER_JANK", BEHAVIOR, list);
  if (ret != 0) {
    FML_LOG(ERROR) << "HiAppEvent_Write error, ret = " << ret;
  }

  OH_HiAppEvent_DestroyParamList(list);
  return ret;
}

// Report statistic jank event to HiAppEvent
int OhosHiappEventDDL::WriteStatisticFrame(void) {
  if (MissedFrameInfos.size() == 0) {
    FML_LOG(INFO) << "Size of MissedFrameInfos is zero";
    return -1;
  }

  ParamList list = OH_HiAppEvent_CreateParamList();
  if (list == nullptr) {
    FML_LOG(ERROR) << "CreateParamList error";
    return -1;
  }

  int totalMissedFrames = 0;
  // The maximum dropped frame time and its corresponding index
  int64_t maxDiffTime = 0;
  int targetIndex = 0;
  int index = 0;
  for (auto it = MissedFrameInfos.begin(); it != MissedFrameInfos.end(); it++) {
    totalMissedFrames += (*it).vsyncTransitionsMissed;
    int frameDurationMicros = (*it).frameDurationMicros;
    if (frameDurationMicros > maxDiffTime) {
      maxDiffTime = frameDurationMicros;
      targetIndex = index;
    }
    index++;
  }
  // The maximum dropped frame time corresponding frame rate
  int maxFPS = 0;
  if (maxDiffTime > 0) {
    maxFPS = static_cast<int>(SECOND_TO_MICROS_UNIT / maxDiffTime);
  }

  // The first missed frame vsync start time
  int64_t vsyncStartTime = MissedFrameInfos.front().vsyncStartTimeMicros;
  // Convert to UTC time
  int64_t frontRasterFinishTimeMicros = MissedFrameInfos.front().rasterFinishTimeMicros;
  int64_t diff = (frontRasterFinishTimeMicros - vsyncStartTime) / MICROS_TO_MILLIS_UNIT;
  int64_t vsyncStartTimeUTC = MissedFrameInfos.front().UTCTimeStampMillis - diff;

  // The last missed frame's expected vsync time
  int64_t backLastestTargetTime = MissedFrameInfos.back().latestVsyncTargetTimeMicros;
  // Convert to UTC time
  int64_t backRasterFinishTimeMicros = MissedFrameInfos.back().rasterFinishTimeMicros;
  diff = (backLastestTargetTime - backRasterFinishTimeMicros) / MICROS_TO_MILLIS_UNIT;
  int64_t backLastestTargetTimeUTC = MissedFrameInfos.back().UTCTimeStampMillis + diff;

  OH_HiAppEvent_AddStringParam(list, "frameworkName", "FLUTTER");
  OH_HiAppEvent_AddInt32Param(list, "versionCode", 0);

  OH_HiAppEvent_AddInt32Param(list, "maxMissedFrameRate", maxFPS);
  OH_HiAppEvent_AddInt32Param(list, "totalMissedFrames", totalMissedFrames);
  OH_HiAppEvent_AddInt64Param(list, "maxFrameTime", maxDiffTime / MICROS_TO_MILLIS_UNIT);
  OH_HiAppEvent_AddInt64Param(list, "startTime", vsyncStartTimeUTC);
  OH_HiAppEvent_AddInt64Param(list, "endTime", backLastestTargetTimeUTC);
  OH_HiAppEvent_AddInt64Param(list, "pid", getpid());

  int ret =
      OH_HiAppEvent_Write("PERFORMANCE", "OTHER_JANK_STAT", STATISTIC, list);
  if (ret != 0) {
    FML_LOG(ERROR) << "HiAppEvent_Write error, ret = " << ret;
  }

  OH_HiAppEvent_DestroyParamList(list);
  return ret;
}

// Report scrolled frame jank event to HiAppEvent
int OhosHiappEventDDL::WriteScrolledFrame(void) {
  if (MissedFrameInfosScroll.size() == 0) {
    FML_LOG(INFO) << "Size of MissedFrameInfosScroll is zero";
    return -1;
  }

  ParamList list = OH_HiAppEvent_CreateParamList(); // Create a pointer to the parameter list
  if (list == nullptr) {
    FML_LOG(ERROR) << "CreateParamList error";
    return -1;
  }
  // Total missed frames
  int totalMissedFrames = 0;
  // The maximum dropped frame time and its corresponding index
  int64_t maxDiffTime = 0;
  int targetIndex = 0;
  int index = 0;
  for (auto it = MissedFrameInfosScroll.begin(); it != MissedFrameInfosScroll.end(); it++) {
    totalMissedFrames += (*it).vsyncTransitionsMissed; // Calculate total missed frames

    int frameDurationMicros = (*it).frameDurationMicros;
    if (frameDurationMicros > maxDiffTime) {
      maxDiffTime = frameDurationMicros;
      targetIndex = index;
    }
    index++;
  }
  
  // The frame rate corresponding to the longest frame loss time
  int maxFPS = 0;
  if (maxDiffTime > 0) {
    maxFPS = static_cast<int>(SECOND_TO_MICROS_UNIT / maxDiffTime);
  }

  // TODO: The frame number corresponding to the longest frame loss time

  // The first missed frame vsync start time
  int64_t vsyncStartTime = MissedFrameInfosScroll.front().vsyncStartTimeMicros;
  // Convert to UTC time
  int64_t frontRasterFinishTimeMicros = MissedFrameInfosScroll.front().rasterFinishTimeMicros;
  int64_t diff = (frontRasterFinishTimeMicros - vsyncStartTime) / MICROS_TO_MILLIS_UNIT;
  int64_t vsyncStartTimeUTC = MissedFrameInfosScroll.front().UTCTimeStampMillis - diff;

  // The last missed frame's expected vsync time
  int64_t backLastestTargetTime = MissedFrameInfosScroll.back().latestVsyncTargetTimeMicros;
  // Convert to UTC time
  int64_t backRasterFinishTimeMicros = MissedFrameInfosScroll.back().rasterFinishTimeMicros;
  diff = (backLastestTargetTime - backRasterFinishTimeMicros) / MICROS_TO_MILLIS_UNIT;
  int64_t backLastestTargetTimeUTC = MissedFrameInfosScroll.back().UTCTimeStampMillis + diff;

  // The total number of frames during the scrolled frame jank reporting process
  int totalFrames = MissedFrameInfosScroll.back().frameNumber - MissedFrameInfosScroll.front().frameNumber + 1;

  OH_HiAppEvent_AddStringParam(list, "frameworkName", "FLUTTER");
  OH_HiAppEvent_AddInt32Param(list, "versionCode", 0);

  // Start time (unit: ms)
  OH_HiAppEvent_AddInt64Param(list, "startTime", vsyncStartTimeUTC);
  // End time (unit: ms)
  OH_HiAppEvent_AddInt64Param(list, "endTime", backLastestTargetTimeUTC);
  // Maximum dropped frame duration (unit: ms)
  OH_HiAppEvent_AddInt64Param(list, "maxFrameTime", maxDiffTime / MICROS_TO_MILLIS_UNIT);
  // TODO: The frame number corresponding to the longest frame loss time

  // The frame rate corresponding to the longest frame loss time
  OH_HiAppEvent_AddInt32Param(list, "maxMissedFrameRate", maxFPS);
  // Total missed frames
  OH_HiAppEvent_AddInt32Param(list, "totalMissedFrames", totalMissedFrames);
  // Total frames
  OH_HiAppEvent_AddInt32Param(list, "totalFrames", totalFrames);
  // Number of scrolls since last scrolled frame jank report
  OH_HiAppEvent_AddInt32Param(list, "recentScrollCount", recentScrollCount);
  // Process ID
  OH_HiAppEvent_AddInt64Param(list, "pid", getpid());

  int ret = // Event tracking
      OH_HiAppEvent_Write("PERFORMANCE", "OTHER_JANK_SCROLL", BEHAVIOR, list);
  if (ret != 0) {
    FML_LOG(ERROR) << "HiAppEvent_Write error, ret = " << ret;
  }

  OH_HiAppEvent_DestroyParamList(list);
  return ret;
}

void OhosHiappEventDDL::Flush(void) {
  Init();

  FlushAllIn(OhosHiappEventFlag::kSingleFlag);
  FlushAllIn(OhosHiappEventFlag::kStaticFlag);

  MissedFrameInfos.clear();
}

void OhosHiappEventDDL::FlushScroll(void) {
  Init();

  // Check if there were any frames lost during scrolling.
  // If there were, increment the scroll count.
  if (MissedFrameInfosScroll.size() != 0) {
    recentScrollCount++;
  } 

  FlushAllIn(OhosHiappEventFlag::kScrolledFlag);

 // Scrolled frame jank report completed, reset
  recentScrollCount = 0;
  MissedFrameInfosScroll.clear();
}

void OhosHiappEventDDL::FlushAllIn(OhosHiappEventFlag type) {
  if (!isValid_) {
    FML_LOG(ERROR) << "flush isValid_ false";
    return;
  }

  if (type == OhosHiappEventFlag::kScrolledFlag) {
    if (MissedFrameInfosScroll.empty()) {
      return;
    }
  } else {
    if (MissedFrameInfos.empty()) {
      return;
    }
  }

  HiAppEvent_Processor* processor = reinterpret_cast<HiAppEvent_Processor*>(
      createProcessorFunc_("xperfbridge"));
  if (processor == nullptr) {
    FML_LOG(ERROR) << "processor == nullptr";
    return;
  }

  setReportPoliceFunc_(processor, 1, 1, true, true);

  switch (type) {
    case OhosHiappEventFlag::kSingleFlag:
      setReportEventFunc_(processor, "PERFORMANCE", HIAPPEVENT_OTHER_JANK, true);
      break;
    case OhosHiappEventFlag::kStaticFlag:
      setReportEventFunc_(processor, "PERFORMANCE", HIAPPEVENT_OTHER_JANK_STAT, true);
      break;
    case OhosHiappEventFlag::kScrolledFlag:
      setReportEventFunc_(processor, "PERFORMANCE", HIAPPEVENT_OTHER_JANK_SCROLL, true);
      break;
    default:
      break;
  }

  int64_t processorId = addFunc_(processor);
  if (processorId <= 0) {
    FML_LOG(ERROR) << "processorId error";
    destroyProcessor_(processor);
    return;
  }

  int ret = -1;
  switch (type) {
    case OhosHiappEventFlag::kSingleFlag:
      ret = WriteSingleFrame();
      break;
    case OhosHiappEventFlag::kStaticFlag:
      ret = WriteStatisticFrame();
      break;
    case OhosHiappEventFlag::kScrolledFlag:
      ret = WriteScrolledFrame();
      break;
    default:
      break;
  }

  destroyProcessor_(processor);
  if (ret != 0) {
    FML_LOG(ERROR) << "flush error: type = " << static_cast<int32_t>(type);
  }
  return;
}

void OhosHiappEventDDL::RecordScrollStatus(int scrollStatus) {
  if (scrollStatus != ScrollStatus.load()) {
      ScrollStatus.store(scrollStatus);
  }
  return;
}

};  // namespace hiappevent

};  // namespace fml
