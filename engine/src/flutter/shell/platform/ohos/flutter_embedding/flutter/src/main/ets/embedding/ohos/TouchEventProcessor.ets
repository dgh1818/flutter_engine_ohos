/*
* Copyright (c) 2024 Hunan OpenValley Digital Industry Development Co., Ltd.
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
/** Handle the motion events received by the FlutterNapi. */
// import PlainArray from '@ohos.util.PlainArray';
// import { TouchEvent } from '@ohos.multimodalInput.touchEvent';
// import Queue from '@ohos.util.Queue';

import { CustomTouchEvent, CustomTouchObject } from '../../plugin/platform/CustomTouchEvent';
import display from '@ohos.display';
import { FlutterManager } from './FlutterManager';
import { EmbeddingNodeController } from './EmbeddingNodeController';
import Any from '../../plugin/common/Any';

const OH_NATIVEXCOMPONENT_UNKNOWN = 4;
const OH_NATIVEXCOMPONENT_TOOL_TYPE_UNKNOWN = 0;

class OH_NativeXComponent_TouchPoint {
  id: number = 0;
  screenX: number = 0.0;
  screenY: number = 0.0;
  x: number = 0.0;
  y: number = 0.0;
  type: number = OH_NATIVEXCOMPONENT_UNKNOWN;
  size: number = 0;
  force: number = 0;
  timeStamp: number = 0;
  isPressed: boolean = false;

  constructor(id: number,
              screenX: number,
              screenY: number,
              x: number,
              y: number,
              type: number,
              size: number,
              force: number,
              timeStamp: number,
              isPressed: boolean) {
    this.id = id;
    this.screenX = screenX;
    this.screenY = screenY;
    this.x = x;
    this.y = y;
    this.type = type;
    this.size = size;
    this.force = force;
    this.timeStamp = timeStamp;
    this.isPressed = isPressed;
  }
}

class OH_NativeXComponent_TouchEvent {
  id: number = 0;
  screenX: number = 0.0;
  screenY: number = 0.0;
  x: number = 0.0;
  y: number = 0.0;
  type: number = OH_NATIVEXCOMPONENT_UNKNOWN;
  size: number = 0;
  force: number = 0;
  deviceId: number = 0;
  timeStamp: number = 0;
  touchPoints: OH_NativeXComponent_TouchPoint[] = [];
  numPoints: number = 0;

  constructor(id: number,
              screenX: number,
              screenY: number,
              x: number,
              y: number,
              type: number,
              size: number,
              force: number,
              deviceId: number,
              timeStamp: number,
              touchPoints: OH_NativeXComponent_TouchPoint[],
              numPoints: number) {
    this.id = id;
    this.screenX = screenX;
    this.screenY = screenY;
    this.x = x;
    this.y = y;
    this.type = type;
    this.size = size;
    this.force = force;
    this.deviceId = deviceId;
    this.timeStamp = timeStamp;
    this.touchPoints = touchPoints;
    this.numPoints = numPoints;
  }
}

class TouchPacket {
  touchEvent: OH_NativeXComponent_TouchEvent;
  toolType: number = OH_NATIVEXCOMPONENT_TOOL_TYPE_UNKNOWN;
  tiltX: number = 0;
  tiltY: number = 0;

  constructor(touchEvent: OH_NativeXComponent_TouchEvent,
              toolType: number,
              tiltX: number,
              tiltY: number) {
    this.touchEvent = touchEvent;
    this.toolType = toolType;
    this.tiltX = tiltX;
    this.tiltY = tiltY;
  }
}

export default class TouchEventProcessor {
  private static instance: TouchEventProcessor;

  static getInstance(): TouchEventProcessor {
    if (TouchEventProcessor.instance == null) {
      TouchEventProcessor.instance = new TouchEventProcessor();
    }
    return TouchEventProcessor.instance;
  }

  private decodeTouchPacket(strings: Array<string>, densityPixels: number, top: number, left: number): TouchPacket {
    let offset: number = 0;
    let numPoint: number = parseInt(strings[offset++]);
    let changesId: number = parseInt(strings[offset++]);
    let changesscreenX: number = (parseFloat(strings[offset++]) / densityPixels);
    let changesscreenY: number = (parseFloat(strings[offset++]) / densityPixels);
    let changesX: number = ((parseFloat(strings[offset++]) / densityPixels) - left);
    let changesY: number = ((parseFloat(strings[offset++]) / densityPixels) - top);
    let changesType: number = parseInt(strings[offset++]);
    let changesSize: number = parseFloat(strings[offset++]);
    let changesForce: number = parseFloat(strings[offset++]);
    let changesDeviceId: number = parseInt(strings[offset++]);
    let changesTimeStamp: number = parseInt(strings[offset++]);

    const touchPoints: OH_NativeXComponent_TouchPoint[] = [];
    for (let i = 0; i < numPoint; i++) {
      const touchPoint: OH_NativeXComponent_TouchPoint = new OH_NativeXComponent_TouchPoint(
        parseInt(strings[offset++]),
        (parseFloat(strings[offset++]) / densityPixels),
        (parseFloat(strings[offset++]) / densityPixels),
        ((parseFloat(strings[offset++]) / densityPixels) - left),
        ((parseFloat(strings[offset++]) / densityPixels) - top),
        parseInt(strings[offset++]),
        parseFloat(strings[offset++]),
        parseFloat(strings[offset++]),
        parseInt(strings[offset++]),
        parseInt(strings[offset++]) === 1 ? true : false
      );
      touchPoints.push(touchPoint);
    }

    const touchEventInput: OH_NativeXComponent_TouchEvent = new OH_NativeXComponent_TouchEvent(
      changesId,
      changesscreenX,
      changesscreenY,
      changesX,
      changesY,
      changesType,
      changesSize,
      changesForce,
      changesDeviceId,
      changesTimeStamp,
      touchPoints,
      numPoint
    );

    let toolTypeInput: number = parseInt(strings[offset++]);
    let tiltXTouch: number = parseInt(strings[offset++]);
    let tiltYTouch: number = parseInt(strings[offset++]);

    const touchpointeventpacket: TouchPacket = new TouchPacket(
      touchEventInput,
      toolTypeInput,
      tiltXTouch,
      tiltYTouch
    );
    return touchpointeventpacket;
  }

  private constureCustomTouchEventImpl(touchPacket: TouchPacket): CustomTouchEvent {
    let changes1: CustomTouchObject = new CustomTouchObject(
      touchPacket.touchEvent.type,
      touchPacket.touchEvent.id,
      touchPacket.touchEvent.screenX,
      touchPacket.touchEvent.screenY,
      touchPacket.touchEvent.screenX,
      touchPacket.touchEvent.screenY,
      touchPacket.touchEvent.screenX,
      touchPacket.touchEvent.screenY,
      touchPacket.touchEvent.x,
      touchPacket.touchEvent.y
    );

    let touches: CustomTouchObject[] = [];
    let touchPointer:number = touchPacket.touchEvent.numPoints;
    for (let i = 0; i < touchPointer; i++) {
        let touchesItem: CustomTouchObject = new CustomTouchObject(
        touchPacket.touchEvent.touchPoints[i].type,
        touchPacket.touchEvent.touchPoints[i].id,
        touchPacket.touchEvent.touchPoints[i].screenX,
        touchPacket.touchEvent.touchPoints[i].screenY,
        touchPacket.touchEvent.touchPoints[i].screenX,
        touchPacket.touchEvent.touchPoints[i].screenY,
        touchPacket.touchEvent.touchPoints[i].screenX,
        touchPacket.touchEvent.touchPoints[i].screenY,
        touchPacket.touchEvent.touchPoints[i].x,
        touchPacket.touchEvent.touchPoints[i].y
      );
      touches.push(touchesItem);
    }

    let customTouchEvent1: CustomTouchEvent = new CustomTouchEvent(
        touchPacket.touchEvent.type,
        touches,
        [changes1],
        touchPacket.touchEvent.timeStamp,
        SourceType.TouchScreen,
        touchPacket.touchEvent.force,
        touchPacket.tiltX,
        touchPacket.tiltY,
        touchPacket.toolType
    );

    return customTouchEvent1;
  }

  /** Construct the CustomTouchEvent and return. */
  public constureCustomTouchEvent(strings: Array<string>, top: number, left: number) : CustomTouchEvent {
    let densityPixels:number = display.getDefaultDisplaySync().densityPixels;

    let touchPacket: TouchPacket = this.decodeTouchPacket(strings, densityPixels, top, left);
    let customTouchEvent: CustomTouchEvent = this.constureCustomTouchEventImpl(touchPacket);
    return customTouchEvent;
  }

  public postTouchEvent(strings: Array<string>) {
    FlutterManager.getInstance().getFlutterViewList().forEach((value) => {
      let length = value.getDVModel().children.length;
      for (let index = length - 1; index >= 0; index--) {
        let dvModel = value.getDVModel().children[index];
        let params = dvModel.getLayoutParams();
        let params2 = params as Record<string, Any>;
        let nodeController = params2['nodeController'] as EmbeddingNodeController;
        let left = params2['left'] as number ?? 0;
        let top = params2['top'] as number ?? 0;
        let width = params2['width'] as number ?? 0;
        let height = params2['height'] as number ?? 0;
        let customTouchEvent = TouchEventProcessor.instance.constureCustomTouchEvent(strings, top, left);
        let x = customTouchEvent.changedTouches[0].screenX;
        let y = customTouchEvent.changedTouches[0].screenY;
        if (TouchEventProcessor.instance.checkHitPlatformView(left, top, width, height, x, y)) {
          nodeController.postEvent(customTouchEvent);
          break;
        }
      }
    });
  }

  public checkHitPlatformView(left: number, top: number, width: number, height: number, x: number, y: number): boolean {
    if (x >= left && x <= (left + width) && y >= top && y <= (top + height)) {
      return true;
    } else {
      return false;
    }
  }
}
