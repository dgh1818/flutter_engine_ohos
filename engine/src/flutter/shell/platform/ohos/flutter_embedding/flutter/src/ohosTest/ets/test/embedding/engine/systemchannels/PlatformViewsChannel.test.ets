import { describe, it, expect, Level } from '@ohos/hypium';
import DartExecutor from '../../../../../../main/ets/embedding/engine/dart/DartExecutor';
import FlutterNapi from '../../../../../../main/ets/embedding/engine/FlutterNapi';
import PlatformViewsChannel, {
  PlatformViewCreationRequest,
  PlatformViewResizeRequest,
  PlatformViewBufferSize,
  PlatformViewTouch,
  PlatformViewsHandler,
  PlatformViewBufferResized,
} from '../../../../../../main/ets/embedding/engine/systemchannels/PlatformViewsChannel';
import { ByteBuffer } from '../../../../../../main/ets/util/ByteBuffer';
import Any from '../../../../../../main/ets/plugin/common/Any';
import MethodCall from '../../../../../../main/ets/plugin/common/MethodCall';
import { MethodResult } from '../../../../../../main/ets/plugin/common/MethodChannel';
import resourceManager from '@ohos.resourceManager';
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

class FakePlatformViewsHandler implements PlatformViewsHandler {
  disposeCalls: number = 0;
  lastDisposeViewId: number = -1;
  clearFocusCalls: number = 0;
  lastClearFocusViewId: number = -1;
  syncCalls: number = 0;
  lastSyncValue: boolean = false;
  hoverCalls: number = 0;
  lastHoverViewId: number = -1;
  createTextureResult: number = 42;
  createTextureCalls: number = 0;
  createPlatformViewCalls: number = 0;
  resizeCalls: number = 0;

  createForPlatformViewLayer(request: PlatformViewCreationRequest): void {
    this.createPlatformViewCalls++;
  }

  createForTextureLayer(request: PlatformViewCreationRequest): number {
    this.createTextureCalls++;
    return this.createTextureResult;
  }

  dispose(viewId: number): void {
    this.disposeCalls++;
    this.lastDisposeViewId = viewId;
  }

  resize(request: PlatformViewResizeRequest, onComplete: PlatformViewBufferResized): void {
    this.resizeCalls++;
    onComplete.run(new PlatformViewBufferSize(100, 200));
  }

  offset(viewId: number, top: number, left: number, transform?: Array<number>): void {}

  onTouch(touch: PlatformViewTouch): void {}

  setDirection(viewId: number, direction: Direction): void {}

  clearFocus(viewId: number): void {
    this.clearFocusCalls++;
    this.lastClearFocusViewId = viewId;
  }

  synchronizeToNativeViewHierarchy(yes: boolean): void {
    this.syncCalls++;
    this.lastSyncValue = yes;
  }

  hover(viewId: number): void {
    this.hoverCalls++;
    this.lastHoverViewId = viewId;
  }
}

function createChannel(): PlatformViewsChannel {
  const flutterNapi: FlutterNapi = new FlutterNapi();
  const context = getContext();
  const rm: resourceManager.ResourceManager = context.resourceManager;
  const dartExecutor: DartExecutor = new DartExecutor(flutterNapi, rm);
  return new PlatformViewsChannel(dartExecutor);
}

export default function platformViewsChannelTest() {
  describe('PlatformViewCreationRequest.constructor', () => {
    it('assigns all fields correctly', Level.LEVEL0, () => {
      const params: ByteBuffer = ByteBuffer.from(new ArrayBuffer(0));
      const request = new PlatformViewCreationRequest(1, 'test_view', 10, 20, 100, 200, 0, params);
      expect(request.viewId).assertEqual(1);
      expect(request.viewType).assertEqual('test_view');
      expect(request.logicalTop).assertEqual(10);
      expect(request.logicalLeft).assertEqual(20);
      expect(request.logicalWidth).assertEqual(100);
      expect(request.logicalHeight).assertEqual(200);
      expect(request.direction).assertEqual(0);
    });

    it('handles zero values', Level.LEVEL0, () => {
      const params: ByteBuffer = ByteBuffer.from(new ArrayBuffer(0));
      const request = new PlatformViewCreationRequest(0, '', 0, 0, 0, 0, 0, params);
      expect(request.viewId).assertEqual(0);
      expect(request.viewType).assertEqual('');
    });
  });

  describe('PlatformViewResizeRequest.constructor', () => {
    it('assigns all fields correctly', Level.LEVEL0, () => {
      const request = new PlatformViewResizeRequest(5, 300, 400);
      expect(request.viewId).assertEqual(5);
      expect(request.newLogicalWidth).assertEqual(300);
      expect(request.newLogicalHeight).assertEqual(400);
    });

    it('handles zero values', Level.LEVEL0, () => {
      const request = new PlatformViewResizeRequest(0, 0, 0);
      expect(request.viewId).assertEqual(0);
      expect(request.newLogicalWidth).assertEqual(0);
      expect(request.newLogicalHeight).assertEqual(0);
    });
  });

  describe('PlatformViewBufferSize.constructor', () => {
    it('assigns width and height', Level.LEVEL0, () => {
      const size = new PlatformViewBufferSize(1920, 1080);
      expect(size.width).assertEqual(1920);
      expect(size.height).assertEqual(1080);
    });

    it('handles zero values', Level.LEVEL0, () => {
      const size = new PlatformViewBufferSize(0, 0);
      expect(size.width).assertEqual(0);
      expect(size.height).assertEqual(0);
    });
  });

  describe('PlatformViewTouch.constructor', () => {
    it('assigns all fields correctly', Level.LEVEL1, () => {
      const touch = new PlatformViewTouch(
        1, 100, 200, 0, 1, null, null, 0, 0, 1.0, 1.0, 1, 0, 0, 0, 1
      );
      expect(touch.viewId).assertEqual(1);
      expect(touch.downTime).assertEqual(100);
      expect(touch.eventTime).assertEqual(200);
      expect(touch.action).assertEqual(0);
      expect(touch.pointerCount).assertEqual(1);
      expect(touch.metaState).assertEqual(0);
      expect(touch.buttonState).assertEqual(0);
      expect(touch.xPrecision).assertEqual(1.0);
      expect(touch.yPrecision).assertEqual(1.0);
      expect(touch.deviceId).assertEqual(1);
      expect(touch.edgeFlags).assertEqual(0);
      expect(touch.source).assertEqual(0);
      expect(touch.flags).assertEqual(0);
      expect(touch.motionEventId).assertEqual(1);
    });

    it('handles all zero values', Level.LEVEL0, () => {
      const touch = new PlatformViewTouch(
        0, 0, 0, 0, 0, null, null, 0, 0, 0, 0, 0, 0, 0, 0, 0
      );
      expect(touch.viewId).assertEqual(0);
      expect(touch.downTime).assertEqual(0);
      expect(touch.eventTime).assertEqual(0);
    });
  });

  describe('PlatformViewsChannel.construct', () => {
    it('constructsWithoutCrash', Level.LEVEL1, () => {
      const channel: PlatformViewsChannel = createChannel();
      expect(channel).not().assertNull();
    });
  });

  describe('PlatformViewsChannel.setPlatformViewsHandler', () => {
    it('setsHandlerWithoutCrash', Level.LEVEL1, () => {
      const channel: PlatformViewsChannel = createChannel();
      const handler: FakePlatformViewsHandler = new FakePlatformViewsHandler();
      channel.setPlatformViewsHandler(handler);
      channel.setPlatformViewsHandler(null);
    });
  });

  describe('PlatformViewsChannel.dispose', () => {
    it('dispatchesToHandlerWithCorrectViewId', Level.LEVEL1, () => {
      const channel: PlatformViewsChannel = createChannel();
      const handler: FakePlatformViewsHandler = new FakePlatformViewsHandler();
      channel.setPlatformViewsHandler(handler);
      const result: FakeMethodResult = new FakeMethodResult();
      const args: Map<string, number> = new Map();
      args.set('id', 42);
      const call: MethodCall = new MethodCall('dispose', args);
      channel.dispose(call, result);
      expect(handler.disposeCalls).assertEqual(1);
      expect(handler.lastDisposeViewId).assertEqual(42);
      expect(result.successCalls).assertEqual(1);
    });
  });

  describe('PlatformViewsChannel.clearFocus', () => {
    it('dispatchesToHandlerWithCorrectViewId', Level.LEVEL1, () => {
      const channel: PlatformViewsChannel = createChannel();
      const handler: FakePlatformViewsHandler = new FakePlatformViewsHandler();
      channel.setPlatformViewsHandler(handler);
      const result: FakeMethodResult = new FakeMethodResult();
      const call: MethodCall = new MethodCall('clearFocus', 7);
      channel.clearFocus(call, result);
      expect(handler.clearFocusCalls).assertEqual(1);
      expect(handler.lastClearFocusViewId).assertEqual(7);
      expect(result.successCalls).assertEqual(1);
    });
  });

  describe('PlatformViewsChannel.synchronizeToNativeViewHierarchy', () => {
    it('dispatchesToHandlerWithTrueValue', Level.LEVEL1, () => {
      const channel: PlatformViewsChannel = createChannel();
      const handler: FakePlatformViewsHandler = new FakePlatformViewsHandler();
      channel.setPlatformViewsHandler(handler);
      const result: FakeMethodResult = new FakeMethodResult();
      const call: MethodCall = new MethodCall('synchronizeToNativeViewHierarchy', true);
      channel.synchronizeToNativeViewHierarchy(call, result);
      expect(handler.syncCalls).assertEqual(1);
      expect(handler.lastSyncValue).assertEqual(true);
      expect(result.successCalls).assertEqual(1);
    });

    it('dispatchesToHandlerWithFalseValue', Level.LEVEL1, () => {
      const channel: PlatformViewsChannel = createChannel();
      const handler: FakePlatformViewsHandler = new FakePlatformViewsHandler();
      channel.setPlatformViewsHandler(handler);
      const result: FakeMethodResult = new FakeMethodResult();
      const call: MethodCall = new MethodCall('synchronizeToNativeViewHierarchy', false);
      channel.synchronizeToNativeViewHierarchy(call, result);
      expect(handler.syncCalls).assertEqual(1);
      expect(handler.lastSyncValue).assertEqual(false);
      expect(result.successCalls).assertEqual(1);
    });
  });

  describe('PlatformViewsChannel.hover', () => {
    it('dispatchesToHandlerWithCorrectViewId', Level.LEVEL1, () => {
      const channel: PlatformViewsChannel = createChannel();
      const handler: FakePlatformViewsHandler = new FakePlatformViewsHandler();
      channel.setPlatformViewsHandler(handler);
      const result: FakeMethodResult = new FakeMethodResult();
      const call: MethodCall = new MethodCall('hover', 15);
      channel.hover(call, result);
      expect(handler.hoverCalls).assertEqual(1);
      expect(handler.lastHoverViewId).assertEqual(15);
      expect(result.successCalls).assertEqual(1);
    });
  });

  describe('PlatformViewsChannel.create.textureLayer', () => {
    it('dispatchesCreateForTextureLayerAndReturnsTextureId', Level.LEVEL1, () => {
      const channel: PlatformViewsChannel = createChannel();
      const handler: FakePlatformViewsHandler = new FakePlatformViewsHandler();
      channel.setPlatformViewsHandler(handler);
      const result: FakeMethodResult = new FakeMethodResult();
      const args: Map<string, Any> = new Map();
      args.set('id', 1);
      args.set('viewType', 'test_view');
      args.set('width', 100);
      args.set('height', 200);
      args.set('direction', 0);
      const call: MethodCall = new MethodCall('create', args);
      channel.create(call, result);
      expect(handler.createTextureCalls).assertEqual(1);
      expect(result.successCalls).assertEqual(1);
      expect(result.successResult).assertEqual(42);
    });
  });

  describe('PlatformViewsChannel.resize', () => {
    it('dispatchesResizeToHandler', Level.LEVEL1, () => {
      const channel: PlatformViewsChannel = createChannel();
      const handler: FakePlatformViewsHandler = new FakePlatformViewsHandler();
      channel.setPlatformViewsHandler(handler);
      const result: FakeMethodResult = new FakeMethodResult();
      const args: Map<string, number> = new Map();
      args.set('id', 3);
      args.set('width', 300);
      args.set('height', 400);
      const call: MethodCall = new MethodCall('resize', args);
      channel.resize(call, result);
      expect(handler.resizeCalls).assertEqual(1);
      expect(result.successCalls).assertEqual(1);
    });
  });
}

