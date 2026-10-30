/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, Level } from '@ohos/hypium';
import FlutterNapi from '../../../../../main/ets/embedding/engine/FlutterNapi';
import FlutterLoader from '../../../../../main/ets/embedding/engine/loader/FlutterLoader';
import ApplicationInfoLoader from '../../../../../main/ets/embedding/engine/loader/ApplicationInfoLoader';
import common from '@ohos.app.ability.common';

export default function flutterEngineInitVerifyTest() {
  describe('FlutterEngineInitVerify', () => {
    it('verify direct init and attach succeed', Level.LEVEL0, async (done: Function) => {
      const context = getContext() as common.UIAbilityContext;
      const flutterNapi = new FlutterNapi();
      const appInfo = ApplicationInfoLoader.load(context);

      const shellArgs: string[] = [];
      shellArgs.push('--icu-symbol-prefix=_binary_icudtl_dat');
      shellArgs.push('--icu-native-lib-path=' + appInfo.nativeLibraryDir + '/libflutter.so');
      shellArgs.push('--aot-shared-library-name=' + appInfo.nativeLibraryDir + '/libapp.so');
      shellArgs.push('--cache-dir-path=' + context.cacheDir + '/');
      shellArgs.push('--prefetched-default-font-manager');
      shellArgs.push('--leak-vm=true');

      flutterNapi.init(context, shellArgs, '', context.filesDir + '/', context.cacheDir + '/', Date.now());
      flutterNapi.attachToNative();
      try {
        expect(flutterNapi.isAttached()).assertEqual(true);
        expect(flutterNapi.nativeShellHolderId).not().assertEqual(null);
        expect(flutterNapi.nativeShellHolderId).not().assertEqual(0);
      } finally {
        flutterNapi.detachFromNativeAndReleaseResources();
      }
      done();
    });
  });
}
