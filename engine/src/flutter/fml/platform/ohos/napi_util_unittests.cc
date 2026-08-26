/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/fml/platform/ohos/napi_util.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace fml {
namespace napi {
std::string NapiGetLastError(napi_env env, napi_status status);
bool IsArrayBuffer(napi_env env, napi_value value);
}

namespace testing {
namespace {

extern "C" {
void StubNapiReset(void);
void StubNapiSetValuetype(napi_valuetype t);
void StubNapiSetString(const char* s);
void StubNapiSetArrayLength(uint32_t n);
void StubNapiSetArrayLike(bool is_arraybuffer,
                          bool is_array,
                          bool is_typedarray);
void StubNapiSetArraybufferData(void* data, size_t len);
void StubNapiSetCreateArraybufferValue(napi_value v);
void StubNapiFailTypeof(napi_status s);
void StubNapiFailStringUtf8(napi_status s, int skip);
void StubNapiFailArrayLength(napi_status s);
void StubNapiFailIsArraybuffer(napi_status s);
void StubNapiFailIsArray(napi_status s);
void StubNapiFailArraybufferInfo(napi_status s);
void StubNapiFailCreateArraybuffer(napi_status s);
void StubNapiFailReference(napi_status s);
void StubNapiFailNamedProperty(napi_status s);
void StubNapiFailCallFunction(napi_status s);
}

napi_env FakeEnv() {
  return reinterpret_cast<napi_env>(0x1);
}

napi_value FakeValue() {
  return reinterpret_cast<napi_value>(0x2);
}

using namespace fml::napi;

class NapiUtilTest : public ::testing::Test {
 protected:
  void SetUp() override { StubNapiReset(); }
  void TearDown() override { StubNapiReset(); }
};

TEST_F(NapiUtilTest, IsNullWithNullptrShortCircuits) {
  EXPECT_TRUE(NapiIsNull(FakeEnv(), nullptr));
}

TEST_F(NapiUtilTest, IsNullWithNapiNull) {
  StubNapiSetValuetype(napi_null);
  EXPECT_TRUE(NapiIsNull(FakeEnv(), FakeValue()));
}

TEST_F(NapiUtilTest, IsNullWithOtherType) {
  StubNapiSetValuetype(napi_number);
  EXPECT_FALSE(NapiIsNull(FakeEnv(), FakeValue()));
}

TEST_F(NapiUtilTest, IsTypeMatches) {
  StubNapiSetValuetype(napi_string);
  EXPECT_TRUE(NapiIsType(FakeEnv(), FakeValue(), napi_string));
}

TEST_F(NapiUtilTest, IsTypeMismatch) {
  StubNapiSetValuetype(napi_string);
  EXPECT_FALSE(NapiIsType(FakeEnv(), FakeValue(), napi_number));
}

TEST_F(NapiUtilTest, IsTypeFailsOnTypeofError) {
  StubNapiFailTypeof(napi_generic_failure);
  EXPECT_FALSE(NapiIsType(FakeEnv(), FakeValue(), napi_undefined));
}

TEST_F(NapiUtilTest, IsNotTypeDelegates) {
  StubNapiSetValuetype(napi_string);
  EXPECT_FALSE(NapiIsNotType(FakeEnv(), FakeValue(), napi_string));
  EXPECT_TRUE(NapiIsNotType(FakeEnv(), FakeValue(), napi_number));
}

TEST_F(NapiUtilTest, GetLastErrorContainsPrefixAndMessage) {
  std::string msg = NapiGetLastError(FakeEnv(), napi_invalid_arg);
  EXPECT_NE(msg.find("Napi Error:"), std::string::npos);
  EXPECT_NE(msg.find("ace_napi_stub: no JS runtime"), std::string::npos);
}

TEST_F(NapiUtilTest, IsAnyTypeFirstArgMatches) {
  StubNapiSetValuetype(napi_number);
  EXPECT_TRUE(NapiIsAnyType(FakeEnv(), FakeValue(), napi_number, napi_string));
}

TEST_F(NapiUtilTest, IsAnyTypeLaterArgMatches) {
  StubNapiSetValuetype(napi_string);
  EXPECT_TRUE(NapiIsAnyType(FakeEnv(), FakeValue(), napi_number, napi_string));
}

TEST_F(NapiUtilTest, IsAnyTypeFailsOnTypeofError) {
  StubNapiFailTypeof(napi_generic_failure);
  EXPECT_FALSE(
      NapiIsAnyType(FakeEnv(), FakeValue(), napi_number, napi_string));
}

TEST_F(NapiUtilTest, PrintValueTypesMixedArgs) {
  StubNapiSetValuetype(napi_number);
  napi_value args[] = {nullptr, FakeValue()};
  EXPECT_NO_FATAL_FAILURE({ NapiPrintValueTypes(FakeEnv(), 2, args); });
}

TEST_F(NapiUtilTest, PrintValueTypeCoversAllValuetypes) {
  const napi_valuetype types[] = {
      napi_number,   napi_string,  napi_boolean, napi_object,
      napi_function, napi_null,    napi_symbol,  napi_external,
      napi_bigint,   napi_undefined,
  };
  EXPECT_NO_FATAL_FAILURE({
    for (napi_valuetype t : types) {
      StubNapiSetValuetype(t);
      NapiPrintValueType(FakeEnv(), FakeValue());
    }
  });
}

TEST_F(NapiUtilTest, PrintValueTypeFlagsArrayAndTypedarray) {
  StubNapiSetValuetype(napi_object);
  StubNapiSetArrayLike(false, true, true);
  EXPECT_NO_FATAL_FAILURE({ NapiPrintValueType(FakeEnv(), FakeValue()); });
}

TEST_F(NapiUtilTest, PrintValueTypeFailsOnTypeofError) {
  StubNapiFailTypeof(napi_generic_failure);
  EXPECT_NO_FATAL_FAILURE({ NapiPrintValueType(FakeEnv(), FakeValue()); });
}

TEST_F(NapiUtilTest, IsArraybufferTrueWhenArraybuffer) {
  StubNapiSetArrayLike(true, false, false);
  EXPECT_TRUE(IsArrayBuffer(FakeEnv(), FakeValue()));
}

TEST_F(NapiUtilTest, IsArraybufferTrueWhenPlainArray) {
  StubNapiSetArrayLike(false, true, false);
  EXPECT_TRUE(IsArrayBuffer(FakeEnv(), FakeValue()));
}

TEST_F(NapiUtilTest, IsArraybufferFailsOnArraybufferStatus) {
  StubNapiFailIsArraybuffer(napi_generic_failure);
  EXPECT_FALSE(IsArrayBuffer(FakeEnv(), FakeValue()));
}

TEST_F(NapiUtilTest, IsArraybufferFailsOnArrayStatus) {
  StubNapiFailIsArray(napi_generic_failure);
  EXPECT_FALSE(IsArrayBuffer(FakeEnv(), FakeValue()));
}

TEST_F(NapiUtilTest, IsArraybufferReturnsTrueEvenForScalar) {
  StubNapiSetArrayLike(false, false, false);
  EXPECT_TRUE(IsArrayBuffer(FakeEnv(), FakeValue()));
}

TEST_F(NapiUtilTest, GetStringOfNapiNullYieldsEmpty) {
  StubNapiSetValuetype(napi_null);
  std::string out = "sentinel";
  EXPECT_EQ(GetString(FakeEnv(), FakeValue(), out), kSuccess);
  EXPECT_EQ(out, "");
}

TEST_F(NapiUtilTest, GetStringOfNonStringFails) {
  StubNapiSetValuetype(napi_number);
  std::string out;
  EXPECT_EQ(GetString(FakeEnv(), FakeValue(), out), kErrorType);
}

TEST_F(NapiUtilTest, GetStringSucceeds) {
  StubNapiSetValuetype(napi_string);
  StubNapiSetString("hello");
  std::string out;
  EXPECT_EQ(GetString(FakeEnv(), FakeValue(), out), kSuccess);
  EXPECT_EQ(out, "hello");
}

TEST_F(NapiUtilTest, GetStringFailsOnLengthQuery) {
  StubNapiSetValuetype(napi_string);
  StubNapiFailStringUtf8(napi_generic_failure, 0);
  std::string out;
  EXPECT_EQ(GetString(FakeEnv(), FakeValue(), out),
            static_cast<int32_t>(napi_generic_failure));
}

TEST_F(NapiUtilTest, GetStringFailsOnCopy) {
  StubNapiSetValuetype(napi_string);
  StubNapiSetString("abc");
  StubNapiFailStringUtf8(napi_generic_failure, 1);
  std::string out;
  EXPECT_EQ(GetString(FakeEnv(), FakeValue(), out),
            static_cast<int32_t>(napi_generic_failure));
}

TEST_F(NapiUtilTest, GetArrayStringEmptyArray) {
  StubNapiSetArrayLength(0);
  std::vector<std::string> out;
  EXPECT_EQ(GetArrayString(FakeEnv(), FakeValue(), out), kSuccess);
  EXPECT_TRUE(out.empty());
}

TEST_F(NapiUtilTest, GetArrayStringTwoNullElements) {
  StubNapiSetArrayLength(2);
  std::vector<std::string> out;
  EXPECT_EQ(GetArrayString(FakeEnv(), FakeValue(), out), kSuccess);
  EXPECT_EQ(out.size(), 2u);
  EXPECT_EQ(out[0], "");
  EXPECT_EQ(out[1], "");
}

TEST_F(NapiUtilTest, GetArrayStringFailsOnLength) {
  StubNapiFailArrayLength(napi_generic_failure);
  std::vector<std::string> out;
  EXPECT_EQ(GetArrayString(FakeEnv(), FakeValue(), out),
            static_cast<int32_t>(napi_generic_failure));
}

TEST_F(NapiUtilTest, GetArrayBufferOfNapiNullFails) {
  StubNapiSetValuetype(napi_null);
  void* data = nullptr;
  size_t len = 0;
  EXPECT_EQ(GetArrayBuffer(FakeEnv(), FakeValue(), &data, &len), kErrorNull);
}

TEST_F(NapiUtilTest, GetArrayBufferNotArraybufferFails) {
  StubNapiFailIsArraybuffer(napi_generic_failure);
  void* data = nullptr;
  size_t len = 0;
  EXPECT_EQ(GetArrayBuffer(FakeEnv(), FakeValue(), &data, &len),
            static_cast<int32_t>(napi_invalid_arg));
}

TEST_F(NapiUtilTest, GetArrayBufferFailsOnInfo) {
  StubNapiSetArrayLike(true, false, false);
  StubNapiFailArraybufferInfo(napi_generic_failure);
  void* data = nullptr;
  size_t len = 0;
  EXPECT_EQ(GetArrayBuffer(FakeEnv(), FakeValue(), &data, &len),
            static_cast<int32_t>(napi_generic_failure));
}

TEST_F(NapiUtilTest, GetArrayBufferNullDataFails) {
  StubNapiSetArrayLike(true, false, false);
  StubNapiSetArraybufferData(nullptr, 0);
  void* data = nullptr;
  size_t len = 0;
  EXPECT_EQ(GetArrayBuffer(FakeEnv(), FakeValue(), &data, &len), kErrorNull);
}

TEST_F(NapiUtilTest, GetArrayBufferSucceeds) {
  StubNapiSetArrayLike(true, false, false);
  char payload[4] = {};
  StubNapiSetArraybufferData(payload, sizeof(payload));
  void* data = nullptr;
  size_t len = 0;
  EXPECT_EQ(GetArrayBuffer(FakeEnv(), FakeValue(), &data, &len), kSuccess);
  EXPECT_EQ(data, &payload[0]);
  EXPECT_EQ(len, 4u);
}

TEST_F(NapiUtilTest, CreateArrayBufferNullInputReturnsNull) {
  EXPECT_EQ(CreateArrayBuffer(FakeEnv(), nullptr, 4), nullptr);
}

TEST_F(NapiUtilTest, CreateArrayBufferFailsOnCreate) {
  StubNapiFailCreateArraybuffer(napi_generic_failure);
  char input[4] = {};
  EXPECT_EQ(CreateArrayBuffer(FakeEnv(), input, sizeof(input)), nullptr);
}

TEST_F(NapiUtilTest, CreateArrayBufferSucceeds) {
  napi_value stub_result = reinterpret_cast<napi_value>(0x99);
  StubNapiSetCreateArraybufferValue(stub_result);
  char input[4] = {1, 2, 3, 4};
  EXPECT_EQ(CreateArrayBuffer(FakeEnv(), input, sizeof(input)), stub_result);
}

TEST_F(NapiUtilTest, InvokeJsMethodSucceeds) {
  EXPECT_EQ(InvokeJsMethod(FakeEnv(), reinterpret_cast<napi_ref>(0x1),
                           "onDone", 0, nullptr),
            napi_ok);
}

TEST_F(NapiUtilTest, InvokeJsMethodFailsOnReference) {
  StubNapiFailReference(napi_generic_failure);
  EXPECT_EQ(InvokeJsMethod(FakeEnv(), reinterpret_cast<napi_ref>(0x1),
                           "onDone", 0, nullptr),
            napi_generic_failure);
}

TEST_F(NapiUtilTest, InvokeJsMethodFailsOnNamedProperty) {
  StubNapiFailNamedProperty(napi_generic_failure);
  EXPECT_EQ(InvokeJsMethod(FakeEnv(), reinterpret_cast<napi_ref>(0x1),
                           "onDone", 0, nullptr),
            napi_generic_failure);
}

TEST_F(NapiUtilTest, InvokeJsMethodFailsOnCallFunction) {
  StubNapiFailCallFunction(napi_generic_failure);
  EXPECT_EQ(InvokeJsMethod(FakeEnv(), reinterpret_cast<napi_ref>(0x1),
                           "onDone", 0, nullptr),
            napi_generic_failure);
}

}
}
}
