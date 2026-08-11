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
import FlutterManager, { FullScreenListener } from '../../../../../main/ets/embedding/ohos/FlutterManager';

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
  })
}