/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, Level } from '@ohos/hypium';
import { TraceSection } from '../../../../main/ets/util/TraceSection';
import Any from '../../../../main/ets/plugin/common/Any';

function repeatChar(ch: string, n: number): string {
  let s: string = '';
  for (let i = 0; i < n; i++) {
    s += ch;
  }
  return s;
}

export default function traceSectionTest() {
  describe('TraceSection.begin', () => {
    it('crops names over 124 to exactly 124 + ellipsis', Level.LEVEL1, () => {
      const longName = repeatChar('C', 130);
      // cropSectionName is private; verify the exact crop through the class object
      // (TraceSection.ets:27-29 truncates to 124 code units then appends "...").
      const cropped: string = (TraceSection as Any).cropSectionName(longName);
      expect(cropped).assertEqual(longName.substring(0, 124) + '...');
    });

    it('returns a positive taskId', Level.LEVEL1, () => {
      const id = TraceSection.begin('short');
      expect(id).assertLarger(0);
    });

    it('returns increasing taskIds across multiple begin calls', Level.LEVEL1, () => {
      const id1 = TraceSection.begin('first');
      const id2 = TraceSection.begin('second');
      const id3 = TraceSection.begin('third');
      expect(id2).assertLarger(id1);
      expect(id3).assertLarger(id2);
    });

    it('accepts a long section name without throwing (exercises crop else-branch)', Level.LEVEL1, () => {
      const longName = repeatChar('A', 200);
      let threw = false;
      try {
        TraceSection.begin(longName);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('accepts a section name of exactly 124 chars without throwing', Level.LEVEL1, () => {
      const name124 = repeatChar('B', 124);
      let threw = false;
      try {
        TraceSection.begin(name124);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('accepts a section name of exactly 125 chars without throwing (boundary)', Level.LEVEL1, () => {
      const name125 = repeatChar('C', 125);
      let threw = false;
      try {
        TraceSection.begin(name125);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('accepts an empty section name without throwing', Level.LEVEL1, () => {
      let threw = false;
      try {
        TraceSection.begin('');
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('TraceSection.end', () => {
    it('does not throw for a short name', Level.LEVEL1, () => {
      let threw = false;
      try {
        TraceSection.end('short');
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('does not throw for a long name (exercises crop else-branch)', Level.LEVEL1, () => {
      const longName = repeatChar('X', 300);
      let threw = false;
      try {
        TraceSection.end(longName);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('does not throw for an empty name', Level.LEVEL1, () => {
      let threw = false;
      try {
        TraceSection.end('');
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('TraceSection.endWithId', () => {
    it('does not throw with a specific taskId', Level.LEVEL1, () => {
      let threw = false;
      try {
        TraceSection.endWithId('short', 1);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('does not throw with taskId 0', Level.LEVEL1, () => {
      let threw = false;
      try {
        TraceSection.endWithId('short', 0);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('does not throw with a long name (exercises crop else-branch)', Level.LEVEL1, () => {
      const longName = repeatChar('Y', 500);
      let threw = false;
      try {
        TraceSection.endWithId(longName, 42);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('does not throw with an empty name', Level.LEVEL1, () => {
      let threw = false;
      try {
        TraceSection.endWithId('', 7);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });
}
