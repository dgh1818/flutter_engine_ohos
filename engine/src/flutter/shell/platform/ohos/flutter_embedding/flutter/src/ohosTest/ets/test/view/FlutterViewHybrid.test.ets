/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, expect, it } from '@ohos/hypium';
import common from '@ohos.app.ability.common';
import Any from '../../../../main/ets/plugin/common/Any';
import { FlutterView } from '../../../../main/ets/view/FlutterView';

function makeTouchEvent(): TouchEvent {
  return {
    type: TouchType.Down,
    timestamp: 1000,
    changedTouches: [{ id: 1, windowX: 10, windowY: 20 }]
  } as TouchEvent;
}

function makeHoverEvent(): HoverEvent {
  return {
    windowX: 12,
    windowY: 34,
    timestamp: 2000
  } as HoverEvent;
}

function makeAxisEvent(): AxisEvent {
  return {
    action: 2,
    windowX: 1,
    windowY: 2,
    timestamp: 3000,
    getHorizontalAxisValue: (): number => 0,
    getVerticalAxisValue: (): number => 3
  } as AxisEvent;
}

/** Duck-typed Hybrid controller so dispatch*ToEngineHybrid can leave the early-return. */
class StubHybridController {
  hoverCalls: number = 0;
  hoverValues: boolean[] = [];
  axisCalls: number = 0;
  touchCalls: number = 0;

  dispatchTouchToEngine(viewId: number, event: TouchEvent): void {
    if (viewId >= 0 && event != null) {
      this.touchCalls++;
    }
  }

  dispatchHoverToEngine(viewId: number, isHover: boolean, event: HoverEvent): void {
    if (viewId >= 0 && event != null) {
      this.hoverCalls++;
      this.hoverValues.push(isHover);
    }
  }

  dispatchAxisToEngine(viewId: number, event: AxisEvent): void {
    if (viewId >= 0 && event != null) {
      this.axisCalls++;
    }
  }
}

class StubFlutterNapi {
  attached: boolean = false;
  hybridEnabled: boolean = false;

  isAttached(): boolean {
    return this.attached;
  }

  isHybridCompositionEnabled(): boolean {
    return this.hybridEnabled;
  }
}

class StubFlutterLoader {
  isEnableHcpp: boolean = false;
  isEnableImpeller: boolean = false;
}

class StubFlutterEngine {
  hybrid: StubHybridController | null = new StubHybridController();
  napi: StubFlutterNapi = new StubFlutterNapi();
  loader: StubFlutterLoader | null = new StubFlutterLoader();

  getPlatformViewsControllerHybrid(): StubHybridController | null {
    return this.hybrid;
  }

  getFlutterNapi(): StubFlutterNapi {
    return this.napi;
  }

  getFlutterLoader(): StubFlutterLoader | null {
    return this.loader;
  }
}

export default function FlutterViewHybridTest() {
  describe('FlutterViewHybridTest', () => {
    it('isHybridCompositionEnabled_false_when_detached', 0, () => {
      const context: common.UIAbilityContext = getContext() as common.UIAbilityContext;
      const view: FlutterView = new FlutterView('hcpp_ut_view_detached', context);
      expect(view.isAttachedToFlutterEngine()).assertFalse();
      expect(view.isHybridCompositionEnabled()).assertFalse();
    })

    it('isHybridCompositionEnabled_uses_napi_when_attached', 0, () => {
      const context: common.UIAbilityContext = getContext() as common.UIAbilityContext;
      const view: FlutterView = new FlutterView('hcpp_ut_view_hcpp_napi', context);
      const engine: StubFlutterEngine = new StubFlutterEngine();
      engine.napi.attached = true;
      engine.napi.hybridEnabled = true;
      const anyView: Any = view as Any;
      anyView.flutterEngine = engine;
      expect(view.isHybridCompositionEnabled()).assertTrue();
      engine.napi.hybridEnabled = false;
      expect(view.isHybridCompositionEnabled()).assertFalse();
    })

    it('isHybridCompositionEnabled_loader_fallback_all_flag_combos', 0, () => {
      const context: common.UIAbilityContext = getContext() as common.UIAbilityContext;
      const view: FlutterView = new FlutterView('hcpp_ut_view_hcpp_loader', context);
      const engine: StubFlutterEngine = new StubFlutterEngine();
      engine.napi.attached = false;
      const anyView: Any = view as Any;
      anyView.flutterEngine = engine;

      engine.loader = null;
      expect(view.isHybridCompositionEnabled()).assertFalse();

      engine.loader = new StubFlutterLoader();
      engine.loader.isEnableHcpp = true;
      engine.loader.isEnableImpeller = false;
      expect(view.isHybridCompositionEnabled()).assertFalse();

      engine.loader.isEnableHcpp = false;
      engine.loader.isEnableImpeller = true;
      expect(view.isHybridCompositionEnabled()).assertFalse();

      engine.loader.isEnableHcpp = true;
      engine.loader.isEnableImpeller = true;
      expect(view.isHybridCompositionEnabled()).assertTrue();
    })

    it('dispatchTouchToEngineHybrid_safe_when_detached', 0, () => {
      const context: common.UIAbilityContext = getContext() as common.UIAbilityContext;
      const view: FlutterView = new FlutterView('hcpp_ut_view_touch', context);
      view.dispatchTouchToEngineHybrid(1, makeTouchEvent());
      expect(view.isAttachedToFlutterEngine()).assertFalse();
    })

    it('dispatchHoverToEngineHybrid_safe_when_detached', 0, () => {
      const context: common.UIAbilityContext = getContext() as common.UIAbilityContext;
      const view: FlutterView = new FlutterView('hcpp_ut_view_hover', context);
      view.dispatchHoverToEngineHybrid(1, true, makeHoverEvent());
      view.dispatchHoverToEngineHybrid(1, false, makeHoverEvent());
      expect(view.isAttachedToFlutterEngine()).assertFalse();
    })

    it('dispatchAxisToEngineHybrid_safe_when_detached', 0, () => {
      const context: common.UIAbilityContext = getContext() as common.UIAbilityContext;
      const view: FlutterView = new FlutterView('hcpp_ut_view_axis', context);
      view.dispatchAxisToEngineHybrid(1, makeAxisEvent());
      expect(view.isAttachedToFlutterEngine()).assertFalse();
    })

    it('dispatchHoverToEngineHybrid_forwards_when_engine_attached', 0, () => {
      const context: common.UIAbilityContext = getContext() as common.UIAbilityContext;
      const view: FlutterView = new FlutterView('hcpp_ut_view_hover_fwd', context);
      const engine: StubFlutterEngine = new StubFlutterEngine();
      const anyView: Any = view as Any;
      anyView.flutterEngine = engine;
      view.dispatchHoverToEngineHybrid(7, true, makeHoverEvent());
      view.dispatchHoverToEngineHybrid(7, false, makeHoverEvent());
      expect(engine.hybrid!.hoverCalls).assertEqual(2);
      expect(engine.hybrid!.hoverValues.length).assertEqual(2);
      expect(engine.hybrid!.hoverValues[0]).assertTrue();
      expect(engine.hybrid!.hoverValues[1]).assertFalse();
    })

    it('dispatchAxisToEngineHybrid_forwards_when_engine_attached', 0, () => {
      const context: common.UIAbilityContext = getContext() as common.UIAbilityContext;
      const view: FlutterView = new FlutterView('hcpp_ut_view_axis_fwd', context);
      const engine: StubFlutterEngine = new StubFlutterEngine();
      const anyView: Any = view as Any;
      anyView.flutterEngine = engine;
      view.dispatchAxisToEngineHybrid(8, makeAxisEvent());
      expect(engine.hybrid!.axisCalls).assertEqual(1);
    })

    it('dispatchTouchToEngineHybrid_forwards_when_engine_attached', 0, () => {
      const context: common.UIAbilityContext = getContext() as common.UIAbilityContext;
      const view: FlutterView = new FlutterView('hcpp_ut_view_touch_fwd', context);
      const engine: StubFlutterEngine = new StubFlutterEngine();
      const anyView: Any = view as Any;
      anyView.flutterEngine = engine;
      view.dispatchTouchToEngineHybrid(9, makeTouchEvent());
      expect(engine.hybrid!.touchCalls).assertEqual(1);
    })

    it('dispatch_hybrid_safe_when_engine_has_no_controller', 0, () => {
      const context: common.UIAbilityContext = getContext() as common.UIAbilityContext;
      const view: FlutterView = new FlutterView('hcpp_ut_view_no_hybrid', context);
      const engine: StubFlutterEngine = new StubFlutterEngine();
      engine.hybrid = null;
      const anyView: Any = view as Any;
      anyView.flutterEngine = engine;
      view.dispatchTouchToEngineHybrid(1, makeTouchEvent());
      view.dispatchHoverToEngineHybrid(1, true, makeHoverEvent());
      view.dispatchAxisToEngineHybrid(1, makeAxisEvent());
      expect(view.isAttachedToFlutterEngine()).assertTrue();
    })
  })
}
