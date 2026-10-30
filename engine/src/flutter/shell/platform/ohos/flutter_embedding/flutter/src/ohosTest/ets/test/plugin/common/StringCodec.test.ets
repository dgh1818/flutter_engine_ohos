/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, Level } from '@ohos/hypium';
import StringCodec from '../../../../../main/ets/plugin/common/StringCodec';
import StringUtils from '../../../../../main/ets/util/StringUtils';

export default function stringCodecTest() {
  const codec = StringCodec.INSTANCE;

  describe('StringCodec.encodeMessage', () => {
    it('encodes empty string to empty buffer', Level.LEVEL1, () => {
      const encoded = codec.encodeMessage('');
      expect(encoded.byteLength).assertEqual(0);
    });

    it('encodes ASCII string with correct byte length', Level.LEVEL1, () => {
      const encoded = codec.encodeMessage('hello');
      expect(encoded.byteLength).assertEqual(5);
      expect(StringUtils.arrayBufferToString(encoded)).assertEqual('hello');
    });

    it('encodes Unicode string with multi-byte length', Level.LEVEL1, () => {
      const encoded = codec.encodeMessage('你好');
      expect(encoded.byteLength).assertEqual(6);
      expect(StringUtils.arrayBufferToString(encoded)).assertEqual('你好');
    });

    it('encodes single character', Level.LEVEL1, () => {
      const encoded = codec.encodeMessage('x');
      expect(encoded.byteLength).assertEqual(1);
    });

    it('encodes null as empty buffer', Level.LEVEL1, () => {
      // StringCodec's signature says `string` (non-null) but the source has a null check.
      // Use a wrapper to bypass ArkTS strict null-type checking.
      interface Wrapper { msg: string | null; }
      const wrapper: Wrapper = { msg: null };
      const encoded = codec.encodeMessage(wrapper.msg as string);
      expect(encoded.byteLength).assertEqual(0);
    });
  });

  describe('StringCodec.decodeMessage', () => {
    it('decodes null as empty string', Level.LEVEL1, () => {
      expect(codec.decodeMessage(null)).assertEqual('');
    });

    it('decodes empty buffer as empty string', Level.LEVEL1, () => {
      expect(codec.decodeMessage(new ArrayBuffer(0))).assertEqual('');
    });

    it('decodes buffer to original string', Level.LEVEL1, () => {
      const original = 'round trip';
      const encoded = codec.encodeMessage(original);
      expect(codec.decodeMessage(encoded)).assertEqual(original);
    });

    it('decodes Unicode buffer correctly', Level.LEVEL1, () => {
      const original = '你好世界';
      const encoded = codec.encodeMessage(original);
      expect(codec.decodeMessage(encoded)).assertEqual(original);
    });
  });

  describe('StringCodec.round_trip', () => {
    it('round-trips various strings', Level.LEVEL1, () => {
      const samples: Array<string> = new Array<string>('', 'a', 'hello world', '你好', 'mixed 你好 world');
      for (let i = 0; i < samples.length; i++) {
        const s = samples[i];
        expect(codec.decodeMessage(codec.encodeMessage(s))).assertEqual(s);
      }
    });
  });
}
