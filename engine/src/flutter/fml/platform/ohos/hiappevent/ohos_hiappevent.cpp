/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */
#define STATISTICTYPE "OTHER_JANK_STAT"
#define SINGLETYPE "OTHER_JANK"
#define STATISICFLAG 3
#define SINLEFLAG 2


#include "ohos_hiappevent.h"

#include <deviceinfo.h>
#include <dlfcn.h>
#include <unistd.h>
#include <map>
#include <thread>
#include "flutter/fml/logging.h"
#include "flutter/fml/platform/ohos/dynamic_library_loader.h"

namespace fml {

namespace hiappevent {

static std::shared_ptr<OhosHiappEventDDL> instance_ = nullptr;
static std::once_flag instanceFlag_;

static constexpr char HIAPPEVENT_LIB_NAME[] = "libhiappevent_ndk.z.so";
static const int MISSED_FRAME_INFOS_SIZE = 10;
static const int REQUIRED_API_VERSION = 18;
static const int ARGUMENT_SIZE = 3;
static const int VSYNC_TRANSITIONS_MISSED_SIZE = 2;

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
  if (MissedFrameInfos.size() == 0) {
    return -1;
  }

  int64_t endTimeMicros = MissedFrameInfos.front().endTimeMicros;
  int64_t targetTime = MissedFrameInfos.front().targetTime / 1000;
  int64_t lastestTargetTime = MissedFrameInfos.front().lastestTargetTime / 1000;
  int missedFrame = MissedFrameInfos.front().missedFrame;

  if (endTimeMicros < targetTime) {
    FML_LOG(ERROR) << "report error, endTime is less than targetTime";
    return -1;
  }

  int64_t budgetTime = (lastestTargetTime - targetTime) / missedFrame;
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

void OhosHiappEventDDL::Flush(void) {
  Init();

  FlushAllIn(SINLEFLAG);
  FlushAllIn(STATISICFLAG);

  MissedFrameInfos.clear();
}

void OhosHiappEventDDL::FlushAllIn(int type) {
  if (!isValid_) {
    FML_LOG(ERROR) << "flush isValid_ false";
    return;
  }

  if (MissedFrameInfos.size() == 0) {
    return;
  }

  if (type != STATISICFLAG && type != SINLEFLAG) {
    return;
  }

  HiAppEvent_Processor* processor = reinterpret_cast<HiAppEvent_Processor*>(
      createProcessorFunc_("xperfbridge"));
  if (processor == nullptr) {
    FML_LOG(ERROR) << "processor == nullptr";
    return;
  }

  setReportPoliceFunc_(processor, 1, 1, true, true);
  if (type == SINLEFLAG) {
    setReportEventFunc_(processor, "PERFORMANCE", SINGLETYPE, true);
  } else {
    setReportEventFunc_(processor, "PERFORMANCE", STATISTICTYPE, true);
  }

  int64_t processorId = addFunc_(processor);
  if (processorId <= 0) {
    FML_LOG(ERROR) << "processorId error";
    destroyProcessor_(processor);
    return;
  }

  int ret = -1;
  switch (type) {
    case SINLEFLAG:
      ret = WriteSingleFrame();
      break;
    case STATISICFLAG:
      ret = WriteStatisticFrame();
      break;
    default:
      break;
  }

  destroyProcessor_(processor);
  if (ret != 0) {
    FML_LOG(ERROR) << "flush error: type = " << type;
  }
  return;
}

};  // namespace hiappevent

};  // namespace fml
