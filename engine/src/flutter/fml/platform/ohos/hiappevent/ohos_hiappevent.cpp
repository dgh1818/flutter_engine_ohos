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
#include <chrono>
#include <ctime>

#include <climits>
#include <cstdint>

namespace fml {

namespace hiappevent {

const char* OhosHiappEventDDL::GetFlutterVersion() {
  return FLUTTER_VERSION;
}

static std::shared_ptr<OhosHiappEventDDL> instance_ = nullptr;
static std::once_flag instanceFlag_;

static constexpr char HIAPPEVENT_LIB_NAME[] = "libhiappevent_ndk.z.so";

static constexpr char COUNTER_FRAME_DROP[] = "Flutter Lost Frames";
static constexpr char FRAME_DROP_DURATION[] = "Flutter Hitch Time";
static constexpr char HIAPPEVENT_OTHER_JANK[] = "OTHER_JANK";
static constexpr char HIAPPEVENT_OTHER_JANK_STAT[] = "OTHER_JANK_STAT";
static constexpr char HIAPPEVENT_OTHER_JANK_SCROLL[] = "OTHER_JANK_SCROLL";
static constexpr int64_t K_SCROLL_JANK_THRESHOLD_US =
    50 * 1000;  // 丢帧超过50ms才上报
static constexpr int64_t SECOND_TO_MICROS_UNIT =
    1 * 1000 * 1000;  // Unit conversion: second to microsecond
static constexpr int64_t MICROS_TO_MILLIS_UNIT =
    1 * 1000;  // Unit conversion: microsecond to millisecond
static constexpr int32_t K_FLUTTER_DART_FRAMEWORK_TYPE =
    0;  // OH_FLUTTER_DART enum value (API 26)

static const int MISSED_FRAME_INFOS_SIZE = 10;
static const int REQUIRED_API_VERSION = 18;
static constexpr int32_t REQUIRED_HITRACE_EX_API_VERSION = 19;

static int recent_scroll_count =
    0;  // New member: Number of scroll sessions since last scroll-jank report

static constexpr int32_t TRACE_ARGS_BUFFER_SIZE = 128;

std::atomic<int> ScrollStatus{-1};  // Cross-thread visible scroll state
std::atomic<uint64_t> scroll_start_frame_{
    0};  // Frame ID at the beginning of scrolling
std::atomic<uint64_t> scroll_end_frame_{0};  // Frame ID at the end of scrolling
std::atomic<int64_t> scroll_start_time_utc_ms{0};  // Scroll start UTC time (ms)
std::atomic<int64_t> scroll_end_time_utc_ms{0};    // Scroll end UTC time (ms)
std::atomic<uint64_t> last_frame_number_{
    0};  // Last frame number observed by rasterizer (atomic)

std::shared_ptr<OhosHiappEventDDL> OhosHiappEventDDL::GetInstance() {
  std::call_once(instanceFlag_, [&] {
    instance_ = std::shared_ptr<OhosHiappEventDDL>(new OhosHiappEventDDL());
  });
  return instance_;
}

OhosHiappEventDDL::OhosHiappEventDDL(void)
    : loader_(std::make_unique<flutter::DynamicLibraryLoader>(
          HIAPPEVENT_LIB_NAME)) {
  apiVersion_ = flutter::DynamicLibraryLoader::GetApiVersion();
  Init();
}

OhosHiappEventDDL::~OhosHiappEventDDL() {}

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
      {"OH_HiAppEvent_ReportFrameworkMemAnomaly",
       reinterpret_cast<void**>(&reportFrameworkMemAnomaly_), 26},
  };

  isValid_ = loader_->LoadSymbols(symbols);

  // Load extended HiTrace functions
  if (apiVersion_ >= REQUIRED_HITRACE_EX_API_VERSION) {
    dlerror();  // Clear error
    startAsyncTraceExFunc_ = reinterpret_cast<StartAsyncTraceExFunc>(
        dlsym(RTLD_DEFAULT, "OH_HiTrace_StartAsyncTraceEx"));
    const char* startError = dlerror();

    dlerror();  // Clear error
    finishAsyncTraceExFunc_ = reinterpret_cast<FinishAsyncTraceExFunc>(
        dlsym(RTLD_DEFAULT, "OH_HiTrace_FinishAsyncTraceEx"));
    const char* finishError = dlerror();

    if (startError || finishError) {
      FML_LOG(WARNING) << "OH_HiTrace_StartAsyncTraceEx or "
                          "OH_HiTrace_FinishAsyncTraceEx not found";
    }
  }

  isInit_ = true;
  return;
}

//  Record scroll jank event based on MissedFrameInfo struct
void OhosHiappEventDDL::ReportScrollJANKEvent(
    const MissedFrameInfo& missed_frame_info) {
  // 50ms threshold
  if (missed_frame_info.frame_duration_micros < K_SCROLL_JANK_THRESHOLD_US) {
    FML_LOG(INFO) << "Ignore scroll jank: frameCost="
                  << missed_frame_info.frame_duration_micros << "us (<50ms)";
    return;
  }

  WriteJANKEventToTrace(missed_frame_info, OhosDropFrameReason::kScroll);
  missed_frame_infos_scroll.push_back(missed_frame_info);
}

// Record jank event based on MissedFrameInfo struct
void OhosHiappEventDDL::ReportJANKEvent(
    const MissedFrameInfo& missed_frame_info) {
  WriteJANKEventToTrace(missed_frame_info, OhosDropFrameReason::kCommon);

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
    FML_LOG(ERROR)
        << "OH_HiAppEvent_CreateParamList() failed, returned nullptr";
    return -1;
  }

  int missed_frame = missed_frame_infos.front().vsync_transitions_missed;

  // The first missed frame vsync start time
  int64_t vsync_start_time = missed_frame_infos.front().vsync_start_time_micros;
  // Convert to UTC time
  int64_t front_raster_finish_time_micros =
      missed_frame_infos.front().raster_finish_time_micros;
  int64_t diff = (front_raster_finish_time_micros - vsync_start_time) /
                 MICROS_TO_MILLIS_UNIT;
  int64_t vsync_start_time_utc =
      missed_frame_infos.front().utc_time_stamp_millis - diff;

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
    FML_LOG(ERROR) << "OH_HiAppEvent_Write() error, ret = " << ret;
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
    FML_LOG(ERROR)
        << "OH_HiAppEvent_CreateParamList() failed, returned nullptr";
    return -1;
  }

  int total_missed_frames = 0;
  // The maximum dropped frame time and its corresponding index
  int64_t max_diff_time = 0;
  // The frame budget time corresponding to the longest frame loss time
  int64_t max_frame_budget = 0;
  int target_index = 0;
  int index = 0;
  for (auto it = missed_frame_infos.begin(); it != missed_frame_infos.end();
       it++) {
    total_missed_frames += (*it).vsync_transitions_missed;
    int64_t frame_duration_micros = (*it).frame_duration_micros;
    if (frame_duration_micros > max_diff_time) {
      max_diff_time = frame_duration_micros;
      max_frame_budget = (*it).frame_budget_time_micros;
      target_index = index;
    }
    index++;
  }
  // The maximum dropped frame time corresponding frame rate
  int max_FPS = 0;
  if (max_frame_budget > 0) {
    max_FPS = static_cast<int>(SECOND_TO_MICROS_UNIT / max_frame_budget);
  }

  // The actual maximum frame time (raster finish time - vsync start time)
  int64_t max_frame_time = 0;
  if (target_index >= 0 &&
      target_index < static_cast<int>(missed_frame_infos.size())) {
    max_frame_time =
        missed_frame_infos[target_index].raster_finish_time_micros -
        missed_frame_infos[target_index].vsync_start_time_micros;
  }

  // The first missed frame vsync start time
  int64_t vsync_start_time = missed_frame_infos.front().vsync_start_time_micros;
  // Convert to UTC time
  int64_t front_raster_finish_time_micros =
      missed_frame_infos.front().raster_finish_time_micros;
  int64_t offset_ms = missed_frame_infos.front().utc_time_stamp_millis -
                      front_raster_finish_time_micros / MICROS_TO_MILLIS_UNIT;
  int64_t vsync_start_time_utc =
      vsync_start_time / MICROS_TO_MILLIS_UNIT + offset_ms;
  // The last missed frame's expected vsync time
  int64_t back_lastest_target_time =
      missed_frame_infos.back().latest_vsync_target_time_micros;
  // Convert to UTC time
  int64_t back_lastest_target_time_utc =
      back_lastest_target_time / MICROS_TO_MILLIS_UNIT + offset_ms;

  OH_HiAppEvent_AddStringParam(list, "frameworkName", "FLUTTER");
  OH_HiAppEvent_AddInt32Param(list, "versionCode", 0);

  OH_HiAppEvent_AddInt32Param(list, "maxMissedFrameRate", max_FPS);
  OH_HiAppEvent_AddInt32Param(list, "totalMissedFrames", total_missed_frames);
  OH_HiAppEvent_AddInt64Param(list, "maxFrameTime",
                              max_frame_time / MICROS_TO_MILLIS_UNIT);
  OH_HiAppEvent_AddInt64Param(list, "startTime", vsync_start_time_utc);
  OH_HiAppEvent_AddInt64Param(list, "endTime", back_lastest_target_time_utc);
  OH_HiAppEvent_AddInt64Param(list, "pid", getpid());

  int ret =
      OH_HiAppEvent_Write("PERFORMANCE", "OTHER_JANK_STAT", STATISTIC, list);
  if (ret != 0) {
    FML_LOG(ERROR) << "OH_HiAppEvent_Write() error, ret = " << ret;
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

  ParamList list = OH_HiAppEvent_CreateParamList();  // Create a pointer to the
                                                     // parameter list
  if (list == nullptr) {
    FML_LOG(ERROR)
        << "OH_HiAppEvent_CreateParamList() failed, returned nullptr";
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
  // The frame budget time corresponding to the longest frame loss time
  int64_t max_frame_budget = 0;
  int target_index = 0;
  int index = 0;
  for (auto it = missed_frame_infos_scroll.begin();
       it != missed_frame_infos_scroll.end(); it++) {
    total_missed_frames += (*it).vsync_transitions_missed;
    int64_t frame_duration_micros = (*it).frame_duration_micros;
    if (frame_duration_micros > max_diff_time) {
      max_diff_time = frame_duration_micros;
      max_frame_budget = (*it).frame_budget_time_micros;
      target_index = index;
    }
    index++;
  }

  // The frame rate corresponding to the longest frame loss time
  int max_FPS = 0;
  if (max_frame_budget > 0) {
    max_FPS = static_cast<int>(SECOND_TO_MICROS_UNIT / max_frame_budget);
  }

  int64_t max_frame_time = 0;
  uint64_t frame_number = 0;
  if (target_index >= 0 &&
      target_index < static_cast<int>(missed_frame_infos_scroll.size())) {
    max_frame_time =
        missed_frame_infos_scroll[target_index].raster_finish_time_micros -
        missed_frame_infos_scroll[target_index].vsync_start_time_micros;

    // The frame number corresponding to the longest frame loss time
    frame_number = missed_frame_infos_scroll[target_index].frame_number;
  }

  int64_t scroll_start_utc =
      scroll_start_time_utc_ms.load(std::memory_order_relaxed);
  int64_t scroll_end_utc =
      scroll_end_time_utc_ms.load(std::memory_order_relaxed);

  if (scroll_start_utc == 0) {
    int64_t vsync_start_time =
        missed_frame_infos_scroll.front().vsync_start_time_micros;
    int64_t front_raster_finish_time_micros =
        missed_frame_infos_scroll.front().raster_finish_time_micros;
    int64_t offset_ms =
        missed_frame_infos_scroll.front().utc_time_stamp_millis -
        front_raster_finish_time_micros / MICROS_TO_MILLIS_UNIT;
    scroll_start_utc = vsync_start_time / MICROS_TO_MILLIS_UNIT + offset_ms;
  }
  if (scroll_end_utc == 0) {
    int64_t back_lastest_target_time =
        missed_frame_infos_scroll.back().latest_vsync_target_time_micros;
    int64_t front_raster_finish_time_micros =
        missed_frame_infos_scroll.front().raster_finish_time_micros;
    int64_t offset_ms =
        missed_frame_infos_scroll.front().utc_time_stamp_millis -
        front_raster_finish_time_micros / MICROS_TO_MILLIS_UNIT;
    scroll_end_utc =
        back_lastest_target_time / MICROS_TO_MILLIS_UNIT + offset_ms;
  }

  OH_HiAppEvent_AddStringParam(list, "frameworkName", "FLUTTER");
  OH_HiAppEvent_AddInt32Param(list, "versionCode", 0);

  // Start time (unit: ms)
  OH_HiAppEvent_AddInt64Param(list, "startTime", scroll_start_utc);
  // End time (unit: ms)
  OH_HiAppEvent_AddInt64Param(list, "endTime", scroll_end_utc);
  // Maximum dropped frame duration (unit: ms)
  OH_HiAppEvent_AddInt64Param(list, "maxFrameTime",
                              max_frame_time / MICROS_TO_MILLIS_UNIT);
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

  int ret =  // Event tracking
      OH_HiAppEvent_Write("PERFORMANCE", "OTHER_JANK_SCROLL", BEHAVIOR, list);
  if (ret != 0) {
    FML_LOG(ERROR) << "OH_HiAppEvent_Write() error, ret = " << ret;
  }

  // Reset scroll start and end frame IDs
  scroll_start_frame_.store(0, std::memory_order_relaxed);
  scroll_end_frame_.store(0, std::memory_order_relaxed);

  scroll_start_time_utc_ms.store(0, std::memory_order_relaxed);
  scroll_end_time_utc_ms.store(0, std::memory_order_relaxed);

  OH_HiAppEvent_DestroyParamList(list);
  return ret;
}

void OhosHiappEventDDL::WriteJANKEventToTrace(
    const MissedFrameInfo& missed_frame_info,
    OhosDropFrameReason reason) {
  auto frame_number = missed_frame_info.frame_number;
  auto vsync_transitions_missed = missed_frame_info.vsync_transitions_missed;
  auto drop_duration_micros = missed_frame_info.frame_duration_micros;
  double drop_duration_millis = drop_duration_micros / 1000.0;

  char buffer[TRACE_ARGS_BUFFER_SIZE];
  const char* reasonStr =
      (reason == OhosDropFrameReason::kScroll ? "scroll" : "common");
  int written =
      std::snprintf(buffer, TRACE_ARGS_BUFFER_SIZE,
                    "frame_number=%lu,dropped=%d,duration=%.2lfms,reason=%s",
                    static_cast<unsigned long>(frame_number),
                    vsync_transitions_missed, drop_duration_millis, reasonStr);
  if (written < 0 || written >= TRACE_ARGS_BUFFER_SIZE) {
    buffer[TRACE_ARGS_BUFFER_SIZE - 1] = '\0';
  }

  OH_HiTrace_CountTrace(COUNTER_FRAME_DROP, vsync_transitions_missed);
  if (startAsyncTraceExFunc_ && finishAsyncTraceExFunc_) {
    startAsyncTraceExFunc_(HITRACE_LEVEL_INFO, FRAME_DROP_DURATION,
                           static_cast<int32_t>(frame_number), "", buffer);
    finishAsyncTraceExFunc_(HITRACE_LEVEL_INFO, FRAME_DROP_DURATION,
                            static_cast<int32_t>(frame_number));
  }
}

void OhosHiappEventDDL::Flush(void) {
  FlushAllIn(OhosHiappEventFlag::kSingleFlag);
  FlushAllIn(OhosHiappEventFlag::kStaticFlag);

  missed_frame_infos.clear();
}

void OhosHiappEventDDL::FlushScroll(void) {
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
      setReportEventFunc_(processor, "PERFORMANCE", HIAPPEVENT_OTHER_JANK,
                          true);
      break;
    case OhosHiappEventFlag::kStaticFlag:
      setReportEventFunc_(processor, "PERFORMANCE", HIAPPEVENT_OTHER_JANK_STAT,
                          true);
      break;
    case OhosHiappEventFlag::kScrolledFlag:
      setReportEventFunc_(processor, "PERFORMANCE",
                          HIAPPEVENT_OTHER_JANK_SCROLL, true);
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

  const uint64_t cur_frame_number =
      last_frame_number_.load(std::memory_order_relaxed);
  scroll_start_frame_.store(cur_frame_number, std::memory_order_relaxed);
  // Init end = start, so totalFrames is at least 1 if we flush immediately.
  scroll_end_frame_.store(cur_frame_number, std::memory_order_relaxed);

  // Record scroll start UTC time
  auto now = std::chrono::system_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch());
  scroll_start_time_utc_ms.store(static_cast<int64_t>(duration.count()),
                                 std::memory_order_relaxed);
  // Reset scroll end time
  scroll_end_time_utc_ms.store(0, std::memory_order_relaxed);
}

void OhosHiappEventDDL::OnScrollEndAndFlush() {
  ScrollStatus.store(static_cast<int>(ScrollingStatus::kScrollEnd),
                     std::memory_order_relaxed);

  // Snapshot end from last seen raster frame.
  const uint64_t end = last_frame_number_.load(std::memory_order_relaxed);
  scroll_end_frame_.store(end, std::memory_order_relaxed);

  // Record scroll end UTC time
  auto now = std::chrono::system_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch());
  scroll_end_time_utc_ms.store(static_cast<int64_t>(duration.count()),
                               std::memory_order_relaxed);

  // Increment scroll count for every scroll session
  recent_scroll_count++;

  // Now flush AFTER we have end frame id.
  FlushScroll();
}

void OhosHiappEventDDL::ReportMemoryUsage(int64_t oldUsed, int64_t newUsed) {
  if (reportFrameworkMemAnomaly_ == nullptr) {
    FML_LOG(WARNING) << "reportFrameworkMemAnomaly_ is nullptr, cannot report "
                        "memory anomaly";
    return;
  }

  auto version = GetFlutterVersion();
  int64_t totalUsed = oldUsed + newUsed;
  int64_t totalMB = totalUsed / (1024 * 1024);
  int64_t oldMB = oldUsed / (1024 * 1024);
  int64_t newMB = newUsed / (1024 * 1024);

  std::string desc = "Dart heap memory usage exceeds threshold: total = " +
                     std::to_string(totalMB) +
                     " MB (old = " + std::to_string(oldMB) +
                     " MB, new = " + std::to_string(newMB) + " MB)";

  FML_LOG(WARNING) << desc;
  reportFrameworkMemAnomaly_(K_FLUTTER_DART_FRAMEWORK_TYPE, version,
                             desc.c_str());
}

};  // namespace hiappevent

};  // namespace fml
