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

#include <cstdint>
#include <climits>

namespace fml {

namespace hiappevent {

static std::shared_ptr<OhosHiappEventDDL> instance_ = nullptr;
static std::once_flag instanceFlag_;

static constexpr char HIAPPEVENT_LIB_NAME[] = "libhiappevent_ndk.z.so";

static constexpr char HIAPPEVENT_OTHER_JANK[] = "OTHER_JANK";
static constexpr char HIAPPEVENT_OTHER_JANK_STAT[] = "OTHER_JANK_STAT";
static constexpr char HIAPPEVENT_OTHER_JANK_SCROLL[] = "OTHER_JANK_SCROLL";
static constexpr int64_t K_SCROLL_JANK_THRESHOLD_US = 50 * 1000; // 丢帧超过50ms才上报
static constexpr int64_t SECOND_TO_MICROS_UNIT = 1 * 1000 * 1000; // Unit conversion: second to microsecond
static constexpr int64_t MICROS_TO_MILLIS_UNIT = 1 * 1000; // Unit conversion: microsecond to millisecond 

static const int MISSED_FRAME_INFOS_SIZE = 10;
static const int REQUIRED_API_VERSION = 18;

static int recent_scroll_count = 0; // New member: Number of scroll sessions since last scroll-jank report

std::atomic<int> ScrollStatus{-1}; // Cross-thread visible scroll state
std::atomic<uint64_t> scroll_start_frame_{0}; // Frame ID at the beginning of scrolling
std::atomic<uint64_t> scroll_end_frame_{0};   // Frame ID at the end of scrolling
std::atomic<uint64_t> last_frame_number_{0}; // Last frame number observed by rasterizer (atomic)

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
void OhosHiappEventDDL::ReportScrollJANKEvent(const MissedFrameInfo& missed_frame_info) {
  // 50ms threshold
  if (missed_frame_info.frame_duration_micros < K_SCROLL_JANK_THRESHOLD_US) {
     FML_LOG(INFO)
         << "Ignore scroll jank: frameCost="
         << missed_frame_info.frame_duration_micros << "us (<50ms)";
     return;
   }

  missed_frame_infos_scroll.push_back(missed_frame_info);
}

// Record jank event based on MissedFrameInfo struct
void OhosHiappEventDDL::ReportJANKEvent(const MissedFrameInfo& missed_frame_info) {
  if (missed_frame_infos.size() == MISSED_FRAME_INFOS_SIZE) {
    // missed_frame_infos is full.
    FML_LOG(INFO) << "Vector stops push_back";
    return;
  } else if (missed_frame_infos.size() > MISSED_FRAME_INFOS_SIZE) {
    return;
  }

  missed_frame_infos.push_back(missed_frame_info); 
}

// Report single jank event to HiAppEvent
int OhosHiappEventDDL::WriteSingleFrame(void) {
  if (missed_frame_infos.size() == 0) {
    FML_LOG(INFO) << "Size of missed_frame_infos is zero";
    return -1;
  }

  ParamList list = OH_HiAppEvent_CreateParamList();
  if (list == nullptr) {
    FML_LOG(ERROR) << "CreateParamList error";
    return -1;
  }

  int missed_frame = missed_frame_infos.front().vsync_transitions_missed;

  // The first missed frame vsync start time
  int64_t vsync_start_time = missed_frame_infos.front().vsync_start_time_micros;
  // Convert to UTC time
  int64_t front_raster_finish_time_micros = missed_frame_infos.front().raster_finish_time_micros;
  int64_t diff = (front_raster_finish_time_micros - vsync_start_time) / MICROS_TO_MILLIS_UNIT;
  int64_t vsync_start_time_utc = missed_frame_infos.front().utc_time_stamp_millis - diff;

  // The last missed frame's end time (converted to UTC)
  int64_t end_time_utc = missed_frame_infos.front().utc_time_stamp_millis;

  if (end_time_utc < vsync_start_time_utc) {
    FML_LOG(ERROR) << "Report error, endTime is less than targetTime";
    return -1;
  }

  OH_HiAppEvent_AddStringParam(list, "frameworkName", "FLUTTER");
  OH_HiAppEvent_AddInt32Param(list, "versionCode", 0);
  OH_HiAppEvent_AddInt32Param(list, "missedFrames", missed_frame);
  OH_HiAppEvent_AddInt64Param(list, "startTime", vsync_start_time_utc);
  OH_HiAppEvent_AddInt64Param(list, "endTime", end_time_utc);
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
  if (missed_frame_infos.size() == 0) {
    FML_LOG(INFO) << "Size of missed_frame_infos is zero";
    return -1;
  }

  ParamList list = OH_HiAppEvent_CreateParamList();
  if (list == nullptr) {
    FML_LOG(ERROR) << "CreateParamList error";
    return -1;
  }

  int total_missed_frames = 0;
  // The maximum dropped frame time and its corresponding index
  int64_t max_diff_time = 0;
  int target_index = 0;
  int index = 0;
  for (auto it = missed_frame_infos.begin(); it != missed_frame_infos.end(); it++) {
    total_missed_frames += (*it).vsync_transitions_missed;
    int frame_duration_micros = (*it).frame_duration_micros;
    if (frame_duration_micros > max_diff_time) {
      max_diff_time = frame_duration_micros;
      target_index = index;
    }
    index++;
  }
  // The maximum dropped frame time corresponding frame rate
  int max_FPS = 0;
  if (max_diff_time > 0) {
    max_FPS = static_cast<int>(SECOND_TO_MICROS_UNIT / max_diff_time);
  }

  // The first missed frame vsync start time
  int64_t vsync_start_time = missed_frame_infos.front().vsync_start_time_micros;
  // Convert to UTC time
  int64_t front_raster_finish_time_micros = missed_frame_infos.front().raster_finish_time_micros;
  int64_t diff = (front_raster_finish_time_micros - vsync_start_time) / MICROS_TO_MILLIS_UNIT;
  int64_t vsync_start_time_utc = missed_frame_infos.front().utc_time_stamp_millis - diff;
  // The last missed frame's expected vsync time
  int64_t back_lastest_target_time = missed_frame_infos.back().latest_vsync_target_time_micros;
  // Convert to UTC time
  int64_t back_raster_finish_time_micros = missed_frame_infos.back().raster_finish_time_micros;
  diff = (back_lastest_target_time - back_raster_finish_time_micros) / MICROS_TO_MILLIS_UNIT;
  int64_t back_lastest_target_time_utc = missed_frame_infos.back().utc_time_stamp_millis + diff;

  OH_HiAppEvent_AddStringParam(list, "frameworkName", "FLUTTER");
  OH_HiAppEvent_AddInt32Param(list, "versionCode", 0);

  OH_HiAppEvent_AddInt32Param(list, "maxMissedFrameRate", max_FPS);
  OH_HiAppEvent_AddInt32Param(list, "totalMissedFrames", total_missed_frames);
  OH_HiAppEvent_AddInt64Param(list, "maxFrameTime", max_diff_time / MICROS_TO_MILLIS_UNIT);
  OH_HiAppEvent_AddInt64Param(list, "startTime", vsync_start_time_utc);
  OH_HiAppEvent_AddInt64Param(list, "endTime", back_lastest_target_time_utc);
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
  if (missed_frame_infos_scroll.size() == 0) {
    FML_LOG(INFO) << "Size of missed_frame_infos_scroll is zero";
    return -1;
  }

  ParamList list = OH_HiAppEvent_CreateParamList(); // Create a pointer to the parameter list
  if (list == nullptr) {
    FML_LOG(ERROR) << "CreateParamList error";
    return -1;
  }

  // The total number of frames during the scrolled frame jank reporting process
  uint64_t start_frame_id = scroll_start_frame_.load();
  uint64_t end_frame_id = scroll_end_frame_.load();  
  int total_frames_during_scroll = 0;
  if (start_frame_id != 0 && end_frame_id >= start_frame_id) {
    const uint64_t diff = end_frame_id - start_frame_id + 1;
    total_frames_during_scroll = (diff > static_cast<uint64_t>(INT32_MAX))
                                  ? INT32_MAX
                                  : static_cast<int>(diff);
  }

  // Total missed frames
  int total_missed_frames = 0;
  // The maximum dropped frame time and its corresponding index
  int64_t max_diff_time = 0;
  int target_index = 0;
  int index = 0;
  for (auto it = missed_frame_infos_scroll.begin(); it != missed_frame_infos_scroll.end(); it++) {
    total_missed_frames += (*it).vsync_transitions_missed; // Calculate total missed frames

    int frame_duration_micros = (*it).frame_duration_micros;
    if (frame_duration_micros > max_diff_time) {
      max_diff_time = frame_duration_micros;
      target_index = index;
    }
    index++;
  }
  
  // The frame rate corresponding to the longest frame loss time
  int max_FPS = 0;
  if (max_diff_time > 0) {
    max_FPS = static_cast<int>(SECOND_TO_MICROS_UNIT / max_diff_time);
  }

  // The frame number corresponding to the longest frame loss time
  uint64_t frame_number = missed_frame_infos_scroll[target_index].frame_number;

  // The first missed frame vsync start time
  int64_t vsync_start_time = missed_frame_infos_scroll.front().vsync_start_time_micros;
  // Convert to UTC time
  int64_t front_raster_finish_time_micros = missed_frame_infos_scroll.front().raster_finish_time_micros;
  int64_t diff = (front_raster_finish_time_micros - vsync_start_time) / MICROS_TO_MILLIS_UNIT;
  int64_t vsync_start_time_utc = missed_frame_infos_scroll.front().utc_time_stamp_millis - diff;
  // The last missed frame's expected vsync time
  int64_t back_lastest_target_time = missed_frame_infos_scroll.back().latest_vsync_target_time_micros;
  // Convert to UTC time
  int64_t back_raster_finish_time_micros = missed_frame_infos_scroll.back().raster_finish_time_micros;
  diff = (back_lastest_target_time - back_raster_finish_time_micros) / MICROS_TO_MILLIS_UNIT;
  int64_t back_lastest_target_time_utc = missed_frame_infos_scroll.back().utc_time_stamp_millis + diff;

  OH_HiAppEvent_AddStringParam(list, "frameworkName", "FLUTTER");
  OH_HiAppEvent_AddInt32Param(list, "versionCode", 0);

  // Start time (unit: ms)
  OH_HiAppEvent_AddInt64Param(list, "startTime", vsync_start_time_utc);
  // End time (unit: ms)
  OH_HiAppEvent_AddInt64Param(list, "endTime", back_lastest_target_time_utc);
  // Maximum dropped frame duration (unit: ms)
  OH_HiAppEvent_AddInt64Param(list, "maxFrameTime", max_diff_time / MICROS_TO_MILLIS_UNIT);
  // TODO: The frame number corresponding to the longest frame loss time
  OH_HiAppEvent_AddInt64Param(list, "frameId", frame_number);
  // The frame rate corresponding to the longest frame loss time
  OH_HiAppEvent_AddInt32Param(list, "maxMissedFrameRate", max_FPS);
  // Total missed frames
  OH_HiAppEvent_AddInt32Param(list, "totalMissedFrames", total_missed_frames);
  // Total frames
  OH_HiAppEvent_AddInt32Param(list, "totalFrames", total_frames_during_scroll);
  // Number of scrolls since last scrolled frame jank report
  OH_HiAppEvent_AddInt32Param(list, "recentScrollCount", recent_scroll_count);
  // Process ID
  OH_HiAppEvent_AddInt64Param(list, "pid", getpid());

  int ret = // Event tracking
      OH_HiAppEvent_Write("PERFORMANCE", "OTHER_JANK_SCROLL", BEHAVIOR, list);
  if (ret != 0) {
    FML_LOG(ERROR) << "HiAppEvent_Write error, ret = " << ret;
  }

  // Reset scroll start and end frame IDs
  scroll_start_frame_.store(0, std::memory_order_relaxed);
  scroll_end_frame_.store(0, std::memory_order_relaxed);

  OH_HiAppEvent_DestroyParamList(list);
  return ret;
}

void OhosHiappEventDDL::Flush(void) {
  Init();

  FlushAllIn(OhosHiappEventFlag::kSingleFlag);
  FlushAllIn(OhosHiappEventFlag::kStaticFlag);

  missed_frame_infos.clear();
}

void OhosHiappEventDDL::FlushScroll(void) {
  Init();

  recent_scroll_count++;
  if (missed_frame_infos_scroll.size() == 0) {
    return;
  } 

  FlushAllIn(OhosHiappEventFlag::kScrolledFlag);

 // Scrolled frame jank report completed, reset
  recent_scroll_count = 0;
  missed_frame_infos_scroll.clear();
}

void OhosHiappEventDDL::FlushAllIn(OhosHiappEventFlag type) {
  if (!isValid_) {
    FML_LOG(ERROR) << "flush isValid_ false";
    return;
  }

  if (type == OhosHiappEventFlag::kScrolledFlag) {
    if (missed_frame_infos_scroll.empty()) {
      return;
    }
  } else {
    if (missed_frame_infos.empty()) {
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

void OhosHiappEventDDL::UpdateLastFrameNumber(uint64_t frame_number) {
  // Relaxed is enough: we only need latest value, no ordering constraints.
  last_frame_number_.store(frame_number, std::memory_order_relaxed);
}

void OhosHiappEventDDL::OnScrollStart() {
  // Mark state first
  ScrollStatus.store(static_cast<int>(ScrollingStatus::kScrollStart),
                    std::memory_order_relaxed);

  const uint64_t cur_frame_number = last_frame_number_.load(std::memory_order_relaxed);
  scroll_start_frame_.store(cur_frame_number, std::memory_order_relaxed);
  // Init end = start, so totalFrames is at least 1 if we flush immediately.
  scroll_end_frame_.store(cur_frame_number, std::memory_order_relaxed);
}

void OhosHiappEventDDL::OnScrollEndAndFlush() {
  ScrollStatus.store(static_cast<int>(ScrollingStatus::kScrollEnd),
                    std::memory_order_relaxed);

  // Snapshot end from last seen raster frame.
  const uint64_t end = last_frame_number_.load(std::memory_order_relaxed);
  scroll_end_frame_.store(end, std::memory_order_relaxed);

  // Now flush AFTER we have end frame id.
  FlushScroll();
}



};  // namespace hiappevent

};  // namespace fml
