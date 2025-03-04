/*
 * Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
