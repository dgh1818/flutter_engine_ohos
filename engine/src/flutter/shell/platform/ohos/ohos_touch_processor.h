/*
 * Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd.
 * All rights reserved. Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE_KHZG file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_OHOS_TOUCH_PROCESSOR_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_OHOS_TOUCH_PROCESSOR_H_
#include <ace/xcomponent/native_interface_xcomponent.h>
#include <arkui/ui_input_event.h>
#include <string>
#include <vector>
#include "flutter/fml/platform/ohos/dynamic_library_loader.h"
#include "flutter/lib/ui/window/pointer_data.h"
#include "napi_common.h"

namespace flutter {

class OhosTouchProcessor {
 public:
  typedef struct {
    OH_NativeXComponent_TouchEvent* touchEventInput;
    OH_NativeXComponent_TouchPointToolType toolTypeInput;
    float tiltX;
    float tiltY;
  } TouchPacket;

 public:
  void HandleTouchEvent(int64_t shell_holderID,
                        OH_NativeXComponent* component,
                        OH_NativeXComponent_TouchEvent* touchEvent);
  void HandleAxisEvent(int64_t shell_holderID,
                       OH_NativeXComponent* component,
                       ArkUI_UIInputEvent* event);
  void HandleScaleEvent(int64_t shell_holderID,
                        OH_NativeXComponent* component,
                        ArkUI_UIInputEvent* event);
  void HandleScrollEvent(int64_t shell_holderID,
                         OH_NativeXComponent* component,
                         ArkUI_UIInputEvent* event);
  void HandlePanZooomEvent(int64_t shell_holderID,
                           OH_NativeXComponent* component,
                           ArkUI_UIInputEvent* event);
  void HandleMouseEvent(int64_t shell_holderID,
                        OH_NativeXComponent* component,
                        OH_NativeXComponent_MouseEvent mouseEvent,
                        double offsetY,
                        bool isLeave = false);
  void HandleVirtualTouchEvent(int64_t shell_holderID,
                               OH_NativeXComponent* component,
                               OH_NativeXComponent_TouchEvent* touchEvent);
  flutter::PointerData::Change getPointerChangeForAction(int maskedAction);
  flutter::PointerData::DeviceKind getPointerDeviceTypeForToolType(
      int toolType);
  flutter::PointerData::Change getPointerChangeForMouseAction(
      OH_NativeXComponent_MouseEventAction mouseAction);
  PointerButtonMouse getPointerButtonFromMouse(
      OH_NativeXComponent_MouseEventButton mouseButton);

 public:
  OH_NativeXComponent_TouchPointToolType touchType_;

 public:
  OhosTouchProcessor();
  ~OhosTouchProcessor();

 private:
  float accumulatedPanX_ = 0.0;
  float accumulatedPanY_ = 0.0;
  float accumulatedScale_ = 1.0;

 private:
  int apiVersion_;
  std::unique_ptr<DynamicLibraryLoader> loader_;
  // 共享库名称
  static constexpr char UI_INPUT_EVENT_LIB_NAME[] = "libace_ndk.z.so";
  // 动态加载的函数指针
  int32_t (*dynamicGetDeviceId_)(ArkUI_UIInputEvent*);
  int32_t (*dynamicGetAxisAction_)(ArkUI_UIInputEvent*);
  int32_t (*dynamicGetModifierKeyStates_)(ArkUI_UIInputEvent*, uint64_t*);

 private:
  std::shared_ptr<std::string[]> packagePacketData(
      std::unique_ptr<OhosTouchProcessor::TouchPacket> touchPacket);

  void PlatformViewOnTouchEvent(int64_t shellHolderID,
                                OH_NativeXComponent_TouchPointToolType toolType,
                                OH_NativeXComponent* component,
                                OH_NativeXComponent_TouchEvent* touchEvent);
};
}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_OHOS_TOUCH_PROCESSOR_H_