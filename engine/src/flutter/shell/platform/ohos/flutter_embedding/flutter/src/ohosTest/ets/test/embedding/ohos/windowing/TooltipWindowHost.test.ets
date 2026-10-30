/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, expect, it, Level } from '@ohos/hypium';
import window from '@ohos.window';
import { TooltipWindowHost, BirthPlacement } from
  '../../../../../../main/ets/embedding/ohos/windowing/TooltipWindowHost';
import { WindowPositionResult } from
  '../../../../../../main/ets/embedding/engine/FlutterNapi';
import { WindowArchetype } from
  '../../../../../../main/ets/embedding/ohos/windowing/WindowingUtils';
import { WindowHostContext } from
  '../../../../../../main/ets/embedding/ohos/windowing/WindowHostContext';
import { FakeWindow, FakeHostContext, FakeRect, sleep, tooltipRequest } from './Fakes';

/** Concrete subclass exposing the protected hooks. */
class TestTooltipHost extends TooltipWindowHost {
  constructor(ctx: WindowHostContext, viewId: number, parentViewId: number) {
    super(ctx, viewId, tooltipRequest(parentViewId));
  }

  // ---- computePlacementRect seams ----

  testDpr: number = 2.0;

  /** 0 → returns placePhysical (PHYSICAL px); else "no positioner". */
  placementRc: number = 1;
  placePhysical: window.Rect = { left: 160, top: 240, width: 250, height: 100 };

  /** Positioner args per call (child W/H, parent L/T/W/H, work L/T/W/H —
   * all PHYSICAL). */
  placementArgs: Array<number> = new Array<number>();
  placementCalls: number = 0;
  workAreaPhysicalRect: window.Rect = { left: 0, top: 0, width: 1000, height: 800 };

  protected computePlacement(pos: WindowPositionResult,
                             childSize: window.Size,
                             parentRect: window.Rect,
                             workArea: window.Rect): boolean {
    this.placementArgs = [childSize.width, childSize.height,
      parentRect.left, parentRect.top, parentRect.width, parentRect.height,
      workArea.left, workArea.top, workArea.width, workArea.height];
    this.placementCalls += 1;
    if (this.placementRc === 0) {
      pos.set(this.placePhysical.left, this.placePhysical.top,
        this.placePhysical.width, this.placePhysical.height);
      return true;
    }
    return false;
  }

  protected async workAreaPhysical(): Promise<window.Rect> {
    return this.workAreaPhysicalRect;
  }

  protected densityPixels(_parentWnd: window.Window | undefined): number {
    return this.testDpr;
  }

  async placementRectForTest(parentWnd: window.Window,
                             logical: window.Size): Promise<BirthPlacement> {
    return this.computePlacementRect(parentWnd, logical);
  }
}

// Parent-move tracking / policy close is deliberately NOT host-side; the
// window's lifetime policy belongs to the framework.

/** Decorated parent: frame (100,200 1000x800) + (10,20) inset → content
 * rect (110,220 980x760) in physical px. */
function decoratedParent(): FakeWindow {
  const fake = new FakeWindow();
  fake.props.windowRect = new FakeRect(100, 200, 1000, 800) as ESObject as window.Rect;
  fake.props.drawableRect = new FakeRect(10, 20, 980, 760) as ESObject as window.Rect;
  return fake;
}

/** Host with `own` as its OS window and a view-0 parent host. */
function mountedHost(ctx: FakeHostContext, own: FakeWindow,
                     parent: FakeWindow, viewId: number): TestTooltipHost {
  const parentHost = new TestTooltipHost(ctx, 0, 0);
  parentHost.setWindow(parent as ESObject as window.Window);
  ctx.hosts.set('0', parentHost);
  const host = new TestTooltipHost(ctx, viewId, 0);
  host.setWindow(own as ESObject as window.Window);
  return host;
}

export default function tooltipWindowHostTest() {
  describe('TooltipWindowHost.constructor', () => {
    it('passes the TOOLTIP archetype', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      const host: TestTooltipHost = new TestTooltipHost(ctx, 1, 0);
      expect(host.archetype).assertEqual(WindowArchetype.TOOLTIP);
    });
  });

  describe('TooltipWindowHost.computePlacementRect', () => {
    it('anchors at the positioner result verbatim (physical domain)',
      Level.LEVEL0, async (done: Function) => {
        const ctx = new FakeHostContext();
        const parent = decoratedParent();
        const host = new TestTooltipHost(ctx, 5, 0);
        host.placementRc = 0;
        const birth = await host.placementRectForTest(
          parent as ESObject as window.Window, { width: 400, height: 200 });
        expect(birth.placed).assertTrue();
        expect(birth.rect.left).assertEqual(160);
        expect(birth.rect.top).assertEqual(240);
        expect(birth.rect.width).assertEqual(250);
        expect(birth.rect.height).assertEqual(100);
        done();
      });

    it('feeds the positioner funnel-physical size + parent content rect + work area, all physical',
      Level.LEVEL0, async (done: Function) => {
        const ctx = new FakeHostContext();
        const parent = decoratedParent();
        const host = new TestTooltipHost(ctx, 6, 0);
        host.placementRc = 0;
        await host.placementRectForTest(
          parent as ESObject as window.Window, { width: 400, height: 200 });
        const a = host.placementArgs;
        expect(a.length).assertEqual(10);
        expect(a[0]).assertEqual(800);   // funnel 400x200 × dpr 2
        expect(a[1]).assertEqual(400);
        expect(a[2]).assertEqual(110);   // content rect, physical
        expect(a[3]).assertEqual(220);
        expect(a[4]).assertEqual(980);
        expect(a[5]).assertEqual(760);
        expect(a[6]).assertEqual(0);     // work area, physical
        expect(a[7]).assertEqual(0);
        expect(a[8]).assertEqual(1000);
        expect(a[9]).assertEqual(800);
        done();
      });

    it('keeps default system placement when there is no positioner (rc !== 0)',
      Level.LEVEL0, async (done: Function) => {
        const ctx = new FakeHostContext();
        const parent = decoratedParent();
        const host = new TestTooltipHost(ctx, 7, 0);
        host.placementRc = 1;
        const birth = await host.placementRectForTest(
          parent as ESObject as window.Window, { width: 400, height: 200 });
        expect(birth.placed).assertFalse();
        expect(birth.rect.left).assertEqual(0);
        expect(birth.rect.top).assertEqual(0);
        expect(birth.rect.width).assertEqual(800);  // funnel × dpr
        expect(birth.rect.height).assertEqual(400);
        expect(host.placementArgs[0]).assertEqual(800);
        expect(host.placementArgs[1]).assertEqual(400);
        done();
      });

    it('applies a positioner shrink to the birth size (win32 rect verbatim)',
      Level.LEVEL0, async (done: Function) => {
        const ctx = new FakeHostContext();
        const parent = decoratedParent();
        const host = new TestTooltipHost(ctx, 8, 0);
        host.placementRc = 0;
        host.placePhysical = { left: 160, top: 240, width: 120, height: 80 };
        const birth = await host.placementRectForTest(
          parent as ESObject as window.Window, { width: 400, height: 200 });
        expect(birth.placed).assertTrue();
        expect(birth.rect.width).assertEqual(120);
        expect(birth.rect.height).assertEqual(80);
        done();
      });

    it('keeps the funnel size when the positioner returns degenerate dims',
      Level.LEVEL0, async (done: Function) => {
        const ctx = new FakeHostContext();
        const parent = decoratedParent();
        const host = new TestTooltipHost(ctx, 9, 0);
        host.placementRc = 0;
        host.placePhysical = { left: 160, top: 240, width: 0, height: 0 };
        const birth = await host.placementRectForTest(
          parent as ESObject as window.Window, { width: 400, height: 200 });
        expect(birth.placed).assertTrue();
        expect(birth.rect.left).assertEqual(160);
        expect(birth.rect.top).assertEqual(240);
        expect(birth.rect.width).assertEqual(800);  // funnel fallback, no clamp
        expect(birth.rect.height).assertEqual(400);
        done();
      });
  });

  describe('TooltipWindowHost.didUpdateViewSize', () => {
    it('feeds the stored content size (physical, verbatim) to the positioner, not the funnel size',
      Level.LEVEL0, async (done: Function) => {
        const ctx = new FakeHostContext();
        const own = new FakeWindow();
        const parent = new FakeWindow();
        const host = mountedHost(ctx, own, parent, 11);
        host.placementRc = 0;

        // No content report yet → early-out (birth stays offscreen).
        await host.updatePosition();
        expect(host.placementCalls).assertEqual(0);

        host.didUpdateViewSize(600, 300);
        await sleep(0);
        expect(host.placementCalls).assertEqual(1);
        expect(host.placementArgs[0]).assertEqual(600);
        expect(host.placementArgs[1]).assertEqual(300);
        done();
      });

    it('dedups unchanged content sizes (win32 DidUpdateViewSize early-out)',
      Level.LEVEL0, async (done: Function) => {
        const ctx = new FakeHostContext();
        const own = new FakeWindow();
        const parent = new FakeWindow();
        const host = mountedHost(ctx, own, parent, 12);
        host.placementRc = 0;

        host.didUpdateViewSize(600, 300);
        host.didUpdateViewSize(600, 300);  // unchanged → no re-placement
        await sleep(0);
        expect(host.placementCalls).assertEqual(1);
        done();
      });

    it('coalesces reports arriving while a placement is in flight (latest wins)',
      Level.LEVEL0, async (done: Function) => {
        const ctx = new FakeHostContext();
        const own = new FakeWindow();
        const parent = new FakeWindow();
        const host = mountedHost(ctx, own, parent, 13);
        host.placementRc = 0;

        // Three reports while the first placement is in flight → two runs,
        // the re-run seeing the LATEST size (physical).
        host.didUpdateViewSize(600, 300);
        host.didUpdateViewSize(700, 350);
        host.didUpdateViewSize(800, 400);
        await sleep(0);
        await sleep(0);
        expect(host.placementCalls).assertEqual(2);
        expect(host.placementArgs[0]).assertEqual(800);
        expect(host.placementArgs[1]).assertEqual(400);
        done();
      });

    it('moves and resizes the sub-window to the positioner rect (physical)',
      Level.LEVEL0, async (done: Function) => {
        const ctx = new FakeHostContext();
        const own = new FakeWindow();
        const parent = new FakeWindow();
        const host = mountedHost(ctx, own, parent, 14);
        host.placementRc = 0;
        host.didUpdateViewSize(600, 300);
        await sleep(0);
        await sleep(0);
        expect(own.moveCalls.length).assertEqual(1);
        expect(own.moveCalls[0][0]).assertEqual(160);
        expect(own.moveCalls[0][1]).assertEqual(240);
        expect(own.resizeCalls.length).assertEqual(1);
        expect(own.resizeCalls[0][0]).assertEqual(250);
        expect(own.resizeCalls[0][1]).assertEqual(100);
        done();
      });
  });
}
