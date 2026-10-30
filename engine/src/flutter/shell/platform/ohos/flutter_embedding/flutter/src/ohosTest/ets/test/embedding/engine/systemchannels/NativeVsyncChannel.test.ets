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

  describe('NativeVsyncChannel.onMethodCall.isEnable', () => {
    it('handles isEnable with true (calls NAPI, may fail silently)', Level.LEVEL1, () => {
      const channel: NativeVsyncChannel = createChannel();
      const result: FakeMethodResult = new FakeMethodResult();
      const args: Map<string, Any> = new Map();
      args.set('isEnable', true);
      const call: MethodCall = new MethodCall('isEnable', args);
      // In test env, FlutterNapi.SetDVsyncSwitch may throw or be a no-op.
      // The source's success path doesn't call any result method - so we
      // just verify no throw.
      let threw = false;
      try {
        channel.onMethodCall(call, result);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('handles isEnable with false', Level.LEVEL1, () => {
      const channel: NativeVsyncChannel = createChannel();
      const result: FakeMethodResult = new FakeMethodResult();
      const args: Map<string, Any> = new Map();
      args.set('isEnable', false);
      const call: MethodCall = new MethodCall('isEnable', args);
      let threw = false;
      try {
        channel.onMethodCall(call, result);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('NativeVsyncChannel.onMethodCall.sendVelocity', () => {
    it('handles sendVelocity with translate type', Level.LEVEL1, () => {
      const channel: NativeVsyncChannel = createChannel();
      const result: FakeMethodResult = new FakeMethodResult();
      const args: Map<string, Any> = new Map();
      args.set('type', 'translate');
      args.set('velocity', 1.0);
      const call: MethodCall = new MethodCall('sendVelocity', args);
      let threw = false;
      try {
        channel.onMethodCall(call, result);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('handles sendVelocity with non-translate type (covers line 64 false branch)', Level.LEVEL1, () => {
      const channel: NativeVsyncChannel = createChannel();
      const result: FakeMethodResult = new FakeMethodResult();
      const args: Map<string, Any> = new Map();
      args.set('type', 'scale');
      args.set('velocity', 0.5);
      const call: MethodCall = new MethodCall('sendVelocity', args);
      let threw = false;
      try {
        channel.onMethodCall(call, result);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('NativeVsyncChannel.onMethodCall.checkLTPOSwitchStatus', () => {
    it('handles checkLTPOSwitchStatus (NAPI may return undefined)', Level.LEVEL1, () => {
      const channel: NativeVsyncChannel = createChannel();
      const result: FakeMethodResult = new FakeMethodResult();
      const call: MethodCall = new MethodCall('checkLTPOSwitchStatus', null);
      let threw = false;
      try {
        channel.onMethodCall(call, result);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });
}
