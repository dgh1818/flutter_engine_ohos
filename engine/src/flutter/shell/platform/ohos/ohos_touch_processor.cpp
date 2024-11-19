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

#include "flutter/shell/platform/ohos/ohos_touch_processor.h"
#include "flutter/fml/trace_event.h"
#include "flutter/lib/ui/window/pointer_data_packet.h"
#include "flutter/shell/platform/ohos/ohos_shell_holder.h"

namespace flutter {

constexpr int MSEC_PER_SECOND = 1000;
constexpr int PER_POINTER_MEMBER = 10;
constexpr int CHANGES_POINTER_MEMBER = 10;
constexpr int TOUCH_EVENT_ADDITIONAL_ATTRIBUTES = 4;

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
  FML_LOG(DEBUG) << "screenX:" << touchPacket->touchEventInput->screenX
                 << " screenY:" << touchPacket->touchEventInput->screenY
                 << " x:" << touchPacket->touchEventInput->x
                 << " y:" << touchPacket->touchEventInput->y;
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
    FML_LOG(DEBUG) << "touches [" << i << "] screenX:"
                   << touchPacket->touchEventInput->touchPoints[i].screenX
                   << " screenY:"
                   << touchPacket->touchEventInput->touchPoints[i].screenY
                   << " x:" << touchPacket->touchEventInput->touchPoints[i].x
                   << " y:" << touchPacket->touchEventInput->touchPoints[i].y;
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
  if (pointerData.kind == PointerData::DeviceKind::kTouch) {
    if (pointerData.change == PointerData::Change::kDown ||
        pointerData.change == PointerData::Change::kMove) {
      pointerData.buttons = kPointerButtonTouchContact;
    }
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
    double offsetY) {
  const int numTouchPoints = 1;
  std::unique_ptr<flutter::PointerDataPacket> packet =
      std::make_unique<flutter::PointerDataPacket>(numTouchPoints);
  PointerData pointerData;
  pointerData.Clear();
  pointerData.embedder_id = mouseEvent.button;
  pointerData.time_stamp = mouseEvent.timestamp / MSEC_PER_SECOND;
  pointerData.change = getPointerChangeForMouseAction(mouseEvent.action);
  pointerData.physical_y = mouseEvent.y;
  pointerData.physical_x = mouseEvent.x;
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
  pointerData.kind = PointerData::DeviceKind::kTouch;
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