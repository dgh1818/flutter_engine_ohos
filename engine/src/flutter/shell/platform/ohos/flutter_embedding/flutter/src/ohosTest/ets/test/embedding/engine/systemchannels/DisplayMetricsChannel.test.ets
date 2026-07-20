import { describe, it, expect, Level } from '@ohos/hypium';
import DartExecutor from '../../../../../../main/ets/embedding/engine/dart/DartExecutor';
import FlutterNapi from '../../../../../../main/ets/embedding/engine/FlutterNapi';
import DisplayMetricsChannel from '../../../../../../main/ets/embedding/engine/systemchannels/DisplayMetricsChannel';
import MethodCall from '../../../../../../main/ets/plugin/common/MethodCall';
import { MethodResult } from '../../../../../../main/ets/plugin/common/MethodChannel';
import Any from '../../../../../../main/ets/plugin/common/Any';
import resourceManager from '@ohos.resourceManager';
import { common } from '@kit.AbilityKit';

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

function createChannel(): DisplayMetricsChannel {
  const flutterNapi: FlutterNapi = new FlutterNapi();
  const context: common.Context = getContext();
  const rm: resourceManager.ResourceManager = context.resourceManager;
  const dartExecutor: DartExecutor = new DartExecutor(flutterNapi, rm);
  return new DisplayMetricsChannel(dartExecutor, context);
}

export default function displayMetricsChannelTest() {
  describe('DisplayMetricsChannel.construct', () => {
    it('constructsWithoutCrash', Level.LEVEL1, () => {
      const channel: DisplayMetricsChannel = createChannel();
      expect(channel).not().assertNull();
    });
  });

  describe('DisplayMetricsChannel.customDpiActive', () => {
    it('isFalseByDefault', Level.LEVEL0, () => {
      const channel: DisplayMetricsChannel = createChannel();
      expect(channel.isCustomDpiActive()).assertEqual(false);
    });

    it('setTrueThenReturnsTrue', Level.LEVEL0, () => {
      const channel: DisplayMetricsChannel = createChannel();
      channel.setCustomDpiActive(true);
      expect(channel.isCustomDpiActive()).assertEqual(true);
    });

    it('setFalseThenReturnsFalse', Level.LEVEL0, () => {
      const channel: DisplayMetricsChannel = createChannel();
      channel.setCustomDpiActive(true);
      channel.setCustomDpiActive(false);
      expect(channel.isCustomDpiActive()).assertEqual(false);
    });
  });

  describe('DisplayMetricsChannel.currentUri', () => {
    it('isEmptyByDefault', Level.LEVEL0, () => {
      const channel: DisplayMetricsChannel = createChannel();
      expect(channel.getCurrentUri()).assertEqual('');
    });

    it('setThenReturnsValue', Level.LEVEL0, () => {
      const channel: DisplayMetricsChannel = createChannel();
      channel.setCurrentUri('test_uri');
      expect(channel.getCurrentUri()).assertEqual('test_uri');
    });
  });

  describe('DisplayMetricsChannel.onMethodCall.updateDpiScale', () => {
    it('customDpiActiveWithUriReturnsEarlySuccess', Level.LEVEL1, () => {
      const channel: DisplayMetricsChannel = createChannel();
      channel.setCustomDpiActive(true);
      channel.setCurrentUri('custom_page');
      const result: FakeMethodResult = new FakeMethodResult();
      const args: Map<string, number> = new Map();
      args.set('dpiScale', 2.0);
      const call: MethodCall = new MethodCall('updateDpiScale', args);
      channel.onMethodCall(call, result);
      expect(result.successCalls).assertEqual(1);
      expect(result.successResult).assertEqual(true);
    });

    it('withoutCustomDpiEmitsEventAndReturnsSuccess', Level.LEVEL1, () => {
      const channel: DisplayMetricsChannel = createChannel();
      const result: FakeMethodResult = new FakeMethodResult();
      const args: Map<string, number> = new Map();
      args.set('dpiScale', 3.0);
      const call: MethodCall = new MethodCall('updateDpiScale', args);
      channel.onMethodCall(call, result);
      expect(result.successCalls).assertEqual(1);
    });
  });

  describe('DisplayMetricsChannel.onMethodCall.unknown', () => {
    it('callsNotImplementedForUnknownMethod', Level.LEVEL1, () => {
      const channel: DisplayMetricsChannel = createChannel();
      const result: FakeMethodResult = new FakeMethodResult();
      const call: MethodCall = new MethodCall('unknown.method', null);
      channel.onMethodCall(call, result);
      expect(result.notImplCalls).assertEqual(1);
    });
  });
}
