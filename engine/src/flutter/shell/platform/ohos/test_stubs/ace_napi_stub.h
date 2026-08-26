/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_TESTING_ACE_NAPI_STUB_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_TESTING_ACE_NAPI_STUB_H_

#include <stdint.h>
#include "napi/native_api.h"

extern "C" {

void StubNapiReset(void);

void StubNapiSetValuetype(napi_valuetype t);
void StubNapiSetString(const char* s);
void StubNapiFailStringUtf8(napi_status s, int skip);
void StubNapiSetArrayLength(uint32_t n);
void StubNapiFailArrayLength(napi_status s);
void StubNapiSetArrayLike(bool is_arraybuffer, bool is_array,
                          bool is_typedarray);
void StubNapiSetArraybufferData(void* data, size_t len);
void StubNapiSetCbArgc(size_t argc);
void StubNapiFailInt64OnCall(int nth);
void StubNapiFailInt32OnCall(int nth);
void StubNapiFailUint32OnCall(int nth);
void StubNapiFailDoubleOnCall(int nth);
void StubNapiFailBoolOnCall(int nth);
void StubNapiFailGetBooleanOnCall(int nth);
void StubNapiSetInt32Value(int32_t value);
void StubNapiSetInt64Value(int64_t value);
void StubNapiSetDoubleValue(double value);
void StubNapiSetBigintLossless(bool lossless);
void StubNapiFailReference(napi_status s);
void StubNapiFailCallFunction(napi_status s);
void StubNapiFailNamedProperty(napi_status s);
napi_module* StubNapiGetRegisteredModule(void);

}

#endif  // FLUTTER_SHELL_PLATFORM_OHOS_TESTING_ACE_NAPI_STUB_H_
