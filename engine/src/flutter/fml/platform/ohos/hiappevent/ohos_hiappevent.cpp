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

namespace fml {

namespace hiappevent {

static std::shared_ptr<OhosHiappEventDDL> instance_ = nullptr;
static std::once_flag instanceFlag_;

static constexpr char HIAPPEVENT_LIB_NAME[] = "libhiappevent_ndk.z.so";

static constexpr char HIAPPEVENT_OTHER_JANK[] = "OTHER_JANK";
static constexpr char HIAPPEVENT_OTHER_JANK_STAT[] = "OTHER_JANK_STAT";
static constexpr char HIAPPEVENT_OTHER_JANK_SCROLL[] = "OTHER_JANK_SCROLL";
static constexpr int64_t kScrollJankThresholdUs = 50 * 1000; // 丢帧超过50ms才上报

static const int MISSED_FRAME_INFOS_SIZE = 10;
static const int REQUIRED_API_VERSION = 18;
static const int ARGUMENT_SIZE = 3;
static const int VSYNC_TRANSITIONS_MISSED_SIZE = 2;

static int recentScrollCount = 0; // 新增成员 上次丢帧到本次丢帧发生的滑动次数

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

void OhosHiappEventDDL::ReportScrollJANKEvent(int64_t endTimeMicros,
                                        const char** argumentValues,
                                        int argumentCount) {
  if (argumentCount < ARGUMENT_SIZE) {
    FML_LOG(ERROR) << "Array data overflow";
    return;
  }

  FML_LOG(ERROR) << "kemin ReportJANKEvent, endTimeMicros = " << endTimeMicros;

  /*  argumentValues
      [0]:frame_target_time
      [1]:current_frame_target_time
      [2]:vsync_transitions_missed
   */

  // TODO: 重新获取UTC时间，并填入到 endTimeMicros targetTime lastestTargetTime

  MissedFrameInfo info;
  info.endTimeMicros = endTimeMicros;
  info.targetTime = std::stoll(argumentValues[0]);
  info.lastestTargetTime = std::stoll(argumentValues[1]);
  info.missedFrame = std::stoi(argumentValues[VSYNC_TRANSITIONS_MISSED_SIZE]);

  // 认为低于丢帧时长阈值的丢帧事件不属于丢帧，故不上报

  // 计算单个 vsync 的时间（us）
  int64_t budgetTime =
      (info.lastestTargetTime - info.targetTime) / info.missedFrame;
  budgetTime /= 1000;  // ns -> us

  // 该帧理论上的 vsync 开始时间（us）
  int64_t vsyncStartTime =
      (info.targetTime / 1000) - budgetTime;

  // 实际帧耗时（us）
  int64_t frameCost = info.endTimeMicros - vsyncStartTime;

  // 50ms 阈值判定
  if (frameCost < kScrollJankThresholdUs) {
    FML_LOG(INFO)
        << "Ignore scroll jank: frameCost="
        << frameCost << "us (<50ms)";
    return;
  }

  MissedFrameInfosScroll.push_back(info);
}

void OhosHiappEventDDL::ReportJANKEvent(int64_t endTimeMicros,
                                        const char** argumentValues,
                                        int argumentCount) {
  if (argumentCount < ARGUMENT_SIZE) {
    FML_LOG(ERROR) << "Array data overflow";
    return;
  }

  if (MissedFrameInfos.size() == MISSED_FRAME_INFOS_SIZE) {
    // MissedFrameInfos is full.
    FML_LOG(INFO) << "vector stop push_back";
    return;
  } else if (MissedFrameInfos.size() > MISSED_FRAME_INFOS_SIZE) {
    return;
  }

  /*  argumentValues
      [0]:frame_target_time
      [1]:current_frame_target_time
      [2]:vsync_transitions_missed
   */
  MissedFrameInfo info;
  info.endTimeMicros = endTimeMicros;
  info.targetTime = std::stoll(argumentValues[0]);
  info.lastestTargetTime = std::stoll(argumentValues[1]);
  info.missedFrame = std::stoi(argumentValues[VSYNC_TRANSITIONS_MISSED_SIZE]);
  MissedFrameInfos.push_back(info);
}

int OhosHiappEventDDL::WriteSingleFrame(void) {
  // if (MissedFrameInfos.size() == 0) {
  //   return -1;
  // }

  // int64_t endTimeMicros = MissedFrameInfos.front().endTimeMicros;
  // int64_t targetTime = MissedFrameInfos.front().targetTime / 1000;
  // int64_t lastestTargetTime = MissedFrameInfos.front().lastestTargetTime / 1000;
  // int missedFrame = MissedFrameInfos.front().missedFrame;

  // kemin
  FML_LOG(ERROR) << "kemin WriteSingleFrame entry";
  if (MissedFrameInfosScroll.size() == 0) {
    return -1;
  }

  int64_t endTimeMicros = MissedFrameInfosScroll.front().endTimeMicros;
  int64_t targetTime = MissedFrameInfosScroll.front().targetTime / 1000;
  int64_t lastestTargetTime = MissedFrameInfosScroll.front().lastestTargetTime / 1000;
  int missedFrame = MissedFrameInfosScroll.front().missedFrame;

  FML_LOG(ERROR) << "kemin WriteSingleFrame entry";

  if (endTimeMicros < targetTime) {
    FML_LOG(ERROR) << "report error, endTime is less than targetTime";
    return -1;
  }

  int64_t budgetTime = (lastestTargetTime - targetTime) / missedFrame; // The duration of a vsync interval
  int64_t vsyncStartTime = targetTime - budgetTime;
  ParamList list = OH_HiAppEvent_CreateParamList();
  if (list == nullptr) {
    FML_LOG(ERROR) << "CreateParamList error";
    return -1;
  }

  OH_HiAppEvent_AddStringParam(list, "frameworkName", "FLUTTER");
  OH_HiAppEvent_AddInt32Param(list, "versionCode", 0);
  OH_HiAppEvent_AddInt32Param(list, "missedFrames", missedFrame);
  OH_HiAppEvent_AddInt64Param(list, "startTime", vsyncStartTime);
  OH_HiAppEvent_AddInt64Param(list, "endTime", endTimeMicros);
  OH_HiAppEvent_AddInt64Param(list, "pid", getpid());

  int ret = OH_HiAppEvent_Write("PERFORMANCE", "OTHER_JANK", BEHAVIOR, list);
  if (ret != 0) {
    FML_LOG(ERROR) << "HiAppEvent_Write error, ret = " << ret;
  }

  OH_HiAppEvent_DestroyParamList(list);
  return ret;
}

int OhosHiappEventDDL::WriteStatisticFrame(void) {
  if (MissedFrameInfos.size() == 0) {
    FML_LOG(ERROR) << "size of MissedFrameInfos is zero";
    return -1;
  }

  ParamList list = OH_HiAppEvent_CreateParamList();
  if (list == nullptr) {
    FML_LOG(ERROR) << "CreateParamList error";
    return -1;
  }

  int totalMissedFrames = 0;
  int maxMissedFrame = 0;
  int targetIndex = 0;
  int index = 0;
  for (auto it = MissedFrameInfos.begin(); it != MissedFrameInfos.end(); it++) {
    totalMissedFrames += (*it).missedFrame;

    if ((*it).missedFrame > maxMissedFrame) {
      maxMissedFrame = (*it).missedFrame;
      targetIndex = index;
    }
    index++;
  }
  // 丢帧最大值
  int64_t maxEndTimeMicros = MissedFrameInfos.at(targetIndex).endTimeMicros;
  int64_t maxTargetTime = MissedFrameInfos.at(targetIndex).targetTime / 1000;
  int64_t maxLastestTargetTime =
      MissedFrameInfos.at(targetIndex).lastestTargetTime / 1000;

  int64_t maxBudget = (maxLastestTargetTime - maxTargetTime) / maxMissedFrame;
  int64_t maxVsyncStartTime = maxTargetTime - maxBudget;
  int64_t maxDiffTime = maxEndTimeMicros - maxVsyncStartTime;
  int maxFPS = 1000000 / maxBudget;

  // 开始时间 第一次丢帧vsync开始时间
  int64_t frontTargetTime = MissedFrameInfos.front().targetTime / 1000;
  int64_t frontLastestTargetTime =
      MissedFrameInfos.front().lastestTargetTime / 1000;
  int64_t frontbudgetTime = (frontLastestTargetTime - frontTargetTime) /
                            MissedFrameInfos.front().missedFrame;
  int64_t vsyncStartTime = frontTargetTime - frontbudgetTime;

  // 结束时间 最后一次丢帧vsync结束时间
  int64_t backLastestTargetTime =
      MissedFrameInfos.back().lastestTargetTime / 1000;

  OH_HiAppEvent_AddStringParam(list, "frameworkName", "FLUTTER");
  OH_HiAppEvent_AddInt32Param(list, "versionCode", 0);

  OH_HiAppEvent_AddInt32Param(list, "maxMissedFrameRate", maxFPS);
  OH_HiAppEvent_AddInt32Param(list, "totalMissedFrames", totalMissedFrames);
  OH_HiAppEvent_AddInt64Param(list, "maxFrameTime", maxDiffTime);
  OH_HiAppEvent_AddInt64Param(list, "startTime", vsyncStartTime);
  OH_HiAppEvent_AddInt64Param(list, "endTime", backLastestTargetTime);
  OH_HiAppEvent_AddInt64Param(list, "pid", getpid());

  int ret =
      OH_HiAppEvent_Write("PERFORMANCE", "OTHER_JANK_STAT", STATISTIC, list);
  if (ret != 0) {
    FML_LOG(ERROR) << "HiAppEvent_Write error, ret = " << ret;
  }

  OH_HiAppEvent_DestroyParamList(list);
  return ret;
}

int OhosHiappEventDDL::WriteScrolledFrame(void) {
  if (MissedFrameInfosScroll.size() == 0) {
    FML_LOG(ERROR) << "size of MissedFrameInfosScroll is zero";
    return -1;
  }

  ParamList list = OH_HiAppEvent_CreateParamList(); // 创建参数列表指针
  if (list == nullptr) {
    FML_LOG(ERROR) << "CreateParamList error";
    return -1;
  }

  // 总丢帧个数
  int totalMissedFrames = 0;

  // 最大丢帧数，及对应下标
  int maxMissedFrame = 0;
  int targetIndex = 0;

  int index = 0;
  for (auto it = MissedFrameInfosScroll.begin(); it != MissedFrameInfosScroll.end(); it++) {
    totalMissedFrames += (*it).missedFrame;

    if ((*it).missedFrame > maxMissedFrame) {
      maxMissedFrame = (*it).missedFrame;
      targetIndex = index;
    }
    index++;
  }

  // 最大丢帧的目标完成时间
  int64_t maxTargetTime = MissedFrameInfosScroll.at(targetIndex).targetTime / 1000;

  // 最大丢帧的下一帧目标完成时间
  int64_t maxLastestTargetTime =
      MissedFrameInfosScroll.at(targetIndex).lastestTargetTime / 1000;

  // 最大丢帧发生时的帧间隔
  int64_t maxBudget = (maxLastestTargetTime - maxTargetTime) / maxMissedFrame;

  // 最大丢帧的帧开始时间
  int64_t maxVsyncStartTime = maxTargetTime - maxBudget;

  // 最大丢帧的实际结束时间
  int64_t maxEndTimeMicros = MissedFrameInfosScroll.at(targetIndex).endTimeMicros;

  // 最大丢帧时长
  int64_t maxDiffTime = maxEndTimeMicros - maxVsyncStartTime;

  // 最大丢帧发生时的帧率
  int maxFPS = 1000000 / maxBudget;

  // 第一次丢帧情况
  int64_t frontTargetTime = MissedFrameInfosScroll.front().targetTime / 1000;
  int64_t frontLastestTargetTime =
      MissedFrameInfosScroll.front().lastestTargetTime / 1000;
  int64_t frontbudgetTime = (frontLastestTargetTime - frontTargetTime) /
                            MissedFrameInfosScroll.front().missedFrame;
  int64_t vsyncStartTime = frontTargetTime - frontbudgetTime; // 第一次丢帧vsync开始时间


  // 结束时间 最后一次丢帧vsync结束时间
  int64_t backLastestTargetTime =
      MissedFrameInfosScroll.back().lastestTargetTime / 1000;

  OH_HiAppEvent_AddStringParam(list, "frameworkName", "FLUTTER");
  OH_HiAppEvent_AddInt32Param(list, "versionCode", 0);

  // TODO：时间转为UTC时间

  // 开始时间(单位：us)
  OH_HiAppEvent_AddInt64Param(list, "startTime", vsyncStartTime);
  // 结束时间(单位：us)
  OH_HiAppEvent_AddInt64Param(list, "endTime", backLastestTargetTime);
  // 最大丢帧时长
  OH_HiAppEvent_AddInt64Param(list, "maxFrameTime", maxDiffTime);
  // 最长帧发生时刻的帧率
  OH_HiAppEvent_AddInt32Param(list, "maxMissedFrameRate", maxFPS);
  // 总丢帧个数
  OH_HiAppEvent_AddInt32Param(list, "totalMissedFrames", totalMissedFrames);
  // 总帧数
  OH_HiAppEvent_AddInt32Param(list, "totalFrames", 1); // kemin 漏了，后面补充
  // 距离上次上报的滑动次数
  OH_HiAppEvent_AddInt32Param(list, "recentScrollCount", recentScrollCount);
  // 进程ID
  OH_HiAppEvent_AddInt64Param(list, "pid", getpid());

  int ret = // 执行事件打点
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

// 判断是否丢过帧，若丢过滑动次数+1
  if (MissedFrameInfosScroll.size() != 0) {
    recentScrollCount++;
  } 

  FlushAllIn(OhosHiappEventFlag::kScrolledFlag);

 // 滑动丢帧上报结束，重置滑动次数
  recentScrollCount = 0;
  MissedFrameInfosScroll.clear();
}

void OhosHiappEventDDL::FlushAllIn(OhosHiappEventFlag type) {
  if (!isValid_) {
    FML_LOG(ERROR) << "flush isValid_ false";
    return;
  }

  if (MissedFrameInfos.size() == 0) {
    return;
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

};  // namespace hiappevent

};  // namespace fml
