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

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_OHOS_XCOMPONENT_ADAPTER_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_OHOS_XCOMPONENT_ADAPTER_H_
#include <ace/xcomponent/native_interface_xcomponent.h>
#include <arkui/native_interface_accessibility.h>
#include <map>
#include <string>
#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"
#include "flutter/shell/platform/ohos/ohos_touch_processor.h"
#include "napi/native_api.h"
#include "napi_common.h"
#include "ohos_shell_holder.h"
namespace flutter {

class XComponentBase {
 private:
  void BindXComponentCallback();
  void BindAccessibilityProviderCallback();

 public:
  XComponentBase(std::string id);
  ~XComponentBase();

  void AttachFlutterEngine(std::string shellholderId);
  void PreDraw(std::string shellholderId, int width, int height);
  void DetachFlutterEngine();
  void SetNativeXComponent(OH_NativeXComponent* nativeXComponent);

  // Callback, called by ACE XComponent
  void OnSurfaceCreated(OH_NativeXComponent* component, void* window);
  void OnSurfaceChanged(OH_NativeXComponent* component, void* window);
  void OnSurfaceDestroyed(OH_NativeXComponent* component, void* window);
  void OnDispatchTouchEvent(OH_NativeXComponent* component, void* window);
  void OnDispatchMouseEvent(OH_NativeXComponent* component, void* window);
  void OnDispatchMouseWheelEvent(mouseWheelEvent event);

  // Accessibility callback
  int32_t FindAccessibilityNodeInfosById(
      int64_t elementId,
      ArkUI_AccessibilitySearchMode mode,
      int32_t requestId,
      ArkUI_AccessibilityElementInfoList* elementList);
  int32_t FindAccessibilityNodeInfosByText(
      int64_t elementId,
      const char* text,
      int32_t requestId,
      ArkUI_AccessibilityElementInfoList* elementList);
  int32_t FindFocusedAccessibilityNode(
      int64_t elementId,
      ArkUI_AccessibilityFocusType focusType,
      int32_t requestId,
      ArkUI_AccessibilityElementInfo* elementinfo);
  int32_t FindNextFocusAccessibilityNode(
      int64_t elementId,
      ArkUI_AccessibilityFocusMoveDirection direction,
      int32_t requestId,
      ArkUI_AccessibilityElementInfo* elementList);
  int32_t ExecuteAccessibilityAction(
      int64_t elementId,
      ArkUI_Accessibility_ActionType action,
      ArkUI_AccessibilityActionArguments* actionArguments,
      int32_t requestId);
  int32_t ClearFocusedFocusAccessibilityNode(int64_t elementId);
  int32_t GetAccessibilityNodeCursorPosition(int64_t elementId,
                                             int32_t requestId,
                                             int32_t* index);

  ArkUI_AccessibilityProvider* GetArkUIAccessibilityServiceProvider(
      OH_NativeXComponent* nativeXComponent);

  OH_NativeXComponent_TouchEvent touchEvent_;
  OH_NativeXComponent_Callback callback_;
  OH_NativeXComponent_MouseEvent_Callback mouseCallback_;
  ArkUI_AccessibilityProviderCallbacks accessibilityProviderCallback_;
  std::string id_;
  std::string shellholderId_;
  OHOSShellHolder* shellholder_ptr_ = nullptr;
  bool is_engine_attached_ = false;
  bool is_surface_present_ = false;
  bool is_surface_preloaded_ = false;
  OH_NativeXComponent* nativeXComponent_ = nullptr;
  ArkUI_AccessibilityProvider* provider_ = nullptr;
  void* window_ = nullptr;
  uint64_t width_ = 0;
  uint64_t height_ = 0;
  OhosTouchProcessor ohosTouchProcessor_;
};

class XComponentAdapter {
 public:
  XComponentAdapter(/* args */);
  ~XComponentAdapter();
  static XComponentAdapter* GetInstance();
  bool Export(napi_env env, napi_value exports);
  void SetNativeXComponent(std::string& id,
                           OH_NativeXComponent* nativeXComponent);
  void AttachFlutterEngine(std::string& id, std::string& shellholderId);
  void PreDraw(std::string& id,
               std::string& shellholderId,
               int width,
               int height);
  void DetachFlutterEngine(std::string& id);
  void OnMouseWheel(std::string& id, mouseWheelEvent event);
  XComponentBase* GetCurrentXcomponent();
  void SetCurrentXcomponentId(std::string id);

 public:
  std::map<std::string, XComponentBase*> xcomponetMap_;
  std::mutex xcomponentMap_mutex_;

 private:
  std::string current_xcomponent_id_ = "";
  static XComponentAdapter mXComponentAdapter;
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_OHOS_OHOS_XCOMPONENT_ADAPTER_H_