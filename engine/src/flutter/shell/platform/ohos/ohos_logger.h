/*
 * Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd.
 * All rights reserved. Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE_KHZG file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_OHOS_LOGGER_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_OHOS_LOGGER_H_
#include <hilog/log.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  /** Debug level to be used by {@link OH_LOG_DEBUG} */
  kOhosLogDebug = 3,
  /** Informational level to be used by {@link OH_LOG_INFO} */
  kOhosLogInfo = 4,
  /** Warning level to be used by {@link OH_LOG_WARN} */
  kOhosLogWarn = 5,
  /** Error level to be used by {@link OH_LOG_ERROR} */
  kOhosLogError = 6,
  /** Fatal level to be used by {@link OH_LOG_FATAL} */
  kOhosLogFatal = 7,
} OhosLogLevel;

extern int ohos_log(OhosLogLevel level, const char* fmt, ...);

#ifdef __cplusplus
}  // end extern
#endif

#endif  // FLUTTER_SHELL_PLATFORM_OHOS_OHOS_LOGGER_H_
