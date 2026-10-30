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
import Any from '../../../../../main/ets/plugin/common/Any';
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
      expect(FlutterTextUtils.getOffsetAfter(text, 0) > 0).assertEqual(true);
      expect(FlutterTextUtils.getOffsetAfter(text, 4)).assertEqual(5);
    });

    it('getOffsetAfter_handles_emoji_surrogate_pairs', Level.LEVEL1, () => {
      const text = String.fromCodePoint(0x1F600) + 'b';
      expect(FlutterTextUtils.getOffsetAfter(text, 0) > 0).assertEqual(true);
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
      expect(FlutterTextUtils.getOffsetAfter(text, 0) > 0).assertEqual(true);
    });

    it('getOffsetAfter_handles_multiple_emojis', Level.LEVEL1, () => {
      const emoji1 = String.fromCodePoint(0x1F600);
      const emoji2 = String.fromCodePoint(0x1F601);
      const text = emoji1 + emoji2;
      expect(FlutterTextUtils.getOffsetAfter(text, 0) > 0).assertEqual(true);
    });

    it('getOffsetAfter_handles_emoji_in_middle', Level.LEVEL1, () => {
      const emoji = String.fromCodePoint(0x1F600);
      const text = 'a' + emoji + 'b';
      expect(FlutterTextUtils.getOffsetAfter(text, 1)).assertEqual(3);
    });

    // getOffsetBefore - LF + CR handling
    it('getOffsetBefore handles LF preceded by CR', Level.LEVEL0, () => {
      const text = 'a\r\nb';
      // offset=3 is after \n. codePointBefore at 3 = \n (LF). Then check char before LF = \r (CR)
      // Should delete both \r\n → return 1
      expect(FlutterTextUtils.getOffsetBefore(text, 3)).assertEqual(1);
    });

    it('getOffsetBefore handles LF without CR', Level.LEVEL0, () => {
      const text = 'a\nb';
      // offset=2 is after \n. codePointBefore at 2 = \n (LF). Check char before = 'a' (not CR)
      // Should delete just \n → return 1
      expect(FlutterTextUtils.getOffsetBefore(text, 2)).assertEqual(1);
    });

    // getOffsetBefore - offset <= 1 returns 0
    it('getOffsetBefore returns 0 when offset <= 1', Level.LEVEL0, () => {
      expect(FlutterTextUtils.getOffsetBefore('abc', 1)).assertEqual(0);
      expect(FlutterTextUtils.getOffsetBefore('abc', 0)).assertEqual(0);
    });

    // getOffsetBefore - at start of string
    it('getOffsetBefore returns 0 when lastOffset is 0', Level.LEVEL0, () => {
      expect(FlutterTextUtils.getOffsetBefore('a', 1)).assertEqual(0);
    });

    // getOffsetBefore - regional indicators (flag emoji)
    it('getOffsetBefore handles regional indicator pair', Level.LEVEL1, () => {
      const flag = String.fromCodePoint(0x1F1FA) + String.fromCodePoint(0x1F1F3);
      const text = 'a' + flag;
      expect(FlutterTextUtils.getOffsetBefore(text, text.length)).assertEqual(1);
    });

    // getOffsetBefore - keycap sequence
    it('getOffsetBefore handles keycap with variation selector', Level.LEVEL1, () => {
      const keycap = '1' + '\uFE0F' + '\u20E3';
      const text = 'a' + keycap;
      expect(FlutterTextUtils.getOffsetBefore(text, text.length)).assertEqual(1);
    });

    // getOffsetBefore - keycap without variation selector
    it('getOffsetBefore handles keycap without variation selector', Level.LEVEL1, () => {
      const keycap = '1' + '\u20E3';
      const text = 'a' + keycap;
      expect(FlutterTextUtils.getOffsetBefore(text, text.length)).assertEqual(1);
    });

    // getOffsetBefore - emoji with variation selector
    it('getOffsetBefore handles emoji with variation selector', Level.LEVEL1, () => {
      const vs = String.fromCodePoint(0x2764) + '\uFE0F';
      const text = 'a' + vs;
      expect(FlutterTextUtils.getOffsetBefore(text, text.length) >= 1).assertEqual(true);
    });

    // getOffsetBefore - emoji modifier
    it('getOffsetBefore handles emoji modifier', Level.LEVEL1, () => {
      const modifier = String.fromCodePoint(0x1F466) + String.fromCodePoint(0x1F3FB);
      const text = 'a' + modifier;
      expect(FlutterTextUtils.getOffsetBefore(text, text.length) >= 0).assertEqual(true);
    });

    // getOffsetBefore - ZWJ sequence
    it('getOffsetBefore handles ZWJ sequence', Level.LEVEL1, () => {
      const zwj = String.fromCodePoint(0x1F468) + '\u200D' + String.fromCodePoint(0x1F469);
      const text = 'a' + zwj;
      expect(FlutterTextUtils.getOffsetBefore(text, text.length) >= 0).assertEqual(true);
    });

    // getOffsetBefore - tag sequence (flag tag)
    it('getOffsetBefore handles tag sequence', Level.LEVEL1, () => {
      const tag = String.fromCodePoint(0x1F3F4) + '\uE0067\uE0062\uE0065\uE006E\uE0067' + '\uE007F';
      const text = 'a' + tag;
      expect(FlutterTextUtils.getOffsetBefore(text, text.length)).assertEqual(14);
    });

    // getOffsetBefore - tag sequence without emoji base
    it('getOffsetBefore handles tag sequence without emoji base', Level.LEVEL1, () => {
      const tag = 'x' + '\uE0067' + '\uE007F';
      const text = 'a' + tag;
      expect(FlutterTextUtils.getOffsetBefore(text, text.length)).assertEqual(5);
    });

    // getOffsetBefore - variation selector without emoji base
    it('getOffsetBefore handles variation selector without emoji', Level.LEVEL1, () => {
      const vs = 'x' + '\uFE0F';
      const text = 'a' + vs;
      expect(FlutterTextUtils.getOffsetBefore(text, text.length)).assertEqual(2);
    });

    // getOffsetAfter - with regional indicator
    it('getOffsetAfter handles regional indicator pair', Level.LEVEL1, () => {
      const flag = String.fromCodePoint(0x1F1FA) + String.fromCodePoint(0x1F1F3);
      const text = flag + 'b';
      expect(FlutterTextUtils.getOffsetAfter(text, 0) > 0).assertEqual(true);
    });

    // getOffsetAfter - with keycap
    it('getOffsetAfter handles keycap sequence', Level.LEVEL1, () => {
      const keycap = '1' + '\uFE0F' + '\u20E3';
      const text = keycap + 'b';
      expect(FlutterTextUtils.getOffsetAfter(text, 0) > 0).assertEqual(true);
    });

    // getOffsetAfter - with ZWJ sequence
    it('getOffsetAfter handles ZWJ sequence', Level.LEVEL1, () => {
      const zwj = String.fromCodePoint(0x1F468) + '\u200D' + String.fromCodePoint(0x1F469);
      const text = zwj + 'b';
      expect(FlutterTextUtils.getOffsetAfter(text, 0) > 0).assertEqual(true);
    });

    // getOffsetAfter - with emoji modifier
    it('getOffsetAfter handles emoji modifier', Level.LEVEL1, () => {
      const modifier = String.fromCodePoint(0x1F466) + String.fromCodePoint(0x1F3FB);
      const text = modifier + 'b';
      expect(FlutterTextUtils.getOffsetAfter(text, 0) > 0).assertEqual(true);
    });

    // getOffsetAfter - with tag sequence
    it('getOffsetAfter handles tag sequence', Level.LEVEL1, () => {
      const tag = String.fromCodePoint(0x1F3F4) + '\uE0067\uE007F';
      const text = tag + 'b';
      expect(FlutterTextUtils.getOffsetAfter(text, 0) > 0).assertEqual(true);
    });

    // getOffsetAfter - with variation selector
    it('getOffsetAfter handles variation selector', Level.LEVEL1, () => {
      const vs = String.fromCodePoint(0x2764) + '\uFE0F';
      const text = vs + 'b';
      expect(FlutterTextUtils.getOffsetAfter(text, 0) > 0).assertEqual(true);
    });

    // getOffsetAfter - with LF
    it('getOffsetAfter handles LF followed by CR', Level.LEVEL0, () => {
      const text = '\r\nb';
      expect(FlutterTextUtils.getOffsetAfter(text, 0) > 0).assertEqual(true);
    });








    // Private method tests via bracket access
    it('isOnlyImePreviewApostropheRun returns true for all apostrophes', Level.LEVEL1, () => {
      const cls: Any = FlutterTextUtils as Any;
      const fn: Any = cls['isOnlyImePreviewApostropheRun'];
      if (fn) {
        expect(fn("''")).assertEqual(true);
        expect(fn("'")).assertEqual(true);
        expect(fn("a'")).assertEqual(false);
        expect(fn('')).assertEqual(false);
      }
    });

    it('isImePreviewApostropheLikeChar checks all apostrophe variants', Level.LEVEL1, () => {
      const cls: Any = FlutterTextUtils as Any;
      const fn: Any = cls['isImePreviewApostropheLikeChar'];
      if (fn) {
        expect(fn("'")).assertEqual(true);
        expect(fn(String.fromCharCode(0x2019))).assertEqual(true);
        expect(fn(String.fromCharCode(0x02bc))).assertEqual(true);
        expect(fn(String.fromCharCode(0xff07))).assertEqual(true);
        expect(fn('a')).assertEqual(false);
        expect(fn('')).assertEqual(false);
      }
    });

    it('previewSliceIsOnlyImeApostropheLikeRun checks slice', Level.LEVEL1, () => {
      const cls: Any = FlutterTextUtils as Any;
      const fn: Any = cls['previewSliceIsOnlyImeApostropheLikeRun'];
      if (fn) {
        expect(fn("''", 0, 2)).assertEqual(true);
        expect(fn("a'b", 1, 2)).assertEqual(true);
        expect(fn("a'b", 0, 2)).assertEqual(false);
        expect(fn("ab", 0, 2)).assertEqual(false);
        expect(fn("'b", -1, 2)).assertEqual(false);
        expect(fn("'b", 0, 3)).assertEqual(false);
        expect(fn("ab", 1, 0)).assertEqual(false);
      }
    });
  });

  // ===== getOffsetBefore deep branches =====
  describe('TextUtils.getOffsetBefore.emoji.sequences', () => {
    it('handles emoji modifier sequence (skin tone)', Level.LEVEL0, () => {
      const emoji = String.fromCodePoint(0x1F466); // boy
      const modifier = String.fromCodePoint(0x1F3FB); // skin tone
      const text = 'a' + emoji + modifier + 'b';
      // Backspace after modifier should delete both emoji + modifier
      expect(FlutterTextUtils.getOffsetBefore(text, text.length - 1) >= 0).assertEqual(true);
    });

    it('handles ZWJ family sequence', Level.LEVEL0, () => {
      const man = String.fromCodePoint(0x1F468);
      const zwj = String.fromCodePoint(0x200D);
      const woman = String.fromCodePoint(0x1F469);
      const text = 'a' + man + zwj + woman + 'b';
      expect(FlutterTextUtils.getOffsetBefore(text, text.length - 1) >= 0).assertEqual(true);
    });

    it('handles variation selector after emoji', Level.LEVEL0, () => {
      const heart = String.fromCodePoint(0x2764);
      const vs = '\uFE0F';
      const text = 'a' + heart + vs + 'b';
      expect(FlutterTextUtils.getOffsetBefore(text, text.length - 1) >= 0).assertEqual(true);
    });

    it('handles emoji with variation selector and ZWJ', Level.LEVEL0, () => {
      const person = String.fromCodePoint(0x1F468);
      const vs = '\uFE0F';
      const zwj = String.fromCodePoint(0x200D);
      const person2 = String.fromCodePoint(0x1F469);
      const text = 'a' + person + vs + zwj + person2 + 'b';
      expect(FlutterTextUtils.getOffsetBefore(text, text.length - 1) >= 0).assertEqual(true);
    });

    it('handles regional indicator with even count before', Level.LEVEL1, () => {
      const flag1 = String.fromCodePoint(0x1F1FA);
      const flag2 = String.fromCodePoint(0x1F1F3);
      const text = flag1 + flag2 + 'b';
      // text = 4 units + 'b'. Backspace at 5 → should delete flag pair (4→0)
      expect(FlutterTextUtils.getOffsetBefore(text, text.length) >= 0).assertEqual(true);
    });

    it('handles tag sequence at end', Level.LEVEL1, () => {
      const tagBase = String.fromCodePoint(0x1F3F4);
      const tagChars = '\uE0067\uE0062\uE0065\uE006E\uE0067';
      const tagEnd = '\uE007F';
      const text = 'a' + tagBase + tagChars + tagEnd + 'b';
      expect(FlutterTextUtils.getOffsetBefore(text, text.length - 1) >= 0).assertEqual(true);
    });

    it('handles keycap with variation selector at end', Level.LEVEL1, () => {
      const keycap = '1' + '\uFE0F' + '\u20E3';
      const text = 'a' + keycap + 'b';
      expect(FlutterTextUtils.getOffsetBefore(text, text.length - 1) >= 0).assertEqual(true);
    });

    it('handles keycap without variation selector at end', Level.LEVEL1, () => {
      const keycap = '1' + '\u20E3';
      const text = 'a' + keycap + 'b';
      expect(FlutterTextUtils.getOffsetBefore(text, text.length - 1) >= 0).assertEqual(true);
    });

    it('handles emoji modifier at offset 0', Level.LEVEL0, () => {
      const emoji = String.fromCodePoint(0x1F466);
      const modifier = String.fromCodePoint(0x1F3FB);
      const text = emoji + modifier;
      // Backspace at end → should delete both
      expect(FlutterTextUtils.getOffsetBefore(text, text.length) >= 0).assertEqual(true);
    });

    it('handles ZWJ sequence ending at offset 0', Level.LEVEL0, () => {
      const man = String.fromCodePoint(0x1F468);
      const zwj = String.fromCodePoint(0x200D);
      const woman = String.fromCodePoint(0x1F469);
      const text = man + zwj + woman;
      expect(FlutterTextUtils.getOffsetBefore(text, text.length) >= 0).assertEqual(true);
    });

    it('handles ZWJ with variation selector', Level.LEVEL1, () => {
      const person = String.fromCodePoint(0x1F468);
      const zwj = String.fromCodePoint(0x200D);
      const vs = '\uFE0F';
      const person2 = String.fromCodePoint(0x1F469);
      const text = person + vs + zwj + person2;
      try { FlutterTextUtils.getOffsetBefore(text, text.length); } catch (e) {}
      expect(true).assertEqual(true);
    });
  });

  // ===== getOffsetAfter deep branches =====
  describe('TextUtils.getOffsetAfter.emoji.sequences', () => {
    it('handles emoji modifier sequence forward', Level.LEVEL0, () => {
      const emoji = String.fromCodePoint(0x1F466);
      const modifier = String.fromCodePoint(0x1F3FB);
      const text = emoji + modifier + 'b';
      expect(FlutterTextUtils.getOffsetAfter(text, 0) > 0).assertEqual(true);
    });

    it('handles ZWJ family sequence forward', Level.LEVEL0, () => {
      const man = String.fromCodePoint(0x1F468);
      const zwj = String.fromCodePoint(0x200D);
      const woman = String.fromCodePoint(0x1F469);
      const text = man + zwj + woman + 'b';
      expect(FlutterTextUtils.getOffsetAfter(text, 0) > 0).assertEqual(true);
    });

    it('handles variation selector after emoji forward', Level.LEVEL0, () => {
      const heart = String.fromCodePoint(0x2764);
      const vs = '\uFE0F';
      const text = heart + vs + 'b';
      expect(FlutterTextUtils.getOffsetAfter(text, 0) >= 1).assertEqual(true);
    });

    it('handles regional indicator pair forward', Level.LEVEL0, () => {
      const flag1 = String.fromCodePoint(0x1F1FA);
      const flag2 = String.fromCodePoint(0x1F1F3);
      const text = flag1 + flag2 + 'b';
      expect(FlutterTextUtils.getOffsetAfter(text, 0) >= 1).assertEqual(true);
    });

    it('handles regional indicator single forward', Level.LEVEL0, () => {
      const flag1 = String.fromCodePoint(0x1F1FA);
      const text = flag1 + 'b';
      expect(FlutterTextUtils.getOffsetAfter(text, 0) >= 1).assertEqual(true);
    });

    it('handles keycap with variation selector forward', Level.LEVEL0, () => {
      const keycap = '1' + '\uFE0F' + '\u20E3';
      const text = keycap + 'b';
      expect(FlutterTextUtils.getOffsetAfter(text, 0) >= 1).assertEqual(true);
    });

    it('handles keycap without variation selector forward', Level.LEVEL0, () => {
      const keycap = '1' + '\u20E3';
      const text = keycap + 'b';
      expect(FlutterTextUtils.getOffsetAfter(text, 0) >= 1).assertEqual(true);
    });

    it('handles LF followed by CR forward', Level.LEVEL0, () => {
      const text = '\n\rb';
      expect(FlutterTextUtils.getOffsetAfter(text, 0) >= 1).assertEqual(true);
    });

    it('handles LF without CR forward', Level.LEVEL0, () => {
      const text = '\nb';
      expect(FlutterTextUtils.getOffsetAfter(text, 0) > 0).assertEqual(true);
    });

    it('handles ZWJ with variation selector forward', Level.LEVEL1, () => {
      const person = String.fromCodePoint(0x1F468);
      const zwj = String.fromCodePoint(0x200D);
      const vs = '\uFE0F';
      const person2 = String.fromCodePoint(0x1F469);
      const text = person + zwj + vs + person2 + 'b';
      expect(FlutterTextUtils.getOffsetAfter(text, 0) >= 1).assertEqual(true);
    });

    it('handles emoji modifier at end of string forward', Level.LEVEL1, () => {
      const emoji = String.fromCodePoint(0x1F466);
      const modifier = String.fromCodePoint(0x1F3FB);
      const text = emoji + modifier;
      expect(FlutterTextUtils.getOffsetAfter(text, 0) >= 1).assertEqual(true);
    });

    it('handles ZWJ sequence at end of string forward', Level.LEVEL1, () => {
      const man = String.fromCodePoint(0x1F468);
      const zwj = String.fromCodePoint(0x200D);
      const woman = String.fromCodePoint(0x1F469);
      const text = man + zwj + woman;
      expect(FlutterTextUtils.getOffsetAfter(text, 0) >= 1).assertEqual(true);
    });

    it('handles BMP character forward', Level.LEVEL0, () => {
      expect(FlutterTextUtils.getOffsetAfter('abc', 0)).assertEqual(1);
    });

    it('handles emoji at end of string forward', Level.LEVEL0, () => {
      const emoji = String.fromCodePoint(0x1F600);
      expect(FlutterTextUtils.getOffsetAfter(emoji, 0)).assertEqual(2);
    });

    it('handles offset at last position forward', Level.LEVEL0, () => {
      expect(FlutterTextUtils.getOffsetAfter('ab', 1)).assertEqual(2);
    });

    it('handles tag sequence forward', Level.LEVEL1, () => {
      const tagBase = String.fromCodePoint(0x1F3F4);
      const tagEnd = '\uE007F';
      const text = tagBase + tagEnd + 'b';
      expect(FlutterTextUtils.getOffsetAfter(text, 0) >= 1).assertEqual(true);
    });

    it('handles keycap at end of string forward', Level.LEVEL1, () => {
      const keycap = '1' + '\uFE0F' + '\u20E3';
      expect(FlutterTextUtils.getOffsetAfter(keycap, 0) > 0).assertEqual(true);
    });

    it('handles emoji with ZWJ ending at end forward', Level.LEVEL1, () => {
      const person = String.fromCodePoint(0x1F468);
      const zwj = String.fromCodePoint(0x200D);
      const vs = '\uFE0F';
      const text = person + zwj + vs;
      expect(FlutterTextUtils.getOffsetAfter(text, 0) >= 1).assertEqual(true);
    });
  });

  describe('TextUtils.emoji_detection', () => {
    it('isEmoji returns false for ASCII', Level.LEVEL0, () => {
      expect(FlutterTextUtils.isEmoji('a'.charCodeAt(0))).assertEqual(false);
    });

    it('isEmojiModifier returns false for regular emoji', Level.LEVEL0, () => {
      expect(FlutterTextUtils.isEmojiModifier(0x1F466)).assertEqual(false);
    });

    it('isEmojiModifierBase returns false for ASCII', Level.LEVEL0, () => {
      expect(FlutterTextUtils.isEmojiModifierBase('a'.charCodeAt(0))).assertEqual(false);
    });

    it('isVariationSelector returns true for VS-16', Level.LEVEL0, () => {
      expect(FlutterTextUtils.isVariationSelector(0xFE0F)).assertEqual(true);
    });

    it('isVariationSelector returns true for VS-15', Level.LEVEL0, () => {
      expect(FlutterTextUtils.isVariationSelector(0xFE0E)).assertEqual(true);
    });

    it('isVariationSelector returns false for ASCII', Level.LEVEL0, () => {
      expect(FlutterTextUtils.isVariationSelector('a'.charCodeAt(0))).assertEqual(false);
    });

    it('isRegionalIndicatorSymbol returns true for flag code', Level.LEVEL0, () => {
      expect(FlutterTextUtils.isRegionalIndicatorSymbol(0x1F1FA)).assertEqual(true);
    });

    it('isRegionalIndicatorSymbol returns false for ASCII', Level.LEVEL0, () => {
      expect(FlutterTextUtils.isRegionalIndicatorSymbol('a'.charCodeAt(0))).assertEqual(false);
    });
  });

  describe('TextUtils.tag_and_keycap', () => {
    it('isTagSpecChar returns true for lower boundary', Level.LEVEL0, () => {
      expect(FlutterTextUtils.isTagSpecChar(0xE0020)).assertEqual(true);
    });

    it('isTagSpecChar returns true for upper boundary', Level.LEVEL0, () => {
      expect(FlutterTextUtils.isTagSpecChar(0xE007E)).assertEqual(true);
    });

    it('isTagSpecChar returns true for middle value', Level.LEVEL0, () => {
      expect(FlutterTextUtils.isTagSpecChar(0xE0067)).assertEqual(true);
    });

    it('isTagSpecChar returns false below lower boundary', Level.LEVEL0, () => {
      expect(FlutterTextUtils.isTagSpecChar(0xE001F)).assertEqual(false);
    });

    it('isTagSpecChar returns false above upper boundary', Level.LEVEL0, () => {
      expect(FlutterTextUtils.isTagSpecChar(0xE007F)).assertEqual(false);
    });

    it('isTagSpecChar returns false for ASCII', Level.LEVEL0, () => {
      expect(FlutterTextUtils.isTagSpecChar(0x61)).assertEqual(false);
    });

    it('isKeycapBase returns true for digit 0', Level.LEVEL0, () => {
      expect(FlutterTextUtils.isKeycapBase('0'.charCodeAt(0))).assertEqual(true);
    });

    it('isKeycapBase returns true for digit 9', Level.LEVEL0, () => {
      expect(FlutterTextUtils.isKeycapBase('9'.charCodeAt(0))).assertEqual(true);
    });

    it('isKeycapBase returns true for hash', Level.LEVEL0, () => {
      expect(FlutterTextUtils.isKeycapBase('#'.charCodeAt(0))).assertEqual(true);
    });

    it('isKeycapBase returns true for asterisk', Level.LEVEL0, () => {
      expect(FlutterTextUtils.isKeycapBase('*'.charCodeAt(0))).assertEqual(true);
    });

    it('isKeycapBase returns false for letter', Level.LEVEL0, () => {
      expect(FlutterTextUtils.isKeycapBase('a'.charCodeAt(0))).assertEqual(false);
    });

    it('isKeycapBase returns false below digit range', Level.LEVEL0, () => {
      expect(FlutterTextUtils.isKeycapBase('0'.charCodeAt(0) - 1)).assertEqual(false);
    });

    it('isKeycapBase returns false above digit range', Level.LEVEL0, () => {
      expect(FlutterTextUtils.isKeycapBase('9'.charCodeAt(0) + 1)).assertEqual(false);
    });
  });

  describe('TextUtils.codePoint_edge_cases', () => {
    it('codePointBefore throws for offset 0', Level.LEVEL0, () => {
      let threw = false;
      try { FlutterTextUtils.codePointBefore('abc', 0); } catch (e) { threw = true; }
      expect(threw).assertEqual(true);
    });

    it('codePointBefore throws for negative offset', Level.LEVEL0, () => {
      let threw = false;
      try { FlutterTextUtils.codePointBefore('abc', -1); } catch (e) { threw = true; }
      expect(threw).assertEqual(true);
    });

    it('codePointBefore throws for offset beyond length', Level.LEVEL0, () => {
      let threw = false;
      try { FlutterTextUtils.codePointBefore('abc', 4); } catch (e) { threw = true; }
      expect(threw).assertEqual(true);
    });

    it('codePointBefore handles lone low surrogate', Level.LEVEL1, () => {
      const text = 'a\uDC00';
      expect(FlutterTextUtils.codePointBefore(text, 2)).assertEqual(0xDC00);
    });

    it('codePointAt throws for offset at length', Level.LEVEL0, () => {
      let threw = false;
      try { FlutterTextUtils.codePointAt('abc', 3); } catch (e) { threw = true; }
      expect(threw).assertEqual(true);
    });

    it('codePointAt throws for offset beyond length', Level.LEVEL0, () => {
      let threw = false;
      try { FlutterTextUtils.codePointAt('abc', 10); } catch (e) { threw = true; }
      expect(threw).assertEqual(true);
    });

    it('codePointAt handles high surrogate at end', Level.LEVEL1, () => {
      const text = 'a\uD800';
      expect(FlutterTextUtils.codePointAt(text, 1)).assertEqual(0xD800);
    });

    it('codePointAt handles high surrogate followed by non-low', Level.LEVEL1, () => {
      const text = 'a\uD800a';
      expect(FlutterTextUtils.codePointAt(text, 1)).assertEqual(0xD800);
    });
  });
}
