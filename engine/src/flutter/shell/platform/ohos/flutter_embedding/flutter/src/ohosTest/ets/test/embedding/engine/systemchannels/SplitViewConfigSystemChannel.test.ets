import { describe, it, expect, Level } from '@ohos/hypium';
import DartExecutor from '../../../../../../main/ets/embedding/engine/dart/DartExecutor';
import FlutterNapi from '../../../../../../main/ets/embedding/engine/FlutterNapi';
import SplitViewConfigSystemChannel from '../../../../../../main/ets/embedding/engine/systemchannels/SplitViewConfigSystemChannel';
import resourceManager from '@ohos.resourceManager';
import { common } from '@kit.AbilityKit';

function createChannel(): SplitViewConfigSystemChannel {
  const flutterNapi: FlutterNapi = new FlutterNapi();
  const context: common.Context = getContext();
  const rm: resourceManager.ResourceManager = context.resourceManager;
  const dartExecutor: DartExecutor = new DartExecutor(flutterNapi, rm);
  return new SplitViewConfigSystemChannel(dartExecutor, context);
}

export default function splitViewConfigSystemChannelTest() {
  describe('SplitViewConfigSystemChannel.construct', () => {
    it('constructsWithoutCrash', Level.LEVEL1, () => {
      const channel: SplitViewConfigSystemChannel = createChannel();
      expect(channel).not().assertNull();
    });
  });

  describe('SplitViewConfigSystemChannel.sendConfigToFlutter', () => {
    it('returnsEarlyWhenRawfileNotExist', Level.LEVEL1, () => {
      const channel: SplitViewConfigSystemChannel = createChannel();
      channel.sendConfigToFlutter();
    });
  });
}
