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
import StringCodec from '../../../../../main/ets/plugin/common/StringCodec';
import StringUtils from '../../../../../main/ets/util/StringUtils';

export default function StringCodecTest() {
  describe('StringCodecTest', () => {
    const codec = StringCodec.INSTANCE;
    it('encodeMessage_emptyString_ReturnEmptyString', 0, () => {
      const testString = "";
      const encodedMessage = codec.encodeMessage(testString);
      const decodedString = StringUtils.arrayBufferToString(encodedMessage);
      expect(decodedString).assertEqual(testString);
    })
    it('decodeMessage_emptyString_ReturnEmptyString', 0, () => {
      const encodedMessage = new ArrayBuffer(0);
      const decodedString = codec.decodeMessage(encodedMessage);
      expect(decodedString).assertEqual("");
    })
    it('decodeMessage_null_ReturnEmptyString', 0, () => {
      const decodedString = codec.decodeMessage(null);
      expect(decodedString).assertEqual("");
    })
  })
}