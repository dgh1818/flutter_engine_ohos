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
import { beforeAll, describe, expect, it } from '@ohos/hypium';
import { FlutterPlugin, } from '../../../../../main/ets/embedding/engine/plugins/FlutterPlugin';
import FlutterEngine from '../../../../../main/ets/embedding/engine/FlutterEngine';
import PlatformViewsController from '../../../../../main/ets/plugin/platform/PlatformViewsController';
import common from '@ohos.app.ability.common';
import FlutterEngineConnectionRegistry from '../../../../../main/ets/embedding/engine/FlutterEngineConnectionRegistry';
import AbilityConstant from '@ohos.app.ability.AbilityConstant';

export default function FlutterEngineConnectionRegistryTest() {
  let flutterEngineConnectionRegistry: FlutterEngineConnectionRegistry;
  let flutterEngine: FlutterEngine;
  let context: common.UIAbilityContext;
  const platformViewsController = new PlatformViewsController();
  const pluginClassName = 'test';

  describe('FlutterEngineConnectionRegistryTest', () => {

    beforeAll(() => {
      context = getContext() as common.UIAbilityContext;
      flutterEngine = new FlutterEngine(context, null, null, platformViewsController);
      const flutterLoader = flutterEngine.getFlutterLoader();
      flutterEngineConnectionRegistry = new FlutterEngineConnectionRegistry(context, flutterEngine, flutterLoader);
    })

    it('has', 0, async (done: Function) => {
      console.info("uitest: has begin");
      if (flutterEngineConnectionRegistry) {
        const hasFlag = flutterEngineConnectionRegistry.has(pluginClassName);
        expect(hasFlag).not().assertNull();
      }
      done();
    })

    it('get', 0, async (done: Function) => {
      console.info("uitest: get begin");
      if (flutterEngineConnectionRegistry) {
        const name: FlutterPlugin = flutterEngineConnectionRegistry.get(pluginClassName);
        expect(name).not().assertNull();
      }
      done();
    })

    it('onSaveState', 0, async (done: Function) => {
      console.info("uitest: onSaveState begin");
      const reason = AbilityConstant.StateType.CONTINUATION;
      const wantParam: Record<string, object> = {};
      if (flutterEngineConnectionRegistry) {
        const result: AbilityConstant.OnSaveResult = flutterEngineConnectionRegistry.onSaveState(reason, wantParam);
        expect(result).not().assertNull();
      }
      done();
    })
  })
}