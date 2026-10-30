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

import common from '@ohos.app.ability.common';
import FlutterLoader from '../../../../../../main/ets/embedding/engine/loader/FlutterLoader';
import ApplicationInfoLoader from '../../../../../../main/ets/embedding/engine/loader/ApplicationInfoLoader';
import { describe, expect, it } from '@ohos/hypium';
import FlutterNapi from '../../../../../../main/ets/embedding/engine/FlutterNapi';
import Any from '../../../../../../main/ets/plugin/common/Any';

const HCPP_SHELL_ARG = '--enable-ohos-hybrid-composition';

class RecordingFlutterNapi {
  lastShellArgs: Array<string> = [];

  init(_context: Object, shellArgs: Array<string>, _kernelPath: string, _appStoragePath: string,
    _engineCachesPath: string, _initTimeMillis: number): void {
    this.lastShellArgs = shellArgs;
  }
}

function makeLoader(): FlutterLoader {
  return new FlutterLoader(new FlutterNapi());
}

export default function FlutterLoaderTest() {
  describe('FlutterLoaderTest', () => {
    it('initializationTest', 0, async (done: Function) => {
      let flutterNapi: FlutterNapi = new FlutterNapi();
      let flutterLoader: FlutterLoader = new FlutterLoader(flutterNapi);
      expect(flutterLoader.initialized).assertFalse();
      done();
    })

    it('initResultTest', 0, async (done: Function) => {
      let flutterNapi: FlutterNapi = new FlutterNapi();
      let flutterLoader: FlutterLoader = new FlutterLoader(flutterNapi);
      expect(flutterLoader.initResult).assertNull();
      done();
    })

    it('flutterNapiTest', 0, async (done: Function) => {
      let flutterNapi: FlutterNapi = new FlutterNapi();
      let flutterLoader: FlutterLoader = new FlutterLoader(flutterNapi);
      expect(flutterLoader.flutterNapi).not().assertNull();
      done();
    })

    it('startInitializationTest', 0, async (done: Function) => {
      let flutterNapi: FlutterNapi = new FlutterNapi();
      let flutterLoader: FlutterLoader = new FlutterLoader(flutterNapi);
      let context: common.Context = getContext();
      expect(flutterLoader.initialized).assertFalse();
      flutterLoader.startInitialization(context);
      expect(flutterLoader.flutterApplicationInfo).not().assertNull();
      expect(flutterLoader.context).not().assertNull();
      done();
    })

    it('startInitialization_hcpp_false_when_buildinfo_omits_flag', 0, () => {
      const loader: FlutterLoader = makeLoader();
      loader.startInitialization(getContext());
      expect(loader.isEnableHcpp).assertFalse();
    })

    it('startInitialization_hcpp_true_when_getBuildInfo_returns_true', 0, () => {
      const loader: FlutterLoader = makeLoader();
      const buildInfo: Map<string, string> = new Map();
      buildInfo.set('enable_impeller', 'true');
      buildInfo.set('enable_ohos_hybrid_composition', 'true');
      const anyLoader: Any = loader as Any;
      anyLoader.getBuildInfo = (_ctx: common.Context): Map<string, string> => buildInfo;
      loader.startInitialization(getContext());
      expect(loader.isEnableHcpp).assertTrue();
    })

    it('ensureInitializationComplete_pushes_hcpp_arg_when_enabled_via_code', 0, () => {
      const loader: FlutterLoader = makeLoader();
      loader.startInitialization(getContext());
      const recording: RecordingFlutterNapi = new RecordingFlutterNapi();
      loader.flutterNapi = recording as Any as FlutterNapi;
      loader.initialized = false;
      loader.isEnableHcpp = true;
      loader.ensureInitializationComplete(null);
      expect(recording.lastShellArgs.indexOf(HCPP_SHELL_ARG) >= 0).assertTrue();
      expect(loader.initialized).assertTrue();
    })

    it('ensureInitializationComplete_omits_hcpp_arg_when_disabled', 0, () => {
      const loader: FlutterLoader = makeLoader();
      loader.startInitialization(getContext());
      const recording: RecordingFlutterNapi = new RecordingFlutterNapi();
      loader.flutterNapi = recording as Any as FlutterNapi;
      loader.initialized = false;
      loader.isEnableHcpp = false;
      loader.ensureInitializationComplete(null);
      expect(recording.lastShellArgs.indexOf(HCPP_SHELL_ARG) >= 0).assertFalse();
    })

    it('getLookupKeyForAsset_without_package', 0, () => {
      const loader: FlutterLoader = makeLoader();
      loader.startInitialization(getContext());
      const result: string = loader.getLookupKeyForAsset('fonts/foo.ttf');
      expect(result.indexOf('fonts/foo.ttf') >= 0).assertTrue();
      expect(result).assertEqual(loader.flutterApplicationInfo!.flutterAssetsDir + '/fonts/foo.ttf');
    })

    it('getLookupKeyForAsset_with_package', 0, () => {
      const loader: FlutterLoader = makeLoader();
      loader.startInitialization(getContext());
      const result: string = loader.getLookupKeyForAsset('fonts/foo.ttf', 'my_pkg');
      expect(result.indexOf('packages/my_pkg/fonts/foo.ttf') >= 0).assertTrue();
    })

    it('fullAssetPathFrom_uninitialized_returns_empty', 0, () => {
      const loader: FlutterLoader = makeLoader();
      expect(loader.fullAssetPathFrom('fonts/foo.ttf')).assertEqual("");
    })

    it('fullAssetPathFrom_initialized_joins_assets_dir', 0, () => {
      const loader: FlutterLoader = makeLoader();
      loader.startInitialization(getContext());
      const result: string = loader.fullAssetPathFrom('fonts/foo.ttf');
      expect(result).assertEqual(loader.flutterApplicationInfo!.flutterAssetsDir + '/fonts/foo.ttf');
    })

    it('ensureInitializationComplete_debug_mode_pushes_debug_args', 0, () => {
      const loader: FlutterLoader = makeLoader();
      loader.startInitialization(getContext());
      loader.flutterApplicationInfo!.isDebugMode = true;
      const recording: RecordingFlutterNapi = new RecordingFlutterNapi();
      loader.flutterNapi = recording as Any as FlutterNapi;
      loader.initialized = false;
      loader.ensureInitializationComplete(null);
      expect(recording.lastShellArgs.indexOf('--enable-checked-mode') >= 0).assertTrue();
    })

    it('ensureInitializationComplete_profile_mode_pushes_vmservice', 0, () => {
      const loader: FlutterLoader = makeLoader();
      loader.startInitialization(getContext());
      // The vmservice arg is only pushed in the non-debug branch, so force that
      // path regardless of the HAP's actual build mode (test HAP is a debug build).
      loader.flutterApplicationInfo!.isDebugMode = false;
      loader.flutterApplicationInfo!.isProfile = true;
      const recording: RecordingFlutterNapi = new RecordingFlutterNapi();
      loader.flutterNapi = recording as Any as FlutterNapi;
      loader.initialized = false;
      loader.ensureInitializationComplete(null);
      expect(recording.lastShellArgs.indexOf('--aot-vmservice-shared-library-name=libvmservice_snapshot.so') >= 0).assertTrue();
    })

    it('ensureInitializationComplete_domain_network_policy', 0, () => {
      const loader: FlutterLoader = makeLoader();
      loader.startInitialization(getContext());
      loader.flutterApplicationInfo!.domainNetworkPolicy = 'example.com';
      const recording: RecordingFlutterNapi = new RecordingFlutterNapi();
      loader.flutterNapi = recording as Any as FlutterNapi;
      loader.initialized = false;
      loader.ensureInitializationComplete(null);
      expect(recording.lastShellArgs.indexOf('--domain-network-policy=example.com') >= 0).assertTrue();
    })
    it('ensureInitializationComplete_merges_metadata_args', 0, () => {
      const loader: FlutterLoader = makeLoader();
      loader.startInitialization(getContext());
      const recording: RecordingFlutterNapi = new RecordingFlutterNapi();
      loader.flutterNapi = recording as Any as FlutterNapi;
      loader.initialized = false;
      // Patch our own static as the metadata seam: the test bundle declares
      // no metadata, and the bundleManager namespace cannot be mocked.
      const fakeMetadata: Map<string, string> = new Map();
      fakeMetadata.set('flutter.engine.EnableDartProfiling', 'true');
      fakeMetadata.set('flutter.engine.EnableImpeller', 'false');
      fakeMetadata.set('flutter.engine.TestFlag', 'true');
      fakeMetadata.set('test.plain.key', 'x');
      const anyAIL: Any = ApplicationInfoLoader as Any;
      const orig: Any = anyAIL.getModuleMetadata;
      try {
        anyAIL.getModuleMetadata = (): Map<string, string> => fakeMetadata;
        loader.ensureInitializationComplete(null);
      } finally {
        anyAIL.getModuleMetadata = orig;
      }
      const args: Array<string> = recording.lastShellArgs;
      expect(args.indexOf('--enable-dart-profiling') >= 0).assertTrue();
      expect(args.indexOf('--enable-impeller=false') >= 0).assertTrue();
      // The test-only flag and non-engine keys are never forwarded.
      expect(args.indexOf('--test-flag') >= 0).assertFalse();
    })

    it('ensureInitializationComplete_filters_want_args_in_release_mode', 0, () => {
      const loader: FlutterLoader = makeLoader();
      loader.startInitialization(getContext());
      // Force the release configuration: neither debug mode nor profile.
      const anyInfo: Any = (loader as Any).flutterApplicationInfo as Any;
      const origDebugMode: Any = anyInfo.isDebugMode;
      const origProfile: Any = anyInfo.isProfile;
      anyInfo.isDebugMode = false;
      anyInfo.isProfile = false;
      const recording: RecordingFlutterNapi = new RecordingFlutterNapi();
      loader.flutterNapi = recording as Any as FlutterNapi;
      loader.initialized = false;
      try {
        loader.ensureInitializationComplete(
          ['--trace-skia', '--enable-dart-profiling', '--unknown-want-flag']);
      } finally {
        anyInfo.isDebugMode = origDebugMode;
        anyInfo.isProfile = origProfile;
      }
      const args: Array<string> = recording.lastShellArgs;
      // Not allowed in release: dropped from the Want args.
      expect(args.indexOf('--trace-skia') >= 0).assertFalse();
      // Allowed and unknown args are passed through (flutter/flutter#182557).
      expect(args.indexOf('--enable-dart-profiling') >= 0).assertTrue();
      expect(args.indexOf('--unknown-want-flag') >= 0).assertTrue();
    })
  })
}
