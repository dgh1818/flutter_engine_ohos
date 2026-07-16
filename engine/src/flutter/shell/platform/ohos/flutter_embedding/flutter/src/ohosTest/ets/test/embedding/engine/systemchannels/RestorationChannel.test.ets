/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

import { describe, expect, it } from '@ohos/hypium';
import { resourceManager } from '@kit.LocalizationKit';
import { util } from '@kit.ArkTS';
import MethodChannel, { MethodCallHandler, MethodResult } from '../../../../../../main/ets/plugin/common/MethodChannel';
import MethodCall from '../../../../../../main/ets/plugin/common/MethodCall';
import DartExecutor from '../../../../../../main/ets/embedding/engine/dart/DartExecutor';
import RestorationChannel from '../../../../../../main/ets/embedding/engine/systemchannels/RestorationChannel';
import FlutterNapi from '../../../../../../main/ets/embedding/engine/FlutterNapi';
import StandardMethodCodec from '../../../../../../main/ets/plugin/common/StandardMethodCodec';

let TAG = "RestorationChannel";

export default function RestorationChannelUnitTest() {

  describe('RestorationChannelTest', () => {
    it("RestorationChannel_getRestorationData", 0, getRestorationData);
  })
}

function getRestorationData() {
  let channel = getRestorationChannel();
  expect(channel.getRestorationData()).assertInstanceOf("Uint8Array")
}

function getRestorationChannel(): RestorationChannel {
  let flutterNapi = new FlutterNapi();
  let manager: resourceManager.ResourceManager = getContext().resourceManager;
  let dartExecutor = new DartExecutor(flutterNapi, manager);
  let rawChannel = new MethodChannel(dartExecutor, "flutter/mousecursor", StandardMethodCodec.INSTANCE);
  let restorationChannel = new RestorationChannel(rawChannel, false);
  return restorationChannel;
}

class TestMethodCallHandler implements MethodCallHandler {
  onMethodCall(call: MethodCall, result: MethodResult): void {
  }
}

/**
 * string转Uint8Array
 * @param value
 * @returns
 */
export function stringToUint8Array(str: string): Uint8Array {
  try {
    let textEncoder = new util.TextEncoder("utf-8");
    let array: Uint8Array = textEncoder.encodeInto(str);
    return array
  } catch (err) {
    return new Uint8Array()
  }
}
