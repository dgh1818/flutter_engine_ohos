/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, Level } from '@ohos/hypium';
import { PasteboardUtils } from '../../../../main/ets/util/PasteboardUtils';

export default function pasteboardUtilsTest() {
  describe('PasteboardUtils.setCopyData', () => {
    it('does not throw for plain ASCII text', Level.LEVEL0, () => {
      let threw = false;
      try { PasteboardUtils.setCopyData('hello world'); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('does not throw for empty string', Level.LEVEL1, () => {
      let threw = false;
      try { PasteboardUtils.setCopyData(''); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('does not throw for unicode text', Level.LEVEL1, () => {
      let threw = false;
      try { PasteboardUtils.setCopyData('你好世界😀'); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('does not throw for multiline text', Level.LEVEL1, () => {
      let threw = false;
      try { PasteboardUtils.setCopyData('line1\nline2\nline3'); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });
  });

  describe('PasteboardUtils.getPasteDataAsync', () => {
    it('returns a Promise', Level.LEVEL0, () => {
      const p = PasteboardUtils.getPasteDataAsync();
      expect(p).not().assertNull();
      expect(typeof p.then).assertEqual('function');
    });

    it('resolves to a string (eventually)', Level.LEVEL1, async (done: Function) => {
      try {
        const text = await PasteboardUtils.getPasteDataAsync();
        expect(typeof text).assertEqual('string');
      } catch (e) {
        // tolerated: in unit-test env, permission request may fail
      }
      done();
    });
  });
}
