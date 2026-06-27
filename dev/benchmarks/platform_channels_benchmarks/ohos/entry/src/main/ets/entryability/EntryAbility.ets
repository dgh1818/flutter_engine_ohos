/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import {
  Any,
  BackgroundBasicMessageChannel,
  BasicMessageChannel,
  BinaryCodec,
  FlutterAbility,
  FlutterEngine,
  Reply,
  SendableMessageHandler,
  SendableStandardMessageCodec,
  StandardMessageCodec,
} from '@ohos/flutter_ohos';
import { GeneratedPluginRegistrant } from '../plugins/GeneratedPluginRegistrant';

export default class EntryAbility extends FlutterAbility {
  private byteBufferCache: ArrayBuffer | null = null;

  configureFlutterEngine(flutterEngine: FlutterEngine) {
    super.configureFlutterEngine(flutterEngine)
    GeneratedPluginRegistrant.registerWith(flutterEngine)

    const reset: BasicMessageChannel<Any> = new BasicMessageChannel(
      flutterEngine.getDartExecutor(),
      'dev.flutter.echo.reset',
      StandardMessageCodec.INSTANCE,
    );
    reset.setMessageHandler({
      onMessage: (message: Any, reply: Reply<Any>) => {
        this.byteBufferCache = null;
      },
    });

    const basicStandard: BasicMessageChannel<Any> = new BasicMessageChannel(
      flutterEngine.getDartExecutor(),
      'dev.flutter.echo.basic.standard',
      StandardMessageCodec.INSTANCE,
    );
    basicStandard.setMessageHandler({
      onMessage: (message: Any, reply: Reply<Any>) => reply.reply(message),
    });

    const basicBinary = new BasicMessageChannel<ArrayBuffer>(
      flutterEngine.getDartExecutor(),
      'dev.flutter.echo.basic.binary',
      BinaryCodec.INSTANCE_DIRECT,
    );
    basicBinary.setMessageHandler({
      onMessage: (message, reply) => {
        if (message == null) {
          reply.reply(message);
          return;
        }
        if (this.byteBufferCache == null) {
          const copy = new ArrayBuffer(message.byteLength);
          new Uint8Array(copy).set(new Uint8Array(message));
          this.byteBufferCache = copy;
        }
        reply.reply(this.byteBufferCache);
      },
    });

    const taskQueue = flutterEngine.getDartExecutor().makeBackgroundTaskQueue();
    const backgroundStandard: BackgroundBasicMessageChannel<Any> = new BackgroundBasicMessageChannel<Any>(
      flutterEngine.getDartExecutor(),
      'dev.flutter.echo.background.standard',
      SendableStandardMessageCodec.INSTANCE,
      taskQueue,
    );
    backgroundStandard.setMessageHandler(new BackgroundStandardHandler());
  }
}

@Sendable
class BackgroundStandardHandler implements SendableMessageHandler<Any> {
  onMessage(message: Any, reply: Reply<Any>): void {
    reply.reply(message);
  }
}
