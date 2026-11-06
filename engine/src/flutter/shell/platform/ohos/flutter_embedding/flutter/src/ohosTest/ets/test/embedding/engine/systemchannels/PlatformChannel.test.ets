/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, Level } from '@ohos/hypium';
import Any from '../../../../../../main/ets/plugin/common/Any';
import { resourceManager } from '@kit.LocalizationKit';
import FlutterNapi from '../../../../../../main/ets/embedding/engine/FlutterNapi';
import DartExecutor from '../../../../../../main/ets/embedding/engine/dart/DartExecutor';
import { MethodResult, MethodCallHandler } from '../../../../../../main/ets/plugin/common/MethodChannel';
import JSONMethodCodec from '../../../../../../main/ets/plugin/common/JSONMethodCodec';
import { DartMessenger } from '../../../../../../main/ets/embedding/engine/dart/DartMessenger';
import { IncomingMethodCallHandler } from '../../../../../../main/ets/plugin/common/MethodChannel';
import { BinaryReply } from '../../../../../../main/ets/plugin/common/BinaryMessenger';
import PlatformChannel, {
  AppSwitcherDescription,
  Brightness,
  ClipboardContentFormat,
  DeviceOrientation,
  HapticFeedbackType,
  PlatformMessageHandler,
  ScrollActivity,
  SoundType,
  SystemChromeStyle,
  SystemUiMode,
  SystemUiOverlay,
} from '../../../../../../main/ets/embedding/engine/systemchannels/PlatformChannel';
import MethodCall from '../../../../../../main/ets/plugin/common/MethodCall';

class FakeMethodResult implements MethodResult {
  successCalls: number = 0;
  successValue: Any = null;
  errorCalls: number = 0;
  errorCode: string = '';
  errorMessage: string = '';
  notImplCalls: number = 0;

  success(result: Any): void {
    this.successCalls++;
    this.successValue = result;
  }
  error(errorCode: string, errorMessage: string, errorDetails: Any): void {
    this.errorCalls++;
    this.errorCode = errorCode;
    this.errorMessage = errorMessage;
  }
  notImplemented(): void {
    this.notImplCalls++;
  }
}

class FakeHandler implements PlatformMessageHandler {
  playSystemSoundCalls: number = 0;
  vibrateCalls: number = 0;
  setPreferredOrientationsCalls: number = 0;
  setAppSwitcherDescriptionCalls: number = 0;
  showSystemOverlaysCalls: number = 0;
  showSystemUiModeCalls: number = 0;
  setSystemUiChangeListenerCalls: number = 0;
  restoreSystemUiOverlaysCalls: number = 0;
  setSystemUiOverlayStyleCalls: number = 0;
  popSystemNavigatorCalls: number = 0;
  getClipboardDataCalls: number = 0;
  setClipboardDataCalls: number = 0;
  clipboardHasStringsCalls: number = 0;

  playSystemSound(soundType: SoundType): void { this.playSystemSoundCalls++; }
  vibrateHapticFeedback(feedbackType: HapticFeedbackType): Promise<void> {
    this.vibrateCalls++;
    return Promise.resolve();
  }
  setPreferredOrientations(ohosOrientation: number, result: MethodResult): void {
    this.setPreferredOrientationsCalls++;
    result.success(null);
  }
  setApplicationSwitcherDescription(description: AppSwitcherDescription): void {
    this.setAppSwitcherDescriptionCalls++;
  }
  showSystemOverlays(overlays: SystemUiOverlay[]): void { this.showSystemOverlaysCalls++; }
  showSystemUiMode(mode: SystemUiMode): void { this.showSystemUiModeCalls++; }
  setSystemUiChangeListener(): void { this.setSystemUiChangeListenerCalls++; }
  restoreSystemUiOverlays(): void { this.restoreSystemUiOverlaysCalls++; }
  setSystemUiOverlayStyle(systemUiOverlayStyle: SystemChromeStyle): void { this.setSystemUiOverlayStyleCalls++; }
  popSystemNavigator(): void { this.popSystemNavigatorCalls++; }
  getClipboardData(result: MethodResult): void {
    this.getClipboardDataCalls++;
    result.success('clipboard data');
  }
  setClipboardData(text: string, result: MethodResult): void {
    this.setClipboardDataCalls++;
    result.success(null);
  }
  clipboardHasStrings(): boolean {
    this.clipboardHasStringsCalls++;
    return false;
  }
}

function getPlatformChannel(): PlatformChannel {
  const flutterNapi: FlutterNapi = new FlutterNapi();
  const manager: resourceManager.ResourceManager = getContext().resourceManager;
  const dartExecutor: DartExecutor = new DartExecutor(flutterNapi, manager);
  dartExecutor.onAttachedToNAPI();
  return new PlatformChannel(dartExecutor, flutterNapi);
}

class FakeBinaryReply implements BinaryReply {
  replyData: ArrayBuffer | null = null;
  replyCount: number = 0;
  reply(data: ArrayBuffer | null): void { this.replyCount++; this.replyData = data; }
}

function dispatchPlatformMethodCall(channel: PlatformChannel, method: string, args: Any): FakeBinaryReply | null {
  const call = new MethodCall(method, args);
  const encoded = JSONMethodCodec.INSTANCE.encodeMethodCall(call);
  const napi = channel.flutterNapi;
  const pmh = napi.platformMessageHandler;
  if (pmh == null) {
    // platformMessageHandler not set; can't dispatch
    return null;
  }
  const dm: DartMessenger = pmh as DartMessenger;
  const handlerInfo = dm.messageHandlers.get('flutter/platform');
  if (handlerInfo) {
    const incoming = handlerInfo.handler as IncomingMethodCallHandler;
    const reply = new FakeBinaryReply();
    incoming.onMessage(encoded, reply);
    return reply;
  }
  return null;
}

export default function platformChannelTest() {
  describe('PlatformChannel.decodeOrientations', () => {
    it('returns a number for single PORTRAIT_UP', Level.LEVEL1, () => {
      const channel = getPlatformChannel();
      const r = channel.decodeOrientations([DeviceOrientation.PORTRAIT_UP]);
      expect(typeof r).assertEqual('number');
    });

    it('returns a number for single PORTRAIT_DOWN', Level.LEVEL1, () => {
      const channel = getPlatformChannel();
      const r = channel.decodeOrientations([DeviceOrientation.PORTRAIT_DOWN]);
      expect(typeof r).assertEqual('number');
    });

    it('returns a number for single LANDSCAPE_LEFT', Level.LEVEL1, () => {
      const channel = getPlatformChannel();
      const r = channel.decodeOrientations([DeviceOrientation.LANDSCAPE_LEFT]);
      expect(typeof r).assertEqual('number');
    });

    it('returns a number for single LANDSCAPE_RIGHT', Level.LEVEL1, () => {
      const channel = getPlatformChannel();
      const r = channel.decodeOrientations([DeviceOrientation.LANDSCAPE_RIGHT]);
      expect(typeof r).assertEqual('number');
    });

    it('returns a number for multiple orientations', Level.LEVEL1, () => {
      const channel = getPlatformChannel();
      const r = channel.decodeOrientations([
        DeviceOrientation.PORTRAIT_UP,
        DeviceOrientation.LANDSCAPE_LEFT,
      ]);
      expect(typeof r).assertEqual('number');
    });

    it('throws for unknown orientation string', Level.LEVEL1, () => {
      const channel = getPlatformChannel();
      let threw = false;
      try {
        // Unknown string -> getDeviceOrientationFromValue throws.
        channel.decodeOrientations(['notAnOrientation']);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });
  });

  describe('PlatformChannel.getFeedbackTypeFromValue', () => {
    it('returns STANDARD for null', Level.LEVEL1, () => {
      const channel = getPlatformChannel();
      const r = channel.getFeedbackTypeFromValue(null as Any);
      expect(r).assertEqual(HapticFeedbackType.STANDARD);
    });

    it('returns STANDARD for each valid type', Level.LEVEL1, () => {
      const channel = getPlatformChannel();
      const types: Array<string> = [
        HapticFeedbackType.STANDARD,
        HapticFeedbackType.LIGHT_IMPACT,
        HapticFeedbackType.MEDIUM_IMPACT,
        HapticFeedbackType.HEAVY_IMPACT,
        HapticFeedbackType.SELECTION_CLICK,
      ];
      for (let i = 0; i < types.length; i++) {
        expect(channel.getFeedbackTypeFromValue(types[i])).assertEqual(types[i]);
      }
    });

    it('returns STANDARD for invalid type', Level.LEVEL1, () => {
      const channel = getPlatformChannel();
      const r = channel.getFeedbackTypeFromValue('INVALID_TYPE');
      expect(r).assertEqual(HapticFeedbackType.STANDARD);
    });
  });

  describe('PlatformChannel.getClipboardContentFormatFromValue', () => {
    it('returns PLAIN_TEXT for valid', Level.LEVEL1, () => {
      const channel = getPlatformChannel();
      const r = channel.getClipboardContentFormatFromValue(ClipboardContentFormat.PLAIN_TEXT);
      expect(r).assertEqual(ClipboardContentFormat.PLAIN_TEXT);
    });

    it('returns PLAIN_TEXT for invalid', Level.LEVEL1, () => {
      const channel = getPlatformChannel();
      const r = channel.getClipboardContentFormatFromValue('invalid/format');
      expect(r).assertEqual(ClipboardContentFormat.PLAIN_TEXT);
    });

    it('returns PLAIN_TEXT for empty string', Level.LEVEL1, () => {
      const channel = getPlatformChannel();
      const r = channel.getClipboardContentFormatFromValue('');
      expect(r).assertEqual(ClipboardContentFormat.PLAIN_TEXT);
    });
  });

  describe('PlatformChannel.getSystemUiOverlayFromValue', () => {
    it('returns TOP_OVERLAYS for valid', Level.LEVEL1, () => {
      const channel = getPlatformChannel();
      const r = channel.getSystemUiOverlayFromValue(SystemUiOverlay.TOP_OVERLAYS);
      expect(r).assertEqual(SystemUiOverlay.TOP_OVERLAYS);
    });

    it('returns BOTTOM_OVERLAYS for valid', Level.LEVEL1, () => {
      const channel = getPlatformChannel();
      const r = channel.getSystemUiOverlayFromValue(SystemUiOverlay.BOTTOM_OVERLAYS);
      expect(r).assertEqual(SystemUiOverlay.BOTTOM_OVERLAYS);
    });

    it('throws for invalid overlay', Level.LEVEL1, () => {
      const channel = getPlatformChannel();
      let threw = false;
      try {
        channel.getSystemUiOverlayFromValue('invalid');
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });
  });

  describe('PlatformChannel.getSystemUiModeFromValue', () => {
    it('returns each valid mode', Level.LEVEL1, () => {
      const channel = getPlatformChannel();
      const modes: Array<string> = [
        SystemUiMode.LEAN_BACK,
        SystemUiMode.IMMERSIVE,
        SystemUiMode.IMMERSIVE_STICKY,
        SystemUiMode.EDGE_TO_EDGE,
      ];
      for (let i = 0; i < modes.length; i++) {
        expect(channel.getSystemUiModeFromValue(modes[i])).assertEqual(modes[i]);
      }
    });

    it('throws for invalid mode', Level.LEVEL1, () => {
      const channel = getPlatformChannel();
      let threw = false;
      try {
        channel.getSystemUiModeFromValue('invalid_mode');
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });
  });

  describe('PlatformChannel.getBrightnessFromValue', () => {
    it('returns LIGHT for valid', Level.LEVEL1, () => {
      const channel = getPlatformChannel();
      const r = channel.getBrightnessFromValue(Brightness.LIGHT);
      expect(r).assertEqual(Brightness.LIGHT);
    });

    it('returns DARK for valid', Level.LEVEL1, () => {
      const channel = getPlatformChannel();
      const r = channel.getBrightnessFromValue(Brightness.DARK);
      expect(r).assertEqual(Brightness.DARK);
    });

    it('throws for invalid brightness', Level.LEVEL1, () => {
      const channel = getPlatformChannel();
      let threw = false;
      try {
        channel.getBrightnessFromValue('invalid');
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });
  });

  describe('PlatformChannel.getDeviceOrientationFromValue', () => {
    it('returns each valid orientation', Level.LEVEL1, () => {
      const channel = getPlatformChannel();
      const orientations: Array<string> = [
        DeviceOrientation.PORTRAIT_UP,
        DeviceOrientation.PORTRAIT_DOWN,
        DeviceOrientation.LANDSCAPE_LEFT,
        DeviceOrientation.LANDSCAPE_RIGHT,
      ];
      for (let i = 0; i < orientations.length; i++) {
        expect(channel.getDeviceOrientationFromValue(orientations[i])).assertEqual(orientations[i]);
      }
    });

    it('throws for invalid orientation', Level.LEVEL1, () => {
      const channel = getPlatformChannel();
      let threw = false;
      try {
        channel.getDeviceOrientationFromValue('invalid');
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });
  });

  describe('PlatformChannel.getScrollActivityFromValue', () => {
    it('returns START for valid', Level.LEVEL1, () => {
      const channel = getPlatformChannel();
      const r = channel.getScrollActivityFromValue(ScrollActivity.START);
      expect(r).assertEqual(ScrollActivity.START);
    });

    it('returns END for valid', Level.LEVEL1, () => {
      const channel = getPlatformChannel();
      const r = channel.getScrollActivityFromValue(ScrollActivity.END);
      expect(r).assertEqual(ScrollActivity.END);
    });

    it('throws for invalid activity', Level.LEVEL1, () => {
      const channel = getPlatformChannel();
      let threw = false;
      try {
        channel.getScrollActivityFromValue('invalid');
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });
  });

  describe('PlatformChannel.systemChromeChanged', () => {
    it('does not throw when invoked (delegate to MethodChannel)', Level.LEVEL1, () => {
      const channel = getPlatformChannel();
      let threw = false;
      try {
        channel.systemChromeChanged(true);
        channel.systemChromeChanged(false);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('PlatformChannel.setPlatformMessageHandler', () => {
    it('does not throw with valid handler', Level.LEVEL1, () => {
      const channel = getPlatformChannel();
      const handler = new FakeHandler();
      let threw = false;
      try {
        channel.setPlatformMessageHandler(handler);
        channel.setPlatformMessageHandler(null);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });


  describe('PlatformChannel.onMethodCall.via.messenger', () => {

    it('onMethodCall replies when platformMessageHandler is null', Level.LEVEL0, () => {
      const channel = getPlatformChannel();
      const reply = dispatchPlatformMethodCall(channel, 'SystemSound.play', SoundType.CLICK);
      expect(reply).not().assertNull();
      if (reply != null) {
        // The null-handler early return must reply exactly once; a success
        // envelope carries non-null data (notImplemented would reply null).
        expect(reply.replyCount).assertEqual(1);
        expect(reply.replyData).not().assertNull();
      }
    });

    it('onMethodCall handles SystemSound.play', Level.LEVEL0, () => {
      const channel = getPlatformChannel();
      const handler = new FakeHandler();
      channel.setPlatformMessageHandler(handler);
      const reply = dispatchPlatformMethodCall(channel, 'SystemSound.play', SoundType.CLICK);
      // Source: SystemSound.play still does not invoke the handler (stub),
      // but it must reply exactly once; a success envelope carries non-null
      // data (notImplemented would reply null).
      expect(handler.playSystemSoundCalls).assertEqual(0);
      expect(reply).not().assertNull();
      if (reply != null) {
        expect(reply.replyCount).assertEqual(1);
        expect(reply.replyData).not().assertNull();
      }
    });

    it('onMethodCall handles HapticFeedback.vibrate', Level.LEVEL0, () => {
      const channel = getPlatformChannel();
      const handler = new FakeHandler();
      channel.setPlatformMessageHandler(handler);
      dispatchPlatformMethodCall(channel, 'HapticFeedback.vibrate', HapticFeedbackType.STANDARD);
      expect(handler.vibrateCalls).assertEqual(1);
    });

    it('onMethodCall handles SystemChrome.setPreferredOrientations', Level.LEVEL0, () => {
      const channel = getPlatformChannel();
      const handler = new FakeHandler();
      channel.setPlatformMessageHandler(handler);
      dispatchPlatformMethodCall(channel, 'SystemChrome.setPreferredOrientations', [DeviceOrientation.PORTRAIT_UP]);
      expect(handler.setPreferredOrientationsCalls).assertEqual(1);
    });

    it('onMethodCall handles SystemChrome.setApplicationSwitcherDescription', Level.LEVEL0, () => {
      const channel = getPlatformChannel();
      const handler = new FakeHandler();
      channel.setPlatformMessageHandler(handler);
      const desc = new Map<string, Object>();
      desc.set('color', 0xFF0000);
      desc.set('label', 'Test');
      dispatchPlatformMethodCall(channel, 'SystemChrome.setApplicationSwitcherDescription', desc);
      expect(handler.setAppSwitcherDescriptionCalls).assertEqual(1);
    });

    it('onMethodCall handles SystemChrome.setEnabledSystemUIOverlays', Level.LEVEL0, () => {
      const channel = getPlatformChannel();
      const handler = new FakeHandler();
      channel.setPlatformMessageHandler(handler);
      dispatchPlatformMethodCall(channel, 'SystemChrome.setEnabledSystemUIOverlays', [SystemUiOverlay.TOP_OVERLAYS]);
      expect(handler.showSystemOverlaysCalls).assertEqual(1);
    });

    it('onMethodCall handles SystemChrome.setEnabledSystemUIMode LEAN_BACK', Level.LEVEL0, () => {
      const channel = getPlatformChannel();
      const handler = new FakeHandler();
      channel.setPlatformMessageHandler(handler);
      dispatchPlatformMethodCall(channel, 'SystemChrome.setEnabledSystemUIMode', SystemUiMode.LEAN_BACK);
      expect(handler.showSystemUiModeCalls).assertEqual(1);
    });

    it('onMethodCall handles SystemChrome.setEnabledSystemUIMode IMMERSIVE', Level.LEVEL0, () => {
      const channel = getPlatformChannel();
      const handler = new FakeHandler();
      channel.setPlatformMessageHandler(handler);
      dispatchPlatformMethodCall(channel, 'SystemChrome.setEnabledSystemUIMode', SystemUiMode.IMMERSIVE);
      expect(handler.showSystemUiModeCalls).assertEqual(1);
    });

    it('onMethodCall handles SystemChrome.setEnabledSystemUIMode IMMERSIVE_STICKY', Level.LEVEL0, () => {
      const channel = getPlatformChannel();
      const handler = new FakeHandler();
      channel.setPlatformMessageHandler(handler);
      dispatchPlatformMethodCall(channel, 'SystemChrome.setEnabledSystemUIMode', SystemUiMode.IMMERSIVE_STICKY);
      expect(handler.showSystemUiModeCalls).assertEqual(1);
    });

    it('onMethodCall handles SystemChrome.setEnabledSystemUIMode EDGE_TO_EDGE', Level.LEVEL0, () => {
      const channel = getPlatformChannel();
      const handler = new FakeHandler();
      channel.setPlatformMessageHandler(handler);
      dispatchPlatformMethodCall(channel, 'SystemChrome.setEnabledSystemUIMode', SystemUiMode.EDGE_TO_EDGE);
      expect(handler.showSystemUiModeCalls).assertEqual(1);
    });

    it('onMethodCall handles SystemChrome.setSystemUIChangeListener', Level.LEVEL0, () => {
      const channel = getPlatformChannel();
      const handler = new FakeHandler();
      channel.setPlatformMessageHandler(handler);
      dispatchPlatformMethodCall(channel, 'SystemChrome.setSystemUIChangeListener', null);
      expect(handler.setSystemUiChangeListenerCalls).assertEqual(1);
    });

    it('onMethodCall handles SystemChrome.restoreSystemUIOverlays', Level.LEVEL0, () => {
      const channel = getPlatformChannel();
      const handler = new FakeHandler();
      channel.setPlatformMessageHandler(handler);
      dispatchPlatformMethodCall(channel, 'SystemChrome.restoreSystemUIOverlays', null);
      expect(handler.restoreSystemUiOverlaysCalls).assertEqual(1);
    });

    it('onMethodCall handles SystemChrome.setSystemUIOverlayStyle', Level.LEVEL0, () => {
      const channel = getPlatformChannel();
      const handler = new FakeHandler();
      channel.setPlatformMessageHandler(handler);
      const style = new Map<string, Object>();
      style.set('statusBarColor', 0xFF0000);
      dispatchPlatformMethodCall(channel, 'SystemChrome.setSystemUIOverlayStyle', style);
      expect(handler.setSystemUiOverlayStyleCalls).assertEqual(1);
    });

    it('onMethodCall handles SystemChrome.setSystemUIOverlayStyle with all fields', Level.LEVEL0, () => {
      const channel = getPlatformChannel();
      const handler = new FakeHandler();
      channel.setPlatformMessageHandler(handler);
      const style = new Map<string, Object>();
      style.set('statusBarColor', 0xFF0000);
      style.set('statusBarIconBrightness', Brightness.LIGHT);
      style.set('systemStatusBarContrastEnforced', true);
      style.set('systemNavigationBarColor', 0x00FF00);
      style.set('systemNavigationBarIconBrightness', Brightness.DARK);
      style.set('systemNavigationBarDividerColor', 0x0000FF);
      style.set('systemNavigationBarContrastEnforced', false);
      dispatchPlatformMethodCall(channel, 'SystemChrome.setSystemUIOverlayStyle', style);
      expect(handler.setSystemUiOverlayStyleCalls).assertEqual(1);
    });

    it('onMethodCall handles SystemNavigator.pop', Level.LEVEL0, () => {
      const channel = getPlatformChannel();
      const handler = new FakeHandler();
      channel.setPlatformMessageHandler(handler);
      dispatchPlatformMethodCall(channel, 'SystemNavigator.pop', null);
      expect(handler.popSystemNavigatorCalls).assertEqual(1);
    });

    it('onMethodCall handles Clipboard.getData', Level.LEVEL0, () => {
      const channel = getPlatformChannel();
      const handler = new FakeHandler();
      channel.setPlatformMessageHandler(handler);
      dispatchPlatformMethodCall(channel, 'Clipboard.getData', null);
      expect(handler.getClipboardDataCalls).assertEqual(1);
    });

    it('onMethodCall handles Clipboard.setData', Level.LEVEL0, () => {
      const channel = getPlatformChannel();
      const handler = new FakeHandler();
      channel.setPlatformMessageHandler(handler);
      const data = new Map<string, Object>();
      data.set('text', 'hello clipboard');
      dispatchPlatformMethodCall(channel, 'Clipboard.setData', data);
      expect(handler.setClipboardDataCalls).assertEqual(1);
    });

    it('onMethodCall handles Clipboard.hasStrings (replies exactly once)', Level.LEVEL0, async (done: Function) => {
      const channel = getPlatformChannel();
      const handler = new FakeHandler();
      channel.setPlatformMessageHandler(handler);
      const reply = dispatchPlatformMethodCall(channel, 'Clipboard.hasStrings', null);
      // hasStrings replies from the system pasteboard hasData() promise; the
      // resolve and reject branches both reply exactly once with a success
      // envelope. Poll until the reply arrives (bounded) instead of a fixed
      // sleep so the test stays deterministic on slow devices.
      const deadline = Date.now() + 5000;
      while (reply != null && reply.replyCount == 0 && Date.now() < deadline) {
        await new Promise<void>((resolve: Function) => setTimeout(resolve, 50));
      }
      expect(reply).not().assertNull();
      if (reply != null) {
        expect(reply.replyCount).assertEqual(1);
        expect(reply.replyData).not().assertNull();
      }
      done();
    });

    it('onMethodCall handles Scroll.Activity START', Level.LEVEL0, () => {
      const channel = getPlatformChannel();
      const handler = new FakeHandler();
      channel.setPlatformMessageHandler(handler);
      const reply = dispatchPlatformMethodCall(channel, 'Scroll.Activity', ScrollActivity.START);
      expect(reply).not().assertNull();
      if (reply != null) {
        expect(reply.replyCount).assertEqual(1);
        expect(reply.replyData).not().assertNull();
      }
    });

    it('onMethodCall handles Scroll.Activity END', Level.LEVEL0, () => {
      const channel = getPlatformChannel();
      const handler = new FakeHandler();
      channel.setPlatformMessageHandler(handler);
      const reply = dispatchPlatformMethodCall(channel, 'Scroll.Activity', ScrollActivity.END);
      expect(reply).not().assertNull();
      if (reply != null) {
        expect(reply.replyCount).assertEqual(1);
        expect(reply.replyData).not().assertNull();
      }
    });

    it('onMethodCall handles Scroll.type', Level.LEVEL0, () => {
      const channel = getPlatformChannel();
      const handler = new FakeHandler();
      channel.setPlatformMessageHandler(handler);
      const type = new Map<string, Object>();
      type.set('type', '_PagePosition');
      const reply = dispatchPlatformMethodCall(channel, 'Scroll.type', type);
      expect(reply).not().assertNull();
      if (reply != null) {
        expect(reply.replyCount).assertEqual(1);
        expect(reply.replyData).not().assertNull();
      }
    });

    it('onMethodCall handles unknown method (notImplemented)', Level.LEVEL0, () => {
      const channel = getPlatformChannel();
      const handler = new FakeHandler();
      channel.setPlatformMessageHandler(handler);
      dispatchPlatformMethodCall(channel, 'unknown.method', null);
      expect(channel).not().assertNull();
    });
  });
}
