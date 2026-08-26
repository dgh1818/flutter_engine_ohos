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

#include <hiappevent/hiappevent.h>
#include <hitrace/trace.h>
#include <unistd.h>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "flutter/fml/platform/ohos/dynamic_library_loader.h"
#include "gtest/gtest.h"

#define private public
#include "flutter/fml/platform/ohos/hiappevent/ohos_hiappevent.h"
#undef private

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
  bool create_paramlist_return_null = false;
  std::string write_domain;
  std::string write_name;
  EventType write_type = FAULT;
  std::map<std::string, std::string> string_params;
  std::map<std::string, int32_t> int32_params;
  std::map<std::string, int64_t> int64_params;
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

int32_t GetInt32Param(const char* name) {
  auto it = GetCallLog().int32_params.find(name);
  return it == GetCallLog().int32_params.end() ? -999999 : it->second;
}

int64_t GetInt64Param(const char* name) {
  auto it = GetCallLog().int64_params.find(name);
  return it == GetCallLog().int64_params.end() ? -999999LL : it->second;
}

// A fake non-null ParamList pointer used by the stub.
ParamList FakeParamList() {
  return reinterpret_cast<ParamList>(0x1234);
}

struct FakeProcessorLog {
  int create_count = 0;
  std::string create_name;
  bool create_return_null = false;
  int set_policy_count = 0;
  int set_event_count = 0;
  std::string set_event_domain;
  std::string set_event_name;
  bool set_event_realtime = false;
  int add_count = 0;
  int64_t add_return = 5;
  int destroy_count = 0;
  int report_mem_count = 0;
  int32_t report_mem_fw_type = -1;
  std::string report_mem_ver;
  std::string report_mem_desc;
  int start_trace_ex_count = 0;
  std::string start_trace_name;
  int32_t start_trace_task_id = 0;
  std::string start_trace_args;
  int finish_trace_ex_count = 0;
};

FakeProcessorLog& GetFakeLog() {
  static FakeProcessorLog log;
  return log;
}

void ResetFakeLog() {
  GetFakeLog() = FakeProcessorLog{};
}

}

// ===== Strong stub definitions of OHos native API functions =====
// These definitions are strong symbols in the main executable; at link time
// they take precedence over the same symbols in libhiappevent_ndk.z.so and
// libhitrace_ndk.z.so (standard ELF symbol interposition).

extern "C" {
ParamList OH_HiAppEvent_CreateParamList(void) {
  GetCallLog().create_paramlist_count++;
  return GetCallLog().create_paramlist_return_null ? nullptr
                                                   : FakeParamList();
}

void OH_HiAppEvent_DestroyParamList(ParamList list) {
  GetCallLog().destroy_paramlist_count++;
}

ParamList OH_HiAppEvent_AddStringParam(ParamList list,
                                       const char* name,
                                       const char* str) {
  GetCallLog().add_string_param_count++;
  if (name && str) {
    GetCallLog().string_params[name] = str;
  }
  return list;
}

ParamList OH_HiAppEvent_AddInt32Param(ParamList list,
                                      const char* name,
                                      int32_t num) {
  GetCallLog().add_int32_param_count++;
  if (name) {
    GetCallLog().int32_params[name] = num;
  }
  return list;
}

ParamList OH_HiAppEvent_AddInt64Param(ParamList list,
                                      const char* name,
                                      int64_t num) {
  GetCallLog().add_int64_param_count++;
  if (name) {
    GetCallLog().int64_params[name] = num;
  }
  return list;
}

int OH_HiAppEvent_Write(const char* domain,
                        const char* name,
                        enum EventType type,
                        const ParamList list) {
  GetCallLog().write_count++;
  GetCallLog().write_domain = domain ? domain : "";
  GetCallLog().write_name = name ? name : "";
  GetCallLog().write_type = type;
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

HiAppEvent_Processor* FakeCreateProcessor(const char* name) {
  GetFakeLog().create_count++;
  GetFakeLog().create_name = name ? name : "";
  return GetFakeLog().create_return_null
             ? nullptr
             : reinterpret_cast<HiAppEvent_Processor*>(0x5678);
}

int FakeSetReportPolicy(HiAppEvent_Processor* processor,
                        int periodReport,
                        int batchReport,
                        bool onStartReport,
                        bool onBackgroundReport) {
  GetFakeLog().set_policy_count++;
  return 0;
}

int FakeSetReportEvent(HiAppEvent_Processor* processor,
                       const char* domain,
                       const char* name,
                       bool isRealTime) {
  GetFakeLog().set_event_count++;
  GetFakeLog().set_event_domain = domain ? domain : "";
  GetFakeLog().set_event_name = name ? name : "";
  GetFakeLog().set_event_realtime = isRealTime;
  return 0;
}

int64_t FakeAddProcessor(HiAppEvent_Processor* processor) {
  GetFakeLog().add_count++;
  return GetFakeLog().add_return;
}

void FakeDestroyProcessor(HiAppEvent_Processor* processor) {
  GetFakeLog().destroy_count++;
}

void FakeReportMemAnomaly(int32_t fwType,
                          const char* fwVer,
                          const char* description) {
  GetFakeLog().report_mem_count++;
  GetFakeLog().report_mem_fw_type = fwType;
  GetFakeLog().report_mem_ver = fwVer ? fwVer : "";
  GetFakeLog().report_mem_desc = description ? description : "";
}

void FakeStartAsyncTraceEx(uint64_t level,
                           const char* name,
                           int32_t taskId,
                           const char* customCategory,
                           const char* customArgs) {
  GetFakeLog().start_trace_ex_count++;
  GetFakeLog().start_trace_name = name ? name : "";
  GetFakeLog().start_trace_task_id = taskId;
  GetFakeLog().start_trace_args = customArgs ? customArgs : "";
}

void FakeFinishAsyncTraceEx(uint64_t level, const char* name, int32_t taskId) {
  GetFakeLog().finish_trace_ex_count++;
}

// ===== Test fixture =====

class OhosHiappEventTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ResetCallLog();
    ResetFakeLog();
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

  static void InstallFakes(OhosHiappEventDDL& ddl) {
    ddl.isValid_ = true;
    ddl.createProcessorFunc_ = &FakeCreateProcessor;
    ddl.setReportPoliceFunc_ = &FakeSetReportPolicy;
    ddl.setReportEventFunc_ = &FakeSetReportEvent;
    ddl.addFunc_ = &FakeAddProcessor;
    ddl.destroyProcessor_ = &FakeDestroyProcessor;
    ddl.reportFrameworkMemAnomaly_ = &FakeReportMemAnomaly;
    ddl.startAsyncTraceExFunc_ = &FakeStartAsyncTraceEx;
    ddl.finishAsyncTraceExFunc_ = &FakeFinishAsyncTraceEx;
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

TEST_F(OhosHiappEventTest, OnScrollEndWithoutEventsStillUpdatesState) {
  OhosHiappEventDDL ddl;
  ddl.OnScrollEndAndFlush();
  EXPECT_EQ(ScrollStatus.load(), static_cast<int>(ScrollingStatus::kScrollEnd));
  EXPECT_EQ(scroll_end_frame_.load(), 0u);
  EXPECT_NE(scroll_end_time_utc_ms.load(), 0);
  EXPECT_EQ(GetCallLog().write_count, 0);
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
  EXPECT_EQ(ddl.missed_frame_infos.size(), 10u);
  ddl.Flush();
  EXPECT_TRUE(ddl.missed_frame_infos.empty());
}

// ===== Flush / FlushScroll with no events =====

TEST_F(OhosHiappEventTest, FlushWithNoEventsWritesNothing) {
  OhosHiappEventDDL ddl;
  ddl.Flush();
  EXPECT_EQ(GetCallLog().write_count, 0);
  EXPECT_TRUE(ddl.missed_frame_infos.empty());
}

TEST_F(OhosHiappEventTest, FlushScrollWithNoEventsWritesNothing) {
  OhosHiappEventDDL ddl;
  ddl.FlushScroll();
  EXPECT_EQ(GetCallLog().write_count, 0);
  EXPECT_TRUE(ddl.missed_frame_infos_scroll.empty());
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

TEST_F(OhosHiappEventTest, ReportMemoryUsageSubMegabyteTruncatesToZeroMb) {
  OhosHiappEventDDL ddl;
  InstallFakes(ddl);
  ddl.ReportMemoryUsage(512 * 1024, 512 * 1024);
  EXPECT_EQ(GetFakeLog().report_mem_count, 1);
  EXPECT_EQ(GetFakeLog().report_mem_desc,
            "Dart heap memory usage exceeds threshold: "
            "total = 1 MB (old = 0 MB, new = 0 MB)");
}

TEST_F(OhosHiappEventTest, ReportMemoryUsageWithZeroValues) {
  OhosHiappEventDDL ddl;
  InstallFakes(ddl);
  ddl.ReportMemoryUsage(0, 0);
  EXPECT_EQ(GetFakeLog().report_mem_count, 1);
  EXPECT_EQ(GetFakeLog().report_mem_desc,
            "Dart heap memory usage exceeds threshold: "
            "total = 0 MB (old = 0 MB, new = 0 MB)");
}

// ===== WriteJANKEventToTrace (indirectly via ReportJANKEvent) =====

TEST_F(OhosHiappEventTest, TraceBufferOverflowHandledGracefully) {
  OhosHiappEventDDL ddl;
  // Use a very large frame number and duration to test buffer formatting
  ddl.ReportJANKEvent(MakeFrameInfo(INT64_MAX, INT_MAX, UINT64_MAX));
  EXPECT_EQ(GetCallLog().count_trace_count, 1);
  EXPECT_EQ(GetCallLog().count_trace_value, INT_MAX);
}

TEST_F(OhosHiappEventTest, InitEarlyReturnWhenApiVersionBelow18) {
  OhosHiappEventDDL ddl;
  ddl.apiVersion_ = 17;
  ddl.isInit_ = false;
  ddl.Init();
  EXPECT_FALSE(ddl.isInit_);
}

TEST_F(OhosHiappEventTest, InitEarlyReturnWhenAlreadyInitialized) {
  OhosHiappEventDDL ddl;
  ddl.isInit_ = true;
  const bool valid_before = ddl.isValid_;
  ddl.Init();
  EXPECT_TRUE(ddl.isInit_);
  EXPECT_EQ(ddl.isValid_, valid_before);
  EXPECT_EQ(GetFakeLog().create_count, 0);
}

TEST_F(OhosHiappEventTest, InitLoadsHiTraceExFunctionsAtApi19) {
  OhosHiappEventDDL ddl;
  ddl.apiVersion_ = 19;
  ddl.isInit_ = false;
  ddl.Init();
  EXPECT_TRUE(ddl.isInit_);
}

TEST_F(OhosHiappEventTest, InitSkipsHiTraceExFunctionsAtApi18) {
  OhosHiappEventDDL ddl;
  ddl.startAsyncTraceExFunc_ = nullptr;
  ddl.finishAsyncTraceExFunc_ = nullptr;
  ddl.apiVersion_ = 18;
  ddl.isInit_ = false;
  ddl.Init();
  EXPECT_TRUE(ddl.isInit_);
  EXPECT_EQ(ddl.startAsyncTraceExFunc_, nullptr);
  EXPECT_EQ(ddl.finishAsyncTraceExFunc_, nullptr);
}

TEST_F(OhosHiappEventTest, ReportJANKEventSkipsWhenVectorOverCapacity) {
  OhosHiappEventDDL ddl;
  ddl.missed_frame_infos.resize(11);
  ddl.ReportJANKEvent(MakeFrameInfo(100000, 1, 1));
  EXPECT_EQ(GetCallLog().count_trace_count, 1);
  EXPECT_EQ(ddl.missed_frame_infos.size(), 11u);
}

TEST_F(OhosHiappEventTest, WriteSingleFrameEmptyReturnsMinusOne) {
  OhosHiappEventDDL ddl;
  EXPECT_EQ(ddl.WriteSingleFrame(), -1);
  EXPECT_EQ(GetCallLog().create_paramlist_count, 0);
}

TEST_F(OhosHiappEventTest, WriteSingleFrameNullParamListReturnsMinusOne) {
  OhosHiappEventDDL ddl;
  ddl.missed_frame_infos.push_back(MakeFrameInfo(60000));
  GetCallLog().create_paramlist_return_null = true;
  EXPECT_EQ(ddl.WriteSingleFrame(), -1);
  EXPECT_EQ(GetCallLog().write_count, 0);
}

TEST_F(OhosHiappEventTest, WriteSingleFrameWritesExpectedParams) {
  OhosHiappEventDDL ddl;
  ddl.missed_frame_infos.push_back(MakeFrameInfo(60000, 2, 7));
  EXPECT_EQ(ddl.WriteSingleFrame(), 0);
  EXPECT_EQ(GetCallLog().write_domain, "PERFORMANCE");
  EXPECT_EQ(GetCallLog().write_name, "OTHER_JANK");
  EXPECT_EQ(GetCallLog().write_type, BEHAVIOR);
  EXPECT_EQ(GetCallLog().string_params["frameworkName"], "FLUTTER");
  EXPECT_EQ(GetInt32Param("versionCode"), 0);
  EXPECT_EQ(GetInt32Param("missedFrames"), 2);
  EXPECT_EQ(GetInt64Param("startTime"), static_cast<int64_t>(99940));
  EXPECT_EQ(GetInt64Param("endTime"), static_cast<int64_t>(100000));
  EXPECT_EQ(GetCallLog().destroy_paramlist_count, 1);
}

TEST_F(OhosHiappEventTest, WriteSingleFrameEndTimeBeforeStartReturnsMinusOne) {
  OhosHiappEventDDL ddl;
  MissedFrameInfo info = MakeFrameInfo(60000);
  info.raster_finish_time_micros = 0;
  ddl.missed_frame_infos.push_back(info);
  EXPECT_EQ(ddl.WriteSingleFrame(), -1);
  EXPECT_EQ(GetCallLog().write_count, 0);
}

TEST_F(OhosHiappEventTest, WriteSingleFramePropagatesWriteError) {
  OhosHiappEventDDL ddl;
  ddl.missed_frame_infos.push_back(MakeFrameInfo(60000));
  GetCallLog().write_return_value = 7;
  EXPECT_EQ(ddl.WriteSingleFrame(), 7);
  EXPECT_EQ(GetCallLog().destroy_paramlist_count, 1);
}

TEST_F(OhosHiappEventTest, WriteStatisticFrameEmptyReturnsMinusOne) {
  OhosHiappEventDDL ddl;
  EXPECT_EQ(ddl.WriteStatisticFrame(), -1);
  EXPECT_EQ(GetCallLog().create_paramlist_count, 0);
}

TEST_F(OhosHiappEventTest, WriteStatisticFrameNullParamListReturnsMinusOne) {
  OhosHiappEventDDL ddl;
  ddl.missed_frame_infos.push_back(MakeFrameInfo(60000));
  GetCallLog().create_paramlist_return_null = true;
  EXPECT_EQ(ddl.WriteStatisticFrame(), -1);
  EXPECT_EQ(GetCallLog().write_count, 0);
}

TEST_F(OhosHiappEventTest, WriteStatisticFrameAggregatesEntries) {
  OhosHiappEventDDL ddl;
  ddl.missed_frame_infos.push_back(MakeFrameInfo(100000, 2, 1));
  ddl.missed_frame_infos.push_back(MakeFrameInfo(60000, 1, 2));
  ddl.missed_frame_infos.push_back(MakeFrameInfo(60000, 3, 3));
  EXPECT_EQ(ddl.WriteStatisticFrame(), 0);
  EXPECT_EQ(GetCallLog().write_name, "OTHER_JANK_STAT");
  EXPECT_EQ(GetCallLog().write_type, STATISTIC);
  EXPECT_EQ(GetInt32Param("maxMissedFrameRate"), 59);
  EXPECT_EQ(GetInt32Param("totalMissedFrames"), 6);
  EXPECT_EQ(GetInt64Param("maxFrameTime"), static_cast<int64_t>(100));
  EXPECT_EQ(GetInt64Param("startTime"), static_cast<int64_t>(99900));
  EXPECT_EQ(GetInt64Param("endTime"), static_cast<int64_t>(99902));
}

TEST_F(OhosHiappEventTest, WriteStatisticFrameZeroBudgetYieldsZeroRate) {
  OhosHiappEventDDL ddl;
  MissedFrameInfo a = MakeFrameInfo(100000, 2, 1);
  a.frame_budget_time_micros = 0;
  MissedFrameInfo b = MakeFrameInfo(60000, 1, 2);
  b.frame_budget_time_micros = 0;
  ddl.missed_frame_infos.push_back(a);
  ddl.missed_frame_infos.push_back(b);
  EXPECT_EQ(ddl.WriteStatisticFrame(), 0);
  EXPECT_EQ(GetInt32Param("maxMissedFrameRate"), 0);
}

TEST_F(OhosHiappEventTest, WriteStatisticFramePropagatesWriteError) {
  OhosHiappEventDDL ddl;
  ddl.missed_frame_infos.push_back(MakeFrameInfo(60000));
  GetCallLog().write_return_value = 7;
  EXPECT_EQ(ddl.WriteStatisticFrame(), 7);
}

TEST_F(OhosHiappEventTest, WriteScrolledFrameEmptyReturnsMinusOne) {
  OhosHiappEventDDL ddl;
  EXPECT_EQ(ddl.WriteScrolledFrame(), -1);
  EXPECT_EQ(GetCallLog().create_paramlist_count, 0);
}

TEST_F(OhosHiappEventTest, WriteScrolledFrameNullParamListReturnsMinusOne) {
  OhosHiappEventDDL ddl;
  ddl.missed_frame_infos_scroll.push_back(MakeFrameInfo(60000));
  GetCallLog().create_paramlist_return_null = true;
  EXPECT_EQ(ddl.WriteScrolledFrame(), -1);
  EXPECT_EQ(GetCallLog().write_count, 0);
}

TEST_F(OhosHiappEventTest, WriteScrolledFrameUsesGlobalFrameRangeAndTimes) {
  OhosHiappEventDDL ddl;
  scroll_start_frame_.store(100);
  scroll_end_frame_.store(150);
  scroll_start_time_utc_ms.store(5000);
  scroll_end_time_utc_ms.store(6000);
  ddl.missed_frame_infos_scroll.push_back(MakeFrameInfo(100000, 2, 11));
  ddl.missed_frame_infos_scroll.push_back(MakeFrameInfo(60000, 1, 12));
  EXPECT_EQ(ddl.WriteScrolledFrame(), 0);
  EXPECT_EQ(GetCallLog().write_name, "OTHER_JANK_SCROLL");
  EXPECT_EQ(GetCallLog().write_type, BEHAVIOR);
  EXPECT_EQ(GetInt32Param("totalFrames"), 51);
  EXPECT_EQ(GetInt32Param("totalMissedFrames"), 3);
  EXPECT_EQ(GetInt32Param("maxMissedFrameRate"), 59);
  EXPECT_EQ(GetInt64Param("frameId"), static_cast<int64_t>(11));
  EXPECT_EQ(GetInt64Param("maxFrameTime"), static_cast<int64_t>(100));
  EXPECT_EQ(GetInt64Param("startTime"), static_cast<int64_t>(5000));
  EXPECT_EQ(GetInt64Param("endTime"), static_cast<int64_t>(6000));
  EXPECT_EQ(scroll_start_frame_.load(), 0u);
  EXPECT_EQ(scroll_end_frame_.load(), 0u);
  EXPECT_EQ(scroll_start_time_utc_ms.load(), 0);
  EXPECT_EQ(scroll_end_time_utc_ms.load(), 0);
}

TEST_F(OhosHiappEventTest, WriteScrolledFrameZeroStartFrameGivesZeroTotal) {
  OhosHiappEventDDL ddl;
  ddl.missed_frame_infos_scroll.push_back(MakeFrameInfo(60000));
  EXPECT_EQ(ddl.WriteScrolledFrame(), 0);
  EXPECT_EQ(GetInt32Param("totalFrames"), 0);
}

TEST_F(OhosHiappEventTest, WriteScrolledFrameEndBeforeStartGivesZeroTotal) {
  OhosHiappEventDDL ddl;
  scroll_start_frame_.store(150);
  scroll_end_frame_.store(100);
  ddl.missed_frame_infos_scroll.push_back(MakeFrameInfo(60000));
  EXPECT_EQ(ddl.WriteScrolledFrame(), 0);
  EXPECT_EQ(GetInt32Param("totalFrames"), 0);
}

TEST_F(OhosHiappEventTest, WriteScrolledFrameClampsRangeToInt32Max) {
  OhosHiappEventDDL ddl;
  scroll_start_frame_.store(1);
  scroll_end_frame_.store(UINT64_MAX);
  ddl.missed_frame_infos_scroll.push_back(MakeFrameInfo(60000));
  EXPECT_EQ(ddl.WriteScrolledFrame(), 0);
  EXPECT_EQ(GetInt32Param("totalFrames"), INT32_MAX);
}

TEST_F(OhosHiappEventTest, WriteScrolledFrameFallsBackToFrameInfoTimes) {
  OhosHiappEventDDL ddl;
  MissedFrameInfo info = MakeFrameInfo(60000, 2, 11);
  info.frame_budget_time_micros = 0;
  ddl.missed_frame_infos_scroll.push_back(info);
  EXPECT_EQ(ddl.WriteScrolledFrame(), 0);
  EXPECT_EQ(GetInt64Param("startTime"), static_cast<int64_t>(99940));
  EXPECT_EQ(GetInt64Param("endTime"), static_cast<int64_t>(99942));
  EXPECT_EQ(GetInt32Param("maxMissedFrameRate"), 0);
  EXPECT_EQ(GetInt64Param("frameId"), static_cast<int64_t>(11));
}

TEST_F(OhosHiappEventTest, WriteScrolledFramePropagatesWriteError) {
  OhosHiappEventDDL ddl;
  ddl.missed_frame_infos_scroll.push_back(MakeFrameInfo(60000));
  GetCallLog().write_return_value = 7;
  EXPECT_EQ(ddl.WriteScrolledFrame(), 7);
}

TEST_F(OhosHiappEventTest, TraceExFunctionsCalledWhenLoaded) {
  OhosHiappEventDDL ddl;
  InstallFakes(ddl);
  ddl.ReportJANKEvent(MakeFrameInfo(60000, 2, 9));
  EXPECT_EQ(GetFakeLog().start_trace_ex_count, 1);
  EXPECT_EQ(GetFakeLog().start_trace_name, "Flutter Hitch Time");
  EXPECT_EQ(GetFakeLog().start_trace_task_id, 9);
  EXPECT_EQ(GetFakeLog().start_trace_args,
            "frame_number=9,dropped=2,duration=60.00ms,reason=common");
  EXPECT_EQ(GetFakeLog().finish_trace_ex_count, 1);
  EXPECT_EQ(GetCallLog().count_trace_count, 1);
}

TEST_F(OhosHiappEventTest, TraceExFunctionsSkippedWhenMissing) {
  OhosHiappEventDDL ddl;
  ddl.startAsyncTraceExFunc_ = nullptr;
  ddl.finishAsyncTraceExFunc_ = nullptr;
  ddl.ReportJANKEvent(MakeFrameInfo(60000, 1, 5));
  EXPECT_EQ(GetFakeLog().start_trace_ex_count, 0);
  EXPECT_EQ(GetCallLog().count_trace_count, 1);
}

TEST_F(OhosHiappEventTest, FlushAllInSkippedWhenNotValid) {
  OhosHiappEventDDL ddl;
  ddl.isValid_ = false;
  ddl.missed_frame_infos.push_back(MakeFrameInfo(60000));
  ddl.FlushAllIn(OhosHiappEventFlag::kSingleFlag);
  EXPECT_EQ(GetFakeLog().create_count, 0);
  EXPECT_EQ(GetCallLog().write_count, 0);
}

TEST_F(OhosHiappEventTest, FlushAllInSkippedWhenVectorEmpty) {
  OhosHiappEventDDL ddl;
  InstallFakes(ddl);
  ddl.FlushAllIn(OhosHiappEventFlag::kSingleFlag);
  EXPECT_EQ(GetFakeLog().create_count, 0);
}

TEST_F(OhosHiappEventTest, FlushAllInSkippedWhenScrollVectorEmpty) {
  OhosHiappEventDDL ddl;
  InstallFakes(ddl);
  ddl.FlushAllIn(OhosHiappEventFlag::kScrolledFlag);
  EXPECT_EQ(GetFakeLog().create_count, 0);
}

TEST_F(OhosHiappEventTest, FlushAllInSingleFlagReportsOtherJank) {
  OhosHiappEventDDL ddl;
  InstallFakes(ddl);
  ddl.missed_frame_infos.push_back(MakeFrameInfo(60000, 2, 1));
  ddl.FlushAllIn(OhosHiappEventFlag::kSingleFlag);
  EXPECT_EQ(GetFakeLog().create_count, 1);
  EXPECT_EQ(GetFakeLog().create_name, "xperfbridge");
  EXPECT_EQ(GetFakeLog().set_policy_count, 1);
  EXPECT_EQ(GetFakeLog().set_event_count, 1);
  EXPECT_EQ(GetFakeLog().set_event_domain, "PERFORMANCE");
  EXPECT_EQ(GetFakeLog().set_event_name, "OTHER_JANK");
  EXPECT_TRUE(GetFakeLog().set_event_realtime);
  EXPECT_EQ(GetFakeLog().add_count, 1);
  EXPECT_EQ(GetFakeLog().destroy_count, 1);
  EXPECT_EQ(GetCallLog().write_name, "OTHER_JANK");
  EXPECT_EQ(ddl.missed_frame_infos.size(), 1u);
}

TEST_F(OhosHiappEventTest, FlushAllInStaticFlagReportsOtherJankStat) {
  OhosHiappEventDDL ddl;
  InstallFakes(ddl);
  ddl.missed_frame_infos.push_back(MakeFrameInfo(60000));
  ddl.FlushAllIn(OhosHiappEventFlag::kStaticFlag);
  EXPECT_EQ(GetFakeLog().set_event_name, "OTHER_JANK_STAT");
  EXPECT_EQ(GetCallLog().write_name, "OTHER_JANK_STAT");
  EXPECT_EQ(GetFakeLog().destroy_count, 1);
}

TEST_F(OhosHiappEventTest, FlushAllInScrolledFlagReportsOtherJankScroll) {
  OhosHiappEventDDL ddl;
  InstallFakes(ddl);
  ddl.missed_frame_infos_scroll.push_back(MakeFrameInfo(60000));
  ddl.FlushAllIn(OhosHiappEventFlag::kScrolledFlag);
  EXPECT_EQ(GetFakeLog().set_event_name, "OTHER_JANK_SCROLL");
  EXPECT_EQ(GetCallLog().write_name, "OTHER_JANK_SCROLL");
  EXPECT_EQ(GetFakeLog().destroy_count, 1);
}

TEST_F(OhosHiappEventTest, FlushAllInUnknownFlagTakesDefaultPaths) {
  OhosHiappEventDDL ddl;
  InstallFakes(ddl);
  ddl.missed_frame_infos.push_back(MakeFrameInfo(60000));
  ddl.FlushAllIn(static_cast<OhosHiappEventFlag>(99));
  EXPECT_EQ(GetFakeLog().set_event_count, 0);
  EXPECT_EQ(GetCallLog().write_count, 0);
  EXPECT_EQ(GetFakeLog().destroy_count, 1);
}

TEST_F(OhosHiappEventTest, FlushAllInSkippedWhenProcessorNull) {
  OhosHiappEventDDL ddl;
  InstallFakes(ddl);
  GetFakeLog().create_return_null = true;
  ddl.missed_frame_infos.push_back(MakeFrameInfo(60000));
  ddl.FlushAllIn(OhosHiappEventFlag::kSingleFlag);
  EXPECT_EQ(GetFakeLog().set_policy_count, 0);
  EXPECT_EQ(GetCallLog().write_count, 0);
}

TEST_F(OhosHiappEventTest, FlushAllInDestroyWhenProcessorIdInvalid) {
  OhosHiappEventDDL ddl;
  InstallFakes(ddl);
  GetFakeLog().add_return = 0;
  ddl.missed_frame_infos.push_back(MakeFrameInfo(60000));
  ddl.FlushAllIn(OhosHiappEventFlag::kSingleFlag);
  EXPECT_EQ(GetFakeLog().add_count, 1);
  EXPECT_EQ(GetFakeLog().destroy_count, 1);
  EXPECT_EQ(GetCallLog().write_count, 0);
}

TEST_F(OhosHiappEventTest, FlushWritesAndClearsPendingEvents) {
  OhosHiappEventDDL ddl;
  InstallFakes(ddl);
  ddl.missed_frame_infos.push_back(MakeFrameInfo(60000, 1, 1));
  ddl.missed_frame_infos.push_back(MakeFrameInfo(70000, 2, 2));
  ddl.Flush();
  EXPECT_EQ(GetCallLog().write_count, 2);
  EXPECT_TRUE(ddl.missed_frame_infos.empty());
}

TEST_F(OhosHiappEventTest, FlushScrollWritesAndClearsScrollEvents) {
  OhosHiappEventDDL ddl;
  InstallFakes(ddl);
  ddl.missed_frame_infos_scroll.push_back(MakeFrameInfo(60000));
  ddl.FlushScroll();
  EXPECT_EQ(GetCallLog().write_count, 1);
  EXPECT_EQ(GetCallLog().write_name, "OTHER_JANK_SCROLL");
  EXPECT_TRUE(ddl.missed_frame_infos_scroll.empty());
}

TEST_F(OhosHiappEventTest, ReportMemoryUsageInvokesFrameworkAnomalyCallback) {
  OhosHiappEventDDL ddl;
  InstallFakes(ddl);
  ddl.ReportMemoryUsage(2 * 1024 * 1024, 3 * 1024 * 1024);
  EXPECT_EQ(GetFakeLog().report_mem_count, 1);
  EXPECT_EQ(GetFakeLog().report_mem_fw_type, 0);
  EXPECT_EQ(GetFakeLog().report_mem_ver,
            std::string(OhosHiappEventDDL::GetFlutterVersion()));
  EXPECT_EQ(GetFakeLog().report_mem_desc,
            "Dart heap memory usage exceeds threshold: "
            "total = 5 MB (old = 2 MB, new = 3 MB)");
}

TEST_F(OhosHiappEventTest, ReportMemoryUsageSkippedWhenCallbackMissing) {
  OhosHiappEventDDL ddl;
  ddl.reportFrameworkMemAnomaly_ = nullptr;
  ddl.ReportMemoryUsage(1024 * 1024, 1024 * 1024);
  EXPECT_EQ(GetFakeLog().report_mem_count, 0);
}

TEST_F(OhosHiappEventTest, TraceExSkippedWhenOnlyStartFuncLoaded) {
  OhosHiappEventDDL ddl;
  InstallFakes(ddl);
  ddl.finishAsyncTraceExFunc_ = nullptr;
  ddl.ReportJANKEvent(MakeFrameInfo(60000, 2, 5));
  EXPECT_EQ(GetFakeLog().start_trace_ex_count, 0);
  EXPECT_EQ(GetFakeLog().finish_trace_ex_count, 0);
  EXPECT_EQ(GetCallLog().count_trace_count, 1);
}

TEST_F(OhosHiappEventTest, WriteStatisticFrameLaterEntryIsMax) {
  OhosHiappEventDDL ddl;
  ddl.missed_frame_infos.push_back(MakeFrameInfo(60000, 1, 1));
  ddl.missed_frame_infos.push_back(MakeFrameInfo(120000, 4, 2));
  EXPECT_EQ(ddl.WriteStatisticFrame(), 0);
  EXPECT_EQ(GetInt64Param("maxFrameTime"), static_cast<int64_t>(120));
  EXPECT_EQ(GetInt32Param("totalMissedFrames"), 5);
  EXPECT_EQ(GetInt64Param("startTime"), static_cast<int64_t>(99940));
  EXPECT_EQ(GetInt64Param("endTime"), static_cast<int64_t>(99942));
}

TEST_F(OhosHiappEventTest, FlushAllInWriteErrorStillDestroysProcessor) {
  OhosHiappEventDDL ddl;
  InstallFakes(ddl);
  GetCallLog().write_return_value = 7;
  ddl.missed_frame_infos.push_back(MakeFrameInfo(60000, 1, 1));
  ddl.FlushAllIn(OhosHiappEventFlag::kSingleFlag);
  EXPECT_EQ(GetCallLog().write_count, 1);
  EXPECT_EQ(GetFakeLog().destroy_count, 1);
}

TEST_F(OhosHiappEventTest, ReportJANKEventRejectsPushAtExactCapacity) {
  OhosHiappEventDDL ddl;
  for (int i = 0; i < 10; i++) {
    ddl.ReportJANKEvent(MakeFrameInfo(60000, 1, static_cast<uint64_t>(i)));
  }
  EXPECT_EQ(ddl.missed_frame_infos.size(), 10u);
  ddl.ReportJANKEvent(MakeFrameInfo(60000, 1, 10));
  EXPECT_EQ(ddl.missed_frame_infos.size(), 10u);
  EXPECT_EQ(GetCallLog().count_trace_count, 11);
}

TEST_F(OhosHiappEventTest, WriteScrolledFrameSingleFrameWindow) {
  OhosHiappEventDDL ddl;
  scroll_start_frame_.store(7);
  scroll_end_frame_.store(7);
  scroll_start_time_utc_ms.store(4000);
  scroll_end_time_utc_ms.store(4100);
  ddl.missed_frame_infos_scroll.push_back(MakeFrameInfo(60000, 1, 7));
  EXPECT_EQ(ddl.WriteScrolledFrame(), 0);
  EXPECT_EQ(GetInt32Param("totalFrames"), 1);
  EXPECT_EQ(GetInt64Param("frameId"), static_cast<int64_t>(7));
  EXPECT_EQ(GetInt64Param("startTime"), static_cast<int64_t>(4000));
  EXPECT_EQ(GetInt64Param("endTime"), static_cast<int64_t>(4100));
  EXPECT_EQ(scroll_start_frame_.load(), 0u);
}

}  // namespace testing
}  // namespace hiappevent
}  // namespace fml
