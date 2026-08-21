/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, expect, it } from '@ohos/hypium';
import FlutterNapi, { HybridCompositionHandler } from '../../../../../main/ets/embedding/engine/FlutterNapi';
import Any from '../../../../../main/ets/plugin/common/Any';

export default function FlutterNapiHybridTest() {
  describe('FlutterNapiHybridTest', () => {
    it('isHybridCompositionEnabled_false_when_shell_null', 0, () => {
      const napi: FlutterNapi = new FlutterNapi();
      expect(napi.nativeShellHolderId).assertNull();
      expect(napi.isHybridCompositionEnabled()).assertFalse();
    })

    it('dispatchTouchToEngine_noop_when_shell_null', 0, () => {
      const napi: FlutterNapi = new FlutterNapi();
      napi.dispatchTouchToEngine([]);
      napi.dispatchTouchToEngine([0, 1, 2]);
      expect(napi.nativeShellHolderId).assertNull();
    })

    it('hybrid_callbacks_safe_without_handler', 0, () => {
      const napi: FlutterNapi = new FlutterNapi();
      napi.setHybridCompositionHandler(null);
      napi.onDisplayPlatformViewHybrid(1, 0, 0, 10, 20, 10, 20);
      napi.onDisplayOverlayHybrid(2, 1, 2, 3, 4);
      napi.onDisplayMutatorsHybrid(3, [1, 0, 0, 1, 0, 0]);
      napi.hidePlatformViewHybrid(4);
      napi.showOverlaySurfaceHybrid();
      napi.hideOverlaySurfaceHybrid();
      napi.onBeginFrameHybrid();
      napi.onEndFrameHybrid();
      expect(napi.isHybridCompositionEnabled()).assertFalse();
    })

    it('hybrid_callbacks_forward_to_mock_handler', 0, () => {
      const napi: FlutterNapi = new FlutterNapi();
      const handler: FakeHybridCompositionHandler = new FakeHybridCompositionHandler();
      napi.setHybridCompositionHandler(handler);

      napi.onDisplayPlatformViewHybrid(7, 1, 2, 30, 40, 50, 60);
      expect(handler.displayPvCalls).assertEqual(1);
      expect(handler.lastViewId).assertEqual(7);
      expect(handler.lastPvWidth).assertEqual(30);
      expect(handler.lastPvHeight).assertEqual(40);
      expect(handler.lastViewWidth).assertEqual(50);
      expect(handler.lastViewHeight).assertEqual(60);

      napi.onDisplayOverlayHybrid(8, 9, 10, 11, 12);
      expect(handler.displayOverlayCalls).assertEqual(1);
      expect(handler.lastOverlayViewId).assertEqual(8);
      expect(handler.lastOverlayX).assertEqual(9);
      expect(handler.lastOverlayY).assertEqual(10);
      expect(handler.lastOverlayW).assertEqual(11);
      expect(handler.lastOverlayH).assertEqual(12);

      napi.onDisplayMutatorsHybrid(9, [1, 0, 0, 1, 5, 6]);
      expect(handler.displayMutatorsCalls).assertEqual(1);
      expect(handler.lastMutatorsViewId).assertEqual(9);
      expect(handler.lastMutatorsData.length).assertEqual(6);

      napi.hidePlatformViewHybrid(10);
      expect(handler.hidePvCalls).assertEqual(1);
      expect(handler.lastHideViewId).assertEqual(10);

      napi.showOverlaySurfaceHybrid();
      napi.hideOverlaySurfaceHybrid();
      napi.onBeginFrameHybrid();
      napi.onEndFrameHybrid();
      expect(handler.showOverlayCalls).assertEqual(1);
      expect(handler.hideOverlayCalls).assertEqual(1);
      expect(handler.beginFrameCalls).assertEqual(1);
      expect(handler.endFrameCalls).assertEqual(1);

      // clear handler → 不再转发
      napi.setHybridCompositionHandler(null);
      napi.onDisplayPlatformViewHybrid(99, 0, 0, 1, 1, 1, 1);
      napi.hidePlatformViewHybrid(99);
      expect(handler.displayPvCalls).assertEqual(1);
      expect(handler.hidePvCalls).assertEqual(1);
    })

    it('isHybridCompositionEnabled_returns_cached_value', 0, () => {
      const napi: FlutterNapi = new FlutterNapi();
      const anyNapi: Any = napi as Any;
      anyNapi.hybridCompositionEnabledCached = true;
      expect(napi.isHybridCompositionEnabled()).assertTrue();
      anyNapi.hybridCompositionEnabledCached = false;
      expect(napi.isHybridCompositionEnabled()).assertFalse();
    })
  })
}

class FakeHybridCompositionHandler implements HybridCompositionHandler {
  displayPvCalls: number = 0;
  displayOverlayCalls: number = 0;
  displayMutatorsCalls: number = 0;
  hidePvCalls: number = 0;
  showOverlayCalls: number = 0;
  hideOverlayCalls: number = 0;
  beginFrameCalls: number = 0;
  endFrameCalls: number = 0;
  lastViewId: number = -1;
  lastPvWidth: number = -1;
  lastPvHeight: number = -1;
  lastViewWidth: number = -1;
  lastViewHeight: number = -1;
  lastOverlayViewId: number = -1;
  lastOverlayX: number = -1;
  lastOverlayY: number = -1;
  lastOverlayW: number = -1;
  lastOverlayH: number = -1;
  lastMutatorsViewId: number = -1;
  lastMutatorsData: number[] = [];
  lastHideViewId: number = -1;

  onDisplayPlatformViewHybrid(viewId: number, x: number, y: number, width: number,
    height: number, viewWidth: number, viewHeight: number): void {
    this.displayPvCalls++;
    this.lastViewId = viewId;
    this.lastPvWidth = width;
    this.lastPvHeight = height;
    this.lastViewWidth = viewWidth;
    this.lastViewHeight = viewHeight;
  }

  onDisplayOverlayHybrid(viewId: number, x: number, y: number, width: number,
    height: number): void {
    this.displayOverlayCalls++;
    this.lastOverlayViewId = viewId;
    this.lastOverlayX = x;
    this.lastOverlayY = y;
    this.lastOverlayW = width;
    this.lastOverlayH = height;
  }

  onDisplayMutatorsHybrid(viewId: number, data: number[]): void {
    this.displayMutatorsCalls++;
    this.lastMutatorsViewId = viewId;
    this.lastMutatorsData = data;
  }

  hidePlatformViewHybrid(viewId: number): void {
    this.hidePvCalls++;
    this.lastHideViewId = viewId;
  }

  showOverlaySurfaceHybrid(): void {
    this.showOverlayCalls++;
  }

  hideOverlaySurfaceHybrid(): void {
    this.hideOverlayCalls++;
  }

  onBeginFrameHybrid(): void {
    this.beginFrameCalls++;
  }

  onEndFrameHybrid(): void {
    this.endFrameCalls++;
  }
}
