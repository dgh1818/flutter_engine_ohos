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
import { abilityDelegatorRegistry } from '@kit.TestKit';
import { Want } from '@kit.AbilityKit';
import { window } from '@kit.ArkUI';
import AbilityConstant from '@ohos.app.ability.AbilityConstant';
import { FlutterPlugin, FlutterPluginBinding } from '../../../../../main/ets/embedding/engine/plugins/FlutterPlugin';
import { FlutterAbility } from '../../../../../main/ets/embedding/ohos/FlutterAbility';
import FlutterEngine from '../../../../../main/ets/embedding/engine/FlutterEngine';
import { FlutterView } from '../../../../../main/ets/view/FlutterView';
import FlutterManager from '../../../../../main/ets/embedding/ohos/FlutterManager';
import { FakeWindowStage } from './windowing/Fakes';

const bundleName = abilityDelegatorRegistry.getArguments().bundleName;
const want: Want = {
  bundleName: bundleName,
  abilityName: 'EntryAbility'
}

function sleep(time: number) {
  return new Promise<void>((resolve: Function) => setTimeout(resolve, time));
}

export default function FlutterAbilityTest() {
  describe('FlutterAbilityTest', () => {
    it('FlutterAbilityTest', 0, async (done: Function) => {
      const flutterAbility: FlutterAbility = new FlutterAbility();
      const flutterView: FlutterView | null = flutterAbility.getFlutterView();
      expect(flutterView).assertNull();
      await sleep(1000);
      const pagePath: string = flutterAbility.pagePath();
      expect(pagePath).assertEqual("pages/Index");
      done();
    })

    it('TestFlutterAbilityOnSaveState', 0, async (done: Function) => {
      const flutterAbility: FlutterAbility = new FlutterAbility();
      const reason: AbilityConstant.StateType = AbilityConstant.StateType.CONTINUATION
      const wantParam: Record<string, Object> = {
        "testParams": want
      }
      await sleep(1000);
      const result = flutterAbility.onSaveState(reason, wantParam);
      expect(result).not().assertNull();
      done();
    })

    it('TestGetWant', 0, async (done: Function) => {
      const flutterAbility: FlutterAbility = new FlutterAbility();
      const result = flutterAbility.getWant();
      expect(result).assertUndefined();
      done();
    })

    it('TestGetFlutterAbilityAndEntryDelegate', 0, async (done: Function) => {
      const flutterAbility: FlutterAbility = new FlutterAbility();
      const result = flutterAbility.getFlutterAbilityAndEntryDelegate();
      expect(result).assertNull();
      done();
    })

    it('TestShouldDispatchAppLifecycleState', 0, async (done: Function) => {
      const flutterAbility: FlutterAbility = new FlutterAbility();
      const result = flutterAbility.shouldDispatchAppLifecycleState();
      expect(result).assertTrue();
      done();
    })

    it('TestProvideFlutterEngine', 0, async (done: Function) => {
      const flutterAbility: FlutterAbility = new FlutterAbility();
      const result = flutterAbility.provideFlutterEngine(getContext());
      expect(result).assertNull();
      done();
    })

    it('TestProvidePlatformPlugin', 0, async (done: Function) => {
      const flutterAbility: FlutterAbility = new FlutterAbility();
      const flutterEngine = new FlutterEngine(getContext(), null, null, null);
      const result = flutterAbility.providePlatformPlugin(flutterEngine);
      expect(result).not().assertNull();
      done();
    })

    it('TestPopSystemNavigator', 0, async (done: Function) => {
      const flutterAbility: FlutterAbility = new FlutterAbility();
      const result = flutterAbility.popSystemNavigator();
      expect(result).assertFalse();
      done();
    })

    it('TestShouldAttachEngineToAbility', 0, async (done: Function) => {
      const flutterAbility: FlutterAbility = new FlutterAbility();
      const result = flutterAbility.shouldAttachEngineToAbility();
      expect(result).assertTrue();
      done();
    })

    it('TestGetDartEntrypointLibraryUri', 0, async (done: Function) => {
      const flutterAbility: FlutterAbility = new FlutterAbility();
      const result = flutterAbility.getDartEntrypointLibraryUri();
      expect(result).assertEqual("");
      done();
    })

    it('TestGetAppBundlePath', 0, async (done: Function) => {
      const flutterAbility: FlutterAbility = new FlutterAbility();
      const result = flutterAbility.getAppBundlePath();
      expect(result).assertEqual("");
      done();
    })

    it('TestAttachToEngineAutomatically', 0, async (done: Function) => {
      const flutterAbility: FlutterAbility = new FlutterAbility();
      const result = flutterAbility.attachToEngineAutomatically();
      expect(result).assertTrue();
      done();
    })

    it('TestGetExclusiveAppComponent', 0, async (done: Function) => {
      const flutterAbility: FlutterAbility = new FlutterAbility();
      const result = flutterAbility.getExclusiveAppComponent();
      expect(result).assertNull();
      done();
    })

    it('TestGetFlutterEngine', 0, async (done: Function) => {
      const flutterAbility: FlutterAbility = new FlutterAbility();
      const result = flutterAbility.getFlutterEngine();
      expect(result).assertNull();
      done();
    })

    it('TestHostedViewIdDefaultsToImplicit', 0, async (done: Function) => {
      const flutterAbility: FlutterAbility = new FlutterAbility();
      // 0 = implicit main host; only spawned Regular hosts carry a numeric id.
      expect(flutterAbility.hostedViewId).assertEqual(0);
      done();
    })

    it('TestWindowStageEventCallbackBranches', 0, (done: Function) => {
      // onWindowStageCreate registers windowStageEvent BEFORE createView throws
      // (delegate is null in the UT process), so a fake stage captures the real
      // callback and we can drive ACTIVE / INACTIVE / SHOWN to cover all three
      // activated branches of windowStageEventCallback.
      const flutterAbility: FlutterAbility = new FlutterAbility();
      const stage = new FakeWindowStage();
      try {
        flutterAbility.onWindowStageCreate(stage as ESObject as window.WindowStage);
      } catch (e) {
      }
      // ACTIVE → activated=true (native call guarded by try/catch).
      stage.windowStageEventCb?.(window.WindowStageEventType.ACTIVE);
      // INACTIVE → activated=false.
      stage.windowStageEventCb?.(window.WindowStageEventType.INACTIVE);
      // SHOWN → activated stays null → notification skipped.
      stage.windowStageEventCb?.(window.WindowStageEventType.SHOWN);
      FlutterManager.getInstance().popWindowStage(flutterAbility);
      done();
    })

    it('TestOnDestroyWithHostedViewId', 0, (done: Function) => {
      // Numeric hostedViewId: OS-close notification path runs; no registered
      // window host → removeHost + deleteViewAbility drop the per-view maps.
      const flutterAbility: FlutterAbility = new FlutterAbility();
      flutterAbility.hostedViewId = 9401;
      const cachedWant: Want = {
        bundleName: bundleName,
        abilityName: 'EntryAbility',
        parameters: {
          // ArkTS: no computed property names — mirror the literal constant
          // FlutterAbilityLaunchConfigs.EXTRA_CACHED_ENGINE_ID ("cached_engine_id").
          'cached_engine_id': 'cached-engine'
        }
      };
      // launchWant lives on UIAbility; injecting cached-engine params makes
      // shouldDestroyEngineWithHost() short-circuit without the private
      // delegate, so onDestroy reaches the destroy path instead of throwing.
      (flutterAbility as ESObject).launchWant = cachedWant;
      FlutterManager.getInstance().setViewAbility('9401', flutterAbility);
      try {
        flutterAbility.onDestroy();
      } catch (e) {
      }
      expect(FlutterManager.getInstance().getViewAbility('9401')).assertUndefined();
      expect(FlutterManager.getInstance().getWindowHostOf('9401')).assertUndefined();
      done();
    })

    it('TestOnDestroyImplicitHost', 0, (done: Function) => {
      // hostedViewId stays 0 (implicit main view): OS-close block skipped.
      const flutterAbility: FlutterAbility = new FlutterAbility();
      const cachedWant: Want = {
        bundleName: bundleName,
        abilityName: 'EntryAbility',
        parameters: {
          // ArkTS: no computed property names — mirror the literal constant
          // FlutterAbilityLaunchConfigs.EXTRA_CACHED_ENGINE_ID ("cached_engine_id").
          'cached_engine_id': 'cached-engine'
        }
      };
      (flutterAbility as ESObject).launchWant = cachedWant;
      try {
        flutterAbility.onDestroy();
      } catch (e) {
      }
      done();
    })
  })

}

class TestFlutterPlugin implements FlutterPlugin {
  getUniqueClassName(): string {
    return "engine test";
  }

  onAttachedToEngine(binding: FlutterPluginBinding): void {
    console.info("uitest: onAttachedToEngine");
  }

  onDetachedFromEngine(binding: FlutterPluginBinding): void {
    console.info("uitest: onDetachedFromEngine");
  }
}

let plugin: FlutterPlugin = new TestFlutterPlugin();
