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
import AbilityConstant from '@ohos.app.ability.AbilityConstant';
import { Options } from '../../../../../main/ets/embedding/engine/FlutterEngineGroup';
import { Want, ConfigurationConstant, UIAbility } from '@kit.AbilityKit';
import { FlutterAbilityAndEntryDelegate, Host } from '../../../../../main/ets/embedding/ohos/FlutterAbilityAndEntryDelegate';
import window from '@ohos.window';
import common from '@ohos.app.ability.common';
import { FlutterPlugin, FlutterPluginBinding } from '../../../../../main/ets/embedding/engine/plugins/FlutterPlugin';
import FlutterEngine from '../../../../../main/ets/embedding/engine/FlutterEngine';
import FlutterShellArgs from '../../../../../main/ets/embedding/engine/FlutterShellArgs';
import PlatformPlugin from '../../../../../main/ets/plugin/PlatformPlugin';
import SensitiveContentPlugin from '../../../../../main/ets/plugin/view/SensitiveContentPlugin';
import ExclusiveAppComponent from '../../../../../main/ets/embedding/ohos/ExclusiveAppComponent';
import ActivityControlSurface from '../../../../../main/ets/embedding/engine/plugins/ability/AbilityControlSurface';

const bundleName = abilityDelegatorRegistry.getArguments().bundleName;
const delegator = abilityDelegatorRegistry.getAbilityDelegator();

class TestPlugin implements FlutterPlugin {
  onAttachedToEngine(binding: FlutterPluginBinding): void {}
  onDetachedFromEngine(binding: FlutterPluginBinding): void {}
  getUniqueClassName(): string { return 'TestPlugin'; }
}

/**
 * Host stub that reports shouldAttachEngineToAbility() as configurable,
 * used to drive the plugin state restore branch of the delegate.
 */
class RestoreTestHost implements Host {
  private ability: UIAbility;
  private attachToAbility: boolean;
  constructor(ability: UIAbility, attachToAbility: boolean) {
    this.ability = ability;
    this.attachToAbility = attachToAbility;
  }
  getAbility(): UIAbility { return this.ability; }
  shouldDispatchAppLifecycleState(): boolean { return false; }
  detachFromFlutterEngine(): void {}
  shouldAttachEngineToAbility(): boolean { return this.attachToAbility; }
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

/**
 * Records every onRestoreInstanceState call so tests can assert what the
 * delegate distributed through onRestoreInstanceState.
 */
class SpyControlSurface implements ActivityControlSurface {
  restoreCallCount: number = 0;
  lastRestoredState: Record<string, Object> | null = null;
  attachToAbility(exclusiveActivity: ExclusiveAppComponent<UIAbility>): void {}
  detachFromAbility(): void {}
  onNewWant(want: Want, launchParams: AbilityConstant.LaunchParam): void {}
  onWindowFocusChanged(hasFocus: boolean): void {}
  onSaveState(reason: AbilityConstant.StateType, wantParam: Record<string, Object>): AbilityConstant.OnSaveResult {
    return AbilityConstant.OnSaveResult.ALL_REJECT;
  }
  onRestoreInstanceState(wantParam: Record<string, Object>): void {
    this.restoreCallCount++;
    this.lastRestoredState = wantParam;
  }
}

/**
 * Legacy ActivityControlSurface that intentionally does not implement the
 * optional onRestoreInstanceState() method, simulating old implementations
 * compiled against a previous version of the interface.
 */
class LegacyControlSurface implements ActivityControlSurface {
  attachToAbility(exclusiveActivity: ExclusiveAppComponent<UIAbility>): void {}
  detachFromAbility(): void {}
  onNewWant(want: Want, launchParams: AbilityConstant.LaunchParam): void {}
  onWindowFocusChanged(hasFocus: boolean): void {}
  onSaveState(reason: AbilityConstant.StateType, wantParam: Record<string, Object>): AbilityConstant.OnSaveResult {
    return AbilityConstant.OnSaveResult.ALL_REJECT;
  }
}

/**
 * Engine stub whose ActivityControlSurface can be replaced by any
 * ActivityControlSurface implementation (spy or legacy).
 */
class SpyEngine extends FlutterEngine {
  readonly controlSurface: ActivityControlSurface;
  constructor(context: common.Context, controlSurface: ActivityControlSurface) {
    super(context, null, null, null);
    this.controlSurface = controlSurface;
  }
  getAbilityControlSurface(): ActivityControlSurface | null {
    return this.controlSurface;
  }
}

export default function FlutterAbilityAndEntryDelegateTest() {
  describe('FlutterAbilityAndEntryDelegateTest', () => {

    it('TestAddEntrypointOptions', 0, async (done: Function) => {
      const flutterAbilityAndEntryDelegate: FlutterAbilityAndEntryDelegate = new FlutterAbilityAndEntryDelegate();
      const options: Options = new Options(getContext());
      const result = flutterAbilityAndEntryDelegate.addEntrypointOptions(options);
      expect(result).not().assertNull();
      done();
    })

    it('TestShouldDispatchAppLifecycleState', 0, async (done: Function) => {
      const flutterAbilityAndEntryDelegate: FlutterAbilityAndEntryDelegate = new FlutterAbilityAndEntryDelegate();
      const result = flutterAbilityAndEntryDelegate.shouldDispatchAppLifecycleState();
      expect(result).not().assertNull();
      done();
    })

    it('TestGetFlutterNapi', 0, async (done: Function) => {
      const flutterAbilityAndEntryDelegate: FlutterAbilityAndEntryDelegate = new FlutterAbilityAndEntryDelegate();
      const result = flutterAbilityAndEntryDelegate.getFlutterNapi();
      expect(result).assertNull();
      done();
    })

    it('TestOnSaveState', 0, async (done: Function) => {
      const flutterAbilityAndEntryDelegate: FlutterAbilityAndEntryDelegate = new FlutterAbilityAndEntryDelegate();
      const reason: AbilityConstant.StateType = AbilityConstant.StateType.CONTINUATION
      const want: Want = {
        bundleName: bundleName,
        abilityName: 'EntryAbility'
      }
      const wantParam: Record<string, Object> = {
        "testParams": want
      }
      const result = flutterAbilityAndEntryDelegate.onSaveState(reason, wantParam);
      expect(result).assertEqual(5);
      done();
    })

    it('TestIsFlutterEngineFromHost', 0, async (done: Function) => {
      const flutterAbilityAndEntryDelegate: FlutterAbilityAndEntryDelegate = new FlutterAbilityAndEntryDelegate();
      const result = flutterAbilityAndEntryDelegate.isFlutterEngineFromHost();
      expect(result).not().assertNull();
      done();
    })

    it('TestGetFlutterEngine', 0, async (done: Function) => {
      const flutterAbilityAndEntryDelegate: FlutterAbilityAndEntryDelegate = new FlutterAbilityAndEntryDelegate();
      const result = flutterAbilityAndEntryDelegate.getFlutterEngine();
      expect(result).assertNull();
      done();
    })

    it('TestDoInitialFlutterViewRunNoHost', 0, async (done: Function) => {
      // No host, no engine: entrypoint executes with DEFAULT_INITIAL_ROUTE,
      // intent path skipped (host==null), findAppBundlePath() fallback used.
      const delegate = new FlutterAbilityAndEntryDelegate();
      delegate.onWindowStageCreate();
      done();
    })

    it('TestDoInitialFlutterViewRunSkipsWhenDartRunning', 0, async (done: Function) => {
      // Warm engine: isExecutingDart() true → entrypoint skipped, split-view
      // config still sent.
      const delegate = new FlutterAbilityAndEntryDelegate();
      const engine = new FakeDelegateEngine();
      engine.dartExecutor.running = true;
      delegate.flutterEngine = engine as ESObject as FlutterEngine;
      delegate.onWindowStageCreate();
      done();
    })

    it('TestDoInitialFlutterViewRunWithHostNullRoute', 0, async (done: Function) => {
      // Host present but initialRoute null → intent fallback (''), null
      // library uri → default, null bundle path → findAppBundlePath().
      const host = new FakeDelegateHost();
      const delegate = new FlutterAbilityAndEntryDelegate(host as ESObject as Host);
      delegate.onWindowStageCreate();
      done();
    })

    it('TestDoInitialFlutterViewRunWithHostRouteAndBundle', 0, async (done: Function) => {
      // Host with explicit initialRoute + library uri + bundle override →
      // intent skipped, findAppBundlePath() skipped.
      const host = new FakeDelegateHost();
      host.initialRoute = '/home';
      host.libraryUri = 'package:foo/bar.dart';
      host.appBundlePath = '/data/app_bundle';
      const delegate = new FlutterAbilityAndEntryDelegate(host as ESObject as Host);
      delegate.onWindowStageCreate();
      done();
    })
    it('TestGetFlutterNapi returns null', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      const napi = d.getFlutterNapi();
      expect(napi).assertNull();
      done();
    });

    it('TestShouldDispatchAppLifecycleState returns boolean', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      const r = d.shouldDispatchAppLifecycleState();
      expect(typeof r).assertEqual('boolean');
      done();
    });

    it('TestOnLowMemory does not throw', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      let threw = false;
      try { d.onLowMemory(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('TestEnsureAlive does not throw', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      let threw = false;
      try { d.ensureAlive(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('TestIsFlutterEngineFromHost returns boolean', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      const r = d.isFlutterEngineFromHost();
      expect(typeof r).assertEqual('boolean');
      done();
    });

    it('TestOnShow does not throw without host', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      let threw = false;
      try { d.onShow(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('TestOnPaused does not throw without host', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      let threw = false;
      try { d.onPaused(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('TestOnResumed does not throw without host', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      let threw = false;
      try { d.onResumed(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('TestOnHide does not throw without host', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      let threw = false;
      try { d.onHide(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('TestOnWindowFocusChanged true does not throw', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      let threw = false;
      try { d.onWindowFocusChanged(true); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('TestOnWindowFocusChanged false does not throw', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      let threw = false;
      try { d.onWindowFocusChanged(false); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('TestOnWindowStageChanged SHOWN', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      let threw = false;
      try { d.onWindowStageChanged(window.WindowStageEventType.SHOWN); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('TestOnWindowStageChanged ACTIVE', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      let threw = false;
      try { d.onWindowStageChanged(window.WindowStageEventType.ACTIVE); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('TestOnWindowStageChanged INACTIVE', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      let threw = false;
      try { d.onWindowStageChanged(window.WindowStageEventType.INACTIVE); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('TestOnWindowStageChanged PAUSED', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      let threw = false;
      try { d.onWindowStageChanged(window.WindowStageEventType.PAUSED); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('TestOnWindowStageChanged RESUMED', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      let threw = false;
      try { d.onWindowStageChanged(window.WindowStageEventType.RESUMED); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('TestOnWindowStageChanged HIDDEN', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      let threw = false;
      try { d.onWindowStageChanged(window.WindowStageEventType.HIDDEN); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('TestOnRestoreInstanceState with undefined parameters', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      const want: Want = { bundleName: bundleName, abilityName: 'EntryAbility' };
      let threw = false;
      try { d.onRestoreInstanceState(want); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('TestOnRestoreInstanceState with no framework key', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      const params: Record<string, Object> = {};
      params['other'] = 'value';
      const want: Want = { bundleName: bundleName, abilityName: 'EntryAbility', parameters: params };
      let threw = false;
      try { d.onRestoreInstanceState(want); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('TestOnRestoreInstanceState with framework object', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      const frameworkData: Record<string, Object> = {};
      frameworkData['0'] = 72;
      frameworkData['1'] = 101;
      const params: Record<string, Object> = {};
      params['framework'] = frameworkData;
      const want: Want = { bundleName: bundleName, abilityName: 'EntryAbility', parameters: params };
      let threw = false;
      try { d.onRestoreInstanceState(want); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('TestOnRestoreInstanceState distributes plugin state to registered listeners', 0, async (done: Function) => {
      const ability: UIAbility = await delegator.getCurrentTopAbility();
      const spy = new SpyControlSurface();
      const engine = new SpyEngine(ability.context, spy);
      const d = new FlutterAbilityAndEntryDelegate(new RestoreTestHost(ability, true));
      d.flutterEngine = engine;
      const pluginState: Record<string, Object> = {};
      pluginState['pluginA'] = 'savedValue';
      const params: Record<string, Object> = {};
      params['plugins'] = pluginState;
      const want: Want = { bundleName: bundleName, abilityName: 'EntryAbility', parameters: params };
      d.onRestoreInstanceState(want);
      expect(spy.restoreCallCount).assertEqual(1);
      expect(spy.lastRestoredState!['pluginA']).assertEqual('savedValue');
      done();
    });

    it('TestOnRestoreInstanceState passes empty object when no plugin state key', 0, async (done: Function) => {
      const ability: UIAbility = await delegator.getCurrentTopAbility();
      const spy = new SpyControlSurface();
      const engine = new SpyEngine(ability.context, spy);
      const d = new FlutterAbilityAndEntryDelegate(new RestoreTestHost(ability, true));
      d.flutterEngine = engine;
      const params: Record<string, Object> = {};
      params['other'] = 'value';
      const want: Want = { bundleName: bundleName, abilityName: 'EntryAbility', parameters: params };
      let threw = false;
      try { d.onRestoreInstanceState(want); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      expect(spy.restoreCallCount).assertEqual(1);
      expect(Object.keys(spy.lastRestoredState!).length).assertEqual(0);
      done();
    });

    it('TestOnRestoreInstanceState does not distribute when host does not attach engine', 0, async (done: Function) => {
      const ability: UIAbility = await delegator.getCurrentTopAbility();
      const spy = new SpyControlSurface();
      const engine = new SpyEngine(ability.context, spy);
      const d = new FlutterAbilityAndEntryDelegate(new RestoreTestHost(ability, false));
      d.flutterEngine = engine;
      const pluginState: Record<string, Object> = {};
      pluginState['pluginA'] = 'savedValue';
      const params: Record<string, Object> = {};
      params['plugins'] = pluginState;
      const want: Want = { bundleName: bundleName, abilityName: 'EntryAbility', parameters: params };
      let threw = false;
      try { d.onRestoreInstanceState(want); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      expect(spy.restoreCallCount).assertEqual(0);
      done();
    });

    it('TestOnRestoreInstanceState supports legacy ActivityControlSurface without restore callback', 0, async (done: Function) => {
      const ability: UIAbility = await delegator.getCurrentTopAbility();
      const controlSurface = new LegacyControlSurface();
      const engine = new SpyEngine(ability.context, controlSurface);
      const d = new FlutterAbilityAndEntryDelegate(new RestoreTestHost(ability, true));
      d.flutterEngine = engine;
      const pluginState: Record<string, Object> = {};
      pluginState['pluginA'] = 'savedValue';
      const params: Record<string, Object> = {};
      params['plugins'] = pluginState;
      const want: Want = { bundleName: bundleName, abilityName: 'EntryAbility', parameters: params };
      let threw = false;
      try { d.onRestoreInstanceState(want); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('TestOnRestoreInstanceState passes empty object for invalid plugin state', 0, async (done: Function) => {
      const ability: UIAbility = await delegator.getCurrentTopAbility();
      const spy = new SpyControlSurface();
      const engine = new SpyEngine(ability.context, spy);
      const d = new FlutterAbilityAndEntryDelegate(new RestoreTestHost(ability, true));
      d.flutterEngine = engine;
      const params: Record<string, Object> = {};
      params['plugins'] = 'invalid';
      const want: Want = { bundleName: bundleName, abilityName: 'EntryAbility', parameters: params };
      d.onRestoreInstanceState(want);
      expect(spy.restoreCallCount).assertEqual(1);
      expect(Object.keys(spy.lastRestoredState!).length).assertEqual(0);
      done();
    });

    it('TestDetachFromFlutterEngine does not throw without host', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      let threw = false;
      try { d.detachFromFlutterEngine(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('TestGetAppComponent throws without host', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      let threw = false;
      try { d.getAppComponent(); } catch (e) { threw = true; }
      expect(threw).assertEqual(true);
      done();
    });

    it('TestOnNewWant does not throw without engine', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      const want: Want = { bundleName: bundleName, abilityName: 'EntryAbility' };
      const launchParam: AbilityConstant.LaunchParam = {
        launchReason: AbilityConstant.LaunchReason.START_ABILITY,
        lastExitReason: AbilityConstant.LastExitReason.NORMAL,
        lastExitMessage: ''
      };
      let threw = false;
      try { d.onNewWant(want, launchParam); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('TestRelease does not throw', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      let threw = false;
      try { d.release(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('TestInitWindow does not throw without engine', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      let threw = false;
      try { d.initWindow(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('TestChangeColorMode undefined does not throw', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      let threw = false;
      try { d.changeColorMode(undefined); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('TestChangeColorMode DARK does not throw', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      let threw = false;
      try { d.changeColorMode(ConfigurationConstant.ColorMode.COLOR_MODE_DARK); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('TestOnCheckAndReloadFont does not throw without engine', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      let threw = false;
      try { d.onCheckAndReloadFont(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('TestOnWindowStageCreate does not throw', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      let threw = false;
      try { d.onWindowStageCreate(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('TestOnWindowStageDestroy does not throw', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      let threw = false;
      try { d.onWindowStageDestroy(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('TestAddPlugin does not throw without engine', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      const plugin: TestPlugin = new TestPlugin();
      let threw = false;
      try { d.addPlugin(plugin); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('TestRemovePlugin does not throw without engine', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      const plugin: TestPlugin = new TestPlugin();
      let threw = false;
      try { d.removePlugin(plugin); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });
  })

  describe('FlutterAbilityAndEntryDelegate.attached.branches', () => {
    it('shouldDispatchAppLifecycleState returns isAttached when no host', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      expect(d.shouldDispatchAppLifecycleState()).assertEqual(false);
      d.isAttached = true;
      expect(d.shouldDispatchAppLifecycleState()).assertEqual(true);
      done();
    });

    it('OnPaused with isAttached true does not throw', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      d.isAttached = true;
      let threw = false;
      try { d.onPaused(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('OnResumed with isAttached true does not throw', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      d.isAttached = true;
      let threw = false;
      try { d.onResumed(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('OnShow with isAttached true does not throw', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      d.isAttached = true;
      let threw = false;
      try { d.onShow(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('OnHide with isAttached true does not throw', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      d.isAttached = true;
      let threw = false;
      try { d.onHide(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('OnWindowFocusChanged true with isAttached true does not throw', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      d.isAttached = true;
      let threw = false;
      try { d.onWindowFocusChanged(true); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('OnWindowFocusChanged false with isAttached true does not throw', 0, async (done: Function) => {
      const d = new FlutterAbilityAndEntryDelegate();
      d.isAttached = true;
      let threw = false;
      try { d.onWindowFocusChanged(false); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });
  })
}

/** Host fake with configurable initial route / library uri / bundle path. */
class FakeDelegateHost {
  initialRoute: string | undefined = undefined;
  libraryUri: string | undefined = undefined;
  appBundlePath: string | undefined = undefined;

  getInitialRoute(): string {
    // ESObject-cast at the use site lets us return null/undefined to exercise
    // the intent-fallback branch (host != null && initialRoute == null).
    return this.initialRoute as string;
  }

  getWant(): Want {
    const want: Want = {
      bundleName: 'com.example.flutter_tester',
      abilityName: 'EntryAbility'
    };
    return want;
  }

  getDartEntrypointLibraryUri(): string {
    return this.libraryUri ?? '';
  }

  getDartEntrypointFunctionName(): string {
    return 'main';
  }

  getAppBundlePath(): string {
    return this.appBundlePath ?? '';
  }

  getDartEntrypointArgs(): Array<string> {
    return new Array<string>();
  }
}

/** Minimal FlutterEngine fake exposing just dartExecutor + split-view channel. */
class FakeDelegateEngine {
  dartExecutor: FakeDartExecutor = new FakeDartExecutor();

  getSplitViewConfigSystemChannel(): Object | undefined {
    return undefined;
  }
}

class FakeDartExecutor {
  running: boolean = false;

  isExecutingDart(): boolean {
    return this.running;
  }

  executeDartEntrypoint(entrypoint: Object, args: Object): void {
  }
}
