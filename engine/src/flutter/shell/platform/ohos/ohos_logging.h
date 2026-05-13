/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_OHOS_LOGGING_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_OHOS_LOGGING_H_

#include <hilog/log.h>
#define APP_LOG_DOMAIN 0x0000
#define APP_LOG_TAG "XComFlutterOHOS_Native"

#define LOGD(...)                                                      \
  ((void)OH_LOG_Print(LOG_APP, LOG_DEBUG, APP_LOG_DOMAIN, APP_LOG_TAG, \
                      __VA_ARGS__))

#define LOGI(...)                                                             \
  ((void)OH_LOG_Print(LOG_APP, !(FML_LOG_IS_ON(INFO)) ? LOG_DEBUG : LOG_INFO, \
                      APP_LOG_DOMAIN, APP_LOG_TAG, __VA_ARGS__))

#define LOGW(...)                                                     \
  ((void)OH_LOG_Print(LOG_APP, LOG_WARN, APP_LOG_DOMAIN, APP_LOG_TAG, \
                      __VA_ARGS__))

#define LOGE(...)                                                      \
  ((void)OH_LOG_Print(LOG_APP, LOG_ERROR, APP_LOG_DOMAIN, APP_LOG_TAG, \
                      __VA_ARGS__))

#endif  // FLUTTER_SHELL_PLATFORM_OHOS_OHOS_LOGGING_H_
