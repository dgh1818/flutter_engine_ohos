import { describe, it, expect, Level } from '@ohos/hypium';
import DartExecutor from '../../../../../../main/ets/embedding/engine/dart/DartExecutor';
import FlutterNapi from '../../../../../../main/ets/embedding/engine/FlutterNapi';
import LocalizationChannel, { LocalizationMessageHandler } from '../../../../../../main/ets/embedding/engine/systemchannels/LocalizationChannel';
import MethodCall from '../../../../../../main/ets/plugin/common/MethodCall';
import { MethodResult } from '../../../../../../main/ets/plugin/common/MethodChannel';
import Any from '../../../../../../main/ets/plugin/common/Any';
import resourceManager from '@ohos.resourceManager';

class FakeLocalizationHandler implements LocalizationMessageHandler {
  getStringResourceCalls: number = 0;
  lastKey: string = '';
  lastLocale: string = '';

  getStringResource(key: string, local: string): void {
    this.getStringResourceCalls++;
    this.lastKey = key;
    this.lastLocale = local;
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

function createChannel(): LocalizationChannel {
  const flutterNapi: FlutterNapi = new FlutterNapi();
  const context = getContext();
  const rm: resourceManager.ResourceManager = context.resourceManager;
  const dartExecutor: DartExecutor = new DartExecutor(flutterNapi, rm);
  return new LocalizationChannel(dartExecutor);
}

export default function localizationChannelTest() {
  describe('LocalizationChannel.construct', () => {
    it('constructsWithoutCrash', Level.LEVEL1, () => {
      const channel: LocalizationChannel = createChannel();
      expect(channel).not().assertNull();
    });
  });

  describe('LocalizationChannel.setLocalizationMessageHandler', () => {
    it('setsHandlerWithoutCrash', Level.LEVEL1, () => {
      const channel: LocalizationChannel = createChannel();
      const handler: FakeLocalizationHandler = new FakeLocalizationHandler();
      channel.setLocalizationMessageHandler(handler);
    });
  });

  describe('LocalizationChannel.onMethodCall.noHandler', () => {
    it('returnsEarlyWhenHandlerIsNull', Level.LEVEL1, () => {
      const channel: LocalizationChannel = createChannel();
      const result: FakeMethodResult = new FakeMethodResult();
      const args: Map<string, string> = new Map();
      args.set('key', 'test_key');
      const call: MethodCall = new MethodCall('Localization.getStringResource', args);
      channel.onMethodCall(call, result);
      expect(result.successCalls).assertEqual(0);
      expect(result.notImplCalls).assertEqual(0);
    });
  });

  describe('LocalizationChannel.onMethodCall.getStringResource', () => {
    it('dispatchesToHandlerWithKeyAndLocale', Level.LEVEL1, () => {
      const channel: LocalizationChannel = createChannel();
      const handler: FakeLocalizationHandler = new FakeLocalizationHandler();
      channel.setLocalizationMessageHandler(handler);
      const result: FakeMethodResult = new FakeMethodResult();
      const args: Map<string, string> = new Map();
      args.set('key', 'my_key');
      args.set('locale', 'en_US');
      const call: MethodCall = new MethodCall('Localization.getStringResource', args);
      channel.onMethodCall(call, result);
      expect(handler.getStringResourceCalls).assertEqual(1);
      expect(handler.lastKey).assertEqual('my_key');
      expect(handler.lastLocale).assertEqual('en_US');
      expect(result.successCalls).assertEqual(1);
    });

    it('dispatchesWithEmptyLocaleWhenNotProvided', Level.LEVEL1, () => {
      const channel: LocalizationChannel = createChannel();
      const handler: FakeLocalizationHandler = new FakeLocalizationHandler();
      channel.setLocalizationMessageHandler(handler);
      const result: FakeMethodResult = new FakeMethodResult();
      const args: Map<string, string> = new Map();
      args.set('key', 'only_key');
      const call: MethodCall = new MethodCall('Localization.getStringResource', args);
      channel.onMethodCall(call, result);
      expect(handler.getStringResourceCalls).assertEqual(1);
      expect(handler.lastKey).assertEqual('only_key');
      expect(handler.lastLocale).assertEqual('');
    });
  });

  describe('LocalizationChannel.onMethodCall.unknown', () => {
    it('callsNotImplementedForUnknownMethod', Level.LEVEL1, () => {
      const channel: LocalizationChannel = createChannel();
      const handler: FakeLocalizationHandler = new FakeLocalizationHandler();
      channel.setLocalizationMessageHandler(handler);
      const result: FakeMethodResult = new FakeMethodResult();
      const call: MethodCall = new MethodCall('unknown.method', null);
      channel.onMethodCall(call, result);
      expect(result.notImplCalls).assertEqual(1);
      expect(result.successCalls).assertEqual(0);
    });
  });
}
