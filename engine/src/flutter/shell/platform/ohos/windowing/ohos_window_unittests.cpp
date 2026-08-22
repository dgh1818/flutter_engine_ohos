/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

// Unit tests for the OHOSWindow base class and its Regular/Dialog/Tooltip/
// Popup subclasses.
//
// Only the controller-independent logic is exercised here (constructor params,
// callbacks, geometry/constraints, title cache, birth sizing). The
// facade-touching RequestWindowHost/RequestUiAbilityHost dispatch (base,
// Regular, Dialog kUiAbility/kSubWindow, anchored) needs a real
// OHOSWindowController whose GetNapiFacade() answers null, so those live in
// ohos_window_controller_unittests.cpp — see the shared fixture there.

#include "flutter/fml/build_config.h"  // IWYU pragma: keep  (defines FML_OS_OHOS)

#if defined(FML_OS_OHOS)

#include <gtest/gtest.h>

#include <cstdint>
#include <cstdlib>

#include "flutter/shell/platform/ohos/windowing/ohos_window.h"
#include "flutter/shell/platform/ohos/windowing/ohos_window_dialog.h"
#include "flutter/shell/platform/ohos/windowing/ohos_window_popup.h"
#include "flutter/shell/platform/ohos/windowing/ohos_window_regular.h"
#include "flutter/shell/platform/ohos/windowing/ohos_window_tooltip.h"

namespace flutter {
namespace testing {

namespace {

// The request callbacks are bare function pointers, so a per-file "current
// counter" is wired through statics; gtest runs each suite serially, so no
// cross-test interference.
struct CallbackCounters {
  int should_close = 0;
  int will_close = 0;
  int notify = 0;
};

CallbackCounters* g_counters = nullptr;

void OnShouldClose() {
  if (g_counters != nullptr) {
    ++g_counters->should_close;
  }
}

void OnWillClose() {
  if (g_counters != nullptr) {
    ++g_counters->will_close;
  }
}

void OnNotifyListeners() {
  if (g_counters != nullptr) {
    ++g_counters->notify;
  }
}

// Positioner behavior: when |g_positioner| is null the request has NO
// positioner; when return_null is set the callback returns nullptr (Dart-side
// allocation failure); otherwise it returns a malloc'd rect the production
// code free()s — must be libc-heap memory.
struct PositionerConfig {
  bool return_null = false;
  FlutterWindowRect rect{};
};

PositionerConfig* g_positioner = nullptr;

struct CallbackScope {
  explicit CallbackScope(CallbackCounters* counters) { g_counters = counters; }
  ~CallbackScope() { g_counters = nullptr; }
};

struct PositionerScope {
  explicit PositionerScope(PositionerConfig* config) { g_positioner = config; }
  ~PositionerScope() { g_positioner = nullptr; }
};

FlutterWindowRect* OnGetWindowPosition(const FlutterWindowSize& child_size,
                                       const FlutterWindowRect& parent_rect,
                                       const FlutterWindowRect& output_rect) {
  if (g_positioner == nullptr || g_positioner->return_null) {
    return nullptr;
  }
  FlutterWindowRect* out =
      static_cast<FlutterWindowRect*>(malloc(sizeof(FlutterWindowRect)));
  *out = g_positioner->rect;
  return out;
}

OHOSWindow::InitParams MakeParams(WindowType type = WindowType::kRegular,
                                  WindowHostKind kind = WindowHostKind::kUiAbility,
                                  int64_t view_id = 42,
                                  int64_t parent_view_id = 0) {
  OHOSWindow::InitParams params;
  params.type = type;
  params.host_kind = kind;
  params.view_id = view_id;
  params.parent_view_id = parent_view_id;
  params.host_handle = reinterpret_cast<void*>(static_cast<uintptr_t>(0xbeef));
  params.adopt_entry_ability = false;
  return params;
}

}  // namespace

// ---------------------------------------------------------------------------
// Construction + accessors
// ---------------------------------------------------------------------------

TEST(OHOSWindowTest, ConstructionExposesParams) {
  FlutterWindowCreationRequest request = {};
  request.has_size = true;
  request.size = {200.0, 100.0};
  request.has_constraints = true;
  request.constraints = {10.0, 20.0, 0.0, 0.0};
  request.has_parent = false;
  request.parent_view_id = 0;
  request.on_should_close = OnShouldClose;
  request.on_will_close = OnWillClose;
  request.notify_listeners = OnNotifyListeners;
  request.on_get_window_position = OnGetWindowPosition;

  // Controller-independent: nullptr is fine for every method below.
  OHOSWindow window(nullptr, MakeParams(), request);

  EXPECT_EQ(window.type(), WindowType::kRegular);
  EXPECT_EQ(window.host_kind(), WindowHostKind::kUiAbility);
  EXPECT_EQ(window.view_id(), 42);
  EXPECT_EQ(window.parent_view_id(), 0);
  EXPECT_EQ(window.handle(), reinterpret_cast<void*>(uintptr_t(0xbeef)));
  EXPECT_FALSE(window.adopt_entry_ability());
  // The request is value-copied into the window.
  EXPECT_EQ(window.request().size.width, 200.0);
  EXPECT_EQ(window.request().size.height, 100.0);
  EXPECT_TRUE(window.request().has_constraints);
  EXPECT_EQ(window.request().constraints.min_width, 10.0);
}

TEST(OHOSWindowTest, AdoptEntryAbilityFlagRoundTrips) {
  OHOSWindow::InitParams params = MakeParams();
  params.adopt_entry_ability = true;
  OHOSWindow window(nullptr, params, {});
  EXPECT_TRUE(window.adopt_entry_ability());
}

// ---------------------------------------------------------------------------
// Dart callbacks
// ---------------------------------------------------------------------------

TEST(OHOSWindowTest, FireCallbacksInvokeRequestHandlers) {
  CallbackCounters counters;
  CallbackScope scope(&counters);

  FlutterWindowCreationRequest request = {};
  request.on_should_close = OnShouldClose;
  request.on_will_close = OnWillClose;
  request.notify_listeners = OnNotifyListeners;

  OHOSWindow window(nullptr, MakeParams(), request);
  window.FireShouldClose();
  window.FireWillClose();
  window.FireNotifyListeners();

  EXPECT_EQ(counters.should_close, 1);
  EXPECT_EQ(counters.will_close, 1);
  EXPECT_EQ(counters.notify, 1);
}

TEST(OHOSWindowTest, FireCallbacksWithNullHandlersIsNoOp) {
  // No callbacks wired: firing must not crash.
  OHOSWindow window(nullptr, MakeParams(), {});
  window.FireShouldClose();
  window.FireWillClose();
  window.FireNotifyListeners();
}

// ---------------------------------------------------------------------------
// ComputeWindowPosition
// ---------------------------------------------------------------------------

TEST(OHOSWindowTest, ComputeWindowPositionRejectsNullOut) {
  PositionerConfig config;
  config.return_null = false;
  config.rect = {1.0, 2.0, 3.0, 4.0};
  PositionerScope scope(&config);

  FlutterWindowCreationRequest request = {};
  request.on_get_window_position = OnGetWindowPosition;
  OHOSWindow window(nullptr, MakeParams(), request);

  EXPECT_FALSE(window.ComputeWindowPosition({10, 10}, {0, 0, 100, 100},
                                            {0, 0, 800, 600}, nullptr));
}

TEST(OHOSWindowTest, ComputeWindowPositionWithoutPositioner) {
  OHOSWindow window(nullptr, MakeParams(), {});
  FlutterWindowRect out{99.0, 99.0, 99.0, 99.0};
  // Untouched on failure.
  EXPECT_FALSE(window.ComputeWindowPosition({10, 10}, {0, 0, 100, 100},
                                            {0, 0, 800, 600}, &out));
  EXPECT_EQ(out.left, 99.0);
  EXPECT_EQ(out.top, 99.0);
}

TEST(OHOSWindowTest, ComputeWindowPositionPositionerReturnsNull) {
  PositionerConfig config;
  config.return_null = true;
  PositionerScope scope(&config);

  FlutterWindowCreationRequest request = {};
  request.on_get_window_position = OnGetWindowPosition;
  OHOSWindow window(nullptr, MakeParams(), request);

  FlutterWindowRect out{};
  EXPECT_FALSE(window.ComputeWindowPosition({10, 10}, {0, 0, 100, 100},
                                            {0, 0, 800, 600}, &out));
}

TEST(OHOSWindowTest, ComputeWindowPositionCopiesResult) {
  PositionerConfig config;
  config.rect = {12.5, 30.0, 200.0, 150.0};
  PositionerScope scope(&config);

  FlutterWindowCreationRequest request = {};
  request.on_get_window_position = OnGetWindowPosition;
  OHOSWindow window(nullptr, MakeParams(), request);

  FlutterWindowRect out{};
  EXPECT_TRUE(window.ComputeWindowPosition({200, 150}, {10, 10, 300, 200},
                                           {0, 0, 800, 600}, &out));
  EXPECT_EQ(out.left, 12.5);
  EXPECT_EQ(out.top, 30.0);
  EXPECT_EQ(out.width, 200.0);
  EXPECT_EQ(out.height, 150.0);
}

// ---------------------------------------------------------------------------
// Constraints
// ---------------------------------------------------------------------------

TEST(OHOSWindowTest, GetConstraintsDefaultOpenRange) {
  OHOSWindow window(nullptr, MakeParams(), {});
  // No explicit constraints: min 0 / max 0 == unbounded.
  FlutterWindowConstraints c = window.GetConstraints();
  EXPECT_EQ(c.min_width, 0.0);
  EXPECT_EQ(c.min_height, 0.0);
  EXPECT_EQ(c.max_width, 0.0);
  EXPECT_EQ(c.max_height, 0.0);
}

TEST(OHOSWindowTest, GetConstraintsCopiesStored) {
  FlutterWindowCreationRequest request = {};
  request.has_constraints = true;
  request.constraints = {100.0, 80.0, 1200.0, 800.0};
  OHOSWindow window(nullptr, MakeParams(), request);

  FlutterWindowConstraints c = window.GetConstraints();
  EXPECT_EQ(c.min_width, 100.0);
  EXPECT_EQ(c.min_height, 80.0);
  EXPECT_EQ(c.max_width, 1200.0);
  EXPECT_EQ(c.max_height, 800.0);
}

TEST(OHOSWindowTest, SetRuntimeConstraintsOverridesCreationCopy) {
  OHOSWindow window(nullptr, MakeParams(), {});
  FlutterWindowConstraints c = window.GetConstraints();
  EXPECT_EQ(c.max_width, 0.0);  // untouched default first

  window.SetRuntimeConstraints({400.0, 300.0, 1600.0, 1200.0});
  c = window.GetConstraints();
  EXPECT_EQ(c.min_width, 400.0);
  EXPECT_EQ(c.min_height, 300.0);
  EXPECT_EQ(c.max_width, 1600.0);
  EXPECT_EQ(c.max_height, 1200.0);
}

// ---------------------------------------------------------------------------
// Sub-window birth sizing
// ---------------------------------------------------------------------------

TEST(OHOSWindowTest, GetSubWindowBirthSizePrefersRequestSize) {
  FlutterWindowCreationRequest request = {};
  request.has_size = true;
  request.size = {640.0, 480.0};
  request.has_constraints = true;
  request.constraints = {100.0, 80.0, 0.0, 0.0};
  OHOSWindow window(nullptr, MakeParams(), request);

  double width = -1.0;
  double height = -1.0;
  window.GetSubWindowBirthSize(&width, &height);
  EXPECT_EQ(width, 640.0);
  EXPECT_EQ(height, 480.0);
}

TEST(OHOSWindowTest, GetSubWindowBirthSizeFallsBackToMinConstraint) {
  FlutterWindowCreationRequest request = {};
  request.has_constraints = true;
  request.constraints = {100.0, 80.0, 0.0, 0.0};
  OHOSWindow window(nullptr, MakeParams(), request);

  double width = -1.0;
  double height = -1.0;
  window.GetSubWindowBirthSize(&width, &height);
  // Born small (min), not full-display.
  EXPECT_EQ(width, 100.0);
  EXPECT_EQ(height, 80.0);
}

TEST(OHOSWindowTest, GetSubWindowBirthSizeDefaultsToZero) {
  OHOSWindow window(nullptr, MakeParams(), {});
  double width = -1.0;
  double height = -1.0;
  window.GetSubWindowBirthSize(&width, &height);
  EXPECT_EQ(width, 0.0);
  EXPECT_EQ(height, 0.0);
}

// ---------------------------------------------------------------------------
// Title cache
// ---------------------------------------------------------------------------

TEST(OHOSWindowTest, TitleCacheRoundTrips) {
  OHOSWindow window(nullptr, MakeParams(), {});
  EXPECT_EQ(window.GetTitle(), "");
  window.SetTitleCache("multi-window title");
  EXPECT_EQ(window.GetTitle(), "multi-window title");
  window.SetTitleCache("replaced");
  EXPECT_EQ(window.GetTitle(), "replaced");
}

// ---------------------------------------------------------------------------
// Subclass construction + type dispatch (host request needs a controller —
// see ohos_window_controller_unittests.cpp)
// ---------------------------------------------------------------------------

TEST(OHOSWindowTest, SubclassConstructionCarriesOwnType) {
  OHOSWindow::InitParams params;

  params = MakeParams(WindowType::kRegular, WindowHostKind::kUiAbility);
  OHOSWindowRegular regular(nullptr, params, {});
  EXPECT_EQ(regular.type(), WindowType::kRegular);
  EXPECT_EQ(regular.host_kind(), WindowHostKind::kUiAbility);

  params = MakeParams(WindowType::kDialog, WindowHostKind::kSubWindow, 1, 0);
  OHOSWindowDialog dialog(nullptr, params, {});
  EXPECT_EQ(dialog.type(), WindowType::kDialog);
  EXPECT_EQ(dialog.host_kind(), WindowHostKind::kSubWindow);
  EXPECT_EQ(dialog.parent_view_id(), 0);

  params = MakeParams(WindowType::kTooltip, WindowHostKind::kSubWindow, 2, 0);
  OHOSWindowTooltip tooltip(nullptr, params, {});
  EXPECT_EQ(tooltip.type(), WindowType::kTooltip);

  params = MakeParams(WindowType::kPopup, WindowHostKind::kSubWindow, 3, 0);
  OHOSWindowPopup popup(nullptr, params, {});
  EXPECT_EQ(popup.type(), WindowType::kPopup);
}

}  // namespace testing
}  // namespace flutter

#endif  // FML_OS_OHOS
