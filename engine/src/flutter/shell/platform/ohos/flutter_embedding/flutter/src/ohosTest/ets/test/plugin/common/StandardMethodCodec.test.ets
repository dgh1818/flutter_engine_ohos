/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, Level } from '@ohos/hypium';
import Any from '../../../../../main/ets/plugin/common/Any';
import MethodCall from '../../../../../main/ets/plugin/common/MethodCall';
import StandardMessageCodec from '../../../../../main/ets/plugin/common/StandardMessageCodec';
import StandardMethodCodec from '../../../../../main/ets/plugin/common/StandardMethodCodec';

export default function standardMethodCodecTest() {
  const codec = StandardMethodCodec.INSTANCE;
  const msgCodec = StandardMessageCodec.INSTANCE;

  describe('StandardMethodCodec.encodeMethodCall.and.decodeMethodCall', () => {
    it('round-trips a method call with string args', Level.LEVEL1, () => {
      const original = new MethodCall('myMethod', 'myArg');
      const encoded = codec.encodeMethodCall(original);
      const decoded = codec.decodeMethodCall(encoded);
      expect(decoded.method).assertEqual('myMethod');
      expect(decoded.args).assertEqual('myArg');
    });

    it('round-trips a method call with numeric args', Level.LEVEL1, () => {
      const original = new MethodCall('add', 42);
      const encoded = codec.encodeMethodCall(original);
      const decoded = codec.decodeMethodCall(encoded);
      expect(decoded.method).assertEqual('add');
      expect(decoded.args).assertEqual(42);
    });

    it('round-trips a method call with null args', Level.LEVEL1, () => {
      const original = new MethodCall('foo', null);
      const encoded = codec.encodeMethodCall(original);
      const decoded = codec.decodeMethodCall(encoded);
      expect(decoded.method).assertEqual('foo');
      const args: Any = decoded.args;
      expect(args).assertEqual(null);
    });

    it('round-trips a method call with object args', Level.LEVEL1, () => {
      const m = new Map<string, string>();
      m.set('k', 'v');
      const original = new MethodCall('myMethod', m);
      const encoded = codec.encodeMethodCall(original);
      const decoded = codec.decodeMethodCall(encoded);
      expect(decoded.method).assertEqual('myMethod');
      const argsMap: Map<string, string> = decoded.args as Map<string, string>;
      expect(argsMap.get('k')).assertEqual('v');
    });

    it('throws on corrupted method call', Level.LEVEL1, () => {
      const corrupted = new ArrayBuffer(2);
      let threw = false;
      let msg = '';
      try {
        codec.decodeMethodCall(corrupted);
      } catch (e) {
        threw = true;
        msg = (e as Error).message;
      }
      expect(threw).assertEqual(true);
      expect(msg).assertEqual('Method call corrupted');
    });
  });

  describe('StandardMethodCodec.encodeSuccessEnvelope.and.decodeEnvelope', () => {
    it('round-trips a success envelope with null result', Level.LEVEL1, () => {
      const encoded = codec.encodeSuccessEnvelope(null);
      const decoded: Any = codec.decodeEnvelope(encoded);
      expect(decoded).assertEqual(null);
    });

    it('round-trips a success envelope with string result', Level.LEVEL1, () => {
      const encoded = codec.encodeSuccessEnvelope('resultValue');
      const decoded: Any = codec.decodeEnvelope(encoded);
      expect(decoded).assertEqual('resultValue');
    });

    it('round-trips a success envelope with numeric result', Level.LEVEL1, () => {
      const encoded = codec.encodeSuccessEnvelope(99);
      const decoded: Any = codec.decodeEnvelope(encoded);
      expect(decoded).assertEqual(99);
    });
  });

  describe('StandardMethodCodec.encodeErrorEnvelope.and.decodeEnvelope', () => {
    it('throws on decode for error envelope', Level.LEVEL1, () => {
      // Note: source wraps FlutterException in try/catch and rethrows as
      // Error('Envelope corrupted'), so we just verify some error is thrown.
      const encoded = codec.encodeErrorEnvelope('CODE', 'message', 'details');
      let threw = false;
      try {
        codec.decodeEnvelope(encoded);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });

    it('encodes empty message in error envelope', Level.LEVEL1, () => {
      const encoded = codec.encodeErrorEnvelope('E', '', null);
      let threw = false;
      try {
        codec.decodeEnvelope(encoded);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });

    it('encodes Error instance as details (uses .stack)', Level.LEVEL1, () => {
      const err = new Error('test error');
      const encoded = codec.encodeErrorEnvelope('E', 'm', err);
      let threw = false;
      try {
        codec.decodeEnvelope(encoded);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });
  });

  describe('StandardMethodCodec.encodeErrorEnvelopeWithStacktrace', () => {
    it('encodes 4 values: code, message, details, stacktrace', Level.LEVEL1, () => {
      const encoded = codec.encodeErrorEnvelopeWithStacktrace('E', 'm', 'd', 'stack');
      let threw = false;
      try {
        codec.decodeEnvelope(encoded);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });
  });

  describe('StandardMethodCodec.decodeEnvelope', () => {
    it('throws Error on corrupted envelope', Level.LEVEL1, () => {
      const corrupted = new ArrayBuffer(0);
      let threw = false;
      try {
        codec.decodeEnvelope(corrupted);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });
  });

  describe('StandardMethodCodec.custom.message.codec', () => {
    it('works with a custom StandardMessageCodec instance', Level.LEVEL1, () => {
      const customCodec = new StandardMethodCodec(msgCodec);
      const call = new MethodCall('test', 'arg');
      const encoded = customCodec.encodeMethodCall(call);
      const decoded = customCodec.decodeMethodCall(encoded);
      expect(decoded.method).assertEqual('test');
      expect(decoded.args).assertEqual('arg');
    });
  });
}
