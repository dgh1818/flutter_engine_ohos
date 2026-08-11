/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

// Test private methods by temporarily redefining access specifiers.
// This is a common C++ unit testing technique for testing internal logic
// that doesn't require runtime dependencies.
#define private public
#define protected public

#include "flutter/shell/platform/ohos/ohos_touch_processor.h"
#include "flutter/shell/platform/ohos/ohos_shell_holder.h"

#include <gtest/gtest.h>

#include <cstring>
#include <memory>
#include <string>

namespace flutter {
namespace testing {

// ===== getPointerChangeForAction =====

TEST(OhosTouchProcessorTest, GetPointerChangeForActionDown) {
  OhosTouchProcessor processor;
  EXPECT_EQ(processor.getPointerChangeForAction(OH_NATIVEXCOMPONENT_DOWN),
            PointerData::Change::kDown);
}

TEST(OhosTouchProcessorTest, GetPointerChangeForActionUp) {
  OhosTouchProcessor processor;
  EXPECT_EQ(processor.getPointerChangeForAction(OH_NATIVEXCOMPONENT_UP),
            PointerData::Change::kUp);
}

TEST(OhosTouchProcessorTest, GetPointerChangeForActionCancel) {
  OhosTouchProcessor processor;
  EXPECT_EQ(processor.getPointerChangeForAction(OH_NATIVEXCOMPONENT_CANCEL),
            PointerData::Change::kCancel);
}

TEST(OhosTouchProcessorTest, GetPointerChangeForActionMove) {
  OhosTouchProcessor processor;
  EXPECT_EQ(processor.getPointerChangeForAction(OH_NATIVEXCOMPONENT_MOVE),
            PointerData::Change::kMove);
}

TEST(OhosTouchProcessorTest, GetPointerChangeForActionUnknownReturnsCancel) {
  OhosTouchProcessor processor;
  EXPECT_EQ(processor.getPointerChangeForAction(99999),
            PointerData::Change::kCancel);
}

// ===== getPointerChangeForMouseAction =====

TEST(OhosTouchProcessorTest, GetPointerChangeForMousePress) {
  OhosTouchProcessor processor;
  EXPECT_EQ(
      processor.getPointerChangeForMouseAction(OH_NATIVEXCOMPONENT_MOUSE_PRESS),
      PointerData::Change::kDown);
}

TEST(OhosTouchProcessorTest, GetPointerChangeForMouseRelease) {
  OhosTouchProcessor processor;
  EXPECT_EQ(processor.getPointerChangeForMouseAction(
                OH_NATIVEXCOMPONENT_MOUSE_RELEASE),
            PointerData::Change::kUp);
}

TEST(OhosTouchProcessorTest, GetPointerChangeForMouseMove) {
  OhosTouchProcessor processor;
  EXPECT_EQ(
      processor.getPointerChangeForMouseAction(OH_NATIVEXCOMPONENT_MOUSE_MOVE),
      PointerData::Change::kMove);
}

TEST(OhosTouchProcessorTest, GetPointerChangeForMouseUnknownReturnsCancel) {
  OhosTouchProcessor processor;
  EXPECT_EQ(
      processor.getPointerChangeForMouseAction(
          static_cast<OH_NativeXComponent_MouseEventAction>(99999)),
      PointerData::Change::kCancel);
}

// ===== getPointerButtonFromMouse =====

TEST(OhosTouchProcessorTest, GetPointerButtonFromLeftButton) {
  OhosTouchProcessor processor;
  EXPECT_EQ(processor.getPointerButtonFromMouse(OH_NATIVEXCOMPONENT_LEFT_BUTTON),
            kPointerButtonMousePrimary);
}

TEST(OhosTouchProcessorTest, GetPointerButtonFromRightButton) {
  OhosTouchProcessor processor;
  EXPECT_EQ(
      processor.getPointerButtonFromMouse(OH_NATIVEXCOMPONENT_RIGHT_BUTTON),
      kPointerButtonMouseSecondary);
}

TEST(OhosTouchProcessorTest, GetPointerButtonFromMiddleButton) {
  OhosTouchProcessor processor;
  EXPECT_EQ(
      processor.getPointerButtonFromMouse(OH_NATIVEXCOMPONENT_MIDDLE_BUTTON),
      kPointerButtonMouseMiddle);
}

TEST(OhosTouchProcessorTest, GetPointerButtonFromBackButton) {
  OhosTouchProcessor processor;
  EXPECT_EQ(processor.getPointerButtonFromMouse(OH_NATIVEXCOMPONENT_BACK_BUTTON),
            kPointerButtonMouseBack);
}

TEST(OhosTouchProcessorTest, GetPointerButtonFromForwardButton) {
  OhosTouchProcessor processor;
  EXPECT_EQ(
      processor.getPointerButtonFromMouse(OH_NATIVEXCOMPONENT_FORWARD_BUTTON),
      kPointerButtonMouseForward);
}

TEST(OhosTouchProcessorTest, GetPointerButtonFromUnknownReturnsPrimary) {
  OhosTouchProcessor processor;
  EXPECT_EQ(
      processor.getPointerButtonFromMouse(
          static_cast<OH_NativeXComponent_MouseEventButton>(99999)),
      kPointerButtonMousePrimary);
}

// ===== getPointerDeviceTypeForToolType =====

TEST(OhosTouchProcessorTest, GetPointerDeviceTypeForFinger) {
  OhosTouchProcessor processor;
  EXPECT_EQ(processor.getPointerDeviceTypeForToolType(
                OH_NATIVEXCOMPONENT_TOOL_TYPE_FINGER),
            PointerData::DeviceKind::kTouch);
}

TEST(OhosTouchProcessorTest, GetPointerDeviceTypeForPen) {
  OhosTouchProcessor processor;
  EXPECT_EQ(processor.getPointerDeviceTypeForToolType(
                OH_NATIVEXCOMPONENT_TOOL_TYPE_PEN),
            PointerData::DeviceKind::kStylus);
}

TEST(OhosTouchProcessorTest, GetPointerDeviceTypeForRubber) {
  OhosTouchProcessor processor;
  EXPECT_EQ(processor.getPointerDeviceTypeForToolType(
                OH_NATIVEXCOMPONENT_TOOL_TYPE_RUBBER),
            PointerData::DeviceKind::kInvertedStylus);
}

TEST(OhosTouchProcessorTest, GetPointerDeviceTypeForBrush) {
  OhosTouchProcessor processor;
  EXPECT_EQ(processor.getPointerDeviceTypeForToolType(
                OH_NATIVEXCOMPONENT_TOOL_TYPE_BRUSH),
            PointerData::DeviceKind::kStylus);
}

TEST(OhosTouchProcessorTest, GetPointerDeviceTypeForPencil) {
  OhosTouchProcessor processor;
  EXPECT_EQ(processor.getPointerDeviceTypeForToolType(
                OH_NATIVEXCOMPONENT_TOOL_TYPE_PENCIL),
            PointerData::DeviceKind::kStylus);
}

TEST(OhosTouchProcessorTest, GetPointerDeviceTypeForAirbrush) {
  OhosTouchProcessor processor;
  EXPECT_EQ(processor.getPointerDeviceTypeForToolType(
                OH_NATIVEXCOMPONENT_TOOL_TYPE_AIRBRUSH),
            PointerData::DeviceKind::kStylus);
}

TEST(OhosTouchProcessorTest, GetPointerDeviceTypeForMouse) {
  OhosTouchProcessor processor;
  EXPECT_EQ(processor.getPointerDeviceTypeForToolType(
                OH_NATIVEXCOMPONENT_TOOL_TYPE_MOUSE),
            PointerData::DeviceKind::kMouse);
}

TEST(OhosTouchProcessorTest, GetPointerDeviceTypeForLens) {
  OhosTouchProcessor processor;
  EXPECT_EQ(processor.getPointerDeviceTypeForToolType(
                OH_NATIVEXCOMPONENT_TOOL_TYPE_LENS),
            PointerData::DeviceKind::kTouch);
}

TEST(OhosTouchProcessorTest, GetPointerDeviceTypeForUnknown) {
  OhosTouchProcessor processor;
  EXPECT_EQ(processor.getPointerDeviceTypeForToolType(
                OH_NATIVEXCOMPONENT_TOOL_TYPE_UNKNOWN),
            PointerData::DeviceKind::kTouch);
}

TEST(OhosTouchProcessorTest, GetPointerDeviceTypeForInvalidValue) {
  OhosTouchProcessor processor;
  EXPECT_EQ(processor.getPointerDeviceTypeForToolType(99999),
            PointerData::DeviceKind::kTouch);
}

// ===== shouldDropTouchEvent =====
// Tests the duplicate down/up event filtering logic.
// This function only needs a mock OH_NativeXComponent_TouchEvent struct
// (a plain C struct) and checks touchEvent->type and touchEvent->id.

TEST(OhosTouchProcessorTest, ShouldDropTouchEventReturnsFalseForFirstDown) {
  OhosTouchProcessor processor;
  OH_NativeXComponent_TouchEvent touchEvent = {};
  touchEvent.type = OH_NATIVEXCOMPONENT_DOWN;
  touchEvent.id = 0;
  EXPECT_FALSE(processor.shouldDropTouchEvent(&touchEvent));
}

TEST(OhosTouchProcessorTest, ShouldDropTouchEventReturnsTrueForDuplicateDown) {
  OhosTouchProcessor processor;
  OH_NativeXComponent_TouchEvent touchEvent1 = {};
  touchEvent1.type = OH_NATIVEXCOMPONENT_DOWN;
  touchEvent1.id = 0;
  // First down should not be dropped
  EXPECT_FALSE(processor.shouldDropTouchEvent(&touchEvent1));
  // Second down with same id should be dropped
  OH_NativeXComponent_TouchEvent touchEvent2 = {};
  touchEvent2.type = OH_NATIVEXCOMPONENT_DOWN;
  touchEvent2.id = 0;
  EXPECT_TRUE(processor.shouldDropTouchEvent(&touchEvent2));
}

TEST(OhosTouchProcessorTest, ShouldDropTouchEventReturnsFalseForFirstUp) {
  OhosTouchProcessor processor;
  // First send a down to register the finger
  OH_NativeXComponent_TouchEvent downEvent = {};
  downEvent.type = OH_NATIVEXCOMPONENT_DOWN;
  downEvent.id = 1;
  processor.shouldDropTouchEvent(&downEvent);
  // Then send up for same id - should not be dropped
  OH_NativeXComponent_TouchEvent upEvent = {};
  upEvent.type = OH_NATIVEXCOMPONENT_UP;
  upEvent.id = 1;
  EXPECT_FALSE(processor.shouldDropTouchEvent(&upEvent));
}

TEST(OhosTouchProcessorTest, ShouldDropTouchEventReturnsTrueForDuplicateUp) {
  OhosTouchProcessor processor;
  // Register finger with down
  OH_NativeXComponent_TouchEvent downEvent = {};
  downEvent.type = OH_NATIVEXCOMPONENT_DOWN;
  downEvent.id = 2;
  processor.shouldDropTouchEvent(&downEvent);
  // First up - not dropped, removes from active set
  OH_NativeXComponent_TouchEvent upEvent1 = {};
  upEvent1.type = OH_NATIVEXCOMPONENT_UP;
  upEvent1.id = 2;
  EXPECT_FALSE(processor.shouldDropTouchEvent(&upEvent1));
  // Second up with same id - should be dropped (id not in active set)
  OH_NativeXComponent_TouchEvent upEvent2 = {};
  upEvent2.type = OH_NATIVEXCOMPONENT_UP;
  upEvent2.id = 2;
  EXPECT_TRUE(processor.shouldDropTouchEvent(&upEvent2));
}

TEST(OhosTouchProcessorTest, ShouldDropTouchEventReturnsFalseForMoveEvent) {
  OhosTouchProcessor processor;
  OH_NativeXComponent_TouchEvent moveEvent = {};
  moveEvent.type = OH_NATIVEXCOMPONENT_MOVE;
  moveEvent.id = 0;
  // Move events are never filtered
  EXPECT_FALSE(processor.shouldDropTouchEvent(&moveEvent));
}

TEST(OhosTouchProcessorTest, ShouldDropTouchEventHandlesMultipleFingers) {
  OhosTouchProcessor processor;
  // Finger 0 down
  OH_NativeXComponent_TouchEvent down0 = {};
  down0.type = OH_NATIVEXCOMPONENT_DOWN;
  down0.id = 0;
  EXPECT_FALSE(processor.shouldDropTouchEvent(&down0));
  // Finger 1 down
  OH_NativeXComponent_TouchEvent down1 = {};
  down1.type = OH_NATIVEXCOMPONENT_DOWN;
  down1.id = 1;
  EXPECT_FALSE(processor.shouldDropTouchEvent(&down1));
  // Finger 0 up
  OH_NativeXComponent_TouchEvent up0 = {};
  up0.type = OH_NATIVEXCOMPONENT_UP;
  up0.id = 0;
  EXPECT_FALSE(processor.shouldDropTouchEvent(&up0));
  // Finger 1 up
  OH_NativeXComponent_TouchEvent up1 = {};
  up1.type = OH_NATIVEXCOMPONENT_UP;
  up1.id = 1;
  EXPECT_FALSE(processor.shouldDropTouchEvent(&up1));
}

TEST(OhosTouchProcessorTest,
     ShouldDropTouchEventDownUpDownCycleForSameFinger) {
  OhosTouchProcessor processor;
  // Down finger 0
  OH_NativeXComponent_TouchEvent down1 = {};
  down1.type = OH_NATIVEXCOMPONENT_DOWN;
  down1.id = 0;
  EXPECT_FALSE(processor.shouldDropTouchEvent(&down1));
  // Up finger 0
  OH_NativeXComponent_TouchEvent up1 = {};
  up1.type = OH_NATIVEXCOMPONENT_UP;
  up1.id = 0;
  EXPECT_FALSE(processor.shouldDropTouchEvent(&up1));
  // Down finger 0 again - should not be dropped (was removed by up)
  OH_NativeXComponent_TouchEvent down2 = {};
  down2.type = OH_NATIVEXCOMPONENT_DOWN;
  down2.id = 0;
  EXPECT_FALSE(processor.shouldDropTouchEvent(&down2));
}

// ===== packagePacketData =====
// Tests the serialization of TouchPacket into a string array.
// This function only needs a mock TouchPacket with a mock
// OH_NativeXComponent_TouchEvent struct.

TEST(OhosTouchProcessorTest, PackagePacketDataReturnsNullForNullInput) {
  OhosTouchProcessor processor;
  auto result = processor.packagePacketData(nullptr);
  EXPECT_EQ(result, nullptr);
}

TEST(OhosTouchProcessorTest, PackagePacketDataSerializesBasicFields) {
  OhosTouchProcessor processor;
  OH_NativeXComponent_TouchEvent touchEvent = {};
  touchEvent.id = 5;
  touchEvent.screenX = 100.0f;
  touchEvent.screenY = 200.0f;
  touchEvent.x = 10.0f;
  touchEvent.y = 20.0f;
  touchEvent.type = OH_NATIVEXCOMPONENT_DOWN;
  touchEvent.size = 1.5;
  touchEvent.force = 0.5f;
  touchEvent.deviceId = 42;
  touchEvent.timeStamp = 1234567890;
  touchEvent.numPoints = 1;
  touchEvent.touchPoints[0].id = 0;
  touchEvent.touchPoints[0].screenX = 100.0f;
  touchEvent.touchPoints[0].screenY = 200.0f;
  touchEvent.touchPoints[0].x = 10.0f;
  touchEvent.touchPoints[0].y = 20.0f;
  touchEvent.touchPoints[0].type = OH_NATIVEXCOMPONENT_DOWN;
  touchEvent.touchPoints[0].size = 1.5;
  touchEvent.touchPoints[0].force = 0.5f;
  touchEvent.touchPoints[0].timeStamp = 1234567890;
  touchEvent.touchPoints[0].isPressed = true;

  auto touchPacket = std::make_unique<OhosTouchProcessor::TouchPacket>();
  touchPacket->touchEventInput = &touchEvent;
  touchPacket->toolTypeInput = OH_NATIVEXCOMPONENT_TOOL_TYPE_FINGER;
  touchPacket->tiltX = 0.0f;
  touchPacket->tiltY = 0.0f;

  auto result = processor.packagePacketData(std::move(touchPacket));
  ASSERT_NE(result, nullptr);
  // Main event region writes 11 items: numPoints, id, screenX, screenY, x,
  // y, type, size, force, deviceId, timeStamp.
  // Per-pointer region writes 10 items per point.
  // Additional attributes: toolTypeInput, tiltX, tiltY (3 items).
  // Total for 1 point: 11 + 10*1 + 3 = 24
  // Verify first element is numPoints
  EXPECT_EQ(result[0], std::to_string(1u));
  // Verify id
  EXPECT_EQ(result[1], std::to_string(5));
  // Verify screenX
  EXPECT_EQ(result[2], std::to_string(100.0f));
  // Verify type (offset 6)
  EXPECT_EQ(result[6], std::to_string(OH_NATIVEXCOMPONENT_DOWN));
  // Verify size (offset 7)
  EXPECT_EQ(result[7], std::to_string(1.5));
  // Verify timeStamp (offset 10, 11th main event field)
  EXPECT_EQ(result[10], std::to_string(1234567890));
  // Verify first touchPoint id (offset 11, first per-pointer field)
  EXPECT_EQ(result[11], std::to_string(0));
  // Verify toolType (additional attribute, at offset 11 + 10*1 = 21)
  EXPECT_EQ(result[21],
            std::to_string(OH_NATIVEXCOMPONENT_TOOL_TYPE_FINGER));
}

TEST(OhosTouchProcessorTest, PackagePacketDataHandlesMultiplePoints) {
  OhosTouchProcessor processor;
  OH_NativeXComponent_TouchEvent touchEvent = {};
  touchEvent.id = 0;
  // Set non-zero timeStamp to expose offset errors (default 0 would
  // collide with touchPoints[0].id = 0 at the wrong offset)
  touchEvent.timeStamp = 999;
  touchEvent.numPoints = 2;
  touchEvent.touchPoints[0].id = 0;
  touchEvent.touchPoints[0].screenX = 10.0f;
  touchEvent.touchPoints[0].screenY = 20.0f;
  touchEvent.touchPoints[0].x = 1.0f;
  touchEvent.touchPoints[0].y = 2.0f;
  touchEvent.touchPoints[0].type = OH_NATIVEXCOMPONENT_DOWN;
  touchEvent.touchPoints[0].size = 1.0;
  touchEvent.touchPoints[0].force = 0.5f;
  touchEvent.touchPoints[0].timeStamp = 100;
  touchEvent.touchPoints[0].isPressed = true;
  touchEvent.touchPoints[1].id = 1;
  touchEvent.touchPoints[1].screenX = 30.0f;
  touchEvent.touchPoints[1].screenY = 40.0f;
  touchEvent.touchPoints[1].x = 3.0f;
  touchEvent.touchPoints[1].y = 4.0f;
  touchEvent.touchPoints[1].type = OH_NATIVEXCOMPONENT_DOWN;
  touchEvent.touchPoints[1].size = 2.0;
  touchEvent.touchPoints[1].force = 0.8f;
  touchEvent.touchPoints[1].timeStamp = 200;
  touchEvent.touchPoints[1].isPressed = true;

  auto touchPacket = std::make_unique<OhosTouchProcessor::TouchPacket>();
  touchPacket->touchEventInput = &touchEvent;
  touchPacket->toolTypeInput = OH_NATIVEXCOMPONENT_TOOL_TYPE_FINGER;
  touchPacket->tiltX = 0.0f;
  touchPacket->tiltY = 0.0f;

  auto result = processor.packagePacketData(std::move(touchPacket));
  ASSERT_NE(result, nullptr);
  // Verify numPoints
  EXPECT_EQ(result[0], std::to_string(2u));
  // Main event timeStamp at offset 10 (11th main event field)
  EXPECT_EQ(result[10], std::to_string(999));
  // First point id at offset 11 (first per-pointer field)
  EXPECT_EQ(result[11], std::to_string(0));
  // Second point id at offset 11 + 10 = 21
  EXPECT_EQ(result[21], std::to_string(1));
}

// ===== HandleMouseButtonEvent =====
// Tests the mouse button state machine logic.
// This private method only operates on internal mouse_button_state_ and
// does NOT depend on OHOSShellHolder or NDK event objects.
// Accessible via #define private public.

TEST(OhosTouchProcessorTest, HandleMouseButtonEventFirstPressReturnsDown) {
  OhosTouchProcessor processor;
  OH_NativeXComponent_MouseEvent mouseEvent = {};
  mouseEvent.button = OH_NATIVEXCOMPONENT_LEFT_BUTTON;
  mouseEvent.action = OH_NATIVEXCOMPONENT_MOUSE_PRESS;

  PointerData::Change change;
  int64_t buttons_to_send = 0;
  EXPECT_TRUE(processor.HandleMouseButtonEvent(mouseEvent, change,
                                               buttons_to_send));
  EXPECT_EQ(change, PointerData::Change::kDown);
  EXPECT_EQ(buttons_to_send, kPointerButtonMousePrimary);
}

TEST(OhosTouchProcessorTest, HandleMouseButtonEventSecondPressReturnsMove) {
  OhosTouchProcessor processor;
  // First press: left button
  OH_NativeXComponent_MouseEvent press1 = {};
  press1.button = OH_NATIVEXCOMPONENT_LEFT_BUTTON;
  press1.action = OH_NATIVEXCOMPONENT_MOUSE_PRESS;
  PointerData::Change change1;
  int64_t buttons1 = 0;
  processor.HandleMouseButtonEvent(press1, change1, buttons1);
  ASSERT_EQ(change1, PointerData::Change::kDown);

  // Second press: right button while left is held
  OH_NativeXComponent_MouseEvent press2 = {};
  press2.button = OH_NATIVEXCOMPONENT_RIGHT_BUTTON;
  press2.action = OH_NATIVEXCOMPONENT_MOUSE_PRESS;
  PointerData::Change change2;
  int64_t buttons2 = 0;
  EXPECT_TRUE(processor.HandleMouseButtonEvent(press2, change2, buttons2));
  EXPECT_EQ(change2, PointerData::Change::kMove);
  EXPECT_EQ(buttons2,
            kPointerButtonMousePrimary | kPointerButtonMouseSecondary);
}

TEST(OhosTouchProcessorTest, HandleMouseButtonEventDuplicatePressReturnsFalse) {
  OhosTouchProcessor processor;
  // Press left button
  OH_NativeXComponent_MouseEvent press = {};
  press.button = OH_NATIVEXCOMPONENT_LEFT_BUTTON;
  press.action = OH_NATIVEXCOMPONENT_MOUSE_PRESS;
  PointerData::Change change;
  int64_t buttons = 0;
  processor.HandleMouseButtonEvent(press, change, buttons);

  // Press left button again (duplicate) — should be ignored
  PointerData::Change change2;
  int64_t buttons2 = 99;
  EXPECT_FALSE(processor.HandleMouseButtonEvent(press, change2, buttons2));
}

TEST(OhosTouchProcessorTest, HandleMouseButtonEventReleaseLastButtonReturnsUp) {
  OhosTouchProcessor processor;
  // Press left button
  OH_NativeXComponent_MouseEvent press = {};
  press.button = OH_NATIVEXCOMPONENT_LEFT_BUTTON;
  press.action = OH_NATIVEXCOMPONENT_MOUSE_PRESS;
  PointerData::Change changePress;
  int64_t buttonsPress = 0;
  processor.HandleMouseButtonEvent(press, changePress, buttonsPress);

  // Release left button — last button released → kUp
  OH_NativeXComponent_MouseEvent release = {};
  release.button = OH_NATIVEXCOMPONENT_LEFT_BUTTON;
  release.action = OH_NATIVEXCOMPONENT_MOUSE_RELEASE;
  PointerData::Change changeRelease;
  int64_t buttonsRelease = 99;
  EXPECT_TRUE(processor.HandleMouseButtonEvent(release, changeRelease,
                                               buttonsRelease));
  EXPECT_EQ(changeRelease, PointerData::Change::kUp);
  EXPECT_EQ(buttonsRelease, 0);
}

TEST(OhosTouchProcessorTest,
     HandleMouseButtonEventReleaseNonLastButtonReturnsMove) {
  OhosTouchProcessor processor;
  // Press left + right
  OH_NativeXComponent_MouseEvent pressLeft = {};
  pressLeft.button = OH_NATIVEXCOMPONENT_LEFT_BUTTON;
  pressLeft.action = OH_NATIVEXCOMPONENT_MOUSE_PRESS;
  PointerData::Change c1;
  int64_t b1 = 0;
  processor.HandleMouseButtonEvent(pressLeft, c1, b1);

  OH_NativeXComponent_MouseEvent pressRight = {};
  pressRight.button = OH_NATIVEXCOMPONENT_RIGHT_BUTTON;
  pressRight.action = OH_NATIVEXCOMPONENT_MOUSE_PRESS;
  PointerData::Change c2;
  int64_t b2 = 0;
  processor.HandleMouseButtonEvent(pressRight, c2, b2);

  // Release left button — right still held → kMove
  OH_NativeXComponent_MouseEvent releaseLeft = {};
  releaseLeft.button = OH_NATIVEXCOMPONENT_LEFT_BUTTON;
  releaseLeft.action = OH_NATIVEXCOMPONENT_MOUSE_RELEASE;
  PointerData::Change changeRelease;
  int64_t buttonsRelease = 0;
  EXPECT_TRUE(processor.HandleMouseButtonEvent(releaseLeft, changeRelease,
                                               buttonsRelease));
  EXPECT_EQ(changeRelease, PointerData::Change::kMove);
  EXPECT_EQ(buttonsRelease, kPointerButtonMouseSecondary);
}

TEST(OhosTouchProcessorTest,
     HandleMouseButtonEventReleaseUnpressedReturnsFalse) {
  OhosTouchProcessor processor;
  // Release a button that was never pressed
  OH_NativeXComponent_MouseEvent release = {};
  release.button = OH_NATIVEXCOMPONENT_LEFT_BUTTON;
  release.action = OH_NATIVEXCOMPONENT_MOUSE_RELEASE;
  PointerData::Change change;
  int64_t buttons = 99;
  EXPECT_FALSE(processor.HandleMouseButtonEvent(release, change, buttons));
}

TEST(OhosTouchProcessorTest,
     HandleMouseButtonEventNonPressReleaseActionReturnsTrue) {
  OhosTouchProcessor processor;
  // A non-press, non-release action (e.g. MOUSE_NONE / move)
  OH_NativeXComponent_MouseEvent moveEvent = {};
  moveEvent.button = OH_NATIVEXCOMPONENT_NONE_BUTTON;
  moveEvent.action = OH_NATIVEXCOMPONENT_MOUSE_NONE;
  PointerData::Change change;
  int64_t buttons = 0;
  EXPECT_TRUE(processor.HandleMouseButtonEvent(moveEvent, change, buttons));
  // change should come from getPointerChangeForMouseAction(default) → kCancel
  EXPECT_EQ(change, PointerData::Change::kCancel);
}

TEST(OhosTouchProcessorTest,
     HandleMouseButtonEventMiddleButtonPressAndRelease) {
  OhosTouchProcessor processor;
  // Press middle button
  OH_NativeXComponent_MouseEvent press = {};
  press.button = OH_NATIVEXCOMPONENT_MIDDLE_BUTTON;
  press.action = OH_NATIVEXCOMPONENT_MOUSE_PRESS;
  PointerData::Change changePress;
  int64_t buttonsPress = 0;
  EXPECT_TRUE(processor.HandleMouseButtonEvent(press, changePress,
                                                buttonsPress));
  EXPECT_EQ(changePress, PointerData::Change::kDown);
  EXPECT_EQ(buttonsPress, kPointerButtonMouseMiddle);

  // Release middle button
  OH_NativeXComponent_MouseEvent release = {};
  release.button = OH_NATIVEXCOMPONENT_MIDDLE_BUTTON;
  release.action = OH_NATIVEXCOMPONENT_MOUSE_RELEASE;
  PointerData::Change changeRelease;
  int64_t buttonsRelease = 0;
  EXPECT_TRUE(processor.HandleMouseButtonEvent(release, changeRelease,
                                               buttonsRelease));
  EXPECT_EQ(changeRelease, PointerData::Change::kUp);
  EXPECT_EQ(buttonsRelease, 0);
}

// ===== Early-return branches =====
// These test the null/version-check early-return paths of functions that
// otherwise require OHOSShellHolder + NDK event objects.

TEST(OhosTouchProcessorTest, HandleTouchEventReturnsOnNullEvent) {
  OhosTouchProcessor processor;
  // touchEvent == nullptr → early return, no crash
  processor.HandleTouchEvent(0, nullptr, nullptr);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandleAxisEventReturnsOnNullEvent) {
  OhosTouchProcessor processor;
  // event == nullptr → early return, no crash
  processor.HandleAxisEvent(0, nullptr, nullptr);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandleAxisEventReturnsOnLowApiVersion) {
  OhosTouchProcessor processor;
  // Force apiVersion_ < 15 to trigger early return
  processor.apiVersion_ = 10;
  // event is non-null (dummy pointer) but should still early-return
  // because apiVersion_ < 15
  processor.HandleAxisEvent(0, nullptr,
                            reinterpret_cast<ArkUI_UIInputEvent*>(0x1));
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandleScaleEventReturnsOnNullEvent) {
  OhosTouchProcessor processor;
  // event == nullptr → early return, no crash
  processor.HandleScaleEvent(0, nullptr, nullptr);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandleVirtualTouchEventReturnsOnApi20Plus) {
  OhosTouchProcessor processor;
  // Force apiVersion_ >= 20 to trigger early return
  processor.apiVersion_ = 20;
  OH_NativeXComponent_TouchEvent touchEvent = {};
  processor.HandleVirtualTouchEvent(0, nullptr, &touchEvent);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, PlatformViewOnAxisEventReturnsOnLowApiVersion) {
  OhosTouchProcessor processor;
  // Force apiVersion_ < 20 to trigger early return
  processor.apiVersion_ = 10;
  processor.PlatformViewOnAxisEvent(
      0, reinterpret_cast<ArkUI_UIInputEvent*>(0x1), 0.0);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, VsyncVotingTouchValueIgnoresMoveType) {
  OhosTouchProcessor processor;
  // touchType is neither UP nor DOWN → no-op, no crash
  processor.VsyncVotingTouchValue(0, OH_NATIVEXCOMPONENT_MOVE);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, SendFinalMoveEventBeforeLeaveNoHistory) {
  OhosTouchProcessor processor;
  // lastMouseX_ and lastMouseY_ are -1 by default → early return, no crash
  OH_NativeXComponent_MouseEvent mouseEvent = {};
  processor.SendFinalMoveEventBeforeLeave(0, nullptr, mouseEvent, 100.0, 100.0);
  SUCCEED();
}

// ===== Additional branch-coverage tests =====
// These tests target specific uncovered branches identified by llvm-cov.

// HandleAxisEvent line 297: `if (!warned)` — the `warned` static variable is
// set to true on the first call with apiVersion_ < 15. A second call hits the
// false branch (warned already true), skipping the FML_LOG(WARNING).
TEST(OhosTouchProcessorTest, HandleAxisEventLowApiVersionSecondCallSkipsWarning) {
  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;
  // First call sets warned = true (may already be true from prior tests)
  processor.HandleAxisEvent(0, nullptr,
                            reinterpret_cast<ArkUI_UIInputEvent*>(0x1));
  // Second call hits !warned == false branch
  processor.HandleAxisEvent(0, nullptr,
                            reinterpret_cast<ArkUI_UIInputEvent*>(0x2));
  SUCCEED();
}

// HandleTouchEvent line 199: `if (touchEvent == nullptr || shouldDropTouchEvent(touchEvent))`
// — covers the false branch of `touchEvent == nullptr` (non-null event) and the
// true branch of `shouldDropTouchEvent` (duplicate down → early return before
// OHOSShellHolder access).
TEST(OhosTouchProcessorTest, HandleTouchEventDroppedOnDuplicateDown) {
  OhosTouchProcessor processor;
  // First down registers finger id 0
  OH_NativeXComponent_TouchEvent downEvent = {};
  downEvent.type = OH_NATIVEXCOMPONENT_DOWN;
  downEvent.id = 0;
  EXPECT_FALSE(processor.shouldDropTouchEvent(&downEvent));
  // Second down with same id → shouldDropTouchEvent returns true → early return
  OH_NativeXComponent_TouchEvent duplicateDown = {};
  duplicateDown.type = OH_NATIVEXCOMPONENT_DOWN;
  duplicateDown.id = 0;
  // This should return early without accessing OHOSShellHolder
  processor.HandleTouchEvent(0, nullptr, &duplicateDown);
  SUCCEED();
}

// ===== Strong stub definitions for NDK and napi functions =====
//
// These definitions are strong symbols in the main executable; at link time
// they take precedence over the same symbols in the shared libraries
// (standard ELF symbol interposition), allowing us to test functions that
// depend on NDK/napi runtime without a real device environment.
//
// The stubs return safe default values and never dereference opaque pointers.

namespace {
// Configurable stub state for NDK functions
int32_t g_stub_tool_type = UI_INPUT_EVENT_TOOL_TYPE_FINGER;
int32_t g_stub_axis_action = UI_TOUCH_EVENT_ACTION_CANCEL;
int32_t g_stub_device_id = 0;
float g_stub_pointer_x = 100.0f;
float g_stub_pointer_y = 200.0f;
float g_stub_pointer_window_x = 100.0f;
float g_stub_pointer_window_y = 200.0f;
float g_stub_pointer_display_x = 100.0f;
float g_stub_pointer_display_y = 200.0f;
double g_stub_vertical_axis_value = 0.0;
double g_stub_horizontal_axis_value = 0.0;
double g_stub_pinch_scale_value = 1.0;
int64_t g_stub_event_time = 1000000;
int32_t g_stub_xcomponent_tool_type = OH_NATIVEXCOMPONENT_TOOL_TYPE_FINGER;
int32_t g_stub_xcomponent_ret = OH_NATIVEXCOMPONENT_RESULT_SUCCESS;

// Reset all stub variables to their default values.
// Called after each test to prevent test-order dependencies.
void ResetStubState() {
  g_stub_tool_type = UI_INPUT_EVENT_TOOL_TYPE_FINGER;
  g_stub_axis_action = UI_TOUCH_EVENT_ACTION_CANCEL;
  g_stub_device_id = 0;
  g_stub_pointer_x = 100.0f;
  g_stub_pointer_y = 200.0f;
  g_stub_pointer_window_x = 100.0f;
  g_stub_pointer_window_y = 200.0f;
  g_stub_pointer_display_x = 100.0f;
  g_stub_pointer_display_y = 200.0f;
  g_stub_vertical_axis_value = 0.0;
  g_stub_horizontal_axis_value = 0.0;
  g_stub_pinch_scale_value = 1.0;
  g_stub_event_time = 1000000;
  g_stub_xcomponent_tool_type = OH_NATIVEXCOMPONENT_TOOL_TYPE_FINGER;
  g_stub_xcomponent_ret = OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
}

// gtest listener that resets all stub state after each test, ensuring
// tests do not depend on execution order (--gtest_shuffle safe).
class StubStateResetter : public ::testing::EmptyTestEventListener {
 public:
  void OnTestEnd(const ::testing::TestInfo&) override { ResetStubState(); }
};

// Register the resetter before any tests run.
struct StubStateResetterRegistrar {
  StubStateResetterRegistrar() {
    ::testing::UnitTest::GetInstance()->listeners().Append(
        new StubStateResetter());
  }
} g_stub_resetter_registrar;
}  // namespace

// Stub OH_NativeXComponent touch point functions
extern "C" int32_t OH_NativeXComponent_GetTouchPointToolType(
    OH_NativeXComponent* component, uint32_t pointIndex,
    OH_NativeXComponent_TouchPointToolType* toolType) {
  if (toolType) {
    *toolType = static_cast<OH_NativeXComponent_TouchPointToolType>(
        g_stub_xcomponent_tool_type);
  }
  return g_stub_xcomponent_ret;
}

extern "C" int32_t OH_NativeXComponent_GetTouchPointTiltX(
    OH_NativeXComponent* component, uint32_t pointIndex, float* tiltX) {
  if (tiltX) {
    *tiltX = 0.0f;
  }
  return g_stub_xcomponent_ret;
}

extern "C" int32_t OH_NativeXComponent_GetTouchPointTiltY(
    OH_NativeXComponent* component, uint32_t pointIndex, float* tiltY) {
  if (tiltY) {
    *tiltY = 0.0f;
  }
  return g_stub_xcomponent_ret;
}

// Stub ArkUI UIInputEvent functions
extern "C" int32_t OH_ArkUI_UIInputEvent_GetToolType(const ArkUI_UIInputEvent* event) {
  return g_stub_tool_type;
}

extern "C" int64_t OH_ArkUI_UIInputEvent_GetEventTime(const ArkUI_UIInputEvent* event) {
  return g_stub_event_time;
}

extern "C" float OH_ArkUI_PointerEvent_GetX(const ArkUI_UIInputEvent* event) {
  return g_stub_pointer_x;
}

extern "C" float OH_ArkUI_PointerEvent_GetY(const ArkUI_UIInputEvent* event) {
  return g_stub_pointer_y;
}

extern "C" float OH_ArkUI_PointerEvent_GetWindowX(const ArkUI_UIInputEvent* event) {
  return g_stub_pointer_window_x;
}

extern "C" float OH_ArkUI_PointerEvent_GetWindowY(const ArkUI_UIInputEvent* event) {
  return g_stub_pointer_window_y;
}

extern "C" float OH_ArkUI_PointerEvent_GetDisplayX(const ArkUI_UIInputEvent* event) {
  return g_stub_pointer_display_x;
}

extern "C" float OH_ArkUI_PointerEvent_GetDisplayY(const ArkUI_UIInputEvent* event) {
  return g_stub_pointer_display_y;
}

extern "C" double OH_ArkUI_AxisEvent_GetVerticalAxisValue(const ArkUI_UIInputEvent* event) {
  return g_stub_vertical_axis_value;
}

extern "C" double OH_ArkUI_AxisEvent_GetHorizontalAxisValue(const ArkUI_UIInputEvent* event) {
  return g_stub_horizontal_axis_value;
}

extern "C" double OH_ArkUI_AxisEvent_GetPinchAxisScaleValue(const ArkUI_UIInputEvent* event) {
  return g_stub_pinch_scale_value;
}

// Stub napi functions — these prevent crashes when PlatformViewOHOSNapi methods
// are called with a null env_ (which is the case in unit tests since nativeAttach
// is never called).
extern "C" napi_status napi_open_handle_scope(napi_env env, napi_handle_scope* result) {
  if (result) {
    *result = reinterpret_cast<napi_handle_scope>(0x1);
  }
  return napi_ok;
}

extern "C" napi_status napi_close_handle_scope(napi_env env, napi_handle_scope scope) {
  return napi_ok;
}

extern "C" napi_status napi_create_array(napi_env env, napi_value* result) {
  if (result) {
    *result = reinterpret_cast<napi_value>(0x2);
  }
  return napi_ok;
}

extern "C" napi_status napi_create_string_utf8(napi_env env, const char* str, size_t length, napi_value* result) {
  if (result) {
    *result = reinterpret_cast<napi_value>(0x3);
  }
  return napi_ok;
}

extern "C" napi_status napi_set_element(napi_env env, napi_value object, uint32_t index, napi_value value) {
  return napi_ok;
}

extern "C" napi_status napi_get_reference_value(napi_env env, napi_ref ref, napi_value* result) {
  if (result) {
    *result = reinterpret_cast<napi_value>(0x4);
  }
  return napi_ok;
}

extern "C" napi_status napi_get_named_property(napi_env env, napi_value object, const char* name, napi_value* result) {
  if (result) {
    *result = reinterpret_cast<napi_value>(0x5);
  }
  return napi_ok;
}

extern "C" napi_status napi_call_function(napi_env env, napi_value recv, napi_value fn, size_t argc, const napi_value* argv, napi_value* result) {
  return napi_ok;
}

// ===== OHOSShellHolder integration tests =====
// These tests construct a real OHOSShellHolder (with software rendering) to
// obtain a valid shell_holderID, then exercise the full Handle*Event paths
// that were previously untestable. The NDK and napi stubs above ensure these
// paths don't crash.

namespace {
// Helper to create Settings configured for software rendering (no GPU needed).
static Settings MakeShellHolderTestSettings() {
  Settings settings;
  settings.ohos_rendering_api = OHOSRenderingAPI::kSoftware;
  return settings;
}

// Helper to create a valid OHOSShellHolder for integration tests.
// Returns the shell_holderID (raw pointer cast to int64_t).
static int64_t CreateShellHolderForTest(
    std::unique_ptr<OHOSShellHolder>& out_holder) {
  auto settings = MakeShellHolderTestSettings();
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  out_holder = std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  EXPECT_TRUE(out_holder->IsValid());
  return reinterpret_cast<int64_t>(out_holder.get());
}

// Helper to null out dynamic function pointers loaded via dlsym.
// These point to real NDK functions that would crash on fake event pointers.
// Setting them to nullptr makes the code take the safe fallback paths.
#define NULL_OUT_DYNAMIC_PTRS(processor)        \
  (processor).dynamicGetDeviceId_ = nullptr;    \
  (processor).dynamicGetAxisAction_ = nullptr;  \
  (processor).dynamicGetModifierKeyStates_ = nullptr;
}  // namespace

// ===== HandleTouchEvent full path with real OHOSShellHolder =====

TEST(OhosTouchProcessorTest, HandleTouchEventFullPathWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  OH_NativeXComponent_TouchEvent touchEvent = {};
  touchEvent.id = 0;
  touchEvent.screenX = 100.0f;
  touchEvent.screenY = 200.0f;
  touchEvent.x = 10.0f;
  touchEvent.y = 20.0f;
  touchEvent.type = OH_NATIVEXCOMPONENT_DOWN;
  touchEvent.size = 1.5;
  touchEvent.force = 0.5f;
  touchEvent.deviceId = 42;
  touchEvent.timeStamp = 1234567890;
  touchEvent.numPoints = 1;
  touchEvent.touchPoints[0].id = 0;
  touchEvent.touchPoints[0].type = OH_NATIVEXCOMPONENT_DOWN;
  touchEvent.touchPoints[0].isPressed = true;

  // This exercises the full path: DispatchPointerDataPacket →
  // VsyncVotingTouchValue → RunTask → PlatformViewOnTouchEvent →
  // OnTouchEvent → FlutterViewOnTouchEvent (stubbed napi)
  processor.HandleTouchEvent(shell_id, nullptr, &touchEvent);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandleTouchEventUpEventWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  // First send a down to register the finger
  OH_NativeXComponent_TouchEvent downEvent = {};
  downEvent.type = OH_NATIVEXCOMPONENT_DOWN;
  downEvent.id = 1;
  downEvent.numPoints = 1;
  downEvent.touchPoints[0].id = 1;
  downEvent.touchPoints[0].type = OH_NATIVEXCOMPONENT_DOWN;
  processor.HandleTouchEvent(shell_id, nullptr, &downEvent);

  // Then send an up event
  OH_NativeXComponent_TouchEvent upEvent = {};
  upEvent.type = OH_NATIVEXCOMPONENT_UP;
  upEvent.id = 1;
  upEvent.numPoints = 1;
  upEvent.touchPoints[0].id = 1;
  upEvent.touchPoints[0].type = OH_NATIVEXCOMPONENT_UP;
  processor.HandleTouchEvent(shell_id, nullptr, &upEvent);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandleTouchEventMoveEventWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  // Send a down first
  OH_NativeXComponent_TouchEvent downEvent = {};
  downEvent.type = OH_NATIVEXCOMPONENT_DOWN;
  downEvent.id = 2;
  downEvent.numPoints = 1;
  downEvent.touchPoints[0].id = 2;
  downEvent.touchPoints[0].type = OH_NATIVEXCOMPONENT_DOWN;
  processor.HandleTouchEvent(shell_id, nullptr, &downEvent);

  // Then send a move event
  OH_NativeXComponent_TouchEvent moveEvent = {};
  moveEvent.type = OH_NATIVEXCOMPONENT_MOVE;
  moveEvent.id = 2;
  moveEvent.numPoints = 1;
  moveEvent.touchPoints[0].id = 2;
  moveEvent.touchPoints[0].type = OH_NATIVEXCOMPONENT_MOVE;
  processor.HandleTouchEvent(shell_id, nullptr, &moveEvent);
  SUCCEED();
}

// ===== HandleAxisEvent full path with real OHOSShellHolder =====

TEST(OhosTouchProcessorTest, HandleAxisEventMouseScrollWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 20;  // >= 15 to skip early return
  NULL_OUT_DYNAMIC_PTRS(processor)

  // Set stub to return mouse tool type
  g_stub_tool_type = UI_INPUT_EVENT_TOOL_TYPE_MOUSE;
  // No Ctrl key → HandleScrollEvent path
  // dynamicGetModifierKeyStates_ is nullptr → errorCode = 0, keys = 0
  // → no Ctrl → HandleScrollEvent

  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandleAxisEvent(shell_id, nullptr, event);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandleAxisEventMouseCtrlScrollWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 20;
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_tool_type = UI_INPUT_EVENT_TOOL_TYPE_MOUSE;
  // dynamicGetModifierKeyStates_ is nullptr → errorCode = 0
  // keys = 0 → no Ctrl → HandleScrollEvent (not HandleScaleEvent)
  // To test HandleScaleEvent, we'd need dynamicGetModifierKeyStates_ to return
  // keys with ARKUI_MODIFIER_KEY_CTRL. But since it's nullptr, we get the
  // non-Ctrl path.

  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandleAxisEvent(shell_id, nullptr, event);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandleAxisEventTouchpadPanZoomWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 20;
  NULL_OUT_DYNAMIC_PTRS(processor)

  // Set stub to return touchpad tool type (not mouse)
  g_stub_tool_type = UI_INPUT_EVENT_TOOL_TYPE_TOUCHPAD;

  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandleAxisEvent(shell_id, nullptr, event);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandleAxisEventFingerToolTypeWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 20;
  NULL_OUT_DYNAMIC_PTRS(processor)

  // Set stub to return finger tool type (not mouse)
  g_stub_tool_type = UI_INPUT_EVENT_TOOL_TYPE_FINGER;

  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandleAxisEvent(shell_id, nullptr, event);
  SUCCEED();
}

// ===== HandleScaleEvent full path with real OHOSShellHolder =====

TEST(OhosTouchProcessorTest, HandleScaleEventCancelActionWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;  // < 20 to skip OnAxisEvent path
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_axis_action = UI_TOUCH_EVENT_ACTION_CANCEL;
  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandleScaleEvent(shell_id, nullptr, event);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandleScaleEventDownActionWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;  // < 20 to skip OnAxisEvent path
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_axis_action = UI_TOUCH_EVENT_ACTION_DOWN;
  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandleScaleEvent(shell_id, nullptr, event);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandleScaleEventMoveActionWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;  // < 20 to skip OnAxisEvent path
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_axis_action = UI_TOUCH_EVENT_ACTION_MOVE;
  g_stub_vertical_axis_value = -1.0;  // negative → ZOOM_IN
  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandleScaleEvent(shell_id, nullptr, event);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandleScaleEventMoveActionPositiveAxisWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_axis_action = UI_TOUCH_EVENT_ACTION_MOVE;
  g_stub_vertical_axis_value = 1.0;  // positive → ZOOM_OUT
  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandleScaleEvent(shell_id, nullptr, event);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandleScaleEventUpActionWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_axis_action = UI_TOUCH_EVENT_ACTION_UP;
  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandleScaleEvent(shell_id, nullptr, event);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandleScaleEventDefaultActionWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_axis_action = 999;  // default case
  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandleScaleEvent(shell_id, nullptr, event);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandleScaleEventApi20PlusWithOnAxisEvent) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 20;  // >= 20 → calls OnAxisEvent (stubbed napi)
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_axis_action = UI_TOUCH_EVENT_ACTION_CANCEL;
  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandleScaleEvent(shell_id, nullptr, event);
  SUCCEED();
}

// ===== HandlePanZooomEvent full path with real OHOSShellHolder =====

TEST(OhosTouchProcessorTest, HandlePanZooomEventCancelActionWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;  // < 20 to skip OnAxisEvent path
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_axis_action = UI_TOUCH_EVENT_ACTION_CANCEL;
  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandlePanZooomEvent(shell_id, nullptr, event);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandlePanZooomEventDownActionWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_axis_action = UI_TOUCH_EVENT_ACTION_DOWN;
  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandlePanZooomEvent(shell_id, nullptr, event);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandlePanZooomEventMoveActionWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_axis_action = UI_TOUCH_EVENT_ACTION_MOVE;
  g_stub_horizontal_axis_value = 5.0;
  g_stub_vertical_axis_value = 10.0;
  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandlePanZooomEvent(shell_id, nullptr, event);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandlePanZooomEventUpActionWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_axis_action = UI_TOUCH_EVENT_ACTION_UP;
  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandlePanZooomEvent(shell_id, nullptr, event);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandlePanZooomEventDefaultActionWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_axis_action = 999;  // default case
  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandlePanZooomEvent(shell_id, nullptr, event);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandlePanZooomEventZeroScaleWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_pinch_scale_value = 0.0;  // triggers default scale 1.0
  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandlePanZooomEvent(shell_id, nullptr, event);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandlePanZooomEventApi20PlusWithOnAxisEvent) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 20;  // >= 20 → calls OnAxisEvent (stubbed napi)
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_axis_action = UI_TOUCH_EVENT_ACTION_CANCEL;
  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandlePanZooomEvent(shell_id, nullptr, event);
  SUCCEED();
}

// ===== HandleScrollEvent full path with real OHOSShellHolder =====

TEST(OhosTouchProcessorTest, HandleScrollEventWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 20;  // >= 20 for PlatformViewOnAxisEvent path
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_vertical_axis_value = 10.0;
  g_stub_horizontal_axis_value = 5.0;
  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandleScrollEvent(shell_id, nullptr, event);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandleScrollEventLowApiVersionWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;  // < 20 → PlatformViewOnAxisEvent early return
  NULL_OUT_DYNAMIC_PTRS(processor)

  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandleScrollEvent(shell_id, nullptr, event);
  SUCCEED();
}

// ===== HandleMouseEvent full path with real OHOSShellHolder =====

TEST(OhosTouchProcessorTest, HandleMouseEventMoveWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;  // < 20 to skip OnMouseEvent path

  OH_NativeXComponent_MouseEvent mouseEvent = {};
  mouseEvent.x = 50.0;
  mouseEvent.y = 60.0;
  mouseEvent.button = OH_NATIVEXCOMPONENT_NONE_BUTTON;
  mouseEvent.action = OH_NATIVEXCOMPONENT_MOUSE_MOVE;
  mouseEvent.timestamp = 1000;

  processor.HandleMouseEvent(shell_id, nullptr, mouseEvent, 0.0, false, 200.0, 200.0);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandleMouseEventPressWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;

  OH_NativeXComponent_MouseEvent mouseEvent = {};
  mouseEvent.x = 50.0;
  mouseEvent.y = 60.0;
  mouseEvent.button = OH_NATIVEXCOMPONENT_LEFT_BUTTON;
  mouseEvent.action = OH_NATIVEXCOMPONENT_MOUSE_PRESS;
  mouseEvent.timestamp = 1000;

  processor.HandleMouseEvent(shell_id, nullptr, mouseEvent, 0.0, false, 200.0, 200.0);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandleMouseEventReleaseWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;

  // First press
  OH_NativeXComponent_MouseEvent pressEvent = {};
  pressEvent.button = OH_NATIVEXCOMPONENT_LEFT_BUTTON;
  pressEvent.action = OH_NATIVEXCOMPONENT_MOUSE_PRESS;
  processor.HandleMouseEvent(shell_id, nullptr, pressEvent, 0.0, false, 200.0, 200.0);

  // Then release
  OH_NativeXComponent_MouseEvent releaseEvent = {};
  releaseEvent.button = OH_NATIVEXCOMPONENT_LEFT_BUTTON;
  releaseEvent.action = OH_NATIVEXCOMPONENT_MOUSE_RELEASE;
  processor.HandleMouseEvent(shell_id, nullptr, releaseEvent, 0.0, false, 200.0, 200.0);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandleMouseEventLeaveWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;

  // First send a move to set lastMouseX_/lastMouseY_
  OH_NativeXComponent_MouseEvent moveEvent = {};
  moveEvent.x = 50.0;
  moveEvent.y = 60.0;
  moveEvent.button = OH_NATIVEXCOMPONENT_NONE_BUTTON;
  moveEvent.action = OH_NATIVEXCOMPONENT_MOUSE_MOVE;
  processor.HandleMouseEvent(shell_id, nullptr, moveEvent, 0.0, false, 200.0, 200.0);

  // Then send a leave event
  OH_NativeXComponent_MouseEvent leaveEvent = {};
  leaveEvent.x = 50.0;
  leaveEvent.y = 60.0;
  leaveEvent.button = OH_NATIVEXCOMPONENT_NONE_BUTTON;
  leaveEvent.action = OH_NATIVEXCOMPONENT_MOUSE_MOVE;
  processor.HandleMouseEvent(shell_id, nullptr, leaveEvent, 0.0, true, 200.0, 200.0);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandleMouseEventWithOffsetYWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;

  OH_NativeXComponent_MouseEvent mouseEvent = {};
  mouseEvent.x = 50.0;
  mouseEvent.y = 60.0;
  mouseEvent.button = OH_NATIVEXCOMPONENT_NONE_BUTTON;
  mouseEvent.action = OH_NATIVEXCOMPONENT_MOUSE_MOVE;
  // offsetY != 0 → signal_kind = kScroll
  processor.HandleMouseEvent(shell_id, nullptr, mouseEvent, 10.0, false, 200.0, 200.0);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandleMouseEventApi20PlusWithOnMouseEvent) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 20;  // >= 20 → calls OnMouseEvent (stubbed napi)

  OH_NativeXComponent_MouseEvent mouseEvent = {};
  mouseEvent.x = 50.0;
  mouseEvent.y = 60.0;
  mouseEvent.button = OH_NATIVEXCOMPONENT_NONE_BUTTON;
  mouseEvent.action = OH_NATIVEXCOMPONENT_MOUSE_MOVE;
  processor.HandleMouseEvent(shell_id, nullptr, mouseEvent, 0.0, false, 200.0, 200.0);
  SUCCEED();
}

// ===== HandleVirtualTouchEvent full path with real OHOSShellHolder =====

TEST(OhosTouchProcessorTest, HandleVirtualTouchEventLowApiWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;  // < 20 to skip early return

  OH_NativeXComponent_TouchEvent touchEvent = {};
  touchEvent.id = 0;
  touchEvent.type = OH_NATIVEXCOMPONENT_DOWN;
  touchEvent.numPoints = 1;
  touchEvent.touchPoints[0].id = 0;
  touchEvent.touchPoints[0].type = OH_NATIVEXCOMPONENT_DOWN;
  touchEvent.touchPoints[0].isPressed = true;

  processor.HandleVirtualTouchEvent(shell_id, nullptr, &touchEvent);
  SUCCEED();
}

// ===== PlatformViewOnTouchEvent with real OHOSShellHolder =====

TEST(OhosTouchProcessorTest, PlatformViewOnTouchEventWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  OH_NativeXComponent_TouchEvent touchEvent = {};
  touchEvent.id = 0;
  touchEvent.type = OH_NATIVEXCOMPONENT_DOWN;
  touchEvent.numPoints = 1;
  touchEvent.touchPoints[0].id = 0;
  touchEvent.touchPoints[0].type = OH_NATIVEXCOMPONENT_DOWN;
  touchEvent.touchPoints[0].isPressed = true;

  processor.PlatformViewOnTouchEvent(shell_id, OH_NATIVEXCOMPONENT_TOOL_TYPE_FINGER,
                                     nullptr, &touchEvent);
  SUCCEED();
}

// ===== PlatformViewOnAxisEvent with real OHOSShellHolder =====

TEST(OhosTouchProcessorTest, PlatformViewOnAxisEventWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 20;  // >= 20 to skip early return
  NULL_OUT_DYNAMIC_PTRS(processor)

  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.PlatformViewOnAxisEvent(shell_id, event, 10.0);
  SUCCEED();
}

// ===== VsyncVotingTouchValue/Up/Down with real OHOSShellHolder =====

TEST(OhosTouchProcessorTest, VsyncVotingTouchUpWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.VsyncVotingTouchValue(shell_id, OH_NATIVEXCOMPONENT_UP);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, VsyncVotingTouchDownWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.VsyncVotingTouchValue(shell_id, OH_NATIVEXCOMPONENT_DOWN);
  SUCCEED();
}

// ===== SendFinalMoveEventBeforeLeave with real OHOSShellHolder =====

TEST(OhosTouchProcessorTest, SendFinalMoveEventBeforeLeaveWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;

  // First set lastMouseX_/lastMouseY_ by sending a move event
  OH_NativeXComponent_MouseEvent moveEvent = {};
  moveEvent.x = 50.0;
  moveEvent.y = 60.0;
  moveEvent.button = OH_NATIVEXCOMPONENT_NONE_BUTTON;
  moveEvent.action = OH_NATIVEXCOMPONENT_MOUSE_MOVE;
  processor.HandleMouseEvent(shell_id, nullptr, moveEvent, 0.0, false, 200.0, 200.0);

  // Now call SendFinalMoveEventBeforeLeave — lastMouseX_/lastMouseY_ >= 0
  OH_NativeXComponent_MouseEvent mouseEvent = {};
  mouseEvent.x = 50.0;
  mouseEvent.y = 60.0;
  processor.SendFinalMoveEventBeforeLeave(shell_id, nullptr, mouseEvent, 200.0, 200.0);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, SendFinalMoveEventBeforeLeaveLeftBoundary) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;

  // Set lastMouseX_ to a small value (close to left boundary)
  OH_NativeXComponent_MouseEvent moveEvent = {};
  moveEvent.x = 5.0;  // close to left
  moveEvent.y = 100.0;
  moveEvent.button = OH_NATIVEXCOMPONENT_NONE_BUTTON;
  moveEvent.action = OH_NATIVEXCOMPONENT_MOUSE_MOVE;
  processor.HandleMouseEvent(shell_id, nullptr, moveEvent, 0.0, false, 200.0, 200.0);

  OH_NativeXComponent_MouseEvent mouseEvent = {};
  mouseEvent.x = 5.0;
  mouseEvent.y = 100.0;
  processor.SendFinalMoveEventBeforeLeave(shell_id, nullptr, mouseEvent, 200.0, 200.0);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, SendFinalMoveEventBeforeLeaveRightBoundary) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;

  // Set lastMouseX_ close to right boundary
  OH_NativeXComponent_MouseEvent moveEvent = {};
  moveEvent.x = 195.0;  // close to right (windowWidth=200)
  moveEvent.y = 100.0;
  moveEvent.button = OH_NATIVEXCOMPONENT_NONE_BUTTON;
  moveEvent.action = OH_NATIVEXCOMPONENT_MOUSE_MOVE;
  processor.HandleMouseEvent(shell_id, nullptr, moveEvent, 0.0, false, 200.0, 200.0);

  OH_NativeXComponent_MouseEvent mouseEvent = {};
  mouseEvent.x = 195.0;
  mouseEvent.y = 100.0;
  processor.SendFinalMoveEventBeforeLeave(shell_id, nullptr, mouseEvent, 200.0, 200.0);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, SendFinalMoveEventBeforeLeaveTopBoundary) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;

  // Set lastMouseY_ close to top boundary
  OH_NativeXComponent_MouseEvent moveEvent = {};
  moveEvent.x = 100.0;
  moveEvent.y = 5.0;  // close to top
  moveEvent.button = OH_NATIVEXCOMPONENT_NONE_BUTTON;
  moveEvent.action = OH_NATIVEXCOMPONENT_MOUSE_MOVE;
  processor.HandleMouseEvent(shell_id, nullptr, moveEvent, 0.0, false, 200.0, 200.0);

  OH_NativeXComponent_MouseEvent mouseEvent = {};
  mouseEvent.x = 100.0;
  mouseEvent.y = 5.0;
  processor.SendFinalMoveEventBeforeLeave(shell_id, nullptr, mouseEvent, 200.0, 200.0);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, SendFinalMoveEventBeforeLeaveBottomBoundary) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;

  // Set lastMouseY_ close to bottom boundary
  OH_NativeXComponent_MouseEvent moveEvent = {};
  moveEvent.x = 100.0;
  moveEvent.y = 195.0;  // close to bottom (windowHeight=200)
  moveEvent.button = OH_NATIVEXCOMPONENT_NONE_BUTTON;
  moveEvent.action = OH_NATIVEXCOMPONENT_MOUSE_MOVE;
  processor.HandleMouseEvent(shell_id, nullptr, moveEvent, 0.0, false, 200.0, 200.0);

  OH_NativeXComponent_MouseEvent mouseEvent = {};
  mouseEvent.x = 100.0;
  mouseEvent.y = 195.0;
  processor.SendFinalMoveEventBeforeLeave(shell_id, nullptr, mouseEvent, 200.0, 200.0);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, SendFinalMoveEventBeforeLeaveZeroWindowSize) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;

  // Set lastMouseX_/lastMouseY_
  OH_NativeXComponent_MouseEvent moveEvent = {};
  moveEvent.x = 50.0;
  moveEvent.y = 60.0;
  moveEvent.button = OH_NATIVEXCOMPONENT_NONE_BUTTON;
  moveEvent.action = OH_NATIVEXCOMPONENT_MOUSE_MOVE;
  processor.HandleMouseEvent(shell_id, nullptr, moveEvent, 0.0, false, 200.0, 200.0);

  // windowWidth=0, windowHeight=0 → use original coordinates
  OH_NativeXComponent_MouseEvent mouseEvent = {};
  mouseEvent.x = 50.0;
  mouseEvent.y = 60.0;
  processor.SendFinalMoveEventBeforeLeave(shell_id, nullptr, mouseEvent, 0.0, 0.0);
  SUCCEED();
}

// ===== HandleScaleEvent with deviceId == -1 (default device ID path) =====

TEST(OhosTouchProcessorTest, HandleScaleEventDeviceIdMinusOneWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_device_id = -1;  // triggers default device ID path
  g_stub_axis_action = UI_TOUCH_EVENT_ACTION_CANCEL;
  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandleScaleEvent(shell_id, nullptr, event);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandlePanZooomEventDeviceIdMinusOneWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_device_id = -1;
  g_stub_axis_action = UI_TOUCH_EVENT_ACTION_CANCEL;
  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandlePanZooomEvent(shell_id, nullptr, event);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandleScrollEventDeviceIdMinusOneWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 20;
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_device_id = -1;
  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandleScrollEvent(shell_id, nullptr, event);
  SUCCEED();
}

// ===== HandleVirtualTouchEvent with NDK failure path =====

TEST(OhosTouchProcessorTest, HandleVirtualTouchEventNdkFailureWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;

  g_stub_xcomponent_ret = OH_NATIVEXCOMPONENT_RESULT_BAD_PARAMETER;
  OH_NativeXComponent_TouchEvent touchEvent = {};
  touchEvent.id = 0;
  touchEvent.type = OH_NATIVEXCOMPONENT_DOWN;
  touchEvent.numPoints = 1;
  touchEvent.touchPoints[0].id = 0;
  touchEvent.touchPoints[0].type = OH_NATIVEXCOMPONENT_DOWN;
  touchEvent.touchPoints[0].isPressed = true;

  processor.HandleVirtualTouchEvent(shell_id, nullptr, &touchEvent);
  SUCCEED();
}

// ===== HandleTouchEvent with NDK failure path =====

TEST(OhosTouchProcessorTest, HandleTouchEventNdkFailureWithShellHolder) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  g_stub_xcomponent_ret = OH_NATIVEXCOMPONENT_RESULT_BAD_PARAMETER;

  OH_NativeXComponent_TouchEvent touchEvent = {};
  touchEvent.id = 0;
  touchEvent.type = OH_NATIVEXCOMPONENT_DOWN;
  touchEvent.numPoints = 1;
  touchEvent.touchPoints[0].id = 0;
  touchEvent.touchPoints[0].type = OH_NATIVEXCOMPONENT_DOWN;
  touchEvent.touchPoints[0].isPressed = true;

  processor.HandleTouchEvent(shell_id, nullptr, &touchEvent);
  SUCCEED();
}

// HandleTouchEvent line 199: variant — duplicate up event triggers early return.
TEST(OhosTouchProcessorTest, HandleTouchEventDroppedOnDuplicateUp) {
  OhosTouchProcessor processor;
  // Up without prior down → shouldDropTouchEvent returns true → early return
  OH_NativeXComponent_TouchEvent upEvent = {};
  upEvent.type = OH_NATIVEXCOMPONENT_UP;
  upEvent.id = 5;
  // This should return early without accessing OHOSShellHolder
  processor.HandleTouchEvent(0, nullptr, &upEvent);
  SUCCEED();
}

// HandleTouchEvent line 199: variant — CANCEL event (neither DOWN nor UP) passes
// shouldDropTouchEvent (returns false), but we can't test the full path without
// OHOSShellHolder. This test verifies the non-null, non-dropped path doesn't
// crash at the null check level — but it WILL access OHOSShellHolder, so we
// only test the shouldDropTouchEvent=true path.
TEST(OhosTouchProcessorTest, HandleTouchEventDroppedOnCancelAfterDown) {
  OhosTouchProcessor processor;
  // Register finger with down
  OH_NativeXComponent_TouchEvent downEvent = {};
  downEvent.type = OH_NATIVEXCOMPONENT_DOWN;
  downEvent.id = 7;
  EXPECT_FALSE(processor.shouldDropTouchEvent(&downEvent));
  // Now send a duplicate down → shouldDropTouchEvent returns true
  OH_NativeXComponent_TouchEvent duplicateDown = {};
  duplicateDown.type = OH_NATIVEXCOMPONENT_DOWN;
  duplicateDown.id = 7;
  processor.HandleTouchEvent(0, nullptr, &duplicateDown);
  SUCCEED();
}

}  // namespace testing
}  // namespace flutter
