/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, Level } from '@ohos/hypium';
import BinaryCodec from '../../../../../main/ets/plugin/common/BinaryCodec';

export default function binaryCodecTest() {
  describe('BinaryCodec.encodeMessage', () => {
    it('returns the same buffer instance (pass-through)', Level.LEVEL1, () => {
      const codec = new BinaryCodec(false);
      const buf = new ArrayBuffer(8);
      const result = codec.encodeMessage(buf);
      expect(result).assertEqual(buf);
    });

    it('works with direct mode codec', Level.LEVEL1, () => {
      const codec = new BinaryCodec(true);
      const buf = new ArrayBuffer(4);
      const result = codec.encodeMessage(buf);
      expect(result).assertEqual(buf);
    });

    it('works with empty buffer', Level.LEVEL1, () => {
      const codec = new BinaryCodec(false);
      const buf = new ArrayBuffer(0);
      const result = codec.encodeMessage(buf);
      expect(result.byteLength).assertEqual(0);
    });
  });

  describe('BinaryCodec.decodeMessage', () => {
    it('returns empty buffer when message is null', Level.LEVEL1, () => {
      const codec = new BinaryCodec(false);
      const result = codec.decodeMessage(null);
      expect(result.byteLength).assertEqual(0);
    });

    it('returns empty buffer for null in direct mode', Level.LEVEL1, () => {
      const codec = new BinaryCodec(true);
      const result = codec.decodeMessage(null);
      expect(result.byteLength).assertEqual(0);
    });

    it('returns the same buffer instance in direct mode', Level.LEVEL1, () => {
      const codec = new BinaryCodec(true);
      const buf = new ArrayBuffer(8);
      const result = codec.decodeMessage(buf);
      expect(result).assertEqual(buf);
    });

    it('returns a copy in non-direct mode (slice)', Level.LEVEL1, () => {
      const codec = new BinaryCodec(false);
      const buf = new ArrayBuffer(8);
      const result = codec.decodeMessage(buf);
      // result should be a different instance (slice)
      expect(result).not().assertEqual(buf);
      // but same byte length
      expect(result.byteLength).assertEqual(8);
    });

    it('returns same byte content regardless of mode', Level.LEVEL1, () => {
      const original: Uint8Array = new Uint8Array([1, 2, 3, 4, 5]);
      const buf: ArrayBuffer = new ArrayBuffer(5);
      const view: Uint8Array = new Uint8Array(buf);
      for (let i = 0; i < 5; i++) {
        view[i] = original[i];
      }
      const directCodec: BinaryCodec = new BinaryCodec(true);
      const sliceCodec: BinaryCodec = new BinaryCodec(false);
      const directBuf: ArrayBuffer = directCodec.decodeMessage(buf);
      const sliceBuf: ArrayBuffer = sliceCodec.decodeMessage(buf);
      const directResult: Uint8Array = new Uint8Array(directBuf);
      const sliceResult: Uint8Array = new Uint8Array(sliceBuf);
      expect(directResult.length).assertEqual(5);
      expect(sliceResult.length).assertEqual(5);
      for (let i = 0; i < 5; i++) {
        expect(directResult[i]).assertEqual(original[i]);
        expect(sliceResult[i]).assertEqual(original[i]);
      }
    });
  });

  describe('BinaryCodec.round_trip', () => {
    it('round-trips through encode + decode (direct mode)', Level.LEVEL1, () => {
      const codec = new BinaryCodec(true);
      const original: ArrayBuffer = new ArrayBuffer(3);
      const origView: Uint8Array = new Uint8Array(original);
      origView[0] = 10;
      origView[1] = 20;
      origView[2] = 30;
      const encoded = codec.encodeMessage(original);
      const decoded = codec.decodeMessage(encoded);
      expect(decoded).assertEqual(original);
    });

    it('round-trips through encode + decode (slice mode)', Level.LEVEL1, () => {
      const codec = new BinaryCodec(false);
      const original: ArrayBuffer = new ArrayBuffer(3);
      const origView: Uint8Array = new Uint8Array(original);
      origView[0] = 10;
      origView[1] = 20;
      origView[2] = 30;
      const encoded = codec.encodeMessage(original);
      const decoded = codec.decodeMessage(encoded);
      const originalBytes: Uint8Array = new Uint8Array(original);
      const decodedBytes: Uint8Array = new Uint8Array(decoded);
      for (let i = 0; i < originalBytes.length; i++) {
        expect(decodedBytes[i]).assertEqual(originalBytes[i]);
      }
    });
  });

  describe('BinaryCodec.INSTANCE_DIRECT', () => {
    it('is a direct-mode codec', Level.LEVEL1, () => {
      const buf = new ArrayBuffer(4);
      const result = BinaryCodec.INSTANCE_DIRECT.decodeMessage(buf);
      expect(result).assertEqual(buf);
    });
  });
}
