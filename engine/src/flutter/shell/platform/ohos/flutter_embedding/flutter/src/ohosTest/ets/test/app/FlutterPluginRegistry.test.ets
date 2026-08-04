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

import { beforeAll, describe, expect, it, Level } from '@ohos/hypium';
import FlutterPluginRegistry from '../../../../main/ets/app/FlutterPluginRegistry';
import { FlutterView } from '../../../../main/ets/view/FlutterView';
import common from '@ohos.app.ability.common';

export default function FlutterPluginRegistryTest() {
  let context: common.UIAbilityContext;

  describe('FlutterPluginRegistryTest', () => {

    beforeAll(() => {
      context = getContext() as common.UIAbilityContext;
    })

    it('constructor_creates_valid_instance', Level.LEVEL0, () => {
      const registry = new FlutterPluginRegistry();
      expect(registry).not().assertNull();
    })

    it('attach_with_valid_parameters_succeeds', Level.LEVEL0, () => {
      const registry = new FlutterPluginRegistry();
      const flutterView = new FlutterView("test_view", context);
      registry.attach(flutterView, context);
      expect(registry).not().assertNull();
    })

    it('detach_after_attach_succeeds', Level.LEVEL0, () => {
      const registry = new FlutterPluginRegistry();
      const flutterView = new FlutterView("test_view_detach", context);
      registry.attach(flutterView, context);
      registry.detach();
      expect(registry).not().assertNull();
    })

    it('destroy_without_attach_succeeds', Level.LEVEL0, () => {
      const registry = new FlutterPluginRegistry();
      registry.destroy();
      expect(registry).not().assertNull();
    })

    it('destroy_after_attach_succeeds', Level.LEVEL1, () => {
      const registry = new FlutterPluginRegistry();
      const flutterView = new FlutterView("test_view_destroy_after_attach", context);
      registry.attach(flutterView, context);
      registry.destroy();
      expect(registry).not().assertNull();
    })

    it('onPreEngineRestart_after_attach_succeeds', Level.LEVEL0, () => {
      const registry = new FlutterPluginRegistry();
      const flutterView = new FlutterView("test_view_restart", context);
      registry.attach(flutterView, context);
      registry.onPreEngineRestart();
      expect(registry).not().assertNull();
    })

    it('onPreEngineRestart_without_attach_succeeds', Level.LEVEL1, () => {
      const registry = new FlutterPluginRegistry();
      registry.onPreEngineRestart();
      expect(registry).not().assertNull();
    })

    it('attach_detach_multiple_times_succeeds', Level.LEVEL1, () => {
      const registry = new FlutterPluginRegistry();
      for (let i = 0; i < 3; i++) {
        const flutterView = new FlutterView("test_view_" + i, context);
        registry.attach(flutterView, context);
        registry.detach();
      }
      expect(registry).not().assertNull();
    })

    it('lifecycle_attach_detach_destroy_succeeds', Level.LEVEL1, () => {
      const registry = new FlutterPluginRegistry();
      const flutterView = new FlutterView("test_view_lifecycle", context);
      registry.attach(flutterView, context);
      registry.detach();
      registry.destroy();
      expect(registry).not().assertNull();
    })

    it('repeated_attach_with_same_view_succeeds', Level.LEVEL1, () => {
      const registry = new FlutterPluginRegistry();
      const flutterView = new FlutterView("test_view_repeated", context);
      registry.attach(flutterView, context);
      registry.attach(flutterView, context);
      expect(registry).not().assertNull();
    })

    it('repeated_detach_succeeds', Level.LEVEL1, () => {
      const registry = new FlutterPluginRegistry();
      const flutterView = new FlutterView("test_view_repeated_detach", context);
      registry.attach(flutterView, context);
      registry.detach();
      registry.detach();
      expect(registry).not().assertNull();
    })

    it('multiple_destroy_calls_succeeds', Level.LEVEL1, () => {
      const registry = new FlutterPluginRegistry();
      registry.destroy();
      registry.destroy();
      expect(registry).not().assertNull();
    })

    it('multiple_onPreEngineRestart_calls_succeeds', Level.LEVEL1, () => {
      const registry = new FlutterPluginRegistry();
      const flutterView = new FlutterView("test_view_multiple_restart", context);
      registry.attach(flutterView, context);
      registry.onPreEngineRestart();
      registry.onPreEngineRestart();
      expect(registry).not().assertNull();
    })

    it('attach_after_detach_succeeds', Level.LEVEL1, () => {
      const registry = new FlutterPluginRegistry();
      const flutterView1 = new FlutterView("test_view_1", context);
      const flutterView2 = new FlutterView("test_view_2", context);
      registry.attach(flutterView1, context);
      registry.detach();
      registry.attach(flutterView2, context);
      expect(registry).not().assertNull();
    })
  })
}