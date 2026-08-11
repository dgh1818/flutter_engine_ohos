/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

// Unit tests for fml/platform/ohos/hiappevent/ohos_hiappevent.cpp
//
// Test registration: shell/platform/ohos/BUILD.gn → flutter_ohos_unittests
// (device side, ohos_*_arm64).
//
// The directly-called OHos native API functions (OH_HiAppEvent_*,
// OH_HiTrace_CountTrace, OH_HiTrace_StartAsyncTraceEx,
// OH_HiTrace_FinishAsyncTraceEx) are overridden with strong stub definitions in
// this file. At link time the stubs take precedence over the same symbols in
// the native shared libraries, allowing the tests to assert which function was
// called and with what arguments without triggering real side effects.
//
// Functions loaded via dlsym(handle, ...) (e.g. OH_HiAppEvent_CreateProcessor)
// cannot be stubbed this way and remain the real implementations from
// libhiappevent_ndk.z.so. Tests that exercise FlushAllIn therefore verify
// graceful behaviour regardless of whether the real processor is available.
//
// Coverage gaps: the private methods WriteSingleFrame, WriteStatisticFrame and
// WriteScrolledFrame are only reachable through FlushAllIn after a successful
// createProcessorFunc_("xperfbridge") call. On test devices where the
// "xperfbridge" processor is unavailable (returns nullptr), FlushAllIn takes
// the early-return path and these three functions remain uncovered.
// Additionally, OH_HiTrace_StartAsyncTraceEx/FinishAsyncTraceEx are loaded via
// dlsym(RTLD_DEFAULT, ...) which requires -rdynamic to find the stubs; without
// it the function pointers stay NULL and the extended trace branch is skipped.

#include "flutter/fml/platform/ohos/hiappevent/ohos_hiappevent.h"

#include <unistd.h>

#include <cstring>
#include <string>

#include "gtest/gtest.h"

namespace fml {
namespace hiappevent {
namespace testing {

// ===== Recording state for stub functions =====

namespace {

struct HiAppEventCallLog {
  int create_paramlist_count = 0;
  int destroy_paramlist_count = 0;
  int add_string_param_count = 0;
  int add_int32_param_count = 0;
  int add_int64_param_count = 0;
  int write_count = 0;
  int write_return_value = 0;
  std::string write_domain;
  std::string write_name;
  int count_trace_count = 0;
  std::string count_trace_name;
  int64_t count_trace_value = 0;
  int start_async_trace_ex_count = 0;
  int finish_async_trace_ex_count = 0;
};

HiAppEventCallLog& GetCallLog() {
  static HiAppEventCallLog log;
  return log;
}

void ResetCallLog() {
  GetCallLog() = HiAppEventCallLog{};
}

// A fake non-null ParamList pointer used by the stub.
ParamList FakeParamList() {
  return reinterpret_cast<ParamList>(0x1234);
}

}  // namespace

// ===== Strong stub definitions of OHos native API functions =====
//
// These definitions are strong symbols in the main executable; at link time
// they take precedence over the same symbols in libhiappevent_ndk.z.so and
// libhitrace_ndk.z.so (standard ELF symbol interposition).

extern "C" {
ParamList OH_HiAppEvent_CreateParamList(void) {
  GetCallLog().create_paramlist_count++;
  return FakeParamList();
}

void OH_HiAppEvent_DestroyParamList(ParamList list) {
  GetCallLog().destroy_paramlist_count++;
}

ParamList OH_HiAppEvent_AddStringParam(ParamList list,
                                       const char* name,
                                       const char* str) {
  GetCallLog().add_string_param_count++;
  return list;
}

ParamList OH_HiAppEvent_AddInt32Param(ParamList list,
                                      const char* name,
                                      int32_t num) {
  GetCallLog().add_int32_param_count++;
  return list;
}

ParamList OH_HiAppEvent_AddInt64Param(ParamList list,
                                      const char* name,
                                      int64_t num) {
  GetCallLog().add_int64_param_count++;
  return list;
}

int OH_HiAppEvent_Write(const char* domain,
                        const char* name,
                        enum EventType type,
                        const ParamList list) {
  GetCallLog().write_count++;
  GetCallLog().write_domain = domain ? domain : "";
  GetCallLog().write_name = name ? name : "";
  return GetCallLog().write_return_value;
}

void OH_HiTrace_CountTrace(const char* name, int64_t count) {
  GetCallLog().count_trace_count++;
  GetCallLog().count_trace_name = name ? name : "";
  GetCallLog().count_trace_value = count;
}

void OH_HiTrace_StartAsyncTraceEx(HiTrace_Output_Level level,
                                  const char* name,
                                  int32_t taskId,
                                  const char* customCategory,
                                  const char* customArgs) {
  GetCallLog().start_async_trace_ex_count++;
}

void OH_HiTrace_FinishAsyncTraceEx(HiTrace_Output_Level level,
                                   const char* name,
                                   int32_t taskId) {
  GetCallLog().finish_async_trace_ex_count++;
}

}  // extern "C"

// ===== Test fixture =====

class OhosHiappEventTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ResetCallLog();
    // Reset global atomic state
    ScrollStatus.store(-1);
    scroll_start_frame_.store(0);
    scroll_end_frame_.store(0);
    scroll_start_time_utc_ms.store(0);
    scroll_end_time_utc_ms.store(0);
    last_frame_number_.store(0);
  }

  // Helper: create a MissedFrameInfo with specified duration
  static MissedFrameInfo MakeFrameInfo(int64_t duration_us,
                                       int missed = 1,
                                       uint64_t frame_number = 1) {
    MissedFrameInfo info{};
    info.utc_time_stamp_millis = 100000;
    info.vsync_start_time_micros = 1000;
    info.vsync_target_time_micros = 2000;
    info.latest_vsync_target_time_micros = 3000;
    info.frame_duration_micros = duration_us;
    info.raster_finish_time_micros = 1000 + duration_us;
    info.frame_budget_time_micros = 16667;
    info.frame_number = frame_number;
    info.vsync_transitions_missed = missed;
    return info;
  }
};

// ===== GetFlutterVersion =====

TEST_F(OhosHiappEventTest, GetFlutterVersionReturnsNonNull) {
  EXPECT_NE(OhosHiappEventDDL::GetFlutterVersion(), nullptr);
}

TEST_F(OhosHiappEventTest, GetFlutterVersionReturnsSamePointer) {
  const char* a = OhosHiappEventDDL::GetFlutterVersion();
  const char* b = OhosHiappEventDDL::GetFlutterVersion();
  EXPECT_EQ(a, b);
}

// ===== GetInstance (singleton) =====

TEST_F(OhosHiappEventTest, GetInstanceReturnsSingleton) {
  auto a = OhosHiappEventDDL::GetInstance();
  auto b = OhosHiappEventDDL::GetInstance();
  ASSERT_NE(a, nullptr);
  ASSERT_NE(b, nullptr);
  EXPECT_EQ(a.get(), b.get());
}

// ===== UpdateLastFrameNumber =====

TEST_F(OhosHiappEventTest, UpdateLastFrameNumberStoresValue) {
  OhosHiappEventDDL ddl;
  ddl.UpdateLastFrameNumber(42);
  EXPECT_EQ(last_frame_number_.load(), 42u);
}

TEST_F(OhosHiappEventTest, UpdateLastFrameNumberZero) {
  OhosHiappEventDDL ddl;
  ddl.UpdateLastFrameNumber(0);
  EXPECT_EQ(last_frame_number_.load(), 0u);
}

TEST_F(OhosHiappEventTest, UpdateLastFrameNumberLargeValue) {
  OhosHiappEventDDL ddl;
  ddl.UpdateLastFrameNumber(UINT64_MAX);
  EXPECT_EQ(last_frame_number_.load(), UINT64_MAX);
}

// ===== OnScrollStart =====

TEST_F(OhosHiappEventTest, OnScrollStartSetsScrollStatus) {
  OhosHiappEventDDL ddl;
  ddl.UpdateLastFrameNumber(100);
  ddl.OnScrollStart();
  EXPECT_EQ(ScrollStatus.load(),
            static_cast<int>(ScrollingStatus::kScrollStart));
}

TEST_F(OhosHiappEventTest, OnScrollStartRecordsFrameIds) {
  OhosHiappEventDDL ddl;
  ddl.UpdateLastFrameNumber(100);
  ddl.OnScrollStart();
  EXPECT_EQ(scroll_start_frame_.load(), 100u);
  EXPECT_EQ(scroll_end_frame_.load(), 100u);
}

TEST_F(OhosHiappEventTest, OnScrollStartRecordsTimestamp) {
  OhosHiappEventDDL ddl;
  ddl.UpdateLastFrameNumber(100);
  ddl.OnScrollStart();
  EXPECT_NE(scroll_start_time_utc_ms.load(), 0);
  EXPECT_EQ(scroll_end_time_utc_ms.load(), 0);
}

// ===== OnScrollEndAndFlush =====

TEST_F(OhosHiappEventTest, OnScrollEndSetsScrollStatus) {
  OhosHiappEventDDL ddl;
  ddl.UpdateLastFrameNumber(200);
  ddl.OnScrollEndAndFlush();
  EXPECT_EQ(ScrollStatus.load(), static_cast<int>(ScrollingStatus::kScrollEnd));
}

TEST_F(OhosHiappEventTest, OnScrollEndRecordsFrameId) {
  OhosHiappEventDDL ddl;
  ddl.UpdateLastFrameNumber(200);
  ddl.OnScrollEndAndFlush();
  EXPECT_EQ(scroll_end_frame_.load(), 200u);
}

TEST_F(OhosHiappEventTest, OnScrollEndRecordsTimestamp) {
  OhosHiappEventDDL ddl;
  ddl.UpdateLastFrameNumber(200);
  ddl.OnScrollEndAndFlush();
  EXPECT_NE(scroll_end_time_utc_ms.load(), 0);
}

TEST_F(OhosHiappEventTest, OnScrollEndWithNoEventsDoesNotCrash) {
  OhosHiappEventDDL ddl;
  ddl.OnScrollEndAndFlush();
  SUCCEED();
}

// ===== ReportScrollJANKEvent threshold (50ms = 50000us) =====

TEST_F(OhosHiappEventTest, ReportScrollJANKBelowThresholdIgnored) {
  OhosHiappEventDDL ddl;
  ddl.ReportScrollJANKEvent(MakeFrameInfo(49999));
  // WriteJANKEventToTrace should NOT have been called
  EXPECT_EQ(GetCallLog().count_trace_count, 0);
}

TEST_F(OhosHiappEventTest, ReportScrollJANKAtThresholdRecorded) {
  OhosHiappEventDDL ddl;
  ddl.ReportScrollJANKEvent(MakeFrameInfo(50000));
  // WriteJANKEventToTrace SHOULD have been called
  EXPECT_EQ(GetCallLog().count_trace_count, 1);
}

TEST_F(OhosHiappEventTest, ReportScrollJANKAboveThresholdRecorded) {
  OhosHiappEventDDL ddl;
  ddl.ReportScrollJANKEvent(MakeFrameInfo(100000));
  EXPECT_EQ(GetCallLog().count_trace_count, 1);
}

TEST_F(OhosHiappEventTest, ReportScrollJANKWritesTraceWithScrollReason) {
  OhosHiappEventDDL ddl;
  ddl.ReportScrollJANKEvent(MakeFrameInfo(60000, 3, 42));
  EXPECT_EQ(GetCallLog().count_trace_count, 1);
  EXPECT_EQ(GetCallLog().count_trace_value, 3);
  // startAsyncTraceEx/finishAsyncTraceEx are loaded via dlsym(RTLD_DEFAULT).
  // Without -rdynamic the stubs are not visible to RTLD_DEFAULT, so the
  // function pointers may be NULL. We only verify CountTrace here.
}

TEST_F(OhosHiappEventTest, ReportScrollJANKMultipleEvents) {
  OhosHiappEventDDL ddl;
  for (int i = 0; i < 5; i++) {
    ddl.ReportScrollJANKEvent(MakeFrameInfo(50000 + i * 1000, i + 1));
  }
  EXPECT_EQ(GetCallLog().count_trace_count, 5);
}

// ===== ReportJANKEvent =====

TEST_F(OhosHiappEventTest, ReportJANKEventCallsTrace) {
  OhosHiappEventDDL ddl;
  ddl.ReportJANKEvent(MakeFrameInfo(100000, 2));
  EXPECT_EQ(GetCallLog().count_trace_count, 1);
}

TEST_F(OhosHiappEventTest, ReportJANKEventNoThreshold) {
  OhosHiappEventDDL ddl;
  // Unlike scroll jank, common jank has no 50ms threshold
  ddl.ReportJANKEvent(MakeFrameInfo(100, 1));
  EXPECT_EQ(GetCallLog().count_trace_count, 1);
}

TEST_F(OhosHiappEventTest, ReportJANKEventCapacityLimit) {
  OhosHiappEventDDL ddl;
  // Internal vector capacity is 10; WriteJANKEventToTrace is always called
  for (int i = 0; i < 15; i++) {
    ddl.ReportJANKEvent(MakeFrameInfo(100000, 1, static_cast<uint64_t>(i)));
  }
  // All 15 calls should trigger CountTrace (no threshold for common jank)
  EXPECT_EQ(GetCallLog().count_trace_count, 15);
  // Flush should not crash
  ddl.Flush();
  SUCCEED();
}

// ===== Flush / FlushScroll with no events =====

TEST_F(OhosHiappEventTest, FlushWithNoEventsDoesNotCrash) {
  OhosHiappEventDDL ddl;
  ddl.Flush();
  SUCCEED();
}

TEST_F(OhosHiappEventTest, FlushScrollWithNoEventsDoesNotCrash) {
  OhosHiappEventDDL ddl;
  ddl.FlushScroll();
  SUCCEED();
}

// ===== Full scroll cycle =====

TEST_F(OhosHiappEventTest, FullScrollCycleWithJankEvents) {
  OhosHiappEventDDL ddl;

  // Simulate scroll start
  ddl.UpdateLastFrameNumber(10);
  ddl.OnScrollStart();

  // Simulate jank frames during scroll.
  // ReportScrollJANKEvent skips WriteJANKEventToTrace when duration < 50ms.
  ddl.ReportScrollJANKEvent(MakeFrameInfo(60000, 2, 11));  // above threshold
  ddl.ReportScrollJANKEvent(MakeFrameInfo(70000, 3, 12));  // above threshold
  ddl.ReportScrollJANKEvent(MakeFrameInfo(49000, 1, 13));  // below threshold

  // Only 2 events should have triggered CountTrace (3rd is below threshold)
  EXPECT_EQ(GetCallLog().count_trace_count, 2);

  // Simulate scroll end
  ddl.UpdateLastFrameNumber(20);
  ddl.OnScrollEndAndFlush();

  EXPECT_EQ(ScrollStatus.load(), static_cast<int>(ScrollingStatus::kScrollEnd));
  EXPECT_EQ(scroll_end_frame_.load(), 20u);
}

// ===== ReportMemoryUsage =====

TEST_F(OhosHiappEventTest, ReportMemoryUsageDoesNotCrash) {
  OhosHiappEventDDL ddl;
  ddl.ReportMemoryUsage(100 * 1024 * 1024, 200 * 1024 * 1024);
  SUCCEED();
}

TEST_F(OhosHiappEventTest, ReportMemoryUsageWithZeroValues) {
  OhosHiappEventDDL ddl;
  ddl.ReportMemoryUsage(0, 0);
  SUCCEED();
}

// ===== WriteJANKEventToTrace (indirectly via ReportJANKEvent) =====

TEST_F(OhosHiappEventTest, TraceBufferOverflowHandledGracefully) {
  OhosHiappEventDDL ddl;
  // Use a very large frame number and duration to test buffer formatting
  ddl.ReportJANKEvent(MakeFrameInfo(INT64_MAX, INT_MAX, UINT64_MAX));
  EXPECT_EQ(GetCallLog().count_trace_count, 1);
  EXPECT_EQ(GetCallLog().count_trace_value, INT_MAX);
  SUCCEED();
}

}  // namespace testing
}  // namespace hiappevent
}  // namespace fml
