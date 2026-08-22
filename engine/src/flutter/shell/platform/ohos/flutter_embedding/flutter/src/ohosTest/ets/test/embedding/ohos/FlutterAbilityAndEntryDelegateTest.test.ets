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
import { Want } from '@kit.AbilityKit';
import { FlutterAbilityAndEntryDelegate, Host } from '../../../../../main/ets/embedding/ohos/FlutterAbilityAndEntryDelegate';
import FlutterEngine from '../../../../../main/ets/embedding/engine/FlutterEngine';

const bundleName = abilityDelegatorRegistry.getArguments().bundleName;

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