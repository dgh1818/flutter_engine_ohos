/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */


#include "ace/xcomponent/native_interface_xcomponent.h"
#include "arkui/native_interface_accessibility.h"

namespace {

char g_dummy_element_info;
char g_dummy_event_info;

}  // namespace

extern "C" {

int32_t OH_NativeXComponent_GetXComponentId(OH_NativeXComponent* /*component*/,
                                            char* id,
                                            uint64_t* size) {
  if (id != nullptr && size != nullptr && *size > 0) {
    id[0] = '\0';
    *size = 0;
  }
  return OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
}

int32_t OH_NativeXComponent_GetXComponentSize(OH_NativeXComponent* /*component*/,
                                              const void* /*window*/,
                                              uint64_t* width,
                                              uint64_t* height) {
  if (width != nullptr) {
    *width = 0;
  }
  if (height != nullptr) {
    *height = 0;
  }
  return OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
}

int32_t OH_NativeXComponent_GetTouchEvent(
    OH_NativeXComponent* /*component*/,
    const void* /*window*/,
    OH_NativeXComponent_TouchEvent* /*touchEvent*/) {
  return OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
}

int32_t OH_NativeXComponent_GetMouseEvent(
    OH_NativeXComponent* /*component*/,
    const void* /*window*/,
    OH_NativeXComponent_MouseEvent* /*mouseEvent*/) {
  return OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
}

int32_t OH_NativeXComponent_RegisterCallback(
    OH_NativeXComponent* /*component*/,
    OH_NativeXComponent_Callback* /*callback*/) {
  return OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
}

int32_t OH_NativeXComponent_RegisterMouseEventCallback(
    OH_NativeXComponent* /*component*/,
    OH_NativeXComponent_MouseEvent_Callback* /*callback*/) {
  return OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
}

int32_t OH_NativeXComponent_RegisterUIInputEventCallback(
    OH_NativeXComponent* /*component*/,
    void (* /*callback*/)(OH_NativeXComponent* component,
                          ArkUI_UIInputEvent* event,
                          ArkUI_UIInputEvent_Type type),
    ArkUI_UIInputEvent_Type /*type*/) {
  return OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
}

int32_t OH_NativeXComponent_SetNeedSoftKeyboard(
    OH_NativeXComponent* /*component*/,
    bool /*needSoftKeyboard*/) {
  return OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
}

int32_t OH_NativeXComponent_GetTouchEventSourceType(
    OH_NativeXComponent* /*component*/,
    int32_t /*pointId*/,
    OH_NativeXComponent_EventSourceType* sourceType) {
  if (sourceType != nullptr) {
    *sourceType = OH_NATIVEXCOMPONENT_SOURCE_TYPE_UNKNOWN;
  }
  return OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
}

int32_t OH_NativeXComponent_GetNativeAccessibilityProvider(
    OH_NativeXComponent* /*component*/,
    ArkUI_AccessibilityProvider** handle) {
  if (handle != nullptr) {
    *handle = nullptr;
  }
  return OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
}

int32_t OH_ArkUI_AccessibilityProviderRegisterCallback(
    ArkUI_AccessibilityProvider* /*provider*/,
    ArkUI_AccessibilityProviderCallbacks* /*callbacks*/) {
  return 0;
}

void OH_ArkUI_SendAccessibilityAsyncEvent(
    ArkUI_AccessibilityProvider* /*provider*/,
    ArkUI_AccessibilityEventInfo* /*eventInfo*/,
    void (*callback)(int32_t errorCode)) {
  if (callback != nullptr) {
    callback(0);
  }
}

ArkUI_AccessibilityElementInfo* OH_ArkUI_AddAndGetAccessibilityElementInfo(
    ArkUI_AccessibilityElementInfoList* /*list*/) {
  return reinterpret_cast<ArkUI_AccessibilityElementInfo*>(&g_dummy_element_info);
}

ArkUI_AccessibilityElementInfo* OH_ArkUI_CreateAccessibilityElementInfo(void) {
  return reinterpret_cast<ArkUI_AccessibilityElementInfo*>(&g_dummy_element_info);
}

void OH_ArkUI_DestoryAccessibilityElementInfo(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/) {}

ArkUI_AccessibilityEventInfo* OH_ArkUI_CreateAccessibilityEventInfo(void) {
  return reinterpret_cast<ArkUI_AccessibilityEventInfo*>(&g_dummy_event_info);
}

void OH_ArkUI_DestoryAccessibilityEventInfo(
    ArkUI_AccessibilityEventInfo* /*eventInfo*/) {}

int32_t OH_ArkUI_AccessibilityEventSetEventType(
    ArkUI_AccessibilityEventInfo* /*eventInfo*/,
    ArkUI_AccessibilityEventType /*eventType*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityEventSetTextAnnouncedForAccessibility(
    ArkUI_AccessibilityEventInfo* /*eventInfo*/,
    const char* /*textAnnouncedForAccessibility*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityEventSetElementInfo(
    ArkUI_AccessibilityEventInfo* /*eventInfo*/,
    ArkUI_AccessibilityElementInfo* /*elementInfo*/) {
  return 0;
}

int32_t OH_ArkUI_FindAccessibilityActionArgumentByKey(
    ArkUI_AccessibilityActionArguments* /*arguments*/,
    const char* /*key*/,
    char** value) {
  if (value != nullptr) {
    *value = nullptr;
  }
  return 0;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetElementId(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/,
    int32_t /*elementId*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetParentId(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/,
    int32_t /*parentId*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetComponentType(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/,
    const char* /*componentType*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetContents(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/,
    const char* /*contents*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetAccessibilityText(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/,
    const char* /*accessibilityText*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetChildNodeIds(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/,
    int32_t /*childCount*/,
    int64_t* /*childNodeIds*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetOperationActions(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/,
    int32_t /*operationCount*/,
    ArkUI_AccessibleAction* /*operationActions*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetScreenRect(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/,
    ArkUI_AccessibleRect* /*screenRect*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetCheckable(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/,
    bool /*checkable*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetChecked(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/,
    bool /*checked*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetFocusable(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/,
    bool /*focusable*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetFocused(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/,
    bool /*isFocused*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetVisible(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/,
    bool /*isVisible*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetSelected(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/,
    bool /*selected*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetClickable(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/,
    bool /*clickable*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetLongClickable(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/,
    bool /*longClickable*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetEnabled(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/,
    bool /*isEnabled*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetIsPassword(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/,
    bool /*isPassword*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetScrollable(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/,
    bool /*scrollable*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetEditable(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/,
    bool /*editable*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetSelectedTextStart(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/,
    int32_t /*selectedTextStart*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetSelectedTextEnd(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/,
    int32_t /*selectedTextEnd*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetStartItemIndex(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/,
    int32_t /*startItemIndex*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetEndItemIndex(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/,
    int32_t /*endItemIndex*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetItemCount(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/,
    int32_t /*itemCount*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetAccessibilityOffset(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/,
    int32_t /*offset*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetAccessibilityGroup(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/,
    bool /*accessibilityGroup*/) {
  return 0;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetAccessibilityLevel(
    ArkUI_AccessibilityElementInfo* /*elementInfo*/,
    const char* /*accessibilityLevel*/) {
  return 0;
}

}  // extern "C"

extern "C" int32_t OH_NativeXComponent_GetTouchPointToolType(
    OH_NativeXComponent* component, uint32_t pointIndex,
    OH_NativeXComponent_TouchPointToolType* toolType) {
  if (toolType) {
    *toolType = static_cast<OH_NativeXComponent_TouchPointToolType>(0);
  }
  return OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
}

extern "C" int32_t OH_NativeXComponent_GetTouchPointTiltX(
    OH_NativeXComponent* component, uint32_t pointIndex, float* tiltX) {
  if (tiltX) {
    *tiltX = 0.0f;
  }
  return OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
}

extern "C" int32_t OH_NativeXComponent_GetTouchPointTiltY(
    OH_NativeXComponent* component, uint32_t pointIndex, float* tiltY) {
  if (tiltY) {
    *tiltY = 0.0f;
  }
  return OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
}

extern "C" int32_t OH_ArkUI_UIInputEvent_GetToolType(const ArkUI_UIInputEvent* event) {
  return 0;
}

extern "C" int64_t OH_ArkUI_UIInputEvent_GetEventTime(const ArkUI_UIInputEvent* event) {
  return 0;
}

extern "C" float OH_ArkUI_PointerEvent_GetX(const ArkUI_UIInputEvent* event) { return 0.0f; }
extern "C" float OH_ArkUI_PointerEvent_GetY(const ArkUI_UIInputEvent* event) { return 0.0f; }
extern "C" float OH_ArkUI_PointerEvent_GetWindowX(const ArkUI_UIInputEvent* event) { return 0.0f; }
extern "C" float OH_ArkUI_PointerEvent_GetWindowY(const ArkUI_UIInputEvent* event) { return 0.0f; }
extern "C" float OH_ArkUI_PointerEvent_GetDisplayX(const ArkUI_UIInputEvent* event) { return 0.0f; }
extern "C" float OH_ArkUI_PointerEvent_GetDisplayY(const ArkUI_UIInputEvent* event) { return 0.0f; }
extern "C" double OH_ArkUI_AxisEvent_GetVerticalAxisValue(const ArkUI_UIInputEvent* event) { return 0.0; }
extern "C" double OH_ArkUI_AxisEvent_GetHorizontalAxisValue(const ArkUI_UIInputEvent* event) { return 0.0; }
extern "C" double OH_ArkUI_AxisEvent_GetPinchAxisScaleValue(const ArkUI_UIInputEvent* event) { return 0.0; }
