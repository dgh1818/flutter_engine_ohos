/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, expect, it, Level } from '@ohos/hypium';
import window from '@ohos.window';
import UIAbility from '@ohos.app.ability.UIAbility';
import { DialogWindowHost } from
  '../../../../../../main/ets/embedding/ohos/windowing/DialogWindowHost';
import { WindowArchetype } from
  '../../../../../../main/ets/embedding/ohos/windowing/WindowArchetype';
import { WindowHostContext } from
  '../../../../../../main/ets/embedding/ohos/windowing/WindowHostContext';
import {
  FakeWindow, FakeWindowNoDialogApi, FakeWindowNoDrag, FakeWindowStage,
  FakeAbility, FakeHostContext,
} from './Fakes';

/** Concrete subclass exposing the protected hooks of DialogWindowHost. */
class TestDialogHost extends DialogWindowHost {
  constructor(ctx: WindowHostContext, viewId: number, parentViewId: number, w: number, h: number) {
    super(ctx, viewId, parentViewId, w, h);
  }

  setParentForTest(ability: UIAbility | undefined): void {
    this.parentAbility = ability;
  }

  async placeInitialForTest(subWindow: window.Window, lw: number, lh: number,
    pw: number, ph: number): Promise<void> {
    await this.placeInitial(subWindow, lw, lh, pw, ph);
  }

  async prepareShowForTest(subWindow: window.Window): Promise<void> {
    await this.prepareShow(subWindow);
  }

  async configureAfterShowForTest(subWindow: window.Window): Promise<void> {
    await this.configureAfterShow(subWindow);
  }
}

/** Context with a parent ability + windowStage + a 1000x800 main window. */
function makeDialogCtx(): FakeHostContext {
  const stage = new FakeWindowStage();
  const mainWin = new FakeWindow();
  stage.mainWindow = mainWin as ESObject as window.Window;
  const ctx = new FakeHostContext();
  ctx.stage = stage as ESObject as window.WindowStage;
  const ability: UIAbility = new FakeAbility() as ESObject as UIAbility;
  ctx.parentAbilityFor = ability;
  return ctx;
}

export default function dialogWindowHostTest() {
  describe('DialogWindowHost.constructor', () => {
    it('passes the DIALOG archetype', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      const host: TestDialogHost = new TestDialogHost(ctx, 1, 0, 100, 80);
      expect(host.archetype).assertEqual(WindowArchetype.DIALOG);
    });
  });

  describe('DialogWindowHost.placeInitial', () => {
    it('returns early without a parent ability', Level.LEVEL0, async (done: Function) => {
      const ctx = new FakeHostContext();
      const fake = new FakeWindow();
      const host: TestDialogHost = new TestDialogHost(ctx, 2, 0, 100, 80);
      await host.placeInitialForTest(fake as ESObject as window.Window, 100, 80, 200, 160);
      expect(fake.moveCalls.length).assertEqual(0);
      done();
    });

    it('returns early without a windowStage', Level.LEVEL0, async (done: Function) => {
      const ctx = new FakeHostContext();
      ctx.parentAbilityFor = new FakeAbility() as ESObject as UIAbility;
      const fake = new FakeWindow();
      const host: TestDialogHost = new TestDialogHost(ctx, 3, 0, 100, 80);
      host.setParentForTest(ctx.parentAbilityFor);
      await host.placeInitialForTest(fake as ESObject as window.Window, 100, 80, 200, 160);
      expect(fake.moveCalls.length).assertEqual(0);
      done();
    });

    it('centers the dialog over the parent main window', Level.LEVEL0, async (done: Function) => {
      const ctx = makeDialogCtx();
      const fake = new FakeWindow();
      const host: TestDialogHost = new TestDialogHost(ctx, 4, 0, 100, 80);
      host.setParentForTest(ctx.parentAbilityFor);
      await host.placeInitialForTest(fake as ESObject as window.Window, 100, 80, 200, 160);
      expect(fake.moveCalls.length).assertEqual(1);
      expect(fake.moveCalls[0][0]).assertEqual(400);
      expect(fake.moveCalls[0][1]).assertEqual(320);
      done();
    });

    it('cascades up-left when the parent is itself a sub-window', Level.LEVEL0,
      async (done: Function) => {
        const ctx = makeDialogCtx();
        const parentSub = new FakeWindow();
        parentSub.props.setWindowRect(50, 60, 400, 300);
        ctx.subWindows.set('9', parentSub as ESObject as window.Window);
        const fake = new FakeWindow();
        const host: TestDialogHost = new TestDialogHost(ctx, 5, 9, 100, 80);
        host.setParentForTest(ctx.parentAbilityFor);
        await host.placeInitialForTest(fake as ESObject as window.Window, 100, 80, 200, 160);
        expect(fake.moveCalls.length).assertEqual(1);
        expect(fake.moveCalls[0][0]).assertEqual(110);
        expect(fake.moveCalls[0][1]).assertEqual(90);
        done();
      });

    it('logs a rejected move', Level.LEVEL0, async (done: Function) => {
      const ctx = makeDialogCtx();
      const fake = new FakeWindow();
      fake.rejectMove = true;
      const host: TestDialogHost = new TestDialogHost(ctx, 6, 0, 100, 80);
      host.setParentForTest(ctx.parentAbilityFor);
      await host.placeInitialForTest(fake as ESObject as window.Window, 100, 80, 200, 160);
      expect(fake.moveCalls.length).assertEqual(1);
      done();
    });
  });

  describe('DialogWindowHost.prepareShow', () => {
    it('enables title-bar drag', Level.LEVEL0, async (done: Function) => {
      const ctx = new FakeHostContext();
      const fake = new FakeWindow();
      const host: TestDialogHost = new TestDialogHost(ctx, 7, 0, 100, 80);
      await host.prepareShowForTest(fake as ESObject as window.Window);
      expect(fake.titleMoveEnabled).assertTrue();
      done();
    });

    it('skips when the title-move API is absent', Level.LEVEL0, async (done: Function) => {
      const ctx = new FakeHostContext();
      const fake = new FakeWindowNoDialogApi();
      const host: TestDialogHost = new TestDialogHost(ctx, 8, 0, 100, 80);
      await host.prepareShowForTest(fake as ESObject as window.Window);
      done();
    });

    it('logs a throwing title-move enable', Level.LEVEL0, async (done: Function) => {
      const ctx = new FakeHostContext();
      const fake = new FakeWindow();
      fake.titleMoveThrows = true;
      const host: TestDialogHost = new TestDialogHost(ctx, 9, 0, 100, 80);
      await host.prepareShowForTest(fake as ESObject as window.Window);
      done();
    });
  });

  describe('DialogWindowHost.configureAfterShow', () => {
    it('sets modal then enables drag', Level.LEVEL0, async (done: Function) => {
      const ctx = new FakeHostContext();
      const fake = new FakeWindow();
      const host: TestDialogHost = new TestDialogHost(ctx, 10, 0, 100, 80);
      await host.configureAfterShowForTest(fake as ESObject as window.Window);
      expect(fake.modalCalls.length).assertEqual(1);
      expect(fake.modalCalls[0][0]).assertEqual(1);
      expect(fake.modalCalls[0][1]).assertEqual(0);
      expect(fake.dragEnabled).assertTrue();
      done();
    });

    it('warns and skips when the modal API is absent', Level.LEVEL0, async (done: Function) => {
      const ctx = new FakeHostContext();
      const fake = new FakeWindowNoDialogApi();
      const host: TestDialogHost = new TestDialogHost(ctx, 11, 0, 100, 80);
      await host.configureAfterShowForTest(fake as ESObject as window.Window);
      done();
    });

    it('logs a rejected modal set', Level.LEVEL0, async (done: Function) => {
      const ctx = new FakeHostContext();
      const fake = new FakeWindow();
      fake.rejectModal = true;
      const host: TestDialogHost = new TestDialogHost(ctx, 12, 0, 100, 80);
      await host.configureAfterShowForTest(fake as ESObject as window.Window);
      expect(fake.modalCalls.length).assertEqual(1);
      done();
    });

    it('sets modal but skips the absent drag API', Level.LEVEL0, async (done: Function) => {
      const ctx = new FakeHostContext();
      const fake = new FakeWindowNoDrag();
      const host: TestDialogHost = new TestDialogHost(ctx, 13, 0, 100, 80);
      await host.configureAfterShowForTest(fake as ESObject as window.Window);
      expect(fake.modalCalls.length).assertEqual(1);
      expect(fake.modalCalls[0][0]).assertEqual(1);
      done();
    });

    it('logs a rejected drag enable', Level.LEVEL0, async (done: Function) => {
      const ctx = new FakeHostContext();
      const fake = new FakeWindow();
      fake.rejectDrag = true;
      const host: TestDialogHost = new TestDialogHost(ctx, 14, 0, 100, 80);
      await host.configureAfterShowForTest(fake as ESObject as window.Window);
      expect(fake.modalCalls.length).assertEqual(1);
      done();
    });
  });
}
