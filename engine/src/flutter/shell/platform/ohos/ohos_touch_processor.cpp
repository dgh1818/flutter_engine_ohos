/*
 * Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd.
 * All rights reserved. Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE_KHZG file.
 */

#include "flutter/shell/platform/ohos/ohos_touch_processor.h"
#include <arkui/native_type.h>
#include <dlfcn.h>
#include "flutter/fml/trace_event.h"
#include "flutter/lib/ui/window/pointer_data_packet.h"
#include "flutter/shell/platform/ohos/ohos_shell_holder.h"

namespace flutter {

constexpr int MSEC_PER_SECOND = 1000;
constexpr int PER_POINTER_MEMBER = 10;
constexpr int CHANGES_POINTER_MEMBER = 10;
constexpr int TOUCH_EVENT_ADDITIONAL_ATTRIBUTES = 4;
constexpr int DEFAULT_SCALE_DEVICE_ID = -101;
constexpr int DEFAULT_SRCOLL_DEVICE_ID = -102;
constexpr int DEFAULT_PANZOOM_DEVICE_ID = -103;
constexpr double ZOOM_IN = 10.0 / 8.0;
constexpr double ZOOM_OUT = 1.0 / ZOOM_IN;

PointerData::Change OhosTouchProcessor::getPointerChangeForAction(
    int maskedAction) {
  switch (maskedAction) {
    case OH_NATIVEXCOMPONENT_DOWN:
      return PointerData::Change::kDown;
    case OH_NATIVEXCOMPONENT_UP:
      return PointerData::Change::kUp;
    case OH_NATIVEXCOMPONENT_CANCEL:
      return PointerData::Change::kCancel;
    case OH_NATIVEXCOMPONENT_MOVE:
      return PointerData::Change::kMove;
  }
  return PointerData::Change::kCancel;
}

PointerData::Change OhosTouchProcessor::getPointerChangeForMouseAction(
    OH_NativeXComponent_MouseEventAction mouseAction) {
  switch (mouseAction) {
    case OH_NATIVEXCOMPONENT_MOUSE_PRESS:
      return PointerData::Change::kDown;
    case OH_NATIVEXCOMPONENT_MOUSE_RELEASE:
      return PointerData::Change::kUp;
    case OH_NATIVEXCOMPONENT_MOUSE_MOVE:
      return PointerData::Change::kMove;
    default:
      return PointerData::Change::kCancel;
  }
}

PointerButtonMouse OhosTouchProcessor::getPointerButtonFromMouse(
    OH_NativeXComponent_MouseEventButton mouseButton) {
  switch (mouseButton) {
    case OH_NATIVEXCOMPONENT_LEFT_BUTTON:
      return kPointerButtonMousePrimary;
    case OH_NATIVEXCOMPONENT_RIGHT_BUTTON:
      return kPointerButtonMouseSecondary;
    case OH_NATIVEXCOMPONENT_MIDDLE_BUTTON:
      return kPointerButtonMouseMiddle;
    case OH_NATIVEXCOMPONENT_BACK_BUTTON:
      return kPointerButtonMouseBack;
    case OH_NATIVEXCOMPONENT_FORWARD_BUTTON:
      return kPointerButtonMouseForward;
    default:
      return kPointerButtonMousePrimary;
  }
}

PointerData::DeviceKind OhosTouchProcessor::getPointerDeviceTypeForToolType(
    int toolType) {
  switch (toolType) {
    case OH_NATIVEXCOMPONENT_TOOL_TYPE_FINGER:
      return PointerData::DeviceKind::kTouch;
    case OH_NATIVEXCOMPONENT_TOOL_TYPE_PEN:
      return PointerData::DeviceKind::kStylus;
    case OH_NATIVEXCOMPONENT_TOOL_TYPE_RUBBER:
      return PointerData::DeviceKind::kInvertedStylus;
    case OH_NATIVEXCOMPONENT_TOOL_TYPE_BRUSH:
      return PointerData::DeviceKind::kStylus;
    case OH_NATIVEXCOMPONENT_TOOL_TYPE_PENCIL:
      return PointerData::DeviceKind::kStylus;
    case OH_NATIVEXCOMPONENT_TOOL_TYPE_AIRBRUSH:
      return PointerData::DeviceKind::kStylus;
    case OH_NATIVEXCOMPONENT_TOOL_TYPE_MOUSE:
      return PointerData::DeviceKind::kMouse;
    case OH_NATIVEXCOMPONENT_TOOL_TYPE_LENS:
      return PointerData::DeviceKind::kTouch;
    case OH_NATIVEXCOMPONENT_TOOL_TYPE_UNKNOWN:
      return PointerData::DeviceKind::kTouch;
  }
  return PointerData::DeviceKind::kTouch;
}

std::shared_ptr<std::string[]> OhosTouchProcessor::packagePacketData(
    std::unique_ptr<OhosTouchProcessor::TouchPacket> touchPacket) {
  if (touchPacket == nullptr) {
    return nullptr;
  }
  int numPoints = touchPacket->touchEventInput->numPoints;
  int offset = 0;
  int size = CHANGES_POINTER_MEMBER + PER_POINTER_MEMBER * numPoints +
             TOUCH_EVENT_ADDITIONAL_ATTRIBUTES;
  std::shared_ptr<std::string[]> package(new std::string[size]);

  package[offset++] = std::to_string(touchPacket->touchEventInput->numPoints);

  package[offset++] = std::to_string(touchPacket->touchEventInput->id);
  package[offset++] = std::to_string(touchPacket->touchEventInput->screenX);
  package[offset++] = std::to_string(touchPacket->touchEventInput->screenY);
  package[offset++] = std::to_string(touchPacket->touchEventInput->x);
  package[offset++] = std::to_string(touchPacket->touchEventInput->y);
  package[offset++] = std::to_string(touchPacket->touchEventInput->type);
  package[offset++] = std::to_string(touchPacket->touchEventInput->size);
  package[offset++] = std::to_string(touchPacket->touchEventInput->force);
  package[offset++] = std::to_string(touchPacket->touchEventInput->deviceId);
  package[offset++] = std::to_string(touchPacket->touchEventInput->timeStamp);
  for (int i = 0; i < numPoints; i++) {
    package[offset++] =
        std::to_string(touchPacket->touchEventInput->touchPoints[i].id);
    package[offset++] =
        std::to_string(touchPacket->touchEventInput->touchPoints[i].screenX);
    package[offset++] =
        std::to_string(touchPacket->touchEventInput->touchPoints[i].screenY);
    package[offset++] =
        std::to_string(touchPacket->touchEventInput->touchPoints[i].x);
    package[offset++] =
        std::to_string(touchPacket->touchEventInput->touchPoints[i].y);
    package[offset++] =
        std::to_string(touchPacket->touchEventInput->touchPoints[i].type);
    package[offset++] =
        std::to_string(touchPacket->touchEventInput->touchPoints[i].size);
    package[offset++] =
        std::to_string(touchPacket->touchEventInput->touchPoints[i].force);
    package[offset++] =
        std::to_string(touchPacket->touchEventInput->touchPoints[i].timeStamp);
    package[offset++] =
        std::to_string(touchPacket->touchEventInput->touchPoints[i].isPressed);
  }
  package[offset++] = std::to_string(touchPacket->toolTypeInput);
  package[offset++] = std::to_string(touchPacket->tiltX);
  package[offset++] = std::to_string(touchPacket->tiltY);
  return package;
}

void OhosTouchProcessor::HandleTouchEvent(
    int64_t shell_holderID,
    OH_NativeXComponent* component,
    OH_NativeXComponent_TouchEvent* touchEvent) {
  if (touchEvent == nullptr) {
    return;
  }
  FML_TRACE_EVENT("flutter", "HandleTouchEvent", "timeStamp",
                  touchEvent->timeStamp);
  const int numTouchPoints = 1;
  std::unique_ptr<flutter::PointerDataPacket> packet =
      std::make_unique<flutter::PointerDataPacket>(numTouchPoints);
  PointerData pointerData;
  pointerData.Clear();
  pointerData.embedder_id = touchEvent->id;
  pointerData.time_stamp = touchEvent->timeStamp / MSEC_PER_SECOND;
  pointerData.change = getPointerChangeForAction(touchEvent->type);
  pointerData.physical_y = touchEvent->y;
  pointerData.physical_x = touchEvent->x;
  // Delta will be generated in pointer_data_packet_converter.cc.
  pointerData.physical_delta_x = 0.0;
  pointerData.physical_delta_y = 0.0;
  pointerData.device = touchEvent->id;
  // Pointer identifier will be generated in pointer_data_packet_converter.cc.
  pointerData.pointer_identifier = 0;
  // XComponent not support Scroll
  pointerData.signal_kind = PointerData::SignalKind::kNone;
  pointerData.scroll_delta_x = 0.0;
  pointerData.scroll_delta_y = 0.0;
  pointerData.pressure = touchEvent->force;
  pointerData.pressure_max = 1.0;
  pointerData.pressure_min = 0.0;
  OH_NativeXComponent_TouchPointToolType toolType;
  OH_NativeXComponent_GetTouchPointToolType(component, 0, &toolType);
  pointerData.kind = getPointerDeviceTypeForToolType(toolType);
  // 适配PC兼容模式，kind的值可能不是kTouch，需要确保 pointerData.buttons 被赋值
  if (pointerData.change == PointerData::Change::kDown ||
      pointerData.change == PointerData::Change::kMove) {
    pointerData.buttons = kPointerButtonTouchContact;
  }
  pointerData.pan_x = 0.0;
  pointerData.pan_y = 0.0;
  // Delta will be generated in pointer_data_packet_converter.cc.
  pointerData.pan_delta_x = 0.0;
  pointerData.pan_delta_y = 0.0;
  // The contact area between the fingerpad and the screen
  pointerData.size = touchEvent->size;
  pointerData.scale = 1.0;
  pointerData.rotation = 0.0;
  packet->SetPointerData(0, pointerData);
  auto ohos_shell_holder = reinterpret_cast<OHOSShellHolder*>(shell_holderID);
  ohos_shell_holder->GetPlatformView()->DispatchPointerDataPacket(
      std::move(packet));

  // For DFX
  fml::closure task = [timeStampDFX = touchEvent->timeStamp](void) {
    FML_TRACE_EVENT("flutter", "HandleTouchEventUI", "timeStamp", timeStampDFX);
  };
  ohos_shell_holder->GetPlatformView()->RunTask(OhosThreadType::kUI, task);
  PlatformViewOnTouchEvent(shell_holderID, toolType, component, touchEvent);
}

OhosTouchProcessor::OhosTouchProcessor()
    : apiVersion_(0),
      loader_(std::make_unique<DynamicLibraryLoader>(UI_INPUT_EVENT_LIB_NAME)),
      dynamicGetDeviceId_(nullptr),
      dynamicGetAxisAction_(nullptr),
      dynamicGetModifierKeyStates_(nullptr) {
  apiVersion_ = DynamicLibraryLoader::GetApiVersion();
  FML_LOG(INFO) << "Current SDK API Version: " << apiVersion_;

  std::vector<SymbolInfo> symbols = {
      {"OH_ArkUI_UIInputEvent_GetDeviceId",
       reinterpret_cast<void**>(&dynamicGetDeviceId_), 14},
      {"OH_ArkUI_AxisEvent_GetAxisAction",
       reinterpret_cast<void**>(&dynamicGetAxisAction_), 15},
      {"OH_ArkUI_UIInputEvent_GetModifierKeyStates",
       reinterpret_cast<void**>(&dynamicGetModifierKeyStates_), 17},
  };

  loader_->LoadSymbols(symbols);
}

OhosTouchProcessor::~OhosTouchProcessor() {}

// 处理轴事件：触控板中的捏合缩放和滚动抛滑手势，鼠标中的滚轮滑动和Ctrl+滚轮缩放
void OhosTouchProcessor::HandleAxisEvent(int64_t shell_holderID,
                                         OH_NativeXComponent* component,
                                         ArkUI_UIInputEvent* event) {
  if (event == nullptr) {
    return;
  }

  if (apiVersion_ < 15) {
    // API15 前轴事件接口不完善，会走 XComponentBase::OnDispatchMouseWheelEvent
    // 处理滚动
    return;
  }

  // 获取工具类型
  int32_t toolType = OH_ArkUI_UIInputEvent_GetToolType(event);
  if (toolType == UI_INPUT_EVENT_TOOL_TYPE_MOUSE) {
    // 鼠标滚轮事件
    uint64_t keys = 0;
    int32_t errorCode = dynamicGetModifierKeyStates_ != nullptr
                            ? dynamicGetModifierKeyStates_(event, &keys)
                            : 0;
    if (errorCode != ARKUI_ERROR_CODE_PARAM_INVALID &&
        keys & ARKUI_MODIFIER_KEY_CTRL) {
      // Ctrl+鼠标滚轮
      HandleScaleEvent(shell_holderID, component, event);
    } else {
      // 鼠标滚轮
      HandleScrollEvent(shell_holderID, component, event);
    }
  } else {
    // 捏合缩放和滚动抛滑
    HandlePanZooomEvent(shell_holderID, component, event);
  }
  return;
}

// 处理Ctrl+鼠标滚轮缩放
void OhosTouchProcessor::HandleScaleEvent(int64_t shell_holderID,
                                          OH_NativeXComponent* component,
                                          ArkUI_UIInputEvent* event) {
  if (event == nullptr) {
    return;
  }

  const int numTouchPoints = 1;
  std::unique_ptr<flutter::PointerDataPacket> packet =
      std::make_unique<flutter::PointerDataPacket>(numTouchPoints);
  PointerData pointerData;
  pointerData.Clear();

  // 获取 PointerData 状态类型并处理缩放累计值
  int32_t axisAction = dynamicGetAxisAction_ != nullptr
                           ? dynamicGetAxisAction_(event)
                           : UI_TOUCH_EVENT_ACTION_CANCEL;
  switch (axisAction) {
    case UI_TOUCH_EVENT_ACTION_CANCEL:
      pointerData.change = PointerData::Change::kCancel;
      break;
    case UI_TOUCH_EVENT_ACTION_DOWN:
      pointerData.change = PointerData::Change::kPanZoomStart;
      // 重置累计值
      accumulatedScale_ = 1.0;
      break;
    case UI_TOUCH_EVENT_ACTION_MOVE:
      pointerData.change = PointerData::Change::kPanZoomUpdate;
      // 更新累计值
      accumulatedScale_ *= OH_ArkUI_AxisEvent_GetVerticalAxisValue(event) < 0
                               ? ZOOM_IN
                               : ZOOM_OUT;
      break;
    case UI_TOUCH_EVENT_ACTION_UP:
      pointerData.change = PointerData::Change::kPanZoomEnd;
      break;
    default:
      FML_LOG(ERROR) << "HandleScaleEvent: AxisAction is not defined";
      pointerData.change = PointerData::Change::kCancel;
      break;
  }
  pointerData.scale = accumulatedScale_;

  pointerData.physical_x = OH_ArkUI_PointerEvent_GetX(event);
  pointerData.physical_y = OH_ArkUI_PointerEvent_GetY(event);
  pointerData.time_stamp =
      OH_ArkUI_UIInputEvent_GetEventTime(event) / MSEC_PER_SECOND;
  pointerData.device = dynamicGetDeviceId_ != nullptr
                           ? dynamicGetDeviceId_(event)
                           : DEFAULT_SCALE_DEVICE_ID;
  if (pointerData.device == -1) {
    // 如果 deviceId 为 -1，则设置为默认值
    pointerData.device = DEFAULT_SCALE_DEVICE_ID;
  }
  pointerData.kind = PointerData::DeviceKind::kTrackpad;

  packet->SetPointerData(0, pointerData);
  auto ohos_shell_holder = reinterpret_cast<OHOSShellHolder*>(shell_holderID);
  ohos_shell_holder->GetPlatformView()->DispatchPointerDataPacket(
      std::move(packet));
  return;
}

// 处理鼠标滚轮滚动
void OhosTouchProcessor::HandleScrollEvent(int64_t shell_holderID,
                                           OH_NativeXComponent* component,
                                           ArkUI_UIInputEvent* event) {
  const int numTouchPoints = 1;
  std::unique_ptr<flutter::PointerDataPacket> packet =
      std::make_unique<flutter::PointerDataPacket>(numTouchPoints);
  PointerData pointerData;
  pointerData.Clear();

  // 处理滚动值
  pointerData.scroll_delta_x = OH_ArkUI_AxisEvent_GetHorizontalAxisValue(event);
  pointerData.scroll_delta_y = OH_ArkUI_AxisEvent_GetVerticalAxisValue(event);

  pointerData.physical_x = OH_ArkUI_PointerEvent_GetX(event);
  pointerData.physical_y = OH_ArkUI_PointerEvent_GetY(event);
  pointerData.time_stamp =
      OH_ArkUI_UIInputEvent_GetEventTime(event) / MSEC_PER_SECOND;
  pointerData.device = dynamicGetDeviceId_ != nullptr
                           ? dynamicGetDeviceId_(event)
                           : DEFAULT_SRCOLL_DEVICE_ID;
  if (pointerData.device == -1) {
    // 如果 deviceId 为 -1，则设置为默认值
    pointerData.device = DEFAULT_SRCOLL_DEVICE_ID;
  }
  pointerData.kind = PointerData::DeviceKind::kMouse;
  pointerData.change = PointerData::Change::kHover;
  pointerData.signal_kind = PointerData::SignalKind::kScroll;

  packet->SetPointerData(0, pointerData);
  auto ohos_shell_holder = reinterpret_cast<OHOSShellHolder*>(shell_holderID);
  ohos_shell_holder->GetPlatformView()->DispatchPointerDataPacket(
      std::move(packet));
  return;
}

// 处理触控板双指捏合缩放和双指滚动抛滑
void OhosTouchProcessor::HandlePanZooomEvent(int64_t shell_holderID,
                                             OH_NativeXComponent* component,
                                             ArkUI_UIInputEvent* event) {
  const int numTouchPoints = 1;
  std::unique_ptr<flutter::PointerDataPacket> packet =
      std::make_unique<flutter::PointerDataPacket>(numTouchPoints);
  PointerData pointerData;
  pointerData.Clear();

  // 获取 PointerData 状态类型并处理滑动累计值
  int32_t axisAction = dynamicGetAxisAction_ != nullptr
                           ? dynamicGetAxisAction_(event)
                           : UI_TOUCH_EVENT_ACTION_CANCEL;
  switch (axisAction) {
    case UI_TOUCH_EVENT_ACTION_CANCEL:
      pointerData.change = PointerData::Change::kCancel;
      break;
    case UI_TOUCH_EVENT_ACTION_DOWN:
      pointerData.change = PointerData::Change::kPanZoomStart;
      // 重置累计值
      accumulatedPanX_ = 0.0;
      accumulatedPanY_ = 0.0;
      break;
    case UI_TOUCH_EVENT_ACTION_MOVE:
      pointerData.change = PointerData::Change::kPanZoomUpdate;
      // 更新累计值
      accumulatedPanX_ += 0 - OH_ArkUI_AxisEvent_GetHorizontalAxisValue(event);
      accumulatedPanY_ += 0 - OH_ArkUI_AxisEvent_GetVerticalAxisValue(event);
      break;
    case UI_TOUCH_EVENT_ACTION_UP:
      pointerData.change = PointerData::Change::kPanZoomEnd;
      break;
    default:
      FML_LOG(ERROR) << "HandlePanZooomEvent: AxisAction is not defined";
      pointerData.change = PointerData::Change::kCancel;
      break;
  }
  pointerData.pan_x = accumulatedPanX_;
  pointerData.pan_y = accumulatedPanY_;

  // 处理缩放值
  pointerData.scale = OH_ArkUI_AxisEvent_GetPinchAxisScaleValue(event);
  if (pointerData.scale == 0) {
    // 如果 scale 为 0，则设置为默认值 1.0
    pointerData.scale = 1.0;
  }

  pointerData.physical_x = OH_ArkUI_PointerEvent_GetX(event);
  pointerData.physical_y = OH_ArkUI_PointerEvent_GetY(event);
  pointerData.time_stamp =
      OH_ArkUI_UIInputEvent_GetEventTime(event) / MSEC_PER_SECOND;
  pointerData.device = dynamicGetDeviceId_ != nullptr
                           ? dynamicGetDeviceId_(event)
                           : DEFAULT_PANZOOM_DEVICE_ID;
  if (pointerData.device == -1) {
    // 如果 deviceId 为 -1，则设置为默认值
    pointerData.device = DEFAULT_PANZOOM_DEVICE_ID;
  }
  pointerData.kind = PointerData::DeviceKind::kTrackpad;

  packet->SetPointerData(0, pointerData);
  auto ohos_shell_holder = reinterpret_cast<OHOSShellHolder*>(shell_holderID);
  ohos_shell_holder->GetPlatformView()->DispatchPointerDataPacket(
      std::move(packet));
  return;
}

void OhosTouchProcessor::PlatformViewOnTouchEvent(
    int64_t shellHolderID,
    OH_NativeXComponent_TouchPointToolType toolType,
    OH_NativeXComponent* component,
    OH_NativeXComponent_TouchEvent* touchEvent) {
  int numPoints = touchEvent->numPoints;
  float tiltX = 0.0;
  float tiltY = 0.0;
  OH_NativeXComponent_GetTouchPointTiltX(component, 0, &tiltX);
  OH_NativeXComponent_GetTouchPointTiltY(component, 0, &tiltY);
  std::unique_ptr<OhosTouchProcessor::TouchPacket> touchPacket =
      std::make_unique<OhosTouchProcessor::TouchPacket>();
  touchPacket->touchEventInput = touchEvent;
  touchPacket->toolTypeInput = toolType;
  touchPacket->tiltX = tiltX;
  touchPacket->tiltX = tiltY;

  std::shared_ptr<std::string[]> touchPacketString =
      packagePacketData(std::move(touchPacket));
  int size = CHANGES_POINTER_MEMBER + PER_POINTER_MEMBER * numPoints +
             TOUCH_EVENT_ADDITIONAL_ATTRIBUTES;
  auto ohos_shell_holder = reinterpret_cast<OHOSShellHolder*>(shellHolderID);
  ohos_shell_holder->GetPlatformView()->OnTouchEvent(touchPacketString, size);
}

void OhosTouchProcessor::HandleMouseEvent(
    int64_t shell_holderID,
    OH_NativeXComponent* component,
    OH_NativeXComponent_MouseEvent mouseEvent,
    double offsetY,
    bool isLeave) {
  const int numTouchPoints = 1;
  std::unique_ptr<flutter::PointerDataPacket> packet =
      std::make_unique<flutter::PointerDataPacket>(numTouchPoints);
  PointerData pointerData;
  pointerData.Clear();
  pointerData.embedder_id = mouseEvent.button;
  pointerData.time_stamp = mouseEvent.timestamp / MSEC_PER_SECOND;
  pointerData.change = getPointerChangeForMouseAction(mouseEvent.action);
  // If this is a leave event, dispath a point event that leaves the area.
  pointerData.physical_y = isLeave ? -1 : mouseEvent.y;
  pointerData.physical_x = isLeave ? -1 : mouseEvent.x;
  // Delta will be generated in pointer_data_packet_converter.cc.
  pointerData.physical_delta_x = 0.0;
  pointerData.physical_delta_y = 0.0;
  pointerData.device = mouseEvent.button;
  // Pointer identifier will be generated in pointer_data_packet_converter.cc.
  pointerData.pointer_identifier = 0;
  // XComponent not support Scroll
  // now it's support
  pointerData.signal_kind = offsetY != 0 ? PointerData::SignalKind::kScroll
                                         : PointerData::SignalKind::kNone;
  pointerData.scroll_delta_x = 0.0;
  pointerData.scroll_delta_y = offsetY;
  pointerData.pressure = 0.0;
  pointerData.pressure_max = 1.0;
  pointerData.pressure_min = 0.0;
  pointerData.kind = PointerData::DeviceKind::kMouse;  // kMouse支持鼠标框选文字
  pointerData.buttons = getPointerButtonFromMouse(mouseEvent.button);
  // hover support
  if (mouseEvent.button == OH_NATIVEXCOMPONENT_NONE_BUTTON &&
      pointerData.change == PointerData::Change::kMove) {
    pointerData.change = PointerData::Change::kHover;
    pointerData.kind = PointerData::DeviceKind::kMouse;
    pointerData.buttons = 0;
  }
  pointerData.pan_x = 0.0;
  pointerData.pan_y = 0.0;
  // Delta will be generated in pointer_data_packet_converter.cc.
  pointerData.pan_delta_x = 0.0;
  pointerData.pan_delta_y = 0.0;
  // The contact area between the fingerpad and the screen
  pointerData.size = 0.0;
  pointerData.scale = 1.0;
  pointerData.rotation = 0.0;
  packet->SetPointerData(0, pointerData);
  auto ohos_shell_holder = reinterpret_cast<OHOSShellHolder*>(shell_holderID);
  ohos_shell_holder->GetPlatformView()->DispatchPointerDataPacket(
      std::move(packet));
  return;
}

void OhosTouchProcessor::HandleVirtualTouchEvent(
    int64_t shell_holderID,
    OH_NativeXComponent* component,
    OH_NativeXComponent_TouchEvent* touchEvent) {
  int numPoints = touchEvent->numPoints;
  float tiltX = 0.0;
  float tiltY = 0.0;
  auto ohos_shell_holder = reinterpret_cast<OHOSShellHolder*>(shell_holderID);
  OH_NativeXComponent_TouchPointToolType toolType;
  OH_NativeXComponent_GetTouchPointToolType(component, 0, &toolType);
  OH_NativeXComponent_GetTouchPointTiltX(component, 0, &tiltX);
  OH_NativeXComponent_GetTouchPointTiltY(component, 0, &tiltY);
  std::unique_ptr<OhosTouchProcessor::TouchPacket> touchPacket =
      std::make_unique<OhosTouchProcessor::TouchPacket>();
  touchPacket->touchEventInput = touchEvent;
  touchPacket->toolTypeInput = toolType;
  touchPacket->tiltX = tiltX;
  touchPacket->tiltX = tiltY;

  std::shared_ptr<std::string[]> touchPacketString =
      packagePacketData(std::move(touchPacket));
  int size = CHANGES_POINTER_MEMBER + PER_POINTER_MEMBER * numPoints +
             TOUCH_EVENT_ADDITIONAL_ATTRIBUTES;
  ohos_shell_holder->GetPlatformView()->OnTouchEvent(touchPacketString, size);
  return;
}
}  // namespace flutter