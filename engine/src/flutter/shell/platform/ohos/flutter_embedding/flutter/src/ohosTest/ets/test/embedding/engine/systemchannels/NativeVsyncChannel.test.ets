import { describe, it, expect, Level } from '@ohos/hypium';
import DartExecutor from '../../../../../../main/ets/embedding/engine/dart/DartExecutor';
import FlutterNapi from '../../../../../../main/ets/embedding/engine/FlutterNapi';
import NativeVsyncChannel from '../../../../../../main/ets/embedding/engine/systemchannels/NativeVsyncChannel';
import MethodCall from '../../../../../../main/ets/plugin/common/MethodCall';
import { MethodResult } from '../../../../../../main/ets/plugin/common/MethodChannel';
import Any from '../../../../../../main/ets/plugin/common/Any';
import resourceManager from '@ohos.resourceManager';

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

function createChannel(): NativeVsyncChannel {
  const flutterNapi: FlutterNapi = new FlutterNapi();
  const context = getContext();
  const rm: resourceManager.ResourceManager = context.resourceManager;
  const dartExecutor: DartExecutor = new DartExecutor(flutterNapi, rm);
  return new NativeVsyncChannel(dartExecutor, flutterNapi);
}

export default function nativeVsyncChannelTest() {
  describe('NativeVsyncChannel.construct', () => {
    it('constructsWithoutCrash', Level.LEVEL1, () => {
      const channel: NativeVsyncChannel = createChannel();
      expect(channel).not().assertNull();
    });
  });

  describe('NativeVsyncChannel.onMethodCall.unknown', () => {
    it('callsNotImplementedForUnknownMethod', Level.LEVEL1, () => {
      const channel: NativeVsyncChannel = createChannel();
      const result: FakeMethodResult = new FakeMethodResult();
      const call: MethodCall = new MethodCall('unknown.method', null);
      channel.onMethodCall(call, result);
      expect(result.notImplCalls).assertEqual(1);
      expect(result.successCalls).assertEqual(0);
    });
  });
}
