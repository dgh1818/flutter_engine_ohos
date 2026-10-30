/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

// 路径: flutter/src/ohosTest/ets/test/plugin/common/JSONMessageCodec.test.ets

import { describe, it, expect, Level } from '@ohos/hypium';
import Any from '../../../../../main/ets/plugin/common/Any';
import JSONMessageCodec from '../../../../../main/ets/plugin/common/JSONMessageCodec';
import StringUtils from '../../../../../main/ets/util/StringUtils';
import List from '@ohos.util.List';

interface StringKeyed { key: string; }

export default function jsonMessageCodecTest() {
  const codec = JSONMessageCodec.INSTANCE;

  describe('JSONMessageCodec.encodeMessage', () => {
    it('encodes null as empty buffer', Level.LEVEL1, () => {
      const encoded = codec.encodeMessage(null);
      expect(StringUtils.arrayBufferToString(encoded)).assertEqual('');
    });

    it('encodes a primitive string', Level.LEVEL1, () => {
      const encoded = codec.encodeMessage('hello');
      expect(StringUtils.arrayBufferToString(encoded)).assertEqual('"hello"');
    });

    it('encodes a primitive number', Level.LEVEL1, () => {
      const encoded = codec.encodeMessage(42);
      expect(StringUtils.arrayBufferToString(encoded)).assertEqual('42');
    });

    it('encodes a primitive boolean', Level.LEVEL1, () => {
      const encoded = codec.encodeMessage(true);
      expect(StringUtils.arrayBufferToString(encoded)).assertEqual('true');
    });

    it('encodes a simple object', Level.LEVEL1, () => {
      const obj: StringKeyed = { key: 'value' };
      const encoded = codec.encodeMessage(obj);
      const decoded = StringUtils.arrayBufferToString(encoded);
      expect(decoded).assertContain('"key"');
      expect(decoded).assertContain('"value"');
    });

    it('encodes an array', Level.LEVEL1, () => {
      const arr: Array<number> = new Array<number>(1, 2, 3);
      const encoded = codec.encodeMessage(arr);
      const decoded = StringUtils.arrayBufferToString(encoded);
      expect(decoded).assertEqual('[1,2,3]');
    });
  });

  describe('JSONMessageCodec.decodeMessage', () => {
    it('decodes null as empty buffer', Level.LEVEL1, () => {
      const decoded: Any = codec.decodeMessage(null);
      // Decode null returns a fresh empty ArrayBuffer; compare by byte length
      expect((decoded as ArrayBuffer).byteLength).assertEqual(0);
    });

    it('decodes a JSON string literal', Level.LEVEL1, () => {
      const encoded = StringUtils.stringToArrayBuffer('"hello"');
      const decoded: Any = codec.decodeMessage(encoded);
      expect(decoded).assertEqual('hello');
    });

    it('decodes a JSON number', Level.LEVEL1, () => {
      const encoded = StringUtils.stringToArrayBuffer('42');
      const decoded: Any = codec.decodeMessage(encoded);
      expect(decoded).assertEqual(42);
    });

    it('decodes a JSON object', Level.LEVEL1, () => {
      const encoded = StringUtils.stringToArrayBuffer('{"k":"v"}');
      const decoded: Any = codec.decodeMessage(encoded);
      // Re-encode and verify it matches the original JSON
      const reEncoded = codec.encodeMessage(decoded);
      const json = StringUtils.arrayBufferToString(reEncoded);
      expect(json).assertContain('"k"');
      expect(json).assertContain('"v"');
    });

    it('throws Error on invalid JSON', Level.LEVEL1, () => {
      const encoded = StringUtils.stringToArrayBuffer('{not valid');
      let threw = false;
      try {
        codec.decodeMessage(encoded);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });
  });

  describe('JSONMessageCodec.toBaseData', () => {
    it('returns null for null', Level.LEVEL1, () => {
      expect(codec.toBaseData(null)).assertEqual(null);
    });

    it('returns the value for a primitive string', Level.LEVEL1, () => {
      expect(codec.toBaseData('hello')).assertEqual('hello');
    });

    it('returns the value for a primitive number', Level.LEVEL1, () => {
      expect(codec.toBaseData(42)).assertEqual(42);
    });

    it('converts a Map to a plain object', Level.LEVEL1, () => {
      const m = new Map<string, string>();
      m.set('k', 'v');
      const result: Any = codec.toBaseData(m);
      const reEncoded = codec.encodeMessage(result);
      const json = StringUtils.arrayBufferToString(reEncoded);
      expect(json).assertContain('"k"');
      expect(json).assertContain('"v"');
    });

    it('converts an Array to a plain array', Level.LEVEL1, () => {
      const arr: Array<number> = new Array<number>(1, 2, 3);
      const result: Any = codec.toBaseData(arr);
      const reEncoded = codec.encodeMessage(result);
      const json = StringUtils.arrayBufferToString(reEncoded);
      expect(json).assertEqual('[1,2,3]');
    });

    it('converts a List to a plain array (covers List/LinkedList branch in toBaseData)', Level.LEVEL1, () => {
      const list: List<number> = new List<number>();
      list.add(1);
      list.add(2);
      list.add(3);
      const result: Any = codec.toBaseData(list);
      const reEncoded = codec.encodeMessage(result);
      const json = StringUtils.arrayBufferToString(reEncoded);
      expect(json).assertEqual('[1,2,3]');
    });
  });

  describe('JSONMessageCodec.round_trip', () => {
    it('round-trips a simple object', Level.LEVEL1, () => {
      const original: StringKeyed = { key: 'value' };
      const encoded = codec.encodeMessage(original);
      const decoded: Any = codec.decodeMessage(encoded);
      const reEncoded = codec.encodeMessage(decoded);
      expect(StringUtils.arrayBufferToString(reEncoded)).assertEqual(StringUtils.arrayBufferToString(encoded));
    });

    it('round-trips an array', Level.LEVEL1, () => {
      const original: Array<number> = new Array<number>(1, 2, 3);
      const encoded = codec.encodeMessage(original);
      const decoded: Any = codec.decodeMessage(encoded);
      const reEncoded = codec.encodeMessage(decoded);
      expect(StringUtils.arrayBufferToString(reEncoded)).assertEqual(StringUtils.arrayBufferToString(encoded));
    });
  });
}
