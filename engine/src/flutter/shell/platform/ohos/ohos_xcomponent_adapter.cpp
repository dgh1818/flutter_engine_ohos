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
#include <cstddef>
#include <functional>
#include <utility>
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
  std::lock_guard<std::mutex> lock(xcomponentMap_mutex_);
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
  std::lock_guard<std::mutex> lock(xcomponentMap_mutex_);
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
  std::lock_guard<std::mutex> lock(xcomponentMap_mutex_);
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
  std::lock_guard<std::mutex> lock(xcomponentMap_mutex_);
  auto iter = xcomponetMap_.find(id);
  if (iter != xcomponetMap_.end()) {
    iter->second->DetachFlutterEngine();
  }
  if (OHOS_API_VERSION < 15 && current_xcomponent_id_ == id) {
    SetCurrentXcomponentId("");
  }
}

void XComponentAdapter::OnMouseWheel(std::string& id, mouseWheelEvent event) {
  std::lock_guard<std::mutex> lock(xcomponentMap_mutex_);
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
  std::lock_guard<std::mutex> lock(
      XComponentAdapter::GetInstance()->xcomponentMap_mutex_);
  for (auto it : XComponentAdapter::GetInstance()->xcomponetMap_) {
    if (it.second->nativeXComponent_ == component) {
      LOGD("OnSurfaceCreatedCB is called");
      it.second->OnSurfaceCreated(component, window);
    }
  }
}

void OnSurfaceChangedCB(OH_NativeXComponent* component, void* window) {
  std::lock_guard<std::mutex> lock(
      XComponentAdapter::GetInstance()->xcomponentMap_mutex_);
  for (auto it : XComponentAdapter::GetInstance()->xcomponetMap_) {
    if (it.second->nativeXComponent_ == component) {
      it.second->OnSurfaceChanged(component, window);
    }
  }
}

void OnSurfaceDestroyedCB(OH_NativeXComponent* component, void* window) {
  std::lock_guard<std::mutex> lock(
      XComponentAdapter::GetInstance()->xcomponentMap_mutex_);
  for (auto it = XComponentAdapter::GetInstance()->xcomponetMap_.begin();
       it != XComponentAdapter::GetInstance()->xcomponetMap_.end();) {
    if (it->second->nativeXComponent_ == component) {
      if (OHOS_API_VERSION < 15 &&
          it->second ==
              XComponentAdapter::GetInstance()->GetCurrentXcomponent()) {
        XComponentAdapter::GetInstance()->SetCurrentXcomponentId("");
      }
      it->second->OnSurfaceDestroyed(component, window);
      delete it->second;
      it = XComponentAdapter::GetInstance()->xcomponetMap_.erase(it);
    } else {
      ++it;
    }
  }
}
void DispatchTouchEventCB(OH_NativeXComponent* component, void* window) {
  std::lock_guard<std::mutex> lock(
      XComponentAdapter::GetInstance()->xcomponentMap_mutex_);
  for (auto it : XComponentAdapter::GetInstance()->xcomponetMap_) {
    if (it.second->nativeXComponent_ == component) {
      it.second->OnDispatchTouchEvent(component, window);
    }
  }
}

void DispatchMouseEventCB(OH_NativeXComponent* component, void* window) {
  std::lock_guard<std::mutex> lock(
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
  if (window_ != nullptr) {
    OH_NativeXComponent_MouseEvent mouseEvent;
    int32_t ret =
        OH_NativeXComponent_GetMouseEvent(component, window_, &mouseEvent);
    if (ret == OH_NATIVEXCOMPONENT_RESULT_SUCCESS && is_engine_attached_) {
      LOGD("XComponentManger::OnDispatchMouseLeaveEvent()");
      // the leave mouseEvent data，is the same of last point on the area.
      ohosTouchProcessor_.HandleMouseEvent(std::stoll(shellholderId_),
                                           component, mouseEvent, 0.0, true);
    }
  } else {
    LOGE("OnSurfaceCreated XComponentBase is not attached");
  }
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
  std::lock_guard<std::mutex> lock(
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
  std::lock_guard<std::mutex> lock(
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
  std::lock_guard<std::mutex> lock(
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
  std::lock_guard<std::mutex> lock(
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
  std::lock_guard<std::mutex> lock(
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
  std::lock_guard<std::mutex> lock(
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
  std::lock_guard<std::mutex> lock(
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

void XComponentBase::AttachFlutterEngine(std::string shellholderId) {
  LOGD(
      "XComponentManger::AttachFlutterEngine xcomponentId:%{public}s, "
      "shellholderId:%{public}s",
      id_.c_str(), shellholderId.c_str());
  shellholderId_ = shellholderId;
  shellholder_ptr_ =
      reinterpret_cast<OHOSShellHolder*>(std::stoll(shellholderId_));
  is_engine_attached_ = true;
  if (window_ != nullptr) {
    if (provider_ != nullptr && shellholder_ptr_) {
      shellholder_ptr_->SetAccessibilityProvider(provider_);
    }
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
  shellholder_ptr_ =
      reinterpret_cast<OHOSShellHolder*>(std::stoll(shellholderId_));
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
    OH_NativeXComponent_RegisterCallback(nativeXComponent_, &callback_);
    OH_NativeXComponent_RegisterMouseEventCallback(nativeXComponent_,
                                                   &mouseCallback_);
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
    LOGE("NativeObjectReference failed:%{public}d", ret);
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
  if (window_ != window) {
    LOGE("OnSurfaceDestroyed with different window: %{public}p=>%{public}p",
         window_, window);
  }
  if (window_) {
    int32_t ret = OH_NativeWindow_NativeObjectUnreference(window_);
    if (ret) {
      LOGE("NativeObjectReference failed:%{public}d", ret);
    }
  } else {
    LOGE("OnSurfaceDestroyed with null window!");
  }
  window_ = nullptr;
  LOGD("XComponentManger::OnSurfaceDestroyed");
  if (is_engine_attached_) {
    is_surface_present_ = false;
    is_surface_preloaded_ = false;
    PlatformViewOHOSNapi::SurfaceDestroyed(std::stoll(shellholderId_));

    if (provider_ != nullptr && shellholder_ptr_) {
      shellholder_ptr_->SetAccessibilityProvider(nullptr);
    }
    provider_ = nullptr;

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