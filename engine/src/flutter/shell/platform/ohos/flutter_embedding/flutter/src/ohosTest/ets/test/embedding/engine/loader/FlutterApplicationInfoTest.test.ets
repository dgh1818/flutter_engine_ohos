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

import FlutterApplicationInfo from '../../../../../../main/ets/embedding/engine/loader/FlutterApplicationInfo';
import { describe, expect, it } from '@ohos/hypium';
export default function FlutterApplicationInfoTest() {
  describe('FlutterApplicationInfoTest', () => {
    it('TestLoad', 0, async (done: Function) => {
      let flutterApplicationInfo = new FlutterApplicationInfo("test", "test", "test", "test", "test", "test", false)
      expect(flutterApplicationInfo).not().assertNull();
      done();
    })

    it('null fields fall back to defaults', 0, () => {
      const info = new FlutterApplicationInfo(null, null, null, null, null, '/libs', true);
      expect(info.aotSharedLibraryName).assertEqual('libapp.so');
      expect(info.vmSnapshotData).assertEqual('vm_snapshot_data');
      expect(info.isolateSnapshotData).assertEqual('isolate_snapshot_data');
      expect(info.flutterAssetsDir).assertEqual('flutter_assets');
      expect(info.domainNetworkPolicy).assertEqual('');
      expect(info.nativeLibraryDir).assertEqual('/libs');
      expect(info.automaticallyRegisterPlugins).assertEqual(true);
    })

    it('provided fields are stored', 0, () => {
      const info = new FlutterApplicationInfo('a.so', 'vm', 'iso', 'assets', 'policy', '/n', false);
      expect(info.aotSharedLibraryName).assertEqual('a.so');
      expect(info.vmSnapshotData).assertEqual('vm');
      expect(info.isolateSnapshotData).assertEqual('iso');
      expect(info.flutterAssetsDir).assertEqual('assets');
      expect(info.domainNetworkPolicy).assertEqual('policy');
      expect(info.automaticallyRegisterPlugins).assertEqual(false);
    })
  })
}
