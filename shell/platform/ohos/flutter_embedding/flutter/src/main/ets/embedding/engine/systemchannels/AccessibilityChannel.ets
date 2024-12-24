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

import Log from '../../../util/Log';
import DartExecutor from '../dart/DartExecutor';
import BasicMessageChannel, { MessageHandler, Reply } from '../../../plugin/common/BasicMessageChannel';
import HashMap from '@ohos.util.HashMap';
import FlutterNapi, { AccessibilityDelegate } from '../FlutterNapi';
import StandardMessageCodec from '../../../plugin/common/StandardMessageCodec';
import StringUtils from '../../../util/StringUtils';
import Any from '../../../plugin/common/Any';
import flutter from 'libflutter.so';
import { ByteBuffer } from '../../../util/ByteBuffer';

/**
 * 辅助功能channel
 */
export default class AccessibilityChannel implements MessageHandler<object> {
  private static TAG = "AccessibilityChannel";
  private static CHANNEL_NAME = "flutter/accessibility";
  private channel: BasicMessageChannel<object>;
  private flutterNapi: FlutterNapi;
  private handler: AccessibilityMessageHandler = new DefaultHandler();
  private nextReplyId: number = 1;

  onMessage(message: object, reply: Reply<object>): void {
    if (this.handler == null) {
      Log.i(AccessibilityChannel.TAG, "handler == NULL");
      reply.reply(StringUtils.stringToArrayBuffer(""));
      return;
    }
    let annotatedEvent: HashMap<string, Any> = message as HashMap<string, Any>;
    let type: string = annotatedEvent.get("type") as string;
    let data: HashMap<string, Any> = annotatedEvent.get("data") as HashMap<string, Any>;

    Log.i(AccessibilityChannel.TAG, "Received " + type + " message.");
    switch (type) {
      case "announce": {
        Log.i(AccessibilityChannel.TAG, "Announce");
        let announceMessage: string = data.get("message");
        if (announceMessage != null) {
          Log.i(AccessibilityChannel.TAG, "message is " + announceMessage);
          this.handler.announce(announceMessage);
        }
        break;
      }
      case "tap": {
        Log.i(AccessibilityChannel.TAG, "Tag");
        let nodeId: number = annotatedEvent.get("nodeId");
        if (nodeId != null) {
          this.handler.onTap(nodeId);
        }
        break;
      }
      case "longPress": {
        Log.i(AccessibilityChannel.TAG, "LongPress");
        let nodeId: number = annotatedEvent.get("nodeId");
        if (nodeId != null) {
          this.handler.onLongPress(nodeId);
        }
        break;
      }
      case "tooltip": {
        Log.i(AccessibilityChannel.TAG, "ToolTip");
        let tooltipMessage: string = data.get("message");
        if (tooltipMessage != null) {
          this.handler.onTooltip(tooltipMessage);
        }
        break;
      }
    }
    reply.reply(StringUtils.stringToArrayBuffer(""));
  }

  constructor(dartExecutor: DartExecutor, flutterNapi: FlutterNapi) {
    Log.i(AccessibilityChannel.TAG, "Channel entered");
    this.channel =
      new BasicMessageChannel<object>(dartExecutor, AccessibilityChannel.CHANNEL_NAME, StandardMessageCodec.INSTANCE);
    this.channel.setMessageHandler(this);
    this.flutterNapi = flutterNapi;
  }

  onOhosAccessibilityEnabled(): void {
    let replyId: number = this.nextReplyId++;
    this.flutterNapi.setSemanticsEnabledWithRespId(true, replyId);
    Log.i(AccessibilityChannel.TAG, "onOhosAccessibilityEnabled = true");
  }

  onOhosAccessibilityFeatures(accessibilityFeatureFlags: number): void {
    let replyId: number = this.nextReplyId++;
    this.flutterNapi.setAccessibilityFeatures(accessibilityFeatureFlags, replyId);
    Log.i(AccessibilityChannel.TAG, "onOhosAccessibilityFeatures");
  }

  dispatchSemanticsAction(virtualViewId: number, action: Action): void {
    let replyId: number = this.nextReplyId++;
    this.flutterNapi.dispatchSemanticsAction(virtualViewId, action, replyId);
    Log.i(AccessibilityChannel.TAG, "dispatchSemanticsAction");
  }

  setAccessibilityMessageHandler(handler: AccessibilityMessageHandler): void {
    this.handler = handler;
    let replyId: number = this.nextReplyId++;
    this.flutterNapi.setAccessibilityDelegate(handler, replyId);
  }
}

export interface AccessibilityMessageHandler extends AccessibilityDelegate {
  announce(message: string): void;

  onTap(nodeId: number): void;

  onLongPress(nodeId: number): void;

  onTooltip(nodeId: string): void;
}

export class DefaultHandler implements AccessibilityMessageHandler {
  private static TAG = "AccessibilityMessageHandler";

  announce(message: string): void {
    Log.i(DefaultHandler.TAG, "handler announce.");
    flutter.nativeAnnounce(message);
  }

  onTap(nodeId: number): void {
    Log.i(DefaultHandler.TAG, "handler onTap.");
  }

  onLongPress(nodeId: number): void {
    Log.i(DefaultHandler.TAG, "handler onLongPress.");
  }

  onTooltip(nodeId: string): void {
    Log.i(DefaultHandler.TAG, "handler onTooltip.");
  }

  updateSemantics(buffer: ByteBuffer, strings: string[], stringAttributeArgs: ByteBuffer[]): void {
    Log.i(DefaultHandler.TAG, "handler updateSemantics");
  }

  updateCustomAccessibilityActions(buffer: ByteBuffer, strings: string[]): void {
    Log.i(DefaultHandler.TAG, "handler updateCustomAccessibilityActions");
  }

  accessibilityStateChange(state: Boolean): void {
    Log.i(DefaultHandler.TAG, "handler accessibilityStateChange");
  }
}

export enum Action {
  TAP = 1 << 0,
  LONG_PRESS = 1 << 1,
  SCROLL_LEFT = 1 << 2,
  SCROLL_RIGHT = 1 << 3,
  SCROLL_UP = 1 << 4,
  SCROLL_DOWN = 1 << 5,
  INCREASE = 1 << 6,
  DECREASE = 1 << 7,
  SHOW_ON_SCREEN = 1 << 8,
  MOVE_CURSOR_FORWARD_BY_CHARACTER = 1 << 9,
  MOVE_CURSOR_BACKWARD_BY_CHARACTER = 1 << 10,
  SET_SELECTION = 1 << 11,
  COPY = 1 << 12,
  CUT = 1 << 13,
  PASTE = 1 << 14,
  DID_GAIN_ACCESSIBILITY_FOCUS = 1 << 15,
  DID_LOSE_ACCESSIBILITY_FOCUS = 1 << 16,
  CUSTOM_ACTION = 1 << 17,
  DISMISS = 1 << 18,
  MOVE_CURSOR_FORWARD_BY_WORD = 1 << 19,
  MOVE_CURSOR_BACKWARD_BY_WORD = 1 << 20,
  SET_NEXT = 1 << 21,
}