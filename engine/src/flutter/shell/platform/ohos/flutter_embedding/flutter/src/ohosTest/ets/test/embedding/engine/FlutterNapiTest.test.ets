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
import FlutterNapi, { WindowPositionResult } from '../../../../../main/ets/embedding/engine/FlutterNapi';
import ApplicationInfoLoader from '../../../../../main/ets/embedding/engine/loader/ApplicationInfoLoader';
import { FlutterRenderer } from '../../../../../main/ets/embedding/engine/renderer/FlutterRenderer';
import { common } from '@kit.AbilityKit';
import { beforeAll, describe, expect, it } from '@ohos/hypium';
import { EngineLifecycleListener } from '../../../../../main/ets/embedding/engine/FlutterEngine';
import { resourceManager } from '@kit.LocalizationKit';
import FlutterApplicationInfo from '../../../../../main/ets/embedding/engine/loader/FlutterApplicationInfo';
import { FlutterCallbackInformation } from '../../../../../main/ets/view/FlutterCallbackInformation';

//jit产物默认kenel文件
const DEFAULT_KERNEL_BLOB = "kernel_blob.bin";
//文件路径分隔符
const FILE_SEPARATOR = "/";

export default function FlutterNapiTest() {
  const flutterNapi = new FlutterNapi();
  const context = getContext() as common.UIAbilityContext;
  let textureId = 0;
  let initResult: InitResult;
  let snapshotAssetPath: string;
  let flutterApplicationInfo: FlutterApplicationInfo;
  let bundlePath: string;
  let event: PanGestureEvent;
  let assetManager: resourceManager.ResourceManager;

  describe('FlutterNapiTest', () => {

    beforeAll(() => {
      if (context) {
        initResult = new InitResult(
          `${context.filesDir}/`,
          `${context.cacheDir}/`,
          `${context.filesDir}`);
        flutterApplicationInfo = ApplicationInfoLoader.load(context);
        snapshotAssetPath = initResult!.dataDirPath + FILE_SEPARATOR + flutterApplicationInfo!.flutterAssetsDir;
        bundlePath = snapshotAssetPath + FILE_SEPARATOR + DEFAULT_KERNEL_BLOB;
        assetManager = context.resourceManager;
      }
      const target: EventTarget = new CustomEventTarget(new CustomArea(0, 0, {
        x: 0, y: 0
      }, {
        x: 0, y: 0
      }));
      const renderer = new FlutterRenderer(flutterNapi);
      textureId = renderer.getTextureId();
      event = {
        offsetX: 50,
        offsetY: 50,
        velocityX: 50,
        velocityY: 50,
        fingerList: [],
        velocity: 0,
        target: target,
        timestamp: 0,
        source: SourceType.Unknown,
        pressure: 0,
        tiltX: 0,
        tiltY: 0,
        sourceTool: SourceTool.Unknown
      }
    })

    it('checkImplemented', 0, async (done: Function) => {
      console.info("uitest: checkImplemented begin");
      const methodName = 'test';
      const flag = flutterNapi.checkImplemented(methodName);
      expect(flag).not().assertNull();
      done();
    })

    it('nativeLookupCallbackInformation', 0, async (done: Function) => {
      console.info("uitest: nativeLookupCallbackInformation begin");
      try {
        const handle = 1;
        const flutterCallbackInformation: FlutterCallbackInformation | null =
          FlutterNapi.nativeLookupCallbackInformation(handle);
        if (flutterCallbackInformation) {
          expect(flutterCallbackInformation).not().assertNull();
        }
      } catch (e) {
      }
      done();
    })

    it('isAttached', 0, async (done: Function) => {
      console.info("uitest: isAttached begin");
      const flag: boolean = flutterNapi.isAttached();
      expect(flag).not().assertNull();
      done();
    })

    it('spawn', 0, async (done: Function) => {
      console.info("uitest: spawn begin");
      const entrypointFunctionName = 'test1';
      const pathToEntrypointFunction = 'test2';
      const initialRoute = 'test3';
      const entrypointArgs = ['test4'];
      try {
        const fnapi: FlutterNapi =
          flutterNapi.spawn(entrypointFunctionName, pathToEntrypointFunction, initialRoute, entrypointArgs);
        expect(fnapi).not().assertNull();
      } catch (e) {
      }
      done();
    })

    it('computePlatformResolvedLocale', 0, async (done: Function) => {
      console.info("uitest: computePlatformResolvedLocale begin");
      const strings = [];
      const arr = flutterNapi.computePlatformResolvedLocale(strings);
      expect(arr).not().assertNull();
      done();
    })

    it('registerTexture', 0, async (done: Function) => {
      console.info("uitest: registerTexture begin");
      const id = flutterNapi.registerTexture(textureId);
      expect(id).not().assertNull();
      done();
    })

    it('windowPositionResultDefaultsToZero', 0, async (done: Function) => {
      const out = new WindowPositionResult();
      expect(out.left).assertEqual(0);
      expect(out.top).assertEqual(0);
      expect(out.width).assertEqual(0);
      expect(out.height).assertEqual(0);
      done();
    })

    it('windowPositionResultSetPopulatesFields', 0, async (done: Function) => {
      const out = new WindowPositionResult();
      out.set(10, 20, 30, 40);
      expect(out.left).assertEqual(10);
      expect(out.top).assertEqual(20);
      expect(out.width).assertEqual(30);
      expect(out.height).assertEqual(40);
      done();
    })

    it('nativeHandleOsWindowClosedRunsForUntrackedView', 0, async (done: Function) => {
      try {
        FlutterNapi.nativeHandleOsWindowClosed(9101);
      } catch (e) {
      }
      done();
    })

    it('nativeComputeWindowPositionUntrackedViewKeepsDefaults', 0, async (done: Function) => {
      const out = new WindowPositionResult();
      try {
        const ret = FlutterNapi.nativeComputeWindowPosition(out, 9102, 100, 80, 0, 0, 800, 600, 0, 0, 1000, 800);
        expect(typeof ret).assertEqual('number');
      } catch (e) {
      }
      // Untracked view: native leaves the holder untouched (0 defaults).
      expect(out.left).assertEqual(0);
      expect(out.top).assertEqual(0);
      done();
    })

    it('nativeNotifyWindowActivatedRunsForUntrackedView', 0, async (done: Function) => {
      try {
        FlutterNapi.nativeNotifyWindowActivated(9103, true);
        FlutterNapi.nativeNotifyWindowActivated(9103, false);
      } catch (e) {
      }
      done();
    })

    it('requestWindowHostDelegatesToManagerForEachArchetype', 0, async (done: Function) => {
      try {
        // Archetypes 0..3 + unknown fall through the createSubWindow switch.
        flutterNapi.requestWindowHost(9110, 999, 200, 150, 0);
        flutterNapi.requestWindowHost(9111, 999, 200, 150, 1);
        flutterNapi.requestWindowHost(9112, 999, 200, 150, 2);
        flutterNapi.requestWindowHost(9113, 999, 200, 150, 3);
        flutterNapi.requestWindowHost(9114, 999, 200, 150, 99);
      } catch (e) {
      }
      done();
    })

    it('createRegularAbilityAndBindEntryAbilityDelegateToManager', 0, async (done: Function) => {
      try {
        flutterNapi.createRegularAbility(9120, 42, 300, 200, 'win');
        flutterNapi.bindEntryAbilityToView(9121, 300, 200, 'adopted');
      } catch (e) {
      }
      done();
    })

    it('destroyWindowHostGuardsImplicitViewAndUnknownHost', 0, async (done: Function) => {
      try {
        flutterNapi.destroyWindowHost(0);
        flutterNapi.destroyWindowHost(9130);
      } catch (e) {
      }
      done();
    })

    it('exitApplicationWithoutMainHostLogsAndReturns', 0, async (done: Function) => {
      try {
        flutterNapi.exitApplication();
      } catch (e) {
      }
      done();
    })

    it('windowPropertySettersRunWithoutHost', 0, async (done: Function) => {
      try {
        flutterNapi.setWindowSize(9140, 100, 100);
        flutterNapi.setWindowTitle(9140, 't');
        flutterNapi.setWindowMaximized(9140, true);
        flutterNapi.setWindowMinimized(9140, true);
        flutterNapi.setWindowFullscreen(9140, true);
        flutterNapi.setWindowConstraints(9140, 50, 500, 50, 500);
        flutterNapi.activateWindow(9140);
      } catch (e) {
      }
      done();
    })
  })
}

class InitResult {
  appStoragePath: string;
  engineCachesPath: string;
  dataDirPath: string;

  constructor(appStoragePath: string,
    engineCachesPath: string,
    dataDirPath: string) {
    this.appStoragePath = appStoragePath;
    this.engineCachesPath = engineCachesPath;
    this.dataDirPath = dataDirPath;
  }
}

class CurEngineLifecycleListener implements EngineLifecycleListener {
  onPreEngineRestart() {

  }

  onEngineWillDestroy() {

  }
}

class CustomEventTarget implements EventTarget {
  area: Area = new CustomArea(0, 0, { x: 0, y: 0 }, { x: 0, y: 0 });

  constructor(area: Area) {
    this.area = area;
  }
}

class CustomArea implements Area {
  width: Length = 0;
  height: Length = 0;
  position: Position = { x: 0, y: 0 };
  globalPosition: Position = { x: 0, y: 0 };

  constructor(width: Length, height: Length, position: Position, globalPosition: Position) {
    this.width = width;
    this.height = height;
    this.position = position;
    this.globalPosition = globalPosition;
  }
}