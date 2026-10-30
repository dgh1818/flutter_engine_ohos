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
import FlutterNapi, { HybridCompositionHandler, AccessibilityDelegate }
  from '../../../../../main/ets/embedding/engine/FlutterNapi';
import { PlatformMessageHandler } from '../../../../../main/ets/embedding/engine/dart/PlatformMessageHandler';
import { EngineLifecycleListener } from '../../../../../main/ets/embedding/engine/FlutterEngine';
import { Action } from '../../../../../main/ets/embedding/engine/systemchannels/AccessibilityChannel';
import Any from '../../../../../main/ets/plugin/common/Any';
import { describe, expect, it } from '@ohos/hypium';

/**
 * State-machine / pure-logic coverage for FlutterNapi.
 *
 * Uses a fresh FlutterNapi instance per test so the private-field injections
 * never leak between cases (the existing FlutterNapiTest shares one instance).
 * All native-bound happy paths (isAttached()==true leading into `flutter.*`
 * calls) are intentionally avoided; we drive the detached / guard / cached /
 * injected arms that are pure ETS state logic. Static `hasInit` mutations are
 * save/restored around each use to stay order-independent.
 */

class FakePlatformMessageHandler implements PlatformMessageHandler {
  fromDartCount: number = 0;
  responseCount: number = 0;
  lastChannel: string = '';

  handleMessageFromDart(channel: String, _message: ArrayBuffer, _replyId: number, _messageData: number): void {
    this.fromDartCount++;
    this.lastChannel = channel as string;
  }

  handlePlatformMessageResponse(_replyId: number, _reply: ArrayBuffer): void {
    this.responseCount++;
  }
}

class FakeHybridCompositionHandler implements HybridCompositionHandler {
  displayPlatformViewCount: number = 0;
  displayOverlayCount: number = 0;
  displayMutatorsCount: number = 0;
  hidePlatformViewCount: number = 0;
  showOverlayCount: number = 0;
  hideOverlayCount: number = 0;
  beginFrameCount: number = 0;
  endFrameCount: number = 0;

  onDisplayPlatformViewHybrid(_viewId: number, _x: number, _y: number, _width: number,
    _height: number, _viewWidth: number, _viewHeight: number): void {
    this.displayPlatformViewCount++;
  }

  onDisplayOverlayHybrid(_viewId: number, _x: number, _y: number, _width: number, _height: number): void {
    this.displayOverlayCount++;
  }

  onDisplayMutatorsHybrid(_viewId: number, _data: number[]): void {
    this.displayMutatorsCount++;
  }

  hidePlatformViewHybrid(_viewId: number): void {
    this.hidePlatformViewCount++;
  }

  showOverlaySurfaceHybrid(): void {
    this.showOverlayCount++;
  }

  hideOverlaySurfaceHybrid(): void {
    this.hideOverlayCount++;
  }

  onBeginFrameHybrid(): void {
    this.beginFrameCount++;
  }

  onEndFrameHybrid(): void {
    this.endFrameCount++;
  }
}

class FakeEngineLifecycleListener implements EngineLifecycleListener {
  restartCount: number = 0;
  willDestroyCount: number = 0;

  onPreEngineRestart(): void {
    this.restartCount++;
  }

  onEngineWillDestroy(): void {
    this.willDestroyCount++;
  }
}

class FakeAccessibilityDelegate implements AccessibilityDelegate {
  stateCount: number = 0;
  lastState: Boolean | null = null;

  accessibilityStateChange(state: Boolean): void {
    this.stateCount++;
    this.lastState = state;
  }
}

export default function FlutterNapiStateTest() {
  describe('FlutterNapiStateTest', () => {

    it('TestCheckImplementedBothArms', 0, () => {
      const napi = new FlutterNapi();
      // default hasImplemented=false -> L258 T (log) -> returns false
      expect(napi.checkImplemented('m')).assertEqual(false);
      napi.hasImplemented = true;
      expect(napi.checkImplemented('m')).assertEqual(true);
    })

    it('TestIsAttachedBothArms', 0, () => {
      const napi = new FlutterNapi();
      expect(napi.isAttached()).assertEqual(false);
      napi.nativeShellHolderId = 5;
      expect(napi.isAttached()).assertEqual(true);
    })

    it('TestNotifyLowMemoryWarningBothArms', 0, () => {
      // detached: L342 T -> log + return
      const detached = new FlutterNapi();
      let threw = false;
      try {
        detached.notifyLowMemoryWarning();
      } catch (_e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
      // attached: L342 F -> empty-stub nativeNotifyLowMemoryWarning (no throw)
      const attached = new FlutterNapi();
      attached.nativeShellHolderId = 5;
      threw = false;
      try {
        attached.notifyLowMemoryWarning();
      } catch (_e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    })

    it('TestHandlePlatformMessageResponseCallsHandler', 0, () => {
      const napi = new FlutterNapi();
      const handler = new FakePlatformMessageHandler();
      napi.platformMessageHandler = handler;
      napi.handlePlatformMessageResponse(1, new ArrayBuffer(0));
      expect(handler.responseCount).assertEqual(1);
    })

    it('TestHandlePlatformMessageResponseNoHandler', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try {
        napi.handlePlatformMessageResponse(1, new ArrayBuffer(0));
      } catch (_e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    })

    it('TestHandlePlatformMessageDispatchWhenReady', 0, () => {
      const napi = new FlutterNapi();
      const handler = new FakePlatformMessageHandler();
      napi.platformMessageHandler = handler;
      napi.handlePlatformMessage('test/channel', new ArrayBuffer(0), 2, 3);
      expect(handler.fromDartCount).assertEqual(1);
      expect(handler.lastChannel).assertEqual('test/channel');
    })

    it('TestHandlePlatformMessageQueuesDuringPreload', 0, () => {
      const napi = new FlutterNapi();
      const handler = new FakePlatformMessageHandler();
      napi.platformMessageHandler = handler;
      napi.setPreloading(); // ready = false
      napi.handlePlatformMessage('queued/channel', new ArrayBuffer(0), 2, 3);
      expect(handler.fromDartCount).assertEqual(0); // not dispatched
      napi.processPendingMessages(); // drains queue
      expect(handler.fromDartCount).assertEqual(1);
      expect(handler.lastChannel).assertEqual('queued/channel');
    })

    it('TestSetPreloadingSecondCallNoop', 0, () => {
      const napi = new FlutterNapi();
      napi.setPreloading(); // first: L640 T
      expect((napi as Any).firstPreloading).assertEqual(false);
      (napi as Any).readyForHandleMessage = true;
      napi.setPreloading(); // second: L640 F (firstPreloading already false)
      expect((napi as Any).readyForHandleMessage).assertEqual(true);
    })

    it('TestProcessPendingMessagesEmptyNoHandler', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try {
        napi.processPendingMessages();
      } catch (_e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    })

    it('TestProcessPendingMessagesKeepsWhenNoHandler', 0, () => {
      const napi = new FlutterNapi();
      napi.handlePlatformMessage('no/handler', new ArrayBuffer(0), 1, 0); // enqueued
      napi.processPendingMessages(); // while(1>0 && null-handler) -> false, stays queued
      expect((napi as Any).pendingMessages.length).assertEqual(1);
    })

    it('TestLifecycleListenerDispatchAndRemove', 0, () => {
      const napi = new FlutterNapi();
      const listener = new FakeEngineLifecycleListener();
      napi.addEngineLifecycleListener(listener);
      napi.onPreEngineRestart();
      expect(listener.restartCount).assertEqual(1);
      napi.removeEngineLifecycleListener(listener);
      napi.onPreEngineRestart();
      expect(listener.restartCount).assertEqual(1); // removed, no dispatch
    })

    it('TestHybridDispatchCallsHandler', 0, () => {
      const napi = new FlutterNapi();
      const handler = new FakeHybridCompositionHandler();
      (napi as Any).hybridCompositionHandler = handler;
      napi.onDisplayPlatformViewHybrid(1, 0, 0, 10, 10, 10, 10);
      napi.onDisplayOverlayHybrid(1, 0, 0, 10, 10);
      napi.onDisplayMutatorsHybrid(1, [1, 2]);
      napi.hidePlatformViewHybrid(1);
      napi.showOverlaySurfaceHybrid();
      napi.hideOverlaySurfaceHybrid();
      napi.onBeginFrameHybrid();
      napi.onEndFrameHybrid();
      expect(handler.displayPlatformViewCount).assertEqual(1);
      expect(handler.displayOverlayCount).assertEqual(1);
      expect(handler.displayMutatorsCount).assertEqual(1);
      expect(handler.hidePlatformViewCount).assertEqual(1);
      expect(handler.showOverlayCount).assertEqual(1);
      expect(handler.hideOverlayCount).assertEqual(1);
      expect(handler.beginFrameCount).assertEqual(1);
      expect(handler.endFrameCount).assertEqual(1);
    })

    it('TestHybridDispatchNoHandler', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try {
        napi.onDisplayPlatformViewHybrid(1, 0, 0, 10, 10, 10, 10);
        napi.onDisplayOverlayHybrid(1, 0, 0, 10, 10);
        napi.onDisplayMutatorsHybrid(1, [1]);
        napi.hidePlatformViewHybrid(1);
        napi.showOverlaySurfaceHybrid();
        napi.hideOverlaySurfaceHybrid();
        napi.onBeginFrameHybrid();
        napi.onEndFrameHybrid();
      } catch (_e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    })

    it('TestDispatchSemanticsActionBothArms', 0, () => {
      const detached = new FlutterNapi();
      let threw = false;
      try {
        detached.dispatchSemanticsAction(1, Action.TAP, 1);
      } catch (_e) {
        threw = true;
      }
      expect(threw).assertEqual(false); // detached: else arm
      const attached = new FlutterNapi();
      attached.nativeShellHolderId = 5;
      threw = false;
      try {
        attached.dispatchSemanticsAction(1, Action.TAP, 1);
      } catch (_e) {
        threw = true;
      }
      expect(threw).assertEqual(false); // attached: empty-stub T arm
    })

    it('TestSetAccessibilityDelegateBothArms', 0, () => {
      const delegate = new FakeAccessibilityDelegate();
      const detached = new FlutterNapi();
      let threw = false;
      try {
        detached.setAccessibilityDelegate(delegate, 1);
      } catch (_e) {
        threw = true;
      }
      expect(threw).assertEqual(false); // detached: else arm
      const attached = new FlutterNapi();
      attached.nativeShellHolderId = 5;
      attached.setAccessibilityDelegate(delegate, 1); // T arm: stores delegate
      expect(attached.accessibilityDelegate).assertEqual(delegate);
    })

    it('TestAccessibilityStateChangeDelegateCalled', 0, () => {
      const napi = new FlutterNapi();
      const delegate = new FakeAccessibilityDelegate();
      napi.accessibilityDelegate = delegate;
      let threw = false;
      try {
        napi.accessibilityStateChange(true);
      } catch (_e) {
        threw = true;
      }
      expect(threw).assertEqual(false); // L1012 T, L1016 F (detached)
      expect(delegate.stateCount).assertEqual(1);
      expect(delegate.lastState).assertEqual(true);
    })

    it('TestAccessibilityStateChangeNoDelegate', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try {
        napi.accessibilityStateChange(true);
      } catch (_e) {
        threw = true;
      }
      expect(threw).assertEqual(false); // L1012 F, L1016 F
    })

    it('TestIsHybridCompositionEnabledCachedValues', 0, () => {
      const napi = new FlutterNapi();
      (napi as Any).hybridCompositionEnabledCached = true;
      expect(napi.isHybridCompositionEnabled()).assertEqual(true);
      (napi as Any).hybridCompositionEnabledCached = false;
      expect(napi.isHybridCompositionEnabled()).assertEqual(false);
    })

    it('TestIsHybridCompositionEnabledDetached', 0, () => {
      const napi = new FlutterNapi();
      // cached null -> L811 F; nativeShellHolderId null -> L814 T -> false
      expect(napi.isHybridCompositionEnabled()).assertEqual(false);
    })

    it('TestOnTouchEventDetached', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try {
        napi.onTouchEvent([]);
      } catch (_e) {
        threw = true;
      }
      expect(threw).assertEqual(false); // L1366 F: detached no-op
    })

    it('TestOnTouchEventHybridEnabledSkips', 0, () => {
      const napi = new FlutterNapi();
      napi.nativeShellHolderId = 5;
      (napi as Any).hybridCompositionEnabledCached = true; // isHybridCompositionEnabled L811 T
      // arbiter null -> hasTextureViews false -> L1369 T -> return before postTouchEvent
      let threw = false;
      try {
        napi.onTouchEvent([]);
      } catch (_e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    })

    it('TestSpawnReturnsPresetInstance', 0, () => {
      const napi = new FlutterNapi();
      const preset = new FlutterNapi();
      (napi as Any).nextSpawnNapi = preset;
      const ret = napi.spawn('f', 'p', 'r', ['a']);
      expect(ret).assertEqual(preset); // L519 T
      expect((napi as Any).nextSpawnNapi).assertEqual(null); // cleared
    })

    it('TestSpawnAsyncReturnsPresetInstance', 0, async (done: Function) => {
      const napi = new FlutterNapi();
      const preset = new FlutterNapi();
      (napi as Any).nextSpawnNapi = preset;
      const ret = await napi.spawnAsync('f', 'p', 'r', ['a']);
      expect(ret).assertEqual(preset); // L545 T
      expect((napi as Any).nextSpawnNapi).assertEqual(null);
      done();
    })

    it('TestDetachedMessageGuardsNoThrow', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try {
        napi.dispatchEmptyPlatformMessage('ch', 1);
        napi.dispatchPlatformMessage('ch', new ArrayBuffer(0), 0, 1);
        napi.invokePlatformMessageEmptyResponseCallback(1);
        napi.invokePlatformMessageResponseCallback(1, new ArrayBuffer(0), 0);
        napi.setSemanticsEnabledWithRespId(true, 1);
        napi.setSemanticsEnabled(true);
        napi.setFontWeightScale(1.0);
        napi.setFlutterNavigationAction(1, true);
        napi.dispatchTouchToEngine([]);
      } catch (_e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    })

    it('TestTextureDetachedGuardReturns', 0, () => {
      const napi = new FlutterNapi();
      expect(napi.registerTexture(1)).assertEqual(0);
      expect(napi.getTextureNativeWindowId(1)).assertEqual(0);
      expect(napi.getTextureNativeWindowPtr(1)).assertEqual(BigInt('0'));
      expect(napi.setExternalNativeImage(1, 0)).assertEqual(false);
      expect(napi.setExternalNativeImagePtr(1, BigInt('0'))).assertEqual(false);
      expect(napi.resetExternalTexture(1, false)).assertEqual(0);
    })

    it('TestTextureDetachedNoThrow', 0, () => {
      const napi = new FlutterNapi();
      let threw = false;
      try {
        napi.unregisterTexture(1);
        napi.setTextureBackGroundColor(1, 0xFFFFFFFF);
        napi.setTextureBufferSize(1, 100, 100);
        napi.notifyTextureResizing(1, 100, 100);
        napi.enableFrameCache(true);
        napi.setPipVisible(true);
      } catch (_e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    })

    it('TestOnFirstFramePreloadSetsFlag', 0, () => {
      const napi = new FlutterNapi();
      napi.onFirstFrame(1); // L671 T: preload flag set
      expect(napi.isPreloadedFlutterUi).assertEqual(true);
      expect(napi.isDisplayingFlutterUi).assertEqual(false);
    })

    it('TestOnFirstFrameNotPreloadStateMachine', 0, () => {
      const napi = new FlutterNapi();
      napi.onFirstFrame(0); // L671 F -> processPendingMessages, L675 F -> set displaying
      expect(napi.isDisplayingFlutterUi).assertEqual(true);
      napi.onFirstFrame(0); // L675 T: already displaying -> early return
      expect(napi.isDisplayingFlutterUi).assertEqual(true);
    })

    it('TestXComponentDisPatchMouseWheelNonMouse', 0, () => {
      const napi = new FlutterNapi();
      const event: PanGestureEvent = {
        offsetX: 0,
        offsetY: 0,
        velocityX: 0,
        velocityY: 0,
        fingerList: [],
        velocity: 0,
        target: null as ESObject as EventTarget,
        timestamp: 0,
        source: SourceType.Unknown,
        pressure: 0,
        tiltX: 0,
        tiltY: 0,
        sourceTool: SourceTool.Unknown
      };
      let threw = false;
      try {
        napi.xComponentDisPatchMouseWheel('xc', 'type', event);
      } catch (_e) {
        threw = true;
      }
      expect(threw).assertEqual(false); // L1091 T: not a mouse source
    })

    it('TestXComponentDisPatchMouseWheelNoValidFinger', 0, () => {
      const napi = new FlutterNapi();
      const event: PanGestureEvent = {
        offsetX: 0,
        offsetY: 0,
        velocityX: 0,
        velocityY: 0,
        fingerList: [],
        velocity: 0,
        target: null as ESObject as EventTarget,
        timestamp: 0,
        source: SourceType.Mouse,
        pressure: 0,
        tiltX: 0,
        tiltY: 0,
        sourceTool: SourceTool.Unknown
      };
      let threw = false;
      try {
        napi.xComponentDisPatchMouseWheel('xc', 'type', event);
      } catch (_e) {
        threw = true;
      }
      expect(threw).assertEqual(false); // L1091 F + L1095 T: mouse but no valid finger
    })
  })
}
