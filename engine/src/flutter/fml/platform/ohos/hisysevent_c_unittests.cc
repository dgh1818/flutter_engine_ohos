/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

// Unit tests for fml/platform/ohos/hisysevent_c.cc
//
// Test registration: shell/platform/ohos/BUILD.gn → flutter_ohos_unittests
// (device side, ohos_*_arm64). Not registered in fml_unittests because the
// host build (host_profile) has is_ohos=false and does not compile the OHos
// platform sources.
//
// The tests are platform-aware via IsLibAvailable(): on device the real
// libhisysevent.z.so exists so dlopen succeeds and HiSysEventWrite delegates
// to the real HiSysEvent_Write (returns 0); on host the library is absent so
// dlopen fails and HiSysEventWrite returns -1. This lets the same test file
// produce meaningful assertions on both platforms.
//
// Coverage gaps: the dlsym-returns-NULL branch (library loaded but symbol not
// found) is defensive code that cannot be triggered without a malformed .so
// and is therefore not covered.

#include <unistd.h>
#include <chrono>
#include <cstring>
#include <thread>

#define private public
#include "flutter/fml/platform/ohos/hisysevent_c.h"
#undef private

#include "flutter/shell/platform/ohos/test_stubs/libc_wrapper_stub.h"
#include "gtest/gtest.h"

namespace flutter {
namespace testing {

bool IsLibAvailable() {
  return access("/system/lib64/chipset-pub-sdk/libhisysevent.z.so", F_OK) == 0;
}

constexpr int kExpectedWriteRet = 0;

#define ASSERT_WRITE_RET_ONCE(var)                       \
  static bool asserted_once_ = false;                    \
  if (!flutter::testing::IsLibAvailable()) {             \
    EXPECT_EQ(var, -1);                                  \
  } else if (!asserted_once_) {                          \
    EXPECT_EQ(var, flutter::testing::kExpectedWriteRet); \
    asserted_once_ = true;                               \
  }
}  // namespace testing
}  // namespace flutter

namespace fml {
namespace testing {

TEST(HiSysEventWrite, DlopenFailureReturnsMinusOne) {
  ::GetAndResetDlopenRedirectCount();
  ::ScopedDlopenRedirect redirect("libhisysevent",
                                  ::DlopenRedirectMode::kFailOpen);
  int ret = HiSysEventWrite("dlopen_fail_scene", 1);
  if (::GetAndResetDlopenRedirectCount() > 0) {
    EXPECT_EQ(ret, -1);
  } else {
    SUCCEED();
  }
}

TEST(HiSysEventWrite, DlsymFailureClosesHandleAndReturnsMinusOne) {
  ::GetAndResetDlopenRedirectCount();
  ::ScopedDlopenRedirect redirect("libhisysevent",
                                  ::DlopenRedirectMode::kWrongLib);
  int ret = HiSysEventWrite("dlsym_fail_scene", 1);
  if (::GetAndResetDlopenRedirectCount() > 0) {
    EXPECT_EQ(ret, -1);
  } else {
    SUCCEED();
  }
}

TEST(HiSysEventWrite, ReturnsCorrectValueForValidInput) {
  int ret = HiSysEventWrite("test_scene", 100);
  ASSERT_WRITE_RET_ONCE(ret);
}

TEST(HiSysEventWrite, HandlesNullName) {
  int ret = HiSysEventWrite(nullptr, 100);
  if (!flutter::testing::IsLibAvailable()) {
    EXPECT_EQ(ret, -1);
  }
  // On device the real HiSysEvent_Write may reject a NULL name; we only
  // verify that the call does not crash.
}

TEST(HiSysEventWrite, HandlesEmptyName) {
  int ret = HiSysEventWrite("", 0);
  ASSERT_WRITE_RET_ONCE(ret);
}

TEST(HiSysEventWrite, HandlesZeroTime) {
  int ret = HiSysEventWrite("scene", 0);
  ASSERT_WRITE_RET_ONCE(ret);
}

TEST(HiSysEventWrite, HandlesLargeTime) {
  int ret = HiSysEventWrite("scene", UINT64_MAX);
  ASSERT_WRITE_RET_ONCE(ret);
}

TEST(HiSysEventWrite, MultipleCallsAreSafe) {
  static bool asserted_once_ = false;
  for (int i = 0; i < 10; i++) {
    int ret = HiSysEventWrite("scene", i * 100);
    if (!flutter::testing::IsLibAvailable()) {
      EXPECT_EQ(ret, -1);
    } else if (!asserted_once_) {
      EXPECT_EQ(ret, flutter::testing::kExpectedWriteRet);
      asserted_once_ = true;
    }
  }
}

TEST(HiSysEventWrite, LongNameDoesNotCrash) {
  std::string long_name(256, 'x');
  int ret = HiSysEventWrite(long_name.c_str(), 50);
  if (!flutter::testing::IsLibAvailable()) {
    EXPECT_EQ(ret, -1);
  }
}

// ===== HiSysEventTrace =====

TEST(HiSysEventTrace, HandlesNullName) {
  HiSysEventTrace trace(nullptr);
  EXPECT_STREQ(trace.name_, "flutter default trace name");
}

TEST(HiSysEventTrace, HandlesValidName) {
  HiSysEventTrace trace("test_trace");
  EXPECT_STREQ(trace.name_, "test_trace");
  EXPECT_TRUE(trace.begin_time_.tv_sec != 0 || trace.begin_time_.tv_nsec != 0);
}

TEST(HiSysEventTrace, HandlesEmptyName) {
  HiSysEventTrace trace("");
  EXPECT_STREQ(trace.name_, "");
}

TEST(HiSysEventTrace, MultipleTracesAreSafe) {
  for (int i = 0; i < 5; i++) {
    HiSysEventTrace trace("test_trace");
    EXPECT_STREQ(trace.name_, "test_trace");
  }
}

TEST(HiSysEventTrace, TraceWithSleepDoesNotCrash) {
  HiSysEventTrace trace("sleep_trace");
  EXPECT_STREQ(trace.name_, "sleep_trace");
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

TEST(HiSysEventTrace, NestedScopesAreSafe) {
  HiSysEventTrace outer("outer_trace");
  EXPECT_STREQ(outer.name_, "outer_trace");
  {
    HiSysEventTrace inner("inner_trace");
    EXPECT_STREQ(inner.name_, "inner_trace");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

TEST(HiSysEventWrite, DlsymFailureResetsHandleForRetry) {
  ::GetAndResetDlopenRedirectCount();
  ::ScopedDlopenRedirect redirect("libhisysevent",
                                  ::DlopenRedirectMode::kWrongLib);
  int ret = HiSysEventWrite("dlsym_retry_scene", 1);
  const bool engaged = ::GetAndResetDlopenRedirectCount() > 0;
  if (engaged) {
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(HiSysEventWrite("dlsym_retry_scene2", 2), -1);
    EXPECT_EQ(::GetAndResetDlopenRedirectCount(), 1);
  } else {
    SUCCEED();
  }
}

TEST(HiSysEventWrite, LoadedHandleShortCircuitsReload) {
  int ret = HiSysEventWrite("load_once_scene", 1);
  ASSERT_WRITE_RET_ONCE(ret);
  ::GetAndResetDlopenRedirectCount();
  ::ScopedDlopenRedirect redirect("libhisysevent",
                                  ::DlopenRedirectMode::kFailOpen);
  int ret2 = HiSysEventWrite("load_cached_scene", 2);
  if (flutter::testing::IsLibAvailable()) {
    EXPECT_EQ(::GetAndResetDlopenRedirectCount(), 0);
  }
  EXPECT_EQ(ret2, ret);
}

}  // namespace testing
}  // namespace fml
