/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, expect, it, Level } from '@ohos/hypium';
import window from '@ohos.window';
import UIAbility from '@ohos.app.ability.UIAbility';
import { FlutterWindowHost } from
  '../../../../../../main/ets/embedding/ohos/windowing/FlutterWindowHost';
import { WindowArchetype } from
  '../../../../../../main/ets/embedding/ohos/windowing/WindowingUtils';
import { WindowHostContext } from
  '../../../../../../main/ets/embedding/ohos/windowing/WindowHostContext';
import { FlutterView } from
  '../../../../../../main/ets/view/FlutterView';
import { FakeWindow, FakeHostContext, FakeFlutterView, regularRequest, sleep } from './Fakes';
import { NO_PARENT_VIEW_ID, WindowConstraints } from '../../../../../../main/ets/embedding/ohos/windowing/WindowingUtils';

/** Concrete subclass exposing the protected lifecycle hooks. */
class TestFlutterWindowHost extends FlutterWindowHost {
  closeHostCalls: number = 0;
  closeHostForCascadeCalls: number = 0;
  cascadeAbilityForTest: UIAbility | undefined = undefined;

  /** Fixed density for the densityPixels seam — deterministic on device. */
  testDpr: number = 1.0;

  constructor(ctx: WindowHostContext, viewId: number, dpr: number = 1.0) {
    super(ctx, viewId, NO_PARENT_VIEW_ID, '', WindowArchetype.REGULAR);
    this.testDpr = dpr;
  }

  async attach(): Promise<void> {
  }

  protected densityPixels(_win: window.Window | undefined): number {
    return this.testDpr;
  }

  protected closeHost(): void {
    this.closeHostCalls += 1;
  }

  protected closeHostForCascade(parentAbility: UIAbility): void {
    this.closeHostForCascadeCalls += 1;
    this.cascadeAbilityForTest = parentAbility;
  }

  failAttachForTest(where: string): void {
    this.failAttach(where);
  }
}

/** Host carrying both sizes for setWindowSize clamp assertions. */
class SizedFlutterWindowHost extends FlutterWindowHost {
  private readonly reqWidth: number;
  private readonly reqHeight: number;

  constructor(ctx: WindowHostContext, viewId: number, w: number, h: number) {
    super(ctx, viewId, NO_PARENT_VIEW_ID, '', WindowArchetype.REGULAR);
    this.reqWidth = w;
    this.reqHeight = h;
  }

  async attach(): Promise<void> {
  }

  protected closeHost(): void {
  }

  protected closeHostForCascade(parentAbility: UIAbility): void {
  }

  requestedWidthForTest(): number {
    return this.reqWidth;
  }

  requestedHeightForTest(): number {
    return this.reqHeight;
  }
}

export default function flutterWindowHostTest() {
  describe('FlutterWindowHost.constructor', () => {
    it('copies viewId/parentViewId/archetype/sizes into fields', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const host: SizedFlutterWindowHost = new SizedFlutterWindowHost(ctx, 42, 300, 200);
      expect(host.viewId).assertEqual(42);
      expect(String(host.viewId)).assertEqual('42');
      expect(host.parentViewId).assertEqual(NO_PARENT_VIEW_ID);
      expect(host.archetype).assertEqual(WindowArchetype.REGULAR);
      expect(host.requestedWidthForTest()).assertEqual(300);
      expect(host.requestedHeightForTest()).assertEqual(200);
    });
  });

  describe('FlutterWindowHost.setWindowSize', () => {
    it('resizes with physical px scaled by dpr', Level.LEVEL0, (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 1, 2.0);
      host.setWindow(fake as ESObject as window.Window);
      host.setWindowSize(100, 50);
      expect(fake.resizeCalls.length).assertEqual(1);
      expect(fake.resizeCalls[0][0]).assertEqual(200);
      expect(fake.resizeCalls[0][1]).assertEqual(100);
      done();
    });

    it('returns early without a window', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 2);
      host.setWindowSize(100, 50);
      expect(host.getWindow()).assertEqual(null);
    });

    it('returns early on non-positive sizes', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 3);
      host.setWindow(fake as ESObject as window.Window);
      host.setWindowSize(0, 50);
      host.setWindowSize(100, -5);
      expect(fake.resizeCalls.length).assertEqual(0);
    });

    it('clamps against the current constraint range', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 4); // dpr 1.0
      host.setWindow(fake as ESObject as window.Window);
      const limits: window.WindowLimits = {
        maxWidth: 200,
        maxHeight: 300,
        minWidth: 50,
        minHeight: 60,
      };
      host.setWindowLimits(limits);
      host.setWindowSize(100, 100);
      expect(fake.resizeCalls[0][0]).assertEqual(100);
      expect(fake.resizeCalls[0][1]).assertEqual(100);
      host.setWindowSize(10, 10);
      expect(fake.resizeCalls[1][0]).assertEqual(50);
      expect(fake.resizeCalls[1][1]).assertEqual(60);
      host.setWindowSize(500, 500);
      expect(fake.resizeCalls[2][0]).assertEqual(200);
      expect(fake.resizeCalls[2][1]).assertEqual(300);
    });

    it('logs the rejection when resize fails', Level.LEVEL0, async (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      fake.rejectResize = true;
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 5);
      host.setWindow(fake as ESObject as window.Window);
      host.setWindowSize(100, 50);
      await sleep(50);
      expect(fake.resizeCalls.length).assertEqual(1);
      done();
    });

    it('adds the decoration inset so the CONTENT area matches (2in1 title bar)',
      Level.LEVEL0, () => {
        const ctx: FakeHostContext = new FakeHostContext();
        const fake = new FakeWindow();
        fake.decorHeightVp = 37; // 37 vp × dpr 2.0 = 74 px title-bar inset
        const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 7, 2.0);
        host.setWindow(fake as ESObject as window.Window);
        host.setWindowSize(100, 50);
        // phys 200x100 + decor 0x74 → outer 200x174 → drawable content 200x100.
        expect(fake.resizeCalls.length).assertEqual(1);
        expect(fake.resizeCalls[0][0]).assertEqual(200);
        expect(fake.resizeCalls[0][1]).assertEqual(174);
      });

    it('degrades to a plain resize when getWindowProperties throws', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      fake.propsThrows = true;
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 8, 2.0);
      host.setWindow(fake as ESObject as window.Window);
      host.setWindowSize(100, 50);
      expect(fake.resizeCalls.length).assertEqual(1);
      expect(fake.resizeCalls[0][0]).assertEqual(200);
      expect(fake.resizeCalls[0][1]).assertEqual(100);
    });

  });

  describe('FlutterWindowHost.setWindowLimits', () => {
    it('stores and pushes the limits to the window', Level.LEVEL0, (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 6); // dpr 1.0: pushed PHYSICAL px == logical values
      host.setWindow(fake as ESObject as window.Window);
      const limits: window.WindowLimits = {
        maxWidth: 200,
        maxHeight: 300,
        minWidth: 50,
        minHeight: 60,
      };
      host.setWindowLimits(limits);
      expect(fake.constraintCalls.length).assertEqual(1);
      expect(fake.constraintCalls[0][0]).assertEqual(50);
      expect(fake.constraintCalls[0][1]).assertEqual(200);
      expect(fake.constraintCalls[0][2]).assertEqual(60);
      expect(fake.constraintCalls[0][3]).assertEqual(300);
      done();
    });

    it('still clamps later resizes when the WM push fails', Level.LEVEL0, async (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      fake.rejectConstraints = true;
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 7); // dpr 1.0
      host.setWindow(fake as ESObject as window.Window);
      const limits: window.WindowLimits = {
        maxWidth: 100,
        maxHeight: 200,
        minWidth: 10,
        minHeight: 20,
      };
      host.setWindowLimits(limits);
      await sleep(50);
      expect(fake.constraintCalls.length).assertEqual(1);
      // push rejected, but the stored range still clamps later resizes
      host.setWindowSize(500, 500);
      expect(fake.resizeCalls[0][0]).assertEqual(100);
      expect(fake.resizeCalls[0][1]).assertEqual(200);
      done();
    });

    it('returns early without a window', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 8);
      const limits: window.WindowLimits = {
        maxWidth: 4,
        maxHeight: 3,
        minWidth: 1,
        minHeight: 2,
      };
      host.setWindowLimits(limits);
      expect(host.getWindow()).assertEqual(null);
    });

    it('logs the rejection when the WM push fails', Level.LEVEL0, async (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      fake.rejectConstraints = true;
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 9);
      host.setWindow(fake as ESObject as window.Window);
      const limits: window.WindowLimits = {
        maxWidth: 200,
        maxHeight: 300,
        minWidth: 50,
        minHeight: 60,
      };
      host.setWindowLimits(limits);
      await sleep(50);
      expect(fake.constraintCalls.length).assertEqual(1);
      done();
    });
  });

  describe('FlutterWindowHost.activate', () => {
    it('re-shows the window', Level.LEVEL0, (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 10);
      host.setWindow(fake as ESObject as window.Window);
      host.activate();
      expect(fake.showCount).assertEqual(1);
      done();
    });

    it('returns early without a window', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 11);
      host.activate();
      expect(host.getWindow()).assertEqual(null);
    });

    it('logs the rejection when show fails', Level.LEVEL0, async (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      fake.rejectShow = true;
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 12);
      host.setWindow(fake as ESObject as window.Window);
      host.activate();
      await sleep(50);
      expect(fake.showCount).assertEqual(1);
      done();
    });
  });

  describe('FlutterWindowHost.setWindowTitle', () => {
    // TEMPORARILY REVERTED: runtime Dart SetTitle pushes still arrive but are
    // not applied to the OS window — sub-window/parent title application is
    // disabled and no title is recorded.

    it('applies no title to the OS window', Level.LEVEL0, (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 13);
      host.setWindow(fake as ESObject as window.Window);
      host.setWindowTitle('hello');
      host.setWindowTitle('');
      host.setWindowTitle();
      expect(fake.titles.length).assertEqual(0);
      done();
    });

    it('calls through safely without a window', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 14);
      host.setWindowTitle('hello');
      host.setWindowTitle();
      expect(host.getWindow()).assertEqual(null);
    });

    it('never reaches the window even when a title set would fail', Level.LEVEL0, async (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      fake.rejectTitle = true;
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 15);
      host.setWindow(fake as ESObject as window.Window);
      host.setWindowTitle('hello');
      await sleep(50);
      expect(fake.titles.length).assertEqual(0);
      done();
    });
  });

  describe('FlutterWindowHost.setWindowMaximized', () => {
    it('maximizes when true', Level.LEVEL0, (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 16);
      host.setWindow(fake as ESObject as window.Window);
      host.setWindowMaximized(true);
      expect(fake.maximizeCount).assertEqual(1);
      done();
    });

    it('restores when false', Level.LEVEL0, (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 17);
      host.setWindow(fake as ESObject as window.Window);
      host.setWindowMaximized(false);
      expect(fake.restoreCount).assertEqual(1);
      done();
    });

    it('returns early without a window', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 18);
      host.setWindowMaximized(true);
      expect(host.getWindow()).assertEqual(null);
    });

    it('logs the rejection when maximize fails', Level.LEVEL0, async (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      fake.rejectMaximize = true;
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 19);
      host.setWindow(fake as ESObject as window.Window);
      host.setWindowMaximized(true);
      await sleep(50);
      expect(fake.maximizeCount).assertEqual(1);
      done();
    });
  });

  describe('FlutterWindowHost.setWindowMinimized', () => {
    it('minimizes when true', Level.LEVEL0, (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 20);
      host.setWindow(fake as ESObject as window.Window);
      host.setWindowMinimized(true);
      expect(fake.minimizeCount).assertEqual(1);
      done();
    });

    it('restores when false', Level.LEVEL0, (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 21);
      host.setWindow(fake as ESObject as window.Window);
      host.setWindowMinimized(false);
      expect(fake.restoreCount).assertEqual(1);
      done();
    });

    it('returns early without a window', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 22);
      host.setWindowMinimized(true);
      expect(host.getWindow()).assertEqual(null);
    });

    it('logs the rejection when minimize fails', Level.LEVEL0, async (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      fake.rejectMinimize = true;
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 23);
      host.setWindow(fake as ESObject as window.Window);
      host.setWindowMinimized(true);
      await sleep(50);
      expect(fake.minimizeCount).assertEqual(1);
      done();
    });
  });

  describe('FlutterWindowHost.setWindowFullscreen', () => {
    it('sets fullscreen on the window', Level.LEVEL0, (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 24);
      host.setWindow(fake as ESObject as window.Window);
      host.setWindowFullscreen(true);
      expect(fake.fullscreenArgs[0]).assertEqual(true);
      done();
    });

    it('returns early without a window', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 25);
      host.setWindowFullscreen(true);
      expect(host.getWindow()).assertEqual(null);
    });

    it('logs the rejection when fullscreen fails', Level.LEVEL0, async (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      fake.rejectFullscreen = true;
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 26);
      host.setWindow(fake as ESObject as window.Window);
      host.setWindowFullscreen(true);
      await sleep(50);
      expect(fake.fullscreenArgs.length).assertEqual(1);
      done();
    });
  });

  describe('FlutterWindowHost.destroy', () => {
    it('detaches, closes and cleans up bookkeeping', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fv = new FakeFlutterView();
      const fvAsView: FlutterView = fv as ESObject as FlutterView;
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 27);
      host.setFlutterView(fvAsView);
      host.destroy();
      expect(host.closeHostCalls).assertEqual(1);
      expect(ctx.deletedViews.includes('27')).assertTrue();
      expect(ctx.removedHosts.includes(27)).assertTrue();
    });

    it('detaches the registered FlutterView when present', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fv = new FakeFlutterView();
      const fvAsView: FlutterView = fv as ESObject as FlutterView;
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 28);
      host.setFlutterView(fvAsView);
      host.destroy();
      expect(fv.detachCalls).assertEqual(1);
    });

    it('destroys cleanly when no FlutterView is registered', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 40);
      host.destroy();
      expect(host.closeHostCalls).assertEqual(1);
      expect(ctx.removedHosts.includes(40)).assertTrue();
    });

    it('logs a throwing FlutterView detach and continues', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fv = new FakeFlutterView();
      fv.detachThrows = true;
      const fvAsView: FlutterView = fv as ESObject as FlutterView;
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 41);
      host.setFlutterView(fvAsView);
      host.destroy();
      expect(fv.detachCalls).assertEqual(1);
      expect(ctx.removedHosts.includes(41)).assertTrue();
    });
  });

  describe('FlutterWindowHost.handleOsClosed', () => {
    it('detaches and cleans up bookkeeping', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fv = new FakeFlutterView();
      const fvAsView: FlutterView = fv as ESObject as FlutterView;
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 29);
      host.setFlutterView(fvAsView);
      host.handleOsClosed();
      expect(fv.detachCalls).assertEqual(1);
      expect(ctx.removedHosts.includes(29)).assertTrue();
    });
  });

  describe('FlutterWindowHost.closeForCascade', () => {
    it('detaches, cascades and cleans up bookkeeping', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fv = new FakeFlutterView();
      const fvAsView: FlutterView = fv as ESObject as FlutterView;
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 30);
      host.setFlutterView(fvAsView);
      const ability: UIAbility = new FakeAbilityObj() as ESObject as UIAbility;
      host.closeForCascade(ability);
      expect(fv.detachCalls).assertEqual(1);
      expect(host.closeHostForCascadeCalls).assertEqual(1);
      expect(host.cascadeAbilityForTest).assertEqual(ability);
      expect(ctx.removedHosts.includes(30)).assertTrue();
    });
  });

  describe('FlutterWindowHost.failAttach', () => {
    it('reports to native and cleans up bookkeeping', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 31);
      host.failAttachForTest('test');
      expect(ctx.removedHosts.includes(31)).assertTrue();
    });
  });
}

/** Minimal object cast to UIAbility for lifecycle tests. */
class FakeAbilityObj {
  context: Object = new Object();
}
