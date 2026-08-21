/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, expect, it, Level } from '@ohos/hypium';
import { BusinessError } from '@ohos.base';
import window from '@ohos.window';
import UIAbility from '@ohos.app.ability.UIAbility';
import { SubWindowHost } from
  '../../../../../../main/ets/embedding/ohos/windowing/SubWindowHost';
import { WindowArchetype } from
  '../../../../../../main/ets/embedding/ohos/windowing/WindowArchetype';
import { WindowHostContext } from
  '../../../../../../main/ets/embedding/ohos/windowing/WindowHostContext';
import FlutterEngine from
  '../../../../../../main/ets/embedding/engine/FlutterEngine';
import { FlutterView } from
  '../../../../../../main/ets/view/FlutterView';
import {
  FakeWindow, FakeWindowStage, FakeAbility, FakeFlutterView, FakeHostContext, sleep,
} from './Fakes';

/** Subclass exposing protected hooks and signalling attach-chain completion. */
class TestSubWindowHost extends SubWindowHost {
  attachChainDone: boolean = false;

  constructor(ctx: WindowHostContext, viewId: number, parentViewId: number, w: number, h: number) {
    super(ctx, viewId, parentViewId, WindowArchetype.POPUP, w, h);
  }

  protected async configureAfterShow(subWindow: window.Window): Promise<void> {
    this.attachChainDone = true;
  }

  closeHostForTest(): void {
    this.closeHost();
  }

  closeHostForCascadeForTest(ability: UIAbility): void {
    this.closeHostForCascade(ability);
  }

  defaultLogicalSizeForTest(): number[] {
    return this.defaultLogicalSize();
  }
}

/** Subclass running the BASE configureAfterShow hook (no override). */
class BaseHookSubWindowHost extends SubWindowHost {
  constructor(ctx: WindowHostContext, viewId: number, parentViewId: number, w: number, h: number) {
    super(ctx, viewId, parentViewId, WindowArchetype.POPUP, w, h);
  }
}

/** BusinessError fake carrying just `code` (what the hosts check). */
class FakeErr {
  code: number = 0;
  message: string = '';
}

function makeErr(code: number): BusinessError {
  const e = new FakeErr();
  e.code = code;
  e.message = 'boom';
  return e as ESObject as BusinessError;
}

function makeCtx(stage: FakeWindowStage | undefined): FakeHostContext {
  const ctx = new FakeHostContext();
  ctx.stage = stage as ESObject as window.WindowStage;
  return ctx;
}

export default function subWindowHostTest() {
  describe('SubWindowHost.attach earlyFailures', () => {
    it('fails when no parent ability can be resolved', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      const host: TestSubWindowHost = new TestSubWindowHost(ctx, 1, 0, 100, 80);
      host.attach();
      expect(ctx.removedHosts.includes('1')).assertTrue();
    });

    it('fails when the parent ability has no windowStage', Level.LEVEL0, () => {
      const ctx = makeCtx(undefined);
      const ability: UIAbility = new FakeAbility() as ESObject as UIAbility;
      ctx.parentAbilityFor = ability;
      const host: TestSubWindowHost = new TestSubWindowHost(ctx, 2, 0, 100, 80);
      host.attach();
      expect(ctx.removedHosts.includes('2')).assertTrue();
    });

    it('fails when createSubWindow reports an error', Level.LEVEL0, () => {
      const stage = new FakeWindowStage();
      stage.subWindowErr = makeErr(401);
      const ctx = makeCtx(stage);
      ctx.parentAbilityFor = new FakeAbility() as ESObject as UIAbility;
      const host: TestSubWindowHost = new TestSubWindowHost(ctx, 3, 0, 100, 80);
      host.attach();
      expect(stage.createSubWindowCount).assertEqual(1);
      expect(ctx.removedHosts.includes('3')).assertTrue();
    });

    it('fails when the shared engine is unavailable', Level.LEVEL0, () => {
      const stage = new FakeWindowStage();
      stage.subWindow = new FakeWindow() as ESObject as window.Window;
      const ctx = makeCtx(stage);
      ctx.parentAbilityFor = new FakeAbility() as ESObject as UIAbility;
      ctx.engine = null;
      const fv = new FakeFlutterView();
      ctx.flutterViews.set('4', fv as ESObject as FlutterView);
      const host: TestSubWindowHost = new TestSubWindowHost(ctx, 4, 0, 100, 80);
      host.attach();
      expect(ctx.removedHosts.includes('4')).assertTrue();
    });
  });

  describe('SubWindowHost.attach success', () => {
    it('creates a new FlutterView then fails without a shared engine', Level.LEVEL0,
      async (done: Function) => {
        const fake = new FakeWindow();
        const stage = new FakeWindowStage();
        stage.subWindow = fake as ESObject as window.Window;
        const ctx = makeCtx(stage);
        ctx.parentAbilityFor = new FakeAbility() as ESObject as UIAbility;
        ctx.engine = null;
        const host: TestSubWindowHost = new TestSubWindowHost(ctx, 5, 9, 100, 80);
        host.attach();
        await sleep(150);
        // Engine missing → attach aborts BEFORE loadContent; the fresh
        // FlutterView is created and registered, then failAttach's
        // cleanupBookkeeping drops it together with the host.
        expect(fake.loadContentCount).assertEqual(0);
        expect(ctx.flutterViews.has('5')).assertFalse();
        expect(ctx.removedHosts.includes('5')).assertTrue();
        done();
      });

    it('attaches to the shared engine and completes the launch chain', Level.LEVEL0,
      async (done: Function) => {
        const fake = new FakeWindow();
        const stage = new FakeWindowStage();
        stage.subWindow = fake as ESObject as window.Window;
        const ctx = makeCtx(stage);
        ctx.parentAbilityFor = new FakeAbility() as ESObject as UIAbility;
        ctx.engine = new Object() as ESObject as FlutterEngine;
        const fv = new FakeFlutterView();
        ctx.flutterViews.set('5', fv as ESObject as FlutterView);
        const host: TestSubWindowHost = new TestSubWindowHost(ctx, 5, 9, 100, 80);
        host.attach();
        await sleep(150);
        expect(fake.loadContentCount).assertEqual(1);
        expect(fake.resizeCalls.length).assertEqual(1);
        expect(fake.resizeCalls[0][0]).assertEqual(200);
        expect(fake.resizeCalls[0][1]).assertEqual(160);
        expect(fake.showCount).assertEqual(1);
        expect(host.attachChainDone).assertTrue();
        expect(ctx.viewAbilities.has('5')).assertTrue();
        expect(ctx.parentRectListenerCalls).assertEqual(1);
        expect(stage.createSubWindowName).assertEqual('flutter-5');
        done();
      });

    it('reuses an attached FlutterView and skips engine attach', Level.LEVEL0,
      async (done: Function) => {
        const fake = new FakeWindow();
        const stage = new FakeWindowStage();
        stage.subWindow = fake as ESObject as window.Window;
        const ctx = makeCtx(stage);
        ctx.parentAbilityFor = new FakeAbility() as ESObject as UIAbility;
        ctx.engine = new Object() as ESObject as FlutterEngine;
        const fv = new FakeFlutterView();
        fv.attached = true;
        ctx.flutterViews.set('6', fv as ESObject as FlutterView);
        const host: TestSubWindowHost = new TestSubWindowHost(ctx, 6, 9, 100, 80);
        host.attach();
        await sleep(150);
        expect(fv.attachCalls).assertEqual(0);
        expect(host.attachChainDone).assertTrue();
        done();
      });

    it('attaches an un-attached FlutterView to the shared engine', Level.LEVEL0,
      async (done: Function) => {
        const fake = new FakeWindow();
        const stage = new FakeWindowStage();
        stage.subWindow = fake as ESObject as window.Window;
        const ctx = makeCtx(stage);
        ctx.parentAbilityFor = new FakeAbility() as ESObject as UIAbility;
        ctx.engine = new Object() as ESObject as FlutterEngine;
        const fv = new FakeFlutterView();
        ctx.flutterViews.set('7', fv as ESObject as FlutterView);
        const host: TestSubWindowHost = new TestSubWindowHost(ctx, 7, 9, 100, 80);
        host.attach();
        await sleep(150);
        expect(fv.attachCalls).assertEqual(1);
        expect(fv.attached).assertTrue();
        expect(host.attachChainDone).assertTrue();
        done();
      });

    it('uses the default logical size when the requested size is zero', Level.LEVEL0,
      async (done: Function) => {
        const fake = new FakeWindow();
        const stage = new FakeWindowStage();
        stage.subWindow = fake as ESObject as window.Window;
        const ctx = makeCtx(stage);
        ctx.parentAbilityFor = new FakeAbility() as ESObject as UIAbility;
        ctx.engine = new Object() as ESObject as FlutterEngine;
        ctx.dpr = 1.0;
        const fv = new FakeFlutterView();
        ctx.flutterViews.set('8', fv as ESObject as FlutterView);
        const host: TestSubWindowHost = new TestSubWindowHost(ctx, 8, 9, 0, 0);
        host.attach();
        await sleep(150);
        expect(fake.resizeCalls[0][0]).assertEqual(400);
        expect(fake.resizeCalls[0][1]).assertEqual(320);
        done();
      });

    it('logs a failed windowEvent registration and continues', Level.LEVEL0,
      async (done: Function) => {
        const fake = new FakeWindow();
        fake.onThrows = true;
        const stage = new FakeWindowStage();
        stage.subWindow = fake as ESObject as window.Window;
        const ctx = makeCtx(stage);
        ctx.parentAbilityFor = new FakeAbility() as ESObject as UIAbility;
        ctx.engine = new Object() as ESObject as FlutterEngine;
        const fv = new FakeFlutterView();
        ctx.flutterViews.set('9', fv as ESObject as FlutterView);
        const host: TestSubWindowHost = new TestSubWindowHost(ctx, 9, 9, 100, 80);
        host.attach();
        await sleep(150);
        expect(fake.showCount).assertEqual(1);
        expect(host.attachChainDone).assertTrue();
        done();
      });

    it('fires the Dart window-destroyed flow on OS WINDOW_DESTROYED', Level.LEVEL0,
      async (done: Function) => {
        const fake = new FakeWindow();
        const stage = new FakeWindowStage();
        stage.subWindow = fake as ESObject as window.Window;
        const ctx = makeCtx(stage);
        ctx.parentAbilityFor = new FakeAbility() as ESObject as UIAbility;
        ctx.engine = new Object() as ESObject as FlutterEngine;
        const fv = new FakeFlutterView();
        ctx.flutterViews.set('14', fv as ESObject as FlutterView);
        const host: TestSubWindowHost = new TestSubWindowHost(ctx, 14, 9, 100, 80);
        host.attach();
        await sleep(150);
        // The WINDOW_DESTROYED listener is registered after showWindow.
        expect(fake.windowEventCb).not().assertUndefined();
        fake.windowEventCb?.(window.WindowEventType.WINDOW_DESTROYED);
        expect(fv.detachCalls).assertEqual(1);
        expect(ctx.removedHosts.includes('14')).assertTrue();
        done();
      });

    it('runs the base configureAfterShow hook when not overridden', Level.LEVEL0,
      async (done: Function) => {
        const fake = new FakeWindow();
        const stage = new FakeWindowStage();
        stage.subWindow = fake as ESObject as window.Window;
        const ctx = makeCtx(stage);
        ctx.parentAbilityFor = new FakeAbility() as ESObject as UIAbility;
        ctx.engine = new Object() as ESObject as FlutterEngine;
        const fv = new FakeFlutterView();
        ctx.flutterViews.set('13', fv as ESObject as FlutterView);
        const host: BaseHookSubWindowHost = new BaseHookSubWindowHost(ctx, 13, 9, 100, 80);
        host.attach();
        await sleep(150);
        expect(fake.showCount).assertEqual(1);
        expect(ctx.viewAbilities.has('13')).assertTrue();
        done();
      });
  });

  describe('SubWindowHost.attach loadAndResizeFailures', () => {
    it('fails when loadContent reports an error', Level.LEVEL0, async (done: Function) => {
      const fake = new FakeWindow();
      fake.loadContentErr = makeErr(401);
      const stage = new FakeWindowStage();
      stage.subWindow = fake as ESObject as window.Window;
      const ctx = makeCtx(stage);
      ctx.parentAbilityFor = new FakeAbility() as ESObject as UIAbility;
      ctx.engine = new Object() as ESObject as FlutterEngine;
      const fv = new FakeFlutterView();
      ctx.flutterViews.set('10', fv as ESObject as FlutterView);
      const host: TestSubWindowHost = new TestSubWindowHost(ctx, 10, 9, 100, 80);
      host.attach();
      await sleep(150);
      expect(fake.loadContentCount).assertEqual(1);
      expect(ctx.removedHosts.includes('10')).assertTrue();
      expect(fake.destroyCount).assertEqual(1);
      done();
    });

    it('fails when the post-load resize throws', Level.LEVEL0, async (done: Function) => {
      const fake = new FakeWindow();
      fake.rejectResize = true;
      const stage = new FakeWindowStage();
      stage.subWindow = fake as ESObject as window.Window;
      const ctx = makeCtx(stage);
      ctx.parentAbilityFor = new FakeAbility() as ESObject as UIAbility;
      ctx.engine = new Object() as ESObject as FlutterEngine;
      const fv = new FakeFlutterView();
      ctx.flutterViews.set('11', fv as ESObject as FlutterView);
      const host: TestSubWindowHost = new TestSubWindowHost(ctx, 11, 9, 100, 80);
      host.attach();
      await sleep(150);
      expect(ctx.removedHosts.includes('11')).assertTrue();
      expect(fake.destroyCount).assertEqual(1);
      done();
    });
  });

  describe('SubWindowHost.resolveWindow', () => {
    it('returns null before materialization', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      const host: TestSubWindowHost = new TestSubWindowHost(ctx, 20, 0, 100, 80);
      expect(host.resolveWindow()).assertEqual(null);
    });

    it('returns the materialized sub-window', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      const fake = new FakeWindow();
      const host: TestSubWindowHost = new TestSubWindowHost(ctx, 21, 0, 100, 80);
      host.subWindow = fake as ESObject as window.Window;
      expect(host.resolveWindow()).assertEqual(host.subWindow);
    });
  });

  describe('SubWindowHost.defaultLogicalSize', () => {
    it('defaults to 400x320', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      const host: TestSubWindowHost = new TestSubWindowHost(ctx, 22, 0, 100, 80);
      const def = host.defaultLogicalSizeForTest();
      expect(def[0]).assertEqual(400);
      expect(def[1]).assertEqual(320);
    });
  });

  describe('SubWindowHost.closeHost', () => {
    it('returns early when no sub-window exists', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      const host: TestSubWindowHost = new TestSubWindowHost(ctx, 23, 0, 100, 80);
      host.closeHostForTest();
      expect(ctx.restoredForegrounds.length).assertEqual(0);
    });

    it('destroys the sub-window and restores the parent foreground', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      const fake = new FakeWindow();
      const host: TestSubWindowHost = new TestSubWindowHost(ctx, 24, 0, 100, 80);
      host.subWindow = fake as ESObject as window.Window;
      host.closeHostForTest();
      expect(fake.destroyCount).assertEqual(1);
      expect(ctx.restoredForegrounds.includes('24')).assertTrue();
    });

    it('does not restore foreground when destroyWindow errors', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      const fake = new FakeWindow();
      fake.destroyErr = makeErr(401);
      const host: TestSubWindowHost = new TestSubWindowHost(ctx, 25, 0, 100, 80);
      host.subWindow = fake as ESObject as window.Window;
      host.closeHostForTest();
      expect(fake.destroyCount).assertEqual(1);
      expect(ctx.restoredForegrounds.length).assertEqual(0);
    });

    it('swallows a throwing destroyWindow', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      const fake = new FakeWindow();
      fake.destroyThrows = true;
      const host: TestSubWindowHost = new TestSubWindowHost(ctx, 26, 0, 100, 80);
      host.subWindow = fake as ESObject as window.Window;
      host.closeHostForTest();
      expect(fake.destroyCount).assertEqual(1);
    });
  });

  describe('SubWindowHost.closeHostForCascade', () => {
    it('returns early when no sub-window exists', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      const host: TestSubWindowHost = new TestSubWindowHost(ctx, 27, 0, 100, 80);
      const ability: UIAbility = new FakeAbility() as ESObject as UIAbility;
      host.closeHostForCascadeForTest(ability);
      expect(ctx.restoredForegrounds.length).assertEqual(0);
    });

    it('destroys the sub-window without restoring foreground', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      const fake = new FakeWindow();
      const host: TestSubWindowHost = new TestSubWindowHost(ctx, 28, 0, 100, 80);
      host.subWindow = fake as ESObject as window.Window;
      const ability: UIAbility = new FakeAbility() as ESObject as UIAbility;
      host.closeHostForCascadeForTest(ability);
      expect(fake.destroyCount).assertEqual(1);
      expect(ctx.restoredForegrounds.length).assertEqual(0);
    });

    it('logs a destroyWindow error callback', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      const fake = new FakeWindow();
      fake.destroyErr = makeErr(401);
      const host: TestSubWindowHost = new TestSubWindowHost(ctx, 29, 0, 100, 80);
      host.subWindow = fake as ESObject as window.Window;
      const ability: UIAbility = new FakeAbility() as ESObject as UIAbility;
      host.closeHostForCascadeForTest(ability);
      expect(fake.destroyCount).assertEqual(1);
    });

    it('swallows a throwing destroyWindow in cascade close', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      const fake = new FakeWindow();
      fake.destroyThrows = true;
      const host: TestSubWindowHost = new TestSubWindowHost(ctx, 31, 0, 100, 80);
      host.subWindow = fake as ESObject as window.Window;
      const ability: UIAbility = new FakeAbility() as ESObject as UIAbility;
      host.closeHostForCascadeForTest(ability);
      expect(fake.destroyCount).assertEqual(1);
      expect(ctx.restoredForegrounds.length).assertEqual(0);
    });
  });

  describe('SubWindowHost.handleOsClosed', () => {
    it('notifies native then runs base cleanup', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      const fv = new FakeFlutterView();
      ctx.flutterViews.set('30', fv as ESObject as FlutterView);
      const host: TestSubWindowHost = new TestSubWindowHost(ctx, 30, 0, 100, 80);
      host.handleOsClosed();
      expect(fv.detachCalls).assertEqual(1);
      expect(ctx.removedHosts.includes('30')).assertTrue();
    });
  });
}
