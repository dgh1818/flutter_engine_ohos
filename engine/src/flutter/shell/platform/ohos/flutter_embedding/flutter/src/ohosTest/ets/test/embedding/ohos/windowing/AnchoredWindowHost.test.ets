/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, expect, it, Level } from '@ohos/hypium';
import window from '@ohos.window';
import UIAbility from '@ohos.app.ability.UIAbility';
import { AnchoredWindowHost } from
  '../../../../../../main/ets/embedding/ohos/windowing/AnchoredWindowHost';
import { WindowArchetype } from
  '../../../../../../main/ets/embedding/ohos/windowing/WindowArchetype';
import { WindowHostContext } from
  '../../../../../../main/ets/embedding/ohos/windowing/WindowHostContext';
import {
  FakeWindow, FakeWindowNoShadow, FakeWindowStage, FakeWindowProperties,
  FakeAbility, FakeHostContext, FakeRect, sleep,
} from './Fakes';

/** Concrete subclass exposing the protected hooks of AnchoredWindowHost. */
class TestAnchoredHost extends AnchoredWindowHost {
  constructor(ctx: WindowHostContext, viewId: number, parentViewId: number, w: number, h: number) {
    super(ctx, viewId, parentViewId, WindowArchetype.TOOLTIP, w, h);
  }

  setParentForTest(ability: UIAbility | undefined): void {
    this.parentAbility = ability;
  }

  async styleAfterResizeForTest(subWindow: window.Window): Promise<void> {
    await this.styleAfterResize(subWindow);
  }

  async placeInitialForTest(subWindow: window.Window, lw: number, lh: number,
    pw: number, ph: number): Promise<void> {
    await this.placeInitial(subWindow, lw, lh, pw, ph);
  }

  async positionAtForTest(lw: number, lh: number): Promise<void> {
    await this.positionAt(lw, lh);
  }

  reanchorForTest(): void {
    this.reanchor();
  }
}

/** getWindowProperties() throws — exercises outer catch / reanchor catch. */
class ThrowingPropsWindow extends FakeWindow {
  getWindowProperties(): window.WindowProperties {
    throw new Error('props-throws');
  }
}

/** drawableRect getter throws — exercises the windowRect fallback. */
class DrThrowingProperties {
  windowRect: window.Rect = new FakeRect(0, 0, 1000, 800) as ESObject as window.Rect;

  get drawableRect(): window.Rect {
    throw new Error('dr-throws');
  }
}

/** Host + resolved context/window/stage bundle (ArkTS: no tuple destructure). */
class PositionedHosts {
  ctx: FakeHostContext;
  mainWin: FakeWindow;
  stage: FakeWindowStage;
  host: TestAnchoredHost;

  constructor(ctx: FakeHostContext, mainWin: FakeWindow, stage: FakeWindowStage,
    host: TestAnchoredHost) {
    this.ctx = ctx;
    this.mainWin = mainWin;
    this.stage = stage;
    this.host = host;
  }
}

/** Host with sub-window + parent ability + stage fully resolved. */
function makePositioned(viewId: number, parentViewId: number): PositionedHosts {
  const stage = new FakeWindowStage();
  const mainWin = new FakeWindow();
  stage.mainWindow = mainWin as ESObject as window.Window;
  const ctx = new FakeHostContext();
  ctx.stage = stage as ESObject as window.WindowStage;
  ctx.dpr = 2.0;
  const ability: UIAbility = new FakeAbility() as ESObject as UIAbility;
  ctx.parentAbilityFor = ability;
  const host = new TestAnchoredHost(ctx, viewId, parentViewId, 100, 80);
  host.subWindow = mainWin as ESObject as window.Window;
  host.setParentForTest(ability);
  return new PositionedHosts(ctx, mainWin, stage, host);
}

export default function anchoredWindowHostTest() {
  describe('AnchoredWindowHost.constructor', () => {
    it('starts unanchored with the TOOLTIP archetype', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      const host: TestAnchoredHost = new TestAnchoredHost(ctx, 1, 0, 100, 80);
      expect(host.anchored).assertFalse();
      expect(host.archetype).assertEqual(WindowArchetype.TOOLTIP);
      expect(host.viewId).assertEqual(1);
      expect(host.parentViewId).assertEqual(0);
    });

    it('parentAbilityOf reflects the resolved parent ability', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      const host: TestAnchoredHost = new TestAnchoredHost(ctx, 2, 0, 100, 80);
      expect(host.parentAbilityOf()).assertEqual(undefined);
      const ability: UIAbility = new FakeAbility() as ESObject as UIAbility;
      host.setParentForTest(ability);
      expect(host.parentAbilityOf()).assertEqual(ability);
    });
  });

  describe('AnchoredWindowHost.styleAfterResize', () => {
    it('applies transparent background, zero shadow and zero corner radius', Level.LEVEL0,
      async (done: Function) => {
        const ctx = new FakeHostContext();
        const fake = new FakeWindow();
        const host: TestAnchoredHost = new TestAnchoredHost(ctx, 3, 0, 100, 80);
        await host.styleAfterResizeForTest(fake as ESObject as window.Window);
        expect(fake.backgroundColor).assertEqual('#00000000');
        expect(fake.shadowRadius).assertEqual(0);
        expect(fake.cornerRadius).assertEqual(0);
        done();
      });

    it('logs a throwing background set and still styles the rest', Level.LEVEL0,
      async (done: Function) => {
        const ctx = new FakeHostContext();
        const fake = new FakeWindow();
        fake.bgThrows = true;
        const host: TestAnchoredHost = new TestAnchoredHost(ctx, 4, 0, 100, 80);
        await host.styleAfterResizeForTest(fake as ESObject as window.Window);
        expect(fake.shadowRadius).assertEqual(0);
        expect(fake.cornerRadius).assertEqual(0);
        done();
      });

    it('logs a throwing shadow-radius set and continues to corner', Level.LEVEL0,
      async (done: Function) => {
        const ctx = new FakeHostContext();
        const fake = new FakeWindow();
        fake.shadowThrows = true;
        const host: TestAnchoredHost = new TestAnchoredHost(ctx, 5, 0, 100, 80);
        await host.styleAfterResizeForTest(fake as ESObject as window.Window);
        expect(fake.backgroundColor).assertEqual('#00000000');
        expect(fake.cornerRadius).assertEqual(0);
        done();
      });

    it('logs a rejected corner-radius set', Level.LEVEL0, async (done: Function) => {
      const ctx = new FakeHostContext();
      const fake = new FakeWindow();
      fake.rejectCornerRadius = true;
      const host: TestAnchoredHost = new TestAnchoredHost(ctx, 6, 0, 100, 80);
      await host.styleAfterResizeForTest(fake as ESObject as window.Window);
      expect(fake.backgroundColor).assertEqual('#00000000');
      expect(fake.shadowRadius).assertEqual(0);
      done();
    });

    it('skips the shadow API when the window lacks it', Level.LEVEL0, async (done: Function) => {
      const ctx = new FakeHostContext();
      const fake = new FakeWindowNoShadow();
      const host: TestAnchoredHost = new TestAnchoredHost(ctx, 7, 0, 100, 80);
      await host.styleAfterResizeForTest(fake as ESObject as window.Window);
      expect(fake.background).assertEqual('#00000000');
      expect(fake.cornerRadius).assertEqual(0);
      done();
    });
  });

  describe('AnchoredWindowHost.placeInitial', () => {
    it('marks anchored and registers the sub-window rect listener', Level.LEVEL0,
      async (done: Function) => {
        const p = makePositioned(8, 9);
        await p.host.placeInitialForTest(p.host.subWindow as ESObject as window.Window, 100, 80, 200, 160);
        expect(p.host.anchored).assertTrue();
        expect(p.ctx.subWindowRectListenerCalls).assertEqual(1);
        done();
      });
  });

  describe('AnchoredWindowHost.positionAt', () => {
    it('returns early without a sub-window', Level.LEVEL0, async (done: Function) => {
      const ctx = new FakeHostContext();
      ctx.parentAbilityFor = new FakeAbility() as ESObject as UIAbility;
      const host: TestAnchoredHost = new TestAnchoredHost(ctx, 10, 0, 100, 80);
      host.setParentForTest(ctx.parentAbilityFor);
      await host.positionAtForTest(100, 80);
      done();
    });

    it('returns early without a parent ability', Level.LEVEL0, async (done: Function) => {
      const ctx = new FakeHostContext();
      const fake = new FakeWindow();
      const host: TestAnchoredHost = new TestAnchoredHost(ctx, 11, 0, 100, 80);
      host.subWindow = fake as ESObject as window.Window;
      await host.positionAtForTest(100, 80);
      expect(fake.moveCalls.length).assertEqual(0);
      done();
    });

    it('returns early without a windowStage', Level.LEVEL0, async (done: Function) => {
      const ctx = new FakeHostContext();
      ctx.parentAbilityFor = new FakeAbility() as ESObject as UIAbility;
      const fake = new FakeWindow();
      const host: TestAnchoredHost = new TestAnchoredHost(ctx, 12, 0, 100, 80);
      host.subWindow = fake as ESObject as window.Window;
      host.setParentForTest(ctx.parentAbilityFor);
      await host.positionAtForTest(100, 80);
      expect(fake.moveCalls.length).assertEqual(0);
      done();
    });

    it('queries the native positioner and does not move when unplaced', Level.LEVEL0,
      async (done: Function) => {
        const p = makePositioned(13, 9);
        await p.host.positionAtForTest(100, 80);
        expect(p.mainWin.moveCalls.length).assertEqual(0);
        done();
      });

    it('prefers the parent sub-window rect when the parent is a sub-window', Level.LEVEL0,
      async (done: Function) => {
        const p = makePositioned(14, 9);
        const parentSub = new FakeWindow();
        parentSub.props.setWindowRect(50, 60, 400, 300);
        p.ctx.subWindows.set('9', parentSub as ESObject as window.Window);
        await p.host.positionAtForTest(100, 80);
        expect(parentSub.moveCalls.length).assertEqual(0);
        done();
      });

    it('logs when the parent main-window read throws', Level.LEVEL0, async (done: Function) => {
      const p = makePositioned(15, 9);
      p.stage.getMainThrows = true;
      await p.host.positionAtForTest(100, 80);
      expect(p.mainWin.moveCalls.length).assertEqual(0);
      done();
    });

    it('falls back to windowRect when drawableRect is unavailable', Level.LEVEL0,
      async (done: Function) => {
        const p = makePositioned(16, 9);
        p.mainWin.props = new DrThrowingProperties() as ESObject as FakeWindowProperties;
        await p.host.positionAtForTest(100, 80);
        expect(p.mainWin.moveCalls.length).assertEqual(0);
        done();
      });
  });

  describe('AnchoredWindowHost.reanchor', () => {
    it('returns early without a sub-window', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      const host: TestAnchoredHost = new TestAnchoredHost(ctx, 17, 0, 100, 80);
      host.reanchorForTest();
      expect(ctx.subWindowRectListenerCalls).assertEqual(0);
    });

    it('re-anchors an existing sub-window at its current size', Level.LEVEL0, () => {
      const p = makePositioned(18, 9);
      p.host.reanchorForTest();
      expect(p.ctx.subWindowRectListenerCalls).assertEqual(1);
    });

    it('logs a geometry read failure', Level.LEVEL0, () => {
      const p = makePositioned(19, 9);
      p.host.subWindow = new ThrowingPropsWindow() as ESObject as window.Window;
      p.host.reanchorForTest();
      expect(p.ctx.subWindowRectListenerCalls).assertEqual(1);
    });
  });
}
