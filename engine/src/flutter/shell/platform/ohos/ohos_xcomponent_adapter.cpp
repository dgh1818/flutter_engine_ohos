/*
 * Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ohos_xcomponent_adapter.h"
#include <native_buffer/native_buffer.h>
#include <native_window/external_window.h>
#include <functional>
#include <utility>
#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"
#include "fml/trace_event.h"
#include "ohos_logging.h"
#include "types.h"
namespace flutter {

bool g_isMouseLeftActive = false;
double g_scrollDistance = 0.0;
double g_resizeRate = 0.8;

XComponentAdapter XComponentAdapter::mXComponentAdapter;

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
  TRACE_EVENT0("AttachFlutterEngine", shellholderId.c_str());
  auto iter = xcomponetMap_.find(id);
  if (iter == xcomponetMap_.end()) {
    XComponentBase* xcomponet = new XComponentBase(id);
    xcomponetMap_[id] = xcomponet;
  }

  auto findIter = xcomponetMap_.find(id);
  if (findIter != xcomponetMap_.end()) {
    findIter->second->AttachFlutterEngine(shellholderId);
  }
}

void XComponentAdapter::PreDraw(std::string& id,
                                std::string& shellholderId,
                                int width,
                                int height) {
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
  auto iter = xcomponetMap_.find(id);
  if (iter != xcomponetMap_.end()) {
    iter->second->DetachFlutterEngine();
  }
}

void XComponentAdapter::OnMouseWheel(std::string& id, mouseWheelEvent event) {
  auto iter = xcomponetMap_.find(id);
  if (iter != xcomponetMap_.end()) {
    iter->second->OnDispatchMouseWheelEvent(event);
  }
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
  for (auto it : XComponentAdapter::GetInstance()->xcomponetMap_) {
    if (it.second->nativeXComponent_ == component) {
      LOGD("OnSurfaceCreatedCB is called");
      it.second->OnSurfaceCreated(component, window);
    }
  }
}

void OnSurfaceChangedCB(OH_NativeXComponent* component, void* window) {
  for (auto it : XComponentAdapter::GetInstance()->xcomponetMap_) {
    if (it.second->nativeXComponent_ == component) {
      it.second->OnSurfaceChanged(component, window);
    }
  }
}

void OnSurfaceDestroyedCB(OH_NativeXComponent* component, void* window) {
  for (auto it = XComponentAdapter::GetInstance()->xcomponetMap_.begin();
       it != XComponentAdapter::GetInstance()->xcomponetMap_.end();) {
    if (it->second->nativeXComponent_ == component) {
      it->second->OnSurfaceDestroyed(component, window);
      delete it->second;
      it = XComponentAdapter::GetInstance()->xcomponetMap_.erase(it);
    } else {
      ++it;
    }
  }
}
void DispatchTouchEventCB(OH_NativeXComponent* component, void* window) {
  for (auto it : XComponentAdapter::GetInstance()->xcomponetMap_) {
    if (it.second->nativeXComponent_ == component) {
      it.second->OnDispatchTouchEvent(component, window);
    }
  }
}

void DispatchMouseEventCB(OH_NativeXComponent* component, void* window) {
  for (auto it : XComponentAdapter::GetInstance()->xcomponetMap_) {
    if (it.second->nativeXComponent_ == component) {
      it.second->OnDispatchMouseEvent(component, window);
    }
  }
}

void DispatchHoverEventCB(OH_NativeXComponent* component, bool isHover) {
  LOGD("XComponentManger::DispatchHoverEventCB");
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
int32_t FindAccessibilityNodeInfosById(
    int64_t elementId,
    ArkUI_AccessibilitySearchMode mode,
    int32_t requestId,
    ArkUI_AccessibilityElementInfoList* elementList) {
  auto ohosAccessibilityBridge = OhosAccessibilityBridge::GetInstance();
  ohosAccessibilityBridge->FindAccessibilityNodeInfosById(
      elementId, mode, requestId, elementList);
  LOGD("accessibilityProviderCallback_.FindAccessibilityNodeInfosById");
  return 0;
}

/** Called when need to get element infos based on a specified node and text
 * content. */
int32_t FindAccessibilityNodeInfosByText(
    int64_t elementId,
    const char* text,
    int32_t requestId,
    ArkUI_AccessibilityElementInfoList* elementList) {
  auto ohosAccessibilityBridge = OhosAccessibilityBridge::GetInstance();
  ohosAccessibilityBridge->FindAccessibilityNodeInfosByText(
      elementId, text, requestId, elementList);
  LOGD("accessibilityProviderCallback_.FindAccessibilityNodeInfosByText");
  return 0;
}

/** Called when need to get the focused element info based on a specified node.
 */
int32_t FindFocusedAccessibilityNode(
    int64_t elementId,
    ArkUI_AccessibilityFocusType focusType,
    int32_t requestId,
    ArkUI_AccessibilityElementInfo* elementinfo) {
  auto ohosAccessibilityBridge = OhosAccessibilityBridge::GetInstance();
  ohosAccessibilityBridge->FindFocusedAccessibilityNode(elementId, focusType,
                                                        requestId, elementinfo);
  LOGD("accessibilityProviderCallback_.FindFocusedAccessibilityNode");
  return 0;
}

/** Query the node that can be focused based on the reference node. Query the
 * next node that can be focused based on the mode and direction. */
int32_t FindNextFocusAccessibilityNode(
    int64_t elementId,
    ArkUI_AccessibilityFocusMoveDirection direction,
    int32_t requestId,
    ArkUI_AccessibilityElementInfo* elementList) {
  auto ohosAccessibilityBridge = OhosAccessibilityBridge::GetInstance();
  ohosAccessibilityBridge->FindNextFocusAccessibilityNode(
      elementId, direction, requestId, elementList);
  LOGD("accessibilityProviderCallback_.FindNextFocusAccessibilityNode");
  return 0;
}

/** Performing the Action operation on a specified node. */
int32_t ExecuteAccessibilityAction(
    int64_t elementId,
    ArkUI_Accessibility_ActionType action,
    ArkUI_AccessibilityActionArguments* actionArguments,
    int32_t requestId) {
  auto ohosAccessibilityBridge = OhosAccessibilityBridge::GetInstance();
  ohosAccessibilityBridge->ExecuteAccessibilityAction(
      elementId, action, actionArguments, requestId);
  LOGD("accessibilityProviderCallback_.ExecuteAccessibilityAction");
  return 0;
}

/** Clears the focus status of the currently focused node */
int32_t ClearFocusedFocusAccessibilityNode() {
  LOGD("accessibilityProviderCallback_.ClearFocusedFocusAccessibilityNode");
  return 0;
}

/** Queries the current cursor position of a specified node. */
int32_t GetAccessibilityNodeCursorPosition(int64_t elementId,
                                           int32_t requestId,
                                           int32_t* index) {
  auto ohosAccessibilityBridge = OhosAccessibilityBridge::GetInstance();
  ohosAccessibilityBridge->GetAccessibilityNodeCursorPosition(elementId,
                                                              requestId, index);
  LOGD("accessibilityProviderCallback_.GetAccessibilityNodeCursorPosition");
  return 0;
}

void XComponentBase::BindAccessibilityProviderCallback() {
  accessibilityProviderCallback_.findAccessibilityNodeInfosById =
      FindAccessibilityNodeInfosById;
  accessibilityProviderCallback_.findAccessibilityNodeInfosByText =
      FindAccessibilityNodeInfosByText;
  accessibilityProviderCallback_.findFocusedAccessibilityNode =
      FindFocusedAccessibilityNode;
  accessibilityProviderCallback_.findNextFocusAccessibilityNode =
      FindNextFocusAccessibilityNode;
  accessibilityProviderCallback_.executeAccessibilityAction =
      ExecuteAccessibilityAction;
  accessibilityProviderCallback_.clearFocusedFocusAccessibilityNode =
      ClearFocusedFocusAccessibilityNode;
  accessibilityProviderCallback_.getAccessibilityNodeCursorPosition =
      GetAccessibilityNodeCursorPosition;
}

XComponentBase::XComponentBase(std::string id) {
  id_ = id;
  is_engine_attached_ = false;
}

XComponentBase::~XComponentBase() {}

void XComponentBase::AttachFlutterEngine(std::string shellholderId) {
  LOGD(
      "XComponentManger::AttachFlutterEngine xcomponentId:%{public}s, "
      "shellholderId:%{public}s",
      id_.c_str(), shellholderId.c_str());
  shellholderId_ = shellholderId;
  is_engine_attached_ = true;
  if (window_ != nullptr) {
    PlatformViewOHOSNapi::SurfaceCreated(std::stoll(shellholderId_), window_,
                                         width_, height_);
    is_surface_present_ = true;
  }
}

void XComponentBase::PreDraw(std::string shellholderId, int width, int height) {
  LOGD(
      "AttachFlutterEngine XComponentBase is not attached---use preload "
      "%{public}d %{public}d",
      width, height);
  shellholderId_ = std::move(shellholderId);
  is_engine_attached_ = true;
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
    PlatformViewOHOSNapi::SurfaceDestroyed(std::stoll(shellholderId_));
  } else {
    LOGE("DetachFlutterEngine XComponentBase is not attached");
  }
  shellholderId_ = "";
  is_engine_attached_ = false;
  is_surface_present_ = false;
  is_surface_preloaded_ = false;
}

void XComponentBase::SetNativeXComponent(
    OH_NativeXComponent* nativeXComponent) {
  nativeXComponent_ = nativeXComponent;
  if (nativeXComponent_ != nullptr) {
    BindXComponentCallback();
    OH_NativeXComponent_RegisterCallback(nativeXComponent_, &callback_);
    OH_NativeXComponent_RegisterMouseEventCallback(nativeXComponent_,
                                                   &mouseCallback_);
    BindAccessibilityProviderCallback();
    ArkUI_AccessibilityProvider* accessibilityProvider = nullptr;
    int32_t ret1 = OH_NativeXComponent_GetNativeAccessibilityProvider(
        nativeXComponent_, &accessibilityProvider);
    if (ret1 != 0) {
      LOGE("OH_NativeXComponent_GetNativeAccessibilityProvider is failed");
      return;
    }
    int32_t ret2 = OH_ArkUI_AccessibilityProviderRegisterCallback(
        accessibilityProvider, &accessibilityProviderCallback_);
    if (ret2 != 0) {
      LOGE("OH_ArkUI_AccessibilityProviderRegisterCallback is failed");
      return;
    }
    LOGE("OH_ArkUI_AccessibilityProviderRegisterCallback is %{public}d", ret2);

    // 将ArkUI_AccessibilityProvider传到无障碍bridge类
    auto ohosAccessibilityBridge = OhosAccessibilityBridge::GetInstance();
    ohosAccessibilityBridge->provider_ = accessibilityProvider;

    LOGI(
        "XComponentBase::SetNativeXComponent "
        "OH_ArkUI_AccessibilityProviderRegisterCallback is succeed");
  }
}

void XComponentBase::OnSurfaceCreated(OH_NativeXComponent* component,
                                      void* window) {
  LOGD(
      "XComponentManger::OnSurfaceCreated window = %{public}p component = "
      "%{public}p",
      window, component);
  TRACE_EVENT0("OnSurfaceCreated", shellholderId_.c_str());
  window_ = window;
  int32_t ret = OH_NativeXComponent_GetXComponentSize(component, window,
                                                      &width_, &height_);
  if (ret == OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
    LOGD("XComponent Current width:%{public}d,height:%{public}d",
         static_cast<int>(width_), static_cast<int>(height_));
  } else {
    LOGE("GetXComponentSize result:%{public}d", ret);
  }

  LOGD("OnSurfaceCreated,window.size:%{public}d,%{public}d", (int)width_,
       (int)height_);
  ret = SetNativeWindowOpt((OHNativeWindow*)window, width_, height_);
  if (ret) {
    LOGE("SetNativeWindowOpt failed:%{public}d", ret);
  }
  if (is_engine_attached_) {
    ret = OH_NativeWindow_NativeObjectReference(window);
    if (ret) {
      LOGE("NativeObjectReference failed:%{public}d", ret);
    }
    PlatformViewOHOSNapi::SurfaceCreated(std::stoll(shellholderId_), window,
                                         width_, height_);
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
    PlatformViewOHOSNapi::SurfaceChanged(std::stoll(shellholderId_), window,
                                         width_, height_);
  } else {
    LOGE("OnSurfaceChanged XComponentBase is not attached");
  }
}

void XComponentBase::OnSurfaceDestroyed(OH_NativeXComponent* component,
                                        void* window) {
  window_ = nullptr;
  LOGD("XComponentManger::OnSurfaceDestroyed");
  if (is_engine_attached_) {
    is_surface_present_ = false;
    is_surface_preloaded_ = false;
    PlatformViewOHOSNapi::SurfaceDestroyed(std::stoll(shellholderId_));
    int32_t ret = OH_NativeWindow_NativeObjectUnreference(window);
    if (ret) {
      LOGE("NativeObjectReference failed:%{public}d", ret);
    }
  } else {
    LOGE("XComponentManger::OnSurfaceDestroyed XComponentBase is not attached");
  }
}

void XComponentBase::OnDispatchTouchEvent(OH_NativeXComponent* component,
                                          void* window) {
  int32_t ret =
      OH_NativeXComponent_GetTouchEvent(component, window, &touchEvent_);
  if (ret == OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
    if (is_engine_attached_ && is_surface_present_) {
      // if this touchEvent triggered by mouse, return
      OH_NativeXComponent_EventSourceType sourceType;
      int32_t ret2 = OH_NativeXComponent_GetTouchEventSourceType(
          component, touchEvent_.id, &sourceType);
      if (ret2 == OH_NATIVEXCOMPONENT_RESULT_SUCCESS &&
          sourceType == OH_NATIVEXCOMPONENT_SOURCE_TYPE_MOUSE) {
        ohosTouchProcessor_.HandleVirtualTouchEvent(std::stoll(shellholderId_),
                                                    component, &touchEvent_);
        return;
      }
      ohosTouchProcessor_.HandleTouchEvent(std::stoll(shellholderId_),
                                           component, &touchEvent_);
    } else {
      LOGE(
          "XComponentManger::DispatchTouchEvent XComponentBase is not "
          "attached");
    }
  }
}

void XComponentBase::OnDispatchMouseEvent(OH_NativeXComponent* component,
                                          void* window) {
  OH_NativeXComponent_MouseEvent mouseEvent;
  int32_t ret =
      OH_NativeXComponent_GetMouseEvent(component, window, &mouseEvent);
  if (ret == OH_NATIVEXCOMPONENT_RESULT_SUCCESS && is_engine_attached_) {
    if (mouseEvent.button == OH_NATIVEXCOMPONENT_LEFT_BUTTON) {
      if (mouseEvent.action == OH_NATIVEXCOMPONENT_MOUSE_PRESS) {
        g_isMouseLeftActive = true;
      } else if (mouseEvent.action == OH_NATIVEXCOMPONENT_MOUSE_RELEASE) {
        g_isMouseLeftActive = false;
      }
    }
    ohosTouchProcessor_.HandleMouseEvent(std::stoll(shellholderId_), component,
                                         mouseEvent, 0.0);
    return;
  }
  LOGE("XComponentManger::DispatchMouseEvent XComponentBase is not attached");
}

void XComponentBase::OnDispatchMouseWheelEvent(mouseWheelEvent event) {
  std::string shell_holder_str = std::to_string(event.shellHolder);
  if (shell_holder_str != shellholderId_) {
    return;
  }
  if (is_engine_attached_) {
    if (g_isMouseLeftActive) {
      return;
    }
    if (event.eventType == "actionUpdate") {
      OH_NativeXComponent_MouseEvent mouseEvent;
      double scrollY = event.offsetY - g_scrollDistance;
      g_scrollDistance = event.offsetY;
      // fix resize ratio
      mouseEvent.x = event.globalX / g_resizeRate;
      mouseEvent.y = event.globalY / g_resizeRate;
      scrollY = scrollY / g_resizeRate;
      mouseEvent.button = OH_NATIVEXCOMPONENT_NONE_BUTTON;
      mouseEvent.action = OH_NATIVEXCOMPONENT_MOUSE_NONE;
      mouseEvent.timestamp = event.timestamp;
      ohosTouchProcessor_.HandleMouseEvent(std::stoll(shellholderId_), nullptr,
                                           mouseEvent, scrollY);
    } else {
      g_scrollDistance = 0.0;
    }
  } else {
    LOGE(
        "XComponentManger::DispatchMouseWheelEvent XComponentBase is not "
        "attached");
  }
}
}  // namespace flutter