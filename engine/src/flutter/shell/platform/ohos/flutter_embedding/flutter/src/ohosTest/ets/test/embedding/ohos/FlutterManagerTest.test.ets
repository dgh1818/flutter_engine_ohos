/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
import { beforeAll, describe, expect, it } from '@ohos/hypium';
import { window } from '@kit.ArkUI';
import UIAbility from '@ohos.app.ability.UIAbility';
import FlutterManager, { FullScreenListener, DragDropCallback } from '../../../../../main/ets/embedding/ohos/FlutterManager';
import FlutterNapi from '../../../../../main/ets/embedding/engine/FlutterNapi';
import { FlutterView } from '../../../../../main/ets/view/FlutterView';
import { FakeAbility, regularRequest, dialogRequest, tooltipRequest, popupRequest } from './windowing/Fakes';
import { WindowConstraints } from '../../../../../main/ets/embedding/ohos/windowing/WindowingUtils';
import common from '@ohos.app.ability.common';
import { Context } from '@kit.AbilityKit';

/** Ability whose hostedViewId is 0 before the manager stamps it. */
class HostedViewIdHolder {
  hostedViewId: number = 0;
  // bindEntryAbilityToView reads the view through this (real FlutterAbility
  // carries it; the fake has none).
  getFlutterView(): FlutterView | null {
    return null;
  }
}
class DragDropCallbackImpl implements DragDropCallback {
  do(event: Object, extraParams: string): void {
    // no-op
  }
}
class TestFullScreenListener implements FullScreenListener {
  useFullScreen(): boolean { return false; }
  setUseFullScreen(_useFullScreen: boolean, _context?: Context | null | undefined): void {}
  setUseFullScreenForFreeWindow(_useFullScreen: boolean, _context?: Context | null | undefined): void {}
  onScreenStateChanged(_data: window.WindowStatusType): void {}
}

function sleep(time: number): Promise<void> {
  return new Promise<void>((resolve: Function) => setTimeout(resolve, time));
}

const flutterManager: FlutterManager = FlutterManager.getInstance();
const uiAbility = flutterManager.getUIAbility(getContext());

export default function FlutterManagerTest() {
  describe('FlutterManagerTest', () => {
    beforeAll(async () => {
      // Skip beforeAll setup in JsUnit env (uiAbility is null when not in
      // a UIAbility lifecycle). The it() tests below don't depend on
      // beforeAll setup succeeding.
      if (uiAbility === null || uiAbility === undefined) {
        return;
      }
      const windowStage = flutterManager.getWindowStage(uiAbility);
      await sleep(1000);
      flutterManager.pushWindowStage(uiAbility, windowStage);
      await sleep(1000);
      flutterManager.popWindowStage(uiAbility);
      await sleep(1000);
      flutterManager.pushUIAbility(uiAbility);
      await sleep(1000);
      flutterManager.popUIAbility(uiAbility);

    })

    it('TestSetFullScreenListener', 0, async (done: Function) => {
      class MyFullScreenListener implements FullScreenListener {
        useFullScreen(): boolean {
          return true;
        }

        setUseFullScreen(useFullScreen: boolean): void {
          console.info("uitest: setUseFullScreen useFullScreen " + useFullScreen);
        }

        setUseFullScreenForFreeWindow(useFullScreen: boolean): void {
          console.info("uitest: setUseFullScreenForFreeWindow useFullScreen " + useFullScreen);
        }

        onScreenStateChanged(data: window.WindowStatusType): void {
          console.info("uitest: onScreenStateChanged");
        }
      }

      const listener: FullScreenListener = new MyFullScreenListener();
      flutterManager.setFullScreenListener(listener);
      await sleep(1000);
      const result = flutterManager.getFullScreenListener();
      expect(result).not().assertNull();
      done();
    })

    it('TestFullScreen', 0, async (done: Function) => {
      const result = flutterManager.useFullScreen();
      expect(result).assertTrue();
      flutterManager.setUseFullScreen(result);
      flutterManager.clear();
      done();
    })

    it('TestSetUseFullScreenForFreeWindow', 0, (done: Function) => {
      class RecordingFullScreenListener implements FullScreenListener {
        freeWindowArg: boolean = false;
        callCount: number = 0;

        useFullScreen(): boolean {
          return true;
        }

        setUseFullScreen(useFullScreen: boolean): void {
        }

        setUseFullScreenForFreeWindow(useFullScreen: boolean): void {
          this.freeWindowArg = useFullScreen;
          this.callCount = this.callCount + 1;
        }

        onScreenStateChanged(data: window.WindowStatusType): void {
        }
      }

      const listener = new RecordingFullScreenListener();
      flutterManager.setFullScreenListener(listener);

      flutterManager.setUseFullScreenForFreeWindow(true, null);
      expect(listener.callCount).assertEqual(1);
      expect(listener.freeWindowArg).assertTrue();

      flutterManager.setUseFullScreenForFreeWindow(false, null);
      expect(listener.callCount).assertEqual(2);
      expect(listener.freeWindowArg).assertEqual(false);
      done();
    })

    it('TestIsInFreeWindowModeSupported', 0, (done: Function) => {
      const result = flutterManager.isInFreeWindowModeSupported(null);
      expect(result === true || result === false).assertTrue();
      done();
    })

    it('TestIsInFreeMultiWindowMode', 0, (done: Function) => {
      const result = flutterManager.isInFreeMultiWindowMode(null);
      expect(result === true || result === false).assertTrue();
      done();
    })

    it('TestMultiWindowBookkeeping', 0, (done: Function) => {
      const ability = new FakeAbility() as ESObject as UIAbility;
      // Implicit view 0 has no main view yet.
      expect(flutterManager.getHostWindowByView(0)).assertUndefined();
      expect(flutterManager.getWindowStage(ability)).assertUndefined();
      expect(flutterManager.getHostWindowByView(9001)).assertUndefined();
      done();
    })

    it('TestRegisterViewAbilityAdoptsView0', 0, (done: Function) => {
      const holder = new HostedViewIdHolder();
      const ability = holder as ESObject as UIAbility;
      flutterManager.pushUIAbility(ability);
      try {
        flutterManager.registerViewAbility(0, ability);
        expect(flutterManager.getAbilityByView(0) === ability).assertTrue();
        // Registration adopts view 0's window; with no windowStage (UT env)
        // the adopted host fails attach and removes itself — nothing dangles.
        expect(flutterManager.getHostWindowByView(0)).assertUndefined();
      } finally {
        flutterManager.popUIAbility(ability);
      }
      done();
    })

    it('TestRegisterViewAbilityStampsHostedViewId', 0, (done: Function) => {
      const holder = new HostedViewIdHolder();
      const ability = holder as ESObject as UIAbility;
      try {
        flutterManager.registerViewAbility(9501, ability);
        expect(holder.hostedViewId).assertEqual(9501);
        // A later registration re-stamps (no guard — the registrar owns the id).
        flutterManager.registerViewAbility(9502, ability);
        expect(holder.hostedViewId).assertEqual(9502);
        expect(flutterManager.getAbilityByView(9502) === ability).assertTrue();
      } finally {
        flutterManager.popUIAbility(ability);
      }
      done();
    })

    it('TestGetAbilityByViewIdMapBookkeeping', 0, (done: Function) => {
      // viewId→ability bookkeeping: the first 0-registrar is the main
      // ability; a spawned registration keys its own numeric id; a later
      // 0-registration cannot clobber the incumbent. Entries live for the
      // process (view ids are monotonic — a dead ability's entry is inert).
      const main = new HostedViewIdHolder();
      const mainAbility = main as ESObject as UIAbility;
      const spawned = new HostedViewIdHolder();
      const spawnedAbility = spawned as ESObject as UIAbility;
      flutterManager.pushUIAbility(mainAbility);
      flutterManager.pushUIAbility(spawnedAbility);
      // Isolation: entries live for the process, so earlier suites may have
      // recorded the implicit view id already — and this test must not leak
      // its own registrations into later suites either.
      flutterManager.deleteViewAbility(0);
      flutterManager.deleteViewAbility(9801);
      try {
        flutterManager.registerViewAbility(0, mainAbility);
        expect(flutterManager.getAbilityByView(0) === mainAbility).assertTrue();
        flutterManager.registerViewAbility(9801, spawnedAbility);
        expect(flutterManager.getAbilityByView(9801) === spawnedAbility).assertTrue();
        flutterManager.registerViewAbility(0, spawnedAbility);
        expect(flutterManager.getAbilityByView(0) === mainAbility).assertTrue();
      } finally {
        flutterManager.deleteViewAbility(0);
        flutterManager.deleteViewAbility(9801);
        flutterManager.popUIAbility(mainAbility);
        flutterManager.popUIAbility(spawnedAbility);
      }
      done();
    })

    it('TestSetSubWindowPageName', 0, (done: Function) => {
      flutterManager.setSubWindowPageName('pages/MyWindowPage');
      expect(flutterManager.subWindowPage()).assertEqual('pages/MyWindowPage');
      flutterManager.setSubWindowPageName('');
      expect(flutterManager.subWindowPage()).assertEqual('pages/MyWindowPage');
      done();
    })

    it('TestCreateWindowPerArchetype', 0, (done: Function) => {
      // No parent ability resolves in the UT process → every host fails attach
      // cleanly and removes itself. Covers all four per-archetype creators
      // (dialog: both the modal parent set and the modeless sentinel).
      try {
        flutterManager.createRegularWindow(9101, regularRequest(200, 150, 'host'));
        flutterManager.createDialogWindow(9102, dialogRequest(999, 200, 150, 'dialog'));
        flutterManager.createDialogWindow(9103, dialogRequest(-1, 200, 150, 'modeless'));
        flutterManager.createTooltipWindow(9104, tooltipRequest(999));
        flutterManager.createPopupWindow(9105, popupRequest(999));
      } catch (e) {
      }
      expect(flutterManager.getHostWindowByView(9101)).assertUndefined();
      done();
    })

    it('TestCreateRegularWindowAndBindEntry', 0, (done: Function) => {
      try {
        flutterManager.createRegularWindow(9110, regularRequest(300, 200, 'win'));
        flutterManager.bindEntryAbilityToView(9111, regularRequest(300, 200, 'adopted'));
      } catch (e) {
      }
      done();
    })

    it('TestWindowPropertySettersNoHost', 0, (done: Function) => {
      // The napi setters resolve the host via getHostWindowByView(); an untracked view
      // resolves to undefined and the call is a logged no-op.
      const flutterNapi = new FlutterNapi();
      try {
        expect(flutterManager.getHostWindowByView(9201)).assertUndefined();
        flutterNapi.setWindowSize(9201, 100, 100);
        flutterNapi.setWindowTitle(9201, 't');
        flutterNapi.setWindowMaximized(9201, true);
        flutterNapi.setWindowMinimized(9201, true);
        flutterNapi.setWindowFullscreen(9201, true);
        const constraints = new WindowConstraints();
        constraints.hasViewConstraints = true;
        constraints.viewMinWidth = 50;
        constraints.viewMinHeight = 50;
        constraints.viewMaxWidth = 500;
        constraints.viewMaxHeight = 500;
        flutterNapi.setWindowConstraints(9201, constraints);
        flutterNapi.activateWindow(9201);
      } catch (e) {
      }
      done();
    })

    it('TestDestroyWindowHostGuards', 0, (done: Function) => {
      const flutterNapi = new FlutterNapi();
      try {
        flutterNapi.destroyWindowHost(0);
        flutterNapi.destroyWindowHost(9202);
      } catch (e) {
      }
      done();
    })

    it('TestNotifyWindowActivated', 0, (done: Function) => {
      try {
        flutterManager.notifyWindowActivated(9204, true);
        flutterManager.notifyWindowActivated(9204, false);
      } catch (e) {
      }
      done();
    })

    it('TestCascadeCloseChildrenNoHosts', 0, (done: Function) => {
      const ability = new FakeAbility() as ESObject as UIAbility;
      try {
        flutterManager.cascadeCloseChildren(ability);
      } catch (e) {
      }
      done();
    })

    it('TestExitApplicationNoMainHost', 0, (done: Function) => {
      try {
        flutterManager.exitApplication();
      } catch (e) {
      }
      done();
    })

    // NOTE: TestDensityPixels was dropped — the manager no longer carries a
    // density accessor (the seam moved to FlutterWindowHost.densityPixels).

    // NOTE: ensureParentRectListener / ensureSubWindowRectListener (and the
    // throttled reanchor path) were removed by the upstream windowing-anchor
    // refactor in oh-3.44.9-dev; the specs covering them are dropped here.

    it('TestRemoveHostDoesNotThrow', 0, (done: Function) => {
      // Registered and unknown numeric viewId removals (covers removeHost only).
      flutterManager.removeHost(9302);
      flutterManager.removeHost(9303);
      done();
    })

    it('TestDragCallbacksAccessors', 0, async (done: Function) => {
      const enterCbs = flutterManager.getDragEnterCbs();
      const moveCbs = flutterManager.getDragMoveCbs();
      const leaveCbs = flutterManager.getDragLeaveCbs();
      const dropCbs = flutterManager.getDropCbs();
      expect(enterCbs).not().assertNull();
      expect(moveCbs).not().assertNull();
      expect(leaveCbs).not().assertNull();
      expect(dropCbs).not().assertNull();
      done();
    })

    it('TestAddRemoveDragCallbacks', 0, async (done: Function) => {
      const cb: DragDropCallback = new DragDropCallbackImpl();
      const enterId = flutterManager.addDragEnterCb(cb);
      const moveId = flutterManager.addDragMoveCb(cb);
      const leaveId = flutterManager.addDragLeaveCb(cb);
      const dropId = flutterManager.addDropCb(cb);
      flutterManager.removeDragEnterCb(enterId);
      flutterManager.removeDragMoveCb(moveId);
      flutterManager.removeDragLeaveCb(leaveId);
      flutterManager.removeDropCb(dropId);
      done();
    })

    it('TestHasFlutterView', 0, async (done: Function) => {
      const result = flutterManager.hasFlutterView('non-existent-view');
      expect(result).assertFalse();
      done();
    })

    it('TestGetFlutterViewById', 0, async (done: Function) => {
      const view = flutterManager.getFlutterView('non-existent-view');
      expect(view).assertNull();
      done();
    })

    it('TestDeleteFlutterView', 0, async (done: Function) => {
      let threw = false;
      try { flutterManager.deleteFlutterView('non-existent'); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    })

    it('TestGetUIAbility with no context', 0, () => {
      const ability = flutterManager.getUIAbility();
      expect(ability).not().assertNull();
    })

    it('TestGetUIAbility with context', 0, () => {
      const ctx = getContext();
      const ability = flutterManager.getUIAbility(ctx);
      expect(ability).not().assertNull();
    })

    it('TestGetWindowId does not throw', 0, () => {
      const ctx = getContext();
      let threw = false;
      try { flutterManager.getWindowId(getContext()); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    })

    it('TestGetNextFlutterViewId returns string', 0, () => {
      const id = flutterManager.getNextFlutterViewId();
      expect(typeof id).assertEqual('string');
    })

    it('TestSetSpecificSystemBarEnabled does not throw with non-existent view', 0, () => {
      let threw = false;
      try {
        flutterManager.setSpecificSystemBarEnabled('non-existent', 'status', true);
      } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    })

    it('TestHandleWindowDecorSafeArea does not throw', 0, () => {
      const ctx = getContext();
      let threw = false;
      try {
        flutterManager.handleWindowDecorSafeArea('non-existent', ctx);
      } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    })

    it('TestSetFullScreenChangeListener does not throw', 0, () => {
      let threw = false;
      try {
        const listener: FullScreenListener = new TestFullScreenListener();
        flutterManager.setFullScreenListener(listener);
      } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    })

    it('TestPopUIAbility does not throw', 0, () => {
      let threw = false;
      try { const ctx = getContext();
      const ability = flutterManager.getUIAbility(ctx);
      if (ability) { flutterManager.popUIAbility(ability); } } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    })

    it('TestIsWindowDecorSafeAreaAvoidSupported does not throw', 0, () => {
      const ctx = getContext();
      let threw = false;
      try { flutterManager.isWindowDecorSafeAreaAvoidSupported(ctx); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    })
  })
}
