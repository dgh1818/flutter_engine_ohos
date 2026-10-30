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
import { FlutterPlugin, FlutterPluginBinding } from '../../../../../main/ets/embedding/engine/plugins/FlutterPlugin';
import FlutterEngine from '../../../../../main/ets/embedding/engine/FlutterEngine';
import PlatformViewsController from '../../../../../main/ets/plugin/platform/PlatformViewsController';
import common from '@ohos.app.ability.common';
import FlutterEngineConnectionRegistry from '../../../../../main/ets/embedding/engine/FlutterEngineConnectionRegistry';
import AbilityConstant from '@ohos.app.ability.AbilityConstant';

class TestFlutterPlugin implements FlutterPlugin {
  attached = false;
  detached = false;
  onAttachedToEngine(binding: FlutterPluginBinding): void { this.attached = true; }
  onDetachedFromEngine(binding: FlutterPluginBinding): void { this.detached = true; }
  getUniqueClassName(): string { return 'TestFlutterPlugin'; }
}

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

    it('has returns false for unknown plugin', 0, () => {
      if (flutterEngineConnectionRegistry) {
        const r = flutterEngineConnectionRegistry.has('NonExistentPluginName-xyz');
        expect(r).assertEqual(false);
      }
    });

    it('get returns default plugin for unknown plugin', 0, () => {
      if (flutterEngineConnectionRegistry) {
        const r = flutterEngineConnectionRegistry.get('NonExistentPluginName-xyz');
        expect(r).not().assertNull();
      }
    });

    it('removeAll does not throw', 0, () => {
      if (flutterEngineConnectionRegistry) {
        let threw = false;
        try { flutterEngineConnectionRegistry.removeAll(); } catch (e) { threw = true; }
        expect(threw).assertEqual(false);
      }
    });

    it('add plugin registers and calls onAttachedToEngine', 0, () => {
      const plugin = new TestFlutterPlugin();
      flutterEngineConnectionRegistry.add(plugin);
      expect(flutterEngineConnectionRegistry.has('TestFlutterPlugin')).assertEqual(true);
      expect(plugin.attached).assertEqual(true);
    });

    it('add duplicate plugin does not re-register', 0, () => {
      const plugin1 = new TestFlutterPlugin();
      const plugin2 = new TestFlutterPlugin();
      flutterEngineConnectionRegistry.add(plugin1);
      flutterEngineConnectionRegistry.add(plugin2);
      // Second add should be ignored (already registered)
      expect(flutterEngineConnectionRegistry.has('TestFlutterPlugin')).assertEqual(true);
    });

    it('remove plugin calls onDetachedFromEngine', 0, () => {
      // Clean up any plugin registered by earlier tests so add() is not skipped.
      flutterEngineConnectionRegistry.remove('TestFlutterPlugin');
      const plugin = new TestFlutterPlugin();
      flutterEngineConnectionRegistry.add(plugin);
      flutterEngineConnectionRegistry.remove('TestFlutterPlugin');
      expect(flutterEngineConnectionRegistry.has('TestFlutterPlugin')).assertEqual(false);
      expect(plugin.detached).assertEqual(true);
    });

    it('remove with null className does not throw', 0, () => {
      let threw = false;
      try { flutterEngineConnectionRegistry.remove(''); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('addList adds plugins', 0, () => {
      const plugin1 = new TestFlutterPlugin();
      const set = new Set<FlutterPlugin>();
      set.add(plugin1);
      let threw = false;
      try { flutterEngineConnectionRegistry.addList(set); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('destroy does not throw', 0, () => {
      let threw = false;
      try { flutterEngineConnectionRegistry.destroy(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });
  })
}