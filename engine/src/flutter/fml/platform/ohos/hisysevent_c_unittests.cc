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

#include "flutter/fml/platform/ohos/hisysevent_c.h"

#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <thread>

#include "gtest/gtest.h"

namespace fml {
namespace testing {

namespace {

bool IsLibAvailable() {
  return access("/system/lib64/chipset-pub-sdk/libhisysevent.z.so", F_OK) == 0;
}

}  // namespace

// ===== HiSysEventWrite =====

TEST(HiSysEventWrite, ReturnsCorrectValueForValidInput) {
  int ret = HiSysEventWrite("test_scene", 100);
  if (IsLibAvailable()) {
    EXPECT_EQ(ret, 0);
  } else {
    EXPECT_EQ(ret, -1);
  }
}

TEST(HiSysEventWrite, HandlesNullName) {
  int ret = HiSysEventWrite(nullptr, 100);
  if (!IsLibAvailable()) {
    EXPECT_EQ(ret, -1);
  }
  // On device the real HiSysEvent_Write may reject a NULL name; we only
  // verify that the call does not crash.
}

TEST(HiSysEventWrite, HandlesEmptyName) {
  int ret = HiSysEventWrite("", 0);
  if (IsLibAvailable()) {
    EXPECT_EQ(ret, 0);
  } else {
    EXPECT_EQ(ret, -1);
  }
}

TEST(HiSysEventWrite, HandlesZeroTime) {
  int ret = HiSysEventWrite("scene", 0);
  if (IsLibAvailable()) {
    EXPECT_EQ(ret, 0);
  } else {
    EXPECT_EQ(ret, -1);
  }
}

TEST(HiSysEventWrite, HandlesLargeTime) {
  int ret = HiSysEventWrite("scene", UINT64_MAX);
  if (IsLibAvailable()) {
    EXPECT_EQ(ret, 0);
  } else {
    EXPECT_EQ(ret, -1);
  }
}

TEST(HiSysEventWrite, MultipleCallsAreSafe) {
  for (int i = 0; i < 10; i++) {
    int ret = HiSysEventWrite("scene", i * 100);
    if (IsLibAvailable()) {
      EXPECT_EQ(ret, 0);
    } else {
      EXPECT_EQ(ret, -1);
    }
  }
}

TEST(HiSysEventWrite, LongNameDoesNotCrash) {
  std::string long_name(256, 'x');
  int ret = HiSysEventWrite(long_name.c_str(), 50);
  if (!IsLibAvailable()) {
    EXPECT_EQ(ret, -1);
  }
}

// ===== HiSysEventTrace =====

TEST(HiSysEventTrace, HandlesNullName) {
  { HiSysEventTrace trace(nullptr); }
  SUCCEED();
}

TEST(HiSysEventTrace, HandlesValidName) {
  { HiSysEventTrace trace("test_trace"); }
  SUCCEED();
}

TEST(HiSysEventTrace, HandlesEmptyName) {
  { HiSysEventTrace trace(""); }
  SUCCEED();
}

TEST(HiSysEventTrace, MultipleTracesAreSafe) {
  for (int i = 0; i < 5; i++) {
    HiSysEventTrace trace("test_trace");
  }
  SUCCEED();
}

TEST(HiSysEventTrace, TraceWithSleepDoesNotCrash) {
  {
    HiSysEventTrace trace("sleep_trace");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  SUCCEED();
}

TEST(HiSysEventTrace, NestedScopesAreSafe) {
  {
    HiSysEventTrace outer("outer_trace");
    {
      HiSysEventTrace inner("inner_trace");
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
  SUCCEED();
}

}  // namespace testing
}  // namespace fml
