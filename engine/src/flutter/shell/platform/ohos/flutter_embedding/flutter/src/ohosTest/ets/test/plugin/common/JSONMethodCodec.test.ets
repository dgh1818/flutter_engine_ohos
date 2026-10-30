/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, Level, MockKit, when } from '@ohos/hypium';
import Any from '../../../../../main/ets/plugin/common/Any';
import JSONMessageCodec from '../../../../../main/ets/plugin/common/JSONMessageCodec';
import JSONMethodCodec from '../../../../../main/ets/plugin/common/JSONMethodCodec';
import FlutterException from '../../../../../main/ets/plugin/common/FlutterException';
import MethodCall from '../../../../../main/ets/plugin/common/MethodCall';
import StringUtils from '../../../../../main/ets/util/StringUtils';

export default function jsonMethodCodecTest() {
  const codec = JSONMethodCodec.INSTANCE;
  const msgCodec = JSONMessageCodec.INSTANCE;

  describe('JSONMethodCodec.encodeMethodCall', () => {
    it('encodes method name and args into JSON', Level.LEVEL1, () => {
      const call = new MethodCall('myMethod', 'myArg');
      const encoded = codec.encodeMethodCall(call);
      const json = StringUtils.arrayBufferToString(encoded);
      expect(json).assertContain('"method":"myMethod"');
      expect(json).assertContain('"args":"myArg"');
    });

    it('encodes method call with numeric args', Level.LEVEL1, () => {
      const call = new MethodCall('add', 42);
      const encoded = codec.encodeMethodCall(call);
      const json = StringUtils.arrayBufferToString(encoded);
      expect(json).assertContain('"method":"add"');
      expect(json).assertContain('"args":42');
    });

    it('produces a non-empty buffer', Level.LEVEL1, () => {
      const call = new MethodCall('foo', null);
      const encoded = codec.encodeMethodCall(call);
      expect(encoded.byteLength).assertLarger(0);
    });
  });

  describe('JSONMethodCodec.decodeMethodCall', () => {
    it('decodes a valid method call', Level.LEVEL1, () => {
      const call = new MethodCall('test', 'argValue');
      const encoded = codec.encodeMethodCall(call);
      const decoded = codec.decodeMethodCall(encoded);
      expect(decoded.method).assertEqual('test');
      expect(decoded.args).assertEqual('argValue');
    });

    it('decodes a method call with numeric args', Level.LEVEL1, () => {
      const call = new MethodCall('add', 100);
      const encoded = codec.encodeMethodCall(call);
      const decoded = codec.decodeMethodCall(encoded);
      expect(decoded.method).assertEqual('add');
      expect(decoded.args).assertEqual(100);
    });

    it('throws on invalid JSON', Level.LEVEL1, () => {
      const invalid = StringUtils.stringToArrayBuffer('not a json object');
      let threw = false;
      try {
        codec.decodeMethodCall(invalid);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });

    it('throws on JSON missing method field', Level.LEVEL1, () => {
      const bad = StringUtils.stringToArrayBuffer('{"args":"v"}');
      let threw = false;
      try {
        codec.decodeMethodCall(bad);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });
  });

  describe('JSONMethodCodec.encodeSuccessEnvelope', () => {
    it('encodes null result as JSON array with one null element', Level.LEVEL1, () => {
      const encoded = codec.encodeSuccessEnvelope(null);
      const json = StringUtils.arrayBufferToString(encoded);
      expect(json).assertEqual('[null]');
    });

    it('encodes a value result as JSON array with one element', Level.LEVEL1, () => {
      const encoded = codec.encodeSuccessEnvelope('resultValue');
      const json = StringUtils.arrayBufferToString(encoded);
      expect(json).assertEqual('["resultValue"]');
    });

    it('encodes a numeric result', Level.LEVEL1, () => {
      const encoded = codec.encodeSuccessEnvelope(42);
      const json = StringUtils.arrayBufferToString(encoded);
      expect(json).assertEqual('[42]');
    });
  });

  describe('JSONMethodCodec.encodeErrorEnvelope', () => {
    it('encodes code, message, details as 3-element array', Level.LEVEL1, () => {
      const encoded = codec.encodeErrorEnvelope('ERR_CODE', 'err msg', 'details');
      const json = StringUtils.arrayBufferToString(encoded);
      expect(json).assertEqual('["ERR_CODE","err msg","details"]');
    });

    it('encodes empty message and null details', Level.LEVEL1, () => {
      const encoded = codec.encodeErrorEnvelope('E', '', null);
      const json = StringUtils.arrayBufferToString(encoded);
      expect(json).assertEqual('["E","",null]');
    });
  });

  describe('JSONMethodCodec.encodeErrorEnvelopeWithStacktrace', () => {
    it('encodes code, message, details, stack as 4-element array', Level.LEVEL1, () => {
      const encoded = codec.encodeErrorEnvelopeWithStacktrace('E', 'm', 'd', 'stack-trace');
      const json = StringUtils.arrayBufferToString(encoded);
      expect(json).assertEqual('["E","m","d","stack-trace"]');
    });

    it('encodes empty stack trace', Level.LEVEL1, () => {
      const encoded = codec.encodeErrorEnvelopeWithStacktrace('E', 'm', 'd', '');
      const json = StringUtils.arrayBufferToString(encoded);
      expect(json).assertEqual('["E","m","d",""]');
    });
  });

  describe('JSONMethodCodec.decodeEnvelope', () => {
    it('returns the result for a success envelope (length 1)', Level.LEVEL1, () => {
      const encoded = codec.encodeSuccessEnvelope('payload');
      const decoded: Any = codec.decodeEnvelope(encoded);
      expect(decoded).assertEqual('payload');
    });

    it('returns null for a success envelope with null', Level.LEVEL1, () => {
      const encoded = codec.encodeSuccessEnvelope(null);
      const decoded: Any = codec.decodeEnvelope(encoded);
      const result: Any = decoded;
      expect(result).assertEqual(null);
    });

    it('returns numeric result for success envelope', Level.LEVEL1, () => {
      const encoded = codec.encodeSuccessEnvelope(99);
      const decoded: Any = codec.decodeEnvelope(encoded);
      expect(decoded).assertEqual(99);
    });

    it('throws FlutterException for an error envelope (length 3)', Level.LEVEL1, () => {
      const encoded = codec.encodeErrorEnvelope('CODE', 'message', 'details');
      try {
        codec.decodeEnvelope(encoded);
        expect(true).assertEqual(false); // unreachable: expected a throw
      } catch (e) {
        expect(e instanceof FlutterException).assertEqual(true);
        const fe: FlutterException = e as FlutterException;
        expect(fe.code).assertEqual('CODE');
        expect(fe.message).assertEqual('message');
        expect(fe.details).assertEqual('details');
      }
    });

    it('throws Error for an invalid envelope', Level.LEVEL1, () => {
      const bad = StringUtils.stringToArrayBuffer('"not an array"');
      let threw = false;
      try {
        codec.decodeEnvelope(bad);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });
  });
}

function encodeMethodCall() {
  let ob = JSONMethodCodec.INSTANCE;
  let params = new MethodCall("key", "value");
  let encoded = ob.encodeMethodCall(params);
  expect(encoded).not().assertNull();
  expect(encoded.byteLength).assertLarger(0);
  // 进一步验证：解码后与原始值一致
  let decoded: Object = ob.decodeEnvelope(ob.encodeSuccessEnvelope("value"));
  expect(decoded).not().assertNull();
}

function encodeSuccessEnvelope() {
  let mocker: MockKit = new MockKit();
  let ob = JSONMethodCodec.INSTANCE;
  let mockFunc: Function = mocker.mockFunc(ob, ob.encodeSuccessEnvelope);
  let r = new ArrayBuffer(4096);
  when(mockFunc)(null).afterReturn(r);
  expect(ob.encodeSuccessEnvelope(null)).assertEqual(r);
}

function decodeEnvelope() {
  let mocker: MockKit = new MockKit();
  let ob = JSONMethodCodec.INSTANCE;
  let mockFunc: Function = mocker.mockFunc(ob, ob.decodeEnvelope);
  let r = new ArrayBuffer(4096);
  when(mockFunc)(r).afterReturn("any");
  let result: Any = ob.decodeEnvelope(r);
  expect(result).assertEqual("any");
}
