// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <cstring>
#include <iostream>

#include "flutter/fml/build_config.h"
#include "flutter/fml/log_level.h"
#include "flutter/fml/log_settings.h"
#include "flutter/fml/logging.h"

#if defined(FML_OS_ANDROID)
#include <android/log.h>
#elif defined(FML_OS_IOS)
#include <syslog.h>
#elif defined(OS_FUCHSIA)
#include <lib/syslog/structured_backend/cpp/fuchsia_syslog.h>
#include <lib/syslog/structured_backend/fuchsia_syslog.h>
#include <zircon/process.h>
#include "flutter/fml/platform/fuchsia/log_state.h"
#endif

#ifdef FML_OS_OHOS
#include <pthread.h>

extern "C" {
#define OHOS_LOG_TYPE_APP 0
#define HILOG_LOG_DOMAIN 0
#define HILOG_LOG_TAG "XComFlutterEngine"
typedef enum {
  /** Debug level to be used by {@link OH_LOG_DEBUG} */
  HILOG_LOG_DEBUG = 3,
  /** Informational level to be used by {@link OH_LOG_INFO} */
  HILOG_LOG_INFO = 4,
  /** Warning level to be used by {@link OH_LOG_WARN} */
  HILOG_LOG_WARN = 5,
  /** Error level to be used by {@link OH_LOG_ERROR} */
  HILOG_LOG_ERROR = 6,
  /** Fatal level to be used by {@link OH_LOG_FATAL} */
  HILOG_LOG_FATAL = 7,
} HiLog_LogLevel;
int OH_LOG_Print(int type,
                 HiLog_LogLevel level,
                 unsigned int domain,
                 const char* tag,
                 const char* fmt,
                 ...);
#define HILOG_LOG(level, ...)                                       \
  ((void)OH_LOG_Print(OHOS_LOG_TYPE_APP, (level), HILOG_LOG_DOMAIN, \
                      HILOG_LOG_TAG, __VA_ARGS__))
#define HILOG_DEBUG(...)                                                    \
  ((void)OH_LOG_Print(OHOS_LOG_TYPE_APP, HILOG_LOG_DEBUG, HILOG_LOG_DOMAIN, \
                      HILOG_LOG_TAG, __VA_ARGS__))
#define HILOG_ERROR(...)                                                    \
  ((void)OH_LOG_Print(OHOS_LOG_TYPE_APP, HILOG_LOG_ERROR, HILOG_LOG_DOMAIN, \
                      HILOG_LOG_TAG, __VA_ARGS__))
#define HILOG_INFO(...)                                                    \
  ((void)OH_LOG_Print(OHOS_LOG_TYPE_APP, HILOG_LOG_INFO, HILOG_LOG_DOMAIN, \
                      HILOG_LOG_TAG, __VA_ARGS__))
}
#endif
namespace fml {

namespace {

#if !defined(OS_FUCHSIA)
const char* const kLogSeverityNames[kLogNumSeverities] = {
    "INFO", "WARNING", "ERROR", "IMPORTANT", "FATAL"};

const char* GetNameForLogSeverity(LogSeverity severity) {
  if (severity >= kLogInfo && severity < kLogNumSeverities) {
    return kLogSeverityNames[severity];
  }
  return "UNKNOWN";
}
#endif

const char* StripDots(const char* path) {
  while (strncmp(path, "../", 3) == 0) {
    path += 3;
  }
  return path;
}

#if defined(OS_FUCHSIA)

zx_koid_t GetKoid(zx_handle_t handle) {
  zx_info_handle_basic_t info;
  zx_status_t status = zx_object_get_info(handle, ZX_INFO_HANDLE_BASIC, &info,
                                          sizeof(info), nullptr, nullptr);
  return status == ZX_OK ? info.koid : ZX_KOID_INVALID;
}

thread_local zx_koid_t tls_thread_koid{ZX_KOID_INVALID};

zx_koid_t GetCurrentThreadKoid() {
  if (unlikely(tls_thread_koid == ZX_KOID_INVALID)) {
    tls_thread_koid = GetKoid(zx_thread_self());
  }
  ZX_DEBUG_ASSERT(tls_thread_koid != ZX_KOID_INVALID);
  return tls_thread_koid;
}

static zx_koid_t pid = GetKoid(zx_process_self());

static thread_local zx_koid_t tid = GetCurrentThreadKoid();

std::string GetProcessName(zx_handle_t handle) {
  char process_name[ZX_MAX_NAME_LEN];
  zx_status_t status = zx_object_get_property(
      handle, ZX_PROP_NAME, &process_name, sizeof(process_name));
  if (status != ZX_OK) {
    process_name[0] = '\0';
  }
  return process_name;
}

static std::string process_name = GetProcessName(zx_process_self());

static const zx::socket& socket = LogState::Default().socket();

#endif

}  // namespace

LogMessage::LogMessage(LogSeverity severity,
                       const char* file,
                       int line,
                       const char* condition)
    : severity_(severity), file_(StripDots(file)), line_(line) {
#if !defined(OS_FUCHSIA)
  stream_ << "[";
  if (severity >= kLogInfo) {
    stream_ << GetNameForLogSeverity(severity);
  } else {
    stream_ << "VERBOSE" << -severity;
  }
  stream_ << ":" << file_ << "(" << line_ << ")] ";
#endif

  if (condition) {
    stream_ << "Check failed: " << condition << ". ";
  }
}

// static
thread_local std::ostringstream* LogMessage::capture_next_log_stream_ = nullptr;

namespace testing {

LogCapture::LogCapture() {
  fml::LogMessage::CaptureNextLog(&stream_);
}

LogCapture::~LogCapture() {
  fml::LogMessage::CaptureNextLog(nullptr);
}

std::string LogCapture::str() const {
  return stream_.str();
}

}  // namespace testing

// static
void LogMessage::CaptureNextLog(std::ostringstream* stream) {
  LogMessage::capture_next_log_stream_ = stream;
}

LogMessage::~LogMessage() {
#if !defined(OS_FUCHSIA)
  stream_ << std::endl;
#endif
  if (capture_next_log_stream_) {
    *capture_next_log_stream_ << stream_.str();
    capture_next_log_stream_ = nullptr;
  } else {
#if defined(FML_OS_ANDROID)
    android_LogPriority priority =
        (severity_ < 0) ? ANDROID_LOG_VERBOSE : ANDROID_LOG_UNKNOWN;
    switch (severity_) {
      case kLogImportant:
      case kLogInfo:
        priority = ANDROID_LOG_INFO;
        break;
      case kLogWarning:
        priority = ANDROID_LOG_WARN;
        break;
      case kLogError:
        priority = ANDROID_LOG_ERROR;
        break;
      case kLogFatal:
        priority = ANDROID_LOG_FATAL;
        break;
    }
    __android_log_write(priority, "flutter", stream_.str().c_str());
#elif defined(FML_OS_IOS)
    syslog(LOG_ALERT, "%s", stream_.str().c_str());
#elif defined(OS_FUCHSIA)
    FuchsiaLogSeverity severity;
    switch (severity_) {
      case kLogImportant:
      case kLogInfo:
        severity = FUCHSIA_LOG_INFO;
        break;
      case kLogWarning:
        severity = FUCHSIA_LOG_WARNING;
        break;
      case kLogError:
        severity = FUCHSIA_LOG_ERROR;
        break;
      case kLogFatal:
        severity = FUCHSIA_LOG_FATAL;
        break;
      default:
        if (severity_ < 0) {
          severity = FUCHSIA_LOG_DEBUG;
        } else {
          // Unknown severity. Use INFO.
          severity = FUCHSIA_LOG_INFO;
        }
        break;
    }
    fuchsia_syslog::LogBuffer buffer;
    buffer.BeginRecord(severity, std::string_view(file_), line_,
                       std::string_view(stream_.str()), socket.borrow(), 0, pid,
                       tid);
    if (!process_name.empty()) {
      buffer.WriteKeyValue("tag", process_name);
    }
    if (auto tags_ptr = LogState::Default().tags()) {
      for (auto& tag : *tags_ptr) {
        buffer.WriteKeyValue("tag", tag);
      }
    }
    buffer.FlushRecord();
#elif defined(FML_OS_OHOS)
  HiLog_LogLevel fx_severity;

  switch (severity_) {
    case LOG_INFO:
      fx_severity = HILOG_LOG_INFO;
      break;
    case LOG_WARNING:
      fx_severity = HILOG_LOG_WARN;
      break;
    case LOG_ERROR:
      fx_severity = HILOG_LOG_ERROR;
      break;
    case LOG_FATAL:
      fx_severity = HILOG_LOG_FATAL;
      break;
    default:
      fx_severity = HILOG_LOG_INFO;
  }  // end switch
  HILOG_LOG(fx_severity, "Thread:%{public}lu  %{public}s", pthread_self(),
            stream_.str().c_str());

  std::cerr << stream_.str();
  std::cerr.flush();
   
#else
    // Don't use std::cerr here, because it may not be initialized properly yet.
    fprintf(stderr, "%s", stream_.str().c_str());
    fflush(stderr);
#endif
  }

  if (severity_ >= kLogFatal) {
    KillProcess();
  }
}

int GetVlogVerbosity() {
  return std::max(-1, kLogInfo - GetMinLogLevel());
}

bool ShouldCreateLogMessage(LogSeverity severity) {
  return severity >= GetMinLogLevel();
}

void KillProcess() {
#ifdef FML_OS_OHOS
  HILOG_ERROR("FML KILL PROCESS");
#else
  abort();
#endif
}

}  // namespace fml
