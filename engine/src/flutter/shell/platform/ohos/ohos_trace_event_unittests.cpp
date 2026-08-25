/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/fml/trace_event.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "gtest/gtest.h"

// ===== Stub implementations of OHos HiTrace native API functions =====
//
// OHOSTraceTimelineEvent and OHOSTraceEventEnd return void, so the only way to
// verify their behaviour is to intercept the OH_HiTrace_* calls they make.
// These stubs are strong definitions in the main executable; at link time they
// take precedence over the same symbols in libhitrace_ndk.z.so (standard ELF
// symbol interposition), allowing tests to assert which function was called and
// with what arguments.

namespace {

enum class HiTraceCallType {
  kNone,
  kStartTrace,
  kFinishTrace,
  kStartAsyncTrace,
  kFinishAsyncTrace,
};

struct HiTraceCall {
  HiTraceCallType type = HiTraceCallType::kNone;
  std::string name;
  int32_t task_id = 0;
};

// TRACE_EVENT0 fires on every ConcurrentMessageLoop worker wakeup, so
// these stubs are hit from 12+ threads at once; the call log must be
// mutex-guarded or vector growth double-frees.
std::mutex& GetTraceCallsMutex() {
  static std::mutex mutex;
  return mutex;
}

std::vector<HiTraceCall>& GetTraceCalls() {
  static std::vector<HiTraceCall> calls;
  return calls;
}

void ClearTraceCalls() {
  std::lock_guard<std::mutex> lock(GetTraceCallsMutex());
  GetTraceCalls().clear();
}

size_t TraceCallCount() {
  std::lock_guard<std::mutex> lock(GetTraceCallsMutex());
  return GetTraceCalls().size();
}

const HiTraceCall LastTraceCall() {
  std::lock_guard<std::mutex> lock(GetTraceCallsMutex());
  const auto& calls = GetTraceCalls();
  static const HiTraceCall kEmpty;
  return calls.empty() ? kEmpty : calls.back();
}

}  // namespace

extern "C" {
void OH_HiTrace_StartTrace(const char* name) {
  std::lock_guard<std::mutex> lock(GetTraceCallsMutex());
  GetTraceCalls().push_back(
      {HiTraceCallType::kStartTrace, name ? name : "", 0});
}

void OH_HiTrace_FinishTrace(void) {
  std::lock_guard<std::mutex> lock(GetTraceCallsMutex());
  GetTraceCalls().push_back({HiTraceCallType::kFinishTrace, "", 0});
}

void OH_HiTrace_StartAsyncTrace(const char* name, int32_t taskId) {
  std::lock_guard<std::mutex> lock(GetTraceCallsMutex());
  GetTraceCalls().push_back(
      {HiTraceCallType::kStartAsyncTrace, name ? name : "", taskId});
}

void OH_HiTrace_FinishAsyncTrace(const char* name, int32_t taskId) {
  std::lock_guard<std::mutex> lock(GetTraceCallsMutex());
  GetTraceCalls().push_back(
      {HiTraceCallType::kFinishAsyncTrace, name ? name : "", taskId});
}

}  // extern "C"

namespace flutter {
namespace testing {

class OhosTraceEventTest : public ::testing::Test {
 protected:
  void SetUp() override { ClearTraceCalls(); }
};

// ---- Begin event type ----

TEST_F(OhosTraceEventTest, BeginEventCallsStartTrace) {
  fml::tracing::OHOSTraceTimelineEvent(
      "flutter", "Frame", /*timestamp_micros=*/1000, /*id=*/0,
      Dart_Timeline_Event_Begin, /*argument_count=*/0,
      /*argument_names=*/nullptr, /*argument_values=*/nullptr);

  EXPECT_EQ(TraceCallCount(), 1u);
  EXPECT_EQ(LastTraceCall().type, HiTraceCallType::kStartTrace);
  EXPECT_EQ(LastTraceCall().name, "flutter::Frame");
}

TEST_F(OhosTraceEventTest, BeginEventCallsStartTraceWithoutTimestamp) {
  fml::tracing::OHOSTraceTimelineEvent(
      "flutter", "Frame", /*id=*/0, Dart_Timeline_Event_Begin,
      /*argument_count=*/0, /*argument_names=*/nullptr,
      /*argument_values=*/nullptr);

  EXPECT_EQ(TraceCallCount(), 1u);
  EXPECT_EQ(LastTraceCall().type, HiTraceCallType::kStartTrace);
  EXPECT_EQ(LastTraceCall().name, "flutter::Frame");
}

// ---- Async begin / end ----

TEST_F(OhosTraceEventTest, AsyncBeginEventCallsStartAsyncTrace) {
  fml::tracing::OHOSTraceTimelineEvent(
      "flutter", "Frame", /*timestamp_micros=*/0, /*id=*/42,
      Dart_Timeline_Event_Async_Begin, /*argument_count=*/0,
      /*argument_names=*/nullptr, /*argument_values=*/nullptr);

  EXPECT_EQ(TraceCallCount(), 1u);
  EXPECT_EQ(LastTraceCall().type, HiTraceCallType::kStartAsyncTrace);
  EXPECT_EQ(LastTraceCall().name, "flutter::Frame");
  EXPECT_EQ(LastTraceCall().task_id, 42);
}

TEST_F(OhosTraceEventTest, AsyncEndEventCallsFinishAsyncTrace) {
  fml::tracing::OHOSTraceTimelineEvent(
      "flutter", "Frame", /*timestamp_micros=*/0, /*id=*/42,
      Dart_Timeline_Event_Async_End, /*argument_count=*/0,
      /*argument_names=*/nullptr, /*argument_values=*/nullptr);

  EXPECT_EQ(TraceCallCount(), 1u);
  EXPECT_EQ(LastTraceCall().type, HiTraceCallType::kFinishAsyncTrace);
  EXPECT_EQ(LastTraceCall().name, "flutter::Frame");
  EXPECT_EQ(LastTraceCall().task_id, 42);
}

// ---- Flow begin / end (mapped to async trace calls) ----

TEST_F(OhosTraceEventTest, FlowBeginEventCallsStartAsyncTrace) {
  fml::tracing::OHOSTraceTimelineEvent(
      "flutter", "Flow", /*timestamp_micros=*/0, /*id=*/7,
      Dart_Timeline_Event_Flow_Begin, /*argument_count=*/0,
      /*argument_names=*/nullptr, /*argument_values=*/nullptr);

  EXPECT_EQ(TraceCallCount(), 1u);
  EXPECT_EQ(LastTraceCall().type, HiTraceCallType::kStartAsyncTrace);
  EXPECT_EQ(LastTraceCall().name, "flutter::Flow");
  EXPECT_EQ(LastTraceCall().task_id, 7);
}

TEST_F(OhosTraceEventTest, FlowEndEventCallsFinishAsyncTrace) {
  fml::tracing::OHOSTraceTimelineEvent(
      "flutter", "Flow", /*timestamp_micros=*/0, /*id=*/7,
      Dart_Timeline_Event_Flow_End, /*argument_count=*/0,
      /*argument_names=*/nullptr, /*argument_values=*/nullptr);

  EXPECT_EQ(TraceCallCount(), 1u);
  EXPECT_EQ(LastTraceCall().type, HiTraceCallType::kFinishAsyncTrace);
  EXPECT_EQ(LastTraceCall().name, "flutter::Flow");
  EXPECT_EQ(LastTraceCall().task_id, 7);
}

// ---- Filtered event types produce no trace calls ----

TEST_F(OhosTraceEventTest, FilteredEventTypesProduceNoCalls) {
  // clang-format off
  const Dart_Timeline_Event_Type filtered_types[] = {
      Dart_Timeline_Event_End,
      Dart_Timeline_Event_Instant,
      Dart_Timeline_Event_Duration,
      Dart_Timeline_Event_Async_Instant,
      Dart_Timeline_Event_Counter,
      Dart_Timeline_Event_Flow_Step,
  };
  // clang-format on
  for (auto type : filtered_types) {
    ClearTraceCalls();
    fml::tracing::OHOSTraceTimelineEvent("flutter", "Frame", 0, 0, type, 0,
                                         nullptr, nullptr);
    EXPECT_EQ(TraceCallCount(), 0u)
        << "Failed for event type " << static_cast<int>(type);
  }
}

// ---- PointerEvent filtering ----

TEST_F(OhosTraceEventTest, PointerEventWithAsyncBeginIsFiltered) {
  fml::tracing::OHOSTraceTimelineEvent("flutter", "PointerEvent", 0, 1,
                                       Dart_Timeline_Event_Async_Begin, 0,
                                       nullptr, nullptr);
  EXPECT_EQ(TraceCallCount(), 0u);
}

TEST_F(OhosTraceEventTest, PointerEventWithBeginIsNotFiltered) {
  fml::tracing::OHOSTraceTimelineEvent("flutter", "PointerEvent", 0, 0,
                                       Dart_Timeline_Event_Begin, 0, nullptr,
                                       nullptr);
  EXPECT_EQ(TraceCallCount(), 1u);
  EXPECT_EQ(LastTraceCall().type, HiTraceCallType::kStartTrace);
  EXPECT_EQ(LastTraceCall().name, "flutter::PointerEvent");
}

// ---- SceneDisplayLag argument suppression ----

TEST_F(OhosTraceEventTest, SceneDisplayLagWithAsyncBeginIgnoresArguments) {
  const char* names[] = {"key1", "key2"};
  const char* values[] = {"val1", "val2"};
  fml::tracing::OHOSTraceTimelineEvent("flutter", "SceneDisplayLag", 0, 1,
                                       Dart_Timeline_Event_Async_Begin, 2,
                                       names, values);

  EXPECT_EQ(TraceCallCount(), 1u);
  EXPECT_EQ(LastTraceCall().type, HiTraceCallType::kStartAsyncTrace);
  // Arguments must NOT be appended for non-Begin SceneDisplayLag.
  EXPECT_EQ(LastTraceCall().name, "flutter::SceneDisplayLag");
}

TEST_F(OhosTraceEventTest, SceneDisplayLagWithBeginAppendsArguments) {
  const char* names[] = {"key1", "key2"};
  const char* values[] = {"val1", "val2"};
  fml::tracing::OHOSTraceTimelineEvent("flutter", "SceneDisplayLag", 0, 0,
                                       Dart_Timeline_Event_Begin, 2, names,
                                       values);

  EXPECT_EQ(TraceCallCount(), 1u);
  EXPECT_EQ(LastTraceCall().type, HiTraceCallType::kStartTrace);
  EXPECT_EQ(LastTraceCall().name,
            "flutter::SceneDisplayLag key1:val1 key2:val2");
}

// ---- Argument appending ----

TEST_F(OhosTraceEventTest, ArgumentsAreAppendedToTraceName) {
  const char* names[] = {"count", "label"};
  const char* values[] = {"10", "hello"};
  fml::tracing::OHOSTraceTimelineEvent(
      "flutter", "Frame", 0, 0, Dart_Timeline_Event_Begin, 2, names, values);

  EXPECT_EQ(TraceCallCount(), 1u);
  EXPECT_EQ(LastTraceCall().name, "flutter::Frame count:10 label:hello");
}

TEST_F(OhosTraceEventTest, NullArgumentsWithPositiveCountDoesNotAppend) {
  fml::tracing::OHOSTraceTimelineEvent(
      "flutter", "Frame", 0, 0, Dart_Timeline_Event_Begin, 3, nullptr, nullptr);

  EXPECT_EQ(TraceCallCount(), 1u);
  EXPECT_EQ(LastTraceCall().name, "flutter::Frame");
}

TEST_F(OhosTraceEventTest, ZeroArgumentCountProducesNoArgumentSuffix) {
  const char* names[] = {"unused"};
  const char* values[] = {"unused"};
  fml::tracing::OHOSTraceTimelineEvent(
      "flutter", "Frame", 0, 0, Dart_Timeline_Event_Begin, 0, names, values);

  EXPECT_EQ(TraceCallCount(), 1u);
  EXPECT_EQ(LastTraceCall().name, "flutter::Frame");
}

TEST_F(OhosTraceEventTest, MultipleArgumentsAppendedInOrder) {
  const char* names[] = {"a", "b", "c"};
  const char* values[] = {"1", "2", "3"};
  fml::tracing::OHOSTraceTimelineEvent(
      "flutter", "Frame", 0, 0, Dart_Timeline_Event_Begin, 3, names, values);

  EXPECT_EQ(LastTraceCall().name, "flutter::Frame a:1 b:2 c:3");
}

// ---- OHOSTraceEventEnd ----

TEST_F(OhosTraceEventTest, EndCallsFinishTrace) {
  fml::tracing::OHOSTraceEventEnd();

  EXPECT_EQ(TraceCallCount(), 1u);
  EXPECT_EQ(LastTraceCall().type, HiTraceCallType::kFinishTrace);
}

// ---- Edge cases ----

TEST_F(OhosTraceEventTest, EmptyCategoryAndNameProduceScopeSeparator) {
  fml::tracing::OHOSTraceTimelineEvent("", "", 0, 0, Dart_Timeline_Event_Begin,
                                       0, nullptr, nullptr);

  EXPECT_EQ(TraceCallCount(), 1u);
  EXPECT_EQ(LastTraceCall().name, "::");
}

TEST_F(OhosTraceEventTest, SevenArgOverloadMatchesEightArgOverload) {
  const char* names[] = {"x"};
  const char* values[] = {"42"};
  fml::tracing::OHOSTraceTimelineEvent("cat", "evt", /*id=*/5,
                                       Dart_Timeline_Event_Async_Begin, 1,
                                       names, values);

  EXPECT_EQ(TraceCallCount(), 1u);
  EXPECT_EQ(LastTraceCall().type, HiTraceCallType::kStartAsyncTrace);
  EXPECT_EQ(LastTraceCall().name, "cat::evt x:42");
  EXPECT_EQ(LastTraceCall().task_id, 5);
}

// ---- 7-arg overload: exercise branches not covered by tests above ----

TEST_F(OhosTraceEventTest, SevenArgOverloadFilteredEventTypesProduceNoCalls) {
  // clang-format off
  const Dart_Timeline_Event_Type filtered_types[] = {
      Dart_Timeline_Event_End,
      Dart_Timeline_Event_Instant,
      Dart_Timeline_Event_Duration,
      Dart_Timeline_Event_Async_Instant,
      Dart_Timeline_Event_Counter,
      Dart_Timeline_Event_Flow_Step,
  };
  // clang-format on
  for (auto type : filtered_types) {
    ClearTraceCalls();
    fml::tracing::OHOSTraceTimelineEvent("flutter", "Frame", 0, type, 0,
                                         nullptr, nullptr);
    EXPECT_EQ(TraceCallCount(), 0u)
        << "Failed for event type " << static_cast<int>(type);
  }
}

TEST_F(OhosTraceEventTest,
       SevenArgOverloadPointerEventWithAsyncBeginIsFiltered) {
  fml::tracing::OHOSTraceTimelineEvent("flutter", "PointerEvent", 1,
                                       Dart_Timeline_Event_Async_Begin, 0,
                                       nullptr, nullptr);
  EXPECT_EQ(TraceCallCount(), 0u);
}

TEST_F(OhosTraceEventTest, SevenArgOverloadSceneDisplayLagIgnoresArguments) {
  const char* names[] = {"k1"};
  const char* values[] = {"v1"};
  fml::tracing::OHOSTraceTimelineEvent("flutter", "SceneDisplayLag", 1,
                                       Dart_Timeline_Event_Async_Begin, 1,
                                       names, values);
  EXPECT_EQ(TraceCallCount(), 1u);
  EXPECT_EQ(LastTraceCall().name, "flutter::SceneDisplayLag");
}

TEST_F(OhosTraceEventTest, SevenArgOverloadAsyncEndCallsFinishAsyncTrace) {
  fml::tracing::OHOSTraceTimelineEvent("flutter", "Frame", 9,
                                       Dart_Timeline_Event_Async_End, 0,
                                       nullptr, nullptr);
  EXPECT_EQ(TraceCallCount(), 1u);
  EXPECT_EQ(LastTraceCall().type, HiTraceCallType::kFinishAsyncTrace);
  EXPECT_EQ(LastTraceCall().task_id, 9);
}

TEST_F(OhosTraceEventTest, SevenArgOverloadFlowEndCallsFinishAsyncTrace) {
  fml::tracing::OHOSTraceTimelineEvent(
      "flutter", "Flow", 3, Dart_Timeline_Event_Flow_End, 0, nullptr, nullptr);
  EXPECT_EQ(TraceCallCount(), 1u);
  EXPECT_EQ(LastTraceCall().type, HiTraceCallType::kFinishAsyncTrace);
  EXPECT_EQ(LastTraceCall().task_id, 3);
}

}  // namespace testing
}  // namespace flutter
