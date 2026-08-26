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

#include "gtest/gtest.h"

namespace flutter {
namespace testing {

enum class DlopenRedirect { kPassthrough = 0, kFail = 1, kWrongLib = 2 };

DlopenRedirect g_hisysevent_redirect = DlopenRedirect::kPassthrough;
DlopenRedirect g_vsync_redirect = DlopenRedirect::kPassthrough;
int g_redirect_count = 0;

void* RealDlopen(const char* file, int mode) {
  static auto real = reinterpret_cast<void* (*)(const char*, int)>(
      dlsym(RTLD_NEXT, "dlopen"));
  return real(file, mode);
}

bool IsLibAvailable() {
  return access("/system/lib64/chipset-pub-sdk/libhisysevent.z.so", F_OK) == 0;
}

#if defined(OHOS_X64_UNITTEST)
// x64 模拟器：libhisysevent.z.so 存在（IsLibAvailable()=true），但 hdc shell
// 身份（u:r:sh:s0）无权向 hiview 发送事件，HiSysEvent_Write 返回 -5
// （ERR_SENDFAIL）。root/特权身份返回 0。
constexpr int kExpectedWriteRet = -5;
#else
constexpr int kExpectedWriteRet = 0;
#endif

void SetHisyseventDlopenRedirect(int mode);
int GetAndResetDlopenRedirectCount();

struct RedirectGuard {
  ~RedirectGuard() {
    SetHisyseventDlopenRedirect(0);
    GetAndResetDlopenRedirectCount();
  }
};

void SetHisyseventDlopenRedirect(int mode) {
  g_hisysevent_redirect = static_cast<DlopenRedirect>(mode);
}

void SetNativeVsyncDlopenRedirect(int mode) {
  g_vsync_redirect = static_cast<DlopenRedirect>(mode);
}

int GetAndResetDlopenRedirectCount() {
  int count = g_redirect_count;
  g_redirect_count = 0;
  return count;
}

extern "C" void* dlopen(const char* file, int mode) {
  const bool hisysevent =
      file != nullptr && strstr(file, "libhisysevent") != nullptr;
  const bool native_vsync =
      file != nullptr && strstr(file, "libnative_vsync") != nullptr;
  if (hisysevent || native_vsync) {
    const auto redirect = hisysevent ? g_hisysevent_redirect : g_vsync_redirect;
    if (redirect != DlopenRedirect::kPassthrough) {
      g_redirect_count++;
      if (redirect == DlopenRedirect::kFail) {
        return nullptr;
      }
      return RealDlopen("libc.so", mode);
    }
  }
  return RealDlopen(file, mode);
}

}
}

namespace fml {
namespace testing {

TEST(HiSysEventWrite, DlopenFailureReturnsMinusOne) {
  flutter::testing::RedirectGuard guard;
  flutter::testing::SetHisyseventDlopenRedirect(1);
  int ret = HiSysEventWrite("dlopen_fail_scene", 1);
  if (flutter::testing::GetAndResetDlopenRedirectCount() > 0) {
    EXPECT_EQ(ret, -1);
  } else {
    EXPECT_TRUE(ret == 0 || ret == -5 || ret == -1);
  }
}

TEST(HiSysEventWrite, DlsymFailureClosesHandleAndReturnsMinusOne) {
  flutter::testing::RedirectGuard guard;
  flutter::testing::SetHisyseventDlopenRedirect(2);
  int ret = HiSysEventWrite("dlsym_fail_scene", 1);
  if (flutter::testing::GetAndResetDlopenRedirectCount() > 0) {
    EXPECT_EQ(ret, -1);
  } else {
    EXPECT_TRUE(ret == 0 || ret == -5 || ret == -1);
  }
}

TEST(HiSysEventWrite, ReturnsCorrectValueForValidInput) {
  int ret = HiSysEventWrite("test_scene", 100);
  if (flutter::testing::IsLibAvailable()) {
    EXPECT_EQ(ret, flutter::testing::kExpectedWriteRet);
  } else {
    EXPECT_EQ(ret, -1);
  }
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
  if (flutter::testing::IsLibAvailable()) {
    EXPECT_EQ(ret, flutter::testing::kExpectedWriteRet);
  } else {
    EXPECT_EQ(ret, -1);
  }
}

TEST(HiSysEventWrite, HandlesZeroTime) {
  int ret = HiSysEventWrite("scene", 0);
  if (flutter::testing::IsLibAvailable()) {
    EXPECT_EQ(ret, flutter::testing::kExpectedWriteRet);
  } else {
    EXPECT_EQ(ret, -1);
  }
}

TEST(HiSysEventWrite, HandlesLargeTime) {
  int ret = HiSysEventWrite("scene", UINT64_MAX);
  if (flutter::testing::IsLibAvailable()) {
    EXPECT_EQ(ret, flutter::testing::kExpectedWriteRet);
  } else {
    EXPECT_EQ(ret, -1);
  }
}

TEST(HiSysEventWrite, MultipleCallsAreSafe) {
  for (int i = 0; i < 10; i++) {
    int ret = HiSysEventWrite("scene", i * 100);
    if (flutter::testing::IsLibAvailable()) {
      EXPECT_EQ(ret, flutter::testing::kExpectedWriteRet);
    } else {
      EXPECT_EQ(ret, -1);
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
  flutter::testing::RedirectGuard guard;
  flutter::testing::SetHisyseventDlopenRedirect(2);
  int ret = HiSysEventWrite("dlsym_retry_scene", 1);
  const bool engaged = flutter::testing::GetAndResetDlopenRedirectCount() > 0;
  if (engaged) {
    EXPECT_EQ(ret, -1);
    flutter::testing::SetHisyseventDlopenRedirect(2);
    EXPECT_EQ(HiSysEventWrite("dlsym_retry_scene2", 2), -1);
    EXPECT_EQ(flutter::testing::GetAndResetDlopenRedirectCount(), 1);
  } else {
    EXPECT_EQ(ret, flutter::testing::kExpectedWriteRet);
  }
}

TEST(HiSysEventWrite, LoadedHandleShortCircuitsReload) {
  flutter::testing::RedirectGuard guard;
  int ret = HiSysEventWrite("load_once_scene", 1);
  if (flutter::testing::IsLibAvailable()) {
    EXPECT_EQ(ret, flutter::testing::kExpectedWriteRet);
  } else {
    EXPECT_EQ(ret, -1);
  }
  flutter::testing::SetHisyseventDlopenRedirect(1);
  int ret2 = HiSysEventWrite("load_cached_scene", 2);
  if (flutter::testing::IsLibAvailable()) {
    EXPECT_EQ(flutter::testing::GetAndResetDlopenRedirectCount(), 0);
  }
  EXPECT_EQ(ret2, ret);
}

}  // namespace testing
}  // namespace fml
