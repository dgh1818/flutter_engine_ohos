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
import { TextEditingDelta } from '../../../../../main/ets/plugin/editing/TextEditingDelta';

export default function TextEditingDeltaTest() {
  describe('TextEditingDeltaTest', () => {
    it('testToJSON', 0, () => {
      const delta = new TextEditingDelta("Old Text", 0, 0, 0, 0, 2, 5, "Delta Text");
      const json = delta.toJSON();
      expect(json.oldText).assertEqual("Old Text");
      expect(json.deltaText).assertEqual("Delta Text");
      expect(json.deltaStart).assertEqual(2);
      expect(json.deltaEnd).assertEqual(5);
      expect(json.selectionBase).assertEqual(0);
      expect(json.selectionExtent).assertEqual(0);
      expect(json.composingBase).assertEqual(0);
      expect(json.composingExtent).assertEqual(0);
    })
  })
}