/*
 * @License: Copyright (C) Huawei Technologies Co., Ltd. 2024. All rights reserved.
 * @Author: Huawei FSI
 */

import DartExecutor from '../dart/DartExecutor';
import MethodChannel, { MethodCallHandler, MethodResult} from '../../../plugin/common/MethodChannel';
import MethodCall from '../../../plugin/common/MethodCall';
import StandardMethodCodec from '../../../plugin/common/StandardMethodCodec';
import Log from '../../../util/Log';

export default class KerboardChannel implements MethodCallHandler{
  private static TAG = "KerboardChannel";
  private static CHANNEL_NAME = "flutter/keyboard";
  private channel: MethodChannel;
  private handler: KeyboardMethodHandler | null = null;

  onMethodCall(call: MethodCall, result: MethodResult) :void {
    if (this.handler == null) {
      Log.i(KerboardChannel.TAG, "KeyboardMethodHandler is null");
      return;
    }

    let method: string = call.method;
    switch (method) {
      case "getKeyboardState": {
        Log.i(KerboardChannel.TAG, "getKeyboardState enter");
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
    this.channel = new MethodChannel(dartExecutor, KerboardChannel.CHANNEL_NAME, StandardMethodCodec.INSTANCE);
    this.channel.setMethodCallHandler(this);
  }

  public setKeyboardMethodHandler(kerboardMessageHandler: KeyboardMethodHandler | null): void {
    this.handler = kerboardMessageHandler;
  }
}

export interface KeyboardMethodHandler {
  getKeyboardState(): Map<number, number>;
}