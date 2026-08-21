/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, expect, it, Level } from '@ohos/hypium';
import window from '@ohos.window';
import { TooltipWindowHost } from
  '../../../../../../main/ets/embedding/ohos/windowing/TooltipWindowHost';
import { WindowArchetype } from
  '../../../../../../main/ets/embedding/ohos/windowing/WindowArchetype';
import { WindowHostContext } from
  '../../../../../../main/ets/embedding/ohos/windowing/WindowHostContext';
import { FakeWindow, FakeHostContext } from './Fakes';

/** Concrete subclass exposing TooltipWindowHost's protected hooks. */
class TestTooltipHost extends TooltipWindowHost {
  constructor(ctx: WindowHostContext, viewId: number, parentViewId: number, w: number, h: number) {
    super(ctx, viewId, parentViewId, w, h);
  }

  defaultLogicalSizeForTest(): number[] {
    return this.defaultLogicalSize();
  }

  async prepareShowForTest(subWindow: window.Window): Promise<void> {
    await this.prepareShow(subWindow);
  }
}

export default function tooltipWindowHostTest() {
  describe('TooltipWindowHost.constructor', () => {
    it('passes the TOOLTIP archetype', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      const host: TestTooltipHost = new TestTooltipHost(ctx, 1, 0, 100, 80);
      expect(host.archetype).assertEqual(WindowArchetype.TOOLTIP);
    });
  });

  describe('TooltipWindowHost.defaultLogicalSize', () => {
    it('defaults to 400x200', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      const host: TestTooltipHost = new TestTooltipHost(ctx, 2, 0, 100, 80);
      const def = host.defaultLogicalSizeForTest();
      expect(def.length).assertEqual(2);
      expect(def[0]).assertEqual(400);
      expect(def[1]).assertEqual(200);
    });
  });

  describe('TooltipWindowHost.prepareShow', () => {
    it('disables focusability', Level.LEVEL0, async (done: Function) => {
      const ctx = new FakeHostContext();
      const fake = new FakeWindow();
      const host: TestTooltipHost = new TestTooltipHost(ctx, 3, 0, 100, 80);
      await host.prepareShowForTest(fake as ESObject as window.Window);
      expect(fake.focusable).assertFalse();
      done();
    });

    it('logs a rejected focusable set', Level.LEVEL0, async (done: Function) => {
      const ctx = new FakeHostContext();
      const fake = new FakeWindow();
      fake.rejectFocusable = true;
      const host: TestTooltipHost = new TestTooltipHost(ctx, 4, 0, 100, 80);
      await host.prepareShowForTest(fake as ESObject as window.Window);
      expect(fake.focusable).assertTrue();
      done();
    });
  });
}
