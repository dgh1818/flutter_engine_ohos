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
import { describe, expect, it, MockKit, when } from '@ohos/hypium';
import Any from '../../../../../main/ets/plugin/common/Any';
import JSONMethodCodec from '../../../../../main/ets/plugin/common/JSONMethodCodec';
import MethodCall from '../../../../../main/ets/plugin/common/MethodCall';

export default function JSONMethodCodecUnitTest() {
  describe('JSONMethodCodec', () => {
    it("JSONMethodCodec_encodeMethodCall", 0, encodeMethodCall);

    it("JSONMethodCodec_encodeSuccessEnvelope", 0, encodeSuccessEnvelope);

    it("JSONMethodCodec_decodeEnvelope", 0, decodeEnvelope);

  })
}

function encodeMethodCall() {
  let mocker: MockKit = new MockKit();
  let ob = JSONMethodCodec.INSTANCE;
  let mockFunc: Object = mocker.mockFunc(ob, ob.encodeMethodCall);
  let params = new MethodCall("key", "value");
  let r = new ArrayBuffer(4096);
  when(mockFunc)(params).afterReturn(r);
  let result = ob.encodeMethodCall(params);
  expect(result).assertEqual(r);
}

function encodeSuccessEnvelope() {
  let mocker: MockKit = new MockKit();
  let ob = JSONMethodCodec.INSTANCE;
  let mockFunc: Object = mocker.mockFunc(ob, ob.encodeSuccessEnvelope);
  let r = new ArrayBuffer(4096);
  when(mockFunc)(null).afterReturn(r);
  expect(ob.encodeSuccessEnvelope(null)).assertEqual(r);
}

function decodeEnvelope() {
  let mocker: MockKit = new MockKit();
  let ob = JSONMethodCodec.INSTANCE;
  let mockFunc: Object = mocker.mockFunc(ob, ob.decodeEnvelope);
  let r = new ArrayBuffer(4096);
  when(mockFunc)(r).afterReturn("any");
  let result: Any = ob.decodeEnvelope(r);
  expect(result).assertEqual("any");
}