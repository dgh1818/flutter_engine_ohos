/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include <ace/xcomponent/native_interface_xcomponent.h>
#include <gtest/gtest.h>
#include <napi/native_api.h>
#include "flutter/shell/platform/ohos/test_stubs/ace_napi_stub.h"
#include "flutter/shell/platform/ohos/test_stubs/unittest_x64/ace_ndk_stub.h"
#include <string>

#ifndef OHOS_X64_UNITTEST
namespace {
int32_t g_fail_get_id_next = 0;

constexpr int32_t kXcompError = OH_NATIVEXCOMPONENT_RESULT_BAD_PARAMETER;

napi_env FakeEnv() {
  return reinterpret_cast<napi_env>(0x1);
}

napi_value FakeExports() {
  return reinterpret_cast<napi_value>(0x2);
}

TEST(LibraryLoaderTest, RegisteredModuleMetadata) {
  napi_module* mod = StubNapiGetRegisteredModule();
  ASSERT_NE(mod, nullptr);
  EXPECT_EQ(std::string(mod->nm_modname), "flutter");
  EXPECT_EQ(mod->nm_version, 1);
  ASSERT_NE(mod->nm_register_func, nullptr);
}

TEST(LibraryLoaderTest, InitRegistersAndReturnsExports) {
  napi_module* mod = StubNapiGetRegisteredModule();
  ASSERT_NE(mod, nullptr);
  napi_value exports = FakeExports();
  napi_value result = mod->nm_register_func(FakeEnv(), exports);
  EXPECT_EQ(result, exports);
}

TEST(LibraryLoaderTest, InitToleratesXComponentExportFailure) {
  napi_module* mod = StubNapiGetRegisteredModule();
  ASSERT_NE(mod, nullptr);
  StubXcompFailNextGetXComponentId(kXcompError);
  napi_value exports = FakeExports();
  napi_value result = mod->nm_register_func(FakeEnv(), exports);
  EXPECT_EQ(result, exports);
}

TEST(LibraryLoaderTest, InitToleratesNapiPropertyLookupFailure) {
  napi_module* mod = StubNapiGetRegisteredModule();
  ASSERT_NE(mod, nullptr);
  StubNapiFailNamedProperty(napi_generic_failure);
  napi_value exports = FakeExports();
  napi_value result = mod->nm_register_func(FakeEnv(), exports);
  EXPECT_EQ(result, exports);
}

}

extern "C" int32_t OH_NativeXComponent_GetXComponentId(
    OH_NativeXComponent* /*component*/,
    char* id,
    uint64_t* size) {
  if (g_fail_get_id_next != 0) {
    int32_t ret = g_fail_get_id_next;
    g_fail_get_id_next = 0;
    return ret;
  }
  if (id != nullptr && size != nullptr && *size > 0) {
    id[0] = '\0';
    *size = 0;
  }
  return OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
}

extern "C" void StubXcompFailNextGetXComponentId(int32_t ret) {
  g_fail_get_id_next = ret;
}
#endif  // !OHOS_X64_UNITTEST

namespace flutter {
namespace testing {
}
}
