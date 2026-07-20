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
import FlutterNapi from '../../../../../../main/ets/embedding/engine/FlutterNapi';
import DartExecutor from '../../../../../../main/ets/embedding/engine/dart/DartExecutor';
import TextInputChannel from '../../../../../../main/ets/embedding/engine/systemchannels/TextInputChannel';

export default function TextInputChannelUnitTest() {
  describe('TextInputChannel', () => {
    it("TextInputChannel_createEditingStateJSON", 0, createEditingStateJSON);
  })
}

function createEditingStateJSON() {
  let textInputChannel = getTextInputChannel();
  let result = textInputChannel.createEditingStateJSON("test", 0, 1, 0, 2);
  expect(result).assertInstanceOf("Object");
}

function getTextInputChannel(): TextInputChannel {
  let flutterNapi = new FlutterNapi();
  let manager: resourceManager.ResourceManager = getContext().resourceManager;
  let dartExecutor = new DartExecutor(flutterNapi, manager);
  let channel = new TextInputChannel(dartExecutor);
  return channel;
}
