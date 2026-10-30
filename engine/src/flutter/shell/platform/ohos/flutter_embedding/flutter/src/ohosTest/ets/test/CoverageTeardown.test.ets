/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { afterAll, describe, it } from '@ohos/hypium';
import fs from '@ohos.file.fs';
import { abilityDelegatorRegistry } from '@kit.TestKit';

const SAVE_PATH: string = '/data/storage/el2/base/js_coverage.json';
const JS_COVERAGE_FILE: string = 'js_coverage.json';

/**
 * Writes __coverage__ to the app sandbox and prints OHOS_REPORT_COVERAGE_PATH so
 * the host workflow can hdc file recv the full JSON (stdout streaming truncates).
 */
export default function CoverageTeardownTest() {
  describe('CoverageTeardownTest', () => {
    // Hypium only fires a describe's afterAll when the describe has at least one
    // it; this spec exists solely to carry the export in the afterAll below.
    it('export_coverage_to_sandbox', 0, () => {
    });

    afterAll(() => {
      const globalObj: object = new Function('return this')();
      const coverageKey: string = '__coverage__';
      const coverage: object | undefined = globalObj[coverageKey] as object | undefined;
      if (coverage === undefined) {
        console.info('CoverageTeardown: __coverage__ missing, skip file export');
        return;
      }

      const payload: string = JSON.stringify(coverage);

      // A teardown hook must never throw: an escaping BusinessError would fail
      // the whole suite and skip the OHOS_REPORT_COVERAGE_PATH print below,
      // which is the only signal the host coverage workflow depends on.
      try {
        const dirEnd: number = SAVE_PATH.length - JS_COVERAGE_FILE.length;
        const dirPath: string = SAVE_PATH.substring(0, dirEnd);
        try {
          if (!fs.accessSync(dirPath)) {
            fs.mkdirSync(dirPath);
          }
        } catch (_) {
          // accessSync threw (missing dir or unsupported query); recursive mkdir
          // tolerates an already-existing directory instead of re-throwing.
          fs.mkdirSync(dirPath, true);
        }

        // TRUNC guarantees the output starts from an empty file even if a previous
        // run's js_coverage.json survived, so a shorter payload can never leave
        // stale JSON tail behind (which would corrupt the file for the host recv).
        const file = fs.openSync(SAVE_PATH, fs.OpenMode.READ_WRITE | fs.OpenMode.CREATE | fs.OpenMode.TRUNC);
        fs.writeSync(file.fd, payload, { encoding: 'utf-8' });
        fs.closeSync(file);
      } catch (e) {
        console.info('CoverageTeardown: export failed: ' + JSON.stringify(e));
        return;
      }

      let readPath: string = SAVE_PATH;
      try {
        const delegator = abilityDelegatorRegistry.getAbilityDelegator();
        const uid: number = Math.floor(delegator.getAppContext().applicationInfo.uid / 200000);
        const bundleName: string = abilityDelegatorRegistry.getArguments().bundleName;
        readPath = '/data/app/el2/' + uid + '/base/' + bundleName + '/' + JS_COVERAGE_FILE;
      } catch (_) {
        // Use sandbox-relative recv path.
      }

      console.info('OHOS_REPORT_COVERAGE_PATH: ' + readPath);
      console.info('CoverageTeardown: wrote ' + payload.length + ' bytes to ' + SAVE_PATH);
    });
  });
}
