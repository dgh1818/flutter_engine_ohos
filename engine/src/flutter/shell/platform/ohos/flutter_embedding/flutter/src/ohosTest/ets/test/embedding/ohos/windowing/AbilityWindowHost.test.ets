/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, expect, it, Level } from '@ohos/hypium';
import window from '@ohos.window';
import UIAbility from '@ohos.app.ability.UIAbility';
import { common } from '@kit.AbilityKit';
import { AbilityWindowHost, AbilityHostMode } from
  '../../../../../../main/ets/embedding/ohos/windowing/AbilityWindowHost';
import { WindowHostContext } from
  '../../../../../../main/ets/embedding/ohos/windowing/WindowHostContext';
import {
  FakeWindow, FakeWindowStage, FakeAbility, FakeAbilityContext,
  FakeHostContext, sleep,
} from './Fakes';

/** Concrete subclass exposing the protected teardown hooks. */
class TestAbilityHost extends AbilityWindowHost {
  constructor(ctx: WindowHostContext, viewId: number, mode: AbilityHostMode,
    requestId: number, w: number, h: number, title: string) {
    super(ctx, viewId, mode, requestId, w, h, title);
  }

  closeHostForTest(): void {
    this.closeHost();
  }

  closeHostForCascadeForTest(ability: UIAbility): void {
    this.closeHostForCascade(ability);
  }
}

/** resize() throws synchronously — exercises the attachAdopted catch. */
class ThrowingResizeWindow extends FakeWindow {
  resize(w: number, h: number): Promise<void> {
    throw new Error('resize-throws');
  }
}

/** setWindowTitle() throws synchronously — exercises the attachAdopted catch. */
class ThrowingTitleWindow extends FakeWindow {
  setWindowTitle(title: string): Promise<void> {
    throw new Error('title-throws');
  }
}

/** Adoptable context + main window + ability + stage (ArkTS: no tuple
 * destructure). */
class AdoptCtx {
  ctx: FakeHostContext;
  mainWin: FakeWindow;
  ability: FakeAbility;
  stage: FakeWindowStage;

  constructor(ctx: FakeHostContext, mainWin: FakeWindow, ability: FakeAbility,
    stage: FakeWindowStage) {
    this.ctx = ctx;
    this.mainWin = mainWin;
    this.ability = ability;
    this.stage = stage;
  }
}

/** Context where view 0 is the adopted main view of an ability+stage. */
function makeAdoptCtx(): AdoptCtx {
  const stage = new FakeWindowStage();
  const mainWin = new FakeWindow();
  stage.mainWindow = mainWin as ESObject as window.Window;
  const ability = new FakeAbility();
  const ctx = new FakeHostContext();
  ctx.stage = stage as ESObject as window.WindowStage;
  ctx.mainViewId = '0';
  ctx.viewAbilities.set('0', ability as ESObject as UIAbility);
  ctx.dpr = 2.0;
  return new AdoptCtx(ctx, mainWin, ability, stage);
}

/** Host whose view ability terminates via a FakeAbilityContext. */
class TerminateHost {
  ctx: FakeHostContext;
  actx: FakeAbilityContext;
  ability: UIAbility;
  host: TestAbilityHost;

  constructor(ctx: FakeHostContext, actx: FakeAbilityContext, ability: UIAbility,
    host: TestAbilityHost) {
    this.ctx = ctx;
    this.actx = actx;
    this.ability = ability;
    this.host = host;
  }
}

function makeTerminateHost(viewId: number): TerminateHost {
  const ctx = new FakeHostContext();
  const actx = new FakeAbilityContext();
  const ability = new FakeAbility();
  ability.context = actx as ESObject as common.UIAbilityContext;
  const abilityAsUI: UIAbility = ability as ESObject as UIAbility;
  ctx.viewAbilities.set(String(viewId), abilityAsUI);
  const host: TestAbilityHost = new TestAbilityHost(ctx, viewId, AbilityHostMode.SPAWNED, 1, 0, 0, '');
  return new TerminateHost(ctx, actx, abilityAsUI, host);
}

export default function abilityWindowHostTest() {
  describe('AbilityWindowHost.constructor', () => {
    it('ADOPTED host reports isAdopted', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      const host: TestAbilityHost = new TestAbilityHost(ctx, 0, AbilityHostMode.ADOPTED, 1, 0, 0, '');
      expect(host.isAdopted()).assertTrue();
    });

    it('SPAWNED host reports not adopted', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      const host: TestAbilityHost = new TestAbilityHost(ctx, 5, AbilityHostMode.SPAWNED, 1, 0, 0, '');
      expect(host.isAdopted()).assertFalse();
    });
  });

  describe('AbilityWindowHost.attachAdopted', () => {
    it('returns early when no main ability is registered', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      const host: TestAbilityHost = new TestAbilityHost(ctx, 0, AbilityHostMode.ADOPTED, 1, 200, 100, 'mywin');
      host.attach();
    });

    it('returns early when the main ability has no windowStage', Level.LEVEL0, () => {
      const a = makeAdoptCtx();
      a.ctx.stage = undefined;
      const host: TestAbilityHost = new TestAbilityHost(a.ctx, 0, AbilityHostMode.ADOPTED, 1, 200, 100, 'mywin');
      host.attach();
    });

    it('applies preferred size and title to the main window', Level.LEVEL0, () => {
      const a = makeAdoptCtx();
      const host: TestAbilityHost = new TestAbilityHost(a.ctx, 0, AbilityHostMode.ADOPTED, 1, 200, 100, 'mywin');
      host.attach();
      expect(a.mainWin.resizeCalls.length).assertEqual(1);
      expect(a.mainWin.resizeCalls[0][0]).assertEqual(400);
      expect(a.mainWin.resizeCalls[0][1]).assertEqual(200);
      expect(a.mainWin.titles.length).assertEqual(1);
      expect(a.mainWin.titles[0]).assertEqual('mywin');
    });

    it('skips resize for zero requested size', Level.LEVEL0, () => {
      const a = makeAdoptCtx();
      const host: TestAbilityHost = new TestAbilityHost(a.ctx, 0, AbilityHostMode.ADOPTED, 1, 0, 0, 'mywin');
      host.attach();
      expect(a.mainWin.resizeCalls.length).assertEqual(0);
      expect(a.mainWin.titles[0]).assertEqual('mywin');
    });

    it('skips the title when empty', Level.LEVEL0, () => {
      const a = makeAdoptCtx();
      const host: TestAbilityHost = new TestAbilityHost(a.ctx, 0, AbilityHostMode.ADOPTED, 1, 200, 100, '');
      host.attach();
      expect(a.mainWin.resizeCalls.length).assertEqual(1);
      expect(a.mainWin.titles.length).assertEqual(0);
    });

    it('logs a throwing resize and still sets the title', Level.LEVEL0, () => {
      const a = makeAdoptCtx();
      a.stage.mainWindow = new ThrowingResizeWindow() as ESObject as window.Window;
      const host: TestAbilityHost = new TestAbilityHost(a.ctx, 0, AbilityHostMode.ADOPTED, 1, 200, 100, 'mywin');
      host.attach();
      const mainWin = a.stage.mainWindow as ESObject as ThrowingResizeWindow;
      expect(mainWin.titles.length).assertEqual(1);
      expect(mainWin.titles[0]).assertEqual('mywin');
    });

    it('logs a throwing title set', Level.LEVEL0, () => {
      const a = makeAdoptCtx();
      a.stage.mainWindow = new ThrowingTitleWindow() as ESObject as window.Window;
      const host: TestAbilityHost = new TestAbilityHost(a.ctx, 0, AbilityHostMode.ADOPTED, 1, 200, 100, 'mywin');
      host.attach();
      const mainWin = a.stage.mainWindow as ESObject as ThrowingTitleWindow;
      expect(mainWin.resizeCalls.length).assertEqual(1);
    });
  });

  describe('AbilityWindowHost.attachSpawned', () => {
    it('fails when no launch context is available', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      const host: TestAbilityHost = new TestAbilityHost(ctx, 5, AbilityHostMode.SPAWNED, 7, 200, 300, 'win');
      host.attach();
      expect(ctx.removedHosts.includes('5')).assertTrue();
    });

    it('fails when the context has no bundleName', Level.LEVEL0, () => {
      const a = makeAdoptCtx();
      const noBundle = new FakeAbilityContext();
      noBundle.abilityInfo = new Object();
      a.ability.context = noBundle as ESObject as common.UIAbilityContext;
      const host: TestAbilityHost = new TestAbilityHost(a.ctx, 5, AbilityHostMode.SPAWNED, 7, 200, 300, 'win');
      host.attach();
      expect(a.ctx.removedHosts.includes('5')).assertTrue();
    });

    it('launches a RegularWindowAbility with the expected want', Level.LEVEL0, () => {
      const a = makeAdoptCtx();
      const fakeCtx = new FakeAbilityContext();
      a.ability.context = fakeCtx as ESObject as common.UIAbilityContext;
      const host: TestAbilityHost = new TestAbilityHost(a.ctx, 5, AbilityHostMode.SPAWNED, 7, 200, 300, 'win');
      host.attach();
      expect(fakeCtx.startAbilityCount).assertEqual(1);
      const want = fakeCtx.startedWants[0];
      expect(want.bundleName).assertEqual('com.example.flutter_tester');
      expect(want.abilityName).assertEqual('RegularWindowAbility');
      const params = want.parameters;
      expect(params !== undefined).assertTrue();
      if (params !== undefined) {
        expect(params['flutter_view_id']).assertEqual('5');
        expect(params['instanceKey']).assertEqual('7');
        expect(params['flutter_window_request_id']).assertEqual('7');
        expect(params['flutter_preferred_width']).assertEqual('200');
        expect(params['flutter_preferred_height']).assertEqual('300');
        expect(params['flutter_window_title']).assertEqual('win');
      }
    });

    it('fails when startAbility rejects', Level.LEVEL0, async (done: Function) => {
      const a = makeAdoptCtx();
      const fakeCtx = new FakeAbilityContext();
      fakeCtx.rejectStart = true;
      a.ability.context = fakeCtx as ESObject as common.UIAbilityContext;
      const host: TestAbilityHost = new TestAbilityHost(a.ctx, 5, AbilityHostMode.SPAWNED, 7, 200, 300, 'win');
      host.attach();
      await sleep(50);
      expect(fakeCtx.startAbilityCount).assertEqual(1);
      expect(a.ctx.removedHosts.includes('5')).assertTrue();
      done();
    });
  });

  describe('AbilityWindowHost.resolveWindow', () => {
    it('returns null without a main view id', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      const host: TestAbilityHost = new TestAbilityHost(ctx, 0, AbilityHostMode.ADOPTED, 1, 0, 0, '');
      expect(host.resolveWindow()).assertEqual(null);
    });

    it('resolves the adopted main window via the main view id', Level.LEVEL0, () => {
      const a = makeAdoptCtx();
      const host: TestAbilityHost = new TestAbilityHost(a.ctx, 0, AbilityHostMode.ADOPTED, 1, 0, 0, '');
      expect(host.resolveWindow()).assertEqual(a.mainWin);
    });

    it('returns null when the ability has no windowStage', Level.LEVEL0, () => {
      const a = makeAdoptCtx();
      a.ctx.stage = undefined;
      const host: TestAbilityHost = new TestAbilityHost(a.ctx, 0, AbilityHostMode.ADOPTED, 1, 0, 0, '');
      expect(host.resolveWindow()).assertEqual(null);
    });

    it('returns null when getMainWindowSync throws', Level.LEVEL0, () => {
      const a = makeAdoptCtx();
      a.stage.getMainThrows = true;
      const host: TestAbilityHost = new TestAbilityHost(a.ctx, 0, AbilityHostMode.ADOPTED, 1, 0, 0, '');
      expect(host.resolveWindow()).assertEqual(null);
    });

    it('resolves a spawned ability window by decimal view id', Level.LEVEL0, () => {
      const a = makeAdoptCtx();
      a.ctx.viewAbilities.delete('0');
      a.ctx.viewAbilities.set('5', a.ability as ESObject as UIAbility);
      const host: TestAbilityHost = new TestAbilityHost(a.ctx, 5, AbilityHostMode.SPAWNED, 1, 0, 0, '');
      expect(host.resolveWindow()).assertEqual(a.mainWin);
    });

    it('returns null when the ability is missing', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      ctx.stage = new FakeWindowStage() as ESObject as window.WindowStage;
      const host: TestAbilityHost = new TestAbilityHost(ctx, 5, AbilityHostMode.SPAWNED, 1, 0, 0, '');
      expect(host.resolveWindow()).assertEqual(null);
    });
  });

  describe('AbilityWindowHost.closeHost', () => {
    it('terminates the host ability', Level.LEVEL0, () => {
      const t = makeTerminateHost(20);
      t.host.closeHostForTest();
      expect(t.actx.terminateCount).assertEqual(1);
    });

    it('logs a rejected terminateSelf', Level.LEVEL0, async (done: Function) => {
      const t = makeTerminateHost(21);
      t.actx.rejectTerminate = true;
      t.host.closeHostForTest();
      await sleep(50);
      expect(t.actx.terminateCount).assertEqual(1);
      done();
    });

    it('logs a throwing terminateSelf', Level.LEVEL0, () => {
      const t = makeTerminateHost(22);
      t.actx.terminateThrows = true;
      t.host.closeHostForTest();
      expect(t.actx.terminateCount).assertEqual(1);
    });

    it('logs when no host ability exists', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      const host: TestAbilityHost = new TestAbilityHost(ctx, 23, AbilityHostMode.SPAWNED, 1, 0, 0, '');
      host.closeHostForTest();
    });
  });

  describe('AbilityWindowHost.closeHostForCascade', () => {
    it('skips termination when the ability is the cascade parent', Level.LEVEL0, () => {
      const t = makeTerminateHost(24);
      t.host.closeHostForCascadeForTest(t.ability);
      expect(t.actx.terminateCount).assertEqual(0);
    });

    it('terminates when the cascade parent differs', Level.LEVEL0, () => {
      const t = makeTerminateHost(25);
      const other: UIAbility = new FakeAbility() as ESObject as UIAbility;
      t.host.closeHostForCascadeForTest(other);
      expect(t.actx.terminateCount).assertEqual(1);
    });

    it('logs when no host ability exists for cascade', Level.LEVEL0, () => {
      const ctx = new FakeHostContext();
      const host: TestAbilityHost = new TestAbilityHost(ctx, 26, AbilityHostMode.SPAWNED, 1, 0, 0, '');
      const ability: UIAbility = new FakeAbility() as ESObject as UIAbility;
      host.closeHostForCascadeForTest(ability);
    });
  });
}
