/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, expect, it } from '@ohos/hypium';
import PlatformViewsChannelHybrid, {
  PlatformViewsHybridHandler
} from '../../../../../../main/ets/embedding/engine/systemchannels/PlatformViewsChannelHybrid';
import {
  PlatformViewCreationRequest,
  PlatformViewTouch
} from '../../../../../../main/ets/embedding/engine/systemchannels/PlatformViewsChannel';
import MethodCall from '../../../../../../main/ets/plugin/common/MethodCall';
import { BinaryMessageHandler, BinaryReply } from '../../../../../../main/ets/plugin/common/BinaryMessenger';
import { MethodResult } from '../../../../../../main/ets/plugin/common/MethodChannel';
import Any from '../../../../../../main/ets/plugin/common/Any';
import StandardMethodCodec from '../../../../../../main/ets/plugin/common/StandardMethodCodec';
import DartExecutor from '../../../../../../main/ets/embedding/engine/dart/DartExecutor';
import { ByteBuffer } from '../../../../../../main/ets/util/ByteBuffer';

class RecordingHybridHandler implements PlatformViewsHybridHandler {
  lastCreate: PlatformViewCreationRequest | null = null;
  lastDisposeId: number = -1;
  lastTouch: PlatformViewTouch | null = null;
  lastDirectionViewId: number = -1;
  lastDirection: Direction = Direction.Ltr;
  lastClearFocusId: number = -1;
  lastOverscrollViewId: number = -1;
  lastOverscrollOffset: number = 0;
  surfaceControlEnabled: boolean = true;
  throwOnCreate: boolean = false;
  throwOnDispose: boolean = false;
  throwOnTouch: boolean = false;
  throwOnSetDirection: boolean = false;
  throwOnClearFocus: boolean = false;
  throwOnOverscroll: boolean = false;
  throwOnSurfaceControl: boolean = false;

  createForPlatformViewLayer(request: PlatformViewCreationRequest): void {
    if (this.throwOnCreate) {
      throw new Error('create failed');
    }
    this.lastCreate = request;
  }

  dispose(viewId: number): void {
    if (this.throwOnDispose) {
      throw new Error('dispose failed');
    }
    this.lastDisposeId = viewId;
  }

  onTouch(touch: PlatformViewTouch): void {
    if (this.throwOnTouch) {
      throw new Error('touch failed');
    }
    this.lastTouch = touch;
  }

  setDirection(viewId: number, direction: Direction): void {
    if (this.throwOnSetDirection) {
      throw new Error('setDirection failed');
    }
    this.lastDirectionViewId = viewId;
    this.lastDirection = direction;
  }

  clearFocus(viewId: number): void {
    if (this.throwOnClearFocus) {
      throw new Error('clearFocus failed');
    }
    this.lastClearFocusId = viewId;
  }

  isSurfaceControlEnabled(): boolean {
    if (this.throwOnSurfaceControl) {
      throw new Error('isSurfaceControlEnabled failed');
    }
    return this.surfaceControlEnabled;
  }

  setOverscrollOffset(viewId: number, offsetVp: number): void {
    if (this.throwOnOverscroll) {
      throw new Error('setOverscrollOffset failed');
    }
    this.lastOverscrollViewId = viewId;
    this.lastOverscrollOffset = offsetVp;
  }
}

class RecordingMethodResult implements MethodResult {
  successValue: Any = null;
  successCount: number = 0;
  errorCode: string = '';
  errorMessage: string = '';
  notImplementedCount: number = 0;

  success(result: Any): void {
    this.successCount++;
    this.successValue = result;
  }

  error(errorCode: string, errorMessage: string, errorDetails: Any): void {
    this.errorCode = errorCode;
    this.errorMessage = errorMessage;
  }

  notImplemented(): void {
    this.notImplementedCount++;
  }
}

class CapturingBinaryMessenger {
  handlers: Map<string, BinaryMessageHandler> = new Map();

  send(channel: String, message: ArrayBuffer | null, callback?: Object | null): void {
  }

  setMessageHandler(channel: String, handler: BinaryMessageHandler | null): void {
    const key: string = channel.toString();
    if (handler == null) {
      this.handlers.delete(key);
    } else {
      this.handlers.set(key, handler);
    }
  }

  makeBackgroundTaskQueue(): Object {
    return new Object();
  }
}

class RecordingBinaryReply implements BinaryReply {
  replyBuffer: ArrayBuffer | null = null;
  replyCount: number = 0;

  reply(reply: ArrayBuffer | null): void {
    this.replyCount++;
    this.replyBuffer = reply;
  }
}

function makeCreateArgs(id: number, viewType: string, direction: number): Map<string, Object> {
  const args: Map<string, Object> = new Map();
  args.set('id', id);
  args.set('viewType', viewType);
  args.set('direction', direction);
  return args;
}

function makeChannelWithMessenger(handler: PlatformViewsHybridHandler | null):
  [PlatformViewsChannelHybrid, CapturingBinaryMessenger] {
  const messenger: CapturingBinaryMessenger = new CapturingBinaryMessenger();
  const channel: PlatformViewsChannelHybrid =
    new PlatformViewsChannelHybrid(messenger as Any as DartExecutor);
  channel.setPlatformViewsHandler(handler);
  return [channel, messenger];
}

function makeChannel(handler: PlatformViewsHybridHandler | null): PlatformViewsChannelHybrid {
  return makeChannelWithMessenger(handler)[0];
}

function dispatchBinary(
  messenger: CapturingBinaryMessenger,
  method: string,
  args: Any
): RecordingBinaryReply {
  const binaryHandler: BinaryMessageHandler | undefined =
    messenger.handlers.get('flutter/platform_views_2');
  expect(binaryHandler !== undefined).assertEqual(true);
  const encoded: ArrayBuffer =
    StandardMethodCodec.INSTANCE.encodeMethodCall(new MethodCall(method, args));
  const reply: RecordingBinaryReply = new RecordingBinaryReply();
  binaryHandler!.onMessage(encoded, reply);
  return reply;
}

export default function PlatformViewsChannelHybridTest() {
  describe('PlatformViewsChannelHybrid', () => {
    it('create_ltr_builds_request_with_zero_geometry', 0, () => {
      const handler: RecordingHybridHandler = new RecordingHybridHandler();
      const channel: PlatformViewsChannelHybrid = makeChannel(handler);
      const result: RecordingMethodResult = new RecordingMethodResult();
      channel.create(new MethodCall('create', makeCreateArgs(11, 'demo/view', 0)), result);

      expect(handler.lastCreate).not().assertNull();
      expect(handler.lastCreate!.viewId).assertEqual(11);
      expect(handler.lastCreate!.viewType).assertEqual('demo/view');
      expect(handler.lastCreate!.logicalTop).assertEqual(0);
      expect(handler.lastCreate!.logicalLeft).assertEqual(0);
      expect(handler.lastCreate!.logicalWidth).assertEqual(0);
      expect(handler.lastCreate!.logicalHeight).assertEqual(0);
      expect(handler.lastCreate!.direction).assertEqual(Direction.Ltr);
      expect(result.successCount).assertEqual(1);
    });

    it('create_rtl_sets_direction_rtl', 0, () => {
      const handler: RecordingHybridHandler = new RecordingHybridHandler();
      const channel: PlatformViewsChannelHybrid = makeChannel(handler);
      const result: RecordingMethodResult = new RecordingMethodResult();
      channel.create(new MethodCall('create', makeCreateArgs(12, 'rtl/view', 1)), result);

      expect(handler.lastCreate!.direction).assertEqual(Direction.Rtl);
      expect(result.successCount).assertEqual(1);
    });

    it('create_with_params_forwards_bytebuffer', 0, () => {
      const handler: RecordingHybridHandler = new RecordingHybridHandler();
      const channel: PlatformViewsChannelHybrid = makeChannel(handler);
      const result: RecordingMethodResult = new RecordingMethodResult();
      const args: Map<string, Object> = makeCreateArgs(14, 'with/params', 0);
      const params: ByteBuffer = ByteBuffer.from(new ArrayBuffer(4));
      args.set('params', params);
      channel.create(new MethodCall('create', args), result);
      expect(handler.lastCreate).not().assertNull();
      expect(handler.lastCreate!.params).assertEqual(params);
      expect(result.successCount).assertEqual(1);
    });

    it('create_handler_throw_reports_error', 0, () => {
      const handler: RecordingHybridHandler = new RecordingHybridHandler();
      handler.throwOnCreate = true;
      const channel: PlatformViewsChannelHybrid = makeChannel(handler);
      const result: RecordingMethodResult = new RecordingMethodResult();
      channel.create(new MethodCall('create', makeCreateArgs(13, 'err/view', 0)), result);

      expect(result.errorCode).assertEqual('error');
      expect(result.successCount).assertEqual(0);
    });

    it('dispose_forwards_view_id', 0, () => {
      const handler: RecordingHybridHandler = new RecordingHybridHandler();
      const channel: PlatformViewsChannelHybrid = makeChannel(handler);
      const result: RecordingMethodResult = new RecordingMethodResult();
      const args: Map<string, Object> = new Map();
      args.set('id', 21);
      channel.dispose(new MethodCall('dispose', args), result);

      expect(handler.lastDisposeId).assertEqual(21);
      expect(result.successCount).assertEqual(1);
    });

    it('dispose_handler_throw_reports_error', 0, () => {
      const handler: RecordingHybridHandler = new RecordingHybridHandler();
      handler.throwOnDispose = true;
      const channel: PlatformViewsChannelHybrid = makeChannel(handler);
      const result: RecordingMethodResult = new RecordingMethodResult();
      const args: Map<string, Object> = new Map();
      args.set('id', 22);
      channel.dispose(new MethodCall('dispose', args), result);
      expect(result.errorCode).assertEqual('error');
      expect(result.successCount).assertEqual(0);
    });

    it('touch_parses_sixteen_positional_args', 0, () => {
      const handler: RecordingHybridHandler = new RecordingHybridHandler();
      const channel: PlatformViewsChannelHybrid = makeChannel(handler);
      const result: RecordingMethodResult = new RecordingMethodResult();
      const touchArgs: Object[] = [
        31, 100, 200, 0, 1,
        [1], [0, 0, 1, 1, 10, 20, 0, 0, 0],
        0, 0, 1.0, 1.0, 0, 0, 0, 0, 99
      ];
      channel.touch(new MethodCall('touch', touchArgs), result);

      expect(handler.lastTouch).not().assertNull();
      expect(handler.lastTouch!.viewId).assertEqual(31);
      expect(handler.lastTouch!.action).assertEqual(0);
      expect(handler.lastTouch!.pointerCount).assertEqual(1);
      expect(handler.lastTouch!.motionEventId).assertEqual(99);
      expect(result.successCount).assertEqual(1);
    });

    it('touch_handler_throw_reports_error', 0, () => {
      const handler: RecordingHybridHandler = new RecordingHybridHandler();
      handler.throwOnTouch = true;
      const channel: PlatformViewsChannelHybrid = makeChannel(handler);
      const result: RecordingMethodResult = new RecordingMethodResult();
      const touchArgs: Object[] = [
        32, 1, 2, 0, 1,
        [1], [0, 0, 1, 1, 0, 0, 0, 0, 0],
        0, 0, 1.0, 1.0, 0, 0, 0, 0, 1
      ];
      channel.touch(new MethodCall('touch', touchArgs), result);
      expect(result.errorCode).assertEqual('error');
      expect(result.successCount).assertEqual(0);
    });

    it('setDirection_rtl', 0, () => {
      const handler: RecordingHybridHandler = new RecordingHybridHandler();
      const channel: PlatformViewsChannelHybrid = makeChannel(handler);
      const result: RecordingMethodResult = new RecordingMethodResult();
      const args: Map<string, Object> = new Map();
      args.set('id', 41);
      args.set('direction', 1);
      channel.setDirection(new MethodCall('setDirection', args), result);

      expect(handler.lastDirectionViewId).assertEqual(41);
      expect(handler.lastDirection).assertEqual(Direction.Rtl);
      expect(result.successCount).assertEqual(1);
    });

    it('setDirection_ltr', 0, () => {
      const handler: RecordingHybridHandler = new RecordingHybridHandler();
      const channel: PlatformViewsChannelHybrid = makeChannel(handler);
      const result: RecordingMethodResult = new RecordingMethodResult();
      const args: Map<string, Object> = new Map();
      args.set('id', 42);
      args.set('direction', 0);
      channel.setDirection(new MethodCall('setDirection', args), result);
      expect(handler.lastDirection).assertEqual(Direction.Ltr);
      expect(result.successCount).assertEqual(1);
    });

    it('setDirection_handler_throw_reports_error', 0, () => {
      const handler: RecordingHybridHandler = new RecordingHybridHandler();
      handler.throwOnSetDirection = true;
      const channel: PlatformViewsChannelHybrid = makeChannel(handler);
      const result: RecordingMethodResult = new RecordingMethodResult();
      const args: Map<string, Object> = new Map();
      args.set('id', 43);
      args.set('direction', 0);
      channel.setDirection(new MethodCall('setDirection', args), result);
      expect(result.errorCode).assertEqual('error');
    });

    it('clearFocus_forwards_view_id', 0, () => {
      const handler: RecordingHybridHandler = new RecordingHybridHandler();
      const channel: PlatformViewsChannelHybrid = makeChannel(handler);
      const result: RecordingMethodResult = new RecordingMethodResult();
      channel.clearFocus(new MethodCall('clearFocus', 51), result);

      expect(handler.lastClearFocusId).assertEqual(51);
      expect(result.successCount).assertEqual(1);
    });

    it('clearFocus_handler_throw_reports_error', 0, () => {
      const handler: RecordingHybridHandler = new RecordingHybridHandler();
      handler.throwOnClearFocus = true;
      const channel: PlatformViewsChannelHybrid = makeChannel(handler);
      const result: RecordingMethodResult = new RecordingMethodResult();
      channel.clearFocus(new MethodCall('clearFocus', 52), result);
      expect(result.errorCode).assertEqual('error');
    });

    it('isSurfaceControlEnabled_returns_handler_value', 0, () => {
      const handler: RecordingHybridHandler = new RecordingHybridHandler();
      handler.surfaceControlEnabled = false;
      const channel: PlatformViewsChannelHybrid = makeChannel(handler);
      const result: RecordingMethodResult = new RecordingMethodResult();
      channel.isSurfaceControlEnabled(new MethodCall('isSurfaceControlEnabled', null), result);

      expect(result.successValue).assertEqual(false);
      expect(result.successCount).assertEqual(1);
    });

    it('isSurfaceControlEnabled_true_by_default', 0, () => {
      const handler: RecordingHybridHandler = new RecordingHybridHandler();
      const channel: PlatformViewsChannelHybrid = makeChannel(handler);
      const result: RecordingMethodResult = new RecordingMethodResult();
      channel.isSurfaceControlEnabled(new MethodCall('isSurfaceControlEnabled', null), result);
      expect(result.successValue).assertEqual(true);
    });

    it('isSurfaceControlEnabled_null_handler_returns_false', 0, () => {
      const channel: PlatformViewsChannelHybrid = makeChannel(null);
      const result: RecordingMethodResult = new RecordingMethodResult();
      channel.isSurfaceControlEnabled(new MethodCall('isSurfaceControlEnabled', null), result);
      expect(result.successValue).assertEqual(false);
    });

    it('isSurfaceControlEnabled_handler_throw_reports_error', 0, () => {
      const handler: RecordingHybridHandler = new RecordingHybridHandler();
      handler.throwOnSurfaceControl = true;
      const channel: PlatformViewsChannelHybrid = makeChannel(handler);
      const result: RecordingMethodResult = new RecordingMethodResult();
      channel.isSurfaceControlEnabled(new MethodCall('isSurfaceControlEnabled', null), result);
      expect(result.errorCode).assertEqual('error');
    });

    it('setOverscrollOffset_forwards_values', 0, () => {
      const handler: RecordingHybridHandler = new RecordingHybridHandler();
      const channel: PlatformViewsChannelHybrid = makeChannel(handler);
      const result: RecordingMethodResult = new RecordingMethodResult();
      const args: Map<string, Object> = new Map();
      args.set('id', 61);
      args.set('offset', -8.5);
      channel.setOverscrollOffset(new MethodCall('setOverscrollOffset', args), result);

      expect(handler.lastOverscrollViewId).assertEqual(61);
      expect(handler.lastOverscrollOffset).assertEqual(-8.5);
      expect(result.successCount).assertEqual(1);
    });

    it('setOverscrollOffset_handler_throw_reports_error', 0, () => {
      const handler: RecordingHybridHandler = new RecordingHybridHandler();
      handler.throwOnOverscroll = true;
      const channel: PlatformViewsChannelHybrid = makeChannel(handler);
      const result: RecordingMethodResult = new RecordingMethodResult();
      const args: Map<string, Object> = new Map();
      args.set('id', 62);
      args.set('offset', 1);
      channel.setOverscrollOffset(new MethodCall('setOverscrollOffset', args), result);
      expect(result.errorCode).assertEqual('error');
    });

    it('setPlatformViewsHandler_null_then_create_is_safe', 0, () => {
      const channel: PlatformViewsChannelHybrid = makeChannel(null);
      const result: RecordingMethodResult = new RecordingMethodResult();
      channel.create(new MethodCall('create', makeCreateArgs(90, 'noop', 0)), result);
      expect(result.successCount).assertEqual(1);
    });

    it('invokeViewFocused_does_not_throw', 0, () => {
      const channel: PlatformViewsChannelHybrid = makeChannel(new RecordingHybridHandler());
      let threw: boolean = false;
      try {
        channel.invokeViewFocused(3);
      } catch (_e) {
        threw = true;
      }
      expect(threw).assertFalse();
    });

    it('binary_route_create_hits_parsing_callback', 0, () => {
      const handler: RecordingHybridHandler = new RecordingHybridHandler();
      const pair = makeChannelWithMessenger(handler);
      const reply: RecordingBinaryReply =
        dispatchBinary(pair[1], 'create', makeCreateArgs(71, 'via/binary', 0));
      expect(handler.lastCreate).not().assertNull();
      expect(handler.lastCreate!.viewId).assertEqual(71);
      expect(reply.replyCount).assertEqual(1);
      expect(reply.replyBuffer).not().assertNull();
    });

    it('binary_route_unknown_method_not_implemented', 0, () => {
      const handler: RecordingHybridHandler = new RecordingHybridHandler();
      const pair = makeChannelWithMessenger(handler);
      const reply: RecordingBinaryReply = dispatchBinary(pair[1], 'unknown_method', null);
      // notImplemented replies with null envelope
      expect(reply.replyCount).assertEqual(1);
      expect(reply.replyBuffer).assertNull();
    });

    it('binary_route_null_handler_is_silent', 0, () => {
      const pair = makeChannelWithMessenger(null);
      const reply: RecordingBinaryReply =
        dispatchBinary(pair[1], 'create', makeCreateArgs(72, 'silent', 0));
      expect(reply.replyCount).assertEqual(0);
    });

    it('binary_route_dispose_and_setOverscroll', 0, () => {
      const handler: RecordingHybridHandler = new RecordingHybridHandler();
      const pair = makeChannelWithMessenger(handler);
      const disposeArgs: Map<string, Object> = new Map();
      disposeArgs.set('id', 81);
      dispatchBinary(pair[1], 'dispose', disposeArgs);
      expect(handler.lastDisposeId).assertEqual(81);

      const overArgs: Map<string, Object> = new Map();
      overArgs.set('id', 82);
      overArgs.set('offset', 3.25);
      dispatchBinary(pair[1], 'setOverscrollOffset', overArgs);
      expect(handler.lastOverscrollViewId).assertEqual(82);
      expect(handler.lastOverscrollOffset).assertEqual(3.25);
    });

    it('binary_route_touch_setDirection_clearFocus_surfaceControl', 0, () => {
      const handler: RecordingHybridHandler = new RecordingHybridHandler();
      const pair = makeChannelWithMessenger(handler);
      const touchArgs: Object[] = [
        91, 1, 2, 0, 1,
        [1], [0, 0, 1, 1, 0, 0, 0, 0, 0],
        0, 0, 1.0, 1.0, 0, 0, 0, 0, 7
      ];
      dispatchBinary(pair[1], 'touch', touchArgs);
      expect(handler.lastTouch).not().assertNull();
      expect(handler.lastTouch!.viewId).assertEqual(91);

      const dirArgs: Map<string, Object> = new Map();
      dirArgs.set('id', 92);
      dirArgs.set('direction', 1);
      dispatchBinary(pair[1], 'setDirection', dirArgs);
      expect(handler.lastDirectionViewId).assertEqual(92);
      expect(handler.lastDirection).assertEqual(Direction.Rtl);

      dispatchBinary(pair[1], 'clearFocus', 93);
      expect(handler.lastClearFocusId).assertEqual(93);

      handler.surfaceControlEnabled = false;
      dispatchBinary(pair[1], 'isSurfaceControlEnabled', null);
      expect(handler.surfaceControlEnabled).assertFalse();
    });

    it('invokeViewFocused_null_channel_is_safe', 0, () => {
      const channel: PlatformViewsChannelHybrid = makeChannel(new RecordingHybridHandler());
      const anyChannel: Any = channel as Any;
      anyChannel.channel = null;
      let threw: boolean = false;
      try {
        channel.invokeViewFocused(4);
      } catch (_e) {
        threw = true;
      }
      expect(threw).assertFalse();
    });
  });
}
