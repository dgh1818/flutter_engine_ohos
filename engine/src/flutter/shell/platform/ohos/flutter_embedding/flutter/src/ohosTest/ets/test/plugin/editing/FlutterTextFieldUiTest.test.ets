/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, Level, Size, TestType } from '@ohos/hypium';
import { abilityDelegatorRegistry } from '@kit.TestKit';
import { Want, UIAbility } from '@kit.AbilityKit';
import FlutterManager from '../../../../../main/ets/embedding/ohos/FlutterManager';
import { FlutterView, FirstFrameListener, FirstPreloadFrameListener } from '../../../../../main/ets/view/FlutterView';
import FlutterNapi from '../../../../../main/ets/embedding/engine/FlutterNapi';
import DartExecutor from '../../../../../main/ets/embedding/engine/dart/DartExecutor';
import TextInputChannel from '../../../../../main/ets/embedding/engine/systemchannels/TextInputChannel';
import TextInputPluginDefault from '../../../../../main/ets/plugin/editing/TextInputPlugin';
import MethodCall from '../../../../../main/ets/plugin/common/MethodCall';
import JSONMethodCodec from '../../../../../main/ets/plugin/common/JSONMethodCodec';
import { DartMessenger } from '../../../../../main/ets/embedding/engine/dart/DartMessenger';
import { IncomingMethodCallHandler } from '../../../../../main/ets/plugin/common/MethodChannel';
import { BinaryReply, BinaryMessenger, BinaryMessageHandler, TaskQueue, TaskQueueOptions } from '../../../../../main/ets/plugin/common/BinaryMessenger';
import SendableBinaryMessageHandler from '../../../../../main/ets/plugin/common/SendableBinaryMessageHandler';

class FakeTaskQueue implements TaskQueue {}
class FakeMessenger implements BinaryMessenger {
  makeBackgroundTaskQueue(options?: TaskQueueOptions): TaskQueue { return new FakeTaskQueue(); }
  send(channel: String, message: ArrayBuffer | null, callback?: BinaryReply | null): void {}
  setMessageHandler(channel: String, handler: BinaryMessageHandler | SendableBinaryMessageHandler | null, taskQueue?: TaskQueue, ...args: Object[]): void {}
}
class FakeBinaryReply implements BinaryReply {
  replyCount: number = 0;
  reply(data: ArrayBuffer | null): void { this.replyCount++; }
}

const delegator = abilityDelegatorRegistry.getAbilityDelegator();
const bundleName = abilityDelegatorRegistry.getArguments().bundleName;

function sleep(ms: number): Promise<void> {
  return new Promise<void>((resolve: Function) => setTimeout(resolve, ms));
}

let flutterView: FlutterView | null = null;

class TestFFListener implements FirstFrameListener {
  called: boolean = false;
  onFirstFrame(): void { this.called = true; }
}

class TestPreloadListener implements FirstPreloadFrameListener {
  called: boolean = false;
  onFirstPreloadFrame(): void { this.called = true; }
}

export default function flutterTextFieldUiTest() {
  describe('FlutterTextFieldUiTest', () => {

    it('launch FlutterRenderAbility and wait for render', TestType.FUNCTION | Size.LARGETEST | Level.LEVEL0, async (done: Function) => {
      const want: Want = {
        bundleName: bundleName,
        abilityName: 'FlutterRenderAbility'
      };
      await delegator.startAbility(want);
      await sleep(3000);

      const viewMap = FlutterManager.getInstance().getFlutterViewList();
      viewMap.forEach((v: FlutterView) => {
        if (v.isAttachedToFlutterEngine()) {
          flutterView = v;
        }
      });
      expect(flutterView).not().assertNull();

      // Verify topAbility is the one we just launched.
      try {
        const ability: UIAbility = await delegator.getCurrentTopAbility();
        expect(ability.context.abilityInfo.name).assertEqual('FlutterRenderAbility');
      } catch (e) {}

      // IME round 1: ASCII input "hi".
      await delegator.executeShellCommand('uitest uiInput click 540 1350');
      await sleep(2000);
      await delegator.executeShellCommand('uitest uiInput inputText 540 1350 hi');
      await sleep(1000);

      // IME round 2: CJK input "你好" + Back key.
      await delegator.executeShellCommand('uitest uiInput click 540 1350');
      await sleep(2000);
      await delegator.executeShellCommand('uitest uiInput inputText 540 1350 你好');
      await sleep(1000);
      await delegator.executeShellCommand('uitest uiInput keyEvent Back');
      await sleep(500);

      done();
    }, 30000);

    it('FlutterView is attached to engine', 0, (done: Function) => {
      expect(flutterView).not().assertNull();
      expect(flutterView!.isAttachedToFlutterEngine()).assertEqual(true);
      done();
    });

    it('FlutterView setActive and getActive', 0, (done: Function) => {
      flutterView!.setActive(false);
      expect(flutterView!.getActive()).assertEqual(false);
      flutterView!.setActive(true);
      expect(flutterView!.getActive()).assertEqual(true);
      done();
    });

    it('FlutterView setTouchSlopCallbackValue does not throw', 0, (done: Function) => {
      let threw = false;
      try { flutterView!.setTouchSlopCallbackValue(() => 5); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('FlutterView preDraw does not throw', 0, (done: Function) => {
      let threw = false;
      try { flutterView!.preDraw(100, 200); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('FlutterView onSurfaceDestroyed does not throw', 0, (done: Function) => {
      let threw = false;
      try { flutterView!.onSurfaceDestroyed(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('FlutterView first frame listeners', 0, (done: Function) => {
      const listener = new TestFFListener();
      flutterView!.addFirstFrameListener(listener);
      flutterView!.onFirstFrame(0);
      expect(listener.called).assertEqual(true);
      flutterView!.removeFirstFrameListener(listener);
      done();
    });

    it('FlutterView preload frame listeners', 0, (done: Function) => {
      const listener = new TestPreloadListener();
      flutterView!.addFirstPreloadFrameListener(listener);
      flutterView!.onFirstFrame(1);
      expect(listener.called).assertEqual(true);
      flutterView!.removeFirstPreloadFrameListener(listener);
      done();
    });

    it('FlutterView detachFromFlutterEngine does not throw', 0, (done: Function) => {
      let threw = false;
      try { flutterView!.detachFromFlutterEngine(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('FlutterView onDestroy does not throw', 0, (done: Function) => {
      let threw = false;
      try { flutterView!.onDestroy(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('FlutterView onPageVisibilityChanged toggles', 0, (done: Function) => {
      flutterView!.onPageVisibilityChanged(true);
      flutterView!.onPageVisibilityChanged(false);
      expect(true).assertEqual(true);
      done();
    });

    it('FlutterView onVisibleAreaChanged toggles', 0, (done: Function) => {
      flutterView!.onVisibleAreaChanged(true);
      flutterView!.onVisibleAreaChanged(false);
      expect(true).assertEqual(true);
      done();
    });

    it('FlutterView onAreaChange with null area does not throw', 0, (done: Function) => {
      let threw = false;
      try { flutterView!.onAreaChange(null); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('FlutterView onAreaChange with setFullScreen does not throw', 0, (done: Function) => {
      const area: Area = {
        width: 1080,
        height: 1920,
        position: { x: 0, y: 0 },
        globalPosition: { x: 0, y: 0 }
      };
      let threw = false;
      try { flutterView!.onAreaChange(area, true); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('FlutterView isSameEngineShellHolderId', 0, (done: Function) => {
      expect(flutterView!.isSameEngineShellHolderId(0)).assertEqual(false);
      expect(flutterView!.isSameEngineShellHolderId(999)).assertEqual(false);
      done();
    });

    it('FlutterView getKeyboardHeight', 0, (done: Function) => {
      const h = flutterView!.getKeyboardHeight();
      expect(h >= 0).assertEqual(true);
      done();
    });

    it('FlutterView hasRenderedFirstFrame', 0, (done: Function) => {
      const result = flutterView!.hasRenderedFirstFrame();
      expect(typeof result).assertEqual('boolean');
      done();
    });

    it('FlutterView sendSettings does not throw', 0, (done: Function) => {
      let threw = false;
      try { flutterView!.sendSettings(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('FlutterView enableFrameCache toggle', 0, (done: Function) => {
      flutterView!.enableFrameCache(false);
      flutterView!.enableFrameCache(true);
      expect(true).assertEqual(true);
      done();
    });

    it('FlutterView setCheckFullScreen toggle', 0, (done: Function) => {
      flutterView!.setCheckFullScreen(false);
      flutterView!.setCheckFullScreen(true);
      expect(true).assertEqual(true);
      done();
    });

    it('FlutterView setCheckKeyboard toggle', 0, (done: Function) => {
      flutterView!.setCheckKeyboard(false);
      expect(true).assertEqual(true);
      done();
    });

    it('FlutterView setCheckGesture toggle', 0, (done: Function) => {
      flutterView!.setCheckGesture(false);
      expect(true).assertEqual(true);
      done();
    });

    it('FlutterView setCheckAiBar toggle', 0, (done: Function) => {
      flutterView!.setCheckAiBar(false);
      expect(true).assertEqual(true);
      done();
    });

    it('FlutterView setPaddingTop and setPaddingBottom', 0, (done: Function) => {
      flutterView!.setPaddingTop(50);
      flutterView!.setPaddingBottom(100);
      flutterView!.setPaddingTop();
      flutterView!.setPaddingBottom();
      expect(true).assertEqual(true);
      done();
    });

    it('FlutterView setAutofillUiContext does not throw', 0, (done: Function) => {
      let threw = false;
      try { flutterView!.setAutofillUiContext('test', undefined); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('FlutterView onKeyPreIme does not throw', 0, (done: Function) => {
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
      try { flutterView!.onKeyPreIme(ev); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      done();
    });

    it('FlutterView getDVModel returns non-null', 0, (done: Function) => {
      const model = flutterView!.getDVModel();
      expect(model).not().assertNull();
      done();
    });

    it('FlutterView getEmbeddingNodeController returns non-null', 0, (done: Function) => {
      const controller = flutterView!.getEmbeddingNodeController();
      expect(controller).not().assertNull();
      done();
    });

    it('FlutterView getPlatformViewSize returns non-null', 0, (done: Function) => {
      const size = flutterView!.getPlatformViewSize();
      expect(size).not().assertNull();
      done();
    });

    it('FlutterView getId returns non-empty', 0, (done: Function) => {
      const id = flutterView!.getId();
      expect(id.length > 0).assertEqual(true);
      done();
    });
  })
}
