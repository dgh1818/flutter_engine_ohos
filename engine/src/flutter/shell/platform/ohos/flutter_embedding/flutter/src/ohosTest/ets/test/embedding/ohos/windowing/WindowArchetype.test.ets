/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, expect, it, Level } from '@ohos/hypium';
import { WindowArchetype } from
  '../../../../../../main/ets/embedding/ohos/windowing/WindowingUtils';

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
}
