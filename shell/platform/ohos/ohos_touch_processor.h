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
                        double offsetY);
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