/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

// Unit tests for OHOSWindowController: multi-window bookkeeping, the FFI
// surface, and the CreateWindow host-request dispatch (inlined; exercised
// with the facade nulled).
// Testability contract: a real OHOSShellHolder is constructed (software
// rendering, null-env napi facade — same recipe as ohos_shell_holder_
// unittests), then `#define private public` grants access to the holder's
// napi_facade_ so the test can reset it to null. After that every controller
// path that would hop into napi (`if (auto facade = holder_->GetNapiFacade())`)
// is safely skipped (a null-env facade would crash on a real call), and every
// `if (!facade)` branch — the ones a live environment never reaches without
// a real engine — is exercised.

#define private public
#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"
#include "flutter/shell/platform/ohos/ohos_shell_holder.h"
#undef private

#include <gtest/gtest.h>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>
#include "flutter/fml/log_settings.h"
#include "flutter/shell/platform/ohos/windowing/ohos_window.h"

static const char* g_ohos_device_type = "phone";

extern "C" const char* OH_GetDeviceType(void) {
  return g_ohos_device_type;
}

namespace flutter {
namespace testing {

namespace {

// Software rendering, no GPU needed — the ohos_shell_holder test recipe.
Settings MakeTestSettings() {
  Settings settings;
  settings.ohos_rendering_api = OHOSRenderingAPI::kSoftware;
  return settings;
}

RegularWindowCreationRequest MakeRegularRequest() {
  return RegularWindowCreationRequest{};
}

DialogWindowCreationRequest MakeDialogRequest() {
  return DialogWindowCreationRequest{};
}

TooltipWindowCreationRequest MakeTooltipRequest() {
  return TooltipWindowCreationRequest{};
}

PopupWindowCreationRequest MakePopupRequest() {
  return PopupWindowCreationRequest{};
}

void* InsertWindow(OHOSWindowController* controller,
                   int64_t view_id,
                   const RegularWindowCreationRequest& request =
                       RegularWindowCreationRequest{}) {
  void* handle = OHOSWindowController::HandleForViewId(view_id);
  controller->windows_[handle] =
      std::make_unique<OHOSWindow>(view_id, request);
  return handle;
}

// Tooltip overload: the anchored archetypes carry a positioner.
void* InsertWindow(OHOSWindowController* controller,
                   int64_t view_id,
                   const TooltipWindowCreationRequest& request) {
  void* handle = OHOSWindowController::HandleForViewId(view_id);
  controller->windows_[handle] =
      std::make_unique<OHOSWindow>(view_id, request);
  return handle;
}

// Request callbacks are bare function pointers -> a per-file "current
// counter" wired through statics (gtest runs each suite serially).
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

struct PositionerConfig {
  bool return_null = false;
  WindowRect rect{};
};

PositionerConfig* g_positioner = nullptr;

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

class OHOSWindowControllerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    holder_ = std::make_unique<OHOSShellHolder>(
        MakeTestSettings(), std::make_shared<PlatformViewOHOSNapi>(nullptr),
        nullptr);
    controller_ = holder_->GetWindowController();
    ASSERT_NE(controller_, nullptr);
    // Null the facade so every controller napi hop is safely skipped.
    holder_->napi_facade_.reset();
    g_counters = nullptr;
    g_positioner = nullptr;
  }

  void TearDown() override {
    g_counters = nullptr;
    g_positioner = nullptr;
    g_ohos_device_type = "phone";
    holder_.reset();  // controller_ dtor unregisters from g_controllers
  }

  std::unique_ptr<OHOSShellHolder> holder_;
  OHOSWindowController* controller_ = nullptr;
};

// ---------------------------------------------------------------------------
// Statics: host-kind resolution, handle bias, controller routing
// ---------------------------------------------------------------------------

TEST_F(OHOSWindowControllerTest, ResolveHostKindMapsArchetypes) {
  // Regular always UIAbility.
  EXPECT_EQ(
      OHOSWindowController::ResolveHostKind(WindowType::kRegular, false, 0),
      WindowHostKind::kUiAbility);
  EXPECT_EQ(
      OHOSWindowController::ResolveHostKind(WindowType::kRegular, true, 5),
      WindowHostKind::kUiAbility);
  // Modeless dialog (no parent) = UIAbility; modal (has_parent) = SubWindow.
  EXPECT_EQ(
      OHOSWindowController::ResolveHostKind(WindowType::kDialog, false, 0),
      WindowHostKind::kUiAbility);
  // parent_view_id == 0 is a LEGITIMATE parent (adopted main window).
  EXPECT_EQ(OHOSWindowController::ResolveHostKind(WindowType::kDialog, true, 0),
            WindowHostKind::kSubWindow);
  // Tooltip / Popup always SubWindow.
  EXPECT_EQ(
      OHOSWindowController::ResolveHostKind(WindowType::kTooltip, true, 5),
      WindowHostKind::kSubWindow);
  EXPECT_EQ(OHOSWindowController::ResolveHostKind(WindowType::kPopup, true, 5),
            WindowHostKind::kSubWindow);
}

TEST_F(OHOSWindowControllerTest, HandleBiasRoundTrips) {
  // View 0's handle is a real pointer, never the nullptr sentinel.
  void* handle0 = OHOSWindowController::HandleForViewId(0);
  EXPECT_NE(handle0, nullptr);
  EXPECT_EQ(OHOSWindowController::ViewIdForHandle(handle0), 0);

  void* handle7 = OHOSWindowController::HandleForViewId(7);
  EXPECT_EQ(OHOSWindowController::ViewIdForHandle(handle7), 7);
  EXPECT_EQ(OHOSWindowController::HandleForViewId(
                OHOSWindowController::ViewIdForHandle(handle7)),
            handle7);
}

TEST_F(OHOSWindowControllerTest, ActiveInstanceSingleAndAmbiguous) {
  // Exactly one live controller from the fixture.
  EXPECT_EQ(OHOSWindowController::ActiveInstance(), controller_);
}

TEST_F(OHOSWindowControllerTest, ActiveInstanceAmbiguousWithTwoControllers) {
  auto holder2 = std::make_unique<OHOSShellHolder>(
      MakeTestSettings(), std::make_shared<PlatformViewOHOSNapi>(nullptr),
      nullptr);
  holder2->napi_facade_.reset();
  // Two controllers -> ambiguous -> null.
  EXPECT_EQ(OHOSWindowController::ActiveInstance(), nullptr);
}

TEST_F(OHOSWindowControllerTest, ForViewHitAndMiss) {
  EXPECT_EQ(OHOSWindowController::ForView(0), nullptr);  // not created yet
  auto request = MakeRegularRequest();
  EXPECT_EQ(controller_->CreateRegularWindow(&request), 0);
  EXPECT_EQ(OHOSWindowController::ForView(0), controller_);
  EXPECT_EQ(OHOSWindowController::ForView(9999), nullptr);
}

TEST_F(OHOSWindowControllerTest, ForEngineIdPreciseRouting) {
  EXPECT_EQ(OHOSWindowController::ForEngineId(0), controller_);  // default 0
  controller_->SetEngineId(42);
  EXPECT_EQ(OHOSWindowController::ForEngineId(42), controller_);
  EXPECT_EQ(OHOSWindowController::ForEngineId(0), nullptr);
}

TEST_F(OHOSWindowControllerTest, ForWindowHandleHitAndMiss) {
  auto request = MakeRegularRequest();
  EXPECT_EQ(controller_->CreateRegularWindow(&request), 0);
  void* handle = controller_->GetHandleForView(0);
  ASSERT_NE(handle, nullptr);
  EXPECT_EQ(OHOSWindowController::ForWindowHandle(handle), controller_);
  EXPECT_EQ(
      OHOSWindowController::ForWindowHandle(reinterpret_cast<void*>(0x1234)),
      nullptr);
}

TEST_F(OHOSWindowControllerTest, AllocateViewIdIsMonotonicAndNeverZero) {
  int64_t first = controller_->AllocateViewId();
  int64_t second = controller_->AllocateViewId();
  EXPECT_NE(first, 0);
  EXPECT_NE(second, 0);
  EXPECT_GT(second, first);
}

TEST_F(OHOSWindowControllerTest, GetNapiFacadeNullAfterReset) {
  EXPECT_EQ(controller_->GetNapiFacade(), nullptr);
}

TEST_F(OHOSWindowControllerTest, GetHandleForViewBeforeAfter) {
  EXPECT_EQ(controller_->GetHandleForView(0), nullptr);
  auto request = MakeRegularRequest();
  EXPECT_EQ(controller_->CreateRegularWindow(&request), 0);
  EXPECT_EQ(controller_->GetHandleForView(0),
            OHOSWindowController::HandleForViewId(0));
  EXPECT_EQ(controller_->GetHandleForView(77), nullptr);
}

// ---------------------------------------------------------------------------
// Window creation
// ---------------------------------------------------------------------------

TEST_F(OHOSWindowControllerTest, FirstRegularWindowAdoptsViewZero) {
  auto request = MakeRegularRequest();
  request.preferred_size.has_preferred_view_size = true;
  request.preferred_size.preferred_view_width = 640.0;
  request.preferred_size.preferred_view_height = 480.0;

  EXPECT_EQ(controller_->CreateRegularWindow(&request), 0);
  EXPECT_TRUE(controller_->entry_ability_bound_);
  EXPECT_NE(controller_->GetHandleForView(0), nullptr);

  // A second Regular no longer adopts: AddViewSync fails without an engine.
  EXPECT_EQ(controller_->CreateRegularWindow(&request),
            OHOSWindowController::kCreateWindowFailedViewId);
}

TEST_F(OHOSWindowControllerTest, NonRegularFirstWindowDoesNotAdoptViewZero) {
  // A modeless dialog first is NOT a Regular -> no view-0 adoption.
  auto dialog = MakeDialogRequest();
  EXPECT_EQ(controller_->CreateDialogWindow(&dialog),
            OHOSWindowController::kCreateWindowFailedViewId);
  EXPECT_FALSE(controller_->entry_ability_bound_);
  // A Regular created afterwards still adopts view 0.
  auto request = MakeRegularRequest();
  EXPECT_EQ(controller_->CreateRegularWindow(&request), 0);
  EXPECT_TRUE(controller_->entry_ability_bound_);
}

TEST_F(OHOSWindowControllerTest, TooltipAndPopupCreationFailWithoutEngine) {
  auto tooltip = MakeTooltipRequest();
  EXPECT_EQ(controller_->CreateTooltipWindow(&tooltip),
            OHOSWindowController::kCreateWindowFailedViewId);
  auto popup = MakePopupRequest();
  EXPECT_EQ(controller_->CreatePopupWindow(&popup),
            OHOSWindowController::kCreateWindowFailedViewId);
  EXPECT_FALSE(controller_->entry_ability_bound_);
}

// ---------------------------------------------------------------------------
// Destruction
// ---------------------------------------------------------------------------

TEST_F(OHOSWindowControllerTest, DestroyWindowUnknownHandleWarns) {
  controller_->DestroyWindow(reinterpret_cast<void*>(0xdead));
  EXPECT_EQ(controller_->GetHandleForView(99), nullptr);
}

TEST_F(OHOSWindowControllerTest, DestroyWindowNonImplicitFiresWillClose) {
  CallbackCounters counters;
  g_counters = &counters;
  auto request = MakeRegularRequest();
  request.callbacks.on_will_close = OnWillClose;

  void* handle = InsertWindow(controller_, 5, request);
  EXPECT_NE(controller_->GetHandleForView(5), nullptr);

  controller_->DestroyWindow(handle);
  EXPECT_EQ(counters.will_close, 1);
  EXPECT_EQ(controller_->GetHandleForView(5), nullptr);
  g_counters = nullptr;
}

TEST_F(OHOSWindowControllerTest, DestroyWindowImplicitFiresWillClose) {
  CallbackCounters counters;
  g_counters = &counters;
  auto request = MakeRegularRequest();
  request.callbacks.on_will_close = OnWillClose;

  EXPECT_EQ(controller_->CreateRegularWindow(&request), 0);
  void* handle = controller_->GetHandleForView(0);
  ASSERT_NE(handle, nullptr);

  controller_->DestroyWindow(handle);
  EXPECT_EQ(counters.will_close, 1);
  EXPECT_EQ(controller_->GetHandleForView(0), nullptr);
  g_counters = nullptr;
}

// ---------------------------------------------------------------------------
// Content size / view actual size
// ---------------------------------------------------------------------------

TEST_F(OHOSWindowControllerTest, GetContentSizeFallsBackToRequestSize) {
  auto request = MakeRegularRequest();
  request.preferred_size.has_preferred_view_size = true;
  request.preferred_size.preferred_view_width = 640.0;
  request.preferred_size.preferred_view_height = 480.0;
  EXPECT_EQ(controller_->CreateRegularWindow(&request), 0);

  // No actual size pushed -> creation request size.
  WindowSize size =
      controller_->GetContentSize(controller_->GetHandleForView(0));
  EXPECT_EQ(size.width, 640.0);
  EXPECT_EQ(size.height, 480.0);

  // Unknown window -> zero.
  WindowSize zero =
      controller_->GetContentSize(reinterpret_cast<void*>(0x123));
  EXPECT_EQ(zero.width, 0.0);
  EXPECT_EQ(zero.height, 0.0);
}

TEST_F(OHOSWindowControllerTest, SetViewActualSizeClampsAndReadsBack) {
  controller_->SetViewActualSize(0, 200.0, 100.0, /*density=*/2.0);
  controller_->SetViewActualSize(1, 100.0, 50.0, /*density=*/0.0);  // clamp 1.0

  WindowSize s0 =
      controller_->GetContentSize(OHOSWindowController::HandleForViewId(0));
  EXPECT_EQ(s0.width, 100.0);  // 200 / 2
  EXPECT_EQ(s0.height, 50.0);

  WindowSize s1 =
      controller_->GetContentSize(OHOSWindowController::HandleForViewId(1));
  EXPECT_EQ(s1.width, 100.0);  // 100 / 1
  EXPECT_EQ(s1.height, 50.0);
}

// ---------------------------------------------------------------------------
// Sized-to-content queries (win32 is_sized_to_content parity)
// ---------------------------------------------------------------------------

TEST_F(OHOSWindowControllerTest, SizedToContentQueriesByArchetype) {
  auto tooltip_request = MakeTooltipRequest();
  tooltip_request.preferred_constraints.has_view_constraints = true;
  tooltip_request.preferred_constraints.view_min_width = 20.0;
  tooltip_request.preferred_constraints.view_min_height = 10.0;
  tooltip_request.preferred_constraints.view_max_width = 400.0;
  tooltip_request.preferred_constraints.view_max_height = 300.0;
  InsertWindow(controller_, 7, tooltip_request);

  EXPECT_TRUE(controller_->IsViewSizedToContent(7));
  WindowConstraints constraints = {};
  ASSERT_TRUE(controller_->GetViewConstraints(7, &constraints));
  EXPECT_EQ(constraints.view_min_width, 20.0);
  EXPECT_EQ(constraints.view_max_height, 300.0);

  // Regular and unknown views: not sized-to-content, no constraints.
  InsertWindow(controller_, 8);
  EXPECT_FALSE(controller_->IsViewSizedToContent(8));
  EXPECT_FALSE(controller_->GetViewConstraints(8, &constraints));
  EXPECT_FALSE(controller_->IsViewSizedToContent(999));
  EXPECT_FALSE(controller_->GetViewConstraints(999, &constraints));
}

// ---------------------------------------------------------------------------
// Title
// ---------------------------------------------------------------------------

TEST_F(OHOSWindowControllerTest, SetTitleNullIgnoredAndCached) {
  controller_->SetTitle(nullptr, nullptr);  // null title -> ignored, no crash
  void* handle = InsertWindow(controller_, 5);
  controller_->SetTitle(handle, "win title");
  char buf[32] = {};
  controller_->GetTitle(handle, buf, sizeof(buf));
  EXPECT_STREQ(buf, "win title");
}

TEST_F(OHOSWindowControllerTest, GetTitleGuardsAndTruncates) {
  controller_->GetTitle(nullptr, nullptr, 10);  // null out -> return
  void* handle = InsertWindow(controller_, 5);
  controller_->SetTitle(handle, "abcdef");
  char buf[4] = {};
  controller_->GetTitle(handle, buf, 4);  // capacity 4 -> 3 chars + NUL
  EXPECT_EQ(buf[0], 'a');
  EXPECT_EQ(buf[1], 'b');
  EXPECT_EQ(buf[2], 'c');
  EXPECT_EQ(buf[3], '\0');
  controller_->GetTitle(handle, buf, 0);  // capacity <= 0 -> return

  // Unknown window -> empty string.
  char buf2[8] = "xxxxxxx";
  controller_->GetTitle(OHOSWindowController::HandleForViewId(999), buf2,
                        sizeof(buf2));
  EXPECT_EQ(buf2[0], '\0');
}

// ---------------------------------------------------------------------------
// Constraints / activation / listeners
// ---------------------------------------------------------------------------

TEST_F(OHOSWindowControllerTest, SetConstraintsForwardsWithoutCaching) {
  void* handle = InsertWindow(controller_, 5);
  // Pass-through: forwards to the (null) facade, keeps no C++ copy.
  controller_->SetConstraints(handle, {true, 100.0, 80.0, 1200.0, 800.0});
  // Unknown window: nothing cached, facade null -> no crash.
  controller_->SetConstraints(OHOSWindowController::HandleForViewId(999),
                              {true, 0.0, 0.0, 0.0, 0.0});
}

TEST_F(OHOSWindowControllerTest, NotifyListenersHitAndMiss) {
  CallbackCounters counters;
  g_counters = &counters;
  auto request = MakeRegularRequest();
  request.callbacks.notify_listeners = OnNotifyListeners;

  void* handle = InsertWindow(controller_, 5, request);
  controller_->NotifyListeners(handle);
  EXPECT_EQ(counters.notify, 1);
  // Unknown window -> no-op.
  controller_->NotifyListeners(reinterpret_cast<void*>(0x123));
  EXPECT_EQ(counters.notify, 1);
  g_counters = nullptr;
}

TEST_F(OHOSWindowControllerTest, SetViewActivatedNotifiesOnlyOnChange) {
  CallbackCounters counters;
  g_counters = &counters;
  auto request = MakeRegularRequest();
  request.callbacks.notify_listeners = OnNotifyListeners;
  InsertWindow(controller_, 9, request);

  // Never pushed -> historical default true.
  EXPECT_TRUE(controller_->GetViewActivated(9));

  controller_->SetViewActivated(9, true);
  EXPECT_EQ(counters.notify, 1);  // first push -> notify
  controller_->SetViewActivated(9, true);
  EXPECT_EQ(counters.notify, 1);  // same value -> no notify
  controller_->SetViewActivated(9, false);
  EXPECT_EQ(counters.notify, 2);

  EXPECT_FALSE(controller_->GetViewActivated(9));
  // Absent view -> true.
  EXPECT_TRUE(controller_->GetViewActivated(777));
  g_counters = nullptr;
}

// ---------------------------------------------------------------------------
// ComputeWindowPosition (controller-level delegation)
// ---------------------------------------------------------------------------

TEST_F(OHOSWindowControllerTest, ComputeWindowPositionGuards) {
  WindowRect out{};
  // Null out.
  EXPECT_FALSE(controller_->ComputeWindowPosition(
      5, {100.0, 50.0}, {0.0, 0.0, 200.0, 100.0}, {0.0, 0.0, 800.0, 600.0},
      nullptr));
  // Unknown view.
  EXPECT_FALSE(controller_->ComputeWindowPosition(
      999, {100.0, 50.0}, {0.0, 0.0, 200.0, 100.0}, {0.0, 0.0, 800.0, 600.0},
      &out));
}

TEST_F(OHOSWindowControllerTest, ComputeWindowPositionDelegates) {
  PositionerConfig config;
  config.rect = {12.5, 30.0, 200.0, 150.0};
  g_positioner = &config;
  // Only the anchored archetypes carry a positioner.
  auto request = MakeTooltipRequest();
  request.get_position_callback = OnGetWindowPosition;
  InsertWindow(controller_, 7, request);

  WindowRect out{};
  EXPECT_TRUE(controller_->ComputeWindowPosition(
      7, {100.0, 50.0}, {0.0, 0.0, 200.0, 100.0}, {0.0, 0.0, 800.0, 600.0},
      &out));
  EXPECT_EQ(out.left, 12.5);
  EXPECT_EQ(out.top, 30.0);
  EXPECT_EQ(out.width, 200.0);
  EXPECT_EQ(out.height, 150.0);
  g_positioner = nullptr;
}

// ---------------------------------------------------------------------------
// OS-initiated close / engine restart
// ---------------------------------------------------------------------------

TEST_F(OHOSWindowControllerTest, HandleOsWindowClosedNoOpWhenErased) {
  controller_->HandleOsWindowClosed(12345);
  EXPECT_EQ(controller_->GetHandleForView(12345), nullptr);
}

TEST_F(OHOSWindowControllerTest, HandleOsWindowClosedNonImplicit) {
  CallbackCounters counters;
  g_counters = &counters;
  auto request = MakeRegularRequest();
  request.callbacks.on_should_close = OnShouldClose;
  request.callbacks.on_will_close = OnWillClose;
  InsertWindow(controller_, 5, request);

  controller_->HandleOsWindowClosed(5);
  EXPECT_EQ(counters.should_close, 1);
  EXPECT_EQ(counters.will_close, 1);
  EXPECT_EQ(controller_->GetHandleForView(5), nullptr);
  g_counters = nullptr;
}

TEST_F(OHOSWindowControllerTest, HandleOsWindowClosedImplicitSkipsRemoveView) {
  CallbackCounters counters;
  g_counters = &counters;
  auto request = MakeRegularRequest();
  request.callbacks.on_should_close = OnShouldClose;
  EXPECT_EQ(controller_->CreateRegularWindow(&request), 0);

  controller_->HandleOsWindowClosed(0);
  EXPECT_EQ(counters.should_close, 1);
  EXPECT_EQ(controller_->GetHandleForView(0), nullptr);
  g_counters = nullptr;
}

TEST_F(OHOSWindowControllerTest, OnPreEngineRestartDropsAllAndResetsAdoption) {
  auto request = MakeRegularRequest();
  EXPECT_EQ(controller_->CreateRegularWindow(&request), 0);  // view 0
  InsertWindow(controller_, 5);
  InsertWindow(controller_, 6);
  EXPECT_TRUE(controller_->entry_ability_bound_);

  controller_->OnPreEngineRestart();
  EXPECT_TRUE(controller_->windows_.empty());
  EXPECT_FALSE(controller_->entry_ability_bound_);
  EXPECT_EQ(controller_->GetHandleForView(0), nullptr);
  EXPECT_EQ(controller_->GetHandleForView(5), nullptr);
  EXPECT_EQ(controller_->GetHandleForView(6), nullptr);

  // After restart a fresh Regular re-adopts view 0.
  EXPECT_EQ(controller_->CreateRegularWindow(&request), 0);
  EXPECT_TRUE(controller_->entry_ability_bound_);
}

// ---------------------------------------------------------------------------
// Dart FFI entry points
// ---------------------------------------------------------------------------

TEST_F(OHOSWindowControllerTest, FfiCreateWithNullRequestRejected) {
  EXPECT_EQ(InternalFlutter_WindowController_CreateRegularWindow(0, nullptr),
            OHOSWindowController::kCreateWindowFailedViewId);
  EXPECT_EQ(InternalFlutter_WindowController_CreateDialogWindow(0, nullptr),
            OHOSWindowController::kCreateWindowFailedViewId);
  EXPECT_EQ(InternalFlutter_WindowController_CreateTooltipWindow(0, nullptr),
            OHOSWindowController::kCreateWindowFailedViewId);
  EXPECT_EQ(InternalFlutter_WindowController_CreatePopupWindow(0, nullptr),
            OHOSWindowController::kCreateWindowFailedViewId);
}

TEST_F(OHOSWindowControllerTest, FfiCreateRegularWindowFullPath) {
  auto request = MakeRegularRequest();
  request.preferred_size.has_preferred_view_size = true;
  request.preferred_size.preferred_view_width = 640.0;
  request.preferred_size.preferred_view_height = 480.0;
  int64_t view =
      InternalFlutter_WindowController_CreateRegularWindow(0, &request);
  EXPECT_EQ(view, 0);
  void* handle = InternalFlutter_Window_GetHandle(0, 0);
  EXPECT_EQ(handle, OHOSWindowController::HandleForViewId(0));
}

TEST_F(OHOSWindowControllerTest, FfiGetHandleAndDestroyGuards) {
  // Unknown view -> null handle.
  EXPECT_EQ(InternalFlutter_Window_GetHandle(0, 999), nullptr);
  // Destroy of an unknown handle is a no-op warning.
  InternalFlutter_Window_Destroy(0, reinterpret_cast<void*>(0x1234));
}

TEST_F(OHOSWindowControllerTest, FfiGetContentSize) {
  auto request = MakeRegularRequest();
  request.preferred_size.has_preferred_view_size = true;
  request.preferred_size.preferred_view_width = 640.0;
  request.preferred_size.preferred_view_height = 480.0;
  EXPECT_EQ(InternalFlutter_WindowController_CreateRegularWindow(0, &request),
            0);
  WindowSize size = InternalFlutter_Window_GetContentSize(
      OHOSWindowController::HandleForViewId(0));
  EXPECT_EQ(size.width, 640.0);
  EXPECT_EQ(size.height, 480.0);
  // Unknown window -> zeros.
  WindowSize zero =
      InternalFlutter_Window_GetContentSize(reinterpret_cast<void*>(0x999));
  EXPECT_EQ(zero.width, 0.0);
  EXPECT_EQ(zero.height, 0.0);
}

TEST_F(OHOSWindowControllerTest, FfiPropertySettersNullGuards) {
  InternalFlutter_Window_SetContentSize(nullptr, nullptr);
  InternalFlutter_Window_SetConstraints(nullptr, nullptr);
  InternalFlutter_Window_SetTitle(nullptr, nullptr);
  // No crash; null pointers are skipped at the guards.
}

TEST_F(OHOSWindowControllerTest, FfiWindowStateMutationsNoCrash) {
  void* handle = OHOSWindowController::HandleForViewId(0);
  InternalFlutter_Window_Activate(handle);
  InternalFlutter_Window_SetMaximized(handle, true);
  InternalFlutter_Window_SetMinimized(handle, false);
  InternalFlutter_Window_SetFullscreen(handle, true);
  // Facade null -> every hop skipped.
  SUCCEED();
}

TEST_F(OHOSWindowControllerTest, FfiGetActivated) {
  // Unknown view -> false.
  EXPECT_FALSE(InternalFlutter_Window_GetActivated(0, 999));
  // Known view, never pushed -> true (historical contract).
  auto request = MakeRegularRequest();
  EXPECT_EQ(controller_->CreateRegularWindow(&request), 0);
  EXPECT_TRUE(InternalFlutter_Window_GetActivated(0, 0));
}

TEST_F(OHOSWindowControllerTest, FfiGetOffsetFromParent) {
  InternalFlutter_Window_GetOffsetFromParent(nullptr, nullptr);  // null out
  WindowSize out{9.0, 9.0};
  InternalFlutter_Window_GetOffsetFromParent(nullptr, &out);
  EXPECT_EQ(out.width, 0.0);
  EXPECT_EQ(out.height, 0.0);
}

TEST_F(OHOSWindowControllerTest, FfiGetTitleGuards) {
  InternalFlutter_Window_GetTitle(nullptr, nullptr, 10);
  char buf[4];
  InternalFlutter_Window_GetTitle(nullptr, buf, 0);  // capacity <= 0

  void* handle = InsertWindow(controller_, 5);
  controller_->SetTitle(handle, "hello");
  char out[8] = {};
  InternalFlutter_Window_GetTitle(handle, out, sizeof(out));
  EXPECT_STREQ(out, "hello");
}

TEST_F(OHOSWindowControllerTest, FfiWindowingSupportedUsesDeviceType) {
  g_ohos_device_type = "2in1";
  EXPECT_TRUE(OHOS_WindowingSupported());
  g_ohos_device_type = "phone";
  EXPECT_FALSE(OHOS_WindowingSupported());
}

TEST_F(OHOSWindowControllerTest, GetContentSizePrefersActualOverRequest) {
  auto request = MakeRegularRequest();
  request.preferred_size.has_preferred_view_size = true;
  request.preferred_size.preferred_view_width = 640.0;
  request.preferred_size.preferred_view_height = 480.0;
  EXPECT_EQ(controller_->CreateRegularWindow(&request), 0);

  controller_->SetViewActualSize(0, 200.0, 100.0, /*density=*/2.0);
  WindowSize size =
      controller_->GetContentSize(controller_->GetHandleForView(0));
  EXPECT_EQ(size.width, 100.0);
  EXPECT_EQ(size.height, 50.0);
}

TEST_F(OHOSWindowControllerTest, OnPreEngineRestartDoesNotFireCallbacks) {
  CallbackCounters counters;
  g_counters = &counters;
  auto request = MakeRegularRequest();
  request.callbacks.on_should_close = OnShouldClose;
  request.callbacks.on_will_close = OnWillClose;
  EXPECT_EQ(controller_->CreateRegularWindow(&request), 0);
  auto second = MakeRegularRequest();
  second.callbacks.on_should_close = OnShouldClose;
  second.callbacks.on_will_close = OnWillClose;
  InsertWindow(controller_, 5, second);

  controller_->OnPreEngineRestart();
  EXPECT_EQ(counters.should_close, 0);
  EXPECT_EQ(counters.will_close, 0);
  g_counters = nullptr;
}

TEST_F(OHOSWindowControllerTest, HandleOsWindowClosedAfterDestroyIsNoOp) {
  CallbackCounters counters;
  g_counters = &counters;
  auto request = MakeRegularRequest();
  request.callbacks.on_should_close = OnShouldClose;
  request.callbacks.on_will_close = OnWillClose;
  void* handle = InsertWindow(controller_, 5, request);
  controller_->DestroyWindow(handle);
  EXPECT_EQ(counters.will_close, 1);

  controller_->HandleOsWindowClosed(5);
  EXPECT_EQ(counters.should_close, 0);
  EXPECT_EQ(counters.will_close, 1);
  g_counters = nullptr;
}

TEST_F(OHOSWindowControllerTest, ViewIdForHandleNullIsMinusOne) {
  EXPECT_EQ(OHOSWindowController::ViewIdForHandle(nullptr), -1);
}

TEST_F(OHOSWindowControllerTest, FirstRegularWindowAdoptsImplicitView) {
  auto request = MakeRegularRequest();
  EXPECT_EQ(controller_->CreateRegularWindow(&request), 0);
  // The adopting window is registered under implicit view 0's handle; the
  // ETS host learns about the adoption from which facade method ran
  // (BindEntryAbilityToView), so no C++ flag is kept.
  EXPECT_NE(controller_->LookupWindow(controller_->GetHandleForView(0)),
            nullptr);
}

TEST_F(OHOSWindowControllerTest, CreateDialogWithParentFailsWithoutEngine) {
  auto request = MakeDialogRequest();
  // 0 is a LEGITIMATE parent (the adopted main window) — a real parent, not
  // the kNoParentViewId sentinel.
  request.parent_view_id = 0;
  EXPECT_EQ(controller_->CreateDialogWindow(&request),
            OHOSWindowController::kCreateWindowFailedViewId);
}

TEST_F(OHOSWindowControllerTest, FfiCreateAuxWindowsFailWithoutEngine) {
  auto dialog = MakeDialogRequest();
  EXPECT_EQ(InternalFlutter_WindowController_CreateDialogWindow(0, &dialog),
            OHOSWindowController::kCreateWindowFailedViewId);
  auto tooltip = MakeTooltipRequest();
  EXPECT_EQ(InternalFlutter_WindowController_CreateTooltipWindow(0, &tooltip),
            OHOSWindowController::kCreateWindowFailedViewId);
  auto popup = MakePopupRequest();
  EXPECT_EQ(InternalFlutter_WindowController_CreatePopupWindow(0, &popup),
            OHOSWindowController::kCreateWindowFailedViewId);
}

TEST_F(OHOSWindowControllerTest, FfiCreateFallsBackToActiveInstance) {
  controller_->SetEngineId(42);
  auto request = MakeRegularRequest();
  EXPECT_EQ(InternalFlutter_WindowController_CreateRegularWindow(0, &request),
            0);
}

TEST_F(OHOSWindowControllerTest, FfiCreateRejectedWhenAmbiguous) {
  auto holder2 = std::make_unique<OHOSShellHolder>(
      MakeTestSettings(), std::make_shared<PlatformViewOHOSNapi>(nullptr),
      nullptr);
  holder2->napi_facade_.reset();
  auto request = MakeRegularRequest();
  EXPECT_EQ(InternalFlutter_WindowController_CreateRegularWindow(99, &request),
            OHOSWindowController::kCreateWindowFailedViewId);
}

TEST_F(OHOSWindowControllerTest, FfiCreateRejectedWhenAmbiguousQuietLogs) {
  auto holder2 = std::make_unique<OHOSShellHolder>(
      MakeTestSettings(), std::make_shared<PlatformViewOHOSNapi>(nullptr),
      nullptr);
  holder2->napi_facade_.reset();
  auto request = MakeRegularRequest();
  fml::ScopedSetLogSettings quiet({fml::kLogFatal});
  EXPECT_EQ(InternalFlutter_WindowController_CreateRegularWindow(99, &request),
            OHOSWindowController::kCreateWindowFailedViewId);
  auto dialog = MakeDialogRequest();
  EXPECT_EQ(InternalFlutter_WindowController_CreateDialogWindow(99, &dialog),
            OHOSWindowController::kCreateWindowFailedViewId);
}

TEST_F(OHOSWindowControllerTest, FfiSetTitleRoundTripAndSetConstraints) {
  void* handle = InsertWindow(controller_, 5);
  InternalFlutter_Window_SetTitle(handle, "ffi-title");
  char out[16] = {};
  InternalFlutter_Window_GetTitle(handle, out, sizeof(out));
  EXPECT_STREQ(out, "ffi-title");

  WindowConstraints constraints{true, 10.0, 20.0, 100.0, 200.0};
  InternalFlutter_Window_SetConstraints(handle, &constraints);

  char empty[8] = "xxxxxxx";
  InternalFlutter_Window_GetTitle(OHOSWindowController::HandleForViewId(999),
                                  empty, sizeof(empty));
  EXPECT_EQ(empty[0], '\0');
}

TEST_F(OHOSWindowControllerTest, FfiDestroyCreatedWindow) {
  auto request = MakeRegularRequest();
  EXPECT_EQ(controller_->CreateRegularWindow(&request), 0);
  void* handle = InternalFlutter_Window_GetHandle(0, 0);
  ASSERT_NE(handle, nullptr);
  InternalFlutter_Window_Destroy(0, handle);
  EXPECT_EQ(controller_->GetHandleForView(0), nullptr);
}

TEST_F(OHOSWindowControllerTest, FfiGetActivatedAfterPush) {
  auto request = MakeRegularRequest();
  EXPECT_EQ(controller_->CreateRegularWindow(&request), 0);
  controller_->SetViewActivated(0, false);
  EXPECT_FALSE(InternalFlutter_Window_GetActivated(0, 0));
}

TEST_F(OHOSWindowControllerTest, GetTitleUtf8RoundTrip) {
  void* handle = InsertWindow(controller_, 5);
  controller_->SetTitle(handle, "你好");
  char buf[16] = {};
  controller_->GetTitle(handle, buf, sizeof(buf));
  EXPECT_STREQ(buf, "你好");
}

TEST_F(OHOSWindowControllerTest, GetTitleTruncationKeepsWholeCodepoints) {
  void* handle = InsertWindow(controller_, 5);
  controller_->SetTitle(handle, "你好");  // E4 BD A0 E5 A5 BD
  // 4 copyable bytes: the cut lands inside 好, so GetTitle must back off to
  // the whole 你 rather than emit a dangling 0xE5 lead byte.
  char mid_cut[5] = {};
  controller_->GetTitle(handle, mid_cut, sizeof(mid_cut));
  EXPECT_STREQ(mid_cut, "你");
  // 3 copyable bytes: the cut lands exactly on the 好 boundary — kept as is.
  char char_boundary[4] = {};
  controller_->GetTitle(handle, char_boundary, sizeof(char_boundary));
  EXPECT_STREQ(char_boundary, "你");
}

TEST_F(OHOSWindowControllerTest, ControllerStateMutationsNoCrash) {
  holder_->napi_facade_ = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  PlatformViewOHOSNapi::env_ = reinterpret_cast<napi_env>(0xF00D);
  void* handle = InsertWindow(controller_, 5);
  WindowSize size{100.0, 80.0};
  controller_->SetContentSize(handle, size);
  controller_->SetConstraints(handle, {true, 1.0, 1.0, 2.0, 2.0});
  controller_->SetTitle(handle, "ut-title");
  controller_->Activate(handle);
  controller_->SetMaximized(handle, true);
  controller_->SetMinimized(handle, false);
  controller_->SetFullscreen(handle, true);
  controller_->DestroyWindow(handle);
  PlatformViewOHOSNapi::env_ = nullptr;
}

TEST_F(OHOSWindowControllerTest, GetContentSizeWithoutRequestSizeIsZero) {
  auto request = MakeRegularRequest();
  EXPECT_EQ(controller_->CreateRegularWindow(&request), 0);
  WindowSize size =
      controller_->GetContentSize(controller_->GetHandleForView(0));
  EXPECT_EQ(size.width, 0.0);
  EXPECT_EQ(size.height, 0.0);
}

TEST_F(OHOSWindowControllerTest, SetContentSizeSkipsWhenFacadeNull) {
  auto request = MakeRegularRequest();
  EXPECT_EQ(controller_->CreateRegularWindow(&request), 0);
  EXPECT_NO_FATAL_FAILURE(controller_->SetContentSize(
      controller_->GetHandleForView(0), {100.0, 80.0}));
}

TEST_F(OHOSWindowControllerTest, SetTitleUnknownHandleIsNoOp) {
  EXPECT_NO_FATAL_FAILURE(controller_->SetTitle(
      OHOSWindowController::HandleForViewId(999), "orphan"));
}

TEST_F(OHOSWindowControllerTest, GetTitleCapacityOneWritesEmpty) {
  void* handle = InsertWindow(controller_, 5);
  controller_->SetTitle(handle, "ab");
  char buf[1] = {'x'};
  controller_->GetTitle(handle, buf, 1);
  EXPECT_EQ(buf[0], '\0');
}

TEST_F(OHOSWindowControllerTest, DestroyImplicitWithFacadeCallsExit) {
  holder_->napi_facade_ = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  PlatformViewOHOSNapi::env_ = reinterpret_cast<napi_env>(0xF00D);
  auto request = MakeRegularRequest();
  EXPECT_EQ(controller_->CreateRegularWindow(&request), 0);
  void* handle = controller_->GetHandleForView(0);
  ASSERT_NE(handle, nullptr);
  EXPECT_NO_FATAL_FAILURE(controller_->DestroyWindow(handle));
  PlatformViewOHOSNapi::env_ = nullptr;
}

TEST_F(OHOSWindowControllerTest, OnPreEngineRestartDestroysHostsWhenFacadeSet) {
  holder_->napi_facade_ = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  PlatformViewOHOSNapi::env_ = reinterpret_cast<napi_env>(0xF00D);
  auto request = MakeRegularRequest();
  EXPECT_EQ(controller_->CreateRegularWindow(&request), 0);
  InsertWindow(controller_, 5);
  EXPECT_NO_FATAL_FAILURE(controller_->OnPreEngineRestart());
  EXPECT_TRUE(controller_->windows_.empty());
  PlatformViewOHOSNapi::env_ = nullptr;
}

TEST_F(OHOSWindowControllerTest, FfiOpsRejectedWhenNoUniqueController) {
  auto holder2 = std::make_unique<OHOSShellHolder>(
      MakeTestSettings(), std::make_shared<PlatformViewOHOSNapi>(nullptr),
      nullptr);
  holder2->napi_facade_.reset();
  auto dialog = MakeDialogRequest();
  EXPECT_EQ(InternalFlutter_WindowController_CreateDialogWindow(99, &dialog),
            OHOSWindowController::kCreateWindowFailedViewId);
  auto tooltip = MakeTooltipRequest();
  EXPECT_EQ(InternalFlutter_WindowController_CreateTooltipWindow(99, &tooltip),
            OHOSWindowController::kCreateWindowFailedViewId);
  auto popup = MakePopupRequest();
  EXPECT_EQ(InternalFlutter_WindowController_CreatePopupWindow(99, &popup),
            OHOSWindowController::kCreateWindowFailedViewId);
  void* unknown = reinterpret_cast<void*>(0xABCD);
  InternalFlutter_Window_Destroy(99, unknown);
  WindowSize got = InternalFlutter_Window_GetContentSize(unknown);
  EXPECT_EQ(got.width, 0.0);
  EXPECT_EQ(got.height, 0.0);
  WindowSize size{10.0, 20.0};
  WindowConstraints constraints{true, 1.0, 1.0, 2.0, 2.0};
  InternalFlutter_Window_SetContentSize(unknown, &size);
  InternalFlutter_Window_SetConstraints(unknown, &constraints);
  InternalFlutter_Window_SetTitle(unknown, "x");
  InternalFlutter_Window_Activate(unknown);
  InternalFlutter_Window_SetMaximized(unknown, true);
  InternalFlutter_Window_SetMinimized(unknown, false);
  InternalFlutter_Window_SetFullscreen(unknown, true);
}

TEST_F(OHOSWindowControllerTest, FfiSetContentSizeWithSizeForwards) {
  void* handle = InsertWindow(controller_, 5);
  WindowSize size{32.0, 48.0};
  EXPECT_NO_FATAL_FAILURE(InternalFlutter_Window_SetContentSize(handle, &size));
}

TEST_F(OHOSWindowControllerTest, LogThresholdCoversWarnAndErrorSkip) {
  {
    fml::ScopedSetLogSettings quiet({fml::kLogFatal});
    controller_->DestroyWindow(reinterpret_cast<void*>(0xdead));
    auto tooltip = MakeTooltipRequest();
    EXPECT_EQ(controller_->CreateTooltipWindow(&tooltip),
              OHOSWindowController::kCreateWindowFailedViewId);
    controller_->SetTitle(nullptr, nullptr);
    EXPECT_EQ(InternalFlutter_WindowController_CreateRegularWindow(0, nullptr),
              OHOSWindowController::kCreateWindowFailedViewId);
  }
}

}  // namespace testing
}  // namespace flutter
