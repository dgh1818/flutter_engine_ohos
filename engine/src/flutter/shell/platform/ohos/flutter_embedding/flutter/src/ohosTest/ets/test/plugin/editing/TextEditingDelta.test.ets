/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, Level } from '@ohos/hypium';
import { TextEditingDelta } from '../../../../../main/ets/plugin/editing/TextEditingDelta';

export default function textEditingDeltaTest() {
  describe('TextEditingDelta.constructor.with_replacement', () => {
    it('stores all fields and serializes via toJSON', Level.LEVEL1, () => {
      const delta = new TextEditingDelta('Old Text', 0, 0, 0, 0, 2, 5, 'Delta Text');
      const json = delta.toJSON();
      expect(json.oldText).assertEqual('Old Text');
      expect(json.deltaText).assertEqual('Delta Text');
      expect(json.deltaStart).assertEqual(2);
      expect(json.deltaEnd).assertEqual(5);
      expect(json.selectionBase).assertEqual(0);
      expect(json.selectionExtent).assertEqual(0);
      expect(json.composingBase).assertEqual(0);
      expect(json.composingExtent).assertEqual(0);
    });

    it('preserves selection range', Level.LEVEL1, () => {
      const delta = new TextEditingDelta('text', 2, 5, -1, -1, 0, 2, 'XX');
      const json = delta.toJSON();
      expect(json.selectionBase).assertEqual(2);
      expect(json.selectionExtent).assertEqual(5);
      expect(json.composingBase).assertEqual(-1);
      expect(json.composingExtent).assertEqual(-1);
    });
  });

  describe('TextEditingDelta.constructor.without_replacement', () => {
    it('uses empty delta when replacementDestinationStart is undefined', Level.LEVEL1, () => {
      const delta = new TextEditingDelta('old text', 1, 2, -1, -1);
      const json = delta.toJSON();
      expect(json.oldText).assertEqual('old text');
      expect(json.deltaText).assertEqual('');
      expect(json.deltaStart).assertEqual(-1);
      expect(json.deltaEnd).assertEqual(-1);
    });

    it('uses empty delta when replacementDestinationEnd is undefined', Level.LEVEL1, () => {
      const delta = new TextEditingDelta('old text', 1, 2, -1, -1, 0);
      const json = delta.toJSON();
      expect(json.deltaText).assertEqual('');
      expect(json.deltaStart).assertEqual(-1);
    });

    it('uses empty delta when replacementSource is undefined', Level.LEVEL1, () => {
      const delta = new TextEditingDelta('old text', 1, 2, -1, -1, 0, 2);
      const json = delta.toJSON();
      expect(json.deltaText).assertEqual('');
      expect(json.deltaStart).assertEqual(-1);
    });
  });

  describe('TextEditingDelta.toJSON', () => {
    it('returns object with all required keys', Level.LEVEL1, () => {
      const delta = new TextEditingDelta('a', 0, 0, -1, -1, 0, 1, 'b');
      const json = delta.toJSON();
      expect(json.oldText != null).assertEqual(true);
      expect(json.deltaText != null).assertEqual(true);
      expect(typeof json.deltaStart).assertEqual('number');
      expect(typeof json.deltaEnd).assertEqual('number');
      expect(typeof json.selectionBase).assertEqual('number');
      expect(typeof json.selectionExtent).assertEqual('number');
      expect(typeof json.composingBase).assertEqual('number');
      expect(typeof json.composingExtent).assertEqual('number');
    });

    it('converts numbers to strings for oldText and deltaText', Level.LEVEL1, () => {
      const delta = new TextEditingDelta('hello', 0, 5, -1, -1, 0, 5, 'world');
      const json = delta.toJSON();
      expect(typeof json.oldText).assertEqual('string');
      expect(typeof json.deltaText).assertEqual('string');
    });
  });
}
