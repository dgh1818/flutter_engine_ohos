/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/shell/platform/ohos/ohos_logger.h"

#include <string>
#include <thread>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace flutter {
namespace testing {

namespace {

// Returns the expected return value of ohos_log for a given format result: the
// length of the formatted string (excluding the null terminator), or 0 when the
// formatted string is empty.
int ExpectedLength(const std::string& formatted) {
  return static_cast<int>(formatted.size());
}

// All valid log levels defined by OhosLogLevel.
const std::vector<OhosLogLevel>& AllLogLevels() {
  static const std::vector<OhosLogLevel> levels = {
      kOhosLogDebug, kOhosLogInfo, kOhosLogWarn, kOhosLogError, kOhosLogFatal,
  };
  return levels;
}

}  // namespace

// Verifies that ohos_log can be invoked with every supported log level and
// reports the length of the formatted message.
TEST(OhosLogger, AcceptsAllLogLevels) {
  for (OhosLogLevel level : AllLogLevels()) {
    EXPECT_EQ(ohos_log(level, "hello"), 5);
  }
}

// An empty format string produces no output, so ohos_log should take the early
// return path and yield 0.
TEST(OhosLogger, ReturnsZeroForEmptyFormatString) {
  for (OhosLogLevel level : AllLogLevels()) {
    EXPECT_EQ(ohos_log(level, ""), 0);
  }
}

// The return value must match the length of the formatted message (excluding the
// null terminator) for a plain literal.
TEST(OhosLogger, ReturnsLengthForSimpleLiteral) {
  EXPECT_EQ(ohos_log(kOhosLogInfo, "XComFlutterOHOS"),
            ExpectedLength("XComFlutterOHOS"));
}

// Exercises common printf format specifiers and checks the returned length
// against the equivalent std::string formatting.
TEST(OhosLogger, HandlesFormatSpecifiers) {
  EXPECT_EQ(ohos_log(kOhosLogInfo, "count=%d", 42),
            ExpectedLength("count=42"));

  EXPECT_EQ(ohos_log(kOhosLogInfo, "name=%s", "flutter"),
            ExpectedLength("name=flutter"));

  EXPECT_EQ(ohos_log(kOhosLogInfo, "hex=%x", 0xabcd),
            ExpectedLength("hex=abcd"));

  EXPECT_EQ(ohos_log(kOhosLogInfo, "unsigned=%u", 123u),
            ExpectedLength("unsigned=123"));

  EXPECT_EQ(ohos_log(kOhosLogInfo, "long=%ld", 100000L),
            ExpectedLength("long=100000"));
}

// Combines several arguments in a single call to ensure the va_list plumbing
// consumes every argument correctly.
TEST(OhosLogger, HandlesMultipleArguments) {
  EXPECT_EQ(ohos_log(kOhosLogInfo, "i=%d s=%s x=%x", 7, "ab", 0xff),
            ExpectedLength("i=7 s=ab x=ff"));
}

// A formatted result whose length sits just under the internal buffer boundary
// (1023 usable bytes) must be reported faithfully.
TEST(OhosLogger, HandlesLongStringWithinBuffer) {
  const int kLen = 1000;
  std::string payload(kLen, 'A');
  EXPECT_EQ(ohos_log(kOhosLogInfo, "%s", payload.c_str()), kLen);
}

// A formatted result exactly filling the usable buffer (1022 chars + NUL) is the
// largest safe input; the function must still return the true length.
TEST(OhosLogger, HandlesStringFillingBuffer) {
  const int kLen = 1022;
  std::string payload(kLen, 'B');
  EXPECT_EQ(ohos_log(kOhosLogInfo, "%s", payload.c_str()), kLen);
}

// Whitespace and punctuation should pass through unchanged.
TEST(OhosLogger, HandlesSpecialCharacters) {
  EXPECT_EQ(ohos_log(kOhosLogInfo, "line1\nline2\tcol"),
            ExpectedLength("line1\nline2\tcol"));
}

// Calling ohos_log concurrently from several threads should not crash; each
// thread reads its own pthread_self() inside the implementation.
TEST(OhosLogger, SafeToCallFromMultipleThreads) {
  const int kThreadCount = 8;
  std::vector<std::thread> threads;
  std::vector<int> results(kThreadCount, -1);

  for (int i = 0; i < kThreadCount; ++i) {
    threads.emplace_back([&results, i]() {
      results[i] = ohos_log(kOhosLogInfo, "thread=%d", i);
    });
  }
  for (auto& t : threads) {
    t.join();
  }

  for (int i = 0; i < kThreadCount; ++i) {
    EXPECT_EQ(results[i], ExpectedLength("thread=" + std::to_string(i)));
  }
}

}  // namespace testing
}  // namespace flutter
