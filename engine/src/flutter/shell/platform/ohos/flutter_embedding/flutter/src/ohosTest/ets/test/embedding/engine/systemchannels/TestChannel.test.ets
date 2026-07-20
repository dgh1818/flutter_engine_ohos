import { describe, it, expect, Level } from '@ohos/hypium';
import DartExecutor from '../../../../../../main/ets/embedding/engine/dart/DartExecutor';
import FlutterNapi from '../../../../../../main/ets/embedding/engine/FlutterNapi';
import TestChannel from '../../../../../../main/ets/embedding/engine/systemchannels/TestChannel';
import resourceManager from '@ohos.resourceManager';

export default function testChannelTest() {
  describe('TestChannel.construct', () => {
    it('constructsWithoutCrash', Level.LEVEL1, () => {
      const flutterNapi: FlutterNapi = new FlutterNapi();
      const context = getContext();
      const rm: resourceManager.ResourceManager = context.resourceManager;
      const dartExecutor: DartExecutor = new DartExecutor(flutterNapi, rm);
      const channel: TestChannel = new TestChannel(dartExecutor);
      expect(channel).not().assertNull();
    });
  });
}
