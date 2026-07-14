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
import { describe, expect, it, } from '@ohos/hypium';
import PlatformChannel, {
  Brightness,
  ClipboardContentFormat,
  DeviceOrientation,
  HapticFeedbackType,
  SystemUiMode,
  SystemUiOverlay
} from '../../../../../../main/ets/embedding/engine/systemchannels/PlatformChannel';
import { resourceManager } from '@kit.LocalizationKit';
import FlutterNapi from '../../../../../../main/ets/embedding/engine/FlutterNapi';
import DartExecutor from '../../../../../../main/ets/embedding/engine/dart/DartExecutor';
import { MethodResult } from '../../../../../../main/ets/plugin/common/MethodChannel';
import Any from '../../../../../../main/ets/plugin/common/Any';

export default function PlatformChannelUnitTest() {
  describe('PlatformChannelTest', () => {
    it('platformChannel_decodeOrientations', 0, decodeOrientations);

    it('platformChannel_getFeedbackTypeFromValue', 0, getFeedbackTypeFromValue);

    it('platformChannel_getClipboardContentFormatFromValue', 0, getClipboardContentFormatFromValue);

    it('platformChannel_getSystemUiOverlayFromValue', 0, getSystemUiOverlayFromValue);

    it('platformChannel_getSystemUiModeFromValue', 0, getSystemUiModeFromValue);

    it('platformChannel_getBrightnessFromValue', 0, getBrightnessFromValue);

    it('platformChannel_getDeviceOrientationFromValue', 0, getDeviceOrientationFromValue);
  })
}

function decodeOrientations() {
  let channel = getPlatformChannel();
  expect(channel.decodeOrientations([DeviceOrientation.PORTRAIT_UP])).assertInstanceOf("Number")
}

function getFeedbackTypeFromValue() {
  let channel = getPlatformChannel();
  let result = channel.getFeedbackTypeFromValue(HapticFeedbackType.STANDARD);
  expect(result).assertInstanceOf("String")
}

function getClipboardContentFormatFromValue() {
  let channel = getPlatformChannel();
  let result = channel.getClipboardContentFormatFromValue(ClipboardContentFormat.PLAIN_TEXT);
  expect(result).assertInstanceOf("String")
}

function getSystemUiOverlayFromValue() {
  let channel = getPlatformChannel();
  let result = channel.getSystemUiOverlayFromValue(SystemUiOverlay.BOTTOM_OVERLAYS);
  expect(result).assertInstanceOf("String")
}

function getSystemUiModeFromValue() {
  let channel = getPlatformChannel();
  let result = channel.getSystemUiModeFromValue(SystemUiMode.IMMERSIVE);
  expect(result).assertInstanceOf("String")
}

function getBrightnessFromValue() {
  let channel = getPlatformChannel();
  let result = channel.getBrightnessFromValue(Brightness.LIGHT);
  expect(result).assertInstanceOf("String")
}

function getDeviceOrientationFromValue() {
  let channel = getPlatformChannel();
  let result = channel.getDeviceOrientationFromValue(DeviceOrientation.PORTRAIT_UP);
  expect(result).assertInstanceOf("String")
}

function getPlatformChannel(): PlatformChannel {
  let flutterNapi = new FlutterNapi();
  let manager: resourceManager.ResourceManager = getContext().resourceManager;
  let dartExecutor = new DartExecutor(flutterNapi, manager);
  let channel = new PlatformChannel(dartExecutor, flutterNapi);
  return channel;
}

export class MethodResultTest implements MethodResult {
  success = (result: Any): void => {
  };
  error = (errorCode: string, errorMessage: string, errorDetails: Any): void => {
  };
  notImplemented = (): void => {
  }
}
