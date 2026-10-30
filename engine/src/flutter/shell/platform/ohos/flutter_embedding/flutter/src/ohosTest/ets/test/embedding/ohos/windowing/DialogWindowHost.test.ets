/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, expect, it, Level } from '@ohos/hypium';
import { DialogWindowHost } from
  '../../../../../../main/ets/embedding/ohos/windowing/DialogWindowHost';
import { WindowArchetype } from
  '../../../../../../main/ets/embedding/ohos/windowing/WindowingUtils';
import { WindowHostContext } from
  '../../../../../../main/ets/embedding/ohos/windowing/WindowHostContext';
import { FakeHostContext, dialogRequest } from './Fakes';

/** Concrete subclass exposing the protected hooks of DialogWindowHost. */
class TestDialogHost extends DialogWindowHost {
  constructor(ctx: WindowHostContext, viewId: number, parentViewId: number, w: number, h: number) {
    super(ctx, viewId, dialogRequest(parentViewId, w, h, ''));
  }
}

export default function dialogWindowHostTest() {
  describe('DialogWindowHost.constructor', () => {
    it('passes the DIALOG archetype', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      const host: TestDialogHost = new TestDialogHost(ctx, 1, 0, 100, 80);
      expect(host.archetype).assertEqual(WindowArchetype.DIALOG);
    });
  });
}
