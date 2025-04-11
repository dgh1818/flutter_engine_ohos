/*
 * Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_KHZG file.
 */

#include "ohos_logger.h"

#define OHOS_TAG "XComFlutterOHOS"

int ohos_log(OhosLogLevel level, const char* fmt, ...) {
  char buffer[1024];
  int len = 0;
  {
    va_list args;
    va_start(args, fmt);
    len = vsnprintf(buffer, sizeof(buffer) - 1, fmt, args);
    va_end(args);
  }
  if (len <= 0) {
    return len;
  }
  buffer[len] = '\0';
  // int OH_LOG_Print(LogType type, LogLevel level, unsigned int domain, const
  // char *tag, const char *fmt, ...)
  OH_LOG_Print(LOG_APP, (LogLevel)level, LOG_DOMAIN, OHOS_TAG,
               "Thread:%{public}lu  %{public}s", (unsigned long)pthread_self(),
               buffer);
  return len;
}
