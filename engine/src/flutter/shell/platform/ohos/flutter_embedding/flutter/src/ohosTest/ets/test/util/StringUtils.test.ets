/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, Level } from '@ohos/hypium';
import StringUtils from '../../../../main/ets/util/StringUtils';

export default function stringUtilsTest() {
  describe('StringUtils.stringToArrayBuffer', () => {
    it('returns 0-length buffer for empty string (no NAPI call)', Level.LEVEL1, () => {
      const buf: ArrayBuffer = StringUtils.stringToArrayBuffer('');
      expect(buf.byteLength).assertEqual(0);
    });

    it('encodes ASCII string of correct length', Level.LEVEL1, () => {
      const buf: ArrayBuffer = StringUtils.stringToArrayBuffer('hello');
      expect(buf.byteLength).assertEqual(5);
    });

    it('encodes Unicode string to multi-byte buffer', Level.LEVEL1, () => {
      const buf: ArrayBuffer = StringUtils.stringToArrayBuffer('你好');
      expect(buf.byteLength).assertEqual(6);
    });

    it('encodes single ASCII character', Level.LEVEL1, () => {
      const buf: ArrayBuffer = StringUtils.stringToArrayBuffer('A');
      expect(buf.byteLength).assertEqual(1);
    });

    it('round-trips through arrayBufferToString for ASCII', Level.LEVEL1, () => {
      const original = 'Hello, world!';
      const buf: ArrayBuffer = StringUtils.stringToArrayBuffer(original);
      expect(StringUtils.arrayBufferToString(buf)).assertEqual(original);
    });

    it('round-trips through arrayBufferToString for Unicode', Level.LEVEL1, () => {
      const original = '你好世界';
      const buf: ArrayBuffer = StringUtils.stringToArrayBuffer(original);
      expect(StringUtils.arrayBufferToString(buf)).assertEqual(original);
    });
  });

  describe('StringUtils.arrayBufferToString', () => {
    it('returns empty string for 0-length buffer (no NAPI call)', Level.LEVEL1, () => {
      const buf: ArrayBuffer = new ArrayBuffer(0);
      expect(StringUtils.arrayBufferToString(buf)).assertEqual('');
    });

    it('decodes buffer back to original string', Level.LEVEL1, () => {
      const original = 'round trip';
      const buf: ArrayBuffer = StringUtils.stringToArrayBuffer(original);
      expect(StringUtils.arrayBufferToString(buf)).assertEqual(original);
    });

    it('decodes manually-constructed buffer of known bytes', Level.LEVEL1, () => {
      const bytes = new Uint8Array([72, 101, 108, 108, 111]);
      expect(StringUtils.uint8ArrayToString(bytes)).assertEqual('Hello');
    });
  });

  describe('StringUtils.uint8ArrayToString', () => {
    it('returns empty string for empty array (no NAPI call)', Level.LEVEL1, () => {
      const bytes = new Uint8Array(0);
      expect(StringUtils.uint8ArrayToString(bytes)).assertEqual('');
    });

    it('decodes ASCII bytes', Level.LEVEL1, () => {
      const bytes = new Uint8Array([65, 66, 67]);
      expect(StringUtils.uint8ArrayToString(bytes)).assertEqual('ABC');
    });

    it('decodes UTF-8 multi-byte sequence', Level.LEVEL1, () => {
      // '你' = 0xE4 0xBD 0xA0
      const bytes = new Uint8Array([0xE4, 0xBD, 0xA0]);
      expect(StringUtils.uint8ArrayToString(bytes)).assertEqual('你');
    });
  });

  describe('StringUtils.isNotEmpty', () => {
    it('returns true for non-empty string', Level.LEVEL1, () => {
      expect(StringUtils.isNotEmpty('hello')).assertEqual(true);
    });

    it('returns true for single character', Level.LEVEL1, () => {
      expect(StringUtils.isNotEmpty('x')).assertEqual(true);
    });

    it('returns false for empty string', Level.LEVEL1, () => {
      expect(StringUtils.isNotEmpty('')).assertEqual(false);
    });
  });

  describe('StringUtils.isEmpty', () => {
    it('returns true for empty string', Level.LEVEL1, () => {
      expect(StringUtils.isEmpty('')).assertEqual(true);
    });

    it('returns false for non-empty string', Level.LEVEL1, () => {
      expect(StringUtils.isEmpty('hello')).assertEqual(false);
    });

    it('returns false for single character', Level.LEVEL1, () => {
      expect(StringUtils.isEmpty('x')).assertEqual(false);
    });
  });
}
