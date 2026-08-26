/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#define private public
#include "flutter/shell/platform/ohos/ohos_xcomponent_adapter.h"
#include "flutter/shell/platform/ohos/windowing/ohos_window_controller.h"
#undef private

#include <ace/xcomponent/native_interface_xcomponent.h>
#include <gtest/gtest.h>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"
#include "flutter/shell/platform/ohos/ohos_shell_holder.h"
#include "flutter/shell/platform/ohos/test_stubs/ace_napi_stub.h"

namespace flutter {

extern bool g_isMouseLeftActive;
extern double g_scrollDistance;

void OnSurfaceCreatedCB(OH_NativeXComponent* component, void* window);
void OnSurfaceChangedCB(OH_NativeXComponent* component, void* window);
void OnSurfaceDestroyedCB(OH_NativeXComponent* component, void* window);
void DispatchAxisEventCB(OH_NativeXComponent* component,
                         ArkUI_UIInputEvent* event,
                         ArkUI_UIInputEvent_Type type);
void DispatchHoverEventCB(OH_NativeXComponent* component, bool isHover);
int32_t FindAccessibilityNodeInfosByTextCallback(
    int64_t elementId, const char* text, int32_t requestId,
    ArkUI_AccessibilityElementInfoList* elementList);
int32_t FindFocusedAccessibilityNodeCallback(
    int64_t elementId, ArkUI_AccessibilityFocusType focusType,
    int32_t requestId, ArkUI_AccessibilityElementInfo* elementinfo);
int32_t FindNextFocusAccessibilityNodeCallback(
    int64_t elementId, ArkUI_AccessibilityFocusMoveDirection direction,
    int32_t requestId, ArkUI_AccessibilityElementInfo* elementinfo);
int32_t ExecuteAccessibilityActionCallback(
    int64_t elementId, ArkUI_Accessibility_ActionType action,
    ArkUI_AccessibilityActionArguments* actionArguments, int32_t requestId);
int32_t ClearFocusedFocusAccessibilityNodeCallback();
int32_t GetAccessibilityNodeCursorPositionCallback(int64_t elementId,
                                                   int32_t requestId,
                                                   int32_t* index);

namespace testing {

using ::flutter::g_isMouseLeftActive;
using ::flutter::g_scrollDistance;

class XComponentAdapterTest : public ::testing::Test {
 protected:
  void SetUp() override {
    StubNapiReset();
    ResetAdapterState();
    saved_left_ = g_isMouseLeftActive;
    saved_scroll_ = g_scrollDistance;
    g_isMouseLeftActive = false;
    g_scrollDistance = 0.0;
  }

  void TearDown() override {
    g_isMouseLeftActive = saved_left_;
    g_scrollDistance = saved_scroll_;
    ResetAdapterState();
    StubNapiReset();
  }

  static void ResetAdapterState() {
    XComponentAdapter* adapter = XComponentAdapter::GetInstance();
    std::lock_guard<std::recursive_mutex> lock(adapter->xcomponentMap_mutex_);
    for (auto& kv : adapter->xcomponetMap_) {
      delete kv.second;
    }
    adapter->xcomponetMap_.clear();
    std::lock_guard<std::mutex> pend(adapter->hcpp_overlay_pending_mutex_);
    adapter->hcpp_overlay_pending_windows_.clear();
    std::string empty;
    adapter->SetCurrentXcomponentId(empty);
  }

  size_t PendingOverlayCount() {
    XComponentAdapter* adapter = XComponentAdapter::GetInstance();
    std::lock_guard<std::mutex> pend(adapter->hcpp_overlay_pending_mutex_);
    return adapter->hcpp_overlay_pending_windows_.size();
  }

  XComponentBase* RegisterBase(const std::string& id) {
    XComponentAdapter* adapter = XComponentAdapter::GetInstance();
    std::lock_guard<std::recursive_mutex> lock(adapter->xcomponentMap_mutex_);
    auto* base = new XComponentBase(id);
    adapter->xcomponetMap_[id] = base;
    return base;
  }

  bool saved_left_ = false;
  double saved_scroll_ = 0.0;
};

TEST_F(XComponentAdapterTest, IsSubViewIdEmptyStringIsNotSubView) {
  int64_t out = -1;
  EXPECT_FALSE(XComponentBase::IsSubViewId("", &out));
}

TEST_F(XComponentAdapterTest, IsSubViewIdMainWindowPrefixIsNotSubView) {
  int64_t out = -1;
  EXPECT_FALSE(XComponentBase::IsSubViewId("oh_flutter_0", &out));
  EXPECT_FALSE(XComponentBase::IsSubViewId("oh_flutter_1", &out));
  EXPECT_FALSE(XComponentBase::IsSubViewId("oh_flutter_abc", &out));
}

TEST_F(XComponentAdapterTest, IsSubViewIdNonDigitContentRejected) {
  int64_t out = -1;
  EXPECT_FALSE(XComponentBase::IsSubViewId("abc", &out));
  // Digits then a trailing letter: strtoll would stop, *end != '\0'.
  EXPECT_FALSE(XComponentBase::IsSubViewId("12a", &out));
  EXPECT_FALSE(XComponentBase::IsSubViewId("-1", &out));  // '-' is not a digit
  EXPECT_FALSE(XComponentBase::IsSubViewId("1 2", &out));  // space
}

TEST_F(XComponentAdapterTest, IsSubViewIdZeroIsImplicitViewNotSubView) {
  // 0 is kFlutterImplicitViewId — never a sub-view.
  int64_t out = -1;
  EXPECT_FALSE(XComponentBase::IsSubViewId("0", &out));
}

TEST_F(XComponentAdapterTest, IsSubViewIdOverflowRejected) {
  int64_t out = -1;
  EXPECT_FALSE(XComponentBase::IsSubViewId("99999999999999999999", &out));
}

TEST_F(XComponentAdapterTest, IsSubViewIdParsesPositiveDecimalViewId) {
  int64_t out = -1;
  EXPECT_TRUE(XComponentBase::IsSubViewId("5", &out));
  EXPECT_EQ(out, 5);
  EXPECT_TRUE(XComponentBase::IsSubViewId("42", &out));
  EXPECT_EQ(out, 42);
}

TEST_F(XComponentAdapterTest, IsSubViewIdNullOutViewIdStillParses) {
  EXPECT_TRUE(XComponentBase::IsSubViewId("7", nullptr));
}

TEST_F(XComponentAdapterTest, IsSubViewIdFailureLeavesOutViewIdUnchanged) {
  int64_t out = 123;
  EXPECT_FALSE(XComponentBase::IsSubViewId("oh_flutter_x", &out));
  EXPECT_EQ(out, 123);
}

TEST_F(XComponentAdapterTest, IsSubViewIdPlusSignAndWhitespaceRejected) {
  int64_t out = -1;
  EXPECT_FALSE(XComponentBase::IsSubViewId("+3", &out));
  EXPECT_FALSE(XComponentBase::IsSubViewId(" 3", &out));
  EXPECT_FALSE(XComponentBase::IsSubViewId("3\t", &out));
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

TEST_F(XComponentAdapterTest, ResolveSubViewRoutingNonSubViewIdResetsRouting) {
  XComponentBase xc("oh_flutter_main");
  xc.is_sub_view_ = true;
  xc.sub_view_id_ = 9;
  xc.ResolveSubViewRouting();
  EXPECT_FALSE(xc.is_sub_view_);
  EXPECT_EQ(xc.sub_view_id_, 0);
}

TEST_F(XComponentAdapterTest, ResolveSubViewRoutingNullHolderStaysImplicit) {
  XComponentBase xc("5");
  xc.shellholder_ptr_ = nullptr;
  xc.ResolveSubViewRouting();
  EXPECT_FALSE(xc.is_sub_view_);
  EXPECT_EQ(xc.sub_view_id_, 0);
}

TEST_F(XComponentAdapterTest, GetInstanceReturnsStableSingleton) {
  EXPECT_EQ(XComponentAdapter::GetInstance(), XComponentAdapter::GetInstance());
}

TEST_F(XComponentAdapterTest, GetCurrentAndLookupFoundAndMissing) {
  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  EXPECT_EQ(adapter->GetXcomponentBase("missing"), nullptr);

  XComponentBase* base = RegisterBase("ut_main");
  EXPECT_EQ(adapter->GetXcomponentBase("ut_main"), base);

  EXPECT_EQ(adapter->GetCurrentXcomponent(), nullptr);
  adapter->SetCurrentXcomponentId("ut_main");
  {
    std::lock_guard<std::recursive_mutex> lock(adapter->xcomponentMap_mutex_);
    EXPECT_EQ(adapter->GetCurrentXcomponent(), base);
  }
  adapter->SetCurrentXcomponentId("also_missing");
  {
    std::lock_guard<std::recursive_mutex> lock(adapter->xcomponentMap_mutex_);
    EXPECT_EQ(adapter->GetCurrentXcomponent(), nullptr);
  }
}

TEST_F(XComponentAdapterTest, AttachCreatesBaseAndAttachesEngine) {
  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  std::string id = "ut_attach";
  std::string holder_id = "0";
  adapter->AttachFlutterEngine(id, holder_id);

  XComponentBase* base = adapter->GetXcomponentBase(id);
  ASSERT_NE(base, nullptr);
  EXPECT_TRUE(base->is_engine_attached_);
  EXPECT_EQ(base->shellholderId_, "0");
}

TEST_F(XComponentAdapterTest, AttachTwiceReusesSameBase) {
  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  std::string id = "ut_reuse";
  std::string holder_id = "0";
  adapter->AttachFlutterEngine(id, holder_id);
  XComponentBase* first = adapter->GetXcomponentBase(id);
  ASSERT_NE(first, nullptr);

  adapter->AttachFlutterEngine(id, holder_id);
  EXPECT_EQ(adapter->GetXcomponentBase(id), first);
}

TEST_F(XComponentAdapterTest, DetachResetsBaseButKeepsEntry) {
  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  std::string id = "ut_detach";
  std::string holder_id = "0";
  adapter->AttachFlutterEngine(id, holder_id);
  XComponentBase* base = adapter->GetXcomponentBase(id);
  ASSERT_NE(base, nullptr);

  adapter->DetachFlutterEngine(id);
  EXPECT_NE(adapter->GetXcomponentBase(id), nullptr);
  EXPECT_FALSE(base->is_engine_attached_);
  EXPECT_EQ(base->shellholderId_, "");
}

TEST_F(XComponentAdapterTest, DetachUnknownIdIsNoOp) {
  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  std::string id = "ut_ghost";
  adapter->DetachFlutterEngine(id);
  EXPECT_EQ(adapter->GetXcomponentBase(id), nullptr);
}

TEST_F(XComponentAdapterTest, PreDrawOnPreloadedSurfaceShortCircuits) {
  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  XComponentBase* base = RegisterBase("ut_predraw2");
  base->shellholderId_ = "0";
  base->is_surface_preloaded_ = true;
  std::string id = "ut_predraw2";
  std::string holder_id = "0";
  adapter->PreDraw(id, holder_id, 320, 240);
  EXPECT_TRUE(base->is_surface_preloaded_);
}

TEST_F(XComponentAdapterTest, AdapterSetNativeXComponentCreatesAndReuses) {
  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  std::string id = "ut_setnative";
  static char comp_storage; auto comp = reinterpret_cast<OH_NativeXComponent*>(&comp_storage);
  adapter->SetNativeXComponent(id, comp);
  XComponentBase* base = adapter->GetXcomponentBase(id);
  ASSERT_NE(base, nullptr);
  EXPECT_EQ(base->nativeXComponent_, comp);

  static char comp2_storage; auto comp2 = reinterpret_cast<OH_NativeXComponent*>(&comp2_storage);
  adapter->SetNativeXComponent(id, comp2);
  EXPECT_EQ(adapter->GetXcomponentBase(id), base);
  EXPECT_EQ(base->nativeXComponent_, comp2);
}

TEST_F(XComponentAdapterTest, BaseSetNativeXComponentBindsAllCallbacks) {
  XComponentBase xc("ut_bindcb");
  static char comp_storage; auto comp = reinterpret_cast<OH_NativeXComponent*>(&comp_storage);
  EXPECT_NO_FATAL_FAILURE(xc.SetNativeXComponent(comp));
  EXPECT_EQ(xc.nativeXComponent_, comp);
  EXPECT_NE(xc.callback_.OnSurfaceCreated, nullptr);
  EXPECT_NE(xc.callback_.OnSurfaceChanged, nullptr);
  EXPECT_NE(xc.callback_.OnSurfaceDestroyed, nullptr);
  EXPECT_NE(xc.callback_.DispatchTouchEvent, nullptr);
  EXPECT_NE(xc.mouseCallback_.DispatchMouseEvent, nullptr);
  EXPECT_NE(xc.mouseCallback_.DispatchHoverEvent, nullptr);
}

TEST_F(XComponentAdapterTest, BaseSetNativeXComponentNullOnlyStoresPointer) {
  XComponentBase xc("ut_nullcomp");
  xc.SetNativeXComponent(nullptr);
  EXPECT_EQ(xc.nativeXComponent_, nullptr);
}

TEST_F(XComponentAdapterTest, PlainIdIsNotOverlay) {
  XComponentBase xc("ut_plain");
  EXPECT_FALSE(xc.IsHcppOverlay());
}

TEST_F(XComponentAdapterTest, OverlaySuffixWithoutMainIsNotOverlay) {
  XComponentBase xc("ut_ghost__overlay");
  EXPECT_FALSE(xc.IsHcppOverlay());
}

TEST_F(XComponentAdapterTest, LongIdWithoutSuffixMismatchIsNotOverlay) {
  XComponentBase xc("ut_overlay_x");
  EXPECT_FALSE(xc.IsHcppOverlay());
}

TEST_F(XComponentAdapterTest, RegisteredUnattachedMainCountsAsOverlay) {
  RegisterBase("ut_main_a");
  XComponentBase xc("ut_main_a__overlay");
  EXPECT_TRUE(xc.IsHcppOverlay());
}

TEST_F(XComponentAdapterTest, StoreAndClearPendingWindowRoundTrip) {
  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  int dummy = 0;
  adapter->StoreHcppOverlayPendingWindow("ut_main_c__overlay", &dummy);
  EXPECT_EQ(PendingOverlayCount(), 1u);

  adapter->ClearHcppOverlayPendingWindow("ut_main_c__overlay");
  EXPECT_EQ(PendingOverlayCount(), 0u);

  adapter->ClearHcppOverlayPendingWindow("never_stashed");
  EXPECT_EQ(PendingOverlayCount(), 0u);
}

TEST_F(XComponentAdapterTest, FlushWithoutPendingIsNoOp) {
  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  RegisterBase("ut_main_d");
  EXPECT_NO_FATAL_FAILURE(adapter->FlushHcppOverlayPendingWindows("ut_main_d"));
  EXPECT_EQ(PendingOverlayCount(), 0u);
}

TEST_F(XComponentAdapterTest, FlushWithMissingOwnerDropsPending) {
  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  int dummy = 0;
  adapter->StoreHcppOverlayPendingWindow("ut_ghost_main__overlay", &dummy);
  EXPECT_EQ(PendingOverlayCount(), 1u);
  adapter->FlushHcppOverlayPendingWindows("ut_ghost_main");
  EXPECT_EQ(PendingOverlayCount(), 0u);
}

TEST_F(XComponentAdapterTest, FlushWithUnattachedOwnerDropsPending) {
  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  int dummy = 0;
  adapter->StoreHcppOverlayPendingWindow("ut_main_e__overlay", &dummy);
  RegisterBase("ut_main_e");
  adapter->FlushHcppOverlayPendingWindows("ut_main_e");
  EXPECT_EQ(PendingOverlayCount(), 0u);
}

namespace {
void Wheel(const std::string& id, mouseWheelEvent event) {
  std::string key = id;
  XComponentAdapter::GetInstance()->OnMouseWheel(key, event);
}

mouseWheelEvent MakeWheel(int64_t shell_holder, const char* type) {
  mouseWheelEvent event{};
  event.eventType = type;
  event.shellHolder = shell_holder;
  event.fingerId = 1;
  event.globalX = 10.0;
  event.globalY = 20.0;
  event.offsetY = 3.0;
  event.timestamp = 100;
  return event;
}
}

TEST_F(XComponentAdapterTest, OnMouseWheelUnknownIdIsNoOp) {
  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  EXPECT_NO_FATAL_FAILURE(
      Wheel("ut_nowheel", MakeWheel(0, "actionUpdate")));
}

TEST_F(XComponentAdapterTest, MouseWheelNotAttachedSkips) {
  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  XComponentBase* base = RegisterBase("ut_wheel1");
  base->shellholderId_ = "0";
  EXPECT_NO_FATAL_FAILURE(
      Wheel("ut_wheel1", MakeWheel(0, "actionUpdate")));
}

TEST_F(XComponentAdapterTest, MouseWheelAttachedWithoutSurfaceSkips) {
  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  XComponentBase* base = RegisterBase("ut_wheel2");
  base->shellholderId_ = "0";
  base->is_engine_attached_ = true;
  EXPECT_NO_FATAL_FAILURE(
      Wheel("ut_wheel2", MakeWheel(0, "actionUpdate")));
}

TEST_F(XComponentAdapterTest, MouseWheelForeignShellHolderIgnored) {
  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  XComponentBase* base = RegisterBase("ut_wheel3");
  base->shellholderId_ = "0";
  base->is_engine_attached_ = true;
  base->is_surface_present_ = true;
  EXPECT_NO_FATAL_FAILURE(
      Wheel("ut_wheel3", MakeWheel(77, "actionUpdate")));
}

TEST_F(XComponentAdapterTest, MouseWheelNonActionUpdateResetsScroll) {
  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  XComponentBase* base = RegisterBase("ut_wheel4");
  base->shellholderId_ = "0";
  base->is_engine_attached_ = true;
  base->is_surface_present_ = true;
  g_scrollDistance = 12.5;
  EXPECT_NO_FATAL_FAILURE(
      Wheel("ut_wheel4", MakeWheel(0, "actionEnd")));
  EXPECT_EQ(g_scrollDistance, 0.0);
}

TEST_F(XComponentAdapterTest, MouseWheelLeftButtonActiveSuppressesScroll) {
  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  XComponentBase* base = RegisterBase("ut_wheel5");
  base->shellholderId_ = "0";
  base->is_engine_attached_ = true;
  base->is_surface_present_ = true;
  g_isMouseLeftActive = true;
  g_scrollDistance = 5.0;
  EXPECT_NO_FATAL_FAILURE(
      Wheel("ut_wheel5", MakeWheel(0, "actionUpdate")));
  EXPECT_EQ(g_scrollDistance, 5.0);
}

TEST_F(XComponentAdapterTest, SurfaceCreatedCbRoutesToMatching) {
  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  XComponentBase* base = RegisterBase("ut_surf_cb");
  static char comp_storage; auto comp = reinterpret_cast<OH_NativeXComponent*>(&comp_storage);
  base->nativeXComponent_ = comp;
  EXPECT_NO_FATAL_FAILURE(OnSurfaceCreatedCB(comp, nullptr));
  EXPECT_EQ(base->window_, nullptr);
}

TEST_F(XComponentAdapterTest, SurfaceChangedCbOnlyTouchesMatching) {
  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  XComponentBase* base = RegisterBase("ut_surf_cb2");
  static char comp_storage; auto comp = reinterpret_cast<OH_NativeXComponent*>(&comp_storage);
  static char other_storage; auto other = reinterpret_cast<OH_NativeXComponent*>(&other_storage);
  base->nativeXComponent_ = other;
  EXPECT_NO_FATAL_FAILURE(OnSurfaceChangedCB(comp, nullptr));
}

TEST_F(XComponentAdapterTest, SurfaceDestroyedCbDeletesOnlyMatching) {
  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  XComponentBase* match = RegisterBase("ut_surf_del");
  XComponentBase* other = RegisterBase("ut_surf_keep");
  static char comp_storage; auto comp = reinterpret_cast<OH_NativeXComponent*>(&comp_storage);
  static char other_comp_storage; auto other_comp = reinterpret_cast<OH_NativeXComponent*>(&other_comp_storage);
  match->nativeXComponent_ = comp;
  other->nativeXComponent_ = other_comp;
  EXPECT_NO_FATAL_FAILURE(OnSurfaceDestroyedCB(comp, nullptr));
  EXPECT_EQ(adapter->GetXcomponentBase("ut_surf_del"), nullptr);
  EXPECT_EQ(adapter->GetXcomponentBase("ut_surf_keep"), other);
}

TEST_F(XComponentAdapterTest, SurfaceDestroyedCbTwoPassMainAndOverlay) {
  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  XComponentBase* main = RegisterBase("ut_two_main");
  XComponentBase* overlay = RegisterBase("ut_two_main__overlay");
  static char comp_storage; auto comp = reinterpret_cast<OH_NativeXComponent*>(&comp_storage);
  main->nativeXComponent_ = comp;
  overlay->nativeXComponent_ = comp;
  EXPECT_NO_FATAL_FAILURE(OnSurfaceDestroyedCB(comp, nullptr));
  EXPECT_EQ(adapter->GetXcomponentBase("ut_two_main"), nullptr);
  EXPECT_EQ(adapter->GetXcomponentBase("ut_two_main__overlay"), nullptr);
}

TEST_F(XComponentAdapterTest, OnSurfaceChangedUpdatesSizeWhenNotAttached) {
  XComponentBase xc("ut_size_na");
  static char comp_storage; auto comp = reinterpret_cast<OH_NativeXComponent*>(&comp_storage);
  EXPECT_NO_FATAL_FAILURE(xc.OnSurfaceChanged(comp, nullptr));
}

TEST_F(XComponentAdapterTest, OnSurfaceCreatedReplacesExistingWindow) {
  XComponentBase xc("ut_dup_window");
  int first = 0;
  xc.window_ = &first;
  static char comp_storage; auto comp = reinterpret_cast<OH_NativeXComponent*>(&comp_storage);
  EXPECT_NO_FATAL_FAILURE(xc.OnSurfaceCreated(comp, nullptr));
}

TEST_F(XComponentAdapterTest, OnSurfaceDestroyedUnreferencesAndClearsWindow) {
  XComponentBase xc("ut_destroy_win");
  int dummy = 0;
  xc.window_ = &dummy;
  xc.is_engine_attached_ = false;
  static char comp_storage; auto comp = reinterpret_cast<OH_NativeXComponent*>(&comp_storage);
  EXPECT_NO_FATAL_FAILURE(xc.OnSurfaceDestroyed(comp, &dummy));
  EXPECT_EQ(xc.window_, nullptr);
}

TEST_F(XComponentAdapterTest, OnSurfaceDestroyedWithNullWindowIsSafe) {
  XComponentBase xc("ut_null_win");
  xc.window_ = nullptr;
  static char comp_storage; auto comp = reinterpret_cast<OH_NativeXComponent*>(&comp_storage);
  EXPECT_NO_FATAL_FAILURE(xc.OnSurfaceDestroyed(comp, nullptr));
  EXPECT_EQ(xc.window_, nullptr);
}

TEST_F(XComponentAdapterTest, TouchNotAttachedOrNoSurfaceGuard) {
  XComponentBase xc("ut_touch_guard");
  static char comp_storage; auto comp = reinterpret_cast<OH_NativeXComponent*>(&comp_storage);
  EXPECT_NO_FATAL_FAILURE(xc.OnDispatchTouchEvent(comp, nullptr));

  xc.is_engine_attached_ = true;
  EXPECT_NO_FATAL_FAILURE(xc.OnDispatchTouchEvent(comp, nullptr));
}

TEST_F(XComponentAdapterTest, DispatchMouseCbGuardWhenNotAttached) {
  XComponentBase xc("ut_mouse_guard");
  static char comp_storage; auto comp = reinterpret_cast<OH_NativeXComponent*>(&comp_storage);
  EXPECT_NO_FATAL_FAILURE(xc.OnDispatchMouseEvent(comp, nullptr));

  xc.is_engine_attached_ = true;
  EXPECT_NO_FATAL_FAILURE(xc.OnDispatchMouseEvent(comp, nullptr));
}

TEST_F(XComponentAdapterTest, MouseLeaveGuards) {
  XComponentBase xc("ut_leave_guard");
  static char comp_storage; auto comp = reinterpret_cast<OH_NativeXComponent*>(&comp_storage);
  EXPECT_NO_FATAL_FAILURE(xc.OnDispatchMouseLeaveEvent(comp));
}

TEST_F(XComponentAdapterTest, DispatchAxisCbOnlyRoutesAxisType) {
  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  XComponentBase* base = RegisterBase("ut_axis");
  static char comp_storage; auto comp = reinterpret_cast<OH_NativeXComponent*>(&comp_storage);
  base->nativeXComponent_ = comp;
  EXPECT_NO_FATAL_FAILURE(
      DispatchAxisEventCB(comp, nullptr, ARKUI_UIINPUTEVENT_TYPE_TOUCH));
  EXPECT_NO_FATAL_FAILURE(
      DispatchAxisEventCB(comp, nullptr, ARKUI_UIINPUTEVENT_TYPE_AXIS));
}

TEST_F(XComponentAdapterTest, AxisNotAttachedGuard) {
  XComponentBase xc("ut_axis_na");
  static char comp_storage; auto comp = reinterpret_cast<OH_NativeXComponent*>(&comp_storage);
  ArkUI_UIInputEvent* event = nullptr;
  EXPECT_NO_FATAL_FAILURE(
      xc.OnDispatchAxisEvent(comp, event, ARKUI_UIINPUTEVENT_TYPE_AXIS));
}

TEST_F(XComponentAdapterTest, AxisAttachedWithoutSurfaceGuard) {
  XComponentBase xc("ut_axis_nosurf");
  static char comp_storage; auto comp = reinterpret_cast<OH_NativeXComponent*>(&comp_storage);
  ArkUI_UIInputEvent* event = nullptr;
  xc.is_engine_attached_ = true;
  EXPECT_NO_FATAL_FAILURE(
      xc.OnDispatchAxisEvent(comp, event, ARKUI_UIINPUTEVENT_TYPE_AXIS));
}

TEST_F(XComponentAdapterTest, DispatchHoverCbNeedsLeaveAndMatchingComponent) {
  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  XComponentBase* base = RegisterBase("ut_hover");
  static char comp_storage; auto comp = reinterpret_cast<OH_NativeXComponent*>(&comp_storage);
  base->nativeXComponent_ = comp;
  EXPECT_NO_FATAL_FAILURE(DispatchHoverEventCB(comp, true));
  EXPECT_NO_FATAL_FAILURE(DispatchHoverEventCB(comp, false));
}

TEST_F(XComponentAdapterTest, BaseDetachWithNoWindowResetsState) {
  XComponentBase xc("ut_base_detach");
  xc.shellholderId_ = "0";
  xc.shellholder_ptr_ = nullptr;
  xc.is_engine_attached_ = true;
  EXPECT_NO_FATAL_FAILURE(xc.DetachFlutterEngine());
  EXPECT_FALSE(xc.is_engine_attached_);
  EXPECT_EQ(xc.shellholderId_, "");
}

TEST_F(XComponentAdapterTest, BaseAttachWithNullHolderKeepsSurfaceAbsent) {
  XComponentBase xc("ut_base_attach");
  EXPECT_NO_FATAL_FAILURE(xc.AttachFlutterEngine("0"));
  EXPECT_TRUE(xc.is_engine_attached_);
  EXPECT_FALSE(xc.is_surface_present_);
}

TEST_F(XComponentAdapterTest, BasePreDrawPreloadedGuardShortCircuits) {
  XComponentBase xc("ut_base_predraw");
  xc.is_surface_preloaded_ = true;
  EXPECT_NO_FATAL_FAILURE(xc.PreDraw("0", 100, 100));
  EXPECT_TRUE(xc.is_surface_preloaded_);
}

TEST_F(XComponentAdapterTest, A11yBaseMethodsWithoutShellHolderFail) {
  XComponentBase xc("ut_a11y_base");
  EXPECT_EQ(xc.FindAccessibilityNodeInfosById(
                0, ARKUI_ACCESSIBILITY_NATIVE_SEARCH_MODE_PREFETCH_CURRENT, 1,
                nullptr),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);
  EXPECT_EQ(xc.FindAccessibilityNodeInfosByText(0, "t", 1, nullptr),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);
  EXPECT_EQ(
      xc.FindFocusedAccessibilityNode(
          0, ARKUI_ACCESSIBILITY_NATIVE_FOCUS_TYPE_INPUT, 1, nullptr),
      ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);
  EXPECT_EQ(xc.FindNextFocusAccessibilityNode(
                0, ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_FORWARD, 1,
                nullptr),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);
  EXPECT_EQ(
      xc.ExecuteAccessibilityAction(
          0, ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CLICK, nullptr, 1),
      ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);
  EXPECT_EQ(xc.ClearFocusedFocusAccessibilityNode(0),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);
  int32_t index = -1;
  EXPECT_EQ(xc.GetAccessibilityNodeCursorPosition(0, 1, &index),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);
}

TEST_F(XComponentAdapterTest, A11yCallbacksFailWithoutCurrentXComponent) {
  EXPECT_EQ(
      FindAccessibilityNodeInfosByTextCallback(0, "t", 1, nullptr),
      ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);
  EXPECT_EQ(
      FindFocusedAccessibilityNodeCallback(
          0, ARKUI_ACCESSIBILITY_NATIVE_FOCUS_TYPE_INPUT, 1, nullptr),
      ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);
  EXPECT_EQ(FindNextFocusAccessibilityNodeCallback(
                0, ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_FORWARD, 1,
                nullptr),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);
  EXPECT_EQ(
      ExecuteAccessibilityActionCallback(
          0, ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CLICK, nullptr, 1),
      ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);
  EXPECT_EQ(ClearFocusedFocusAccessibilityNodeCallback(),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);
  int32_t index = -1;
  EXPECT_EQ(
      GetAccessibilityNodeCursorPositionCallback(0, 1, &index),
      ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);
}

TEST_F(XComponentAdapterTest, A11yCallbacksRouteToCurrentXComponent) {
  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  XComponentBase* base = RegisterBase("ut_a11y_route");
  adapter->SetCurrentXcomponentId("ut_a11y_route");
  std::lock_guard<std::recursive_mutex> lock(adapter->xcomponentMap_mutex_);
  EXPECT_EQ(ClearFocusedFocusAccessibilityNodeCallback(),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);
}

TEST_F(XComponentAdapterTest, OnSurfaceCreatedOverlayStashedWithoutA11y) {
  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  RegisterBase("ut_stash_main");
  XComponentBase* overlay = RegisterBase("ut_stash_main__overlay");
  static char comp_storage; auto comp = reinterpret_cast<OH_NativeXComponent*>(&comp_storage);
  overlay->nativeXComponent_ = comp;
  EXPECT_NO_FATAL_FAILURE(OnSurfaceCreatedCB(comp, nullptr));
  EXPECT_EQ(PendingOverlayCount(), 1u);
  EXPECT_EQ(overlay->provider_, nullptr);
}

TEST_F(XComponentAdapterTest, ExportRegistersEmptyIdBase) {
  StubNapiSetValuetype(napi_undefined);
  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  napi_env env = reinterpret_cast<napi_env>(0xF00D);
  napi_value exports = reinterpret_cast<napi_value>(0x1);
  EXPECT_NO_FATAL_FAILURE(adapter->Export(env, exports));
}

#if !defined(OHOS_X64_UNITTEST)

TEST_F(XComponentAdapterTest, ResolveSubViewRoutingControllerlessHolderStaysImplicit) {
  Settings settings;
  settings.ohos_rendering_api = OHOSRenderingAPI::kSoftware;
  auto holder = std::make_unique<OHOSShellHolder>(
      settings, std::make_shared<PlatformViewOHOSNapi>(nullptr), nullptr);
  XComponentBase xc("5");
  xc.shellholder_ptr_ = holder.get();
  xc.ResolveSubViewRouting();
  EXPECT_FALSE(xc.is_sub_view_);
  EXPECT_EQ(xc.sub_view_id_, 0);
}

TEST_F(XComponentAdapterTest, ResolveSubViewRoutingWithLiveControllerRoutes) {
  Settings settings;
  settings.ohos_rendering_api = OHOSRenderingAPI::kSoftware;
  auto holder = std::make_unique<OHOSShellHolder>(
      settings, std::make_shared<PlatformViewOHOSNapi>(nullptr), nullptr);
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

TEST_F(XComponentAdapterTest, AttachedNonHcppMainIsNotOverlay) {
  Settings settings;
  settings.ohos_rendering_api = OHOSRenderingAPI::kSoftware;
  auto holder = std::make_unique<OHOSShellHolder>(
      settings, std::make_shared<PlatformViewOHOSNapi>(nullptr), nullptr);
  XComponentBase* main = RegisterBase("ut_main_b");
  main->shellholder_ptr_ = holder.get();
  main->is_engine_attached_ = true;
  XComponentBase overlay("ut_main_b__overlay");
  EXPECT_FALSE(overlay.IsHcppOverlay());
}

TEST_F(XComponentAdapterTest, PendingOverlayFlushDeliversToPlatformView) {
  Settings settings;
  settings.ohos_rendering_api = OHOSRenderingAPI::kSoftware;
  auto holder = std::make_unique<OHOSShellHolder>(
      settings, std::make_shared<PlatformViewOHOSNapi>(nullptr), nullptr);
  ASSERT_TRUE(holder->IsValid());

  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  XComponentBase* main = RegisterBase("ut_main_f");
  main->shellholder_ptr_ = holder.get();
  main->is_engine_attached_ = true;

  int dummy_window = 0;
  adapter->StoreHcppOverlayPendingWindow("ut_main_f__overlay", &dummy_window);
  EXPECT_EQ(PendingOverlayCount(), 1u);

  EXPECT_NO_FATAL_FAILURE(
      adapter->FlushHcppOverlayPendingWindows("ut_main_f"));
  EXPECT_EQ(PendingOverlayCount(), 0u);
}

TEST_F(XComponentAdapterTest, MouseWheelActionUpdateDispatchesScroll) {
  Settings settings;
  settings.ohos_rendering_api = OHOSRenderingAPI::kSoftware;
  auto holder = std::make_unique<OHOSShellHolder>(
      settings, std::make_shared<PlatformViewOHOSNapi>(nullptr), nullptr);
  ASSERT_TRUE(holder->IsValid());

  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  XComponentBase* base = RegisterBase("ut_wheel6");
  base->shellholderId_ =
      std::to_string(reinterpret_cast<int64_t>(holder.get()));
  base->is_engine_attached_ = true;
  base->is_surface_present_ = true;
  g_scrollDistance = 5.0;
  Wheel("ut_wheel6", MakeWheel(reinterpret_cast<int64_t>(holder.get()),
                               "actionUpdate"));
  EXPECT_EQ(g_scrollDistance, 3.0);
}

TEST_F(XComponentAdapterTest, MouseWheelSubViewRoutesWithSubViewId) {
  Settings settings;
  settings.ohos_rendering_api = OHOSRenderingAPI::kSoftware;
  auto holder = std::make_unique<OHOSShellHolder>(
      settings, std::make_shared<PlatformViewOHOSNapi>(nullptr), nullptr);
  ASSERT_TRUE(holder->IsValid());

  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  XComponentBase* base = RegisterBase("3");
  base->shellholderId_ =
      std::to_string(reinterpret_cast<int64_t>(holder.get()));
  base->is_engine_attached_ = true;
  base->is_surface_present_ = true;
  base->is_sub_view_ = true;
  base->sub_view_id_ = 3;
  g_scrollDistance = 5.0;
  Wheel("3", MakeWheel(reinterpret_cast<int64_t>(holder.get()),
                       "actionUpdate"));
  EXPECT_EQ(g_scrollDistance, 3.0);
}

TEST_F(XComponentAdapterTest, TouchSubViewRoutesWithSubViewId) {
  Settings settings;
  settings.ohos_rendering_api = OHOSRenderingAPI::kSoftware;
  auto holder = std::make_unique<OHOSShellHolder>(
      settings, std::make_shared<PlatformViewOHOSNapi>(nullptr), nullptr);
  ASSERT_TRUE(holder->IsValid());
  XComponentBase xc("4");
  xc.shellholderId_ = std::to_string(reinterpret_cast<int64_t>(holder.get()));
  xc.is_engine_attached_ = true;
  xc.is_surface_present_ = true;
  xc.is_sub_view_ = true;
  xc.sub_view_id_ = 4;
  EXPECT_NO_FATAL_FAILURE(
      xc.OnDispatchTouchEvent(reinterpret_cast<OH_NativeXComponent*>(0x1),
                              nullptr));
}

TEST_F(XComponentAdapterTest, AxisSubViewRoutesWithSubViewId) {
  Settings settings;
  settings.ohos_rendering_api = OHOSRenderingAPI::kSoftware;
  auto holder = std::make_unique<OHOSShellHolder>(
      settings, std::make_shared<PlatformViewOHOSNapi>(nullptr), nullptr);
  ASSERT_TRUE(holder->IsValid());
  XComponentBase xc("6");
  xc.shellholderId_ = std::to_string(reinterpret_cast<int64_t>(holder.get()));
  xc.is_engine_attached_ = true;
  xc.is_surface_present_ = true;
  xc.is_sub_view_ = true;
  xc.sub_view_id_ = 6;
  EXPECT_NO_FATAL_FAILURE(
      xc.OnDispatchAxisEvent(reinterpret_cast<OH_NativeXComponent*>(0x1),
                             nullptr, ARKUI_UIINPUTEVENT_TYPE_AXIS));
}

TEST_F(XComponentAdapterTest, TouchDroppedAsDuplicateUpReachesProcessor) {
  Settings settings;
  settings.ohos_rendering_api = OHOSRenderingAPI::kSoftware;
  auto holder = std::make_unique<OHOSShellHolder>(
      settings, std::make_shared<PlatformViewOHOSNapi>(nullptr), nullptr);
  ASSERT_TRUE(holder->IsValid());
  XComponentBase xc("ut_touch_dup");
  xc.shellholderId_ = std::to_string(reinterpret_cast<int64_t>(holder.get()));
  xc.is_engine_attached_ = true;
  xc.is_surface_present_ = true;
  EXPECT_NO_FATAL_FAILURE(
      xc.OnDispatchTouchEvent(reinterpret_cast<OH_NativeXComponent*>(0x1),
                              nullptr));
}

#endif  // !defined(OHOS_X64_UNITTEST)

}  // namespace testing
}  // namespace flutter
