/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include <ace/xcomponent/native_interface_xcomponent.h>
#include <gtest/gtest.h>
#include <napi/native_api.h>
#include <string>
#include "flutter/shell/platform/ohos/test_stubs/ace_graphic_ndk_stub.h"
#include "flutter/shell/platform/ohos/test_stubs/ace_napi_stub.h"

namespace {
constexpr int32_t kXcompError = OH_NATIVEXCOMPONENT_RESULT_BAD_PARAMETER;

napi_env FakeEnv() {
  static char storage;
  return reinterpret_cast<napi_env>(&storage);
}

napi_value FakeExports() {
  static char storage;
  return reinterpret_cast<napi_value>(&storage);
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

}  // namespace
