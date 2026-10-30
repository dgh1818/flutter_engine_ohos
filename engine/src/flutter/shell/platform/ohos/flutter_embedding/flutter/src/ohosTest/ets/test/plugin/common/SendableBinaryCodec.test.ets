/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, Level } from '@ohos/hypium';
import SendableBinaryCodec from '../../../../../main/ets/plugin/common/SendableBinaryCodec';

export default function sendableBinaryCodecTest() {
  describe('SendableBinaryCodec.encodeMessage', () => {
    it('returns the same buffer instance (pass-through)', Level.LEVEL1, () => {
      const codec = new SendableBinaryCodec(false);
      const buf: ArrayBuffer = new ArrayBuffer(8);
      const result = codec.encodeMessage(buf);
      expect(result).assertEqual(buf);
    });

    it('works with direct mode codec', Level.LEVEL1, () => {
      const codec = new SendableBinaryCodec(true);
      const buf: ArrayBuffer = new ArrayBuffer(4);
      const result = codec.encodeMessage(buf);
      expect(result).assertEqual(buf);
    });
  });

  describe('SendableBinaryCodec.decodeMessage', () => {
    it('returns empty buffer when message is null (non-direct)', Level.LEVEL1, () => {
      const codec = new SendableBinaryCodec(false);
      const result = codec.decodeMessage(null);
      expect(result.byteLength).assertEqual(0);
    });

    it('returns empty buffer when message is null (direct)', Level.LEVEL1, () => {
      const codec = new SendableBinaryCodec(true);
      const result = codec.decodeMessage(null);
      expect(result.byteLength).assertEqual(0);
    });

    it('returns the same buffer instance in direct mode', Level.LEVEL1, () => {
      const codec = new SendableBinaryCodec(true);
      const buf: ArrayBuffer = new ArrayBuffer(8);
      const result = codec.decodeMessage(buf);
      expect(result).assertEqual(buf);
    });

    it('returns a copy in non-direct mode (slice)', Level.LEVEL1, () => {
      const codec = new SendableBinaryCodec(false);
      const buf: ArrayBuffer = new ArrayBuffer(8);
      const result = codec.decodeMessage(buf);
      expect(result).not().assertEqual(buf);
      expect(result.byteLength).assertEqual(8);
    });
  });

  describe('SendableBinaryCodec.round_trip', () => {
    it('round-trips in direct mode (same instance)', Level.LEVEL1, () => {
      const codec = new SendableBinaryCodec(true);
      const original: ArrayBuffer = new ArrayBuffer(3);
      const view: Uint8Array = new Uint8Array(original);
      view[0] = 10;
      view[1] = 20;
      view[2] = 30;
      const encoded = codec.encodeMessage(original);
      const decoded = codec.decodeMessage(encoded);
      expect(decoded).assertEqual(original);
    });

    it('round-trips in non-direct mode (same bytes)', Level.LEVEL1, () => {
      const codec = new SendableBinaryCodec(false);
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

  describe('SendableBinaryCodec.INSTANCE_DIRECT', () => {
    it('is a direct-mode codec', Level.LEVEL1, () => {
      const buf: ArrayBuffer = new ArrayBuffer(4);
      const result = SendableBinaryCodec.INSTANCE_DIRECT.decodeMessage(buf);
      expect(result).assertEqual(buf);
    });
  });
}
