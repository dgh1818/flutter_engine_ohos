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
import SendableJSONMethodCodec from '../../../../../main/ets/plugin/common/SendableJSONMethodCodec';

export default function SendableJSONMethodCodecTest() {
  describe('SendableJSONMethodCodecTest', () => {
    const codec = SendableJSONMethodCodec.INSTANCE;
    it('throws an error for invalid JSON', 0, () => {
      const invalidJSON = new ArrayBuffer(1);

      try {
        codec.decodeMethodCall(invalidJSON);
        //fail('Expected an error to be thrown');
        throw new Error('Expected an error to be thrown');
      } catch (e) {
        expect(e.message).assertContain('Invalid JSON')
      }

      try {
        codec.decodeEnvelope(invalidJSON);
        //fail('Expected an error to be thrown');
        throw new Error('Expected an error to be thrown');
      } catch (e) {
        expect(e.message).assertContain('Invalid JSON')
      }
    })
  })
}