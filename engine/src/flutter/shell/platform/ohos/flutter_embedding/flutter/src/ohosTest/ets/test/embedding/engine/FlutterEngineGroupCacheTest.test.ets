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
import FlutterEngineGroup from '../../../../../main/ets/embedding/engine/FlutterEngineGroup';
import FlutterEngineGroupCache from '../../../../../main/ets/embedding/engine/FlutterEngineGroupCache';

export default function FlutterEngineGroupCacheTest() {
  const flutterEngineGroupCache = new FlutterEngineGroupCache();
  const engineGroupId: string = '';

  describe('FlutterEngineGroupCacheTest', () => {

    it('instance', 0, async (done: Function) => {
      console.info("uitest: instance begin");
      const instance = FlutterEngineGroupCache.instance;
      expect(instance).not().assertNull();
      done();
    })

    it('contains', 0, async (done: Function) => {
      console.info("uitest: contains begin");
      const flag = flutterEngineGroupCache.contains(engineGroupId);
      expect(flag).not().assertNull();
      done();
    })

    it('get', 0, async (done: Function) => {
      console.info("uitest: get begin");
      const engGroupId: FlutterEngineGroup | null = flutterEngineGroupCache.get(engineGroupId);
      if (engGroupId) {
        expect(engGroupId).not().assertNull();
      }
      done();
    })

    it('contains returns false for unknown id', 0, () => {
      const r = flutterEngineGroupCache.contains('non-existent-group-xyz');
      expect(r).assertEqual(false);
    });

    it('get returns null for unknown id', 0, () => {
      const r = flutterEngineGroupCache.get('non-existent-group-xyz');
      expect(r).assertNull();
    });

    it('put with undefined engineGroup does not throw', 0, () => {
      let threw = false;
      try { flutterEngineGroupCache.put('test-group-id', undefined); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('put with engineGroup does not throw (covers if branch)', 0, () => {
      let threw = false;
      try {
        const group = new FlutterEngineGroup();
        flutterEngineGroupCache.put('test-group-with-engine', group);
      } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });
  })
}