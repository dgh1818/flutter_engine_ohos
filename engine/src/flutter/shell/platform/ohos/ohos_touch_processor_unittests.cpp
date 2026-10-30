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

#include <gtest/gtest.h>
#include <cstring>
#include <memory>
#include <string>
#include "flutter/fml/log_settings.h"
#include "flutter/shell/platform/ohos/ohos_shell_holder.h"
#include "flutter/shell/platform/ohos/ohos_touch_processor.h"
#include "flutter/shell/platform/ohos/test_stubs/ace_graphic_ndk_stub.h"
#include "flutter/shell/platform/ohos/test_stubs/ace_napi_stub.h"

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
  EXPECT_EQ(processor.getPointerChangeForMouseAction(
                static_cast<OH_NativeXComponent_MouseEventAction>(99999)),
            PointerData::Change::kCancel);
}

// ===== getPointerButtonFromMouse =====

TEST(OhosTouchProcessorTest, GetPointerButtonFromLeftButton) {
  OhosTouchProcessor processor;
  EXPECT_EQ(
      processor.getPointerButtonFromMouse(OH_NATIVEXCOMPONENT_LEFT_BUTTON),
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
  EXPECT_EQ(
      processor.getPointerButtonFromMouse(OH_NATIVEXCOMPONENT_BACK_BUTTON),
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
  EXPECT_EQ(processor.getPointerButtonFromMouse(
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
  EXPECT_FALSE(processor.shouldDropTouchEvent(&moveEvent));

  OH_NativeXComponent_TouchEvent downEvent = {};
  downEvent.type = OH_NATIVEXCOMPONENT_DOWN;
  downEvent.id = 3;
  EXPECT_FALSE(processor.shouldDropTouchEvent(&downEvent));
  OH_NativeXComponent_TouchEvent cancelEvent = {};
  cancelEvent.type = OH_NATIVEXCOMPONENT_CANCEL;
  cancelEvent.id = 3;
  EXPECT_FALSE(processor.shouldDropTouchEvent(&cancelEvent));
  OH_NativeXComponent_TouchEvent upAfterCancel = {};
  upAfterCancel.type = OH_NATIVEXCOMPONENT_UP;
  upAfterCancel.id = 3;
  EXPECT_TRUE(processor.shouldDropTouchEvent(&upAfterCancel));
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

TEST(OhosTouchProcessorTest, ShouldDropTouchEventDownUpDownCycleForSameFinger) {
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
  EXPECT_EQ(result[21], std::to_string(OH_NATIVEXCOMPONENT_TOOL_TYPE_FINGER));
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
  EXPECT_TRUE(
      processor.HandleMouseButtonEvent(mouseEvent, change, buttons_to_send));
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
  EXPECT_TRUE(
      processor.HandleMouseButtonEvent(release, changeRelease, buttonsRelease));
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
  EXPECT_TRUE(
      processor.HandleMouseButtonEvent(press, changePress, buttonsPress));
  EXPECT_EQ(changePress, PointerData::Change::kDown);
  EXPECT_EQ(buttonsPress, kPointerButtonMouseMiddle);

  // Release middle button
  OH_NativeXComponent_MouseEvent release = {};
  release.button = OH_NATIVEXCOMPONENT_MIDDLE_BUTTON;
  release.action = OH_NATIVEXCOMPONENT_MOUSE_RELEASE;
  PointerData::Change changeRelease;
  int64_t buttonsRelease = 0;
  EXPECT_TRUE(
      processor.HandleMouseButtonEvent(release, changeRelease, buttonsRelease));
  EXPECT_EQ(changeRelease, PointerData::Change::kUp);
  EXPECT_EQ(buttonsRelease, 0);
}

// ===== Early-return branches =====
// These test the null/version-check early-return paths of functions that
// otherwise require OHOSShellHolder + NDK event objects.

TEST(OhosTouchProcessorTest, HandleTouchEventReturnsOnNullEvent) {
  OhosTouchProcessor processor;
  processor.HandleTouchEvent(0, nullptr, nullptr);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, SetViewIdStoresForPointerRouting) {
  OhosTouchProcessor processor;
  EXPECT_EQ(processor.view_id_, 0);
  processor.SetViewId(5);
  EXPECT_EQ(processor.view_id_, 5);
  processor.SetViewId(0);
  EXPECT_EQ(processor.view_id_, 0);
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
  {
    fml::ScopedSetLogSettings loud({fml::kLogInfo});
    processor.PlatformViewOnAxisEvent(
        0, reinterpret_cast<ArkUI_UIInputEvent*>(0x1), 0.0);
  }
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
TEST(OhosTouchProcessorTest,
     HandleAxisEventLowApiVersionSecondCallSkipsWarning) {
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

// HandleTouchEvent line 199: `if (touchEvent == nullptr ||
// shouldDropTouchEvent(touchEvent))` — covers the false branch of `touchEvent
// == nullptr` (non-null event) and the true branch of `shouldDropTouchEvent`
// (duplicate down → early return before OHOSShellHolder access).
TEST(OhosTouchProcessorTest, HandleTouchEventDroppedOnDuplicateDown) {
  GraphicStubKnobGuard knob_guard;
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
// These definitions are strong symbols in the main executable; at link time
// they take precedence over the same symbols in the shared libraries
// (standard ELF symbol interposition), allowing us to test functions that
// depend on NDK/napi runtime without a real device environment.
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
uint64_t g_stub_modifier_keys = 0;
int32_t g_stub_modifier_error = 0;

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
  g_stub_modifier_keys = 0;
  g_stub_modifier_error = 0;
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
  out_holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  EXPECT_TRUE(out_holder->IsValid());
  return reinterpret_cast<int64_t>(out_holder.get());
}

// Helper to null out dynamic function pointers loaded via dlsym.
// These point to real NDK functions that would crash on fake event pointers.
// Setting them to nullptr makes the code take the safe fallback paths.
int32_t StubGetDeviceId(ArkUI_UIInputEvent*) {
  return g_stub_device_id;
}

int32_t StubGetAxisAction(ArkUI_UIInputEvent*) {
  return g_stub_axis_action;
}

int32_t StubGetModifierKeyStates(ArkUI_UIInputEvent*, uint64_t* keys) {
  if (keys != nullptr) {
    *keys = g_stub_modifier_keys;
  }
  return g_stub_modifier_error;
}

#define NULL_OUT_DYNAMIC_PTRS(processor)                 \
  (processor).dynamicGetDeviceId_ = StubGetDeviceId;     \
  (processor).dynamicGetAxisAction_ = StubGetAxisAction; \
  (processor).dynamicGetModifierKeyStates_ = StubGetModifierKeyStates;
}  // namespace

// Stub ArkUI UIInputEvent functions
extern "C" int32_t OH_ArkUI_UIInputEvent_GetToolType(
    const ArkUI_UIInputEvent* event) {
  return g_stub_tool_type;
}

extern "C" int64_t OH_ArkUI_UIInputEvent_GetEventTime(
    const ArkUI_UIInputEvent* event) {
  return g_stub_event_time;
}

extern "C" float OH_ArkUI_PointerEvent_GetX(const ArkUI_UIInputEvent* event) {
  return g_stub_pointer_x;
}

extern "C" float OH_ArkUI_PointerEvent_GetY(const ArkUI_UIInputEvent* event) {
  return g_stub_pointer_y;
}

extern "C" float OH_ArkUI_PointerEvent_GetWindowX(
    const ArkUI_UIInputEvent* event) {
  return g_stub_pointer_window_x;
}

extern "C" float OH_ArkUI_PointerEvent_GetWindowY(
    const ArkUI_UIInputEvent* event) {
  return g_stub_pointer_window_y;
}

extern "C" float OH_ArkUI_PointerEvent_GetDisplayX(
    const ArkUI_UIInputEvent* event) {
  return g_stub_pointer_display_x;
}

extern "C" float OH_ArkUI_PointerEvent_GetDisplayY(
    const ArkUI_UIInputEvent* event) {
  return g_stub_pointer_display_y;
}

extern "C" double OH_ArkUI_AxisEvent_GetVerticalAxisValue(
    const ArkUI_UIInputEvent* event) {
  return g_stub_vertical_axis_value;
}

extern "C" double OH_ArkUI_AxisEvent_GetHorizontalAxisValue(
    const ArkUI_UIInputEvent* event) {
  return g_stub_horizontal_axis_value;
}

extern "C" double OH_ArkUI_AxisEvent_GetPinchAxisScaleValue(
    const ArkUI_UIInputEvent* event) {
  return g_stub_pinch_scale_value;
}

// ===== OHOSShellHolder integration tests =====
// These tests construct a real OHOSShellHolder (with software rendering) to
// obtain a valid shell_holderID, then exercise the full Handle*Event paths
// that were previously untestable. The NDK and napi stubs above ensure these
// paths don't crash.

// ===== HandleTouchEvent full path with real OHOSShellHolder =====

TEST(OhosTouchProcessorTest, HandleTouchEventFullPathWithShellHolder) {
  GraphicStubKnobGuard knob_guard;
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
  GraphicStubKnobGuard knob_guard;
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
  GraphicStubKnobGuard knob_guard;
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
  GraphicStubKnobGuard knob_guard;
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

  g_stub_modifier_keys = ARKUI_MODIFIER_KEY_CTRL;
  g_stub_modifier_error = ARKUI_ERROR_CODE_PARAM_INVALID;
  processor.HandleAxisEvent(shell_id, nullptr, event);
}

TEST(OhosTouchProcessorTest, HandleAxisEventMouseCtrlScrollWithShellHolder) {
  GraphicStubKnobGuard knob_guard;
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 20;
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_tool_type = UI_INPUT_EVENT_TOOL_TYPE_MOUSE;
  g_stub_modifier_keys = ARKUI_MODIFIER_KEY_CTRL;
  g_stub_modifier_error = 0;
  g_stub_axis_action = UI_TOUCH_EVENT_ACTION_DOWN;

  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandleAxisEvent(shell_id, nullptr, event);
  EXPECT_FLOAT_EQ(processor.accumulatedScale_, 1.0f);
}

TEST(OhosTouchProcessorTest, HandleAxisEventTouchpadPanZoomWithShellHolder) {
  GraphicStubKnobGuard knob_guard;
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
  GraphicStubKnobGuard knob_guard;
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

TEST(OhosTouchProcessorTest, HandleScaleEventNullDynamicPtrsFallback) {
  GraphicStubKnobGuard knob_guard;
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 20;
  processor.dynamicGetDeviceId_ = nullptr;
  processor.dynamicGetAxisAction_ = nullptr;
  processor.dynamicGetModifierKeyStates_ = nullptr;

  g_stub_tool_type = UI_INPUT_EVENT_TOOL_TYPE_MOUSE;
  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandleScaleEvent(shell_id, nullptr, event);
  processor.HandlePanZooomEvent(shell_id, nullptr, event);
  processor.HandleScrollEvent(shell_id, nullptr, event);
  processor.HandleAxisEvent(shell_id, nullptr, event);
  EXPECT_FLOAT_EQ(processor.accumulatedScale_, 1.0f);
}

TEST(OhosTouchProcessorTest, HandleScaleEventCancelActionWithShellHolder) {
  GraphicStubKnobGuard knob_guard;
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
  GraphicStubKnobGuard knob_guard;
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;  // < 20 to skip OnAxisEvent path
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_axis_action = UI_TOUCH_EVENT_ACTION_DOWN;
  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandleScaleEvent(shell_id, nullptr, event);
  EXPECT_FLOAT_EQ(processor.accumulatedScale_, 1.0f);

  g_stub_device_id = -1;
  processor.HandleScaleEvent(shell_id, nullptr, event);
  EXPECT_FLOAT_EQ(processor.accumulatedScale_, 1.0f);
}

TEST(OhosTouchProcessorTest, HandleScaleEventMoveActionWithShellHolder) {
  GraphicStubKnobGuard knob_guard;
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;  // < 20 to skip OnAxisEvent path
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_axis_action = UI_TOUCH_EVENT_ACTION_MOVE;
  g_stub_vertical_axis_value = -1.0;  // negative → ZOOM_IN
  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandleScaleEvent(shell_id, nullptr, event);
  EXPECT_FLOAT_EQ(processor.accumulatedScale_, 10.0f / 8.0f);
}

TEST(OhosTouchProcessorTest,
     HandleScaleEventMoveActionPositiveAxisWithShellHolder) {
  GraphicStubKnobGuard knob_guard;
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_axis_action = UI_TOUCH_EVENT_ACTION_MOVE;
  g_stub_vertical_axis_value = 1.0;  // positive → ZOOM_OUT
  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandleScaleEvent(shell_id, nullptr, event);
  EXPECT_FLOAT_EQ(processor.accumulatedScale_, 8.0f / 10.0f);
}

TEST(OhosTouchProcessorTest, HandleScaleEventUpActionWithShellHolder) {
  GraphicStubKnobGuard knob_guard;
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_axis_action = UI_TOUCH_EVENT_ACTION_UP;
  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  const float scale_before = processor.accumulatedScale_;
  processor.HandleScaleEvent(shell_id, nullptr, event);
  EXPECT_FLOAT_EQ(processor.accumulatedScale_, scale_before);
}

TEST(OhosTouchProcessorTest, HandleScaleEventDefaultActionWithShellHolder) {
  GraphicStubKnobGuard knob_guard;
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_axis_action = 999;  // default case
  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandleScaleEvent(shell_id, nullptr, event);
  {
    fml::ScopedSetLogSettings quiet({fml::kLogFatal});
    processor.HandleScaleEvent(shell_id, nullptr, event);
  }
}

TEST(OhosTouchProcessorTest, HandleScaleEventApi20PlusWithOnAxisEvent) {
  GraphicStubKnobGuard knob_guard;
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
  GraphicStubKnobGuard knob_guard;
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
  GraphicStubKnobGuard knob_guard;
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_axis_action = UI_TOUCH_EVENT_ACTION_DOWN;
  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandlePanZooomEvent(shell_id, nullptr, event);
  EXPECT_FLOAT_EQ(processor.accumulatedPanX_, 0.0f);
  EXPECT_FLOAT_EQ(processor.accumulatedPanY_, 0.0f);

  g_stub_device_id = -1;
  processor.HandlePanZooomEvent(shell_id, nullptr, event);
  EXPECT_FLOAT_EQ(processor.accumulatedPanX_, 0.0f);
}

TEST(OhosTouchProcessorTest, HandlePanZooomEventMoveActionWithShellHolder) {
  GraphicStubKnobGuard knob_guard;
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
  EXPECT_FLOAT_EQ(processor.accumulatedPanX_, -5.0f);
  EXPECT_FLOAT_EQ(processor.accumulatedPanY_, -10.0f);
}

TEST(OhosTouchProcessorTest, HandlePanZooomEventUpActionWithShellHolder) {
  GraphicStubKnobGuard knob_guard;
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
  GraphicStubKnobGuard knob_guard;
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_axis_action = 999;  // default case
  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);
  processor.HandlePanZooomEvent(shell_id, nullptr, event);
  {
    fml::ScopedSetLogSettings quiet({fml::kLogFatal});
    processor.HandlePanZooomEvent(shell_id, nullptr, event);
  }
}

TEST(OhosTouchProcessorTest, HandlePanZooomEventZeroScaleWithShellHolder) {
  GraphicStubKnobGuard knob_guard;
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
  GraphicStubKnobGuard knob_guard;
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
  GraphicStubKnobGuard knob_guard;
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
  GraphicStubKnobGuard knob_guard;
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
  GraphicStubKnobGuard knob_guard;
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

  processor.HandleMouseEvent(shell_id, nullptr, mouseEvent, 0.0, false, 200.0,
                             200.0);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandleMouseEventPressWithShellHolder) {
  GraphicStubKnobGuard knob_guard;
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

  processor.HandleMouseEvent(shell_id, nullptr, mouseEvent, 0.0, false, 200.0,
                             200.0);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandleMouseEventReleaseWithShellHolder) {
  GraphicStubKnobGuard knob_guard;
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 10;

  // First press
  OH_NativeXComponent_MouseEvent pressEvent = {};
  pressEvent.button = OH_NATIVEXCOMPONENT_LEFT_BUTTON;
  pressEvent.action = OH_NATIVEXCOMPONENT_MOUSE_PRESS;
  processor.HandleMouseEvent(shell_id, nullptr, pressEvent, 0.0, false, 200.0,
                             200.0);

  processor.HandleMouseEvent(shell_id, nullptr, pressEvent, 0.0, false, 200.0,
                             200.0);

  // Then release
  OH_NativeXComponent_MouseEvent releaseEvent = {};
  releaseEvent.button = OH_NATIVEXCOMPONENT_LEFT_BUTTON;
  releaseEvent.action = OH_NATIVEXCOMPONENT_MOUSE_RELEASE;
  processor.HandleMouseEvent(shell_id, nullptr, releaseEvent, 0.0, false, 200.0,
                             200.0);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandleMouseEventLeaveWithShellHolder) {
  GraphicStubKnobGuard knob_guard;
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
  processor.HandleMouseEvent(shell_id, nullptr, moveEvent, 0.0, false, 200.0,
                             200.0);

  // Then send a leave event
  OH_NativeXComponent_MouseEvent leaveEvent = {};
  leaveEvent.x = 50.0;
  leaveEvent.y = 60.0;
  leaveEvent.button = OH_NATIVEXCOMPONENT_NONE_BUTTON;
  leaveEvent.action = OH_NATIVEXCOMPONENT_MOUSE_MOVE;
  processor.HandleMouseEvent(shell_id, nullptr, leaveEvent, 0.0, true, 200.0,
                             200.0);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandleMouseEventWithOffsetYWithShellHolder) {
  GraphicStubKnobGuard knob_guard;
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
  processor.HandleMouseEvent(shell_id, nullptr, mouseEvent, 10.0, false, 200.0,
                             200.0);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, HandleMouseEventApi20PlusWithOnMouseEvent) {
  GraphicStubKnobGuard knob_guard;
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 20;  // >= 20 → calls OnMouseEvent (stubbed napi)

  OH_NativeXComponent_MouseEvent mouseEvent = {};
  mouseEvent.x = 50.0;
  mouseEvent.y = 60.0;
  mouseEvent.button = OH_NATIVEXCOMPONENT_NONE_BUTTON;
  mouseEvent.action = OH_NATIVEXCOMPONENT_MOUSE_MOVE;
  processor.HandleMouseEvent(shell_id, nullptr, mouseEvent, 0.0, false, 200.0,
                             200.0);
  SUCCEED();
}

// ===== HandleVirtualTouchEvent full path with real OHOSShellHolder =====

TEST(OhosTouchProcessorTest, HandleVirtualTouchEventLowApiWithShellHolder) {
  GraphicStubKnobGuard knob_guard;
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
  GraphicStubKnobGuard knob_guard;
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

  processor.PlatformViewOnTouchEvent(
      shell_id, OH_NATIVEXCOMPONENT_TOOL_TYPE_FINGER, nullptr, &touchEvent);
  SUCCEED();
}

// ===== PlatformViewOnAxisEvent with real OHOSShellHolder =====

TEST(OhosTouchProcessorTest, PlatformViewOnAxisEventWithShellHolder) {
  GraphicStubKnobGuard knob_guard;
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
  GraphicStubKnobGuard knob_guard;
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.VsyncVotingTouchValue(shell_id, OH_NATIVEXCOMPONENT_UP);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, VsyncVotingTouchDownWithShellHolder) {
  GraphicStubKnobGuard knob_guard;
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.VsyncVotingTouchValue(shell_id, OH_NATIVEXCOMPONENT_DOWN);
  SUCCEED();
}

// ===== SendFinalMoveEventBeforeLeave with real OHOSShellHolder =====

TEST(OhosTouchProcessorTest, SendFinalMoveEventBeforeLeaveWithShellHolder) {
  GraphicStubKnobGuard knob_guard;
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
  processor.HandleMouseEvent(shell_id, nullptr, moveEvent, 0.0, false, 200.0,
                             200.0);

  // Now call SendFinalMoveEventBeforeLeave — lastMouseX_/lastMouseY_ >= 0
  OH_NativeXComponent_MouseEvent mouseEvent = {};
  mouseEvent.x = 50.0;
  mouseEvent.y = 60.0;
  processor.SendFinalMoveEventBeforeLeave(shell_id, nullptr, mouseEvent, 200.0,
                                          200.0);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, SendFinalMoveEventBeforeLeaveLeftBoundary) {
  GraphicStubKnobGuard knob_guard;
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
  processor.HandleMouseEvent(shell_id, nullptr, moveEvent, 0.0, false, 200.0,
                             200.0);

  OH_NativeXComponent_MouseEvent mouseEvent = {};
  mouseEvent.x = 5.0;
  mouseEvent.y = 100.0;
  processor.SendFinalMoveEventBeforeLeave(shell_id, nullptr, mouseEvent, 200.0,
                                          200.0);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, SendFinalMoveEventBeforeLeaveRightBoundary) {
  GraphicStubKnobGuard knob_guard;
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
  processor.HandleMouseEvent(shell_id, nullptr, moveEvent, 0.0, false, 200.0,
                             200.0);

  OH_NativeXComponent_MouseEvent mouseEvent = {};
  mouseEvent.x = 195.0;
  mouseEvent.y = 100.0;
  processor.SendFinalMoveEventBeforeLeave(shell_id, nullptr, mouseEvent, 200.0,
                                          200.0);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, SendFinalMoveEventBeforeLeaveTopBoundary) {
  GraphicStubKnobGuard knob_guard;
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
  processor.HandleMouseEvent(shell_id, nullptr, moveEvent, 0.0, false, 200.0,
                             200.0);

  OH_NativeXComponent_MouseEvent mouseEvent = {};
  mouseEvent.x = 100.0;
  mouseEvent.y = 5.0;
  processor.SendFinalMoveEventBeforeLeave(shell_id, nullptr, mouseEvent, 200.0,
                                          200.0);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, SendFinalMoveEventBeforeLeaveBottomBoundary) {
  GraphicStubKnobGuard knob_guard;
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
  processor.HandleMouseEvent(shell_id, nullptr, moveEvent, 0.0, false, 200.0,
                             200.0);

  OH_NativeXComponent_MouseEvent mouseEvent = {};
  mouseEvent.x = 100.0;
  mouseEvent.y = 195.0;
  processor.SendFinalMoveEventBeforeLeave(shell_id, nullptr, mouseEvent, 200.0,
                                          200.0);
  SUCCEED();
}

TEST(OhosTouchProcessorTest, SendFinalMoveEventBeforeLeaveZeroWindowSize) {
  GraphicStubKnobGuard knob_guard;
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
  processor.HandleMouseEvent(shell_id, nullptr, moveEvent, 0.0, false, 200.0,
                             200.0);

  // windowWidth=0, windowHeight=0 → use original coordinates
  OH_NativeXComponent_MouseEvent mouseEvent = {};
  mouseEvent.x = 50.0;
  mouseEvent.y = 60.0;
  processor.SendFinalMoveEventBeforeLeave(shell_id, nullptr, mouseEvent, 0.0,
                                          0.0);
  processor.SendFinalMoveEventBeforeLeave(shell_id, nullptr, mouseEvent, 200.0,
                                          0.0);
  processor.lastMouseY_ = -1.0;
  processor.SendFinalMoveEventBeforeLeave(shell_id, nullptr, mouseEvent, 200.0,
                                          200.0);
  SUCCEED();
}

// ===== HandleScaleEvent with deviceId == -1 (default device ID path) =====

TEST(OhosTouchProcessorTest, HandleScaleEventDeviceIdMinusOneWithShellHolder) {
  GraphicStubKnobGuard knob_guard;
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

TEST(OhosTouchProcessorTest,
     HandlePanZooomEventDeviceIdMinusOneWithShellHolder) {
  GraphicStubKnobGuard knob_guard;
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
  GraphicStubKnobGuard knob_guard;
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

  GraphicStubKnobGuard knob_guard;
  g_stub_graphic_fail_mask =
      kStubFailGetTouchPointToolType | kStubFailTouchTilt;
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
  GraphicStubKnobGuard knob_guard;
  g_stub_graphic_fail_mask =
      kStubFailGetTouchPointToolType | kStubFailTouchTilt;

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

// HandleTouchEvent line 199: variant — duplicate up event triggers early
// return.
TEST(OhosTouchProcessorTest, HandleTouchEventDroppedOnDuplicateUp) {
  GraphicStubKnobGuard knob_guard;
  OhosTouchProcessor processor;
  // Up without prior down → shouldDropTouchEvent returns true → early return
  OH_NativeXComponent_TouchEvent upEvent = {};
  upEvent.type = OH_NATIVEXCOMPONENT_UP;
  upEvent.id = 5;
  // This should return early without accessing OHOSShellHolder
  processor.HandleTouchEvent(0, nullptr, &upEvent);
  SUCCEED();
}

// HandleTouchEvent line 199: variant — CANCEL event (neither DOWN nor UP)
// passes shouldDropTouchEvent (returns false), but we can't test the full path
// without OHOSShellHolder. This test verifies the non-null, non-dropped path
// doesn't crash at the null check level — but it WILL access OHOSShellHolder,
// so we only test the shouldDropTouchEvent=true path.
TEST(OhosTouchProcessorTest, HandleTouchEventDroppedOnCancelAfterDown) {
  GraphicStubKnobGuard knob_guard;
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  OH_NativeXComponent_TouchEvent downEvent = {};
  downEvent.type = OH_NATIVEXCOMPONENT_DOWN;
  downEvent.id = 7;
  downEvent.numPoints = 1;
  downEvent.touchPoints[0].id = 7;
  downEvent.touchPoints[0].type = OH_NATIVEXCOMPONENT_DOWN;
  processor.HandleTouchEvent(shell_id, nullptr, &downEvent);

  OH_NativeXComponent_TouchEvent cancelEvent = {};
  cancelEvent.type = OH_NATIVEXCOMPONENT_CANCEL;
  cancelEvent.id = 7;
  cancelEvent.numPoints = 1;
  cancelEvent.touchPoints[0].id = 7;
  cancelEvent.touchPoints[0].type = OH_NATIVEXCOMPONENT_CANCEL;
  processor.HandleTouchEvent(shell_id, nullptr, &cancelEvent);

  OH_NativeXComponent_TouchEvent upEvent = {};
  upEvent.type = OH_NATIVEXCOMPONENT_UP;
  upEvent.id = 7;
  EXPECT_TRUE(processor.shouldDropTouchEvent(&upEvent));
}

TEST(OhosTouchProcessorTest, HandleScrollEventDensityFallback) {
  GraphicStubKnobGuard knob_guard;
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 20;
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_vertical_axis_value = 10.0;
  g_stub_horizontal_axis_value = 5.0;
  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);

  double saved = PlatformViewOHOSNapi::display_density_pixels;
  PlatformViewOHOSNapi::display_density_pixels = 0.0;
  EXPECT_NO_FATAL_FAILURE(
      processor.HandleScrollEvent(shell_id, nullptr, event));
  PlatformViewOHOSNapi::display_density_pixels = 2.5;
  EXPECT_NO_FATAL_FAILURE(
      processor.HandleScrollEvent(shell_id, nullptr, event));
  PlatformViewOHOSNapi::display_density_pixels = saved;
}

TEST(OhosTouchProcessorTest, PlatformViewOnTouchEventTiltQueryFailure) {
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);

  OhosTouchProcessor processor;
  processor.apiVersion_ = 20;

  OH_NativeXComponent_TouchEvent touchEvent = {};
  touchEvent.numPoints = 1;
  touchEvent.id = 3;
  touchEvent.type = OH_NATIVEXCOMPONENT_MOVE;
  touchEvent.x = 10.0f;
  touchEvent.y = 20.0f;

  GraphicStubKnobGuard knob_guard;
  g_stub_graphic_fail_mask = kStubFailTouchTilt;
  EXPECT_NO_FATAL_FAILURE(processor.PlatformViewOnTouchEvent(
      shell_id, OH_NATIVEXCOMPONENT_TOOL_TYPE_FINGER, nullptr, &touchEvent));
}

TEST(OhosTouchProcessorTest, LogSeverityReplayRemainingEdges) {
  {
    fml::ScopedSetLogSettings loud({fml::kLogInfo});
    OhosTouchProcessor processor;
    OH_NativeXComponent_TouchEvent downEvent = {};
    downEvent.type = OH_NATIVEXCOMPONENT_DOWN;
    downEvent.id = 0;
    EXPECT_FALSE(processor.shouldDropTouchEvent(&downEvent));
    OH_NativeXComponent_TouchEvent duplicateDown = {};
    duplicateDown.type = OH_NATIVEXCOMPONENT_DOWN;
    duplicateDown.id = 0;
    processor.HandleTouchEvent(0, nullptr, &duplicateDown);

    OH_NativeXComponent_TouchEvent upEvent = {};
    upEvent.type = OH_NATIVEXCOMPONENT_UP;
    upEvent.id = 5;
    processor.HandleTouchEvent(0, nullptr, &upEvent);
  }
  {
    fml::ScopedSetLogSettings quiet({fml::kLogFatal});
    OhosTouchProcessor processor;
    processor.HandleAxisEvent(0, nullptr, nullptr);
    processor.apiVersion_ = 10;
    processor.HandleAxisEvent(0, nullptr,
                              reinterpret_cast<ArkUI_UIInputEvent*>(0x2));

    std::unique_ptr<OHOSShellHolder> holder;
    int64_t shell_id = CreateShellHolderForTest(holder);
    OH_NativeXComponent_TouchEvent touchEvent = {};
    touchEvent.numPoints = 1;
    touchEvent.id = 3;
    touchEvent.type = OH_NATIVEXCOMPONENT_MOVE;
    GraphicStubKnobGuard knob_guard;
    g_stub_graphic_fail_mask = kStubFailTouchTilt;
    EXPECT_NO_FATAL_FAILURE(processor.PlatformViewOnTouchEvent(
        shell_id, OH_NATIVEXCOMPONENT_TOOL_TYPE_FINGER, nullptr, &touchEvent));
    g_stub_graphic_fail_mask =
        kStubFailGetTouchPointToolType | kStubFailTouchTilt;
    EXPECT_NO_FATAL_FAILURE(processor.PlatformViewOnTouchEvent(
        shell_id, OH_NATIVEXCOMPONENT_TOOL_TYPE_FINGER, nullptr, &touchEvent));
    processor.apiVersion_ = 10;
    processor.PlatformViewOnAxisEvent(
        0, reinterpret_cast<ArkUI_UIInputEvent*>(0x1), 0.0);
  }
}

// ===== Screen-to-layout coordinate scaling for platform-view packets =====
//
// Regression tests for the WebView click-drift bug: when a custom DPI scale
// (flutter/displaymetrics 'updateDpiScale', e.g. the OHOS Column overflow
// adaptation) makes the Flutter DPR diverge from the system display density,
// pointer packets forwarded to platform views must be mapped from physical
// screen pixels onto the ArkUI layout pixel space (x system_density /
// flutter_dpr), otherwise clicks drift linearly with the coordinate value.
// See GetScreenToLayoutScale / ScaleTouchEventCoordinates in
// ohos_touch_processor.cpp and PlatformViewOHOS::
// GetScreenToPlatformViewLayoutScale.

namespace {

// Saves and restores the process-wide static system display density.
class ScopedSystemDensity {
 public:
  explicit ScopedSystemDensity(double value)
      : saved_(PlatformViewOHOSNapi::display_density_pixels) {
    PlatformViewOHOSNapi::display_density_pixels = value;
  }
  ~ScopedSystemDensity() {
    PlatformViewOHOSNapi::display_density_pixels = saved_;
  }

 private:
  double saved_;
};

// Enables the opt-in napi stub string recording for the lifetime of the
// object and exposes the strings that were forwarded to the JS side.
class ScopedNapiStringRecorder {
 public:
  ScopedNapiStringRecorder() {
    StubNapiClearRecordedStrings();
    StubNapiSetRecordStrings(true);
  }
  ~ScopedNapiStringRecorder() { StubNapiSetRecordStrings(false); }

  size_t size() const { return StubNapiRecordedStringCount(); }
  std::string at(size_t index) const {
    const char* s = StubNapiRecordedStringAt(index);
    return s != nullptr ? std::string(s) : std::string();
  }
};

// The napi stub rejects napi_create_string_utf8 calls made with a null env
// (they return napi_invalid_arg before the recording hook runs), and the
// static PlatformViewOHOSNapi::env_ used by the FlutterViewOn*Event callouts
// is only populated by nativeAttach on a real device. Tests that need the
// recorder to observe the serialized packets must therefore install a fake
// non-null env for the lifetime of the test (same trick as FakeNapiEnv() in
// platform_view_ohos_napi_unittests.cpp).
class ScopedNapiEnv {
 public:
  ScopedNapiEnv() : saved_(PlatformViewOHOSNapi::env_) {
    PlatformViewOHOSNapi::env_ = reinterpret_cast<napi_env>(0xF00D);
  }
  ~ScopedNapiEnv() { PlatformViewOHOSNapi::env_ = saved_; }

 private:
  napi_env saved_;
};

// Two-finger event whose main-event and per-point coordinates differ so that
// field offsets can be told apart in the serialized packet.
OH_NativeXComponent_TouchEvent MakeScalingTestTouchEvent() {
  OH_NativeXComponent_TouchEvent event = {};
  event.id = 7;
  event.screenX = 164.0f;
  event.screenY = 590.0f;
  event.x = 100.0f;
  event.y = 200.0f;
  event.type = OH_NATIVEXCOMPONENT_DOWN;
  event.size = 1.5;
  event.force = 0.5f;
  event.deviceId = 42;
  event.timeStamp = 1234567890;
  event.numPoints = 2;
  event.touchPoints[0].id = 0;
  event.touchPoints[0].screenX = 164.0f;
  event.touchPoints[0].screenY = 590.0f;
  event.touchPoints[0].x = 100.0f;
  event.touchPoints[0].y = 200.0f;
  event.touchPoints[0].type = OH_NATIVEXCOMPONENT_DOWN;
  event.touchPoints[0].size = 1.0;
  event.touchPoints[0].force = 0.5f;
  event.touchPoints[0].timeStamp = 111;
  event.touchPoints[0].isPressed = true;
  event.touchPoints[1].id = 1;
  event.touchPoints[1].screenX = 300.0f;
  event.touchPoints[1].screenY = 600.0f;
  event.touchPoints[1].x = 250.0f;
  event.touchPoints[1].y = 350.0f;
  event.touchPoints[1].type = OH_NATIVEXCOMPONENT_DOWN;
  event.touchPoints[1].size = 2.0;
  event.touchPoints[1].force = 0.8f;
  event.touchPoints[1].timeStamp = 222;
  event.touchPoints[1].isPressed = true;
  return event;
}

}  // namespace

TEST(OhosTouchProcessorTest, ScaleTouchEventCoordinatesIdentityAtUnitScale) {
  const OH_NativeXComponent_TouchEvent event = MakeScalingTestTouchEvent();
  const OH_NativeXComponent_TouchEvent scaled =
      OhosTouchProcessor::ScaleTouchEventCoordinates(event, 1.0);
  EXPECT_EQ(scaled.id, event.id);
  EXPECT_EQ(scaled.screenX, event.screenX);
  EXPECT_EQ(scaled.screenY, event.screenY);
  EXPECT_EQ(scaled.x, event.x);
  EXPECT_EQ(scaled.y, event.y);
  EXPECT_EQ(scaled.type, event.type);
  EXPECT_EQ(scaled.size, event.size);
  EXPECT_EQ(scaled.force, event.force);
  EXPECT_EQ(scaled.deviceId, event.deviceId);
  EXPECT_EQ(scaled.timeStamp, event.timeStamp);
  EXPECT_EQ(scaled.numPoints, event.numPoints);
  for (uint32_t i = 0; i < scaled.numPoints; ++i) {
    EXPECT_EQ(scaled.touchPoints[i].id, event.touchPoints[i].id);
    EXPECT_EQ(scaled.touchPoints[i].screenX, event.touchPoints[i].screenX);
    EXPECT_EQ(scaled.touchPoints[i].screenY, event.touchPoints[i].screenY);
    EXPECT_EQ(scaled.touchPoints[i].x, event.touchPoints[i].x);
    EXPECT_EQ(scaled.touchPoints[i].y, event.touchPoints[i].y);
    EXPECT_EQ(scaled.touchPoints[i].type, event.touchPoints[i].type);
    EXPECT_EQ(scaled.touchPoints[i].size, event.touchPoints[i].size);
    EXPECT_EQ(scaled.touchPoints[i].force, event.touchPoints[i].force);
    EXPECT_EQ(scaled.touchPoints[i].timeStamp, event.touchPoints[i].timeStamp);
    EXPECT_EQ(scaled.touchPoints[i].isPressed, event.touchPoints[i].isPressed);
  }
}

TEST(OhosTouchProcessorTest, ScaleTouchEventCoordinatesScalesCoordinatesOnly) {
  const OH_NativeXComponent_TouchEvent event = MakeScalingTestTouchEvent();
  // system_density 3.25, custom flutter_dpr 3.181 (matches the on-device
  // repro of the click-drift bug).
  const double scale = 3.25 / 3.181;
  const OH_NativeXComponent_TouchEvent scaled =
      OhosTouchProcessor::ScaleTouchEventCoordinates(event, scale);

  EXPECT_NEAR(scaled.screenX, event.screenX * scale, 1e-3);
  EXPECT_NEAR(scaled.screenY, event.screenY * scale, 1e-3);
  EXPECT_NEAR(scaled.x, event.x * scale, 1e-3);
  EXPECT_NEAR(scaled.y, event.y * scale, 1e-3);
  EXPECT_NEAR(scaled.touchPoints[0].screenX,
              event.touchPoints[0].screenX * scale, 1e-3);
  EXPECT_NEAR(scaled.touchPoints[0].screenY,
              event.touchPoints[0].screenY * scale, 1e-3);
  EXPECT_NEAR(scaled.touchPoints[0].x, event.touchPoints[0].x * scale, 1e-3);
  EXPECT_NEAR(scaled.touchPoints[0].y, event.touchPoints[0].y * scale, 1e-3);
  EXPECT_NEAR(scaled.touchPoints[1].screenX,
              event.touchPoints[1].screenX * scale, 1e-3);
  EXPECT_NEAR(scaled.touchPoints[1].screenY,
              event.touchPoints[1].screenY * scale, 1e-3);
  EXPECT_NEAR(scaled.touchPoints[1].x, event.touchPoints[1].x * scale, 1e-3);
  EXPECT_NEAR(scaled.touchPoints[1].y, event.touchPoints[1].y * scale, 1e-3);

  // Non-coordinate fields must be preserved.
  EXPECT_EQ(scaled.id, event.id);
  EXPECT_EQ(scaled.type, event.type);
  EXPECT_EQ(scaled.size, event.size);
  EXPECT_EQ(scaled.force, event.force);
  EXPECT_EQ(scaled.deviceId, event.deviceId);
  EXPECT_EQ(scaled.timeStamp, event.timeStamp);
  EXPECT_EQ(scaled.numPoints, event.numPoints);
  EXPECT_EQ(scaled.touchPoints[0].id, event.touchPoints[0].id);
  EXPECT_EQ(scaled.touchPoints[0].type, event.touchPoints[0].type);
  EXPECT_EQ(scaled.touchPoints[0].size, event.touchPoints[0].size);
  EXPECT_EQ(scaled.touchPoints[0].force, event.touchPoints[0].force);
  EXPECT_EQ(scaled.touchPoints[0].timeStamp, event.touchPoints[0].timeStamp);
  EXPECT_EQ(scaled.touchPoints[1].id, event.touchPoints[1].id);
  EXPECT_EQ(scaled.touchPoints[1].type, event.touchPoints[1].type);
  EXPECT_EQ(scaled.touchPoints[1].size, event.touchPoints[1].size);
  EXPECT_EQ(scaled.touchPoints[1].force, event.touchPoints[1].force);
  EXPECT_EQ(scaled.touchPoints[1].timeStamp, event.touchPoints[1].timeStamp);
}

TEST(OhosTouchProcessorTest, HandleTouchEventScalesPacketWhenCustomDpiActive) {
  GraphicStubKnobGuard knob_guard;
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);
  auto platform_view = holder->GetPlatformView();
  ASSERT_TRUE(platform_view);

  // System density 3.25, custom Flutter DPR 3.181 -> scale 3.25/3.181.
  ScopedSystemDensity density(3.25);
  platform_view->viewport_metrics_.device_pixel_ratio = 3.181;

  OhosTouchProcessor processor;
  OH_NativeXComponent_TouchEvent touchEvent = MakeScalingTestTouchEvent();

  ScopedNapiEnv env_guard;
  ScopedNapiStringRecorder recorder;
  processor.HandleTouchEvent(shell_id, nullptr, &touchEvent);

  // Serialized packet layout:
  // [0] numPoints, [1] id, [2] screenX, [3] screenY, [4] x, [5] y,
  // [6] type, ... then 10 fields per point starting at [11]:
  // id, screenX, screenY, x, y, type, size, force, timeStamp, isPressed.
  ASSERT_GE(recorder.size(), 26u);
  EXPECT_EQ(recorder.at(0), std::to_string(2u));
  const double scale = 3.25 / 3.181;
  EXPECT_NEAR(std::stod(recorder.at(2)), 164.0 * scale, 1e-3);
  EXPECT_NEAR(std::stod(recorder.at(3)), 590.0 * scale, 1e-3);
  EXPECT_NEAR(std::stod(recorder.at(4)), 100.0 * scale, 1e-3);
  EXPECT_NEAR(std::stod(recorder.at(5)), 200.0 * scale, 1e-3);
  // First touch point coordinates start at [12].
  EXPECT_NEAR(std::stod(recorder.at(12)), 164.0 * scale, 1e-3);
  EXPECT_NEAR(std::stod(recorder.at(13)), 590.0 * scale, 1e-3);
  EXPECT_NEAR(std::stod(recorder.at(14)), 100.0 * scale, 1e-3);
  EXPECT_NEAR(std::stod(recorder.at(15)), 200.0 * scale, 1e-3);
  // Second touch point coordinates start at [22].
  EXPECT_NEAR(std::stod(recorder.at(22)), 300.0 * scale, 1e-3);
  EXPECT_NEAR(std::stod(recorder.at(23)), 600.0 * scale, 1e-3);
  EXPECT_NEAR(std::stod(recorder.at(24)), 250.0 * scale, 1e-3);
  EXPECT_NEAR(std::stod(recorder.at(25)), 350.0 * scale, 1e-3);
  // Non-coordinate fields must not change.
  EXPECT_EQ(recorder.at(1), std::to_string(7));
  EXPECT_EQ(recorder.at(6), std::to_string(OH_NATIVEXCOMPONENT_DOWN));
  EXPECT_EQ(recorder.at(10), std::to_string(1234567890));
}

TEST(OhosTouchProcessorTest,
     HandleTouchEventKeepsPacketUnchangedWhenDprsMatch) {
  GraphicStubKnobGuard knob_guard;
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);
  auto platform_view = holder->GetPlatformView();
  ASSERT_TRUE(platform_view);

  // System density and Flutter DPR both 3.25 -> identity scale.
  ScopedSystemDensity density(3.25);
  platform_view->viewport_metrics_.device_pixel_ratio = 3.25;

  OhosTouchProcessor processor;
  OH_NativeXComponent_TouchEvent touchEvent = MakeScalingTestTouchEvent();

  ScopedNapiEnv env_guard;
  ScopedNapiStringRecorder recorder;
  processor.HandleTouchEvent(shell_id, nullptr, &touchEvent);

  ASSERT_GE(recorder.size(), 26u);
  EXPECT_EQ(recorder.at(0), std::to_string(2u));
  // std::to_string(float) prints 6 decimals; unscaled values pass through.
  EXPECT_EQ(recorder.at(2), std::to_string(164.0f));
  EXPECT_EQ(recorder.at(3), std::to_string(590.0f));
  EXPECT_EQ(recorder.at(4), std::to_string(100.0f));
  EXPECT_EQ(recorder.at(5), std::to_string(200.0f));
  EXPECT_EQ(recorder.at(12), std::to_string(164.0f));
  EXPECT_EQ(recorder.at(22), std::to_string(300.0f));
  EXPECT_EQ(recorder.at(25), std::to_string(350.0f));
}

TEST(OhosTouchProcessorTest, HandleMouseEventScalesPacketWhenCustomDpiActive) {
  GraphicStubKnobGuard knob_guard;
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);
  auto platform_view = holder->GetPlatformView();
  ASSERT_TRUE(platform_view);

  ScopedSystemDensity density(3.25);
  platform_view->viewport_metrics_.device_pixel_ratio = 3.181;

  OhosTouchProcessor processor;
  processor.apiVersion_ = 20;  // >= 20 forwards the mouse packet to napi.

  OH_NativeXComponent_MouseEvent mouseEvent = {};
  mouseEvent.x = 50.0f;
  mouseEvent.y = 60.0f;
  mouseEvent.screenX = 100.0f;
  mouseEvent.screenY = 200.0f;
  mouseEvent.button = OH_NATIVEXCOMPONENT_NONE_BUTTON;
  mouseEvent.action = OH_NATIVEXCOMPONENT_MOUSE_MOVE;

  ScopedNapiEnv env_guard;
  ScopedNapiStringRecorder recorder;
  processor.HandleMouseEvent(shell_id, nullptr, mouseEvent, 0.0, false, 200.0,
                             200.0);

  // Serialized packet layout: [0] x, [1] y, [2] screenX, [3] screenY,
  // [4] timestamp, [5] action, [6] button.
  ASSERT_GE(recorder.size(), 7u);
  const double scale = 3.25 / 3.181;
  EXPECT_NEAR(std::stod(recorder.at(0)), 50.0 * scale, 1e-3);
  EXPECT_NEAR(std::stod(recorder.at(1)), 60.0 * scale, 1e-3);
  EXPECT_NEAR(std::stod(recorder.at(2)), 100.0 * scale, 1e-3);
  EXPECT_NEAR(std::stod(recorder.at(3)), 200.0 * scale, 1e-3);
  // Non-coordinate fields must not change.
  EXPECT_EQ(recorder.at(5), std::to_string(OH_NATIVEXCOMPONENT_MOUSE_MOVE));
}

TEST(OhosTouchProcessorTest,
     HandleScrollEventScalesAxisPacketWhenCustomDpiActive) {
  GraphicStubKnobGuard knob_guard;
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);
  auto platform_view = holder->GetPlatformView();
  ASSERT_TRUE(platform_view);

  ScopedSystemDensity density(3.25);
  platform_view->viewport_metrics_.device_pixel_ratio = 3.181;

  OhosTouchProcessor processor;
  processor.apiVersion_ = 20;  // >= 20 forwards the axis packet to napi.
  NULL_OUT_DYNAMIC_PTRS(processor)

  // Stubbed pointer coordinates: x/windowX/displayX = 100,
  // y/windowY/displayY = 200 (defaults reset by StubStateResetter).
  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);

  ScopedNapiEnv env_guard;
  ScopedNapiStringRecorder recorder;
  processor.HandleScrollEvent(shell_id, nullptr, event);

  // Serialized packet layout: [0] action, [1] x, [2] y, [3] windowX,
  // [4] windowY, [5] displayX, [6] displayY, [7] scroll delta.
  ASSERT_GE(recorder.size(), 8u);
  const double scale = 3.25 / 3.181;
  EXPECT_NEAR(std::stod(recorder.at(1)), 100.0 * scale, 1e-3);
  EXPECT_NEAR(std::stod(recorder.at(2)), 200.0 * scale, 1e-3);
  EXPECT_NEAR(std::stod(recorder.at(3)), 100.0 * scale, 1e-3);
  EXPECT_NEAR(std::stod(recorder.at(4)), 200.0 * scale, 1e-3);
  EXPECT_NEAR(std::stod(recorder.at(5)), 100.0 * scale, 1e-3);
  EXPECT_NEAR(std::stod(recorder.at(6)), 200.0 * scale, 1e-3);
}

TEST(OhosTouchProcessorTest,
     HandleScaleEventScalesAxisPacketWhenCustomDpiActive) {
  GraphicStubKnobGuard knob_guard;
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);
  auto platform_view = holder->GetPlatformView();
  ASSERT_TRUE(platform_view);

  ScopedSystemDensity density(3.25);
  platform_view->viewport_metrics_.device_pixel_ratio = 3.181;

  OhosTouchProcessor processor;
  processor.apiVersion_ = 20;  // >= 20 forwards the axis packet to napi.
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_vertical_axis_value = -2.5;  // zoom delta, not a coordinate.

  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);

  ScopedNapiEnv env_guard;
  ScopedNapiStringRecorder recorder;
  processor.HandleScaleEvent(shell_id, nullptr, event);

  // Serialized packet layout: [0] action, [1] x, [2] y, [3] windowX,
  // [4] windowY, [5] displayX, [6] displayY, [7] zoom delta.
  ASSERT_GE(recorder.size(), 8u);
  const double scale = 3.25 / 3.181;
  EXPECT_NEAR(std::stod(recorder.at(1)), 100.0 * scale, 1e-3);
  EXPECT_NEAR(std::stod(recorder.at(2)), 200.0 * scale, 1e-3);
  EXPECT_NEAR(std::stod(recorder.at(3)), 100.0 * scale, 1e-3);
  EXPECT_NEAR(std::stod(recorder.at(4)), 200.0 * scale, 1e-3);
  EXPECT_NEAR(std::stod(recorder.at(5)), 100.0 * scale, 1e-3);
  EXPECT_NEAR(std::stod(recorder.at(6)), 200.0 * scale, 1e-3);
  // The zoom delta is not a coordinate and must not be scaled.
  EXPECT_EQ(recorder.at(7), std::to_string(-2.5));
}

TEST(OhosTouchProcessorTest,
     HandlePanZooomEventScalesAxisPacketWhenCustomDpiActive) {
  GraphicStubKnobGuard knob_guard;
  std::unique_ptr<OHOSShellHolder> holder;
  int64_t shell_id = CreateShellHolderForTest(holder);
  auto platform_view = holder->GetPlatformView();
  ASSERT_TRUE(platform_view);

  ScopedSystemDensity density(3.25);
  platform_view->viewport_metrics_.device_pixel_ratio = 3.181;

  OhosTouchProcessor processor;
  processor.apiVersion_ = 20;  // >= 20 forwards the axis packet to napi.
  NULL_OUT_DYNAMIC_PTRS(processor)

  g_stub_vertical_axis_value = -2.5;  // pan/scroll delta, not a coordinate.

  auto* event = reinterpret_cast<ArkUI_UIInputEvent*>(0x1);

  ScopedNapiEnv env_guard;
  ScopedNapiStringRecorder recorder;
  processor.HandlePanZooomEvent(shell_id, nullptr, event);

  // Serialized packet layout: [0] action, [1] x, [2] y, [3] windowX,
  // [4] windowY, [5] displayX, [6] displayY, [7] pan/scroll delta.
  ASSERT_GE(recorder.size(), 8u);
  const double scale = 3.25 / 3.181;
  EXPECT_NEAR(std::stod(recorder.at(1)), 100.0 * scale, 1e-3);
  EXPECT_NEAR(std::stod(recorder.at(2)), 200.0 * scale, 1e-3);
  EXPECT_NEAR(std::stod(recorder.at(3)), 100.0 * scale, 1e-3);
  EXPECT_NEAR(std::stod(recorder.at(4)), 200.0 * scale, 1e-3);
  EXPECT_NEAR(std::stod(recorder.at(5)), 100.0 * scale, 1e-3);
  EXPECT_NEAR(std::stod(recorder.at(6)), 200.0 * scale, 1e-3);
  // The pan/scroll delta is not a coordinate and must not be scaled.
  EXPECT_EQ(recorder.at(7), std::to_string(-2.5));
}

}  // namespace testing
}  // namespace flutter
