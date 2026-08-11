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
import { describe, expect, it } from '@ohos/hypium';
import Any from '../../../../../main/ets/plugin/common/Any';
import SendableStandardMessageCodec from '../../../../../main/ets/plugin/common/SendableStandardMessageCodec';

export default function SendableStandardMessageCodecTest() {
  describe('SendableStandardMessageCodecTest', () => {
    const codec = SendableStandardMessageCodec.INSTANCE;

    it('testEncodeDecodeNull', 0, () => {
      const encoded = codec.encodeMessage(null);
      const decoded:Any = codec.decodeMessage(encoded);
      expect(decoded).assertNull();
    })

    it('testEncodeDecodeBoolean', 0, () => {
      const encodedTrue = codec.encodeMessage(true);
      const decodedTrue:Any = codec.decodeMessage(encodedTrue);
      expect(decodedTrue).assertTrue();

      const encodedFalse = codec.encodeMessage(false);
      const decodedFalse:Any = codec.decodeMessage(encodedFalse);
      expect(decodedFalse).assertFalse();
    })

    it('testEncodeDecodeNumber', 0, () => {
      const int32Value = 123456;
      const encodedInt32 = codec.encodeMessage(int32Value);
      const decodedInt32:Any = codec.decodeMessage(encodedInt32);
      expect(decodedInt32).assertEqual(int32Value);

      const int64Value = BigInt(9007199254740991); // Max safe integer
      const encodedInt64 = codec.encodeMessage(int64Value);
      const decodedInt64:Any  = codec.decodeMessage(encodedInt64);
      expect(decodedInt64).assertEqual(int64Value);

      const float64Value = 3.14159;
      const encodedFloat64 = codec.encodeMessage(float64Value);
      const decodedFloat64:Any = codec.decodeMessage(encodedFloat64);
      expect(decodedFloat64).assertClose(float64Value, 5);

    })

    it('testEncodeDecodeUint8Array', 0, () => {
      const uint8Array = new Uint8Array([1, 2, 3, 4, 5]);
      const encodedArray = codec.encodeMessage(uint8Array);
      const decodedArray:Any  = codec.decodeMessage(encodedArray);
      expect(decodedArray).assertDeepEquals(uint8Array);
    })

    it('testEncodeDecodeInt32Array', 0, () => {
      const int32Array = new Int32Array([1, 2, 3, 4, 5]);
      const encodedArray = codec.encodeMessage(int32Array);
      const decodedArray:Any  = codec.decodeMessage(encodedArray);
      expect(decodedArray).assertDeepEquals(int32Array);
    })

    it('testEncodeDecodeBigInt64Array', 0, () => {
      const bigInt64Array = new BigInt64Array([BigInt(1), BigInt(2), BigInt(3), BigInt(4), BigInt(5)]);
      const encodedArray = codec.encodeMessage(bigInt64Array);
      const decodedArray:Any  = codec.decodeMessage(encodedArray);
      expect(decodedArray.length).assertEqual(bigInt64Array.length);
      for (let i = 0; i < bigInt64Array.length; i++) {
        expect(decodedArray[i]).assertEqual(bigInt64Array[i]);
      }
    })

    it('testEncodeDecodeFloat64Array', 0, () => {
      const float64Array = new Float64Array([1.1, 2.2, 3.3, 4.4, 5.5]);
      const encodedArray = codec.encodeMessage(float64Array);
      const decodedArray:Any  = codec.decodeMessage(encodedArray);
      expect(decodedArray).assertDeepEquals(float64Array);
    })
  })
}