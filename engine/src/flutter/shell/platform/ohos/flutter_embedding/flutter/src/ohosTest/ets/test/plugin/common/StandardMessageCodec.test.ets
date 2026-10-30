/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, Level } from '@ohos/hypium';
import Any from '../../../../../main/ets/plugin/common/Any';
import StandardMessageCodec from '../../../../../main/ets/plugin/common/StandardMessageCodec';

export default function standardMessageCodecTest() {
  const codec = StandardMessageCodec.INSTANCE;

  describe('StandardMessageCodec.encodeMessage.and.decodeMessage', () => {
    it('round-trips null', Level.LEVEL1, () => {
      const encoded = codec.encodeMessage(null);
      const decoded: Any = codec.decodeMessage(encoded);
      expect(decoded).assertNull();
    });

    it('round-trips boolean true', Level.LEVEL1, () => {
      const encoded = codec.encodeMessage(true);
      const decoded: Any = codec.decodeMessage(encoded);
      expect(decoded).assertTrue();
    });

    it('round-trips boolean false', Level.LEVEL1, () => {
      const encoded = codec.encodeMessage(false);
      const decoded: Any = codec.decodeMessage(encoded);
      expect(decoded).assertFalse();
    });

    it('round-trips int32 number', Level.LEVEL1, () => {
      const number = 123456;
      const encoded = codec.encodeMessage(number);
      const decoded: Any = codec.decodeMessage(encoded);
      expect(decoded).assertEqual(number);
    });

    it('round-trips int64 number (within safe integer range)', Level.LEVEL1, () => {
      const number = BigInt(1234567890123456789);
      const encoded = codec.encodeMessage(number);
      const decoded: Any = codec.decodeMessage(encoded);
      expect(decoded).assertEqual(number);
    });

    it('round-trips negative bigint outside int64 range', Level.LEVEL1, () => {
      const number = BigInt('-295147905179352825856'); // -2^68
      const encoded = codec.encodeMessage(number);
      const decoded: Any = codec.decodeMessage(encoded);
      expect(decoded).assertEqual(number);
    });

    it('round-trips float64', Level.LEVEL1, () => {
      const number = 3.14159;
      const encoded = codec.encodeMessage(number);
      const decoded: Any = codec.decodeMessage(encoded);
      expect(decoded).assertClose(number, 5);
    });

    it('round-trips negative int32', Level.LEVEL1, () => {
      const number = -1;
      const encoded = codec.encodeMessage(number);
      const decoded: Any = codec.decodeMessage(encoded);
      expect(decoded).assertEqual(number);
    });

    it('round-trips zero', Level.LEVEL1, () => {
      const encoded = codec.encodeMessage(0);
      const decoded: Any = codec.decodeMessage(encoded);
      expect(decoded).assertEqual(0);
    });

    it('round-trips string', Level.LEVEL1, () => {
      const original = 'hello world';
      const encoded = codec.encodeMessage(original);
      const decoded: Any = codec.decodeMessage(encoded);
      expect(decoded).assertEqual(original);
    });

    it('round-trips empty string', Level.LEVEL1, () => {
      const encoded = codec.encodeMessage('');
      const decoded: Any = codec.decodeMessage(encoded);
      expect(decoded).assertEqual('');
    });

    it('round-trips Unicode string', Level.LEVEL1, () => {
      const original = '你好世界';
      const encoded = codec.encodeMessage(original);
      const decoded: Any = codec.decodeMessage(encoded);
      expect(decoded).assertEqual(original);
    });

    it('round-trips Uint8Array', Level.LEVEL1, () => {
      const array = new Uint8Array([1, 2, 3, 4, 5]);
      const encoded = codec.encodeMessage(array);
      const decoded: Any = codec.decodeMessage(encoded);
      expect(decoded).assertDeepEquals(array);
    });

    it('round-trips Int32Array', Level.LEVEL1, () => {
      const array = new Int32Array([10, 20, 30]);
      const encoded = codec.encodeMessage(array);
      const decoded: Any = codec.decodeMessage(encoded);
      expect(decoded).assertDeepEquals(array);
    });

    it('round-trips BigInt64Array', Level.LEVEL1, () => {
      const array = new BigInt64Array([BigInt(1), BigInt(2), BigInt(3)]);
      const encoded = codec.encodeMessage(array);
      const decoded: Any = codec.decodeMessage(encoded);
      expect(decoded.length).assertEqual(3);
    });

    it('round-trips Float32Array', Level.LEVEL1, () => {
      const array = new Float32Array([1.1, 2.2, 3.3]);
      const encoded = codec.encodeMessage(array);
      const decoded: Any = codec.decodeMessage(encoded);
      expect(decoded.length).assertEqual(3);
    });

    it('round-trips Float64Array', Level.LEVEL1, () => {
      const array = new Float64Array([1.1, 2.2, 3.3]);
      const encoded = codec.encodeMessage(array);
      const decoded: Any = codec.decodeMessage(encoded);
      expect(decoded.length).assertEqual(3);
    });

    it('round-trips Array of mixed types', Level.LEVEL1, () => {
      const original: Array<Any> = new Array<Any>(1, 'two', true, null);
      const encoded = codec.encodeMessage(original);
      const decoded: Any = codec.decodeMessage(encoded);
      expect(decoded.length).assertEqual(4);
      expect(decoded[0]).assertEqual(1);
      expect(decoded[1]).assertEqual('two');
      expect(decoded[2]).assertEqual(true);
    });

    it('round-trips Map with string keys', Level.LEVEL1, () => {
      const m = new Map<string, number>();
      m.set('a', 1);
      m.set('b', 2);
      const encoded = codec.encodeMessage(m);
      const decoded: Any = codec.decodeMessage(encoded);
      expect(decoded.size).assertEqual(2);
      expect(decoded.get('a')).assertEqual(1);
      expect(decoded.get('b')).assertEqual(2);
    });

    it('round-trips empty Map', Level.LEVEL1, () => {
      const m = new Map<string, number>();
      const encoded = codec.encodeMessage(m);
      const decoded: Any = codec.decodeMessage(encoded);
      expect(decoded.size).assertEqual(0);
    });

    it('round-trips Map with mixed value types', Level.LEVEL1, () => {
      const m: Map<string, Any> = new Map<string, Any>();
      m.set('s', 'value');
      m.set('n', 42);
      m.set('b', true);
      const encoded = codec.encodeMessage(m);
      const decoded: Any = codec.decodeMessage(encoded);
      expect(decoded.size).assertEqual(3);
      const s: Any = decoded.get('s');
      const n: Any = decoded.get('n');
      const b: Any = decoded.get('b');
      expect(s).assertEqual('value');
      expect(n).assertEqual(42);
      expect(b).assertEqual(true);
    });

    it('round-trips a plain object', Level.LEVEL1, () => {
      const obj: Any = { key: 'value', num: 42 };
      const encoded = codec.encodeMessage(obj);
      const decoded: Any = codec.decodeMessage(encoded);
      expect(decoded.get('key')).assertEqual('value');
      expect(decoded.get('num')).assertEqual(42);
    });

    it('round-trips an empty array', Level.LEVEL1, () => {
      const arr: Array<number> = new Array<number>();
      const encoded = codec.encodeMessage(arr);
      const decoded: Any = codec.decodeMessage(encoded);
      expect(decoded.length).assertEqual(0);
    });

    it('round-trips nested array of arrays', Level.LEVEL1, () => {
      const inner1: Array<number> = new Array<number>(1, 2);
      const inner2: Array<number> = new Array<number>(3, 4);
      const outer: Array<Array<number>> = new Array<Array<number>>(inner1, inner2);
      const encoded = codec.encodeMessage(outer);
      const decoded: Any = codec.decodeMessage(encoded);
      expect(decoded.length).assertEqual(2);
      expect(decoded[0].length).assertEqual(2);
      expect(decoded[1].length).assertEqual(2);
    });
  });

  describe('StandardMessageCodec.decodeMessage', () => {
    it('returns null for null input', Level.LEVEL1, () => {
      const decoded: Any = codec.decodeMessage(null);
      expect(decoded).assertNull();
    });

    it('handles zero-length buffer (throws on read)', Level.LEVEL1, () => {
      // Note: source calls readValue() on a 0-length buffer which throws
      // 'getIndex + elementSize > viewSize'. Verify the throw.
      let threw = false;
      try {
        codec.decodeMessage(new ArrayBuffer(0));
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });
  });
}
