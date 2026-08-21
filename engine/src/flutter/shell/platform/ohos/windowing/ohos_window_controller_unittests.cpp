/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

// Unit tests for OHOSWindowController (multi-window bookkeeping + FFI surface)
// and the facade-touching OHOSWindow RequestWindowHost dispatch.
//
// Testability contract: a real OHOSShellHolder is constructed (software
// rendering, null-env napi facade — same recipe as ohos_shell_holder_
// unittests), then `#define private public` grants access to the holder's
// napi_facade_ so the test can reset it to null. After that every controller
// path that would hop into napi (`if (auto facade = holder_->GetNapiFacade())`)
// is safely skipped (a null-env facade would crash on a real call), and every
// `if (!facade)` branch — the ones a live environment never reaches without
// a real engine — is exercised.

#include "flutter/fml/build_config.h"  // IWYU pragma: keep  (defines FML_OS_OHOS)

#if defined(FML_OS_OHOS)

#define private public
#include "flutter/shell/platform/ohos/ohos_shell_holder.h"
#undef private

#include <gtest/gtest.h>

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>

#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"
#include "flutter/shell/platform/ohos/windowing/ohos_window_dialog.h"
#include "flutter/shell/platform/ohos/windowing/ohos_window_popup.h"
#include "flutter/shell/platform/ohos/windowing/ohos_window_regular.h"
#include "flutter/shell/platform/ohos/windowing/ohos_window_tooltip.h"

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

FlutterWindowCreationRequest MakeRequest() {
  return FlutterWindowCreationRequest{};
}

OHOSWindow::InitParams MakeWindowParams(WindowType type,
                                        WindowHostKind kind,
                                        int64_t view_id,
                                        int64_t parent_view_id = 0) {
  OHOSWindow::InitParams params;
  params.type = type;
  params.host_kind = kind;
  params.view_id = view_id;
  params.parent_view_id = parent_view_id;
  params.host_handle = OHOSWindowController::HandleForViewId(view_id);
  params.adopt_entry_ability = false;
  return params;
}

// Inserts a window directly under its biased handle (CreateWindow's non-first
// path needs a live engine's AddViewSync, which is out of UT reach).
void* InsertWindow(OHOSWindowController* controller,
                   int64_t view_id,
                   WindowType type,
                   WindowHostKind kind,
                   const FlutterWindowCreationRequest& request = {}) {
  void* handle = OHOSWindowController::HandleForViewId(view_id);
  controller->windows_[handle] =
      controller->CreateWindowObject(request, MakeWindowParams(
          type, kind, view_id, request.parent_view_id));
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
  FlutterWindowRect rect{};
};

PositionerConfig* g_positioner = nullptr;

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
  EXPECT_EQ(OHOSWindowController::ResolveHostKind(WindowType::kRegular, false,
                                                  0),
            WindowHostKind::kUiAbility);
  EXPECT_EQ(OHOSWindowController::ResolveHostKind(WindowType::kRegular, true,
                                                  5),
            WindowHostKind::kUiAbility);
  // Modeless dialog (no parent) = UIAbility; modal (has_parent) = SubWindow.
  EXPECT_EQ(OHOSWindowController::ResolveHostKind(WindowType::kDialog, false,
                                                  0),
            WindowHostKind::kUiAbility);
  // parent_view_id == 0 is a LEGITIMATE parent (adopted main window).
  EXPECT_EQ(OHOSWindowController::ResolveHostKind(WindowType::kDialog, true, 0),
            WindowHostKind::kSubWindow);
  // Tooltip / Popup always SubWindow.
  EXPECT_EQ(OHOSWindowController::ResolveHostKind(WindowType::kTooltip, true,
                                                  5),
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
  EXPECT_EQ(controller_->CreateRegularWindow(MakeRequest()), 0);
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
  EXPECT_EQ(controller_->CreateRegularWindow(MakeRequest()), 0);
  void* handle = controller_->GetHandleForView(0);
  ASSERT_NE(handle, nullptr);
  EXPECT_EQ(OHOSWindowController::ForWindowHandle(handle), controller_);
  EXPECT_EQ(OHOSWindowController::ForWindowHandle(
                reinterpret_cast<void*>(0x1234)),
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
  EXPECT_EQ(controller_->CreateRegularWindow(MakeRequest()), 0);
  EXPECT_EQ(controller_->GetHandleForView(0),
            OHOSWindowController::HandleForViewId(0));
  EXPECT_EQ(controller_->GetHandleForView(77), nullptr);
}

// ---------------------------------------------------------------------------
// Window creation
// ---------------------------------------------------------------------------

TEST_F(OHOSWindowControllerTest, CreateWindowObjectDispatchesByType) {
  auto regular = controller_->CreateWindowObject(
      MakeRequest(),
      MakeWindowParams(WindowType::kRegular, WindowHostKind::kUiAbility, 1));
  EXPECT_EQ(regular->type(), WindowType::kRegular);
  EXPECT_EQ(regular->host_kind(), WindowHostKind::kUiAbility);

  auto dialog = controller_->CreateWindowObject(
      MakeRequest(),
      MakeWindowParams(WindowType::kDialog, WindowHostKind::kSubWindow, 2, 0));
  EXPECT_EQ(dialog->type(), WindowType::kDialog);

  auto tooltip = controller_->CreateWindowObject(
      MakeRequest(),
      MakeWindowParams(WindowType::kTooltip, WindowHostKind::kSubWindow, 3));
  EXPECT_EQ(tooltip->type(), WindowType::kTooltip);

  auto popup = controller_->CreateWindowObject(
      MakeRequest(),
      MakeWindowParams(WindowType::kPopup, WindowHostKind::kSubWindow, 4));
  EXPECT_EQ(popup->type(), WindowType::kPopup);
}

TEST_F(OHOSWindowControllerTest, FirstRegularWindowAdoptsViewZero) {
  auto request = MakeRequest();
  request.has_size = true;
  request.size = {640.0, 480.0};

  EXPECT_EQ(controller_->CreateRegularWindow(request), 0);
  EXPECT_TRUE(controller_->entry_ability_bound_);
  EXPECT_NE(controller_->GetHandleForView(0), nullptr);

  // A second Regular no longer adopts: AddViewSync fails without an engine.
  EXPECT_EQ(controller_->CreateRegularWindow(request),
            OHOSWindowController::kCreateWindowFailedViewId);
}

TEST_F(OHOSWindowControllerTest, NonRegularFirstWindowDoesNotAdoptViewZero) {
  // A modeless dialog first is NOT a Regular -> no view-0 adoption.
  EXPECT_EQ(controller_->CreateDialogWindow(MakeRequest()),
            OHOSWindowController::kCreateWindowFailedViewId);
  EXPECT_FALSE(controller_->entry_ability_bound_);
  // A Regular created afterwards still adopts view 0.
  EXPECT_EQ(controller_->CreateRegularWindow(MakeRequest()), 0);
  EXPECT_TRUE(controller_->entry_ability_bound_);
}

TEST_F(OHOSWindowControllerTest, TooltipAndPopupCreationFailWithoutEngine) {
  EXPECT_EQ(controller_->CreateTooltipWindow(MakeRequest()),
            OHOSWindowController::kCreateWindowFailedViewId);
  EXPECT_EQ(controller_->CreatePopupWindow(MakeRequest()),
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
  auto request = MakeRequest();
  request.on_will_close = OnWillClose;

  void* handle = InsertWindow(controller_, 5, WindowType::kDialog,
                              WindowHostKind::kSubWindow, request);
  EXPECT_NE(controller_->GetHandleForView(5), nullptr);

  controller_->DestroyWindow(handle);
  EXPECT_EQ(counters.will_close, 1);
  EXPECT_EQ(controller_->GetHandleForView(5), nullptr);
  g_counters = nullptr;
}

TEST_F(OHOSWindowControllerTest, DestroyWindowImplicitFiresWillClose) {
  CallbackCounters counters;
  g_counters = &counters;
  auto request = MakeRequest();
  request.on_will_close = OnWillClose;

  EXPECT_EQ(controller_->CreateRegularWindow(request), 0);
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
  auto request = MakeRequest();
  request.has_size = true;
  request.size = {640.0, 480.0};
  EXPECT_EQ(controller_->CreateRegularWindow(request), 0);

  // No actual size pushed -> creation request size.
  FlutterWindowSize size = controller_->GetContentSize(
      controller_->GetHandleForView(0));
  EXPECT_EQ(size.width, 640.0);
  EXPECT_EQ(size.height, 480.0);

  // Unknown window -> zero.
  FlutterWindowSize zero =
      controller_->GetContentSize(reinterpret_cast<void*>(0x123));
  EXPECT_EQ(zero.width, 0.0);
  EXPECT_EQ(zero.height, 0.0);
}

TEST_F(OHOSWindowControllerTest, SetViewActualSizeClampsAndReadsBack) {
  controller_->SetViewActualSize(0, 200.0, 100.0, /*density=*/2.0);
  controller_->SetViewActualSize(1, 100.0, 50.0, /*density=*/0.0);  // clamp 1.0

  FlutterWindowSize s0 =
      controller_->GetContentSize(OHOSWindowController::HandleForViewId(0));
  EXPECT_EQ(s0.width, 100.0);  // 200 / 2
  EXPECT_EQ(s0.height, 50.0);

  FlutterWindowSize s1 =
      controller_->GetContentSize(OHOSWindowController::HandleForViewId(1));
  EXPECT_EQ(s1.width, 100.0);  // 100 / 1
  EXPECT_EQ(s1.height, 50.0);
}

// ---------------------------------------------------------------------------
// Title
// ---------------------------------------------------------------------------

TEST_F(OHOSWindowControllerTest, SetTitleNullIgnoredAndCached) {
  controller_->SetTitle(nullptr, nullptr);  // null title -> ignored, no crash
  void* handle = InsertWindow(controller_, 5, WindowType::kTooltip,
                              WindowHostKind::kSubWindow);
  controller_->SetTitle(handle, "win title");
  char buf[32] = {};
  controller_->GetTitle(handle, buf, sizeof(buf));
  EXPECT_STREQ(buf, "win title");
}

TEST_F(OHOSWindowControllerTest, GetTitleGuardsAndTruncates) {
  controller_->GetTitle(nullptr, nullptr, 10);  // null out -> return
  void* handle = InsertWindow(controller_, 5, WindowType::kTooltip,
                              WindowHostKind::kSubWindow);
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

TEST_F(OHOSWindowControllerTest, SetConstraintsUpdatesStoredRequest) {
  void* handle = InsertWindow(controller_, 5, WindowType::kRegular,
                              WindowHostKind::kUiAbility);
  controller_->SetConstraints(handle, {100.0, 80.0, 1200.0, 800.0});
  EXPECT_EQ(controller_->LookupWindow(handle)->GetConstraints().min_width,
            100.0);
  EXPECT_EQ(controller_->LookupWindow(handle)->GetConstraints().max_width,
            1200.0);
  // Unknown window: no stored request to touch, facade null -> no crash.
  controller_->SetConstraints(OHOSWindowController::HandleForViewId(999),
                              {0.0, 0.0, 0.0, 0.0});
}

TEST_F(OHOSWindowControllerTest, NotifyListenersHitAndMiss) {
  CallbackCounters counters;
  g_counters = &counters;
  auto request = MakeRequest();
  request.notify_listeners = OnNotifyListeners;

  void* handle = InsertWindow(controller_, 5, WindowType::kTooltip,
                              WindowHostKind::kSubWindow, request);
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
  auto request = MakeRequest();
  request.notify_listeners = OnNotifyListeners;
  InsertWindow(controller_, 9, WindowType::kRegular,
               WindowHostKind::kUiAbility, request);

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
  FlutterWindowRect out{};
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
  auto request = MakeRequest();
  request.on_get_window_position = OnGetWindowPosition;
  InsertWindow(controller_, 7, WindowType::kTooltip,
               WindowHostKind::kSubWindow, request);

  FlutterWindowRect out{};
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
  auto request = MakeRequest();
  request.on_should_close = OnShouldClose;
  request.on_will_close = OnWillClose;
  InsertWindow(controller_, 5, WindowType::kTooltip,
               WindowHostKind::kSubWindow, request);

  controller_->HandleOsWindowClosed(5);
  EXPECT_EQ(counters.should_close, 1);
  EXPECT_EQ(counters.will_close, 1);
  EXPECT_EQ(controller_->GetHandleForView(5), nullptr);
  g_counters = nullptr;
}

TEST_F(OHOSWindowControllerTest, HandleOsWindowClosedImplicitSkipsRemoveView) {
  CallbackCounters counters;
  g_counters = &counters;
  auto request = MakeRequest();
  request.on_should_close = OnShouldClose;
  EXPECT_EQ(controller_->CreateRegularWindow(request), 0);

  controller_->HandleOsWindowClosed(0);
  EXPECT_EQ(counters.should_close, 1);
  EXPECT_EQ(controller_->GetHandleForView(0), nullptr);
  g_counters = nullptr;
}

TEST_F(OHOSWindowControllerTest, OnPreEngineRestartDropsAllAndResetsAdoption) {
  EXPECT_EQ(controller_->CreateRegularWindow(MakeRequest()), 0);  // view 0
  InsertWindow(controller_, 5, WindowType::kDialog,
               WindowHostKind::kSubWindow);
  InsertWindow(controller_, 6, WindowType::kTooltip,
               WindowHostKind::kSubWindow);
  EXPECT_TRUE(controller_->entry_ability_bound_);

  controller_->OnPreEngineRestart();
  EXPECT_TRUE(controller_->windows_.empty());
  EXPECT_FALSE(controller_->entry_ability_bound_);
  EXPECT_EQ(controller_->GetHandleForView(0), nullptr);
  EXPECT_EQ(controller_->GetHandleForView(5), nullptr);
  EXPECT_EQ(controller_->GetHandleForView(6), nullptr);

  // After restart a fresh Regular re-adopts view 0.
  EXPECT_EQ(controller_->CreateRegularWindow(MakeRequest()), 0);
  EXPECT_TRUE(controller_->entry_ability_bound_);
}

// ---------------------------------------------------------------------------
// OHOSWindow RequestWindowHost dispatch (null-facade controller)
// ---------------------------------------------------------------------------

TEST_F(OHOSWindowControllerTest, WindowBaseRequestWindowHostGenericSubWindow) {
  OHOSWindow base(controller_,
                  MakeWindowParams(WindowType::kTooltip,
                                   WindowHostKind::kSubWindow, 5),
                  MakeRequest());
  // Facade null -> logged + returned (base generic SubWindow path).
  base.RequestWindowHost();
  SUCCEED();
}

TEST_F(OHOSWindowControllerTest, WindowRegularRequestWindowHostBothViews) {
  // View 0: BindEntryAbilityToView branch (facade null).
  OHOSWindowRegular regular0(controller_,
                              MakeWindowParams(WindowType::kRegular,
                                               WindowHostKind::kUiAbility, 0),
                              MakeRequest());
  regular0.RequestWindowHost();
  // View 1: CreateRegularAbility branch (facade null).
  OHOSWindowRegular regular1(controller_,
                              MakeWindowParams(WindowType::kRegular,
                                               WindowHostKind::kUiAbility, 1),
                              MakeRequest());
  regular1.RequestWindowHost();
  SUCCEED();
}

TEST_F(OHOSWindowControllerTest, WindowDialogRequestWindowHostBothKinds) {
  // Modeless (kUiAbility) -> RequestUiAbilityHost (facade null).
  OHOSWindowDialog modeless(controller_,
                            MakeWindowParams(WindowType::kDialog,
                                             WindowHostKind::kUiAbility, 2),
                            MakeRequest());
  modeless.RequestWindowHost();
  // Modal (kSubWindow) -> base generic SubWindow (facade null).
  OHOSWindowDialog modal(controller_,
                         MakeWindowParams(WindowType::kDialog,
                                          WindowHostKind::kSubWindow, 3, 0),
                         MakeRequest());
  modal.RequestWindowHost();
  SUCCEED();
}

TEST_F(OHOSWindowControllerTest, WindowAnchoredRequestWindowHost) {
  OHOSWindowTooltip tooltip(controller_,
                            MakeWindowParams(WindowType::kTooltip,
                                             WindowHostKind::kSubWindow, 4),
                            MakeRequest());
  tooltip.RequestWindowHost();
  OHOSWindowPopup popup(controller_,
                        MakeWindowParams(WindowType::kPopup,
                                         WindowHostKind::kSubWindow, 5),
                        MakeRequest());
  popup.RequestWindowHost();
  SUCCEED();
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
  auto request = MakeRequest();
  request.has_size = true;
  request.size = {640.0, 480.0};
  int64_t view = InternalFlutter_WindowController_CreateRegularWindow(
      0, &request);
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
  auto request = MakeRequest();
  request.has_size = true;
  request.size = {640.0, 480.0};
  EXPECT_EQ(InternalFlutter_WindowController_CreateRegularWindow(0, &request),
            0);
  FlutterWindowSize size =
      InternalFlutter_Window_GetContentSize(
          OHOSWindowController::HandleForViewId(0));
  EXPECT_EQ(size.width, 640.0);
  EXPECT_EQ(size.height, 480.0);
  // Unknown window -> zeros.
  FlutterWindowSize zero =
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
  EXPECT_EQ(controller_->CreateRegularWindow(MakeRequest()), 0);
  EXPECT_TRUE(InternalFlutter_Window_GetActivated(0, 0));
}

TEST_F(OHOSWindowControllerTest, FfiGetOffsetFromParent) {
  InternalFlutter_Window_GetOffsetFromParent(nullptr, nullptr);  // null out
  FlutterWindowSize out{9.0, 9.0};
  InternalFlutter_Window_GetOffsetFromParent(nullptr, &out);
  EXPECT_EQ(out.width, 0.0);
  EXPECT_EQ(out.height, 0.0);
}

TEST_F(OHOSWindowControllerTest, FfiGetTitleGuards) {
  InternalFlutter_Window_GetTitle(nullptr, nullptr, 10);
  char buf[4];
  InternalFlutter_Window_GetTitle(nullptr, buf, 0);  // capacity <= 0

  void* handle = InsertWindow(controller_, 5, WindowType::kTooltip,
                              WindowHostKind::kSubWindow);
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
  auto request = MakeRequest();
  request.has_size = true;
  request.size = {640.0, 480.0};
  EXPECT_EQ(controller_->CreateRegularWindow(request), 0);

  controller_->SetViewActualSize(0, 200.0, 100.0, /*density=*/2.0);
  FlutterWindowSize size =
      controller_->GetContentSize(controller_->GetHandleForView(0));
  EXPECT_EQ(size.width, 100.0);
  EXPECT_EQ(size.height, 50.0);
}

TEST_F(OHOSWindowControllerTest, OnPreEngineRestartDoesNotFireCallbacks) {
  CallbackCounters counters;
  g_counters = &counters;
  auto request = MakeRequest();
  request.on_should_close = OnShouldClose;
  request.on_will_close = OnWillClose;
  EXPECT_EQ(controller_->CreateRegularWindow(request), 0);
  InsertWindow(controller_, 5, WindowType::kDialog, WindowHostKind::kSubWindow,
               request);

  controller_->OnPreEngineRestart();
  EXPECT_EQ(counters.should_close, 0);
  EXPECT_EQ(counters.will_close, 0);
  g_counters = nullptr;
}

TEST_F(OHOSWindowControllerTest, HandleOsWindowClosedAfterDestroyIsNoOp) {
  CallbackCounters counters;
  g_counters = &counters;
  auto request = MakeRequest();
  request.on_should_close = OnShouldClose;
  request.on_will_close = OnWillClose;
  void* handle = InsertWindow(controller_, 5, WindowType::kDialog,
                              WindowHostKind::kSubWindow, request);
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

TEST_F(OHOSWindowControllerTest, AdoptedRegularWindowSetsAdoptFlag) {
  EXPECT_EQ(controller_->CreateRegularWindow(MakeRequest()), 0);
  OHOSWindow* window =
      controller_->LookupWindow(controller_->GetHandleForView(0));
  ASSERT_NE(window, nullptr);
  EXPECT_TRUE(window->adopt_entry_ability());
}

TEST_F(OHOSWindowControllerTest, CreateDialogWithParentFailsWithoutEngine) {
  auto request = MakeRequest();
  request.has_parent = true;
  request.parent_view_id = 0;
  EXPECT_EQ(controller_->CreateDialogWindow(request),
            OHOSWindowController::kCreateWindowFailedViewId);
}

TEST_F(OHOSWindowControllerTest, FfiCreateAuxWindowsFailWithoutEngine) {
  auto request = MakeRequest();
  EXPECT_EQ(InternalFlutter_WindowController_CreateDialogWindow(0, &request),
            OHOSWindowController::kCreateWindowFailedViewId);
  EXPECT_EQ(InternalFlutter_WindowController_CreateTooltipWindow(0, &request),
            OHOSWindowController::kCreateWindowFailedViewId);
  EXPECT_EQ(InternalFlutter_WindowController_CreatePopupWindow(0, &request),
            OHOSWindowController::kCreateWindowFailedViewId);
}

TEST_F(OHOSWindowControllerTest, FfiCreateFallsBackToActiveInstance) {
  controller_->SetEngineId(42);
  auto request = MakeRequest();
  EXPECT_EQ(InternalFlutter_WindowController_CreateRegularWindow(0, &request),
            0);
}

TEST_F(OHOSWindowControllerTest, FfiCreateRejectedWhenAmbiguous) {
  auto holder2 = std::make_unique<OHOSShellHolder>(
      MakeTestSettings(), std::make_shared<PlatformViewOHOSNapi>(nullptr),
      nullptr);
  holder2->napi_facade_.reset();
  auto request = MakeRequest();
  EXPECT_EQ(InternalFlutter_WindowController_CreateRegularWindow(99, &request),
            OHOSWindowController::kCreateWindowFailedViewId);
}

TEST_F(OHOSWindowControllerTest, FfiSetTitleAndConstraintsRoundTrip) {
  void* handle = InsertWindow(controller_, 5, WindowType::kTooltip,
                              WindowHostKind::kSubWindow);
  InternalFlutter_Window_SetTitle(handle, "ffi-title");
  char out[16] = {};
  InternalFlutter_Window_GetTitle(handle, out, sizeof(out));
  EXPECT_STREQ(out, "ffi-title");

  FlutterWindowConstraints constraints{10.0, 20.0, 100.0, 200.0};
  InternalFlutter_Window_SetConstraints(handle, &constraints);
  EXPECT_EQ(controller_->LookupWindow(handle)->GetConstraints().min_width,
            10.0);

  char empty[8] = "xxxxxxx";
  InternalFlutter_Window_GetTitle(OHOSWindowController::HandleForViewId(999),
                                  empty, sizeof(empty));
  EXPECT_EQ(empty[0], '\0');
}

TEST_F(OHOSWindowControllerTest, FfiDestroyCreatedWindow) {
  EXPECT_EQ(controller_->CreateRegularWindow(MakeRequest()), 0);
  void* handle = InternalFlutter_Window_GetHandle(0, 0);
  ASSERT_NE(handle, nullptr);
  InternalFlutter_Window_Destroy(0, handle);
  EXPECT_EQ(controller_->GetHandleForView(0), nullptr);
}

TEST_F(OHOSWindowControllerTest, FfiGetActivatedAfterPush) {
  EXPECT_EQ(controller_->CreateRegularWindow(MakeRequest()), 0);
  controller_->SetViewActivated(0, false);
  EXPECT_FALSE(InternalFlutter_Window_GetActivated(0, 0));
}

TEST_F(OHOSWindowControllerTest, GetTitleUtf8RoundTrip) {
  void* handle = InsertWindow(controller_, 5, WindowType::kTooltip,
                              WindowHostKind::kSubWindow);
  controller_->SetTitle(handle, "你好");
  char buf[16] = {};
  controller_->GetTitle(handle, buf, sizeof(buf));
  EXPECT_STREQ(buf, "你好");
}

TEST_F(OHOSWindowControllerTest, ControllerStateMutationsNoCrash) {
  void* handle = InsertWindow(controller_, 5, WindowType::kRegular,
                              WindowHostKind::kUiAbility);
  FlutterWindowSize size{100.0, 80.0};
  controller_->SetContentSize(handle, size);
  controller_->Activate(handle);
  controller_->SetMaximized(handle, true);
  controller_->SetMinimized(handle, false);
  controller_->SetFullscreen(handle, true);
}

}  // namespace testing
}  // namespace flutter

#endif  // FML_OS_OHOS
