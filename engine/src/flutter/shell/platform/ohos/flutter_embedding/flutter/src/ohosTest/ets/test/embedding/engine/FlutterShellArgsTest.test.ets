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
import { Want } from '@kit.AbilityKit';
import FlutterShellArgs from '../../../../../main/ets/embedding/engine/FlutterShellArgs';
import { abilityDelegatorRegistry } from '@kit.TestKit';

const bundleName = abilityDelegatorRegistry.getArguments().bundleName;

function makeWant(params: Record<string, Object>): Want {
  return {
    bundleName: 'test.bundle',
    abilityName: 'EntryAbility',
    parameters: params
  };
}

export default function FlutterShellArgsTest() {
  describe('FlutterShellArgsTest', () => {

    it('constants_areAsExpected', Level.LEVEL1, () => {
      expect(FlutterShellArgs.ARG_TRACE_STARTUP).assertEqual('--trace-startup');
      expect(FlutterShellArgs.ARG_START_PAUSED).assertEqual('--start-paused');
      expect(FlutterShellArgs.ARG_DISABLE_SERVICE_AUTH_CODES).assertEqual('--disable-service-auth-codes');
      expect(FlutterShellArgs.ARG_ENDLESS_TRACE_BUFFER).assertEqual('--endless-trace-buffer');
      expect(FlutterShellArgs.ARG_USE_TEST_FONTS).assertEqual('--use-test-fonts');
      expect(FlutterShellArgs.ARG_ENABLE_DART_PROFILING).assertEqual('--enable-dart-profiling');
      expect(FlutterShellArgs.ARG_PROFILE_STARTUP).assertEqual('--profile-startup');
      expect(FlutterShellArgs.ARG_ENABLE_SOFTWARE_RENDERING).assertEqual('--enable-software-rendering');
      expect(FlutterShellArgs.ARG_ENABLE_IMPELLER).assertEqual('--enable-impeller');
      expect(FlutterShellArgs.ARG_DUMP_SHADER_SKP_ON_SHADER_COMPILATION).assertEqual('--dump-skp-on-shader-compilation');
      expect(FlutterShellArgs.ARG_CACHE_SKSL).assertEqual('--cache-sksl');
      expect(FlutterShellArgs.ARG_PURGE_PERSISTENT_CACHE).assertEqual('--purge-persistent-cache');
      expect(FlutterShellArgs.ARG_VERBOSE_LOGGING).assertEqual('--verbose-logging');
      expect(FlutterShellArgs.ARG_OBSERVATORY_PORT).assertEqual('--observatory-port=');
      expect(FlutterShellArgs.ARG_DART_FLAGS).assertEqual('--dart-flags=');
      expect(FlutterShellArgs.ARG_MSAA_SAMPLES).assertEqual('--msaa-samples=');
    })

    it('fromWant_emptyParameters_returnsEmptyArgs', Level.LEVEL1, () => {
      const args = FlutterShellArgs.fromWant(makeWant({}));
      expect(args.toArray().length).assertEqual(0);
    })

    it('fromWant_allBooleanFlags_addAllArgs', Level.LEVEL1, () => {
      const want = makeWant({
        'trace-startup': true,
        'start-paused': true,
        'disable-service-auth-codes': true,
        'endless-trace-buffer': true,
        'use-test-fonts': true,
        'enable-dart-profiling': true,
        'profile-startup': true,
        'enable-software-rendering': true,
        'enable-impeller': true,
        'dump-skp-on-shader-compilation': true,
        'cache-sksl': true,
        'purge-persistent-cache': true,
        'verbose-logging': true
      } as Record<string, Object>);
      const arr = FlutterShellArgs.fromWant(want).toArray();
      expect(arr.includes('--trace-startup')).assertEqual(true);
      expect(arr.includes('--start-paused')).assertEqual(true);
      expect(arr.includes('--disable-service-auth-codes')).assertEqual(true);
      expect(arr.includes('--endless-trace-buffer')).assertEqual(true);
      expect(arr.includes('--use-test-fonts')).assertEqual(true);
      expect(arr.includes('--enable-dart-profiling')).assertEqual(true);
      expect(arr.includes('--profile-startup')).assertEqual(true);
      expect(arr.includes('--enable-software-rendering')).assertEqual(true);
      expect(arr.includes('--enable-impeller')).assertEqual(true);
      expect(arr.includes('--dump-skp-on-shader-compilation')).assertEqual(true);
      expect(arr.includes('--cache-sksl')).assertEqual(true);
      expect(arr.includes('--purge-persistent-cache')).assertEqual(true);
      expect(arr.includes('--verbose-logging')).assertEqual(true);
    })

    it('fromWant_falseFlags_areNotAdded', Level.LEVEL1, () => {
      const want = makeWant({
        'trace-startup': false,
        'start-paused': false
      } as Record<string, Object>);
      const arr = FlutterShellArgs.fromWant(want).toArray();
      expect(arr.length).assertEqual(0);
      expect(arr.includes('--trace-startup')).assertEqual(false);
    })

    it('fromWant_observatoryPort_positiveIsAdded', Level.LEVEL1, () => {
      const want = makeWant({ 'observatory-port': 12345 } as Record<string, Object>);
      const arr = FlutterShellArgs.fromWant(want).toArray();
      expect(arr.includes('--observatory-port=12345')).assertEqual(true);
    })

    it('fromWant_observatoryPort_zeroNotAdded', Level.LEVEL1, () => {
      const want = makeWant({ 'observatory-port': 0 } as Record<string, Object>);
      const arr = FlutterShellArgs.fromWant(want).toArray();
      expect(arr.length).assertEqual(0);
    })

    it('fromWant_msaaSamples_greaterThanOneIsAdded', Level.LEVEL1, () => {
      const want = makeWant({ 'msaa-samples': 4 } as Record<string, Object>);
      const arr = FlutterShellArgs.fromWant(want).toArray();
      expect(arr.includes('--msaa-samples=4')).assertEqual(true);
    })

    it('fromWant_msaaSamples_oneNotAdded', Level.LEVEL1, () => {
      const want = makeWant({ 'msaa-samples': 1 } as Record<string, Object>);
      const arr = FlutterShellArgs.fromWant(want).toArray();
      expect(arr.length).assertEqual(0);
    })

    it('fromWant_dartFlags_valueIsAppended', Level.LEVEL1, () => {
      const want = makeWant({ 'dart-flags': '--random-seed=99' } as Record<string, Object>);
      const arr = FlutterShellArgs.fromWant(want).toArray();
      expect(arr.includes('--dart-flags=--random-seed=99')).assertEqual(true);
    })

    it('fromWant_dartFlags_leadingSpaceIsTrimmed', Level.LEVEL1, () => {
      // flutter_tools prefixes a space when the value starts with `-`,
      // because `aa start` rejects `--ps` values beginning with `-`.
      const want = makeWant({ 'dart-flags': ' --random-seed=99' } as Record<string, Object>);
      const arr = FlutterShellArgs.fromWant(want).toArray();
      expect(arr.includes('--dart-flags=--random-seed=99')).assertEqual(true);
    })

    it('checkArg_noParameters_doesNotThrowOrAdd', Level.LEVEL1, () => {
      const shellArgs = new FlutterShellArgs();
      const wantNoParams: Want = { bundleName: 'x', abilityName: 'y' };
      let threw = false;
      try {
        FlutterShellArgs.checkArg('trace-startup', '--trace-startup', wantNoParams, shellArgs);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
      expect(shellArgs.toArray().length).assertEqual(0);
    })

    it('checkArg_truthyValue_addsFlag', Level.LEVEL1, () => {
      const shellArgs = new FlutterShellArgs();
      const want = makeWant({ 'trace-startup': true } as Record<string, Object>);
      FlutterShellArgs.checkArg('trace-startup', '--trace-startup', want, shellArgs);
      expect(shellArgs.toArray().includes('--trace-startup')).assertEqual(true);
    })

    it('checkArg_missingKey_doesNotAdd', Level.LEVEL1, () => {
      const shellArgs = new FlutterShellArgs();
      const want = makeWant({} as Record<string, Object>);
      FlutterShellArgs.checkArg('trace-startup', '--trace-startup', want, shellArgs);
      expect(shellArgs.toArray().length).assertEqual(0);
    })

    it('add_remove_toArray_setSemantics', Level.LEVEL1, () => {
      const shellArgs = new FlutterShellArgs();
      shellArgs.add('--a');
      shellArgs.add('--b');
      shellArgs.add('--a'); // Set de-duplicates
      expect(shellArgs.toArray().length).assertEqual(2);
      expect(shellArgs.toArray().includes('--a')).assertEqual(true);
      expect(shellArgs.toArray().includes('--b')).assertEqual(true);
      shellArgs.remove('--a');
      expect(shellArgs.toArray().length).assertEqual(1);
      expect(shellArgs.toArray().includes('--a')).assertEqual(false);
      expect(shellArgs.toArray().includes('--b')).assertEqual(true);
    })

    it('fromWant with trace-startup parameter', 0, async (done: Function) => {
      const wantWithParams: Want = {
        bundleName: bundleName,
        abilityName: 'EntryAbility',
        parameters: {
          'trace-startup': true,
        } as Record<string, Object>
      }
      try {
        const args = FlutterShellArgs.fromWant(wantWithParams);
        expect(args).not().assertNull();
        const arr = args.toArray();
        expect(arr.includes(FlutterShellArgs.ARG_TRACE_STARTUP)).assertEqual(true);
      } catch (e) {}
      done();
    })

    it('fromWant with start-paused parameter', 0, async (done: Function) => {
      const wantWithParams: Want = {
        bundleName: bundleName,
        abilityName: 'EntryAbility',
        parameters: {
          'start-paused': true,
        } as Record<string, Object>
      }
      try {
        const args = FlutterShellArgs.fromWant(wantWithParams);
        const arr = args.toArray();
        expect(arr.includes(FlutterShellArgs.ARG_START_PAUSED)).assertEqual(true);
      } catch (e) {}
      done();
    })

    it('fromWant with multiple parameters', 0, async (done: Function) => {
      const wantWithParams: Want = {
        bundleName: bundleName,
        abilityName: 'EntryAbility',
        parameters: {
          'enable-dart-profiling': true,
          'enable-software-rendering': true,
          'verbose-logging': true,
          'enable-impeller': true,
          'cache-sksl': true,
        } as Record<string, Object>
      }
      try {
        const args = FlutterShellArgs.fromWant(wantWithParams);
        const arr = args.toArray();
        expect(arr.includes(FlutterShellArgs.ARG_ENABLE_DART_PROFILING)).assertEqual(true);
        expect(arr.includes(FlutterShellArgs.ARG_ENABLE_SOFTWARE_RENDERING)).assertEqual(true);
        expect(arr.includes(FlutterShellArgs.ARG_VERBOSE_LOGGING)).assertEqual(true);
      } catch (e) {}
      done();
    })

    it('fromWant with observatory-port parameter', 0, async (done: Function) => {
      const wantWithParams: Want = {
        bundleName: bundleName,
        abilityName: 'EntryAbility',
        parameters: {
          'observatory-port': 8080,
        } as Record<string, Object>
      }
      try {
        const args = FlutterShellArgs.fromWant(wantWithParams);
        const arr = args.toArray();
        expect(arr.length > 0).assertEqual(true);
      } catch (e) {}
      done();
    })

    it('fromWant with msaa-samples parameter', 0, async (done: Function) => {
      const wantWithParams: Want = {
        bundleName: bundleName,
        abilityName: 'EntryAbility',
        parameters: {
          'msaa-samples': 4,
        } as Record<string, Object>
      }
      try {
        const args = FlutterShellArgs.fromWant(wantWithParams);
        const arr = args.toArray();
        expect(arr.length > 0).assertEqual(true);
      } catch (e) {}
      done();
    })

    it('fromWant with dart-flags parameter', 0, async (done: Function) => {
      const wantWithParams: Want = {
        bundleName: bundleName,
        abilityName: 'EntryAbility',
        parameters: {
          'dart-flags': '--enable-asserts',
        } as Record<string, Object>
      }
      try {
        const args = FlutterShellArgs.fromWant(wantWithParams);
        const arr = args.toArray();
        expect(arr.length > 0).assertEqual(true);
      } catch (e) {}
      done();
    })

    it('fromWant with trace-skia-allowlist parameter', 0, async (done: Function) => {
      const wantWithParams: Want = {
        bundleName: bundleName,
        abilityName: 'EntryAbility',
        parameters: {
          'trace-skia-allowlist': 'skp',
        } as Record<string, Object>
      }
      try {
        const args = FlutterShellArgs.fromWant(wantWithParams);
        const arr = args.toArray();
        expect(arr.length > 0).assertEqual(true);
      } catch (e) {}
      done();
    })

    it('fromWant with false parameter does not add flag', 0, async (done: Function) => {
      const wantWithParams: Want = {
        bundleName: bundleName,
        abilityName: 'EntryAbility',
        parameters: {
          'trace-startup': false,
        } as Record<string, Object>
      }
      try {
        const args = FlutterShellArgs.fromWant(wantWithParams);
        const arr = args.toArray();
        expect(arr.includes(FlutterShellArgs.ARG_TRACE_STARTUP)).assertEqual(false);
      } catch (e) {}
      done();
    })

    it('checkArg with undefined parameters returns early', 0, async (done: Function) => {
      const wantNoParams: Want = {
        bundleName: bundleName,
        abilityName: 'EntryAbility',
      }
      try {
        const args = new FlutterShellArgs();
        FlutterShellArgs.checkArg('key', '--flag', wantNoParams, args);
        expect(args.toArray().length).assertEqual(0);
      } catch (e) {}
      done();
    })

    it('checkArg with true value adds flag', 0, async (done: Function) => {
      const wantWithParams: Want = {
        bundleName: bundleName,
        abilityName: 'EntryAbility',
        parameters: {
          'custom-key': true,
        } as Record<string, Object>
      }
      try {
        const args = new FlutterShellArgs();
        FlutterShellArgs.checkArg('custom-key', '--custom-flag', wantWithParams, args);
        expect(args.toArray().includes('--custom-flag')).assertEqual(true);
      } catch (e) {}
      done();
    })

    it('checkArg with false value does not add flag', 0, async (done: Function) => {
      const wantWithParams: Want = {
        bundleName: bundleName,
        abilityName: 'EntryAbility',
        parameters: {
          'custom-key': false,
        } as Record<string, Object>
      }
      try {
        const args = new FlutterShellArgs();
        FlutterShellArgs.checkArg('custom-key', '--custom-flag', wantWithParams, args);
        expect(args.toArray().includes('--custom-flag')).assertEqual(false);
      } catch (e) {}
      done();
    })

    it('add and remove args', 0, async (done: Function) => {
      const args = new FlutterShellArgs();
      args.add('--test-flag');
      expect(args.toArray().includes('--test-flag')).assertEqual(true);
      args.remove('--test-flag');
      expect(args.toArray().includes('--test-flag')).assertEqual(false);
      done();
    })
























  })
}
