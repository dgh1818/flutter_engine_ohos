 /*
 * Copyright (C) Huawei Technologies Co., Ltd. 2024.
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

import DartExecutor from '../dart/DartExecutor';
import MethodChannel, { MethodCallHandler, MethodResult} from '../../../plugin/common/MethodChannel';
import MethodCall from '../../../plugin/common/MethodCall';
import StandardMethodCodec from '../../../plugin/common/StandardMethodCodec';
import Log from '../../../util/Log';

export default class KeyboardChannel implements MethodCallHandler{
  private static TAG = "KeyboardChannel";
  private static CHANNEL_NAME = "flutter/keyboard";
  private channel: MethodChannel;
  private handler: KeyboardMethodHandler | null = null;

  onMethodCall(call: MethodCall, result: MethodResult): void {
    if (this.handler == null) {
      Log.i(KeyboardChannel.TAG, "KeyboardMethodHandler is null");
      return;
    }

    let method: string = call.method;
    switch (method) {
      case "getKeyboardState": {
        Log.i(KeyboardChannel.TAG, "getKeyboardState enter");
        result.success(this.handler?.getKeyboardState());
        break;
      }
      default: {
        result.notImplemented();
        break;
      }
    }
  }

  constructor(dartExecutor: DartExecutor) {
    this.channel = new MethodChannel(dartExecutor, KeyboardChannel.CHANNEL_NAME, StandardMethodCodec.INSTANCE);
    this.channel.setMethodCallHandler(this);
  }

  public setKeyboardMethodHandler(keyboardMessageHandler: KeyboardMethodHandler | null): void {
    this.handler = keyboardMessageHandler;
  }
}

export interface KeyboardMethodHandler {
  getKeyboardState(): Map<number, number>;
}