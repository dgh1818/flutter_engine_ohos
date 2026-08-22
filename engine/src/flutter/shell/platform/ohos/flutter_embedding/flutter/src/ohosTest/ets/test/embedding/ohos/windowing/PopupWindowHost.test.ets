/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, expect, it, Level } from '@ohos/hypium';
import { PopupWindowHost } from
  '../../../../../../main/ets/embedding/ohos/windowing/PopupWindowHost';
import { WindowArchetype } from
  '../../../../../../main/ets/embedding/ohos/windowing/WindowArchetype';
import { WindowHostContext } from
  '../../../../../../main/ets/embedding/ohos/windowing/WindowHostContext';
import { FakeHostContext } from './Fakes';

export default function popupWindowHostTest() {
  describe('PopupWindowHost.constructor', () => {
    it('passes the POPUP archetype', Level.LEVEL0, () => {
      const ctx: WindowHostContext = new FakeHostContext();
      const host: PopupWindowHost = new PopupWindowHost(ctx, 1, 0, 100, 80);
      expect(host.archetype).assertEqual(WindowArchetype.POPUP);
      expect(host.viewId).assertEqual(1);
      expect(host.parentViewId).assertEqual(0);
    });
  });
}
