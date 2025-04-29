/*
 * Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd.
 * All rights reserved. Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE_KHZG file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_OHOS_TOUCH_PROCESSOR_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_OHOS_TOUCH_PROCESSOR_H_
#include <ace/xcomponent/native_interface_xcomponent.h>
#include <string>
#include <vector>
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