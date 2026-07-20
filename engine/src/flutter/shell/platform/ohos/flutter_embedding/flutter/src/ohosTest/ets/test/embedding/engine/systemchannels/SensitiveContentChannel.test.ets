import { describe, it, expect, Level } from '@ohos/hypium';
import DartExecutor from '../../../../../../main/ets/embedding/engine/dart/DartExecutor';
import FlutterNapi from '../../../../../../main/ets/embedding/engine/FlutterNapi';
import SensitiveContentChannel, {
  SENSITIVE_CONTENT_SENSITIVITY,
  NOT_SENSITIVE_CONTENT_SENSITIVITY,
  SensitiveContentMethodHandler,
} from '../../../../../../main/ets/embedding/engine/systemchannels/SensitiveContentChannel';
import MethodCall from '../../../../../../main/ets/plugin/common/MethodCall';
import { MethodResult } from '../../../../../../main/ets/plugin/common/MethodChannel';
import Any from '../../../../../../main/ets/plugin/common/Any';
import resourceManager from '@ohos.resourceManager';

class FakeSensitiveContentHandler implements SensitiveContentMethodHandler {
  setContentCalls: number = 0;
  lastContentSensitivity: number = -1;
  getContentResult: number = NOT_SENSITIVE_CONTENT_SENSITIVITY;
  isSupportedResult: boolean = true;

  setContentSensitivity(requestedContentSensitivity: number): void {
    this.setContentCalls++;
    this.lastContentSensitivity = requestedContentSensitivity;
  }

  getContentSensitivity(): number {
    return this.getContentResult;
  }

  isSupported(): boolean {
    return this.isSupportedResult;
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

function createChannel(): SensitiveContentChannel {
  const flutterNapi: FlutterNapi = new FlutterNapi();
  const context = getContext();
  const rm: resourceManager.ResourceManager = context.resourceManager;
  const dartExecutor: DartExecutor = new DartExecutor(flutterNapi, rm);
  return new SensitiveContentChannel(dartExecutor);
}

export default function sensitiveContentChannelTest() {
  describe('SensitiveContentChannel.constants', () => {
    it('SENSITIVE_CONTENT_SENSITIVITY_equals_1', Level.LEVEL0, () => {
      expect(SENSITIVE_CONTENT_SENSITIVITY).assertEqual(1);
    });

    it('NOT_SENSITIVE_CONTENT_SENSITIVITY_equals_2', Level.LEVEL0, () => {
      expect(NOT_SENSITIVE_CONTENT_SENSITIVITY).assertEqual(2);
    });
  });

  describe('SensitiveContentChannel.construct', () => {
    it('constructsWithoutCrash', Level.LEVEL1, () => {
      const channel: SensitiveContentChannel = createChannel();
      expect(channel).not().assertNull();
    });
  });

  describe('SensitiveContentChannel.setSensitiveContentMethodHandler', () => {
    it('setsHandlerWithoutCrash', Level.LEVEL1, () => {
      const channel: SensitiveContentChannel = createChannel();
      const handler: FakeSensitiveContentHandler = new FakeSensitiveContentHandler();
      channel.setSensitiveContentMethodHandler(handler);
      channel.setSensitiveContentMethodHandler(null);
    });
  });

  describe('SensitiveContentChannel.onMethodCall.noHandler', () => {
    it('returnsEarlyWhenHandlerIsNull', Level.LEVEL1, () => {
      const channel: SensitiveContentChannel = createChannel();
      const result: FakeMethodResult = new FakeMethodResult();
      const call: MethodCall = new MethodCall('SensitiveContent.isSupported', null);
      channel.onMethodCall(call, result);
      expect(result.successCalls).assertEqual(0);
      expect(result.notImplCalls).assertEqual(0);
    });
  });

  describe('SensitiveContentChannel.onMethodCall.isSupported', () => {
    it('dispatchesIsSupportedAndReturnsTrue', Level.LEVEL1, () => {
      const channel: SensitiveContentChannel = createChannel();
      const handler: FakeSensitiveContentHandler = new FakeSensitiveContentHandler();
      handler.isSupportedResult = true;
      channel.setSensitiveContentMethodHandler(handler);
      const result: FakeMethodResult = new FakeMethodResult();
      const call: MethodCall = new MethodCall('SensitiveContent.isSupported', null);
      channel.onMethodCall(call, result);
      expect(result.successCalls).assertEqual(1);
      expect(result.successResult).assertEqual(true);
    });

    it('dispatchesIsSupportedAndReturnsFalse', Level.LEVEL1, () => {
      const channel: SensitiveContentChannel = createChannel();
      const handler: FakeSensitiveContentHandler = new FakeSensitiveContentHandler();
      handler.isSupportedResult = false;
      channel.setSensitiveContentMethodHandler(handler);
      const result: FakeMethodResult = new FakeMethodResult();
      const call: MethodCall = new MethodCall('SensitiveContent.isSupported', null);
      channel.onMethodCall(call, result);
      expect(result.successCalls).assertEqual(1);
      expect(result.successResult).assertEqual(false);
    });
  });

  describe('SensitiveContentChannel.onMethodCall.setContentSensitivity', () => {
    it('dispatchesSetContentSensitivitySensitive', Level.LEVEL1, () => {
      const channel: SensitiveContentChannel = createChannel();
      const handler: FakeSensitiveContentHandler = new FakeSensitiveContentHandler();
      channel.setSensitiveContentMethodHandler(handler);
      const result: FakeMethodResult = new FakeMethodResult();
      const call: MethodCall = new MethodCall('SensitiveContent.setContentSensitivity', SENSITIVE_CONTENT_SENSITIVITY);
      channel.onMethodCall(call, result);
      expect(handler.setContentCalls).assertEqual(1);
      expect(handler.lastContentSensitivity).assertEqual(SENSITIVE_CONTENT_SENSITIVITY);
      expect(result.successCalls).assertEqual(1);
    });

    it('dispatchesSetContentSensitivityNotSensitive', Level.LEVEL1, () => {
      const channel: SensitiveContentChannel = createChannel();
      const handler: FakeSensitiveContentHandler = new FakeSensitiveContentHandler();
      channel.setSensitiveContentMethodHandler(handler);
      const result: FakeMethodResult = new FakeMethodResult();
      const call: MethodCall = new MethodCall('SensitiveContent.setContentSensitivity', NOT_SENSITIVE_CONTENT_SENSITIVITY);
      channel.onMethodCall(call, result);
      expect(handler.setContentCalls).assertEqual(1);
      expect(handler.lastContentSensitivity).assertEqual(NOT_SENSITIVE_CONTENT_SENSITIVITY);
      expect(result.successCalls).assertEqual(1);
    });

    it('returnsErrorForInvalidSensitivityValue', Level.LEVEL1, () => {
      const channel: SensitiveContentChannel = createChannel();
      const handler: FakeSensitiveContentHandler = new FakeSensitiveContentHandler();
      channel.setSensitiveContentMethodHandler(handler);
      const result: FakeMethodResult = new FakeMethodResult();
      const call: MethodCall = new MethodCall('SensitiveContent.setContentSensitivity', 999);
      channel.onMethodCall(call, result);
      expect(result.errorCalls).assertEqual(1);
      expect(handler.setContentCalls).assertEqual(0);
    });
  });

  describe('SensitiveContentChannel.onMethodCall.getContentSensitivity', () => {
    it('returnsSerializedSensitiveValue', Level.LEVEL1, () => {
      const channel: SensitiveContentChannel = createChannel();
      const handler: FakeSensitiveContentHandler = new FakeSensitiveContentHandler();
      handler.getContentResult = SENSITIVE_CONTENT_SENSITIVITY;
      channel.setSensitiveContentMethodHandler(handler);
      const result: FakeMethodResult = new FakeMethodResult();
      const call: MethodCall = new MethodCall('SensitiveContent.getContentSensitivity', null);
      channel.onMethodCall(call, result);
      expect(result.successCalls).assertEqual(1);
      expect(result.successResult).assertEqual(SENSITIVE_CONTENT_SENSITIVITY);
    });

    it('returnsSerializedNotSensitiveValue', Level.LEVEL1, () => {
      const channel: SensitiveContentChannel = createChannel();
      const handler: FakeSensitiveContentHandler = new FakeSensitiveContentHandler();
      handler.getContentResult = NOT_SENSITIVE_CONTENT_SENSITIVITY;
      channel.setSensitiveContentMethodHandler(handler);
      const result: FakeMethodResult = new FakeMethodResult();
      const call: MethodCall = new MethodCall('SensitiveContent.getContentSensitivity', null);
      channel.onMethodCall(call, result);
      expect(result.successCalls).assertEqual(1);
      expect(result.successResult).assertEqual(NOT_SENSITIVE_CONTENT_SENSITIVITY);
    });
  });

  describe('SensitiveContentChannel.onMethodCall.unknown', () => {
    it('callsNotImplementedForUnknownMethod', Level.LEVEL1, () => {
      const channel: SensitiveContentChannel = createChannel();
      const handler: FakeSensitiveContentHandler = new FakeSensitiveContentHandler();
      channel.setSensitiveContentMethodHandler(handler);
      const result: FakeMethodResult = new FakeMethodResult();
      const call: MethodCall = new MethodCall('unknown.method', null);
      channel.onMethodCall(call, result);
      expect(result.notImplCalls).assertEqual(1);
      expect(result.successCalls).assertEqual(0);
    });
  });
}
