/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, Level } from '@ohos/hypium';
import MessageChannelUtils from '../../../../main/ets/util/MessageChannelUtils';
import {
  BinaryMessenger,
  BinaryMessageHandler,
  BinaryReply,
  TaskQueue,
  TaskQueueOptions
} from '../../../../main/ets/plugin/common/BinaryMessenger';
import BasicMessageChannel from '../../../../main/ets/plugin/common/BasicMessageChannel';

class MockBinaryMessenger implements BinaryMessenger {
  public lastChannel: String = '';
  public lastMessage: ArrayBuffer | null = null;
  public callCount: number = 0;

  makeBackgroundTaskQueue(options?: TaskQueueOptions): TaskQueue {
    return {};
  }

  send(channel: String, message: ArrayBuffer | null, callback?: BinaryReply | null): void {
    this.lastChannel = channel;
    this.lastMessage = message;
    this.callCount++;
  }

  setMessageHandler(channel: String, handler: BinaryMessageHandler | null,
    taskQueue?: TaskQueue, ...args: Object[]): void {
  }
}

function bufferToAsciiString(buf: ArrayBuffer): string {
  const bytes = new Uint8Array(buf);
  let s: string = '';
  for (let i = 0; i < bytes.length; i++) {
    s += String.fromCharCode(bytes[i]);
  }
  return s;
}

export default function messageChannelUtilsTest() {
  describe('MessageChannelUtils.resizeChannelBuffer', () => {
    it('sends to CHANNEL_BUFFERS_CHANNEL', Level.LEVEL1, () => {
      const messenger = new MockBinaryMessenger();
      MessageChannelUtils.resizeChannelBuffer(messenger, 'foo', 1024);
      expect(messenger.callCount).assertEqual(1);
      expect(String(messenger.lastChannel)).assertEqual(BasicMessageChannel.CHANNEL_BUFFERS_CHANNEL);
    });

    it('formats message as resize channel newSize', Level.LEVEL1, () => {
      const messenger = new MockBinaryMessenger();
      MessageChannelUtils.resizeChannelBuffer(messenger, 'foo', 1024);
      expect(bufferToAsciiString(messenger.lastMessage as ArrayBuffer)).assertEqual('resize\rfoo\r1024');
    });

    it('contains the channel name in the payload', Level.LEVEL1, () => {
      const messenger = new MockBinaryMessenger();
      MessageChannelUtils.resizeChannelBuffer(messenger, 'myChannel', 1024);
      expect(bufferToAsciiString(messenger.lastMessage as ArrayBuffer)).assertContain('myChannel');
    });

    it('contains the newSize in the payload', Level.LEVEL1, () => {
      const messenger = new MockBinaryMessenger();
      MessageChannelUtils.resizeChannelBuffer(messenger, 'foo', 2048);
      expect(bufferToAsciiString(messenger.lastMessage as ArrayBuffer)).assertContain('2048');
    });

    it('handles newSize of 0', Level.LEVEL1, () => {
      const messenger = new MockBinaryMessenger();
      MessageChannelUtils.resizeChannelBuffer(messenger, 'foo', 0);
      expect(bufferToAsciiString(messenger.lastMessage as ArrayBuffer)).assertEqual('resize\rfoo\r0');
    });

    it('handles negative newSize', Level.LEVEL1, () => {
      const messenger = new MockBinaryMessenger();
      MessageChannelUtils.resizeChannelBuffer(messenger, 'foo', -1);
      expect(bufferToAsciiString(messenger.lastMessage as ArrayBuffer)).assertEqual('resize\rfoo\r-1');
    });

    it('handles very large newSize', Level.LEVEL1, () => {
      const messenger = new MockBinaryMessenger();
      MessageChannelUtils.resizeChannelBuffer(messenger, 'foo', Number.MAX_SAFE_INTEGER);
      expect(bufferToAsciiString(messenger.lastMessage as ArrayBuffer))
        .assertEqual(`resize\rfoo\r${Number.MAX_SAFE_INTEGER}`);
    });

    it('handles empty channel name', Level.LEVEL1, () => {
      const messenger = new MockBinaryMessenger();
      MessageChannelUtils.resizeChannelBuffer(messenger, '', 1024);
      expect(bufferToAsciiString(messenger.lastMessage as ArrayBuffer)).assertEqual('resize\r\r1024');
    });

    it('increments callCount on each call', Level.LEVEL1, () => {
      const messenger = new MockBinaryMessenger();
      MessageChannelUtils.resizeChannelBuffer(messenger, 'foo', 1024);
      MessageChannelUtils.resizeChannelBuffer(messenger, 'bar', 2048);
      MessageChannelUtils.resizeChannelBuffer(messenger, 'baz', 4096);
      expect(messenger.callCount).assertEqual(3);
    });
  });
}
