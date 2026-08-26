/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "deviceinfo.h"

extern "C" {

int g_stub_sdk_api_version = -1;

#if defined(OHOS_X64_UNITTEST)

int OH_GetSdkApiVersion(void) {
  if (g_stub_sdk_api_version < 0) {
    return 20;
  }
  return g_stub_sdk_api_version;
}

#else  // !defined(OHOS_X64_UNITTEST)

int __real_OH_GetSdkApiVersion(void);

int __wrap_OH_GetSdkApiVersion(void) {
  if (g_stub_sdk_api_version < 0) {
    return __real_OH_GetSdkApiVersion();
  }
  return g_stub_sdk_api_version;
}

#endif  // defined(OHOS_X64_UNITTEST)

}
