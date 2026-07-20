import { describe, it, expect, Level } from '@ohos/hypium';
import DartExecutor from '../../../../../../main/ets/embedding/engine/dart/DartExecutor';
import FlutterNapi from '../../../../../../main/ets/embedding/engine/FlutterNapi';
import KeyboardChannel, { KeyboardMethodHandler } from '../../../../../../main/ets/embedding/engine/systemchannels/KeyboardChannel';
import MethodCall from '../../../../../../main/ets/plugin/common/MethodCall';
import { MethodResult } from '../../../../../../main/ets/plugin/common/MethodChannel';
import Any from '../../../../../../main/ets/plugin/common/Any';
import resourceManager from '@ohos.resourceManager';

class FakeKeyboardHandler implements KeyboardMethodHandler {
  getKeyboardState(): Map<number, number> {
    const map = new Map<number, number>();
    map.set(1, 1);
    return map;
  }
}

class FakeMethodResult implements MethodResult {
  successResult: Any | null = null;
  successCalls: number = 0;
  errorCode: string = '';
  errorMessage: string = '';
  errorCalls: number = 0;
  notImplCalls: number = 0;

  success(result: Any): void {
    this.successCalls++;
    this.successResult = result;
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

function createChannel(): KeyboardChannel {
  const flutterNapi: FlutterNapi = new FlutterNapi();
  const context = getContext();
  const rm: resourceManager.ResourceManager = context.resourceManager;
  const dartExecutor: DartExecutor = new DartExecutor(flutterNapi, rm);
  return new KeyboardChannel(dartExecutor);
}

export default function keyboardChannelTest() {
  describe('KeyboardChannel.construct', () => {
    it('constructsWithoutCrash', Level.LEVEL1, () => {
      const channel: KeyboardChannel = createChannel();
      expect(channel).not().assertNull();
    });
  });

  describe('KeyboardChannel.setKeyboardMethodHandler', () => {
    it('setsAndRemovesHandlerWithoutCrash', Level.LEVEL1, () => {
      const channel: KeyboardChannel = createChannel();
      const handler: FakeKeyboardHandler = new FakeKeyboardHandler();
      channel.setKeyboardMethodHandler(handler);
      channel.setKeyboardMethodHandler(null);
    });
  });

  describe('KeyboardChannel.onMethodCall.noHandler', () => {
    it('returnsEarlyWhenHandlerIsNull', Level.LEVEL1, () => {
      const channel: KeyboardChannel = createChannel();
      const result: FakeMethodResult = new FakeMethodResult();
      const call: MethodCall = new MethodCall('getKeyboardState', null);
      channel.onMethodCall(call, result);
      expect(result.successCalls).assertEqual(0);
      expect(result.notImplCalls).assertEqual(0);
    });
  });

  describe('KeyboardChannel.onMethodCall.getKeyboardState', () => {
    it('dispatchesAndReturnsState', Level.LEVEL1, () => {
      const channel: KeyboardChannel = createChannel();
      const handler: FakeKeyboardHandler = new FakeKeyboardHandler();
      channel.setKeyboardMethodHandler(handler);
      const result: FakeMethodResult = new FakeMethodResult();
      const call: MethodCall = new MethodCall('getKeyboardState', null);
      channel.onMethodCall(call, result);
      expect(result.successCalls).assertEqual(1);
    });
  });

  describe('KeyboardChannel.onMethodCall.unknown', () => {
    it('callsNotImplementedForUnknownMethod', Level.LEVEL1, () => {
      const channel: KeyboardChannel = createChannel();
      const handler: FakeKeyboardHandler = new FakeKeyboardHandler();
      channel.setKeyboardMethodHandler(handler);
      const result: FakeMethodResult = new FakeMethodResult();
      const call: MethodCall = new MethodCall('unknown.method', null);
      channel.onMethodCall(call, result);
      expect(result.notImplCalls).assertEqual(1);
      expect(result.successCalls).assertEqual(0);
    });
  });
}
