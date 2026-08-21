/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, expect, it, Level } from '@ohos/hypium';
import { WindowArchetype, archetypeFromInt } from
  '../../../../../../main/ets/embedding/ohos/windowing/WindowArchetype';

export default function windowArchetypeTest() {
  describe('WindowArchetype.enumValues', () => {
    it('REGULAR is 0', Level.LEVEL0, () => {
      expect(WindowArchetype.REGULAR).assertEqual(0);
    });

    it('DIALOG is 1', Level.LEVEL0, () => {
      expect(WindowArchetype.DIALOG).assertEqual(1);
    });

    it('TOOLTIP is 2', Level.LEVEL0, () => {
      expect(WindowArchetype.TOOLTIP).assertEqual(2);
    });

    it('POPUP is 3', Level.LEVEL0, () => {
      expect(WindowArchetype.POPUP).assertEqual(3);
    });
  });

  describe('WindowArchetype.archetypeFromInt', () => {
    it('DIALOG int maps to DIALOG', Level.LEVEL0, () => {
      expect(archetypeFromInt(WindowArchetype.DIALOG)).assertEqual(WindowArchetype.DIALOG);
    });

    it('TOOLTIP int maps to TOOLTIP', Level.LEVEL0, () => {
      expect(archetypeFromInt(WindowArchetype.TOOLTIP)).assertEqual(WindowArchetype.TOOLTIP);
    });

    it('POPUP int maps to POPUP', Level.LEVEL0, () => {
      expect(archetypeFromInt(WindowArchetype.POPUP)).assertEqual(WindowArchetype.POPUP);
    });

    it('REGULAR (0) int maps to REGULAR via default', Level.LEVEL0, () => {
      expect(archetypeFromInt(WindowArchetype.REGULAR)).assertEqual(WindowArchetype.REGULAR);
    });

    it('unknown int falls back to REGULAR', Level.LEVEL0, () => {
      expect(archetypeFromInt(99)).assertEqual(WindowArchetype.REGULAR);
    });
  });
}
