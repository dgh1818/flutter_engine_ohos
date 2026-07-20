import { describe, it, expect, Level } from '@ohos/hypium';
import StatusBarClickChannel from '../../../../../../main/ets/embedding/engine/systemchannels/StatusBarClickChannel';
import { BinaryMessenger, BinaryMessageHandler, BinaryReply, TaskQueue, TaskQueueOptions } from '../../../../../../main/ets/plugin/common/BinaryMessenger';
import SendableBinaryMessageHandler from '../../../../../../main/ets/plugin/common/SendableBinaryMessageHandler';

class FakeTaskQueue implements TaskQueue {}

class FakeMessenger implements BinaryMessenger {
  sendChannel: string = '';
  sendMessage: ArrayBuffer | null = null;
  sendCount: number = 0;

  makeBackgroundTaskQueue(options?: TaskQueueOptions): TaskQueue {
    return new FakeTaskQueue();
  }

  send(channel: String, message: ArrayBuffer | null, callback?: BinaryReply | null): void {
    this.sendChannel = channel as string;
    this.sendMessage = message;
    this.sendCount++;
  }

  setMessageHandler(channel: String, handler: BinaryMessageHandler | SendableBinaryMessageHandler | null,
    taskQueue?: TaskQueue, ...args: Object[]): void {}
}

export default function statusBarClickChannelTest() {
  describe('StatusBarClickChannel.construct', () => {
    it('constructsWithFakeMessenger', Level.LEVEL0, () => {
      const messenger: FakeMessenger = new FakeMessenger();
      const channel: StatusBarClickChannel = new StatusBarClickChannel(messenger);
      expect(channel).not().assertNull();
    });
  });

  // sendClick() calls JSONMethodCodec.encodeMethodCall →
  // StringUtils.stringToArrayBuffer → flutter.nativeEncodeUtf8 (NAPI).
  // NAPI is in the codec encoding step, before messenger.send is called.
  // FakeMessenger cannot intercept it. Untestable without source changes.
}
