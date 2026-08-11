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
import { beforeEach, describe, expect, it } from '@ohos/hypium';
import TextInputChannel from '../../../../../main/ets/embedding/engine/systemchannels/TextInputChannel';
import TextInputPlugin from '../../../../../main/ets/plugin/editing/TextInputPlugin';
import TextInputMethodHandlerImpl from '../../../../../main/ets/plugin/editing/TextInputPlugin';
import common from '@ohos.app.ability.common';
import DartExecutor from '../../../../../main/ets/embedding/engine/dart/DartExecutor';
import FlutterInjector from '../../../../../main/ets/FlutterInjector';

export default function TextInputPluginTest() {
  describe('TextInputPluginTest', () => {
    let textInputPlugin: TextInputPlugin;
    let textInputChannelMock: TextInputChannel;
    let textInputMethodHandlerImpl: TextInputMethodHandlerImpl;
    let dartExecutor: DartExecutor;
    beforeEach(() => {
      let context: common.Context = getContext();
      const assetManager = context.resourceManager;
      const flutterNapi = FlutterInjector.getInstance().getFlutterNapi();
      dartExecutor = new DartExecutor(flutterNapi, assetManager);
      textInputChannelMock = new TextInputChannel(dartExecutor);
      textInputPlugin = new TextInputPlugin(textInputChannelMock, "oh_flutter_1", undefined);
      textInputMethodHandlerImpl = new TextInputMethodHandlerImpl(textInputChannelMock, "oh_flutter_1", undefined);
    })
    it('should initialize correctly', 0, ()=> {
      expect(textInputMethodHandlerImpl).not().assertNull();
    })
    it('Test clearTextInputClient', 0, () => {
      textInputPlugin.clearTextInputClient();
      expect(textInputPlugin).not().assertNull();
    })
  })
}