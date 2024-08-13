/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_LOGGING_H
#define OHOS_LOGGING_H

#include <hilog/log.h>
#define APP_LOG_DOMAIN 0x0000
#define APP_LOG_TAG "XComFlutterOHOS_Native"

#define LOGD(...)                                                                             \
      ((void)OH_LOG_Print(LOG_APP, LOG_DEBUG, APP_LOG_DOMAIN, APP_LOG_TAG, __VA_ARGS__))

#define LOGI(...)                                                                             \
      ((void)OH_LOG_Print(LOG_APP, !(FML_LOG_IS_ON(INFO)) ? LOG_DEBUG : LOG_INFO,             \
      APP_LOG_DOMAIN, APP_LOG_TAG, __VA_ARGS__))

#define LOGW(...)                                                                             \
  ((void)OH_LOG_Print(LOG_APP, LOG_WARN, APP_LOG_DOMAIN, APP_LOG_TAG, __VA_ARGS__))

#define LOGE(...)                                                                             \
  ((void)OH_LOG_Print(LOG_APP, LOG_ERROR, APP_LOG_DOMAIN, APP_LOG_TAG, __VA_ARGS__))

#endif  // OHOS_LOGGING_H
