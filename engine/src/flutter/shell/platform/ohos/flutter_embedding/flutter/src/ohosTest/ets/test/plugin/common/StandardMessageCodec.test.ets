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
import { describe, expect, it } from '@ohos/hypium'
import Any from '../../../../../main/ets/plugin/common/Any';
import StandardMessageCodec from '../../../../../main/ets/plugin/common/StandardMessageCodec';

export default function StandardMessageCodecTest() {
  describe('StandardMessageCodecTest', () => {
    const codec = StandardMessageCodec.INSTANCE;
    it('should encode and decode null', 0, () => {
      const encoded = codec.encodeMessage(null);
      const decoded:Any = codec.decodeMessage(encoded);
      expect(decoded).assertNull();
    })
    it('should encode and decode boolean', 0, () => {
      const encodedTrue = codec.encodeMessage(true);
      const decodedTrue:Any = codec.decodeMessage(encodedTrue);
      expect(decodedTrue).assertTrue();

      const encodedFalse = codec.encodeMessage(false);
      const decodedFalse:Any = codec.decodeMessage(encodedFalse);
      expect(decodedFalse).assertFalse();
    })
    it('should encode and decode number (int32)', 0, () => {
      const number = 123456;
      const encoded = codec.encodeMessage(number);
      const decoded:Any = codec.decodeMessage(encoded);
      expect(decoded).assertEqual(number);
    })
    it('should encode and decode number (int64)', 0, () => {
      const number = BigInt(1234567890123456789);
      const encoded = codec.encodeMessage(number);
      const decoded:Any = codec.decodeMessage(encoded);
      expect(decoded).assertEqual(number);
    })
    it('should encode and decode number (float64)', 0, ()=> {
      const number = 3.14159;;
      const encoded = codec.encodeMessage(number);
      const decoded:Any = codec.decodeMessage(encoded);
      expect(decoded).assertClose(number,5)
    })
    it('should encode and decode Uint8Array', 0, () => {
      const array = new Uint8Array([1, 2, 3, 4, 5]);
      const encoded = codec.encodeMessage(array);
      const decoded:Any = codec.decodeMessage(encoded);
      expect(decoded).assertDeepEquals(array);
    })
    it('should encode and decode Int32Array', 0, () => {
      const array = new Int32Array([1, 2, 3, 4, 5]);
      const encoded = codec.encodeMessage(array);
      const decoded:Any = codec.decodeMessage(encoded);
      expect(decoded).assertDeepEquals(array);
    })
    it('should encode and decode BigInt64Array', 0, () => {
      const array = new BigInt64Array([BigInt(1), BigInt(2), BigInt(3), BigInt(4), BigInt(5)]);
      const encoded = codec.encodeMessage(array);
      const decoded:Any = codec.decodeMessage(encoded);
      expect(decoded.length).assertEqual(array.length);
      for (let i = 0; i < array.length; i++) {
        expect(decoded[i]).assertEqual(array[i]);
      }
    })
    it('should encode and decode Float32Array', 0, () => {
      const array = new Float32Array([1.1, 2.2, 3.3, 4.4, 5.5]);
      const encoded = codec.encodeMessage(array);
      const decoded:Any = codec.decodeMessage(encoded);
      for (let i = 0; i < array.length; i++) {
        expect(decoded[i]).assertEqual(array[i]);
      }
    })
    it('should encode and decode Float64Array', 0, () => {
      const array = new Float64Array([1.1, 2.2, 3.3, 4.4, 5.5]);
      const encoded = codec.encodeMessage(array);
      const decoded:Any = codec.decodeMessage(encoded);
      for (let i = 0; i < array.length; i++) {
        expect(decoded[i]).assertEqual(array[i]);
      }
    })
  })
}