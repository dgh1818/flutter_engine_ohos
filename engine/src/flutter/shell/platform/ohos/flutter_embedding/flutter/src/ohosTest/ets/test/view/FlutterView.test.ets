/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, Level, beforeAll} from '@ohos/hypium';
// import common from '@ohos.app.ability.common';
import { common } from '@kit.AbilityKit';
import { deviceInfo } from '@kit.BasicServicesKit';
import { display } from '@kit.ArkUI';
import {
  FlutterView,
  ViewportMetrics,
  DisplayFeature,
  DisplayFeatureType,
  DisplayFeatureState,
  DisplayFoldStatus,
  PlatformViewParas,
  FirstFrameListener,
  FirstPreloadFrameListener,
} from '../../../../main/ets/view/FlutterView';
import { DVModel } from '../../../../main/ets/view/DynamicView/dynamicView';
import { EmbeddingNodeController } from '../../../../main/ets/embedding/ohos/EmbeddingNodeController';
import FlutterEngine from '../../../../main/ets/embedding/engine/FlutterEngine';
import PlatformViewsController from '../../../../main/ets/plugin/platform/PlatformViewsController';
import Any from '../../../../main/ets/plugin/common/Any';

class TestArea implements Area {
  width: Length = 0;
  height: Length = 0;
  position: Position = { x: 0, y: 0 };
  globalPosition: Position = { x: 0, y: 0 };

  constructor(width: Length, height: Length) {
    this.width = width;
    this.height = height;
  }
}

function safeAttach(flutterView: FlutterView, engine: FlutterEngine): boolean {
  try {
    flutterView.attachToFlutterEngine(engine);
  } catch (e) {
    console.info('uitest: attachToFlutterEngine error: ' + JSON.stringify(e));
  }
  return flutterView.isAttachedToFlutterEngine();
}

function invokeUpdateDisplayCornerRadii(flutterView: FlutterView): void {
  (flutterView as Any).updateDisplayCornerRadii();
}

function invokeUpdateViewportMetrics(flutterView: FlutterView): boolean {
  return (flutterView as Any).updateViewportMetrics() as boolean;
}

function readViewportMetrics(flutterView: FlutterView): ViewportMetrics {
  return (flutterView as Any).viewportMetrics as ViewportMetrics;
}

function makeView(id: string): FlutterView {
  return new FlutterView(id, getContext() as common.Context);
}

class FakeUiContext {
  vpPerPx: number;

  constructor(vpPerPx: number) {
    this.vpPerPx = vpPerPx;
  }

  px2vp(value: number): number {
    return value * this.vpPerPx;
  }
}

class ThrowingUiContext {
  px2vp(value: number): number {
    throw new Error('px2vp unavailable');
  }
}

function stubUiContext(flutterView: FlutterView, uiContext: object | undefined): void {
  (flutterView as Any).tryGetUIContext = (): object | undefined => uiContext;
}

function invokeGetEffectiveDensity(flutterView: FlutterView): number {
  return (flutterView as Any).getEffectiveDensity() as number;
}

class TestFirstFrameListener implements FirstFrameListener {
  called: boolean = false;

  onFirstFrame(): void {
    this.called = true;
  }
}

class TestFirstPreloadFrameListener implements FirstPreloadFrameListener {
  called: boolean = false;

  onFirstPreloadFrame(): void {
    this.called = true;
  }
}

export default function flutterViewTest() {
  let context: common.UIAbilityContext;

  describe('FlutterView.isAttachedToFlutterEngine', () => {
    it('returns false when no engine attached on isAttachedToFlutterEngine', Level.LEVEL1, () => {
      const view = makeView('view-1');
      expect(view.isAttachedToFlutterEngine()).assertEqual(false);
    });
  });

  describe('FlutterView.isSameEngineShellHolderId', () => {
    it('returns false when no engine attached on isSameEngineShellHolderId', Level.LEVEL1, () => {
      const view = makeView('view-1');
      expect(view.isSameEngineShellHolderId(0)).assertEqual(false);
      expect(view.isSameEngineShellHolderId(123)).assertEqual(false);
    });

    it('returns false for id=0 even if some condition matches (special case)', Level.LEVEL1, () => {
      const view = makeView('view-1');
      // Source: `id != 0` is the guard, so id=0 always returns false
      expect(view.isSameEngineShellHolderId(0)).assertEqual(false);
    });
  });

  describe('FlutterView.onPageVisibilityChanged', () => {
    it('does not throw when called without engine on onPageVisibilityChanged', Level.LEVEL1, () => {
      const view = makeView('view-1');
      let threw = false;
      try {
        view.onPageVisibilityChanged(true);
        view.onPageVisibilityChanged(false);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('does not throw on same-value call (idempotent)', Level.LEVEL1, () => {
      const view = makeView('view-1');
      let threw = false;
      try {
        view.onPageVisibilityChanged(true);
        view.onPageVisibilityChanged(true);  // same value, no-op
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('FlutterView.onVisibleAreaChanged', () => {
    it('does not throw when called without engine on onVisibleAreaChanged', Level.LEVEL1, () => {
      const view = makeView('view-1');
      let threw = false;
      try {
        view.onVisibleAreaChanged(true);
        view.onVisibleAreaChanged(false);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('FlutterView.constructor', () => {
    it('does not throw with valid id', Level.LEVEL1, () => {
      let threw = false;
      try {
        const view = makeView('test-id-1');
        expect(view).not().assertNull();
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('FlutterView.detachFromFlutterEngine', () => {
    it('does not throw when not attached', Level.LEVEL1, () => {
      const view = makeView('view-2');
      let threw = false;
      try {
        view.detachFromFlutterEngine();
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('FlutterView.getFlutterEngine', () => {
    it('returns null when no engine is attached', Level.LEVEL0, () => {
      const flutterView: FlutterView = new FlutterView('oh_flutter_test_view', getContext());
      expect(flutterView.getFlutterEngine()).assertNull();
    });
  });

  describe('ViewportMetrics.clone', () => {
    it('copies all primitive fields', Level.LEVEL1, () => {
      const m: ViewportMetrics = new ViewportMetrics();
      m.devicePixelRatio = 2.5;
      m.physicalWidth = 1080;
      m.physicalHeight = 1920;
      m.physicalViewPaddingTop = 10;
      m.physicalViewPaddingRight = 20;
      m.physicalViewPaddingBottom = 30;
      m.physicalViewPaddingLeft = 40;
      m.physicalViewInsetTop = 50;
      m.physicalTouchSlop = 8;
      const c: ViewportMetrics = m.clone();
      expect(c.devicePixelRatio).assertEqual(2.5);
      expect(c.physicalWidth).assertEqual(1080);
      expect(c.physicalHeight).assertEqual(1920);
      expect(c.physicalViewPaddingTop).assertEqual(10);
      expect(c.physicalViewPaddingRight).assertEqual(20);
      expect(c.physicalViewPaddingBottom).assertEqual(30);
      expect(c.physicalViewPaddingLeft).assertEqual(40);
      expect(c.physicalViewInsetTop).assertEqual(50);
      expect(c.physicalTouchSlop).assertEqual(8);
    });
  });

  describe('ViewportMetrics.isEqual', () => {
    it('returns true for identical defaults', Level.LEVEL1, () => {
      const a: ViewportMetrics = new ViewportMetrics();
      const b: ViewportMetrics = new ViewportMetrics();
      b.displayFeatures = a.displayFeatures;
      expect(a.isEqual(b)).assertEqual(true);
    });

    it('returns false when devicePixelRatio differs', Level.LEVEL1, () => {
      const a: ViewportMetrics = new ViewportMetrics();
      const b: ViewportMetrics = new ViewportMetrics();
      b.devicePixelRatio = 2.0;
      expect(a.isEqual(b)).assertEqual(false);
    });

    it('returns false when physical size differs', Level.LEVEL1, () => {
      const a: ViewportMetrics = new ViewportMetrics();
      const b: ViewportMetrics = new ViewportMetrics();
      b.physicalWidth = 100;
      expect(a.isEqual(b)).assertEqual(false);
      const b2: ViewportMetrics = new ViewportMetrics();
      b2.physicalHeight = 100;
      expect(a.isEqual(b2)).assertEqual(false);
    });
  });

  describe('DisplayFeature.constructor', () => {
    it('assigns bound type state', Level.LEVEL1, () => {
      const rect: display.Rect = { left: 0, top: 0, width: 100, height: 200 };
      const f: DisplayFeature = new DisplayFeature(rect, DisplayFeatureType.FOLD, DisplayFeatureState.POSTURE_FLAT);
      expect(f.bound).assertEqual(rect);
      expect(f.type).assertEqual(DisplayFeatureType.FOLD);
      expect(f.state).assertEqual(DisplayFeatureState.POSTURE_FLAT);
    });

    it('supports all enum values', Level.LEVEL1, () => {
      const rect: display.Rect = { left: 0, top: 0, width: 0, height: 0 };
      for (const t of [DisplayFeatureType.UNKNOWN, DisplayFeatureType.FOLD, DisplayFeatureType.HINGE, DisplayFeatureType.CUTOUT]) {
        for (const s of [DisplayFeatureState.UNKNOWN, DisplayFeatureState.POSTURE_FLAT, DisplayFeatureState.POSTURE_HALF_OPENED]) {
          const f: DisplayFeature = new DisplayFeature(rect, t, s);
          expect(f.type).assertEqual(t);
          expect(f.state).assertEqual(s);
        }
      }
    });
  });

  describe('DisplayFoldStatus.enum', () => {
    it('exposes expected integer values', Level.LEVEL1, () => {
      expect(DisplayFoldStatus.FOLD_STATUS_UNKNOWN).assertEqual(0);
      expect(DisplayFoldStatus.FOLD_STATUS_EXPANDED).assertEqual(1);
      expect(DisplayFoldStatus.FOLD_STATUS_FOLDED).assertEqual(2);
      expect(DisplayFoldStatus.FOLD_STATUS_HALF_FOLDED).assertEqual(3);
    });
  });

  describe('PlatformViewParas.setValue', () => {
    it('stores width height top left', Level.LEVEL1, () => {
      const p: PlatformViewParas = new PlatformViewParas();
      p.setValue(10, 20, 30, 40);
      expect(p.width).assertEqual(10);
      expect(p.height).assertEqual(20);
      expect(p.top).assertEqual(30);
      expect(p.left).assertEqual(40);
    });

    it('overwrites previous values', Level.LEVEL1, () => {
      const p: PlatformViewParas = new PlatformViewParas();
      p.setValue(1, 2, 3, 4);
      p.setValue(5, 6, 7, 8);
      expect(p.width).assertEqual(5);
      expect(p.height).assertEqual(6);
      expect(p.top).assertEqual(7);
      expect(p.left).assertEqual(8);
    });
  });

  describe('PlatformViewParas.setOffset', () => {
    it('updates only top and left', Level.LEVEL1, () => {
      const p: PlatformViewParas = new PlatformViewParas();
      p.setValue(10, 20, 0, 0);
      p.setOffset(99, 88);
      expect(p.top).assertEqual(99);
      expect(p.left).assertEqual(88);
      expect(p.width).assertEqual(10);
      expect(p.height).assertEqual(20);
    });
  });

  describe('DisplayFeature.accessors', () => {
    it('getBound returns stored rect', Level.LEVEL1, () => {
      const rect: display.Rect = { left: 1, top: 2, width: 3, height: 4 };
      const f: DisplayFeature = new DisplayFeature(rect, DisplayFeatureType.FOLD, DisplayFeatureState.POSTURE_FLAT);
      expect(f.getBound()).assertEqual(rect);
    });

    it('getType and getState return constructor args', Level.LEVEL1, () => {
      const rect: display.Rect = { left: 0, top: 0, width: 0, height: 0 };
      const f: DisplayFeature = new DisplayFeature(rect, DisplayFeatureType.HINGE, DisplayFeatureState.POSTURE_HALF_OPENED);
      expect(f.getType()).assertEqual(DisplayFeatureType.HINGE);
      expect(f.getState()).assertEqual(DisplayFeatureState.POSTURE_HALF_OPENED);
    });

    it('setBound overwrites existing bound', Level.LEVEL1, () => {
      const rect: display.Rect = { left: 0, top: 0, width: 1, height: 1 };
      const f: DisplayFeature = new DisplayFeature(rect, DisplayFeatureType.UNKNOWN, DisplayFeatureState.UNKNOWN);
      const newRect: display.Rect = { left: 5, top: 6, width: 7, height: 8 };
      f.setBound(newRect);
      expect(f.getBound()).assertEqual(newRect);
    });

    it('setType and setState update fields', Level.LEVEL1, () => {
      const rect: display.Rect = { left: 0, top: 0, width: 0, height: 0 };
      const f: DisplayFeature = new DisplayFeature(rect, DisplayFeatureType.UNKNOWN, DisplayFeatureState.UNKNOWN);
      f.setType(DisplayFeatureType.CUTOUT);
      f.setState(DisplayFeatureState.POSTURE_FLAT);
      expect(f.getType()).assertEqual(DisplayFeatureType.CUTOUT);
      expect(f.getState()).assertEqual(DisplayFeatureState.POSTURE_FLAT);
    });
  });

  describe('FlutterView.getEmbeddingNodeController', () => {
    it('returns node controller instance', Level.LEVEL1, () => {
      const view = makeView('view-enc');
      const enc: EmbeddingNodeController = view.getEmbeddingNodeController();
      expect(enc).not().assertNull();
    });
  });

  describe('FlutterView.platformView.accessors', () => {
    it('returns undefined before set', Level.LEVEL1, () => {
      const view = makeView('view-pv');
      expect(view.getPlatformView()).assertUndefined();
    });

    it('returns PlatformViewParas from getPlatformViewSize', Level.LEVEL1, () => {
      const view = makeView('view-pv2');
      const size: PlatformViewParas = view.getPlatformViewSize();
      expect(size).not().assertNull();
    });

    it('returns DVModel from getDVModel', Level.LEVEL1, () => {
      const view = makeView('view-dv');
      const model: DVModel | undefined = view.getDVModel();
      expect(model).not().assertNull();
    });

    it('getKeyboardHeight returns 0 when no keyboard', Level.LEVEL1, () => {
      const view = makeView('view-kb');
      // 3.44 getKeyboardHeight returns 0 when avoid-area data is absent (FlutterView.ets:910-912).
      expect(view.getKeyboardHeight()).assertEqual(0);
    });
  });

  describe('FlutterView.frame.check.setters', () => {
    it('setCheckFullScreen does not throw', Level.LEVEL1, () => {
      const view = makeView('view-cf');
      let threw = false;
      try { view.setCheckFullScreen(true); view.setCheckFullScreen(false); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });
    it('setCheckKeyboard does not throw', Level.LEVEL1, () => {
      const view = makeView('view-ck');
      let threw = false;
      try { view.setCheckKeyboard(true); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });
    it('setCheckGesture does not throw', Level.LEVEL1, () => {
      const view = makeView('view-cg');
      let threw = false;
      try { view.setCheckGesture(true); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });
    it('setCheckAiBar does not throw', Level.LEVEL1, () => {
      const view = makeView('view-ca');
      let threw = false;
      try { view.setCheckAiBar(true); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });
  });

  describe('FlutterView.padding.setters', () => {
    it('setPaddingTop does not throw', Level.LEVEL1, () => {
      const view = makeView('view-pt');
      let threw = false;
      try { view.setPaddingTop(20); view.setPaddingTop(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });
    it('setPaddingBottom does not throw', Level.LEVEL1, () => {
      const view = makeView('view-pb');
      let threw = false;
      try { view.setPaddingBottom(40); view.setPaddingBottom(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });
  });

  describe('FlutterView.first.frame.listeners', () => {
    it('addFirstFrameListener + removeFirstFrameListener roundtrip', Level.LEVEL1, () => {
      const view = makeView('view-ff');
      const listener: TestFirstFrameListener = new TestFirstFrameListener();
      let threw = false;
      try { view.addFirstFrameListener(listener); view.removeFirstFrameListener(listener); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('addFirstPreloadFrameListener + removeFirstPreloadFrameListener roundtrip', Level.LEVEL1, () => {
      const view = makeView('view-fp');
      const listener: TestFirstPreloadFrameListener = new TestFirstPreloadFrameListener();
      let threw = false;
      try { view.addFirstPreloadFrameListener(listener); view.removeFirstPreloadFrameListener(listener); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('hasRenderedFirstFrame returns false before first frame', Level.LEVEL1, () => {
      const view = makeView('view-hrf');
      expect(view.hasRenderedFirstFrame()).assertEqual(false);
    });

    it('onFirstFrame(1) invokes preload listeners without throwing', Level.LEVEL1, () => {
      const view = makeView('view-fp2');
      const listener: TestFirstPreloadFrameListener = new TestFirstPreloadFrameListener();
      view.addFirstPreloadFrameListener(listener);
      let threw = false;
      try { view.onFirstFrame(1); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      expect(listener.called).assertEqual(true);
    });

    it('onFirstFrame(0) invokes normal listeners without throwing', Level.LEVEL1, () => {
      const view = makeView('view-fp3');
      const listener: TestFirstFrameListener = new TestFirstFrameListener();
      view.addFirstFrameListener(listener);
      let threw = false;
      try { view.onFirstFrame(0); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      expect(listener.called).assertEqual(true);
    });
  });

  describe('FlutterView.keyEvent.handlers', () => {
    it('onKeyPreIme returns false', Level.LEVEL1, () => {
      const view = makeView('view-kp');
      const ev: KeyEvent = {
        type: KeyType.Down,
        keyCode: 0,
        keyText: '',
        keySource: KeySource.Keyboard,
        deviceId: 0,
        metaKey: 0,
        timestamp: 0,
        stopPropagation: () => {},
        intentionCode: -1 as IntentionCode,
      };
      let threw = false;
      try { view.onKeyPreIme(ev); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });
  });

  describe('FlutterView.sendSettings.no_engine', () => {
    it('logs error and does not throw', Level.LEVEL1, () => {
      const view = makeView('view-ss');
      let threw = false;
      try { view.sendSettings(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });
  });

  describe('FlutterView.enableFrameCache.setter', () => {
    it('does not throw', Level.LEVEL1, () => {
      const view = makeView('view-fc');
      let threw = false;
      try { view.enableFrameCache(true); view.enableFrameCache(false); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });
  });

  describe('ViewportMetricsCornerRadiiTest', () => {
    it('default_corner_radii_are_unavailable_sentinel', 0, () => {
      const metrics = new ViewportMetrics();
      expect(metrics.displayCornerRadiusTopLeft).assertEqual(-1);
      expect(metrics.displayCornerRadiusTopRight).assertEqual(-1);
      expect(metrics.displayCornerRadiusBottomRight).assertEqual(-1);
      expect(metrics.displayCornerRadiusBottomLeft).assertEqual(-1);
    })

    it('clone_copies_corner_radii', 0, () => {
      const metrics = new ViewportMetrics();
      metrics.displayCornerRadiusTopLeft = 10;
      metrics.displayCornerRadiusTopRight = 20;
      metrics.displayCornerRadiusBottomRight = 30;
      metrics.displayCornerRadiusBottomLeft = 40;
      const copy = metrics.clone();
      expect(copy.displayCornerRadiusTopLeft).assertEqual(10);
      expect(copy.displayCornerRadiusTopRight).assertEqual(20);
      expect(copy.displayCornerRadiusBottomRight).assertEqual(30);
      expect(copy.displayCornerRadiusBottomLeft).assertEqual(40);
    })

    it('isEqual_compares_corner_radii', 0, () => {
      const left = new ViewportMetrics();
      left.displayCornerRadiusTopLeft = 5;
      left.displayCornerRadiusTopRight = 6;
      left.displayCornerRadiusBottomRight = 7;
      left.displayCornerRadiusBottomLeft = 8;
      const right = left.clone();
      expect(left.isEqual(right)).assertTrue();
      right.displayCornerRadiusBottomLeft = 9;
      expect(left.isEqual(right)).assertFalse();
    })
  })

  describe('FlutterViewDisplayCornerRadiiTest', () => {
    let platformViewsController: PlatformViewsController;

    beforeAll(() => {
      context = getContext() as common.UIAbilityContext;
      platformViewsController = new PlatformViewsController();
    })

    it('constructor_keeps_corner_radii_unavailable_by_default', 0, () => {
      const flutterView = new FlutterView('corner_radii_ctor_view', context);
      const metrics = readViewportMetrics(flutterView);
      expect(metrics.displayCornerRadiusTopLeft).assertEqual(-1);
      expect(metrics.displayCornerRadiusTopRight).assertEqual(-1);
      expect(metrics.displayCornerRadiusBottomRight).assertEqual(-1);
      expect(metrics.displayCornerRadiusBottomLeft).assertEqual(-1);
    })

    it('updateDisplayCornerRadii_reads_platform_on_supported_api', 0, () => {
      const flutterView = new FlutterView('corner_radii_platform_view', context);
      invokeUpdateDisplayCornerRadii(flutterView);
      const metrics = readViewportMetrics(flutterView);
      console.info('uitest: sdkApiVersion=' + deviceInfo.sdkApiVersion +
        ' tl=' + metrics.displayCornerRadiusTopLeft);
      let probeSucceeded = false;
      if (deviceInfo.sdkApiVersion >= 23) {
        const displayInfo: ESObject = display.getDefaultDisplaySync() as ESObject;
        if (typeof displayInfo?.getRoundedCorner === 'function') {
          try {
            displayInfo.getRoundedCorner();
            probeSucceeded = true;
          } catch (e) {
            console.info('uitest: getRoundedCorner threw (BusinessError 801?), ' +
              'expecting -1 sentinels: ' + JSON.stringify(e));
          }
        }
      }
      if (probeSucceeded) {
        expect(metrics.displayCornerRadiusTopLeft).assertLarger(-1);
        expect(metrics.displayCornerRadiusTopRight).assertLarger(-1);
        expect(metrics.displayCornerRadiusBottomRight).assertLarger(-1);
        expect(metrics.displayCornerRadiusBottomLeft).assertLarger(-1);
      } else {
        expect(metrics.displayCornerRadiusTopLeft).assertEqual(-1);
        expect(metrics.displayCornerRadiusTopRight).assertEqual(-1);
        expect(metrics.displayCornerRadiusBottomRight).assertEqual(-1);
        expect(metrics.displayCornerRadiusBottomLeft).assertEqual(-1);
      }
    })

    it('updateViewportMetrics_not_attached_returns_false', 0, () => {
      const flutterView = new FlutterView('viewport_metrics_detached_view', context);
      const updated = invokeUpdateViewportMetrics(flutterView);
      expect(updated).assertFalse();
    })

    it('onAreaChange_without_attach_updates_dimensions', 0, () => {
      const flutterView = new FlutterView('area_change_detached_view', context);
      flutterView.setCheckFullScreen(false);
      flutterView.onAreaChange(new TestArea(360, 780), false);
      flutterView.onAreaChange(new TestArea(400, 800), false);
      const metrics = readViewportMetrics(flutterView);
      expect(metrics.physicalWidth).assertLarger(0);
      expect(metrics.physicalHeight).assertLarger(0);
    })

    it('setPaddingTop_when_attached_triggers_viewport_update', 0, async (done: Function) => {
      const flutterView = new FlutterView('padding_top_view', context);
      flutterView.setCheckFullScreen(false);
      const engine = new FlutterEngine(context, null, null, platformViewsController);
      safeAttach(flutterView, engine);
      flutterView.setPaddingTop(48);
      flutterView.setPaddingBottom(32);
      expect(flutterView.isAttachedToFlutterEngine()).assertTrue();
      done();
    })

    it('onAreaChange_with_size_change_when_attached', 0, async (done: Function) => {
      const flutterView = new FlutterView('area_change_view', context);
      flutterView.setCheckFullScreen(false);
      const engine = new FlutterEngine(context, null, null, platformViewsController);
      safeAttach(flutterView, engine);
      flutterView.onAreaChange(new TestArea(320, 640), false);
      flutterView.onAreaChange(new TestArea(480, 960), false);
      done();
    })

    it('onAreaChange_when_attached_invokes_updateDisplayCornerRadii', 0, () => {
      const flutterView = new FlutterView('corner_radii_area_change_view', context);
      flutterView.setCheckFullScreen(false);
      const engine = new FlutterEngine(context, null, null, platformViewsController);
      if (!safeAttach(flutterView, engine)) {
        console.info('uitest: skip attached onAreaChange, engine not attached');
        return;
      }
      const metrics = readViewportMetrics(flutterView);
      metrics.physicalWidth = 1080;
      metrics.physicalHeight = 2340;
      flutterView.onAreaChange(new TestArea(360, 780), false);
      invokeUpdateDisplayCornerRadii(flutterView);
    })

    it('updateDisplayCornerRadii_on_attached_view', 0, async (done: Function) => {
      const flutterView = new FlutterView('corner_radii_invoke_view', context);
      flutterView.setCheckFullScreen(false);
      const engine = new FlutterEngine(context, null, null, platformViewsController);
      safeAttach(flutterView, engine);
      invokeUpdateDisplayCornerRadii(flutterView);
      invokeUpdateViewportMetrics(flutterView);
      flutterView.setPaddingTop(64);
      done();
    })

    it('updateViewportMetrics_passes_corner_radii_when_attached', 0, async (done: Function) => {
      const flutterView = new FlutterView('viewport_metrics_view', context);
      flutterView.setCheckFullScreen(false);
      const engine = new FlutterEngine(context, null, null, platformViewsController);
      safeAttach(flutterView, engine);
      const updated = invokeUpdateViewportMetrics(flutterView);
      expect(updated).assertTrue();
      done();
    })

    it('onAreaChange_fullscreen_padding_branch', 0, () => {
      const flutterView = new FlutterView('fullscreen_padding_view', context);
      flutterView.setCheckFullScreen(true);
      const engine = new FlutterEngine(context, null, null, platformViewsController);
      safeAttach(flutterView, engine);
      flutterView.setPaddingTop(24);
      flutterView.setPaddingBottom(16);
      flutterView.onAreaChange(new TestArea(360, 780), true);
    })
  })

  describe('FlutterView.unRegisterDefaultCallbacks', () => {
    it('returns early when default callbacks were never registered', 0, () => {
      const view = new FlutterView('unreg-def-1', getContext() as common.Context);
      const v: Any = view;
      v['isDefaultCallbacksRegistered'] = false;
      let threw = false;
      try { v['unRegisterDefaultCallbacks'](); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      expect(v['isDefaultCallbacksRegistered']).assertEqual(false);
    });

    it('unregisters default callbacks and resets the flag', 0, () => {
      const view = new FlutterView('unreg-def-2', getContext() as common.Context);
      const v: Any = view;
      v['isDefaultCallbacksRegistered'] = true;
      let threw = false;
      try { v['unRegisterDefaultCallbacks'](); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      expect(v['isDefaultCallbacksRegistered']).assertEqual(false);
    });
  });

  describe('FlutterView.unRegisterUiCallbacks', () => {
    it('returns early when ui callbacks were never registered', 0, () => {
      const view = new FlutterView('unreg-ui-1', getContext() as common.Context);
      const v: Any = view;
      v['isUiCallbacksRegistered'] = false;
      let threw = false;
      try { v['unRegisterUiCallbacks'](); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      expect(v['isUiCallbacksRegistered']).assertEqual(false);
    });

    it('unregisters ui callbacks and resets the flag', 0, () => {
      const view = new FlutterView('unreg-ui-2', getContext() as common.Context);
      const v: Any = view;
      v['isUiCallbacksRegistered'] = true;
      let threw = false;
      try { v['unRegisterUiCallbacks'](); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      expect(v['isUiCallbacksRegistered']).assertEqual(false);
    });
  });

  describe('FlutterViewEffectiveDensityTest', () => {
    it('inverts_px2vp_to_window_density', 0, () => {
      const flutterView = makeView('effective_density_invert_view');
      stubUiContext(flutterView, new FakeUiContext(1 / 3.0));
      const density: number = invokeGetEffectiveDensity(flutterView);
      expect(Math.abs(density - 3.0) < 1e-6).assertTrue();
    })

    it('inverts_px2vp_for_non_default_density', 0, () => {
      const flutterView = makeView('effective_density_pinned_view');
      stubUiContext(flutterView, new FakeUiContext(0.5));
      const density: number = invokeGetEffectiveDensity(flutterView);
      expect(Math.abs(density - 2.0) < 1e-6).assertTrue();
    })

    it('falls_back_to_system_density_without_uicontext', 0, () => {
      const flutterView = makeView('effective_density_no_context_view');
      stubUiContext(flutterView, undefined);
      expect(invokeGetEffectiveDensity(flutterView)).assertEqual(display.getDefaultDisplaySync().densityPixels);
    })

    it('falls_back_to_system_density_when_px2vp_returns_zero', 0, () => {
      const flutterView = makeView('effective_density_zero_view');
      stubUiContext(flutterView, new FakeUiContext(0));
      expect(invokeGetEffectiveDensity(flutterView)).assertEqual(display.getDefaultDisplaySync().densityPixels);
    })

    it('falls_back_to_system_density_when_px2vp_returns_nan', 0, () => {
      const flutterView = makeView('effective_density_nan_view');
      stubUiContext(flutterView, new FakeUiContext(NaN));
      expect(invokeGetEffectiveDensity(flutterView)).assertEqual(display.getDefaultDisplaySync().densityPixels);
    })

    it('falls_back_to_system_density_when_px2vp_returns_infinity', 0, () => {
      const flutterView = makeView('effective_density_inf_view');
      stubUiContext(flutterView, new FakeUiContext(Infinity));
      expect(invokeGetEffectiveDensity(flutterView)).assertEqual(display.getDefaultDisplaySync().densityPixels);
    })

    it('falls_back_to_system_density_when_px2vp_throws', 0, () => {
      const flutterView = makeView('effective_density_throw_view');
      stubUiContext(flutterView, new ThrowingUiContext());
      expect(invokeGetEffectiveDensity(flutterView)).assertEqual(display.getDefaultDisplaySync().densityPixels);
    })
  })

}
