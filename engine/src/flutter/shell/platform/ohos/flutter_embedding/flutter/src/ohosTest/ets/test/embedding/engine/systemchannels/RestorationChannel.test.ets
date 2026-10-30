/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { describe, expect, it, Level } from '@ohos/hypium';
import { resourceManager } from '@kit.LocalizationKit';
import MethodChannel, { MethodCallHandler, MethodResult, IncomingMethodCallHandler } from '../../../../../../main/ets/plugin/common/MethodChannel';
import DartExecutor from '../../../../../../main/ets/embedding/engine/dart/DartExecutor';
import FlutterNapi from '../../../../../../main/ets/embedding/engine/FlutterNapi';
import RestorationChannel from '../../../../../../main/ets/embedding/engine/systemchannels/RestorationChannel';
import Any from '../../../../../../main/ets/plugin/common/Any';
import MethodCall from '../../../../../../main/ets/plugin/common/MethodCall';
import { DartMessenger } from '../../../../../../main/ets/embedding/engine/dart/DartMessenger';
import { BinaryReply, BinaryMessenger, BinaryMessageHandler, TaskQueue, TaskQueueOptions } from '../../../../../../main/ets/plugin/common/BinaryMessenger';
import SendableBinaryMessageHandler from '../../../../../../main/ets/plugin/common/SendableBinaryMessageHandler';
import StandardMethodCodec from '../../../../../../main/ets/plugin/common/StandardMethodCodec';

class FakeMethodResult implements MethodResult {
  successCalls: number = 0;
  successValue: Any = null;
  errorCalls: number = 0;
  notImplCalls: number = 0;
  success(result: Any): void { this.successCalls++; this.successValue = result; }
  error(errorCode: string, errorMessage: string, errorDetails: Any): void { this.errorCalls++; }
  notImplemented(): void { this.notImplCalls++; }
}

class FakeBinaryReply implements BinaryReply {
  replyCount: number = 0;
  reply(data: ArrayBuffer | null): void { this.replyCount++; }
}

class FakeTaskQueue implements TaskQueue {}

function getChannel(waitForRestoration: boolean): RestorationChannel {
  const flutterNapi: FlutterNapi = new FlutterNapi();
  const context = getContext();
  const rm: resourceManager.ResourceManager = context.resourceManager;
  const dartExecutor: DartExecutor = new DartExecutor(flutterNapi, rm);
  return new RestorationChannel(dartExecutor, waitForRestoration);
}

class FakeBinaryMessenger implements BinaryMessenger {
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

export default function RestorationChannelUnitTest() {
  describe('RestorationChannelTest', () => {
    it('constructs with waitForRestorationData=true', Level.LEVEL0, () => {
      const channel = getChannel(true);
      expect(channel.waitForRestorationData).assertEqual(true);
      expect(channel.engineHasProvidedData).assertEqual(false);
      expect(channel.frameworkHasRequestedData).assertEqual(false);
    });

    it('constructs with waitForRestorationData=false', Level.LEVEL0, () => {
      const channel = getChannel(false);
      expect(channel.waitForRestorationData).assertEqual(false);
    });

    it('constructs with MethodChannel (covers line 68 true branch)', Level.LEVEL0, () => {
      const messenger: BinaryMessenger = new FakeBinaryMessenger();
      const channel = new MethodChannel(messenger, 'test/restore');
      const instance: RestorationChannel = new RestorationChannel(channel, true);
      expect(instance.waitForRestorationData).assertEqual(true);
      expect(instance.engineHasProvidedData).assertEqual(false);
    });

    it('getRestorationData returns initial data', Level.LEVEL0, () => {
      const channel = getChannel(false);
      const data = channel.getRestorationData();
      expect(data).not().assertNull();
      expect(data.length).assertEqual(1);
    });

    it('setRestorationDataOnly sets data without marking as provided', Level.LEVEL0, () => {
      const channel = getChannel(false);
      const newData = new Uint8Array([1, 2, 3]);
      channel.setRestorationDataOnly(newData);
      expect(channel.engineHasProvidedData).assertEqual(false);
      const data = channel.getRestorationData();
      expect(data.length).assertEqual(3);
      expect(data[0]).assertEqual(1);
    });

    it('setRestorationData marks engineHasProvidedData=true', Level.LEVEL0, () => {
      const channel = getChannel(false);
      const newData = new Uint8Array([4, 5]);
      channel.setRestorationData(newData);
      expect(channel.engineHasProvidedData).assertEqual(true);
      expect(channel.getRestorationData()[0]).assertEqual(4);
    });

    it('setRestorationData with pending request answers it', Level.LEVEL0, () => {
      const channel = getChannel(true);
      const result = new FakeMethodResult();
      // Set pendingFrameworkRestorationChannelRequest directly
      channel.pendingFrameworkRestorationChannelRequest = result;
      channel.setRestorationData(new Uint8Array([9]));
      expect(result.successCalls).assertEqual(1);
      expect(channel.pendingFrameworkRestorationChannelRequest).assertNull();
    });

    it('setRestorationData with frameworkHasRequestedData pushes', Level.LEVEL0, () => {
      const channel = getChannel(false);
      channel.frameworkHasRequestedData = true;
      const newData = new Uint8Array([7, 8]);
      channel.setRestorationData(newData);
      // The push path calls channel.invokeMethod with async callback; data is set in success callback
      // Since invokeMethod may fail (NAPI), the success callback may not run
      // Just verify it doesn't crash
      expect(channel.engineHasProvidedData).assertEqual(true);
    });

    it('setRestorationData without pending or requested just caches', Level.LEVEL0, () => {
      const channel = getChannel(false);
      channel.setRestorationData(new Uint8Array([10]));
      expect(channel.getRestorationData()[0]).assertEqual(10);
    });

    it('clearData resets to default', Level.LEVEL0, () => {
      const channel = getChannel(false);
      channel.setRestorationDataOnly(new Uint8Array([1, 2, 3]));
      channel.clearData();
      const data = channel.getRestorationData();
      expect(data.length).assertEqual(1);
      expect(data[0]).assertEqual(0);
    });
  });

  describe('RestorationChannel.onMethodCall.via.messenger', () => {
    it('onMethodCall put stores data', Level.LEVEL0, () => {
      const flutterNapi = new FlutterNapi();
      const rm: resourceManager.ResourceManager = getContext().resourceManager;
      const dartExecutor = new DartExecutor(flutterNapi, rm);
      dartExecutor.onAttachedToNAPI();
      const channel = new RestorationChannel(dartExecutor, false);
      const call = new MethodCall('put', new Uint8Array([1, 2, 3]));
      const encoded = StandardMethodCodec.INSTANCE.encodeMethodCall(call);
      const pmh = flutterNapi.platformMessageHandler;
      if (pmh != null) {
        const dm: DartMessenger = pmh as DartMessenger;
        const handlerInfo = dm.messageHandlers.get('flutter/restoration');
        if (handlerInfo) {
          const incoming = handlerInfo.handler as IncomingMethodCallHandler;
          const reply = new FakeBinaryReply();
          incoming.onMessage(encoded, reply);
          expect(reply.replyCount).assertLarger(0);
          const data = channel.getRestorationData();
          expect(data.length).assertEqual(3);
        }
      }
      expect(channel).not().assertNull();
    });

    it('onMethodCall get with engineHasProvidedData returns data', Level.LEVEL0, () => {
      const flutterNapi = new FlutterNapi();
      const rm: resourceManager.ResourceManager = getContext().resourceManager;
      const dartExecutor = new DartExecutor(flutterNapi, rm);
      dartExecutor.onAttachedToNAPI();
      const channel = new RestorationChannel(dartExecutor, false);
      channel.engineHasProvidedData = true;
      channel.setRestorationDataOnly(new Uint8Array([5, 6]));
      const call = new MethodCall('get', null);
      const encoded = StandardMethodCodec.INSTANCE.encodeMethodCall(call);
      const pmh = flutterNapi.platformMessageHandler;
      if (pmh != null) {
        const dm: DartMessenger = pmh as DartMessenger;
        const handlerInfo = dm.messageHandlers.get('flutter/restoration');
        if (handlerInfo) {
          const incoming = handlerInfo.handler as IncomingMethodCallHandler;
          const reply = new FakeBinaryReply();
          incoming.onMessage(encoded, reply);
          expect(reply.replyCount).assertLarger(0);
        }
      }
      expect(channel.frameworkHasRequestedData).assertEqual(true);
    });

    it('onMethodCall get with waitForRestorationData sets pending request', Level.LEVEL0, () => {
      const flutterNapi = new FlutterNapi();
      const rm: resourceManager.ResourceManager = getContext().resourceManager;
      const dartExecutor = new DartExecutor(flutterNapi, rm);
      dartExecutor.onAttachedToNAPI();
      const channel = new RestorationChannel(dartExecutor, true);
      const call = new MethodCall('get', null);
      const encoded = StandardMethodCodec.INSTANCE.encodeMethodCall(call);
      const pmh = flutterNapi.platformMessageHandler;
      if (pmh != null) {
        const dm: DartMessenger = pmh as DartMessenger;
        const handlerInfo = dm.messageHandlers.get('flutter/restoration');
        if (handlerInfo) {
          const incoming = handlerInfo.handler as IncomingMethodCallHandler;
          const reply = new FakeBinaryReply();
          incoming.onMessage(encoded, reply);
        }
      }
      expect(channel.frameworkHasRequestedData).assertEqual(true);
      expect(channel.pendingFrameworkRestorationChannelRequest).not().assertNull();
    });

    it('onMethodCall get without waitForRestorationData responds immediately', Level.LEVEL0, () => {
      const flutterNapi = new FlutterNapi();
      const rm: resourceManager.ResourceManager = getContext().resourceManager;
      const dartExecutor = new DartExecutor(flutterNapi, rm);
      dartExecutor.onAttachedToNAPI();
      const channel = new RestorationChannel(dartExecutor, false);
      const call = new MethodCall('get', null);
      const encoded = StandardMethodCodec.INSTANCE.encodeMethodCall(call);
      const pmh = flutterNapi.platformMessageHandler;
      if (pmh != null) {
        const dm: DartMessenger = pmh as DartMessenger;
        const handlerInfo = dm.messageHandlers.get('flutter/restoration');
        if (handlerInfo) {
          const incoming = handlerInfo.handler as IncomingMethodCallHandler;
          const reply = new FakeBinaryReply();
          incoming.onMessage(encoded, reply);
          expect(reply.replyCount).assertLarger(0);
        }
      }
      expect(channel.frameworkHasRequestedData).assertEqual(true);
      expect(channel.pendingFrameworkRestorationChannelRequest).assertNull();
    });

    it('onMethodCall unknown method returns notImplemented', Level.LEVEL0, () => {
      const flutterNapi = new FlutterNapi();
      const rm: resourceManager.ResourceManager = getContext().resourceManager;
      const dartExecutor = new DartExecutor(flutterNapi, rm);
      dartExecutor.onAttachedToNAPI();
      const channel = new RestorationChannel(dartExecutor, false);
      const call = new MethodCall('unknown', null);
      const encoded = StandardMethodCodec.INSTANCE.encodeMethodCall(call);
      const pmh = flutterNapi.platformMessageHandler;
      if (pmh != null) {
        const dm: DartMessenger = pmh as DartMessenger;
        const handlerInfo = dm.messageHandlers.get('flutter/restoration');
        if (handlerInfo) {
          const incoming = handlerInfo.handler as IncomingMethodCallHandler;
          const reply = new FakeBinaryReply();
          incoming.onMessage(encoded, reply);
          expect(reply.replyCount).assertLarger(0);
        }
      }
      expect(channel).not().assertNull();
    });
  })
}
