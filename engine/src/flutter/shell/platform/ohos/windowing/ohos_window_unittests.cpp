/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <cstdlib>
#include "flutter/shell/platform/ohos/windowing/ohos_window.h"

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
  WindowRect rect{};
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

WindowRect* OnGetWindowPosition(const WindowSize& child_size,
                                       const WindowRect& parent_rect,
                                       const WindowRect& output_rect) {
  if (g_positioner == nullptr || g_positioner->return_null) {
    return nullptr;
  }
  WindowRect* out =
      static_cast<WindowRect*>(malloc(sizeof(WindowRect)));
  *out = g_positioner->rect;
  return out;
}

}  // namespace

// ---------------------------------------------------------------------------
// Construction + accessors
// ---------------------------------------------------------------------------

TEST(OHOSWindowTest, ConstructionStoresViewIdAndRequest) {
  RegularWindowCreationRequest request = {};
  request.preferred_size.has_preferred_view_size = true;
  request.preferred_size.preferred_view_width = 200.0;
  request.preferred_size.preferred_view_height = 100.0;
  request.preferred_constraints.has_view_constraints = true;
  request.preferred_constraints.view_min_width = 10.0;
  request.preferred_constraints.view_min_height = 20.0;
  request.callbacks.on_should_close = OnShouldClose;
  request.callbacks.on_will_close = OnWillClose;
  request.callbacks.notify_listeners = OnNotifyListeners;

  OHOSWindow window(42, request);

  EXPECT_EQ(window.view_id(), 42);
  // The lifetime state is seeded from the typed request.
  EXPECT_TRUE(window.HasPreferredSize());
  EXPECT_EQ(window.PreferredSize().width, 200.0);
  EXPECT_EQ(window.PreferredSize().height, 100.0);
}

// ---------------------------------------------------------------------------
// Dart callbacks
// ---------------------------------------------------------------------------

TEST(OHOSWindowTest, FireCallbacksInvokeRequestHandlers) {
  CallbackCounters counters;
  CallbackScope scope(&counters);

  RegularWindowCreationRequest request = {};
  request.callbacks.on_should_close = OnShouldClose;
  request.callbacks.on_will_close = OnWillClose;
  request.callbacks.notify_listeners = OnNotifyListeners;

  OHOSWindow window(42, request);
  window.FireShouldClose();
  window.FireWillClose();
  window.FireNotifyListeners();

  EXPECT_EQ(counters.should_close, 1);
  EXPECT_EQ(counters.will_close, 1);
  EXPECT_EQ(counters.notify, 1);
}

TEST(OHOSWindowTest, FireCallbacksWithNullHandlersIsNoOp) {
  // No callbacks wired: firing must not crash.
  OHOSWindow window(42, RegularWindowCreationRequest{});
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

  TooltipWindowCreationRequest request = {};
  request.get_position_callback = OnGetWindowPosition;
  OHOSWindow window(42, request);

  EXPECT_FALSE(window.ComputeWindowPosition({10, 10}, {0, 0, 100, 100},
                                            {0, 0, 800, 600}, nullptr));
}

TEST(OHOSWindowTest, ComputeWindowPositionWithoutPositioner) {
  OHOSWindow window(42, RegularWindowCreationRequest{});
  WindowRect out{99.0, 99.0, 99.0, 99.0};
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

  TooltipWindowCreationRequest request = {};
  request.get_position_callback = OnGetWindowPosition;
  OHOSWindow window(42, request);

  WindowRect out{};
  EXPECT_FALSE(window.ComputeWindowPosition({10, 10}, {0, 0, 100, 100},
                                            {0, 0, 800, 600}, &out));
}

TEST(OHOSWindowTest, ComputeWindowPositionCopiesResult) {
  PositionerConfig config;
  config.rect = {12.5, 30.0, 200.0, 150.0};
  PositionerScope scope(&config);

  TooltipWindowCreationRequest request = {};
  request.get_position_callback = OnGetWindowPosition;
  OHOSWindow window(42, request);

  WindowRect out{};
  EXPECT_TRUE(window.ComputeWindowPosition({200, 150}, {10, 10, 300, 200},
                                           {0, 0, 800, 600}, &out));
  EXPECT_EQ(out.left, 12.5);
  EXPECT_EQ(out.top, 30.0);
  EXPECT_EQ(out.width, 200.0);
  EXPECT_EQ(out.height, 150.0);
}

// ---------------------------------------------------------------------------
// Title cache
// ---------------------------------------------------------------------------

TEST(OHOSWindowTest, TitleCacheRoundTrips) {
  OHOSWindow window(42, RegularWindowCreationRequest{});
  EXPECT_EQ(window.GetTitle(), "");
  window.SetTitleCache("multi-window title");
  EXPECT_EQ(window.GetTitle(), "multi-window title");
  window.SetTitleCache("replaced");
  EXPECT_EQ(window.GetTitle(), "replaced");
}

// ---------------------------------------------------------------------------
// Sized-to-content (win32 is_sized_to_content parity: tooltip only)
// ---------------------------------------------------------------------------

TEST(OHOSWindowTest, TooltipIsSizedToContentAndKeepsConstraints) {
  TooltipWindowCreationRequest request = {};
  request.preferred_constraints.has_view_constraints = true;
  request.preferred_constraints.view_min_width = 20.0;
  request.preferred_constraints.view_min_height = 10.0;
  request.preferred_constraints.view_max_width = 400.0;
  request.preferred_constraints.view_max_height = 300.0;
  request.parent_view_id = 0;
  request.get_position_callback = OnGetWindowPosition;

  OHOSWindow window(7, request);

  EXPECT_TRUE(window.IsSizedToContent());
  WindowConstraints constraints = {};
  ASSERT_TRUE(window.GetConstraints(&constraints));
  EXPECT_EQ(constraints.view_min_width, 20.0);
  EXPECT_EQ(constraints.view_min_height, 10.0);
  EXPECT_EQ(constraints.view_max_width, 400.0);
  EXPECT_EQ(constraints.view_max_height, 300.0);
}

TEST(OHOSWindowTest, TooltipWithoutConstraintsHasNoMetricsRange) {
  TooltipWindowCreationRequest request = {};
  OHOSWindow window(7, request);
  EXPECT_TRUE(window.IsSizedToContent());
  WindowConstraints constraints = {};
  EXPECT_FALSE(window.GetConstraints(&constraints));
}

TEST(OHOSWindowTest, PopupIsSizedToContentRegularAndDialogAreNot) {
  PopupWindowCreationRequest popup_request = {};
  popup_request.preferred_constraints.has_view_constraints = true;
  popup_request.preferred_constraints.view_min_width = 5.0;
  popup_request.get_position_callback = OnGetWindowPosition;
  OHOSWindow popup(8, popup_request);
  // Sized to content like the tooltip (OHOS divergence: win32 has no popup
  // host to mirror; the OHOS popup follows its content).
  EXPECT_TRUE(popup.IsSizedToContent());
  WindowConstraints constraints = {};
  EXPECT_TRUE(popup.GetConstraints(&constraints));
  EXPECT_EQ(constraints.view_min_width, 5.0);

  OHOSWindow regular(9, RegularWindowCreationRequest{});
  EXPECT_FALSE(regular.IsSizedToContent());
  EXPECT_FALSE(regular.GetConstraints(&constraints));

  OHOSWindow dialog(10, DialogWindowCreationRequest{});
  EXPECT_FALSE(dialog.IsSizedToContent());
  EXPECT_FALSE(dialog.GetConstraints(&constraints));
}

}  // namespace testing
}  // namespace flutter
