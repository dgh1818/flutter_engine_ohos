import { describe, it, expect, Level } from '@ohos/hypium';
import DartExecutor from '../../../../../../main/ets/embedding/engine/dart/DartExecutor';
import FlutterNapi from '../../../../../../main/ets/embedding/engine/FlutterNapi';
import MouseCursorChannel, { MouseCursorMethodHandler } from '../../../../../../main/ets/embedding/engine/systemchannels/MouseCursorChannel';
import MethodCall from '../../../../../../main/ets/plugin/common/MethodCall';
import { MethodResult } from '../../../../../../main/ets/plugin/common/MethodChannel';
import Any from '../../../../../../main/ets/plugin/common/Any';
import resourceManager from '@ohos.resourceManager';
import HashMap from '@ohos.util.HashMap';

class FakeMouseCursorHandler implements MouseCursorMethodHandler {
  activateCalls: number = 0;
  lastKind: string = '';

  activateSystemCursor(kind: String): void {
    this.activateCalls++;
    this.lastKind = kind as string;
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

function createChannel(): MouseCursorChannel {
  const flutterNapi: FlutterNapi = new FlutterNapi();
  const context = getContext();
  const rm: resourceManager.ResourceManager = context.resourceManager;
  const dartExecutor: DartExecutor = new DartExecutor(flutterNapi, rm);
  return new MouseCursorChannel(dartExecutor);
}

export default function mouseCursorChannelTest() {
  describe('MouseCursorChannel.construct', () => {
    it('constructsWithoutCrash', Level.LEVEL1, () => {
      const channel: MouseCursorChannel = createChannel();
      expect(channel).not().assertNull();
    });
  });

  describe('MouseCursorChannel.setMethodHandler', () => {
    it('setsAndRemovesHandlerWithoutCrash', Level.LEVEL1, () => {
      const channel: MouseCursorChannel = createChannel();
      const handler: FakeMouseCursorHandler = new FakeMouseCursorHandler();
      channel.setMethodHandler(handler);
      channel.setMethodHandler(null);
    });
  });

  describe('MouseCursorChannel.onMethodCall.noHandler', () => {
    it('returnsEarlyWhenHandlerIsNull', Level.LEVEL1, () => {
      const channel: MouseCursorChannel = createChannel();
      const result: FakeMethodResult = new FakeMethodResult();
      const args: HashMap<string, string> = new HashMap();
      args.set('kind', 'pointer');
      const call: MethodCall = new MethodCall('activateSystemCursor', args);
      channel.onMethodCall(call, result);
      expect(result.successCalls).assertEqual(0);
      expect(result.notImplCalls).assertEqual(0);
    });
  });

  describe('MouseCursorChannel.onMethodCall.activateSystemCursor', () => {
    it('dispatchesToHandlerWithCorrectKind', Level.LEVEL1, () => {
      const channel: MouseCursorChannel = createChannel();
      const handler: FakeMouseCursorHandler = new FakeMouseCursorHandler();
      channel.setMethodHandler(handler);
      const result: FakeMethodResult = new FakeMethodResult();
      const args: HashMap<string, string> = new HashMap();
      args.set('kind', 'pointer');
      const call: MethodCall = new MethodCall('activateSystemCursor', args);
      channel.onMethodCall(call, result);
      expect(handler.activateCalls).assertEqual(1);
      expect(handler.lastKind).assertEqual('pointer');
      expect(result.successCalls).assertEqual(1);
      expect(result.successResult).assertEqual(true);
    });
  });

  describe('MouseCursorChannel.onMethodCall.unknown', () => {
    it('doesNothingForUnknownMethod', Level.LEVEL1, () => {
      const channel: MouseCursorChannel = createChannel();
      const handler: FakeMouseCursorHandler = new FakeMouseCursorHandler();
      channel.setMethodHandler(handler);
      const result: FakeMethodResult = new FakeMethodResult();
      const call: MethodCall = new MethodCall('unknown.method', null);
      channel.onMethodCall(call, result);
      expect(result.successCalls).assertEqual(0);
      expect(result.notImplCalls).assertEqual(0);
    });
  });

  describe('MouseCursorChannel.synthesizeMethodCall', () => {
    it('delegatesToOnMethodCall', Level.LEVEL1, () => {
      const channel: MouseCursorChannel = createChannel();
      const handler: FakeMouseCursorHandler = new FakeMouseCursorHandler();
      channel.setMethodHandler(handler);
      const result: FakeMethodResult = new FakeMethodResult();
      const args: HashMap<string, string> = new HashMap();
      args.set('kind', 'basic');
      const call: MethodCall = new MethodCall('activateSystemCursor', args);
      channel.synthesizeMethodCall(call, result);
      expect(handler.activateCalls).assertEqual(1);
      expect(handler.lastKind).assertEqual('basic');
      expect(result.successCalls).assertEqual(1);
    });
  });
}
