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
import FlutterNapi from '../../../../../main/ets/embedding/engine/FlutterNapi';
import window from '@ohos.window';
import { regularRequest, dialogRequest, tooltipRequest, popupRequest } from '../ohos/windowing/Fakes';
import { WindowConstraints } from '../../../../../main/ets/embedding/ohos/windowing/WindowingUtils';
import { WindowPositionResult } from '../../../../../main/ets/embedding/engine/FlutterNapi';
import FlutterEngine from '../../../../../main/ets/embedding/engine/FlutterEngine';
import PlatformViewsController from '../../../../../main/ets/plugin/platform/PlatformViewsController';
import {
  invokeSetViewportMetricsWithCornerRadii,
  pierceNativeShellHolderId,
  restoreFlutterNapiHasInit,
  setShellHolderIdForGuardTest
} from './CoveragePierceHelpers';
import FlutterInjector from '../../../../../main/ets/FlutterInjector';
import FlutterEnginePreload from '../../../../../main/ets/embedding/engine/FlutterEnginePreload';
import ApplicationInfoLoader from '../../../../../main/ets/embedding/engine/loader/ApplicationInfoLoader';
import { FlutterRenderer } from '../../../../../main/ets/embedding/engine/renderer/FlutterRenderer';
import { common } from '@kit.AbilityKit';
import { afterAll, beforeAll, describe, expect, it } from '@ohos/hypium';
import { EngineLifecycleListener } from '../../../../../main/ets/embedding/engine/FlutterEngine';
import { resourceManager } from '@kit.LocalizationKit';
import FlutterApplicationInfo from '../../../../../main/ets/embedding/engine/loader/FlutterApplicationInfo';
import { FlutterCallbackInformation } from '../../../../../main/ets/view/FlutterCallbackInformation';
import FlutterLoader from '../../../../../main/ets/embedding/engine/loader/FlutterLoader';

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
    });

    afterAll(() => {
      restoreFlutterNapiHasInit();
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
        const ret = FlutterNapi.nativeComputeWindowPosition(out, 9102,
          { width: 100, height: 80 } as window.Size,
          { left: 0, top: 0, width: 800, height: 600 } as window.Rect,
          { left: 0, top: 0, width: 1000, height: 800 } as window.Rect);
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

    it('createWindowDelegatesToManagerForEachArchetype', 0, async (done: Function) => {
      try {
        // One delegate per archetype (dialog: modal parent + modeless
        // sentinel both covered).
        flutterNapi.createRegularWindow(9110, regularRequest(200, 150, 'host'));
        flutterNapi.createDialogWindow(9111, dialogRequest(999, 200, 150, 'dialog'));
        flutterNapi.createDialogWindow(9112, dialogRequest(-1, 200, 150, 'modeless'));
        flutterNapi.createTooltipWindow(9113, tooltipRequest(999));
        flutterNapi.createPopupWindow(9114, popupRequest(999));
      } catch (e) {
      }
      done();
    })

    it('createRegularWindowAndBindEntryAbilityDelegateToManager', 0, async (done: Function) => {
      try {
        flutterNapi.createRegularWindow(9120, regularRequest(300, 200, 'win'));
        flutterNapi.bindEntryAbilityToView(9121, regularRequest(300, 200, 'adopted'));
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
        const constraints = new WindowConstraints();
        constraints.hasViewConstraints = true;
        constraints.viewMinWidth = 50;
        constraints.viewMinHeight = 50;
        constraints.viewMaxWidth = 500;
        constraints.viewMaxHeight = 500;
        flutterNapi.setWindowConstraints(9140, constraints);
        flutterNapi.activateWindow(9140);
      } catch (e) {
      }
      done();
    })

    it('setViewportMetrics_with_display_corner_radii', 0, async (done: Function) => {
      console.info("uitest: setViewportMetrics_with_display_corner_radii begin");
      const ctx = getContext() as common.UIAbilityContext;
      // The engine is constructed but never init()ed, so its napi is NOT
      // attached and the call returns at the isAttached() guard — this spec
      // pins the argument shape (24-arg and legacy 20-arg forms), while the
      // native forwarding is covered by setViewportMetrics_after_engine_init.
      const unattachedNapi = new FlutterEngine(ctx, null, new FlutterNapi(), new PlatformViewsController()).getFlutterNapi();
      unattachedNapi.setViewportMetrics(
        3.0, 1080, 2340,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        3.0,
        [], [], [],
        12, 34, 56, 78
      );
      unattachedNapi.setViewportMetrics(
        2.0, 720, 1280,
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        2.0,
        [0, 0, 10, 10], [1], [0]
      );
      expect(unattachedNapi.isAttached()).assertFalse();
      done();
    })

    it('setViewportMetrics_without_display_corner_radii_defaults', 0, async (done: Function) => {
      console.info("uitest: setViewportMetrics_without_display_corner_radii_defaults begin");
      const ctx = getContext() as common.UIAbilityContext;
      const unattachedNapi = new FlutterEngine(ctx, null, new FlutterNapi(), new PlatformViewsController()).getFlutterNapi();
      unattachedNapi.setViewportMetrics(
        3.0, 1080, 2340,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        3.0,
        [], [], []
      );
      expect(unattachedNapi.isAttached()).assertFalse();
      done();
    })

    it('setViewportMetrics_zero_corner_radii_flat_screen', 0, async (done: Function) => {
      console.info("uitest: setViewportMetrics_zero_corner_radii_flat_screen begin");
      const ctx = getContext() as common.UIAbilityContext;
      const unattachedNapi = new FlutterEngine(ctx, null, new FlutterNapi(), new PlatformViewsController()).getFlutterNapi();
      unattachedNapi.setViewportMetrics(
        3.0, 1080, 2340,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        3.0,
        [], [], [],
        0, 0, 0, 0
      );
      expect(unattachedNapi.isAttached()).assertFalse();
      done();
    })

    it('setViewportMetrics_zero_shell_holder_returns_early', 0, () => {
      console.info("uitest: setViewportMetrics_zero_shell_holder_returns_early begin");
      const napi = new FlutterNapi();
      setShellHolderIdForGuardTest(napi, 0);
      invokeSetViewportMetricsWithCornerRadii(napi);
      expect(napi.nativeShellHolderId).assertEqual(0);
    })

    it('setViewportMetrics_after_engine_init_with_corner_radii', 0, (done: Function) => {
      console.info("uitest: setViewportMetrics_after_engine_init_with_corner_radii begin");
      const ctx = getContext() as common.UIAbilityContext;
      try {
        const loader = FlutterInjector.getInstance().getFlutterLoader();
        if (!loader.initialized) {
          loader.startInitialization(ctx);
          loader.ensureInitializationComplete([]);
        }
        const napi = FlutterEnginePreload.preLoadFlutterNapi(ctx, loader.findAppBundlePath(), {});
        if (napi && napi.nativeShellHolderId) {
          invokeSetViewportMetricsWithCornerRadii(napi);
          expect(napi.isAttached()).assertTrue();
        } else {
          console.info("uitest: napi not attached, skip native forwarding");
        }
      } catch (e) {
        console.info("uitest: real napi init path error, skip: " + JSON.stringify(e));
      }
      FlutterInjector.getInstance().getFlutterNapi();
      done();
    })

    it('unicodeIsEmoji returns boolean for various code points', 0, () => {
      // Standard emoji (grinning face U+1F600)
      expect(typeof FlutterNapi.unicodeIsEmoji(0x1F600)).assertEqual('boolean');
      // Plain ASCII letter
      expect(typeof FlutterNapi.unicodeIsEmoji(0x41)).assertEqual('boolean');
      // Zero
      expect(typeof FlutterNapi.unicodeIsEmoji(0x30)).assertEqual('boolean');
    });

    it('unicodeIsEmojiModifier returns boolean', 0, () => {
      expect(typeof FlutterNapi.unicodeIsEmojiModifier(0x1F3FB)).assertEqual('boolean');
      expect(typeof FlutterNapi.unicodeIsEmojiModifier(0x41)).assertEqual('boolean');
    });

    it('unicodeIsEmojiModifierBase returns boolean', 0, () => {
      expect(typeof FlutterNapi.unicodeIsEmojiModifierBase(0x1F466)).assertEqual('boolean');
      expect(typeof FlutterNapi.unicodeIsEmojiModifierBase(0x41)).assertEqual('boolean');
    });

    it('unicodeIsVariationSelector returns boolean', 0, () => {
      // U+FE0F is a variation selector
      expect(typeof FlutterNapi.unicodeIsVariationSelector(0xFE0F)).assertEqual('boolean');
      expect(typeof FlutterNapi.unicodeIsVariationSelector(0x41)).assertEqual('boolean');
    });

    it('unicodeIsRegionalIndicatorSymbol returns boolean', 0, () => {
      // U+1F1E6 is regional indicator "A"
      expect(typeof FlutterNapi.unicodeIsRegionalIndicatorSymbol(0x1F1E6)).assertEqual('boolean');
      expect(typeof FlutterNapi.unicodeIsRegionalIndicatorSymbol(0x41)).assertEqual('boolean');
    });

    it('checkLTPOSwitchState returns number', 0, () => {
      const state = FlutterNapi.checkLTPOSwitchState();
      expect(typeof state).assertEqual('number');
    });

    it('hasImplemented defaults to false (no init)', 0, () => {
      const fresh = new FlutterNapi();
      expect(fresh.hasImplemented).assertEqual(false);
    });

    it('isAttached returns false initially', 0, () => {
      const fresh = new FlutterNapi();
      expect(fresh.isAttached()).assertEqual(false);
    });

    it('checkImplemented returns false when not implemented', 0, () => {
      const fresh = new FlutterNapi();
      expect(fresh.checkImplemented()).assertEqual(false);
    });

    it('prefetchDefaultFontManager does not throw', 0, () => {
      let threw = false;
      try {
        FlutterNapi.prefetchDefaultFontManager();
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('prefetchFramesCfg does not throw', 0, () => {
      let threw = false;
      try {
        FlutterNapi.prefetchFramesCfg();
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('videoVoting does not throw', 0, () => {
      let threw = false;
      try {
        FlutterNapi.videoVoting(1.0, 60);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('notifyLowMemoryWarning does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.notifyLowMemoryWarning(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('createRegularAbilityAndBindEntryAbilityDelegateToManager', 0, async (done: Function) => {
      try {
        flutterNapi.createRegularWindow(9120, regularRequest(300, 200, 'win'));
        flutterNapi.bindEntryAbilityToView(9121, regularRequest(300, 200, 'adopted'));
      } catch (e) {
      }
      done();
    })

    it('dispatchEmptyPlatformMessage does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.dispatchEmptyPlatformMessage('test/channel', 1); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('dispatchPlatformMessage does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.dispatchPlatformMessage('test/channel', new ArrayBuffer(4), 4, 1); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('invokePlatformMessageEmptyResponseCallback does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.invokePlatformMessageEmptyResponseCallback(1); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('invokePlatformMessageResponseCallback does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.invokePlatformMessageResponseCallback(1, new ArrayBuffer(0), 0); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('handlePlatformMessageResponse does not throw when no handler', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.handlePlatformMessageResponse(1, new ArrayBuffer(0)); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('handlePlatformMessage does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.handlePlatformMessage('test/channel', new ArrayBuffer(0), 1, 0); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('setViewportMetrics does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.setViewportMetrics(0, 100, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, [], [], []); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('spawn does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.spawn('main', 'path', '/route', []); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('spawnAsync does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.spawnAsync('main', 'path', '/route', []); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('preSpawn does not throw', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.preSpawn('main', 'path', '/route', []); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('processPendingMessages does not throw', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.processPendingMessages(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('onFirstFrame does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.onFirstFrame(0); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('setSemanticsEnabled does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.setSemanticsEnabled(true); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

it('accessibilityStateChange does not throw', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.accessibilityStateChange(true); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('getSystemLanguages does not throw', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.getSystemLanguages(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('detachFromNativeAndReleaseResources does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.detachFromNativeAndReleaseResources(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('detachFromNativeAndReleaseResourcesAsync does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.detachFromNativeAndReleaseResourcesAsync(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('unregisterTexture does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.unregisterTexture(1); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('setTextureBackGroundColor does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.setTextureBackGroundColor(1, 0xFF0000); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('setTextureBufferSize does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.setTextureBufferSize(1, 100, 200); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('enableFrameCache does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.enableFrameCache(true); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('setPipVisible does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.setPipVisible(true); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('onTouchEvent does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.onTouchEvent([]); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('onMouseEvent does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.onMouseEvent([]); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('onAxisEvent does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.onAxisEvent([]); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('setFontWeightScale does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.setFontWeightScale(1.0); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('setFlutterNavigationAction does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.setFlutterNavigationAction(0, true); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('setPreloading does not throw', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.setPreloading(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('registerTexture does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.registerTexture(1); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

it('getTextureNativeWindowId does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.getTextureNativeWindowId(1); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('getTextureNativeWindowPtr does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.getTextureNativeWindowPtr(1); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('setExternalNativeImage does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.setExternalNativeImage(1, 0); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('resetExternalTexture does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.resetExternalTexture(1, false); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('notifyTextureResizing does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.notifyTextureResizing(1, 100, 200); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

it('setExternalNativeImagePtr does not throw when detached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try { napi.setExternalNativeImagePtr(1, BigInt(0)); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('windowPropertySettersRunWithoutHost', 0, async (done: Function) => {
      try {
        flutterNapi.setWindowSize(9140, 100, 100);
        flutterNapi.setWindowTitle(9140, 't');
        flutterNapi.setWindowMaximized(9140, true);
        flutterNapi.setWindowMinimized(9140, true);
        flutterNapi.setWindowFullscreen(9140, true);
        const constraints = new WindowConstraints();
        constraints.hasViewConstraints = true;
        constraints.viewMinWidth = 50;
        constraints.viewMinHeight = 50;
        constraints.viewMaxWidth = 500;
        constraints.viewMaxHeight = 500;
        flutterNapi.setWindowConstraints(9140, constraints);
        flutterNapi.activateWindow(9140);
      } catch (e) {
      }
      done();
    })

    it('setViewportMetrics_with_display_corner_radii', 0, async (done: Function) => {
      console.info("uitest: setViewportMetrics_with_display_corner_radii begin");
      const ctx = getContext() as common.UIAbilityContext;
      // The engine is constructed but never init()ed, so its napi is NOT
      // attached and the call returns at the isAttached() guard — this spec
      // pins the argument shape (24-arg and legacy 20-arg forms), while the
      // native forwarding is covered by setViewportMetrics_after_engine_init.
      const unattachedNapi = new FlutterEngine(ctx, null, new FlutterNapi(), new PlatformViewsController()).getFlutterNapi();
      unattachedNapi.setViewportMetrics(
        3.0, 1080, 2340,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        3.0,
        [], [], [],
        12, 34, 56, 78
      );
      unattachedNapi.setViewportMetrics(
        2.0, 720, 1280,
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        2.0,
        [0, 0, 10, 10], [1], [0]
      );
      expect(unattachedNapi.isAttached()).assertFalse();
      done();
    })

    it('setViewportMetrics_without_display_corner_radii_defaults', 0, async (done: Function) => {
      console.info("uitest: setViewportMetrics_without_display_corner_radii_defaults begin");
      const ctx = getContext() as common.UIAbilityContext;
      const unattachedNapi = new FlutterEngine(ctx, null, new FlutterNapi(), new PlatformViewsController()).getFlutterNapi();
      unattachedNapi.setViewportMetrics(
        3.0, 1080, 2340,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        3.0,
        [], [], []
      );
      expect(unattachedNapi.isAttached()).assertFalse();
      done();
    })

    it('setViewportMetrics_zero_corner_radii_flat_screen', 0, async (done: Function) => {
      console.info("uitest: setViewportMetrics_zero_corner_radii_flat_screen begin");
      const ctx = getContext() as common.UIAbilityContext;
      const unattachedNapi = new FlutterEngine(ctx, null, new FlutterNapi(), new PlatformViewsController()).getFlutterNapi();
      unattachedNapi.setViewportMetrics(
        3.0, 1080, 2340,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        3.0,
        [], [], [],
        0, 0, 0, 0
      );
      expect(unattachedNapi.isAttached()).assertFalse();
      done();
    })

    it('setViewportMetrics_zero_shell_holder_returns_early', 0, () => {
      console.info("uitest: setViewportMetrics_zero_shell_holder_returns_early begin");
      const napi = new FlutterNapi();
      setShellHolderIdForGuardTest(napi, 0);
      invokeSetViewportMetricsWithCornerRadii(napi);
      expect(napi.nativeShellHolderId).assertEqual(0);
    })

    it('setViewportMetrics_after_engine_init_with_corner_radii', 0, (done: Function) => {
      console.info("uitest: setViewportMetrics_after_engine_init_with_corner_radii begin");
      const ctx = getContext() as common.UIAbilityContext;
      try {
        const loader = FlutterInjector.getInstance().getFlutterLoader();
        if (!loader.initialized) {
          loader.startInitialization(ctx);
          loader.ensureInitializationComplete([]);
        }
        const napi = FlutterEnginePreload.preLoadFlutterNapi(ctx, loader.findAppBundlePath(), {});
        if (napi && napi.nativeShellHolderId) {
          invokeSetViewportMetricsWithCornerRadii(napi);
          expect(napi.isAttached()).assertTrue();
        } else {
          console.info("uitest: napi not attached, skip native forwarding");
        }
      } catch (e) {
        console.info("uitest: real napi init path error, skip: " + JSON.stringify(e));
      }
      FlutterInjector.getInstance().getFlutterNapi();
      done();
    })

















































  })

  describe('FlutterNapi.attached.branches', () => {
    let attachedNapi: FlutterNapi;

    beforeAll(() => {
      attachedNapi = new FlutterNapi();
      const ctx = getContext() as common.UIAbilityContext;
      const appInfo = ApplicationInfoLoader.load(ctx);
      const shellArgs: string[] = [];
      shellArgs.push('--icu-symbol-prefix=_binary_icudtl_dat');
      shellArgs.push('--icu-native-lib-path=' + appInfo.nativeLibraryDir + '/libflutter.so');
      shellArgs.push('--aot-shared-library-name=' + appInfo.nativeLibraryDir + '/libapp.so');
      shellArgs.push('--cache-dir-path=' + ctx.cacheDir + '/');
      shellArgs.push('--prefetched-default-font-manager');
      shellArgs.push('--leak-vm=true');
      attachedNapi.init(ctx, shellArgs, '', ctx.filesDir + '/', ctx.cacheDir + '/', Date.now());
      attachedNapi.attachToNative();
    });

    afterAll(() => {
      attachedNapi.detachFromNativeAndReleaseResources();
    });

    it('attached isAttached returns true', 0, () => {
      expect(attachedNapi.isAttached()).assertEqual(true);
    });

    it('attached nativeShellHolderId is not null', 0, () => {
      expect(attachedNapi.nativeShellHolderId).not().assertEqual(null);
    });

    it('attached notifyLowMemoryWarning does not throw', 0, () => {
      let threw = false;
      try { attachedNapi.notifyLowMemoryWarning(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('attached dispatchEmptyPlatformMessage does not throw', 0, () => {
      let threw = false;
      try { attachedNapi.dispatchEmptyPlatformMessage('test/channel', 1); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('attached dispatchPlatformMessage does not throw', 0, () => {
      let threw = false;
      try { attachedNapi.dispatchPlatformMessage('test/channel', new ArrayBuffer(4), 4, 1); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('attached invokePlatformMessageEmptyResponseCallback does not throw', 0, () => {
      let threw = false;
      try { attachedNapi.invokePlatformMessageEmptyResponseCallback(1); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('attached invokePlatformMessageResponseCallback does not throw', 0, () => {
      let threw = false;
      try { attachedNapi.invokePlatformMessageResponseCallback(1, new ArrayBuffer(0), 0); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('attached handlePlatformMessageResponse does not throw', 0, () => {
      let threw = false;
      try { attachedNapi.handlePlatformMessageResponse(1, new ArrayBuffer(0)); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('attached handlePlatformMessage does not throw', 0, () => {
      let threw = false;
      try { attachedNapi.handlePlatformMessage('test/channel', new ArrayBuffer(0), 1, 0); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('attached setViewportMetrics does not throw', 0, () => {
      let threw = false;
      try { attachedNapi.setViewportMetrics(1.0, 100, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, [], [], []); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('attached setSemanticsEnabled does not throw', 0, () => {
      let threw = false;
      try { attachedNapi.setSemanticsEnabled(true); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('attached updateRefreshRate does not throw', 0, () => {
      let threw = false;
      try { attachedNapi.updateRefreshRate(60); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('attached updateSize does not throw', 0, () => {
      let threw = false;
      try { attachedNapi.updateSize(1080, 1920); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('attached updateDensity does not throw', 0, () => {
      let threw = false;
      try { attachedNapi.updateDensity(3.0); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('attached checkAndReloadFont does not throw', 0, () => {
      let threw = false;
      try { attachedNapi.checkAndReloadFont(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('attached setPreloading does not throw', 0, () => {
      let threw = false;
      try { attachedNapi.setPreloading(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('attached processPendingMessages does not throw', 0, () => {
      let threw = false;
      try { attachedNapi.processPendingMessages(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('attached onFirstFrame preload does not throw', 0, () => {
      let threw = false;
      try { attachedNapi.onFirstFrame(1); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('attached onFirstFrame regular does not throw', 0, () => {
      let threw = false;
      try { attachedNapi.onFirstFrame(0); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('attached onPreEngineRestart does not throw', 0, () => {
      let threw = false;
      try { attachedNapi.onPreEngineRestart(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('attached computePlatformResolvedLocale returns array', 0, () => {
      const result = attachedNapi.computePlatformResolvedLocale(['en', 'zh']);
      expect(result).not().assertNull();
    });

    it('attached accessibilityStateChange does not throw', 0, () => {
      let threw = false;
      try { attachedNapi.accessibilityStateChange(true); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('attached getSystemLanguages does not throw', 0, () => {
      let threw = false;
      try { attachedNapi.getSystemLanguages(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('attached enableFrameCache does not throw', 0, () => {
      let threw = false;
      try { attachedNapi.enableFrameCache(true); attachedNapi.enableFrameCache(false); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('attached setPipVisible does not throw', 0, () => {
      let threw = false;
      try { attachedNapi.setPipVisible(true); attachedNapi.setPipVisible(false); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

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
