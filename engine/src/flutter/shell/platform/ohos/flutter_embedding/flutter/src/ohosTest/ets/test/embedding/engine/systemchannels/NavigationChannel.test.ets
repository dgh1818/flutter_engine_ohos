import { describe, it, expect, Level } from '@ohos/hypium';
import DartExecutor from '../../../../../../main/ets/embedding/engine/dart/DartExecutor';
import FlutterNapi from '../../../../../../main/ets/embedding/engine/FlutterNapi';
import NavigationChannel, { NavigatorActivity, NavigatorStatus } from '../../../../../../main/ets/embedding/engine/systemchannels/NavigationChannel';
import { MethodCallHandler, MethodResult } from '../../../../../../main/ets/plugin/common/MethodChannel';
import Any from '../../../../../../main/ets/plugin/common/Any';
import resourceManager from '@ohos.resourceManager';

class DummyHandler implements MethodCallHandler {
  onMethodCall(call: Any, result: MethodResult): void {}
}

function createChannel(): NavigationChannel {
  const flutterNapi: FlutterNapi = new FlutterNapi();
  const context = getContext();
  const rm: resourceManager.ResourceManager = context.resourceManager;
  const dartExecutor: DartExecutor = new DartExecutor(flutterNapi, rm);
  return new NavigationChannel(dartExecutor);
}

export default function navigationChannelTest() {
  describe('NavigatorActivity', () => {
    it('PUSH_equals_push', Level.LEVEL0, () => {
      expect(NavigatorActivity.PUSH).assertEqual('push');
    });

    it('POP_equals_pop', Level.LEVEL0, () => {
      expect(NavigatorActivity.POP).assertEqual('pop');
    });
  });

  describe('NavigatorStatus', () => {
    it('START_equals_start', Level.LEVEL0, () => {
      expect(NavigatorStatus.START).assertEqual('start');
    });

    it('FINISH_equals_finish', Level.LEVEL0, () => {
      expect(NavigatorStatus.FINISH).assertEqual('finish');
    });
  });

  describe('NavigationChannel.construct', () => {
    it('constructsWithoutCrash', Level.LEVEL1, () => {
      const channel: NavigationChannel = createChannel();
      expect(channel).not().assertNull();
    });
  });

  describe('NavigationChannel.setMethodCallHandler', () => {
    it('setsCustomHandlerWithoutCrash', Level.LEVEL1, () => {
      const channel: NavigationChannel = createChannel();
      const handler: DummyHandler = new DummyHandler();
      channel.setMethodCallHandler(handler);
    });
  });
}
