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
  })
}
