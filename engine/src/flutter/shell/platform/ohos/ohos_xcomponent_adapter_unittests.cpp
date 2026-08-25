/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

// Unit tests for the multi-view id routing added to XComponentBase:
//   - IsSubViewId        (purely lexical decimal-view-id parse)
//   - ResolveSubViewRouting (lexical parse + live-controller liveness check)
//
// Both are private; `#define private public` exposes them (established OHOS UT
// pattern). The napi/engine-dependent branches (AttachFlutterEngine surface
// callbacks etc.) are out of UT reach and are covered on-device.

#include "flutter/fml/build_config.h"  // IWYU pragma: keep  (defines FML_OS_OHOS)

#if defined(FML_OS_OHOS)

#define private public
#include "flutter/shell/platform/ohos/ohos_xcomponent_adapter.h"
#undef private

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace flutter {
namespace testing {

// ---------------------------------------------------------------------------
// IsSubViewId
// ---------------------------------------------------------------------------

TEST(IsSubViewIdTest, EmptyStringIsNotSubView) {
  int64_t out = -1;
  EXPECT_FALSE(XComponentBase::IsSubViewId("", &out));
}

TEST(IsSubViewIdTest, MainWindowPrefixIsNotSubView) {
  int64_t out = -1;
  EXPECT_FALSE(XComponentBase::IsSubViewId("oh_flutter_0", &out));
  EXPECT_FALSE(XComponentBase::IsSubViewId("oh_flutter_1", &out));
  EXPECT_FALSE(XComponentBase::IsSubViewId("oh_flutter_abc", &out));
}

TEST(IsSubViewIdTest, NonDigitContentRejected) {
  int64_t out = -1;
  EXPECT_FALSE(XComponentBase::IsSubViewId("abc", &out));
  // Digits then a trailing letter: strtoll would stop, *end != '\0'.
  EXPECT_FALSE(XComponentBase::IsSubViewId("12a", &out));
  EXPECT_FALSE(XComponentBase::IsSubViewId("-1", &out));  // '-' is not a digit
  EXPECT_FALSE(XComponentBase::IsSubViewId("1 2", &out));  // space
}

TEST(IsSubViewIdTest, ZeroIsImplicitViewNotSubView) {
  // 0 is kFlutterImplicitViewId — never a sub-view.
  int64_t out = -1;
  EXPECT_FALSE(XComponentBase::IsSubViewId("0", &out));
  EXPECT_FALSE(XComponentBase::IsSubViewId("00", &out));
}

TEST(IsSubViewIdTest, OverflowRejected) {
  // strtoll sets errno = ERANGE for out-of-range values.
  int64_t out = -1;
  EXPECT_FALSE(XComponentBase::IsSubViewId("99999999999999999999", &out));
}

TEST(IsSubViewIdTest, ParsesPositiveDecimalViewId) {
  int64_t out = 0;
  EXPECT_TRUE(XComponentBase::IsSubViewId("1", &out));
  EXPECT_EQ(out, 1);

  EXPECT_TRUE(XComponentBase::IsSubViewId("123", &out));
  EXPECT_EQ(out, 123);

  // Leading zeros parse in base 10 (not octal): "007" -> 7.
  EXPECT_TRUE(XComponentBase::IsSubViewId("007", &out));
  EXPECT_EQ(out, 7);

  // INT64_MAX parses without overflow.
  EXPECT_TRUE(XComponentBase::IsSubViewId("9223372036854775807", &out));
  EXPECT_EQ(out, INT64_MAX);
}

TEST(IsSubViewIdTest, NullOutViewIdStillParses) {
  EXPECT_TRUE(XComponentBase::IsSubViewId("5", nullptr));
  EXPECT_FALSE(XComponentBase::IsSubViewId("oh_flutter_5", nullptr));
  EXPECT_FALSE(XComponentBase::IsSubViewId("", nullptr));
  EXPECT_FALSE(XComponentBase::IsSubViewId("0", nullptr));
}

TEST(IsSubViewIdTest, FailureLeavesOutViewIdUnchanged) {
  int64_t out = 42;
  EXPECT_FALSE(XComponentBase::IsSubViewId("", &out));
  EXPECT_EQ(out, 42);
  EXPECT_FALSE(XComponentBase::IsSubViewId("oh_flutter_1", &out));
  EXPECT_EQ(out, 42);
  EXPECT_FALSE(XComponentBase::IsSubViewId("0", &out));
  EXPECT_EQ(out, 42);
  EXPECT_FALSE(XComponentBase::IsSubViewId("12a", &out));
  EXPECT_EQ(out, 42);
}

TEST(IsSubViewIdTest, PlusSignAndWhitespaceRejected) {
  int64_t out = -1;
  EXPECT_FALSE(XComponentBase::IsSubViewId("+1", &out));
  EXPECT_FALSE(XComponentBase::IsSubViewId(" 1", &out));
  EXPECT_FALSE(XComponentBase::IsSubViewId("1 ", &out));
}

// ---------------------------------------------------------------------------
// ResolveSubViewRouting (lexical + liveness)
// ---------------------------------------------------------------------------

TEST(ResolveSubViewRoutingTest, NonSubViewIdResetsRouting) {
  XComponentBase xc("oh_flutter_0");
  xc.ResolveSubViewRouting();
  EXPECT_FALSE(xc.is_sub_view_);
  EXPECT_EQ(xc.sub_view_id_, 0);
}

TEST(ResolveSubViewRoutingTest, SubViewIdWithoutControllerStaysImplicit) {
  // Lexical parse succeeds, but shellholder_ptr_ is null (never attached):
  // no controller tracks the view -> stays implicit (white-screen guard).
  XComponentBase xc("5");
  xc.ResolveSubViewRouting();
  EXPECT_FALSE(xc.is_sub_view_);
  EXPECT_EQ(xc.sub_view_id_, 0);
}

#if defined(OHOS_X64_UNITTEST)
TEST(ResolveSubViewRoutingTest, SubViewIdWithLiveControllerSkippedOnX64) {
  GTEST_SKIP() << "Live OHOSShellHolder starts a debug JIT VM, which cannot "
                  "mmap code pages in the x64 emulator shell domain.";
}
#else
TEST(ResolveSubViewRoutingTest, SubViewIdWithLiveControllerRoutes) {
  // Real OHOSShellHolder (software rendering, null facade) + a controller that
  // tracks view 5 -> ResolveSubViewRouting flips to sub-view routing.
  Settings settings;
  settings.ohos_rendering_api = OHOSRenderingAPI::kSoftware;
  auto holder = std::make_unique<OHOSShellHolder>(
      settings, std::make_shared<PlatformViewOHOSNapi>(nullptr), nullptr);
  holder->napi_facade_.reset();
  OHOSWindowController* controller = holder->GetWindowController();
  ASSERT_NE(controller, nullptr);

  XComponentBase xc("5");
  xc.shellholder_ptr_ = holder.get();

  // View 5 not registered yet -> still implicit.
  xc.ResolveSubViewRouting();
  EXPECT_FALSE(xc.is_sub_view_);

  // Register view 5 directly (CreateWindow's non-first path needs a live
  // engine's AddViewSync, which is out of UT reach).
  OHOSWindow::InitParams params;
  params.type = WindowType::kDialog;
  params.host_kind = WindowHostKind::kSubWindow;
  params.view_id = 5;
  params.parent_view_id = 0;
  params.host_handle = OHOSWindowController::HandleForViewId(5);
  controller->windows_[params.host_handle] =
      controller->CreateWindowObject(FlutterWindowCreationRequest{}, params);

  xc.ResolveSubViewRouting();
  EXPECT_TRUE(xc.is_sub_view_);
  EXPECT_EQ(xc.sub_view_id_, 5);

  // Destroy the tracked window: liveness check fails, routing stays implicit.
  controller->DestroyWindow(params.host_handle);
  xc.ResolveSubViewRouting();
  EXPECT_FALSE(xc.is_sub_view_);
  EXPECT_EQ(xc.sub_view_id_, 0);
}
#endif  // !defined(OHOS_X64_UNITTEST)

}  // namespace testing
}  // namespace flutter

#endif  // FML_OS_OHOS
