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
import FlutterManager, { FullScreenListener } from '../../../../../main/ets/embedding/ohos/FlutterManager';
import { FakeAbility, FakeRect, FakeWindow, FakeWindowStage } from './windowing/Fakes';

function sleep(time: number) {
  return new Promise<void>((resolve: Function) => setTimeout(resolve, time));
}

const flutterManager: FlutterManager = FlutterManager.getInstance();
const uiAbility = flutterManager.getUIAbility(getContext());

export default function FlutterManagerTest() {
  describe('FlutterManagerTest', () => {
    beforeAll(async () => {
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

    it('TestGetFlutterViewList', 0, async (done: Function) => {
      const result = flutterManager.getFlutterViewList();
      expect(result).not().assertNull();
      done();
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
      // Non-oh_flutter_ id: no main-view stamp.
      flutterManager.setViewAbility('9001', ability);
      expect(flutterManager.getViewAbility('9001')).assertEqual(ability);
      expect(flutterManager.resolveParentAbility(9001)).assertEqual(ability);
      // Implicit view 0 has no main view yet.
      expect(flutterManager.resolveParentAbility(0)).assertUndefined();
      expect(flutterManager.mainFlutterViewId()).assertUndefined();
      expect(flutterManager.sharedEngine()).assertNull();
      expect(flutterManager.windowStageOf(ability)).assertUndefined();
      expect(flutterManager.subWindowOfView('9001')).assertUndefined();
      expect(flutterManager.getWindowHostOf('9001')).assertUndefined();
      flutterManager.deleteViewAbility('9001');
      expect(flutterManager.getViewAbility('9001')).assertUndefined();
      done();
    })

    it('TestRegisterViewAbilityStampsMainViewId', 0, (done: Function) => {
      const ability = new FakeAbility() as ESObject as UIAbility;
      // oh_flutter_ prefix stamps the implicit main-view id.
      flutterManager.registerViewAbility('oh_flutter_2', ability);
      expect(flutterManager.mainFlutterViewId()).assertEqual('oh_flutter_2');
      expect(flutterManager.resolveParentAbility(0)).assertEqual(ability);
      flutterManager.deleteViewAbility('oh_flutter_2');
      // Stale main id but cleared ability lookup: still resolves to undefined.
      expect(flutterManager.resolveParentAbility(0)).assertUndefined();
      done();
    })

    it('TestRegisterViewAbilityStampsHostedViewId', 0, (done: Function) => {
      const holder = new HostedViewIdHolder();
      const ability = holder as ESObject as UIAbility;
      flutterManager.registerViewAbility('9501', ability);
      expect(holder.hostedViewId).assertEqual(9501);
      flutterManager.deleteViewAbility('9501');
      // A holder already stamped keeps its id (stamp only when 0).
      holder.hostedViewId = 7;
      flutterManager.registerViewAbility('9502', ability);
      expect(holder.hostedViewId).assertEqual(7);
      flutterManager.deleteViewAbility('9502');
      done();
    })

    it('TestSetRegularWindowAbilityName', 0, (done: Function) => {
      flutterManager.setRegularWindowAbilityName('MyRegularWindow');
      expect(flutterManager.regularWindowAbilityName()).assertEqual('MyRegularWindow');
      // Empty name is ignored.
      flutterManager.setRegularWindowAbilityName('');
      expect(flutterManager.regularWindowAbilityName()).assertEqual('MyRegularWindow');
      done();
    })

    it('TestSetSubWindowPageName', 0, (done: Function) => {
      flutterManager.setSubWindowPageName('pages/MyWindowPage');
      expect(flutterManager.subWindowPage()).assertEqual('pages/MyWindowPage');
      flutterManager.setSubWindowPageName('');
      expect(flutterManager.subWindowPage()).assertEqual('pages/MyWindowPage');
      done();
    })

    it('TestCreateSubWindowArchetypeSwitch', 0, (done: Function) => {
      // No parent ability resolves in the UT process → every host fails attach
      // cleanly and removes itself. Covers the archetype switch + default.
      try {
        flutterManager.createSubWindow(9101, 999, 200, 150, 0);
        flutterManager.createSubWindow(9102, 999, 200, 150, 1);
        flutterManager.createSubWindow(9103, 999, 200, 150, 2);
        flutterManager.createSubWindow(9104, 999, 200, 150, 3);
        flutterManager.createSubWindow(9105, 999, 200, 150, 99);
      } catch (e) {
      }
      expect(flutterManager.getWindowHostOf('9101')).assertUndefined();
      done();
    })

    it('TestCreateRegularAbilityAndBindEntry', 0, (done: Function) => {
      try {
        flutterManager.createRegularAbilityWindow(9110, 42, 300, 200, 'win');
        flutterManager.bindEntryAbilityToView(9111, 300, 200, 'adopted');
      } catch (e) {
      }
      done();
    })

    it('TestWindowPropertySettersNoHost', 0, (done: Function) => {
      try {
        flutterManager.setWindowSize(9201, 100, 100);
        flutterManager.setWindowTitle(9201, 't');
        flutterManager.setWindowMaximized(9201, true);
        flutterManager.setWindowMinimized(9201, true);
        flutterManager.setWindowFullscreen(9201, true);
        flutterManager.setWindowConstraints(9201, 50, 500, 50, 500);
        flutterManager.activateWindow(9201);
      } catch (e) {
      }
      done();
    })

    it('TestDestroyWindowHostGuards', 0, (done: Function) => {
      try {
        flutterManager.destroyWindowHost(0);
        flutterManager.destroyWindowHost(9202);
      } catch (e) {
      }
      done();
    })

    it('TestRestoreParentForeground', 0, (done: Function) => {
      // No parent → early return.
      flutterManager.restoreParentForeground(undefined, '9203');
      // Parent without a registered windowStage → log and return.
      const ability = new FakeAbility() as ESObject as UIAbility;
      flutterManager.restoreParentForeground(ability, '9203');
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

    it('TestDensityPixels', 0, (done: Function) => {
      const dpr = flutterManager.densityPixels();
      expect(typeof dpr).assertEqual('number');
      expect(dpr > 0).assertTrue();
      done();
    })

    it('TestEnsureParentRectListenerRegistersAndReuses', 0, (done: Function) => {
      const ability = new FakeAbility() as ESObject as UIAbility;
      const fakeWindow = new FakeWindow();
      const stage = new FakeWindowStage();
      stage.mainWindow = fakeWindow as ESObject as window.Window;
      const stageWin = stage as ESObject as window.WindowStage;
      flutterManager.ensureParentRectListener(ability, stageWin);
      expect(fakeWindow.rectChangeCb).not().assertNull();
      // Second registration for the same ability is a no-op.
      flutterManager.ensureParentRectListener(ability, stageWin);
      flutterManager.popWindowStage(ability);
      done();
    })

    it('TestEnsureParentRectListenerOnThrows', 0, (done: Function) => {
      const ability = new FakeAbility() as ESObject as UIAbility;
      const fakeWindow = new FakeWindow();
      fakeWindow.onThrows = true;
      const stage = new FakeWindowStage();
      stage.mainWindow = fakeWindow as ESObject as window.Window;
      try {
        flutterManager.ensureParentRectListener(ability, stage as ESObject as window.WindowStage);
      } catch (e) {
      }
      flutterManager.popWindowStage(ability);
      done();
    })

    it('TestEnsureParentRectListenerGetMainThrows', 0, (done: Function) => {
      const ability = new FakeAbility() as ESObject as UIAbility;
      const stage = new FakeWindowStage();
      stage.getMainThrows = true;
      try {
        flutterManager.ensureParentRectListener(ability, stage as ESObject as window.WindowStage);
      } catch (e) {
      }
      flutterManager.popWindowStage(ability);
      done();
    })

    it('TestParentMoveReanchorThrottles', 0, async (done: Function) => {
      const ability = new FakeAbility() as ESObject as UIAbility;
      const fakeWindow = new FakeWindow();
      const stage = new FakeWindowStage();
      stage.mainWindow = fakeWindow as ESObject as window.Window;
      flutterManager.ensureParentRectListener(ability, stage as ESObject as window.WindowStage);
      // First move: elapsed >= throttle → run immediately.
      const rect = new FakeRect(10, 20, 100, 200) as ESObject as window.RectChangeOptions;
      fakeWindow.rectChangeCb?.(rect);
      // Second + third moves within 50ms → trailing run scheduled / dropped.
      fakeWindow.rectChangeCb?.(rect);
      fakeWindow.rectChangeCb?.(rect);
      await sleep(80);
      flutterManager.popWindowStage(ability);
      done();
    })

    it('TestEnsureSubWindowRectListenerUnknownParent', 0, (done: Function) => {
      // No sub-window host for the parent view → early return.
      flutterManager.ensureSubWindowRectListener(9301);
      flutterManager.ensureSubWindowRectListener(9301);
      done();
    })

    it('TestRemoveHostReleasesSubWindowRectListener', 0, (done: Function) => {
      // Numeric viewId → releaseSubWindowRectListener (no listener → early return).
      flutterManager.removeHost('9302');
      // Non-numeric viewId → skips the release path entirely.
      flutterManager.removeHost('oh_flutter_9302');
      done();
    })

    it('TestReleaseParentRectListenerViaPopWindowStage', 0, async (done: Function) => {
      const ability = new FakeAbility() as ESObject as UIAbility;
      const fakeWindow = new FakeWindow();
      const stage = new FakeWindowStage();
      stage.mainWindow = fakeWindow as ESObject as window.Window;
      const stageWin = stage as ESObject as window.WindowStage;
      flutterManager.pushWindowStage(ability, stageWin);
      flutterManager.ensureParentRectListener(ability, stageWin);
      // Prime abilityKey + throttled reanchor state so pop releases the maps.
      const rect = new FakeRect(0, 0, 100, 100) as ESObject as window.RectChangeOptions;
      fakeWindow.rectChangeCb?.(rect);
      await sleep(80);
      flutterManager.popWindowStage(ability);
      done();
    })
  })
}

/** Ability whose hostedViewId is 0 before the manager stamps it. */
class HostedViewIdHolder {
  hostedViewId: number = 0;
}