/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, Level } from '@ohos/hypium';
import Any from '../../../../../main/ets/plugin/common/Any';
import MethodCall from '../../../../../main/ets/plugin/common/MethodCall';

interface StringMap { name: string; age: string; }

export default function methodCallTest() {
  describe('MethodCall.constructor', () => {
    it('sets method and args', Level.LEVEL1, () => {
      const call = new MethodCall('foo', 'bar');
      expect(call.method).assertEqual('foo');
      expect(call.args).assertEqual('bar');
    });

    it('accepts null args', Level.LEVEL1, () => {
      const call = new MethodCall('foo', null);
      expect(call.method).assertEqual('foo');
      expect(call.args).assertEqual(null);
    });
  });

  describe('MethodCall.argument', () => {
    it('returns null when args is null', Level.LEVEL1, () => {
      const call = new MethodCall('get', null);
      const result: Any = call.argument('key');
      expect(result).assertEqual(null);
    });

    it('returns value from Map args', Level.LEVEL1, () => {
      const m = new Map<string, string>();
      m.set('k1', 'v1');
      m.set('k2', 'v2');
      const call = new MethodCall('get', m);
      expect(call.argument('k1')).assertEqual('v1');
      expect(call.argument('k2')).assertEqual('v2');
    });

    it('returns undefined for missing key in Map args', Level.LEVEL1, () => {
      const m = new Map<string, string>();
      m.set('k1', 'v1');
      const call = new MethodCall('get', m);
      const result: Any = call.argument('notPresent');
      expect(result).assertEqual(undefined);
    });

    it('returns value from Object args', Level.LEVEL1, () => {
      const obj: StringMap = { name: 'alice', age: '30' };
      const call = new MethodCall('get', obj);
      expect(call.argument('name')).assertEqual('alice');
    });

    it('returns undefined for missing key in Object args', Level.LEVEL1, () => {
      const obj: StringMap = { name: 'alice', age: '30' };
      const call = new MethodCall('get', obj);
      const result: Any = call.argument('notPresent');
      expect(result).assertEqual(undefined);
    });

    it('throws Error when args is primitive (not Map or Object)', Level.LEVEL1, () => {
      const call = new MethodCall('get', 42);
      let threw = false;
      try {
        call.argument('key');
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });

    it('throws Error when args is a string (not Map or Object)', Level.LEVEL1, () => {
      const call = new MethodCall('get', 'not a map');
      let threw = false;
      try {
        call.argument('key');
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });
  });

  describe('MethodCall.hasArgument', () => {
    it('returns false when args is null', Level.LEVEL1, () => {
      const call = new MethodCall('get', null);
      expect(call.hasArgument('key')).assertEqual(false);
    });

    it('returns true for present key in Map args', Level.LEVEL1, () => {
      const m = new Map<string, string>();
      m.set('k1', 'v1');
      const call = new MethodCall('get', m);
      expect(call.hasArgument('k1')).assertEqual(true);
    });

    it('returns false for missing key in Map args', Level.LEVEL1, () => {
      const m = new Map<string, string>();
      m.set('k1', 'v1');
      const call = new MethodCall('get', m);
      expect(call.hasArgument('notPresent')).assertEqual(false);
    });

    it('returns true for present key in Object args', Level.LEVEL1, () => {
      const obj: StringMap = { name: 'alice', age: '30' };
      const call = new MethodCall('get', obj);
      expect(call.hasArgument('name')).assertEqual(true);
    });

    it('returns false for missing key in Object args', Level.LEVEL1, () => {
      const obj: StringMap = { name: 'alice', age: '30' };
      const call = new MethodCall('get', obj);
      expect(call.hasArgument('notPresent')).assertEqual(false);
    });

    it('hasArgument throws Error when args is primitive (not Map or Object)', Level.LEVEL1, () => {
      const call = new MethodCall('get', 42);
      let threw = false;
      try {
        call.hasArgument('key');
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });
  });
}
