/*
 * Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd.
 * All rights reserved. Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE_KHZG file.
 */

#include "ohos_xcomponent_adapter.h"
#include <ace/xcomponent/native_interface_xcomponent.h>
#include <native_buffer/native_buffer.h>
#include <native_window/external_window.h>
#include <stdint.h>
#include <cerrno>
#include <cinttypes>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <utility>
#include <vector>
#include "accessibility/ohos_semantics_bridge.h"
#include "fml/trace_event.h"
#include "napi/platform_view_ohos_napi.h"
#include "ohos_logging.h"
#include "ohos_shell_holder.h"
#include "shell/common/shell.h"
#include "types.h"
namespace flutter {
const int32_t OHOS_API_VERSION = OH_GetSdkApiVersion();

bool g_isMouseLeftActive = false;
double g_scrollDistance = 0.0;
double g_resizeRate = 0.8;

// HCPP overlay XComponent id convention: an overlay surface's id is
// `<mainViewId>__overlay`. The native adapter routes its OHNativeWindow to
// the engine that owns <mainViewId> (see SetHybridCompositionOverlayWindow),
// without attaching it as the main Flutter surface.
static constexpr const char* kOverlayIdSuffix = "__overlay";

static bool IsOverlayId(const std::string& id) {
  if (id.size() <= std::strlen(kOverlayIdSuffix)) {
    return false;
  }
  return id.compare(id.size() - std::strlen(kOverlayIdSuffix),
                    std::strlen(kOverlayIdSuffix), kOverlayIdSuffix) == 0;
}

static std::string StripOverlaySuffix(const std::string& id) {
  if (!IsOverlayId(id)) {
    return id;
  }
  return id.substr(0, id.size() - std::strlen(kOverlayIdSuffix));
}

XComponentAdapter XComponentAdapter::mXComponentAdapter;

std::unique_ptr<DynamicLibraryLoader> XComponentBase::loader_ =
    std::make_unique<DynamicLibraryLoader>(ARKUI_ACE_LIB_NAME);

XComponentAdapter::XComponentAdapter(/* args */) {}

XComponentAdapter::~XComponentAdapter() {}

XComponentAdapter* XComponentAdapter::GetInstance() {
  return &XComponentAdapter::mXComponentAdapter;
}

bool XComponentAdapter::Export(napi_env env, napi_value exports) {
  napi_status status;
  napi_value exportInstance = nullptr;
  OH_NativeXComponent* nativeXComponent = nullptr;
  int32_t ret;
  char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {};
  uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;

  status = napi_get_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ,
                                   &exportInstance);
  LOGD("napi_get_named_property,status = %{public}d", status);
  if (status != napi_ok) {
    return false;
  }

  status = napi_unwrap(env, exportInstance,
                       reinterpret_cast<void**>(&nativeXComponent));
  LOGD("napi_unwrap,status = %{public}d", status);
  if (status != napi_ok) {
    return false;
  }

  ret = OH_NativeXComponent_GetXComponentId(nativeXComponent, idStr, &idSize);
  LOGD("NativeXComponent id:%{public}s", idStr);
  if (ret != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
    LOGE("OH_NativeXComponent_GetXComponentId failed, ret=%{public}d", ret);
    return false;
  }
  std::string id(idStr);
  auto context = XComponentAdapter::GetInstance();
  if (context) {
    context->SetNativeXComponent(id, nativeXComponent);
    return true;
  }

  return false;
}

void XComponentAdapter::SetNativeXComponent(
    std::string& id,
    OH_NativeXComponent* nativeXComponent) {
  std::lock_guard<std::recursive_mutex> lock(xcomponentMap_mutex_);
  auto iter = xcomponetMap_.find(id);
  if (iter == xcomponetMap_.end()) {
    XComponentBase* xcomponet = new XComponentBase(id);
    xcomponetMap_[id] = xcomponet;
  }

  iter = xcomponetMap_.find(id);
  if (iter != xcomponetMap_.end()) {
    iter->second->SetNativeXComponent(nativeXComponent);
  }
}

void XComponentAdapter::AttachFlutterEngine(std::string& id,
                                            std::string& shellholderId) {
  TRACE_EVENT1("flutter", "AttachFlutterEngine", "ShellID",
               shellholderId.c_str());
  std::lock_guard<std::recursive_mutex> lock(xcomponentMap_mutex_);
  auto iter = xcomponetMap_.find(id);
  if (iter == xcomponetMap_.end()) {
    XComponentBase* xcomponet = new XComponentBase(id);
    xcomponetMap_[id] = xcomponet;
  }

  auto findIter = xcomponetMap_.find(id);
  if (findIter != xcomponetMap_.end()) {
    findIter->second->AttachFlutterEngine(shellholderId);
  }
  if (OHOS_API_VERSION < 15) {
    SetCurrentXcomponentId(id);
  }
}

void XComponentAdapter::PreDraw(std::string& id,
                                std::string& shellholderId,
                                int width,
                                int height) {
  std::lock_guard<std::recursive_mutex> lock(xcomponentMap_mutex_);
  auto iter = xcomponetMap_.find(id);
  if (iter == xcomponetMap_.end()) {
    XComponentBase* xcomponet = new XComponentBase(id);
    xcomponetMap_[id] = xcomponet;
  }

  auto findIter = xcomponetMap_.find(id);
  if (findIter != xcomponetMap_.end()) {
    findIter->second->PreDraw(shellholderId, width, height);
  }
}

void XComponentAdapter::DetachFlutterEngine(std::string& id) {
  std::lock_guard<std::recursive_mutex> lock(xcomponentMap_mutex_);
  auto iter = xcomponetMap_.find(id);
  if (iter != xcomponetMap_.end()) {
    iter->second->DetachFlutterEngine();
  }
  if (OHOS_API_VERSION < 15 && current_xcomponent_id_ == id) {
    SetCurrentXcomponentId("");
  }
}

void XComponentAdapter::OnMouseWheel(std::string& id, mouseWheelEvent event) {
  std::lock_guard<std::recursive_mutex> lock(xcomponentMap_mutex_);
  auto iter = xcomponetMap_.find(id);
  if (iter != xcomponetMap_.end()) {
    iter->second->OnDispatchMouseWheelEvent(event);
  }
}

// It must be invoked within the xcomponentMap_mutex_ lock.
XComponentBase* XComponentAdapter::GetCurrentXcomponent() {
  auto iter = xcomponetMap_.find(current_xcomponent_id_);
  if (iter != xcomponetMap_.end()) {
    return xcomponetMap_[current_xcomponent_id_];
  }
  return nullptr;
}

XComponentBase* XComponentAdapter::GetXcomponentBase(const std::string& id) {
  auto iter = xcomponetMap_.find(id);
  if (iter != xcomponetMap_.end()) {
    return iter->second;
  }
  return nullptr;
}

void XComponentAdapter::StoreHcppOverlayPendingWindow(
    const std::string& overlay_id, void* window) {
  std::lock_guard<std::mutex> lock(hcpp_overlay_pending_mutex_);
  hcpp_overlay_pending_windows_[overlay_id] = window;
  LOGI("HCPP overlay window stashed (owner not ready) id=%{public}s",
       overlay_id.c_str());
}

void XComponentAdapter::ClearHcppOverlayPendingWindow(
    const std::string& overlay_id) {
  std::lock_guard<std::mutex> lock(hcpp_overlay_pending_mutex_);
  hcpp_overlay_pending_windows_.erase(overlay_id);
}

void XComponentAdapter::FlushHcppOverlayPendingWindows(
    const std::string& main_id) {
  void* pending = nullptr;
  {
    std::lock_guard<std::mutex> lock(hcpp_overlay_pending_mutex_);
    auto iter = hcpp_overlay_pending_windows_.find(main_id + kOverlayIdSuffix);
    if (iter == hcpp_overlay_pending_windows_.end()) {
      return;
    }
    pending = iter->second;
    hcpp_overlay_pending_windows_.erase(iter);
  }
  XComponentBase* main = GetXcomponentBase(main_id);
  if (main == nullptr || main->shellholder_ptr_ == nullptr) {
    LOGE("HCPP overlay pending flush: owner gone id=%{public}s",
         main_id.c_str());
    return;
  }
  auto platform_view = main->shellholder_ptr_->GetPlatformView();
  if (platform_view) {
    platform_view->SetHybridCompositionOverlayWindow(pending);
    LOGI("HCPP overlay window delivered (pending flush) main=%{public}s",
         main_id.c_str());
  }
}

void XComponentAdapter::SetCurrentXcomponentId(std::string id) {
  current_xcomponent_id_ = std::move(id);
}

static int32_t SetNativeWindowOpt(OHNativeWindow* nativeWindow,
                                  int32_t width,
                                  int32_t height) {
  // Set the read and write scenarios of the native window buffer.
  int code = SET_USAGE;
  int32_t ret = OH_NativeWindow_NativeWindowHandleOpt(
      nativeWindow, code,
      NATIVEBUFFER_USAGE_HW_TEXTURE | NATIVEBUFFER_USAGE_HW_RENDER |
          NATIVEBUFFER_USAGE_MEM_DMA);
  if (ret) {
    LOGE(
        "Set NativeWindow Usage Failed :window:%{public}p ,w:%{public}d x "
        "%{public}d:%{public}d",
        nativeWindow, width, height, ret);
  }
  // Set the width and height of the native window buffer.
  code = SET_BUFFER_GEOMETRY;
  ret =
      OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, width, height);
  if (ret) {
    LOGE(
        "Set NativeWindow GEOMETRY  Failed :window:%{public}p ,w:%{public}d x "
        "%{public}d:%{public}d",
        nativeWindow, width, height, ret);
  }
  // Set the format of the native window buffer.
  code = SET_FORMAT;
  int32_t format = kPixelFmtRgba8888;

  ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, format);
  if (ret) {
    LOGE(
        "Set NativeWindow kPixelFmtRgba8888   Failed :window:%{public}p "
        ",w:%{public}d x %{public}d:%{public}d",
        nativeWindow, width, height, ret);
  }
  return ret;
}

void OnSurfaceCreatedCB(OH_NativeXComponent* component, void* window) {
  std::lock_guard<std::recursive_mutex> lock(
      XComponentAdapter::GetInstance()->xcomponentMap_mutex_);
  for (auto it : XComponentAdapter::GetInstance()->xcomponetMap_) {
    if (it.second->nativeXComponent_ == component) {
      LOGD("OnSurfaceCreatedCB is called");
      it.second->OnSurfaceCreated(component, window);
    }
  }
}

void OnSurfaceChangedCB(OH_NativeXComponent* component, void* window) {
  std::lock_guard<std::recursive_mutex> lock(
      XComponentAdapter::GetInstance()->xcomponentMap_mutex_);
  for (auto it : XComponentAdapter::GetInstance()->xcomponetMap_) {
    if (it.second->nativeXComponent_ == component) {
      it.second->OnSurfaceChanged(component, window);
    }
  }
}

void OnSurfaceDestroyedCB(OH_NativeXComponent* component, void* window) {
  std::lock_guard<std::recursive_mutex> lock(
      XComponentAdapter::GetInstance()->xcomponentMap_mutex_);
  // Two passes: run every matching OnSurfaceDestroyed FIRST, then delete.
  // A page teardown destroys the main surface and its HCPP overlay in the
  // same pass, and the overlay's teardown resolves its owning engine by
  // scanning the map for the main entry — deleting mid-pass (std::map
  // iterates "main" before "main__overlay") previously removed the main
  // entry before the overlay's handler ran, skipping the overlay window
  // teardown and leaving the engine on a freed OHNativeWindow.
  std::vector<XComponentBase*> to_destroy;
  for (auto& kv : XComponentAdapter::GetInstance()->xcomponetMap_) {
    if (kv.second->nativeXComponent_ == component) {
      to_destroy.push_back(kv.second);
    }
  }
  for (XComponentBase* entry : to_destroy) {
    if (OHOS_API_VERSION < 15 &&
        entry == XComponentAdapter::GetInstance()->GetCurrentXcomponent()) {
      XComponentAdapter::GetInstance()->SetCurrentXcomponentId("");
    }
    entry->OnSurfaceDestroyed(component, window);
  }
  for (XComponentBase* entry : to_destroy) {
    XComponentAdapter::GetInstance()->xcomponetMap_.erase(entry->id_);
    delete entry;
  }
}
void DispatchTouchEventCB(OH_NativeXComponent* component, void* window) {
  std::lock_guard<std::recursive_mutex> lock(
      XComponentAdapter::GetInstance()->xcomponentMap_mutex_);
  for (auto it : XComponentAdapter::GetInstance()->xcomponetMap_) {
    if (it.second->nativeXComponent_ == component) {
      it.second->OnDispatchTouchEvent(component, window);
    }
  }
}

void DispatchAxisEventCB(OH_NativeXComponent* component,
                         ArkUI_UIInputEvent* event,
                         ArkUI_UIInputEvent_Type type) {
  std::lock_guard<std::recursive_mutex> lock(
      XComponentAdapter::GetInstance()->xcomponentMap_mutex_);
  for (auto it : XComponentAdapter::GetInstance()->xcomponetMap_) {
    if (it.second->nativeXComponent_ == component) {
      it.second->OnDispatchAxisEvent(component, event, type);
    }
  }
}

void DispatchMouseEventCB(OH_NativeXComponent* component, void* window) {
  std::lock_guard<std::recursive_mutex> lock(
      XComponentAdapter::GetInstance()->xcomponentMap_mutex_);
  for (auto it : XComponentAdapter::GetInstance()->xcomponetMap_) {
    if (it.second->nativeXComponent_ == component) {
      it.second->OnDispatchMouseEvent(component, window);
    }
  }
}

void DispatchHoverEventCB(OH_NativeXComponent* component, bool isHover) {
  LOGD("XComponentManger::DispatchHoverEventCB");
  if (!isHover) {
    for (auto it : XComponentAdapter::GetInstance()->xcomponetMap_) {
      if (it.second->nativeXComponent_ == component) {
        it.second->OnDispatchMouseLeaveEvent(component);
      }
    }
  }
}

void XComponentBase::OnDispatchMouseLeaveEvent(OH_NativeXComponent* component) {
  if (window_ == nullptr) {
    LOGE("OnDispatchMouseLeaveEvent window_ is nullptr");
    return;
  }

  OH_NativeXComponent_MouseEvent mouseEvent;
  int32_t ret = OH_NativeXComponent_GetMouseEvent(component, window_, &mouseEvent);
  if (ret != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
    LOGE("OH_NativeXComponent_GetMouseEvent (leave event) failed, ret=%{public}d", ret);
    return;
  }

  if (!is_engine_attached_) {
    LOGE("OnSurfaceCreated XComponentBase is not attached");
    return;
  }

  // Multi-view pointer routing: stamp this XComponent's view_id so emitted
  // PointerData reaches the right per-view tree.
  int64_t pointer_view_id = is_sub_view_ ? sub_view_id_ : 0;
  ohosTouchProcessor_.SetViewId(pointer_view_id);

  LOGD("XComponentManger::OnDispatchMouseLeaveEvent()");
  // the leave mouseEvent data，is the same of last point on the area.
  ohosTouchProcessor_.HandleMouseEvent(std::stoll(shellholderId_),
                                       component, mouseEvent, 0.0, true,
                                       static_cast<double>(width_),
                                       static_cast<double>(height_));
}

void XComponentBase::BindXComponentCallback() {
  callback_.OnSurfaceCreated = OnSurfaceCreatedCB;
  callback_.OnSurfaceChanged = OnSurfaceChangedCB;
  callback_.OnSurfaceDestroyed = OnSurfaceDestroyedCB;
  callback_.DispatchTouchEvent = DispatchTouchEventCB;
  mouseCallback_.DispatchMouseEvent = DispatchMouseEventCB;
  mouseCallback_.DispatchHoverEvent = DispatchHoverEventCB;
}

/** Called when need to get element infos based on a specified node. */
static int32_t FindAccessibilityNodeInfosByIdCallback(
    int64_t elementId,
    ArkUI_AccessibilitySearchMode mode,
    int32_t requestId,
    ArkUI_AccessibilityElementInfoList* elementList) {
  LOGD(
      "accessibilityProviderCallback_.FindAccessibilityNodeInfosById mode "
      "%{public}d id %{public}ld",
      mode, elementId);
  std::lock_guard<std::recursive_mutex> lock(
      XComponentAdapter::GetInstance()->xcomponentMap_mutex_);
  auto xcomp = XComponentAdapter::GetInstance()->GetCurrentXcomponent();
  if (xcomp != nullptr) {
    return xcomp->FindAccessibilityNodeInfosById(elementId, mode, requestId,
                                                 elementList);
  } else {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
}

/** Called when need to get element infos based on a specified node and text
 * content. */
int32_t FindAccessibilityNodeInfosByTextCallback(
    int64_t elementId,
    const char* text,
    int32_t requestId,
    ArkUI_AccessibilityElementInfoList* elementList) {
  LOGD("accessibilityProviderCallback_.FindAccessibilityNodeInfosByText");
  std::lock_guard<std::recursive_mutex> lock(
      XComponentAdapter::GetInstance()->xcomponentMap_mutex_);
  auto xcomp = XComponentAdapter::GetInstance()->GetCurrentXcomponent();
  if (xcomp != nullptr) {
    return xcomp->FindAccessibilityNodeInfosByText(elementId, text, requestId,
                                                   elementList);
  } else {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
}

/** Called when need to get the focused element info based on a specified node.
 */
int32_t FindFocusedAccessibilityNodeCallback(
    int64_t elementId,
    ArkUI_AccessibilityFocusType focusType,
    int32_t requestId,
    ArkUI_AccessibilityElementInfo* elementinfo) {
  LOGD("accessibilityProviderCallback_.FindFocusedAccessibilityNode");
  std::lock_guard<std::recursive_mutex> lock(
      XComponentAdapter::GetInstance()->xcomponentMap_mutex_);
  auto xcomp = XComponentAdapter::GetInstance()->GetCurrentXcomponent();
  if (xcomp != nullptr) {
    return xcomp->FindFocusedAccessibilityNode(elementId, focusType, requestId,
                                               elementinfo);
  } else {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
}

/** Query the node that can be focused based on the reference node. Query the
 * next node that can be focused based on the mode and direction. */
int32_t FindNextFocusAccessibilityNodeCallback(
    int64_t elementId,
    ArkUI_AccessibilityFocusMoveDirection direction,
    int32_t requestId,
    ArkUI_AccessibilityElementInfo* elementList) {
  LOGD("accessibilityProviderCallback_.FindNextFocusAccessibilityNode");
  std::lock_guard<std::recursive_mutex> lock(
      XComponentAdapter::GetInstance()->xcomponentMap_mutex_);
  auto xcomp = XComponentAdapter::GetInstance()->GetCurrentXcomponent();
  if (xcomp != nullptr) {
    return xcomp->FindNextFocusAccessibilityNode(elementId, direction,
                                                 requestId, elementList);
  } else {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
}

/** Performing the Action operation on a specified node. */
int32_t ExecuteAccessibilityActionCallback(
    int64_t elementId,
    ArkUI_Accessibility_ActionType action,
    ArkUI_AccessibilityActionArguments* actionArguments,
    int32_t requestId) {
  LOGD("accessibilityProviderCallback_.ExecuteAccessibilityAction");
  std::lock_guard<std::recursive_mutex> lock(
      XComponentAdapter::GetInstance()->xcomponentMap_mutex_);
  auto xcomp = XComponentAdapter::GetInstance()->GetCurrentXcomponent();
  if (xcomp != nullptr) {
    return xcomp->ExecuteAccessibilityAction(elementId, action, actionArguments,
                                             requestId);
  } else {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
}

/** Clears the focus status of the currently focused node */
int32_t ClearFocusedFocusAccessibilityNodeCallback() {
  std::lock_guard<std::recursive_mutex> lock(
      XComponentAdapter::GetInstance()->xcomponentMap_mutex_);
  auto xcomp = XComponentAdapter::GetInstance()->GetCurrentXcomponent();
  LOGD("accessibilityProviderCallback_.ClearFocusedFocusAccessibilityNode");
  if (xcomp != nullptr) {
    return xcomp->ClearFocusedFocusAccessibilityNode(0);
  } else {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
}

/** Queries the current cursor position of a specified node. */
int32_t GetAccessibilityNodeCursorPositionCallback(int64_t elementId,
                                                   int32_t requestId,
                                                   int32_t* index) {
  LOGD("accessibilityProviderCallback_.GetAccessibilityNodeCursorPosition");
  std::lock_guard<std::recursive_mutex> lock(
      XComponentAdapter::GetInstance()->xcomponentMap_mutex_);
  auto xcomp = XComponentAdapter::GetInstance()->GetCurrentXcomponent();
  if (xcomp != nullptr) {
    return xcomp->GetAccessibilityNodeCursorPosition(elementId, requestId,
                                                     index);
  } else {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
}

void XComponentBase::BindAccessibilityProviderCallback() {
  accessibilityProviderCallback_.findAccessibilityNodeInfosById =
      FindAccessibilityNodeInfosByIdCallback;
  accessibilityProviderCallback_.findAccessibilityNodeInfosByText =
      FindAccessibilityNodeInfosByTextCallback;
  accessibilityProviderCallback_.findFocusedAccessibilityNode =
      FindFocusedAccessibilityNodeCallback;
  accessibilityProviderCallback_.findNextFocusAccessibilityNode =
      FindNextFocusAccessibilityNodeCallback;
  accessibilityProviderCallback_.executeAccessibilityAction =
      ExecuteAccessibilityActionCallback;
  accessibilityProviderCallback_.clearFocusedFocusAccessibilityNode =
      ClearFocusedFocusAccessibilityNodeCallback;
  accessibilityProviderCallback_.getAccessibilityNodeCursorPosition =
      GetAccessibilityNodeCursorPositionCallback;
}

XComponentBase::XComponentBase(const std::string& id)
    : OH_ArkUI_AccessibilityProviderRegisterCallbackWithInstance_(nullptr),
      id_(id),
      is_engine_attached_(false) {
  if (OHOS_API_VERSION >= 15) {
    loader_->LoadSymbols({
        {ARKUI_REGISTER_CALLBACK_WITH_INSTANCE,
         reinterpret_cast<void**>(
             &OH_ArkUI_AccessibilityProviderRegisterCallbackWithInstance_),
         15},
    });
    multiInstanceXCompAccessibility_ =
        std::make_unique<MultiInstanceXCompAccessibility>();
  }
}

XComponentBase::~XComponentBase() {}

bool XComponentBase::IsSubViewId(const std::string& id, int64_t* out_view_id) {
  if (id.empty()) {
    return false;
  }
  const std::string kMainPrefix = "oh_flutter_";
  if (id.size() >= kMainPrefix.size() &&
      id.compare(0, kMainPrefix.size(), kMainPrefix) == 0) {
    return false;  // main-window id
  }
  for (char c : id) {
    if (c < '0' || c > '9') {
      return false;
    }
  }
  errno = 0;
  char* end = nullptr;
  long long parsed = strtoll(id.c_str(), &end, 10);
  if (errno != 0 || end == id.c_str() || *end != '\0' || parsed == 0) {
    // 0 is kFlutterImplicitViewId — not a sub-view.
    return false;
  }
  if (out_view_id) {
    *out_view_id = static_cast<int64_t>(parsed);
  }
  return true;
}

void XComponentBase::ResolveSubViewRouting() {
  // Lexical parse first (no engine dependency).
  int64_t parsed = 0;
  if (!IsSubViewId(id_, &parsed)) {
    is_sub_view_ = false;
    sub_view_id_ = 0;
    return;
  }
  is_sub_view_ = false;
  sub_view_id_ = 0;
  if (shellholder_ptr_ != nullptr) {
    if (OHOSWindowController* controller =
            shellholder_ptr_->GetWindowController()) {
      if (controller->GetHandleForView(parsed) != nullptr) {
        is_sub_view_ = true;
        sub_view_id_ = parsed;
      }
    }
  }
  if (is_sub_view_) {
    LOGD("XComponent id=%{public}s routes as sub-view viewId=%{public}" PRId64,
         id_.c_str(), sub_view_id_);
  } else {
    LOGD("XComponent id=%{public}s routes as implicit (view 0)", id_.c_str());
  }
}

void XComponentBase::AttachFlutterEngine(std::string shellholderId) {
  LOGD(
      "XComponentManger::AttachFlutterEngine xcomponentId:%{public}s, "
      "shellholderId:%{public}s",
      id_.c_str(), shellholderId.c_str());
  shellholderId_ = shellholderId;
  shellholder_ptr_ =
      reinterpret_cast<OHOSShellHolder*>(std::stoll(shellholderId_));
  is_engine_attached_ = true;
  ResolveSubViewRouting();
  if (window_ != nullptr) {
    if (provider_ != nullptr && shellholder_ptr_) {
      shellholder_ptr_->SetAccessibilityProvider(provider_);
    }
    if (is_sub_view_) {
      PlatformViewOHOSNapi::NotifyCreateForView(
          std::stoll(shellholderId_), sub_view_id_, window_, width_, height_);
    } else {
      PlatformViewOHOSNapi::SurfaceCreated(std::stoll(shellholderId_), window_,
                                           width_, height_);
    }
    is_surface_present_ = true;
  }
  // HCPP: deliver any overlay window that arrived before this attach (the
  // overlay XComponent's OnSurfaceCreated stashed it when the engine was
  // not reachable yet). Fire-and-forget was previously a permanent drop.
  XComponentAdapter::GetInstance()->FlushHcppOverlayPendingWindows(id_);
}

void XComponentBase::PreDraw(std::string shellholderId, int width, int height) {
  LOGD(
      "AttachFlutterEngine XComponentBase is not attached---use preload "
      "%{public}d %{public}d",
      width, height);
  shellholderId_ = std::move(shellholderId);
  shellholder_ptr_ =
      reinterpret_cast<OHOSShellHolder*>(std::stoll(shellholderId_));
  is_engine_attached_ = true;
  ResolveSubViewRouting();
  if (is_surface_preloaded_) {
    return;
  }
  PlatformViewOHOSNapi::SurfacePreload(std::stoll(shellholderId_), width,
                                       height);
  is_surface_preloaded_ = true;
}

void XComponentBase::DetachFlutterEngine() {
  LOGD(
      "XComponentManger::DetachFlutterEngine xcomponentId:%{public}s, "
      "shellholderId:%{public}s",
      id_.c_str(), shellholderId_.c_str());
  if (window_ != nullptr) {
    if (is_sub_view_) {
      PlatformViewOHOSNapi::NotifyDestroyForView(std::stoll(shellholderId_),
                                                 sub_view_id_);
    } else {
      PlatformViewOHOSNapi::SurfaceDestroyed(std::stoll(shellholderId_));
    }
  } else {
    LOGE("DetachFlutterEngine XComponentBase is not attached");
  }

  if (provider_ != nullptr && shellholder_ptr_) {
    shellholder_ptr_->SetAccessibilityProvider(nullptr);
  }

  shellholderId_ = "";
  shellholder_ptr_ = nullptr;
  is_engine_attached_ = false;
  is_surface_present_ = false;
  is_surface_preloaded_ = false;
}

void XComponentBase::SetNativeXComponent(
    OH_NativeXComponent* nativeXComponent) {
  nativeXComponent_ = nativeXComponent;
  if (nativeXComponent_ != nullptr) {
    BindXComponentCallback();
    int32_t ret = OH_NativeXComponent_RegisterCallback(nativeXComponent_, &callback_);
    if (ret != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
      LOGE("OH_NativeXComponent_RegisterCallback failed, ret=%{public}d", ret);
    }
    ret = OH_NativeXComponent_RegisterMouseEventCallback(nativeXComponent_,
                                                         &mouseCallback_);
    if (ret != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
      LOGE("OH_NativeXComponent_RegisterMouseEventCallback failed, ret=%{public}d", ret);
    }
    ret = OH_NativeXComponent_RegisterUIInputEventCallback(
        nativeXComponent_, DispatchAxisEventCB, ARKUI_UIINPUTEVENT_TYPE_AXIS);
    if (ret != ARKUI_ERROR_CODE_NO_ERROR) {
      LOGE("OH_NativeXComponent_RegisterUIInputEventCallback failed, ret=%{public}d", ret);
    }
  }
}

ArkUI_AccessibilityProvider*
XComponentBase::GetArkUIAccessibilityServiceProvider(
    OH_NativeXComponent* nativeXComponent) {
  // register multi-instance accessibility provdier callback when API >= 15
  if (OHOS_API_VERSION >= 15) {
    return GetArkUIAccessibilityServiceProviderWithInstance(nativeXComponent);
  }
  BindAccessibilityProviderCallback();
  ArkUI_AccessibilityProvider* provider = nullptr;
  int32_t ret = OH_NativeXComponent_GetNativeAccessibilityProvider(
      nativeXComponent, &provider);
  if (ret != 0) {
    LOGE("OH_NativeXComponent_GetNativeAccessibilityProvider is failed");
    return nullptr;
  }
  ret = OH_ArkUI_AccessibilityProviderRegisterCallback(
      provider, &accessibilityProviderCallback_);
  if (ret != 0) {
    LOGE("OH_ArkUI_AccessibilityProviderRegisterCallback is failed");
    return nullptr;
  }
  LOGI("XComponentBase::GetArkUIAccessibilityServiceProvider -> finished");
  return provider;
}

ArkUI_AccessibilityProvider*
XComponentBase::GetArkUIAccessibilityServiceProviderWithInstance(
    OH_NativeXComponent* nativeXComponent) {
  // bind the multi-instance accessibility callbacks
  if (multiInstanceXCompAccessibility_ != nullptr) {
    multiInstanceXCompAccessibility_->BindAccessibilityCallbackWithInstance();
  } else {
    LOGE("multiInstanceAccessibility_ is nullptr");
    return nullptr;
  }
  ArkUI_AccessibilityProvider* provider = nullptr;
  int32_t ret = OH_NativeXComponent_GetNativeAccessibilityProvider(
      nativeXComponent, &provider);
  if (ret != 0) {
    LOGE("GetArkUIAccessibilityServiceProviderWithInstance is failed");
    return nullptr;
  }
  // register the accessibility callback with multi-instances
  if (OH_ArkUI_AccessibilityProviderRegisterCallbackWithInstance_ == nullptr) {
    LOGE(
        "OH_ArkUI_AccessibilityProviderRegisterCallbackWithInstance_ is "
        "nullptr");
    return nullptr;
  }
  ret = OH_ArkUI_AccessibilityProviderRegisterCallbackWithInstance_(
      id_.c_str(), provider,
      &multiInstanceXCompAccessibility_->a11yProviderCallbackWithInstance_);
  if (ret != 0) {
    LOGE("OH_ArkUI_AccessibilityProviderRegisterCallback is failed");
    return nullptr;
  }
  LOGI(
      "XComponentBase::GetArkUIAccessibilityServiceProviderWithInstance -> "
      "finished");
  return provider;
}

// See ohos_xcomponent_adapter.h for the gating contract: an overlay is an
// See ohos_xcomponent_adapter.h for the gating contract.
bool XComponentBase::IsHcppOverlay() {
  if (!IsOverlayId(id_)) {
    return false;
  }
  XComponentBase* main = XComponentAdapter::GetInstance()->GetXcomponentBase(
      StripOverlaySuffix(id_));
  if (main == nullptr) {
    return false;
  }
  if (main->shellholder_ptr_ == nullptr) {
    // Registered but not attached yet — spawn/attach race. FlutterPage only
    // creates the overlay XComponent after HCPP is confirmed on, so a
    // registered main means a genuine overlay pair.
    return true;
  }
  auto platform_view = main->shellholder_ptr_->GetPlatformView();
  return platform_view && platform_view->IsHybridCompositionEnabled();
}

// Resolves the engine that owns this overlay via its main XComponent entry
// (id with the "__overlay" suffix stripped). Returns an empty WeakPtr when
// the main entry is gone or its engine is not attached — callers must treat
// that as "no engine to talk to" instead of dereferencing a stale
// shellholder. (The two-pass OnSurfaceDestroyedCB keeps the main entry
// alive while overlay teardown runs in the same page-unload pass.)
static fml::WeakPtr<PlatformViewOHOS> GetOverlayOwnerPlatformView(
    const std::string& overlay_id) {
  XComponentAdapter* adapter = XComponentAdapter::GetInstance();
  std::lock_guard<std::recursive_mutex> lock(adapter->xcomponentMap_mutex_);
  XComponentBase* main =
      adapter->GetXcomponentBase(StripOverlaySuffix(overlay_id));
  if (main == nullptr || main->shellholder_ptr_ == nullptr) {
    return fml::WeakPtr<PlatformViewOHOS>();
  }
  return main->shellholder_ptr_->GetPlatformView();
}

void XComponentBase::OnSurfaceCreated(OH_NativeXComponent* component,
                                      void* window) {
  LOGD(
      "XComponentManger::OnSurfaceCreated window = %{public}p component = "
      "%{public}p",
      window, component);
  TRACE_EVENT1("flutter", "OnSurfaceCreated", "ShellID",
               shellholderId_.c_str());
  if (window_ != nullptr) {
    LOGE("OnSurfaceCreated with not null window %{public}p!", window_);
  }
  window_ = window;
  int32_t ret = OH_NativeXComponent_GetXComponentSize(component, window,
                                                      &width_, &height_);
  if (ret == OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
    LOGD("XComponent Current width:%{public}d,height:%{public}d",
         static_cast<int>(width_), static_cast<int>(height_));
  } else {
    LOGE("GetXComponentSize result:%{public}d", ret);
  }
  ret = OH_NativeWindow_NativeObjectReference(window_);
  if (ret) {
    LOGE("OH_NativeWindow_NativeObjectReference() failed, ret = %{public}d", ret);
  }

  // HCPP overlay XComponent: route its window to the engine that owns the
  // main XComponent (id with "__overlay" stripped, see IsHcppOverlay). The
  // overlay is NEVER attached as the main Flutter surface, so it must not
  // run the main-surface setup below (SetNeedSoftKeyboard / SetNativeWindowOpt
  // / accessibility provider / SurfaceCreated). The NativeObjectReference
  // above is balanced by the unconditional NativeObjectUnreference in
  // OnSurfaceDestroyed.
  if (IsHcppOverlay()) {
    auto platform_view = GetOverlayOwnerPlatformView(id_);
    if (platform_view) {
      // The platform view stashes the window when the HCPP embedder has not
      // been created yet (shell still starting up) and pushes it on embedder
      // creation, so an early overlay surface is no longer dropped.
      platform_view->SetHybridCompositionOverlayWindow(window);
      LOGI("HCPP overlay window registered id=%{public}s", id_.c_str());
    } else {
      // The owning engine's platform view is not reachable yet (shell still
      // creating, or the main XComponent has not attached). Stash the window;
      // FlushHcppOverlayPendingWindows delivers it as soon as the main
      // XComponent attaches the engine. Without this, the fire-once
      // OnSurfaceCreated would drop the window for the whole session and
      // every overlay region above a platform view would be silently
      // discarded.
      XComponentAdapter::GetInstance()->StoreHcppOverlayPendingWindow(id_,
                                                                      window);
    }
    return;
  }

  // This setting ensures that the soft keyboard does not automatically dismiss
  // when the Xcomponent regains focus.
  ret = OH_NativeXComponent_SetNeedSoftKeyboard(component, true);
  if (ret != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
    LOGE("OH_NativeXComponent_SetNeedSoftKeyboard failed result:%{public}d",
         ret);
  }

  LOGD("OnSurfaceCreated,window.size:%{public}d,%{public}d", (int)width_,
       (int)height_);
  ret = SetNativeWindowOpt((OHNativeWindow*)window, width_, height_);
  if (ret) {
    LOGE("SetNativeWindowOpt failed:%{public}d", ret);
  }

  provider_ = GetArkUIAccessibilityServiceProvider(nativeXComponent_);

  if (is_engine_attached_) {
    if (provider_ != nullptr && shellholder_ptr_) {
      shellholder_ptr_->SetAccessibilityProvider(provider_);
    } else {
      LOGE("OnSurfaceCreated AccessibilityProvider is nullptr");
    }

    if (is_sub_view_) {
      // Sub-window view: route its surface to the multi-view pipeline.
      PlatformViewOHOSNapi::NotifyCreateForView(
          std::stoll(shellholderId_), sub_view_id_, window, width_, height_);
    } else {
      PlatformViewOHOSNapi::SurfaceCreated(std::stoll(shellholderId_), window,
                                           width_, height_);
    }
    is_surface_present_ = true;
  } else {
    LOGE("OnSurfaceCreated XComponentBase is not attached");
  }
}

void XComponentBase::OnSurfaceChanged(OH_NativeXComponent* component,
                                      void* window) {
  LOGD("XComponentManger::OnSurfaceChanged ");
  int32_t ret = OH_NativeXComponent_GetXComponentSize(component, window,
                                                      &width_, &height_);
  if (ret == OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
    LOGD("XComponent Current width:%{public}d,height:%{public}d",
         static_cast<int>(width_), static_cast<int>(height_));
  }
  if (is_engine_attached_) {
    if (is_sub_view_) {
      // Route to THIS view's swapchain: the implicit SurfaceChanged would
      // rebuild view 0's and leave this view on a stale window (black screen).
      PlatformViewOHOSNapi::NotifySurfaceChangedForView(
          std::stoll(shellholderId_), sub_view_id_, window, width_, height_);
    } else {
      PlatformViewOHOSNapi::SurfaceChanged(std::stoll(shellholderId_), window,
                                           width_, height_);
    }
  } else {
    LOGE("OnSurfaceChanged XComponentBase is not attached");
  }
}

void XComponentBase::OnSurfaceDestroyed(OH_NativeXComponent* component,
                                        void* window) {
  if (window_ != window) {
    LOGE("OnSurfaceDestroyed with different window: %{public}p=>%{public}p",
         window_, window);
  }
  // HCPP overlay: tear the engine's borrow down BEFORE the underlying window
  // is unreferenced below, otherwise the raster thread may dereference a freed
  // OHNativeWindow. ClearHybridCompositionOverlayWindowSync drains every
  // in-flight raster use (latch on the raster task runner, same pattern as
  // NotifyDestroyed) and destroys the overlay GPU surfaces before returning.
  // The owning engine is resolved via the main XComponent entry (id with the
  // suffix stripped). The two-pass OnSurfaceDestroyedCB below keeps that
  // entry alive while the overlay's teardown runs in the same page-unload
  // pass — the pass ordering previously deleted the main entry first when
  // the main surface unmounted before the overlay's, skipping this teardown
  // and leaving the engine holding a dangling window (use-after-free).
  // Note that the overlay's own XComponentBase is being deleted by the caller
  // after this returns, so do not touch `this` members beyond window_/id_.
  if (IsHcppOverlay()) {
    auto platform_view = GetOverlayOwnerPlatformView(id_);
    if (platform_view) {
      platform_view->ClearHybridCompositionOverlayWindowSync();
      LOGI("HCPP overlay window cleared id=%{public}s", id_.c_str());
    } else {
      LOGI("HCPP overlay destroy: owning engine gone, skip clear id=%{public}s",
           id_.c_str());
    }
    // The overlay surface is gone; drop any stashed pending window (one
    // delivered earlier this session is not affected — the embedder holds
    // its own reference).
    XComponentAdapter::GetInstance()->ClearHcppOverlayPendingWindow(id_);
  }
  // Notify the engine FIRST, drop the window reference LAST: the sub-view
  // teardown holds GPU resources tied to this OHNativeWindow; unreferencing
  // first could free it under in-flight raster work (UAF).
  if (is_engine_attached_) {
    is_surface_present_ = false;
    is_surface_preloaded_ = false;
    if (is_sub_view_) {
      PlatformViewOHOSNapi::NotifyDestroyForView(std::stoll(shellholderId_),
                                                 sub_view_id_);
    } else {
      PlatformViewOHOSNapi::SurfaceDestroyed(std::stoll(shellholderId_));
    }

    if (provider_ != nullptr && shellholder_ptr_) {
      // Only the implicit main view ever set the slot (see OnSurfaceCreated)
      // — clearing here restores null, NOT the previous sub-view's provider.
      shellholder_ptr_->SetAccessibilityProvider(nullptr);
    }
    provider_ = nullptr;
  } else {
    LOGE("XComponentManger::OnSurfaceDestroyed XComponentBase is not attached");
  }
  if (window_) {
    int32_t ret = OH_NativeWindow_NativeObjectUnreference(window_);
    if (ret) {
      LOGE("OH_NativeWindow_NativeObjectUnreference() failed, ret = %{public}d", ret);
    }
  } else {
    LOGE("OnSurfaceDestroyed with null window!");
  }
  window_ = nullptr;
  LOGD("XComponentManger::OnSurfaceDestroyed");
}

void XComponentBase::OnDispatchTouchEvent(OH_NativeXComponent* component,
                                          void* window) {
  int32_t ret =
      OH_NativeXComponent_GetTouchEvent(component, window, &touchEvent_);
  if (ret != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
    LOGE("OH_NativeXComponent_GetTouchEvent failed, ret=%{public}d", ret);
    return;
  }

  if (!is_engine_attached_ || !is_surface_present_) {
    LOGE("XComponentManger::DispatchTouchEvent XComponentBase is not attached");
    return;
  }

  int64_t pointer_view_id = is_sub_view_ ? sub_view_id_ : 0;
  ohosTouchProcessor_.SetViewId(pointer_view_id);

  // if this touchEvent triggered by mouse, return
  OH_NativeXComponent_EventSourceType sourceType;
  ret = OH_NativeXComponent_GetTouchEventSourceType(
      component, touchEvent_.id, &sourceType);
  if (ret != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
    LOGE("OH_NativeXComponent_GetTouchEventSourceType failed, ret=%{public}d, treating as touch event", ret);
    ohosTouchProcessor_.HandleTouchEvent(std::stoll(shellholderId_),
                                         component, &touchEvent_);
    return;
  }

  if (sourceType == OH_NATIVEXCOMPONENT_SOURCE_TYPE_MOUSE) {
    ohosTouchProcessor_.HandleVirtualTouchEvent(std::stoll(shellholderId_),
                                                component, &touchEvent_);
  } else {
    ohosTouchProcessor_.HandleTouchEvent(std::stoll(shellholderId_),
                                         component, &touchEvent_);
  }
}

void XComponentBase::OnDispatchAxisEvent(OH_NativeXComponent* component,
                                         ArkUI_UIInputEvent* event,
                                         ArkUI_UIInputEvent_Type type) {
  if (type == ARKUI_UIINPUTEVENT_TYPE_AXIS) {
    if (is_engine_attached_ && is_surface_present_) {
      int64_t pointer_view_id = is_sub_view_ ? sub_view_id_ : 0;
      ohosTouchProcessor_.SetViewId(pointer_view_id);
      ohosTouchProcessor_.HandleAxisEvent(std::stoll(shellholderId_), component,
                                          event);
    } else {
      LOGE(
          "XComponentManger::DispatchAxisEvent XComponentBase is not attached");
    }
  }
}

void XComponentBase::OnDispatchMouseEvent(OH_NativeXComponent* component,
                                          void* window) {
  OH_NativeXComponent_MouseEvent mouseEvent;
  int32_t ret =
      OH_NativeXComponent_GetMouseEvent(component, window, &mouseEvent);
  if (ret != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
    LOGE("OH_NativeXComponent_GetMouseEvent failed, ret=%{public}d", ret);
    return;
  }

  if (!is_engine_attached_ || !is_surface_present_) {
    // Same guard as the touch path: during teardown the surface is gone but
    // the engine stays attached; a mouse event would touch freed resources.
    LOGE("XComponentManger::DispatchMouseEvent XComponentBase is not attached or surface gone");
    return;
  }

  int64_t pointer_view_id = is_sub_view_ ? sub_view_id_ : 0;
  ohosTouchProcessor_.SetViewId(pointer_view_id);

  // Update global left button state
  if (mouseEvent.button == OH_NATIVEXCOMPONENT_LEFT_BUTTON) {
    if (mouseEvent.action == OH_NATIVEXCOMPONENT_MOUSE_PRESS) {
      g_isMouseLeftActive = true;
    } else if (mouseEvent.action == OH_NATIVEXCOMPONENT_MOUSE_RELEASE) {
      g_isMouseLeftActive = false;
    }
  }

  ohosTouchProcessor_.HandleMouseEvent(std::stoll(shellholderId_), component,
                                       mouseEvent, 0.0, false,
                                       static_cast<double>(width_),
                                       static_cast<double>(height_));
}

void XComponentBase::OnDispatchMouseWheelEvent(mouseWheelEvent event) {
  std::string shell_holder_str = std::to_string(event.shellHolder);
  if (shell_holder_str != shellholderId_) {
    return;
  }
  if (is_engine_attached_ && is_surface_present_) {
    int64_t pointer_view_id = is_sub_view_ ? sub_view_id_ : 0;
    ohosTouchProcessor_.SetViewId(pointer_view_id);
    if (g_isMouseLeftActive) {
      return;
    }
    if (event.eventType == "actionUpdate") {
      OH_NativeXComponent_MouseEvent mouseEvent;
      // 调整鼠标滚轮滚动时，列表滑动的方向。和Windows保持一致。
      double scrollY = g_scrollDistance - event.offsetY;
      g_scrollDistance = event.offsetY;
      // fix resize ratio
      mouseEvent.x = event.globalX / g_resizeRate;
      mouseEvent.y = event.globalY / g_resizeRate;
      scrollY = scrollY / g_resizeRate;
      mouseEvent.button = OH_NATIVEXCOMPONENT_NONE_BUTTON;
      mouseEvent.action = OH_NATIVEXCOMPONENT_MOUSE_NONE;
      mouseEvent.timestamp = event.timestamp;
      ohosTouchProcessor_.HandleMouseEvent(std::stoll(shellholderId_), nullptr,
                                           mouseEvent, scrollY, false,
                                           static_cast<double>(width_),
                                           static_cast<double>(height_));
    } else {
      g_scrollDistance = 0.0;
    }
  } else {
    LOGE(
        "XComponentManger::DispatchMouseWheelEvent XComponentBase is not "
        "attached");
  }
}

int32_t XComponentBase::FindAccessibilityNodeInfosById(
    int64_t elementId,
    ArkUI_AccessibilitySearchMode mode,
    int32_t requestId,
    ArkUI_AccessibilityElementInfoList* elementList) {
  if (shellholder_ptr_) {
    return shellholder_ptr_->FillNodesWithSearch(elementId, mode, elementList);
  } else {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
}

int32_t XComponentBase::FindAccessibilityNodeInfosByText(
    int64_t elementId,
    const char* text,
    int32_t requestId,
    ArkUI_AccessibilityElementInfoList* elementList) {
  if (shellholder_ptr_) {
    return shellholder_ptr_->FillNodesWithSearchText(elementId, text,
                                                     elementList);
  } else {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
}

int32_t XComponentBase::FindFocusedAccessibilityNode(
    int64_t elementId,
    ArkUI_AccessibilityFocusType focusType,
    int32_t requestId,
    ArkUI_AccessibilityElementInfo* elementinfo) {
  if (shellholder_ptr_) {
    return shellholder_ptr_->FindFocusNode(elementId, focusType, elementinfo);
  } else {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
}

int32_t XComponentBase::FindNextFocusAccessibilityNode(
    int64_t elementId,
    ArkUI_AccessibilityFocusMoveDirection direction,
    int32_t requestId,
    ArkUI_AccessibilityElementInfo* elementinfo) {
  if (shellholder_ptr_) {
    return shellholder_ptr_->FindNextFocusNode(elementId, direction,
                                               elementinfo);
  } else {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
}

int32_t XComponentBase::ExecuteAccessibilityAction(
    int64_t elementId,
    ArkUI_Accessibility_ActionType action,
    ArkUI_AccessibilityActionArguments* actionArguments,
    int32_t requestId) {
  if (shellholder_ptr_) {
    return shellholder_ptr_->ExecuteAction(elementId, action, actionArguments);
  } else {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
}

int32_t XComponentBase::ClearFocusedFocusAccessibilityNode(int64_t id) {
  if (shellholder_ptr_) {
    return shellholder_ptr_->ClearAccessibilityFocus(id);
  } else {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
}

int32_t XComponentBase::GetAccessibilityNodeCursorPosition(int64_t elementId,
                                                           int32_t requestId,
                                                           int32_t* index) {
  if (shellholder_ptr_) {
    return shellholder_ptr_->GetAccessibilityNodeCursorPosition(elementId,
                                                                index);
  } else {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
}

}  // namespace flutter