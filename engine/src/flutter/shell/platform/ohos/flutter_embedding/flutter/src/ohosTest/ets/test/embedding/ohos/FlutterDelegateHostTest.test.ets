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
import { describe, expect, it, Level } from '@ohos/hypium';
import { abilityDelegatorRegistry } from '@kit.TestKit';
import { Want, UIAbility } from '@kit.AbilityKit';
import common from '@ohos.app.ability.common';
import { FlutterAbilityAndEntryDelegate, Host } from '../../../../../main/ets/embedding/ohos/FlutterAbilityAndEntryDelegate';
import FlutterEngine from '../../../../../main/ets/embedding/engine/FlutterEngine';
import FlutterShellArgs from '../../../../../main/ets/embedding/engine/FlutterShellArgs';
import PlatformPlugin from '../../../../../main/ets/plugin/PlatformPlugin';
import SensitiveContentPlugin from '../../../../../main/ets/plugin/view/SensitiveContentPlugin';
import ExclusiveAppComponent from '../../../../../main/ets/embedding/ohos/ExclusiveAppComponent';

const delegator = abilityDelegatorRegistry.getAbilityDelegator();
const bundleName = abilityDelegatorRegistry.getArguments().bundleName;

class FakeHost implements Host {
  private context: common.Context;
  constructor(context: common.Context) {
    this.context = context;
  }
  getAbility(): UIAbility { return this.context as ESObject as UIAbility; }
  shouldDispatchAppLifecycleState(): boolean { return true; }
  detachFromFlutterEngine(): void {}
  shouldAttachEngineToAbility(): boolean { return false; }
  getCachedEngineId(): string { return ''; }
  getCachedEngineGroupId(): string | null { return null; }
  shouldDestroyEngineWithHost(): boolean { return true; }
  getFlutterShellArgs(): FlutterShellArgs { return new FlutterShellArgs(); }
  getDartEntrypointArgs(): Array<string> { return []; }
  getDartEntrypointLibraryUri(): string { return ''; }
  getAppBundlePath(): string { return ''; }
  getDartEntrypointFunctionName(): string { return 'main'; }
  getInitialRoute(): string { return '/'; }
  getWant(): Want { return { bundleName: bundleName, abilityName: 'TestAbility' }; }
  shouldRestoreAndSaveState(): boolean { return false; }
  getExclusiveAppComponent(): ExclusiveAppComponent<UIAbility> | null { return null; }
  providePlatformPlugin(flutterEngine: FlutterEngine): PlatformPlugin | undefined { return undefined; }
  provideSensitiveContentPlugin(flutterEngine: FlutterEngine): SensitiveContentPlugin | undefined { return undefined; }
  attachToEngineAutomatically(): boolean { return false; }
  provideFlutterEngine(context: common.Context): FlutterEngine | null { return null; }
  configureFlutterEngine(flutterEngine: FlutterEngine): void {}
  cleanUpFlutterEngine(flutterEngine: FlutterEngine): void {}
  popSystemNavigator(): boolean { return false; }
}

export default function FlutterDelegateHostTest() {
  describe('FlutterDelegateHostTest', () => {

    it('FakeHost onAttach does not throw', Level.LEVEL0, async (done: Function) => {
      const context = getContext() as common.UIAbilityContext;
      const host = new FakeHost(context);
      const delegate = new FlutterAbilityAndEntryDelegate(host);
      let threw = false;
      try {
        delegate.onAttach(context);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
      done();
    });

    it('FakeHost createView does not throw', Level.LEVEL0, async (done: Function) => {
      const context = getContext() as common.UIAbilityContext;
      const host = new FakeHost(context);
      const delegate = new FlutterAbilityAndEntryDelegate(host);
      let threw = false;
      try {
        delegate.onAttach(context);
        const view = delegate.createView(context);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
      done();
    });

    it('FakeHost onDetach does not throw', Level.LEVEL0, async (done: Function) => {
      const context = getContext() as common.UIAbilityContext;
      const host = new FakeHost(context);
      const delegate = new FlutterAbilityAndEntryDelegate(host);
      let threw = false;
      try {
        delegate.onAttach(context);
        delegate.onDetach();
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
      done();
    });

    it('FakeHost shouldDispatchAppLifecycleState returns true', Level.LEVEL0, async (done: Function) => {
      const context = getContext() as common.UIAbilityContext;
      const host = new FakeHost(context);
      expect(host.shouldDispatchAppLifecycleState()).assertEqual(true);
      done();
    });

    it('FakeHost shouldAttachEngineToAbility returns false', Level.LEVEL0, async (done: Function) => {
      const context = getContext() as common.UIAbilityContext;
      const host = new FakeHost(context);
      expect(host.shouldAttachEngineToAbility()).assertEqual(false);
      done();
    });

    it('FakeHost getCachedEngineId returns empty', Level.LEVEL0, async (done: Function) => {
      const context = getContext() as common.UIAbilityContext;
      const host = new FakeHost(context);
      expect(host.getCachedEngineId()).assertEqual('');
      done();
    });

    it('FakeHost shouldDestroyEngineWithHost returns true', Level.LEVEL0, async (done: Function) => {
      const context = getContext() as common.UIAbilityContext;
      const host = new FakeHost(context);
      expect(host.shouldDestroyEngineWithHost()).assertEqual(true);
      done();
    });

    it('FakeHost getDartEntrypointFunctionName returns main', Level.LEVEL0, async (done: Function) => {
      const context = getContext() as common.UIAbilityContext;
      const host = new FakeHost(context);
      expect(host.getDartEntrypointFunctionName()).assertEqual('main');
      done();
    });

    it('FakeHost getInitialRoute returns slash', Level.LEVEL0, async (done: Function) => {
      const context = getContext() as common.UIAbilityContext;
      const host = new FakeHost(context);
      expect(host.getInitialRoute()).assertEqual('/');
      done();
    });

    it('FakeHost attachToEngineAutomatically returns false', Level.LEVEL0, async (done: Function) => {
      const context = getContext() as common.UIAbilityContext;
      const host = new FakeHost(context);
      expect(host.attachToEngineAutomatically()).assertEqual(false);
      done();
    });

    it('FakeHost provideFlutterEngine returns null', Level.LEVEL0, async (done: Function) => {
      const context = getContext() as common.UIAbilityContext;
      const host = new FakeHost(context);
      expect(host.provideFlutterEngine(context)).assertNull();
      done();
    });
  })
}
