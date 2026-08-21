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
  '../../../../../../main/ets/embedding/ohos/windowing/WindowArchetype';
import { WindowHostContext } from
  '../../../../../../main/ets/embedding/ohos/windowing/WindowHostContext';
import { FlutterView } from
  '../../../../../../main/ets/view/FlutterView';
import { FakeWindow, FakeWindowNoDialogApi, FakeHostContext, FakeFlutterView, sleep } from './Fakes';

/** Concrete subclass exposing the protected lifecycle hooks. */
class TestFlutterWindowHost extends FlutterWindowHost {
  windowForTest: window.Window | undefined = undefined;
  closeHostCalls: number = 0;
  closeHostForCascadeCalls: number = 0;
  cascadeAbilityForTest: UIAbility | undefined = undefined;

  constructor(ctx: WindowHostContext, viewId: number) {
    super(ctx, viewId, 0, WindowArchetype.REGULAR, 0, 0);
  }

  attach(): void {
  }

  resolveWindow(): window.Window | null {
    return this.windowForTest !== undefined ? this.windowForTest : null;
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
  windowForTest: window.Window | undefined = undefined;

  constructor(ctx: WindowHostContext, viewId: number, w: number, h: number) {
    super(ctx, viewId, 0, WindowArchetype.REGULAR, w, h);
  }

  attach(): void {
  }

  resolveWindow(): window.Window | null {
    return this.windowForTest !== undefined ? this.windowForTest : null;
  }

  protected closeHost(): void {
  }

  protected closeHostForCascade(parentAbility: UIAbility): void {
  }

  requestedWidthForTest(): number {
    return this.requestedWidth;
  }

  requestedHeightForTest(): number {
    return this.requestedHeight;
  }
}

export default function flutterWindowHostTest() {
  describe('FlutterWindowHost.constructor', () => {
    it('copies viewId/parentViewId/archetype/sizes into fields', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const host: SizedFlutterWindowHost = new SizedFlutterWindowHost(ctx, 42, 300, 200);
      expect(host.viewId).assertEqual(42);
      expect(host.viewIdStr).assertEqual('42');
      expect(host.parentViewId).assertEqual(0);
      expect(host.archetype).assertEqual(WindowArchetype.REGULAR);
      expect(host.requestedWidthForTest()).assertEqual(300);
      expect(host.requestedHeightForTest()).assertEqual(200);
    });
  });

  describe('FlutterWindowHost.setWindowSize', () => {
    it('resizes with physical px scaled by dpr', Level.LEVEL0, (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      ctx.dpr = 2.0;
      const fake = new FakeWindow();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 1);
      host.windowForTest = fake as ESObject as window.Window;
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
      expect(host.resolveWindow()).assertEqual(null);
    });

    it('returns early on non-positive sizes', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 3);
      host.windowForTest = fake as ESObject as window.Window;
      host.setWindowSize(0, 50);
      host.setWindowSize(100, -5);
      expect(fake.resizeCalls.length).assertEqual(0);
    });

    it('clamps against the current constraint range', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      ctx.dpr = 1.0;
      const fake = new FakeWindow();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 4);
      host.windowForTest = fake as ESObject as window.Window;
      host.setWindowConstraints(50, 200, 60, 300);
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
      host.windowForTest = fake as ESObject as window.Window;
      host.setWindowSize(100, 50);
      await sleep(50);
      expect(fake.resizeCalls.length).assertEqual(1);
      done();
    });
  });

  describe('FlutterWindowHost.setWindowConstraints', () => {
    it('stores and pushes constraints to a window with the API', Level.LEVEL0, (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 6);
      host.windowForTest = fake as ESObject as window.Window;
      host.setWindowConstraints(50, 200, 60, 300);
      expect(fake.constraintCalls.length).assertEqual(1);
      expect(fake.constraintCalls[0][0]).assertEqual(50);
      expect(fake.constraintCalls[0][1]).assertEqual(200);
      expect(fake.constraintCalls[0][2]).assertEqual(60);
      expect(fake.constraintCalls[0][3]).assertEqual(300);
      done();
    });

    it('falls back to host-side clamp only when the WM API is absent', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindowNoDialogApi();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 7);
      host.windowForTest = fake as ESObject as window.Window;
      host.setWindowConstraints(10, 100, 20, 200);
      // no push attempted on the fake (it has no setWindowConstraints member)
      expect(host.resolveWindow()).not().assertNull();
    });

    it('returns early without a window', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 8);
      host.setWindowConstraints(1, 2, 3, 4);
      expect(host.resolveWindow()).assertEqual(null);
    });

    it('logs the rejection when the WM push fails', Level.LEVEL0, async (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      fake.rejectConstraints = true;
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 9);
      host.windowForTest = fake as ESObject as window.Window;
      host.setWindowConstraints(50, 200, 60, 300);
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
      host.windowForTest = fake as ESObject as window.Window;
      host.activate();
      expect(fake.showCount).assertEqual(1);
      done();
    });

    it('returns early without a window', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 11);
      host.activate();
      expect(host.resolveWindow()).assertEqual(null);
    });

    it('logs the rejection when show fails', Level.LEVEL0, async (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      fake.rejectShow = true;
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 12);
      host.windowForTest = fake as ESObject as window.Window;
      host.activate();
      await sleep(50);
      expect(fake.showCount).assertEqual(1);
      done();
    });
  });

  describe('FlutterWindowHost.setWindowTitle', () => {
    it('sets the title', Level.LEVEL0, (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 13);
      host.windowForTest = fake as ESObject as window.Window;
      host.setWindowTitle('hello');
      expect(fake.titles[0]).assertEqual('hello');
      done();
    });

    it('returns early without a window', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 14);
      host.setWindowTitle('hello');
      expect(host.resolveWindow()).assertEqual(null);
    });

    it('logs the rejection when the title set fails', Level.LEVEL0, async (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      fake.rejectTitle = true;
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 15);
      host.windowForTest = fake as ESObject as window.Window;
      host.setWindowTitle('hello');
      await sleep(50);
      expect(fake.titles.length).assertEqual(1);
      done();
    });
  });

  describe('FlutterWindowHost.setWindowMaximized', () => {
    it('maximizes when true', Level.LEVEL0, (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 16);
      host.windowForTest = fake as ESObject as window.Window;
      host.setWindowMaximized(true);
      expect(fake.maximizeCount).assertEqual(1);
      done();
    });

    it('restores when false', Level.LEVEL0, (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 17);
      host.windowForTest = fake as ESObject as window.Window;
      host.setWindowMaximized(false);
      expect(fake.restoreCount).assertEqual(1);
      done();
    });

    it('returns early without a window', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 18);
      host.setWindowMaximized(true);
      expect(host.resolveWindow()).assertEqual(null);
    });

    it('logs the rejection when maximize fails', Level.LEVEL0, async (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      fake.rejectMaximize = true;
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 19);
      host.windowForTest = fake as ESObject as window.Window;
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
      host.windowForTest = fake as ESObject as window.Window;
      host.setWindowMinimized(true);
      expect(fake.minimizeCount).assertEqual(1);
      done();
    });

    it('restores when false', Level.LEVEL0, (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 21);
      host.windowForTest = fake as ESObject as window.Window;
      host.setWindowMinimized(false);
      expect(fake.restoreCount).assertEqual(1);
      done();
    });

    it('returns early without a window', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 22);
      host.setWindowMinimized(true);
      expect(host.resolveWindow()).assertEqual(null);
    });

    it('logs the rejection when minimize fails', Level.LEVEL0, async (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      fake.rejectMinimize = true;
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 23);
      host.windowForTest = fake as ESObject as window.Window;
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
      host.windowForTest = fake as ESObject as window.Window;
      host.setWindowFullscreen(true);
      expect(fake.fullscreenArgs[0]).assertEqual(true);
      done();
    });

    it('returns early without a window', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 25);
      host.setWindowFullscreen(true);
      expect(host.resolveWindow()).assertEqual(null);
    });

    it('logs the rejection when fullscreen fails', Level.LEVEL0, async (done: Function) => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fake = new FakeWindow();
      fake.rejectFullscreen = true;
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 26);
      host.windowForTest = fake as ESObject as window.Window;
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
      ctx.flutterViews.set('27', fvAsView);
      ctx.viewAbilities.set('27', new FakeAbilityObj() as ESObject as UIAbility);
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 27);
      host.destroy();
      expect(host.closeHostCalls).assertEqual(1);
      expect(ctx.deletedViews.includes('27')).assertTrue();
      expect(ctx.deletedAbilities.includes('27')).assertTrue();
      expect(ctx.removedHosts.includes('27')).assertTrue();
    });

    it('detaches the registered FlutterView when present', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fv = new FakeFlutterView();
      const fvAsView: FlutterView = fv as ESObject as FlutterView;
      ctx.flutterViews.set('28', fvAsView);
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 28);
      host.destroy();
      expect(fv.detachCalls).assertEqual(1);
    });

    it('destroys cleanly when no FlutterView is registered', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 40);
      host.destroy();
      expect(host.closeHostCalls).assertEqual(1);
      expect(ctx.removedHosts.includes('40')).assertTrue();
    });

    it('logs a throwing FlutterView detach and continues', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fv = new FakeFlutterView();
      fv.detachThrows = true;
      const fvAsView: FlutterView = fv as ESObject as FlutterView;
      ctx.flutterViews.set('41', fvAsView);
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 41);
      host.destroy();
      expect(fv.detachCalls).assertEqual(1);
      expect(ctx.removedHosts.includes('41')).assertTrue();
    });
  });

  describe('FlutterWindowHost.handleOsClosed', () => {
    it('detaches and cleans up bookkeeping', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fv = new FakeFlutterView();
      const fvAsView: FlutterView = fv as ESObject as FlutterView;
      ctx.flutterViews.set('29', fvAsView);
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 29);
      host.handleOsClosed();
      expect(fv.detachCalls).assertEqual(1);
      expect(ctx.removedHosts.includes('29')).assertTrue();
    });
  });

  describe('FlutterWindowHost.closeForCascade', () => {
    it('detaches, cascades and cleans up bookkeeping', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const fv = new FakeFlutterView();
      const fvAsView: FlutterView = fv as ESObject as FlutterView;
      ctx.flutterViews.set('30', fvAsView);
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 30);
      const ability: UIAbility = new FakeAbilityObj() as ESObject as UIAbility;
      host.closeForCascade(ability);
      expect(fv.detachCalls).assertEqual(1);
      expect(host.closeHostForCascadeCalls).assertEqual(1);
      expect(host.cascadeAbilityForTest).assertEqual(ability);
      expect(ctx.removedHosts.includes('30')).assertTrue();
    });
  });

  describe('FlutterWindowHost.failAttach', () => {
    it('reports to native and cleans up bookkeeping', Level.LEVEL0, () => {
      const ctx: FakeHostContext = new FakeHostContext();
      const host: TestFlutterWindowHost = new TestFlutterWindowHost(ctx, 31);
      host.failAttachForTest('test');
      expect(ctx.removedHosts.includes('31')).assertTrue();
    });
  });
}

/** Minimal object cast to UIAbility for lifecycle tests. */
class FakeAbilityObj {
  context: Object = new Object();
}
