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

import { describe, expect, it, Level } from '@ohos/hypium';
import { FlutterTextUtils } from '../../../../../main/ets/plugin/editing/TextUtils';

export default function TextUtilsTest() {
  describe('TextUtilsTest', () => {
    it('codePointBefore_returns_BMP_code_point', Level.LEVEL0, () => {
      const text = 'abc';
      expect(FlutterTextUtils.codePointBefore(text, 2)).assertEqual('b'.charCodeAt(0));
    });

    it('codePointBefore_handles_surrogate_pair_emoji', Level.LEVEL1, () => {
      const text = 'a' + String.fromCodePoint(0x1F600) + 'b';
      expect(FlutterTextUtils.codePointBefore(text, 3)).assertEqual(0x1F600);
    });

    it('codePointAt_returns_BMP_code_point', Level.LEVEL0, () => {
      const text = 'abc';
      expect(FlutterTextUtils.codePointAt(text, 1)).assertEqual('b'.charCodeAt(0));
    });

    it('codePointAt_handles_surrogate_pair_emoji', Level.LEVEL1, () => {
      const text = 'a' + String.fromCodePoint(0x1F600) + 'b';
      expect(FlutterTextUtils.codePointAt(text, 1)).assertEqual(0x1F600);
    });

    it('charCount_returns_1_for_BMP_code_points', Level.LEVEL0, () => {
      expect(FlutterTextUtils.charCount('a'.charCodeAt(0))).assertEqual(1);
    });

    it('charCount_returns_2_for_supplementary_code_points', Level.LEVEL1, () => {
      expect(FlutterTextUtils.charCount(0x1F600)).assertEqual(2);
    });

    it('getOffsetBefore_returns_previous_offset_for_ASCII_text', Level.LEVEL0, () => {
      const text = 'hello';
      expect(FlutterTextUtils.getOffsetBefore(text, 5)).assertEqual(4);
    });

    it('getOffsetBefore_handles_emoji_surrogate_pairs', Level.LEVEL1, () => {
      const text = 'a' + String.fromCodePoint(0x1F600) + 'b';
      expect(FlutterTextUtils.getOffsetBefore(text, 3)).assertEqual(1);
    });

    it('getOffsetAfter_returns_next_offset_for_ASCII_text', Level.LEVEL0, () => {
      const text = 'hello';
      expect(FlutterTextUtils.getOffsetAfter(text, 0)).assertEqual(1);
      expect(FlutterTextUtils.getOffsetAfter(text, 4)).assertEqual(5);
    });

    it('getOffsetAfter_handles_emoji_surrogate_pairs', Level.LEVEL1, () => {
      const text = String.fromCodePoint(0x1F600) + 'b';
      expect(FlutterTextUtils.getOffsetAfter(text, 0)).assertEqual(2);
    });

    it('getOffsetAfter_returns_length_at_last_position', Level.LEVEL0, () => {
      const text = 'abc';
      expect(FlutterTextUtils.getOffsetAfter(text, 2)).assertEqual(3);
      expect(FlutterTextUtils.getOffsetAfter(text, 3)).assertEqual(3);
    });

    it('getOffsetAfter_handles_empty_string', Level.LEVEL0, () => {
      const text = '';
      expect(FlutterTextUtils.getOffsetAfter(text, 0)).assertEqual(0);
    });

    it('getOffsetAfter_handles_single_character', Level.LEVEL0, () => {
      const text = 'a';
      expect(FlutterTextUtils.getOffsetAfter(text, 0)).assertEqual(1);
    });

    it('getOffsetAfter_handles_multiple_emojis', Level.LEVEL1, () => {
      const emoji1 = String.fromCodePoint(0x1F600);
      const emoji2 = String.fromCodePoint(0x1F601);
      const text = emoji1 + emoji2;
      expect(FlutterTextUtils.getOffsetAfter(text, 0)).assertEqual(2);
    });

    it('getOffsetAfter_handles_emoji_in_middle', Level.LEVEL1, () => {
      const emoji = String.fromCodePoint(0x1F600);
      const text = 'a' + emoji + 'b';
      expect(FlutterTextUtils.getOffsetAfter(text, 1)).assertEqual(3);
    });
  });
}
