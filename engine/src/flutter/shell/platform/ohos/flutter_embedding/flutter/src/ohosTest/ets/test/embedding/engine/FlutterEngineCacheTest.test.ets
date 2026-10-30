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
import { beforeAll, describe, expect, it, Level } from '@ohos/hypium';
import FlutterEngine from '../../../../../main/ets/embedding/engine/FlutterEngine';
import FlutterEngineCache from '../../../../../main/ets/embedding/engine/FlutterEngineCache';
import PlatformViewsController from '../../../../../main/ets/plugin/platform/PlatformViewsController';
import common from '@ohos.app.ability.common';

export default function FlutterEngineCacheTest() {
  const flutterEngineCache: FlutterEngineCache = new FlutterEngineCache();
  describe('FlutterEngineCacheTest', () => {
    let context: common.Context;
    let engineA: FlutterEngine;
    let engineB: FlutterEngine;

    beforeAll(() => {
      context = getContext();
      const pvc = new PlatformViewsController();
      engineA = new FlutterEngine(context, null, null, pvc);
      engineB = new FlutterEngine(context, null, null, pvc);
    })

    it('getInstance_returnsSameSingleton', Level.LEVEL1, () => {
      const a = FlutterEngineCache.getInstance();
      const b = FlutterEngineCache.getInstance();
      expect(a).assertEqual(b);
    })

    it('contains_returnsFalseForMissingId', Level.LEVEL1, () => {
      const cache = new FlutterEngineCache();
      expect(cache.contains('missing')).assertEqual(false);
    })

    it('get_returnsNullForMissingId', Level.LEVEL1, () => {
      const cache = new FlutterEngineCache();
      expect(cache.get('missing')).assertEqual(null);
    })

    it('put_thenContainsIsTrue', Level.LEVEL1, () => {
      const cache = new FlutterEngineCache();
      cache.put('id1', engineA);
      expect(cache.contains('id1')).assertEqual(true);
    })

    it('put_thenGetReturnsSameEngine', Level.LEVEL1, () => {
      const cache = new FlutterEngineCache();
      cache.put('id1', engineA);
      expect(cache.get('id1')).assertEqual(engineA);
    })

    it('put_nullRemovesEntry', Level.LEVEL1, () => {
      const cache = new FlutterEngineCache();
      cache.put('id1', engineA);
      cache.put('id1', null);
      expect(cache.contains('id1')).assertEqual(false);
      expect(cache.get('id1')).assertEqual(null);
    })

    it('put_overwriteReplacesEngine', Level.LEVEL1, () => {
      const cache = new FlutterEngineCache();
      cache.put('id1', engineA);
      cache.put('id1', engineB);
      expect(cache.get('id1')).assertEqual(engineB);
    })

    it('containsEngine_falseWhenEmpty', Level.LEVEL1, () => {
      const cache = new FlutterEngineCache();
      expect(cache.containsEngine(engineA)).assertEqual(false);
    })

    it('containsEngine_trueWhenPresent', Level.LEVEL1, () => {
      const cache = new FlutterEngineCache();
      cache.put('id1', engineA);
      expect(cache.containsEngine(engineA)).assertEqual(true);
    })

    it('containsEngine_falseForDifferentInstance', Level.LEVEL1, () => {
      const cache = new FlutterEngineCache();
      cache.put('id1', engineA);
      expect(cache.containsEngine(engineB)).assertEqual(false);
    })

    it('remove_deletesEntry', Level.LEVEL1, () => {
      const cache = new FlutterEngineCache();
      cache.put('id1', engineA);
      cache.remove('id1');
      expect(cache.contains('id1')).assertEqual(false);
      expect(cache.get('id1')).assertEqual(null);
    })

    it('clear_emptiesAllEntries', Level.LEVEL1, () => {
      const cache = new FlutterEngineCache();
      cache.put('a', engineA);
      cache.put('b', engineB);
      cache.clear();
      expect(cache.contains('a')).assertEqual(false);
      expect(cache.contains('b')).assertEqual(false);
      expect(cache.get('a')).assertEqual(null);
      expect(cache.get('b')).assertEqual(null);
    })

    it('contains returns false for unknown id', 0, () => {
      const r = flutterEngineCache.contains('non-existent-id-12345');
      expect(r).assertEqual(false);
    });

    it('get returns null for unknown id', 0, () => {
      const r = flutterEngineCache.get('non-existent-id-12345');
      expect(r).assertNull();
    });

    it('getInstance returns same instance on second call', 0, () => {
      const a = FlutterEngineCache.getInstance();
      const b = FlutterEngineCache.getInstance();
      expect(a === b).assertEqual(true);
    });
  })
}
