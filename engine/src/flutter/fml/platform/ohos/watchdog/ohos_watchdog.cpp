/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */
#include "ohos_watchdog.h"
#include <hicollie/hicollie.h>
#include <functional>
#include <mutex>

#include "flutter/fml/memory/weak_ptr.h"
#include "flutter/fml/logging.h"
#include "flutter/fml/trace_event.h"

namespace fml {

namespace OhosWatchdog {

constexpr int resetRatio = 2;
constexpr std::chrono::seconds checkIntervalTime{3};

class FlutterWatchdog {
 public:
  explicit FlutterWatchdog(fml::RefPtr<fml::TaskRunner> ui);

  void runHiCollieStuckDetectionTask();
  void handleFlutterUiThreadAliveNotification();
  void reportStuckEvent();

 private:
  bool m_flutter_ui_thread_is_alive = false;
  bool m_need_report = true;
  bool m_is_six_second_event = false;

  std::chrono::steady_clock::time_point m_lastWatchTime;
  fml::RefPtr<fml::TaskRunner> m_ui;
  fml::WeakPtrFactory<FlutterWatchdog> weak_factory_;
};

FlutterWatchdog::FlutterWatchdog(fml::RefPtr<fml::TaskRunner> ui)
    : m_ui(std::move(ui)), weak_factory_(this) {
  FML_DLOG(INFO) << "FlutterWatchdog::FlutterWatchdog Thread Id = " <<m_ui->GetTaskQueueId();
}

void FlutterWatchdog::handleFlutterUiThreadAliveNotification() {
  FML_DLOG(INFO)
      << "FlutterWatchdog::handleFlutterUiThreadAliveNotification entry";
  TRACE_EVENT_INSTANT0("flutter", "handleFlutterUiThreadAliveNotification");
  m_flutter_ui_thread_is_alive = true;
  m_need_report = true;
  m_is_six_second_event = false;
}

void FlutterWatchdog::runHiCollieStuckDetectionTask() {
  TRACE_EVENT_INSTANT0("flutter", "runHiCollieStuckDetectionTask");
  if (!m_need_report) {
    return;
  }

  if (m_flutter_ui_thread_is_alive) {
    m_flutter_ui_thread_is_alive = false;
  } else {
    FML_LOG(ERROR) << "FlutterWatchdog: FlutterUiThread is not alive";
    reportStuckEvent();
  }

  fml::TaskRunner::RunNowOrPostTask(
      m_ui, [self = weak_factory_.GetWeakPtr()]() {
        if (!self) {
          return;
        }
        self->handleFlutterUiThreadAliveNotification();
      });

  auto now = std::chrono::steady_clock::now();
  if ((now - m_lastWatchTime) >= (checkIntervalTime / resetRatio)) {
    m_lastWatchTime = now;
  }

  return;
}

void FlutterWatchdog::reportStuckEvent() {
  auto now = std::chrono::steady_clock::now();
  auto timeDelta = now - m_lastWatchTime;
  //假设timeDelta大于6s或小于1.5s则表示上报出现异常，会重新进行上报
  if (timeDelta / resetRatio > checkIntervalTime ||
      timeDelta < checkIntervalTime / resetRatio) {
    FML_LOG(ERROR)
        << "FlutterWatchdog: Thread may be blocked, do not report this "
           "time."
        << "currTime: "
        << std::chrono::duration<double>(now.time_since_epoch()).count()
        << ", lastTime: "
        << std::chrono::duration<double>(m_lastWatchTime.time_since_epoch())
               .count()
        << "Thread Id:" 
        << m_ui->GetTaskQueueId();
    return;
  }

  //m_need_report不为true时代表应用已被杀死，则不需要再上报
  if (!m_need_report) {
    return;
  }

  if (m_is_six_second_event) {
    m_need_report = false;
  }

  FML_LOG(WARNING) << "FlutterWatchdog: calling OH_HiCollie_Report(), "
                    "m_issSixSecondEvent = "
                 << m_is_six_second_event;

  // 如果卡住6秒，则为true。如果卡住3秒，则为False。此时传入的m_is_six_second_event若为false，传入OH_HiCollie_Report接口后
  // 会将m_is_six_second_event置为true，传入为true时则会进行卡死上报。
  HiCollie_ErrorCode reportRes = OH_HiCollie_Report(&m_is_six_second_event);
  if (reportRes == HICOLLIE_SUCCESS) {
    FML_LOG(WARNING) << "FlutterWatchdog: after OH_HiCollie_Report(), "
                      "m_issixsecondevent = "
                   << m_is_six_second_event;
  } else {
    FML_LOG(ERROR) << "FlutterWatchdog: OH_HiCollie_Report() failed with code "
                   << static_cast<int>(reportRes);
  }
}

size_t RunWithFlutterWatchdogSharedPtr(
    const std::function<void(std::vector<std::shared_ptr<FlutterWatchdog>>&)>&
        runFunc) {
  static std::mutex runMutex;
  static std::vector<std::shared_ptr<FlutterWatchdog>> flutterWatchdogVec;

  {
    std::lock_guard<std::mutex> runLock(runMutex);
    runFunc(flutterWatchdogVec);
    return flutterWatchdogVec.size();
  }
}

std::pair<size_t, std::function<void(size_t)>> MakeWatchdog(
    const fml::RefPtr<fml::TaskRunner>& ui) {

  // 创建指向FlutterWatchdog对象的智能指针，并返回其所在vector中的下标
  size_t curSize =
      RunWithFlutterWatchdogSharedPtr([ui](auto& flutterWatchdogVec) {
        flutterWatchdogVec.emplace_back(std::make_shared<FlutterWatchdog>(ui));
      });

  // 获取flutterWatchdogVec中最后一个元素，即当前新建的FlutterWatchdog对象的智能指针
  // 执行runHiCollieStuckDetectionTask方法，开启看门狗
  HiCollie_ErrorCode structDetectionInitRes =
      OH_HiCollie_Init_StuckDetection([]() {
        RunWithFlutterWatchdogSharedPtr([](auto& flutterWatchdogVec) {
          if (flutterWatchdogVec.empty()) {
            return;
          }

          if (flutterWatchdogVec.back()) {
            flutterWatchdogVec.back()->runHiCollieStuckDetectionTask();
          }
        });
      });

  std::function<void(size_t)> handlePtr;
  if (structDetectionInitRes == HICOLLIE_SUCCESS) {
    handlePtr = std::function<void(size_t)>([](size_t index) {
      RunWithFlutterWatchdogSharedPtr([index](auto& flutterWatchdogVec) {
        if (flutterWatchdogVec.size() < index) {
          return;
        }
        flutterWatchdogVec[index].reset();
      });
    });

  } else {
    RunWithFlutterWatchdogSharedPtr([curSize](auto& flutterWatchdogVec) {
      if (flutterWatchdogVec.size() < curSize) {
        return;
      }
      flutterWatchdogVec[curSize].reset();
    });
    FML_LOG(ERROR) << "OH_HiCollie_Init_stuckDetection() failed with code "
                   << static_cast<int>(structDetectionInitRes);

    curSize = 0;
  }

  return make_pair(curSize, handlePtr);
}

}  // namespace OhosWatchdog
}  // namespace fml
