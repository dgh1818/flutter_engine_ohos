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
import SendableJSONMessageCodec from '../../../../../main/ets/plugin/common/SendableJSONMessageCodec';
import StringUtils from '../../../../../main/ets/util/StringUtils';

export default function SendableJSONMessageCodecTest() {
  describe('SendableJSONMessageCodecTest', () => {
    const codec = SendableJSONMessageCodec.INSTANCE;
    it('encodeMessage and decodeMessage with a null message', 0, ()  => {
      const originalMessage = null;
      const encodedMessage = codec.encodeMessage(originalMessage);
      const decodedMessage = StringUtils.arrayBufferToString(encodedMessage);
      expect(decodedMessage).assertEqual("");
    })
  })
}