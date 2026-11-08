/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, expect, it, Level } from '@ohos/hypium';
import window from '@ohos.window';
import { FlutterHostWindowSized } from
  '../../../../../../main/ets/embedding/ohos/windowing/FlutterHostWindowSized';
import { NO_PARENT_VIEW_ID, WindowArchetype,
  WindowConstraints } from '../../../../../../main/ets/embedding/ohos/windowing/WindowingUtils';
import { WindowHostContext } from
  '../../../../../../main/ets/embedding/ohos/windowing/WindowHostContext';
import { FakeWindow, FakeHostContext, sleep } from './Fakes';

/** Concrete sized host: fixed density 1.0 seam, REGULAR archetype, no parent
 * (moveWindow early-outs — placement is not under test here). */
class TestSizedHost extends FlutterHostWindowSized {
  constructor(ctx: WindowHostContext, viewId: number,
              sizedToContent: boolean, resizable: boolean,
              constraints: WindowConstraints = new WindowConstraints(),
              parentViewId: number = NO_PARENT_VIEW_ID) {
    super(ctx, viewId, parentViewId, '', WindowArchetype.REGULAR,
      sizedToContent, resizable, constraints);
  }

  async attach(): Promise<void> {
  }

  protected densityPixels(_win: window.Window | undefined): number {
    return 1.0;
  }

  /** Test probe for the protected border-drag switch. */
  callSetWindowStyleForResizable(): void {
    this.setWindowStyleForResizable();
  }
}

/** Non-resizable content-sized host mounted on a FakeWindow. */
function mountedHost(ctx: FakeHostContext, fake: FakeWindow, viewId: number): TestSizedHost {
  const host = new TestSizedHost(ctx, viewId, true, false);
  host.setWindow(fake as ESObject as window.Window);
  return host;
}

/** Child host (viewId) parented to a FakeWindow-backed host at view 5. */
function parentedChild(ctx: FakeHostContext, viewId: number,
                       sizedToContent: boolean): TestSizedHost {
  const parentFake = new FakeWindow();
  const parent = new TestSizedHost(ctx, 5, true, false);
  parent.setWindow(parentFake as ESObject as window.Window);
  ctx.hosts.set('5', parent);
  const child = new TestSizedHost(ctx, viewId, sizedToContent, false,
    new WindowConstraints(), 5);
  child.setWindow(new FakeWindow() as ESObject as window.Window);
  return child;
}

export default function flutterHostWindowSizedTest() {
  describe('FlutterHostWindowSized', () => {
    describe('setWindowSize (Dart setSize path)', () => {
      it('pins min=max then resizes on content settle, setSize follows without a new pin',
        Level.LEVEL0, async (done: Function) => {
          const ctx = new FakeHostContext();
          const fake = new FakeWindow();
          const host = mountedHost(ctx, fake, 31);
          // Content settles to 300x200: the min=max pin (isForcible) is the
          // only way below the OS free-window minimum, then the resize.
          host.didUpdateViewSize(300, 200);
          expect(fake.constraintCalls.length).assertEqual(1);
          expect(fake.constraintCalls[0][0]).assertEqual(300); // minWidth
          expect(fake.constraintCalls[0][1]).assertEqual(300); // maxWidth
          expect(fake.constraintCalls[0][2]).assertEqual(200); // minHeight
          await sleep(0); // pin promise chain → resize
          expect(fake.resizeCalls.length).assertEqual(1);
          expect(fake.resizeCalls[0][0]).assertEqual(300);
          expect(fake.resizeCalls[0][1]).assertEqual(200);
          // Dart controller.setSize(500, 400) goes through the base
          // setWindowSize: a plain resize, no additional pin push.
          host.setWindowSize(500, 400);
          expect(fake.constraintCalls.length).assertEqual(1);
          expect(fake.resizeCalls.length).assertEqual(2);
          expect(fake.resizeCalls[1][0]).assertEqual(500);
          expect(fake.resizeCalls[1][1]).assertEqual(400);
          done();
        });

      it('plain resize for resizable hosts (no pin to sync)',
        Level.LEVEL0, async (done: Function) => {
          const ctx = new FakeHostContext();
          const fake = new FakeWindow();
          const host = new TestSizedHost(ctx, 32, true, true);
          host.setWindow(fake as ESObject as window.Window);
          host.setWindowSize(500, 400);
          expect(fake.constraintCalls.length).assertEqual(0);
          expect(fake.resizeCalls.length).assertEqual(1);
          expect(fake.resizeCalls[0][0]).assertEqual(500);
          expect(fake.resizeCalls[0][1]).assertEqual(400);
          await sleep(0); // nothing async in flight
          done();
        });

      it('non-resizable fixed-size host: setSize resizes without pinning',
        Level.LEVEL0, async (done: Function) => {
          const ctx = new FakeHostContext();
          const fake = new FakeWindow();
          const host = new TestSizedHost(ctx, 33, false, false);
          host.setWindow(fake as ESObject as window.Window);
          host.setWindowSize(640, 480);
          expect(fake.constraintCalls.length).assertEqual(0);
          expect(fake.resizeCalls.length).assertEqual(1);
          expect(fake.resizeCalls[0][0]).assertEqual(640);
          expect(fake.resizeCalls[0][1]).assertEqual(480);
          done();
        });

      it('clamps setSize against the stored app limits',
        Level.LEVEL0, async (done: Function) => {
          const ctx = new FakeHostContext();
          const fake = new FakeWindow();
          const host = mountedHost(ctx, fake, 34);
          host.setWindowLimits({ minWidth: 100, minHeight: 80, maxWidth: 450, maxHeight: 0 });
          await sleep(0); // base setWindowLimits push (recorded)
          const pushesAfterLimitSync = fake.constraintCalls.length;
          host.setWindowSize(500, 400); // width clamped to max 450, no new push
          expect(fake.constraintCalls.length).assertEqual(pushesAfterLimitSync);
          await sleep(0);
          expect(fake.resizeCalls.length).assertEqual(1);
          expect(fake.resizeCalls[0][0]).assertEqual(450);
          done();
        });

      it('setWindowStyleForResizable disables border-drag for fixed windows only',
        Level.LEVEL0, (done: Function) => {
          const fake = new FakeWindow();
          const fixed = new TestSizedHost(new FakeHostContext(), 35, false, false);
          fixed.setWindow(fake as ESObject as window.Window);
          fixed.callSetWindowStyleForResizable();
          expect(fake.resizeByDragCalls.length).assertEqual(1);
          expect(fake.resizeByDragCalls[0]).assertEqual(false);
          const flexFake = new FakeWindow();
          const flex = new TestSizedHost(new FakeHostContext(), 36, false, true);
          flex.setWindow(flexFake as ESObject as window.Window);
          flex.callSetWindowStyleForResizable();
          expect(flexFake.resizeByDragCalls.length).assertEqual(0);
          done();
        });
    });
    describe('didUpdateViewSize (content-driven resize path)', () => {
      it('resizable host: plain resize then native tracking off (Dart setSize takes over)',
        Level.LEVEL0, async (done: Function) => {
          const ctx = new FakeHostContext();
          const fake = new FakeWindow();
          const host = new TestSizedHost(ctx, 37, true, true);
          host.setWindow(fake as ESObject as window.Window);
          host.didUpdateViewSize(300, 200);
          // Resizable branch: a plain resize, no min=max pin push.
          expect(fake.constraintCalls.length).assertEqual(0);
          expect(fake.resizeCalls.length).assertEqual(1);
          expect(fake.resizeCalls[0][0]).assertEqual(300);
          expect(fake.resizeCalls[0][1]).assertEqual(200);
          await sleep(0); // resize promise → setViewSizedToContent(viewId, false)
          expect(ctx.sizedToContent.length).assertEqual(1);
          expect(ctx.sizedToContent[0][0]).assertEqual(37);
          expect(ctx.sizedToContent[0][1]).assertEqual(0);
          // A duplicate report is deduped: no second resize, no second handoff.
          host.didUpdateViewSize(300, 200);
          expect(fake.resizeCalls.length).assertEqual(1);
          expect(ctx.sizedToContent.length).assertEqual(1);
          done();
        });

      it('resizable host: rejected resize resets dedup so the next report retries',
        Level.LEVEL0, async (done: Function) => {
          const ctx = new FakeHostContext();
          const fake = new FakeWindow();
          fake.rejectResize = true;
          const host = new TestSizedHost(ctx, 38, true, true);
          host.setWindow(fake as ESObject as window.Window);
          host.didUpdateViewSize(300, 200);
          expect(fake.resizeCalls.length).assertEqual(1);
          await sleep(0); // catch → physicalWidth/Height reset
          // No tracking handoff while the resize keeps failing.
          expect(ctx.sizedToContent.length).assertEqual(0);
          // The same report retries once resize recovers (dedup was reset).
          fake.rejectResize = false;
          host.didUpdateViewSize(300, 200);
          expect(fake.resizeCalls.length).assertEqual(2);
          await sleep(0);
          expect(ctx.sizedToContent.length).assertEqual(1);
          expect(ctx.sizedToContent[0][0]).assertEqual(38);
          expect(ctx.sizedToContent[0][1]).assertEqual(0);
          done();
        });
    });
    describe('moveWindow (content-settle placement over the parent)', () => {
      it('sized-to-content: exact parent center, NO cascade (attach parity)',
        Level.LEVEL0, (done: Function) => {
          const ctx = new FakeHostContext();
          const host = parentedChild(ctx, 40, true);
          host.didUpdateViewSize(300, 200); // → window 300x200 over 1000x800
          const fake = host.getWindow() as ESObject as FakeWindow;
          expect(fake.moveCalls.length).assertEqual(1);
          expect(fake.moveCalls[0][0]).assertEqual(350); // (1000-300)/2
          expect(fake.moveCalls[0][1]).assertEqual(300); // (800-200)/2
          done();
        });

      it('fixed-size: parent center minus the nested-dialog cascade',
        Level.LEVEL0, (done: Function) => {
          const ctx = new FakeHostContext();
          const host = parentedChild(ctx, 41, false);
          host.didUpdateViewSize(300, 200);
          const fake = host.getWindow() as ESObject as FakeWindow;
          expect(fake.moveCalls.length).assertEqual(1);
          expect(fake.moveCalls[0][0]).assertEqual(350 - 20); // cascade
          expect(fake.moveCalls[0][1]).assertEqual(300 - 20);
          done();
        });

      it('parentless host never moves (Regular path)',
        Level.LEVEL0, (done: Function) => {
          const ctx = new FakeHostContext();
          const fake = new FakeWindow();
          const host = new TestSizedHost(ctx, 42, true, false);
          host.setWindow(fake as ESObject as window.Window);
          host.didUpdateViewSize(300, 200);
          expect(fake.moveCalls.length).assertEqual(0);
          done();
        });
    });
  });
}
