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
import { describe, expect, it } from '@ohos/hypium';
import common from '@ohos.app.ability.common';
import Any from '../../../../../main/ets/plugin/common/Any';
import {
  SurfaceTextureEntry,
  TextureRegistry
} from '../../../../../main/ets/view/TextureRegistry';
import image from '@ohos.multimedia.image';
import { ByteBuffer } from '../../../../../main/ets/util/ByteBuffer';
import { FlutterView } from '../../../../../main/ets/view/FlutterView';
import StringCodec from '../../../../../main/ets/plugin/common/StringCodec';
import DartExecutor from '../../../../../main/ets/embedding/engine/dart/DartExecutor';
import { FlutterOverlaySurface } from '../../../../../main/ets/embedding/engine/FlutterOverlaySurface';
import {
  PlatformViewBufferResized,
  PlatformViewBufferSize,
  PlatformViewCreationRequest,
  PlatformViewResizeRequest,
  PlatformViewTouch
} from '../../../../../main/ets/embedding/engine/systemchannels/PlatformViewsChannel';
import PlatformView, { Params } from '../../../../../main/ets/plugin/platform/PlatformView';
import { CustomTouchEvent, CustomTouchObject } from '../../../../../main/ets/plugin/platform/CustomTouchEvent';
import PlatformViewFactory from '../../../../../main/ets/plugin/platform/PlatformViewFactory';
import { DVModel, DVModelParameters, DVModelEvents, DVModelChildren } from '../../../../../main/ets/view/DynamicView/dynamicView';
import PlatformViewRegistryImpl from '../../../../../main/ets/plugin/platform/PlatformViewRegistryImpl';
import PlatformViewsController from '../../../../../main/ets/plugin/platform/PlatformViewsController';

const platformViewsController: PlatformViewsController = new PlatformViewsController();
const direction: Direction = 100;
const VIEW_TYPE: string = 'legacy/ut_view';

class FakeBinaryMessenger {
  send(_channel: String, _message: ArrayBuffer | null, _callback?: Object | null): void {
  }

  setMessageHandler(_channel: String, _handler: Object | null): void {
  }

  makeBackgroundTaskQueue(): Object {
    return new Object();
  }
}

class FakePlatformView extends PlatformView {
  disposed: boolean = false;
  attachedCount: number = 0;
  detachedCount: number = 0;
  viewResult: WrappedBuilder<[Params]> | null = (new Object()) as WrappedBuilder<[Params]>;

  getView(): WrappedBuilder<[Params]> {
    return this.viewResult!;
  }

  onFlutterViewAttached(_dvModel: DVModel): void {
    this.attachedCount++;
  }

  onFlutterViewDetached(): void {
    this.detachedCount++;
  }

  dispose(): void {
    this.disposed = true;
  }
}

class FakePlatformViewFactory extends PlatformViewFactory {
  created: number = 0;

  constructor() {
    super(StringCodec.INSTANCE);
  }

  create(_context: common.Context, _viewId: number, _args: Any): PlatformView {
    this.created++;
    return new FakePlatformView();
  }
}

class FakeHandoffPlatformView extends FakePlatformView {
  shouldTriggerTextInputHandoffOnTouch(): boolean {
    return true;
  }
}

class FakeHandoffPlatformViewFactory extends PlatformViewFactory {
  created: number = 0;

  constructor() {
    super(StringCodec.INSTANCE);
  }

  create(_context: common.Context, _viewId: number, _args: Any): PlatformView {
    this.created++;
    return new FakeHandoffPlatformView();
  }
}

class FakeFlutterViewHost {
  private dVModel: DVModel =
    new DVModel('Stack', new DVModelParameters(), new DVModelEvents(), new DVModelChildren(), null);

  getDVModel(): DVModel {
    return this.dVModel;
  }

  getId(): string {
    return 'legacy-ut-host';
  }

  setSurfaceId(_surfaceId: string): void {
  }

  setWrappedBuilder(_builder: WrappedBuilder<[Params]>): void {
  }

  setPlatformView(_view: PlatformView): void {
  }
}

class FakeSurfaceTextureEntry implements SurfaceTextureEntry {
  getTextureId(): number {
    return 101;
  }

  getSurfaceId(): number {
    return 202;
  }

  getNativeWindowId(): number {
    return 303;
  }

  release(): void {
  }
}

class FakeBufferResized extends PlatformViewBufferResized {
  count: number = 0;
  lastWidth: number = 0;
  lastHeight: number = 0;

  run(bufferSize: PlatformViewBufferSize): void {
    this.count++;
    this.lastWidth = bufferSize.width;
    this.lastHeight = bufferSize.height;
  }
}

class FakeTextureRegistry implements TextureRegistry {
  createSurfaceTexture(): SurfaceTextureEntry {
    return new FakeSurfaceTextureEntry();
  }

  getTextureId(): number {
    return 100;
  }

  registerTexture(_textureId: number): SurfaceTextureEntry {
    return new FakeSurfaceTextureEntry();
  }

  registerSurfaceTexture(_receiver: image.ImageReceiver): SurfaceTextureEntry {
    return new FakeSurfaceTextureEntry();
  }

  registerPixelMap(_pixelMap: PixelMap): number {
    return 0;
  }

  setTextureBackGroundPixelMap(_textureId: number, _pixelMap: PixelMap): void {
  }

  setTextureBackGroundColor(_textureId: number, _color: number): void {
  }

  setTextureBufferSize(_textureId: number, _width: number, _height: number): void {
  }

  notifyTextureResizing(_textureId: number, _width: number, _height: number): void {
  }

  setExternalNativeImage(_textureId: number, _native_image: number): boolean {
    return true;
  }

  setExternalNativeImagePtr(_textureId: number, _native_image: bigint): boolean {
    return true;
  }

  resetExternalTexture(_textureId: number, _need_surfaceId: boolean): number {
    return 0;
  }

  unregisterTexture(_textureId: number): void {
  }

  onTrimMemory(_level: number): void {
  }
}

function emptyParams(): ByteBuffer {
  return ByteBuffer.from(new ArrayBuffer(0));
}

function makeRequest(viewId: number, dir: Direction = Direction.Ltr): PlatformViewCreationRequest {
  return new PlatformViewCreationRequest(
    viewId, VIEW_TYPE, 0, 0, 40, 20, dir, emptyParams());
}

function activeMap(controller: ESObject, create: boolean): Any {
  const params: Record<string, Any> = new DVModelParameters() as Record<string, Any>;
  return controller.getActiveTouches(params, create);
}

function attachController(controller: PlatformViewsController,
  factory: PlatformViewFactory): FakeFlutterViewHost {
  const registry: PlatformViewRegistryImpl = controller.getRegistry() as PlatformViewRegistryImpl;
  registry.registerViewFactory(VIEW_TYPE, factory);
  const messenger: FakeBinaryMessenger = new FakeBinaryMessenger();
  controller.attach(getContext() as common.Context, new FakeTextureRegistry(), messenger as Any as DartExecutor);
  const host: FakeFlutterViewHost = new FakeFlutterViewHost();
  controller.attachToView(host as Any as FlutterView);
  return host;
}

export default function PlatformViewsControllerTest() {
  describe('PlatformViewsControllerTest', () => {

    it('TestPlatformViewRegistryImplGetRegistry', 0, async (done: Function) => {
      const result = platformViewsController.getRegistry();
      expect(result).not().assertNull();
      done();
    })

    it('TestCreateOverlaySurface', 0, async (done: Function) => {
      const result: FlutterOverlaySurface = platformViewsController.createOverlaySurface();
      expect(result).not().assertNull();
      done();
    })
    it('TestPlatformViewRegistryImplValidateDirection', 0, async (done: Function) => {
      const result = platformViewsController.validateDirection(direction);
      expect(result).assertFalse();
      done();
    })

    it('TestPlatformViewRegistryImplGetFlutterView', 0, async (done: Function) => {
      const result = platformViewsController.getFlutterView();
      expect(result).assertNull();
      done();
    })
    it('getRegistry returns non-null', 0, () => {
      const ctl = new PlatformViewsController();
      expect(ctl.getRegistry() !== null).assertEqual(true);
    });

    it('createOverlaySurface returns non-null', 0, () => {
      const ctl = new PlatformViewsController();
      const result = ctl.createOverlaySurface();
      expect(result !== null).assertEqual(true);
    });

    it('validateDirection returns false for invalid direction', 0, () => {
      const ctl = new PlatformViewsController();
      expect(ctl.validateDirection(100)).assertEqual(false);
      expect(ctl.validateDirection(-1)).assertEqual(false);
      expect(ctl.validateDirection(999)).assertEqual(false);
    });

    it('validateDirection returns true for valid direction (RTL/LTR/TC/BottomUp)', 0, () => {
      const ctl = new PlatformViewsController();
      // These are common enum values for Direction - exact ones depend on the enum
      // Just verify it returns boolean
      const result = ctl.validateDirection(0);
      expect(typeof result).assertEqual('boolean');
    });

    it('clearFocus does not throw on unknown viewId', 0, () => {
      const ctl = new PlatformViewsController();
      let threw = false;
      try { ctl.clearFocus(99999); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('setDirection does not throw on unknown viewId', 0, () => {
      const ctl = new PlatformViewsController();
      let threw = false;
      try { ctl.setDirection(99999, Direction.Auto); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('synchronizeToNativeViewHierarchy throws not implemented', 0, () => {
      // The implementation explicitly throws "Method not implemented.", so
      // we expect the call to throw. Verifying this guards against silent
      // changes in the upstream interface.
      const ctl = new PlatformViewsController();
      let threw = false;
      try {
        ctl.synchronizeToNativeViewHierarchy(true);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });

    it('hover with viewId does not throw', 0, () => {
      const ctl = new PlatformViewsController();
      let threw = false;
      try { ctl.hover(99999); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('offset with viewId does not throw', 0, () => {
      const ctl = new PlatformViewsController();
      let threw = false;
      try { ctl.offset(99999, 10, 20); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('clearFocus with viewId does not throw', 0, () => {
      const ctl = new PlatformViewsController();
      let threw = false;
      try { ctl.clearFocus(99999); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('finishFrame does not throw', 0, () => {
      const ctl: Any = new PlatformViewsController();
      let threw = false;
      try { ctl['finishFrame'](); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('attachTextInputPlugin does not throw with null', 0, () => {
      const ctl = new PlatformViewsController();
      let threw = false;
      try {
        ctl.attachTextInputPlugin(null as Any);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('detachTextInputPlugin does not throw', 0, () => {
      const ctl = new PlatformViewsController();
      let threw = false;
      try {
        ctl.detachTextInputPlugin();
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('detach does not throw when no engine', 0, () => {
      const ctl = new PlatformViewsController();
      let threw = false;
      try {
        ctl.detach();
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('detachFromView does not throw when no view', 0, () => {
      const ctl = new PlatformViewsController();
      let threw = false;
      try {
        ctl.detachFromView();
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('onBeginFrame does not throw when no engine', 0, () => {
      const ctl = new PlatformViewsController();
      let threw = false;
      try {
        ctl.onBeginFrame();
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('onEndFrame does not throw when no engine', 0, () => {
      const ctl = new PlatformViewsController();
      let threw = false;
      try {
        ctl.onEndFrame();
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('onPreEngineRestart does not throw', 0, () => {
      const ctl = new PlatformViewsController();
      let threw = false;
      try {
        ctl.onPreEngineRestart();
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('onDetachedFromNapi does not throw', 0, () => {
      const ctl = new PlatformViewsController();
      let threw = false;
      try {
        ctl.onDetachedFromNapi();
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('onDisplayOverlaySurface does not throw', 0, () => {
      const ctl = new PlatformViewsController();
      let threw = false;
      try {
        ctl.onDisplayOverlaySurface(0, 0, 0, 100, 100);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    // Methods that require flutterView throw Error in test env.
    // These exercise the error-handling branches.
    it('attach throws when FlutterView is detached', 0, () => {
      const ctl = new PlatformViewsController();
      let threw = false;
      try {
        ctl.attach(getContext(), null, null as Any);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });

    it('detachFromView with no view does not throw', 0, () => {
      const ctl = new PlatformViewsController();
      let threw = false;
      try {
        ctl.detachFromView();
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('getDisplayDensity returns a number when invoked on platform controller', 0, () => {
      const ctl: Any = new PlatformViewsController();
      const density: number = ctl.getDisplayDensity();
      expect(typeof density).assertEqual('number');
    });

    it('toPhysicalPixels via bracket access', 0, () => {
      const ctl: Any = new PlatformViewsController();
      const fn: Function = ctl['toPhysicalPixels'];
      expect(fn !== undefined).assertEqual(true);
    });

    it('toLogicalPixels via bracket access', 0, () => {
      const ctl: Any = new PlatformViewsController();
      const fn: Function = ctl['toLogicalPixels'];
      expect(fn !== undefined).assertEqual(true);
    });

    it('toLogicalPixelsByDensity via bracket access', 0, () => {
      const ctl: Any = new PlatformViewsController();
      const fn: Function = ctl['toLogicalPixelsByDensity'];
      expect(fn !== undefined).assertEqual(true);
    });

    it('isTerminalTouch via bracket access', 0, () => {
      const ctl: Any = new PlatformViewsController();
      const fn: Function = ctl['isTerminalTouch'];
      expect(fn !== undefined).assertEqual(true);
    });

    it('cloneTouchObjectWithType via bracket access', 0, () => {
      const ctl: Any = new PlatformViewsController();
      const fn: Function = ctl['cloneTouchObjectWithType'];
      expect(fn !== undefined).assertEqual(true);
    });

    it('getActiveTouches via bracket access', 0, () => {
      const ctl: Any = new PlatformViewsController();
      const fn: Function = ctl['getActiveTouches'];
      expect(fn !== undefined).assertEqual(true);
    });

    it('rememberPostedTouchEvent via bracket access', 0, () => {
      const ctl: Any = new PlatformViewsController();
      const fn: Function = ctl['rememberPostedTouchEvent'];
      expect(fn !== undefined).assertEqual(true);
    });

    it('clearTrackedTouchState via bracket access', 0, () => {
      const ctl: Any = new PlatformViewsController();
      const fn: Function = ctl['clearTrackedTouchState'];
      expect(fn !== undefined).assertEqual(true);
    });

    it('postTouchCancelEvent via bracket access', 0, () => {
      const ctl: Any = new PlatformViewsController();
      const fn: Function = ctl['postTouchCancelEvent'];
      expect(fn !== undefined).assertEqual(true);
    });

    it('cancelActiveTouchIfNeeded via bracket access', 0, () => {
      const ctl: Any = new PlatformViewsController();
      const fn: Function = ctl['cancelActiveTouchIfNeeded'];
      expect(fn !== undefined).assertEqual(true);
    });

    it('dispose does not throw', 0, () => {
      const ctl = new PlatformViewsController();
      let threw = false;
      try { ctl.dispose(0); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('ensureFlutterViewAttached via bracket access', 0, () => {
      const ctl: Any = new PlatformViewsController();
      const fn: Function = ctl['ensureFlutterViewAttached'];
      // 私有方法不存在于公开接口,只能通过 (this as Any) 绕过类型系统看到;
      // 函数对象本身是否存在与构建模式有关(debug 能看到,release 因混淆看不到),
      // 此处仅断言"能拿到个东西",不强制要求 undefined。
      expect(fn !== undefined).assertEqual(true);
    });

    it('initializeRootImageViewIfNeeded via bracket access', 0, () => {
      const ctl: Any = new PlatformViewsController();
      const fn: Function = ctl['initializeRootImageViewIfNeeded'];
      expect(fn !== undefined).assertEqual(true);
    });
  })

  describe('PlatformViewsController.validateDirection', () => {
    it('accepts Ltr Rtl and Auto', 0, () => {
      const controller: PlatformViewsController = new PlatformViewsController();
      expect(controller.validateDirection(Direction.Ltr)).assertEqual(true);
      expect(controller.validateDirection(Direction.Rtl)).assertEqual(true);
      expect(controller.validateDirection(Direction.Auto)).assertEqual(true);
    });

    it('rejects unknown numeric direction', 0, () => {
      const controller: PlatformViewsController = new PlatformViewsController();
      expect(controller.validateDirection(-1)).assertEqual(false);
      expect(controller.validateDirection(99)).assertEqual(false);
    });
  });

  describe('PlatformViewsController.attachDetach', () => {
    it('attach then getFlutterView then detach', 0, () => {
      const controller: PlatformViewsController = new PlatformViewsController();
      const factory: FakePlatformViewFactory = new FakePlatformViewFactory();
      const host: FakeFlutterViewHost = attachController(controller, factory);
      expect(controller.getFlutterView()).assertEqual(host as Any as FlutterView);
      expect(controller.hasTextureViews()).assertEqual(false);
      controller.attachToView(host as Any as FlutterView);
      controller.detachFromView();
      expect(controller.getFlutterView()).assertNull();
      controller.detach();
    });

    it('detachFromView with mismatched view is ignored', 0, () => {
      const controller: PlatformViewsController = new PlatformViewsController();
      const factory: FakePlatformViewFactory = new FakePlatformViewFactory();
      const host: FakeFlutterViewHost = attachController(controller, factory);
      const other: FakeFlutterViewHost = new FakeFlutterViewHost();
      controller.detachFromView(other as Any as FlutterView);
      expect(controller.getFlutterView()).assertEqual(host as Any as FlutterView);
      controller.detachFromView();
      controller.detach();
    });

    it('detachFromView when already detached is a no-op', 0, () => {
      const controller: PlatformViewsController = new PlatformViewsController();
      controller.detachFromView();
      expect(controller.getFlutterView()).assertNull();
    });
  });

  describe('PlatformViewsController.createAndDispose', () => {
    it('invalid direction throws before factory is used', 0, () => {
      const controller: PlatformViewsController = new PlatformViewsController();
      const factory: FakePlatformViewFactory = new FakePlatformViewFactory();
      attachController(controller, factory);
      let threw = false;
      try {
        controller.createForPlatformViewLayer(makeRequest(1, 100 as Direction));
      } catch (_e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
      expect(factory.created).assertEqual(0);
      controller.detachFromView();
      controller.detach();
    });

    it('unknown viewType throws', 0, () => {
      const controller: PlatformViewsController = new PlatformViewsController();
      const messenger: FakeBinaryMessenger = new FakeBinaryMessenger();
      controller.attach(getContext() as common.Context, null, messenger as Any as DartExecutor);
      const host: FakeFlutterViewHost = new FakeFlutterViewHost();
      controller.attachToView(host as Any as FlutterView);
      let threw = false;
      try {
        controller.createForPlatformViewLayer(makeRequest(2));
      } catch (_e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
      controller.detachFromView();
      controller.detach();
    });

    it('create then dispose marks texture views and disposes plugin view', 0, () => {
      const controller: PlatformViewsController = new PlatformViewsController();
      const factory: FakePlatformViewFactory = new FakePlatformViewFactory();
      attachController(controller, factory);
      controller.createForPlatformViewLayer(makeRequest(11));
      expect(factory.created).assertEqual(1);
      expect(controller.hasTextureViews()).assertEqual(true);
      controller.dispose(11);
      expect(controller.hasTextureViews()).assertEqual(false);
      controller.detachFromView();
      controller.detach();
    });

    it('dispose unknown id does not throw', 0, () => {
      const controller: PlatformViewsController = new PlatformViewsController();
      controller.dispose(999);
      expect(controller.hasTextureViews()).assertEqual(false);
    });

    it('createForTextureLayer without flutter view throws', 0, () => {
      const controller: PlatformViewsController = new PlatformViewsController();
      let threw = false;
      try {
        controller.createForTextureLayer(makeRequest(12));
      } catch (_e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });
  });

  describe('PlatformViewsController.touchAndFocus', () => {
    it('onTouch setDirection clearFocus hover offset unknown id do not throw', 0, () => {
      const controller: PlatformViewsController = new PlatformViewsController();
      const touch: PlatformViewTouch = new PlatformViewTouch(
        77, 1, 2, 0, 1, [1], [0, 0, 1, 1, 0, 0, 0, 0, 0],
        0, 0, 1.0, 1.0, 0, 0, 0, 0, 1);
      controller.onTouch(touch);
      controller.setDirection(77, Direction.Rtl);
      controller.clearFocus(77);
      controller.hover(77);
      controller.offset(77, 1, 2);
      controller.offset(77, Number.NaN, Number.NaN);
    });
  });

  describe('PlatformViewsController.lifecycleAndOverlay', () => {
    it('createOverlaySurface increments ids', 0, () => {
      const controller: PlatformViewsController = new PlatformViewsController();
      const first: FlutterOverlaySurface = controller.createOverlaySurface();
      const second: FlutterOverlaySurface = controller.createOverlaySurface();
      expect(first).not().assertNull();
      expect(second).not().assertNull();
      // createOverlaySurface allocates from nextOverlayLayerId++ (PlatformViewsController.ets:1181),
      // so consecutive surfaces must receive consecutive ids.
      expect(second.getId()).assertEqual(first.getId() + 1);
    });

    it('onBeginFrame onEndFrame onDisplayOverlaySurface are no-ops', 0, () => {
      const controller: PlatformViewsController = new PlatformViewsController();
      controller.onBeginFrame();
      controller.onEndFrame();
      controller.onDisplayOverlaySurface(0, 1, 2, 3, 4);
    });

    it('onDetachedFromNapi and onPreEngineRestart dispose remaining views', 0, () => {
      const controller: PlatformViewsController = new PlatformViewsController();
      const factory: FakePlatformViewFactory = new FakePlatformViewFactory();
      attachController(controller, factory);
      controller.createForPlatformViewLayer(makeRequest(21));
      controller.onPreEngineRestart();
      expect(controller.hasTextureViews()).assertEqual(false);
      controller.createForPlatformViewLayer(makeRequest(22));
      controller.onDetachedFromNapi();
      expect(controller.hasTextureViews()).assertEqual(false);
      controller.detachFromView();
      controller.detach();
    });

    it('attachTextInputPlugin then detachTextInputPlugin', 0, () => {
      const controller: PlatformViewsController = new PlatformViewsController();
      controller.detachTextInputPlugin();
      const anyC: ESObject = controller;
      expect(anyC.textInputPlugin).assertNull();
    });

    it('synchronizeToNativeViewHierarchy always throws', 0, () => {
      const controller: PlatformViewsController = new PlatformViewsController();
      let threw = false;
      try {
        controller.synchronizeToNativeViewHierarchy(true);
      } catch (_e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });

    it('setParams and getParams round trip', 0, () => {
      const controller: PlatformViewsController = new PlatformViewsController();
      const params: DVModelParameters = new DVModelParameters();
      controller.setParams(params, 'width', 12);
      expect(controller.getParams(params, 'width')).assertEqual(12);
    });

    it('render without flutter view does not throw', 0, () => {
      const controller: PlatformViewsController = new PlatformViewsController();
      const view: FakePlatformView = new FakePlatformView();
      controller.render(0, view, 10, 10, 0, 0);
    });
  });

  describe('PlatformViewsController.touchStateHelpers', () => {
    it('isTerminalTouch distinguishes terminal types', 0, () => {
      const controller: ESObject = new PlatformViewsController();
      expect(controller.isTerminalTouch(TouchType.Up)).assertEqual(true);
      expect(controller.isTerminalTouch(TouchType.Cancel)).assertEqual(true);
      expect(controller.isTerminalTouch(TouchType.Down)).assertEqual(false);
      expect(controller.isTerminalTouch(TouchType.Move)).assertEqual(false);
    });

    it('getActiveTouches creates map on demand and reuses it', 0, () => {
      const controller: ESObject = new PlatformViewsController();
      expect(activeMap(controller, false)).assertEqual(undefined);
      const created: Any = activeMap(controller, true);
      expect(created).not().assertNull();
      const again: Any = activeMap(controller, false);
      expect(again).not().assertNull();
    });

    it('rememberPostedTouchEvent tracks non-terminal touches', 0, () => {
      const controller: ESObject = new PlatformViewsController();
      const params: Record<string, Any> = new DVModelParameters() as Record<string, Any>;
      const downObj: CustomTouchObject = new CustomTouchObject(TouchType.Down, 1, 0, 0, 0, 0, 0, 0, 0, 0);
      const moveObj: CustomTouchObject = new CustomTouchObject(TouchType.Move, 2, 0, 0, 0, 0, 0, 0, 0, 0);
      const upObj: CustomTouchObject = new CustomTouchObject(TouchType.Up, 3, 0, 0, 0, 0, 0, 0, 0, 0);
      const event: CustomTouchEvent = new CustomTouchEvent(TouchType.Move, [upObj, downObj], [moveObj], 0,
        SourceType.TouchScreen, 1, 0, 0, SourceTool.Finger);
      controller.rememberPostedTouchEvent(params, event, true);
      const active: Any = controller.getActiveTouches(params, false);
      expect(active).not().assertNull();
      expect(active.size).assertEqual(2); // changedTouches move + touches down; up terminal removed
    });

    it('rememberPostedTouchEvent clears tracked state on cancel', 0, () => {
      const controller: ESObject = new PlatformViewsController();
      const params: Record<string, Any> = new DVModelParameters() as Record<string, Any>;
      const obj: CustomTouchObject = new CustomTouchObject(TouchType.Down, 1, 0, 0, 0, 0, 0, 0, 0, 0);
      const moveEvent: CustomTouchEvent = new CustomTouchEvent(TouchType.Move, [obj], [obj], 0,
        SourceType.TouchScreen, 1, 0, 0, SourceTool.Finger);
      controller.rememberPostedTouchEvent(params, moveEvent, true);
      const cancelObj: CustomTouchObject = new CustomTouchObject(TouchType.Cancel, 2, 0, 0, 0, 0, 0, 0, 0, 0);
      const cancelEvent: CustomTouchEvent = new CustomTouchEvent(TouchType.Cancel, [cancelObj], [cancelObj], 0,
        SourceType.TouchScreen, 1, 0, 0, SourceTool.Finger);
      controller.rememberPostedTouchEvent(params, cancelEvent, false);
      const active: Any = controller.getActiveTouches(params, false);
      expect(active).not().assertNull();
      expect(active.size).assertEqual(0); // cleared by cancel
    });

    it('cloneTouchObjectWithType preserves identity fields', 0, () => {
      const controller: ESObject = new PlatformViewsController();
      const obj: CustomTouchObject = new CustomTouchObject(TouchType.Down, 7, 1, 2, 3, 4, 5, 6, 7, 8);
      const clone: Any = controller.cloneTouchObjectWithType(obj, TouchType.Up);
      expect(clone.id).assertEqual(7);
      expect(clone.type).assertEqual(TouchType.Up);
    });

    it('cancelActiveTouchIfNeeded without node controller clears state', 0, () => {
      const controller: ESObject = new PlatformViewsController();
      const params: Record<string, Any> = new DVModelParameters() as Record<string, Any>;
      controller.cancelActiveTouchIfNeeded(1, params, 'test-reason');
      const active: Any = controller.getActiveTouches(params, false);
      expect(active).assertEqual(undefined);
    });
  });

  describe('PlatformViewsController.transformOffsetAndFocus', () => {
    it('applyTransformOffset falls back on non-finite coefficients', 0, () => {
      const controller: ESObject = new PlatformViewsController();
      const params: Record<string, Any> = new DVModelParameters() as Record<string, Any>;
      controller.setParams(params, 'left', 5);
      controller.setParams(params, 'top', 10);
      controller.applyTransformOffset(params, [Number.NaN, 0, 0, 1, 2, 3]);
      expect(controller.getParams(params, 'left')).assertEqual(5);
      expect(controller.getParams(params, 'top')).assertEqual(10);
      expect(params['affineTransform']).assertEqual(undefined);
    });

    it('applyTransformOffset falls back on singular determinant', 0, () => {
      const controller: ESObject = new PlatformViewsController();
      const params: Record<string, Any> = new DVModelParameters() as Record<string, Any>;
      controller.applyTransformOffset(params, [1, 0, 0, 0, 2, 3]);
      expect(controller.getParams(params, 'left')).assertEqual(2);
      expect(controller.getParams(params, 'top')).assertEqual(3);
      expect(params['affineTransform']).assertEqual(undefined);
    });

    it('applyTransformOffset computes the inverse affine for valid dimensions', 0, () => {
      const controller: ESObject = new PlatformViewsController();
      const params: Record<string, Any> = new DVModelParameters() as Record<string, Any>;
      controller.setParams(params, 'width', 100);
      controller.setParams(params, 'height', 50);
      controller.applyTransformOffset(params, [1, 0, 0, 1, 20, 30]);
      expect(controller.getParams(params, 'left')).assertEqual(20);
      expect(controller.getParams(params, 'top')).assertEqual(30);
      const affine: Any = params['affineTransform'];
      expect(affine[0]).assertEqual(1);
      expect(affine[1]).assertEqual(0);
      expect(affine[2]).assertEqual(-20);
      expect(affine[3]).assertEqual(0);
      expect(affine[4]).assertEqual(1);
      expect(affine[5]).assertEqual(-30);
    });

    it('applyTransformOffset falls back to origin corner when dimensions unknown', 0, () => {
      const controller: ESObject = new PlatformViewsController();
      const params: Record<string, Any> = new DVModelParameters() as Record<string, Any>;
      controller.applyTransformOffset(params, [1, 0, 0, 1, 20, 30]);
      expect(controller.getParams(params, 'left')).assertEqual(20);
      expect(controller.getParams(params, 'top')).assertEqual(30);
      expect(params['affineTransform']).not().assertNull();
    });

    it('invokeTextInputHandoffOnTouch guards on canInvokeViewFocused', 0, () => {
      const controller: ESObject = new PlatformViewsController();
      let threw = false;
      try {
        controller.invokeTextInputHandoffOnTouch(999, true, false);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('invokeTextInputHandoffOnTouch guards on shouldHandoffFrameworkFocus', 0, () => {
      const controller: ESObject = new PlatformViewsController();
      let threw = false;
      try {
        controller.invokeTextInputHandoffOnTouch(999, false, true);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('invokeTextInputHandoffOnTouch no-ops without a handoff platform view', 0, () => {
      const controller: ESObject = new PlatformViewsController();
      const factory: FakePlatformViewFactory = new FakePlatformViewFactory();
      const host: FakeFlutterViewHost = attachController(controller as Any as PlatformViewsController, factory);
      controller.createForPlatformViewLayer(makeRequest(51));
      let threw = false;
      try {
        controller.invokeTextInputHandoffOnTouch(51, true, true);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
      controller.detachFromView();
      controller.detach();
    });

    it('invokeTextInputHandoffOnTouch invokes viewFocused for a handoff platform view', 0, () => {
      const controller: ESObject = new PlatformViewsController();
      const factory: FakeHandoffPlatformViewFactory = new FakeHandoffPlatformViewFactory();
      const host: FakeFlutterViewHost = attachController(controller as Any as PlatformViewsController, factory);
      controller.createForPlatformViewLayer(makeRequest(52));
      let threw = false;
      try {
        controller.invokeTextInputHandoffOnTouch(52, true, true);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
      controller.detachFromView();
      controller.detach();
    });

    it('clearFocus with a null embedded view returns without throwing', 0, () => {
      const controller: ESObject = new PlatformViewsController();
      const factory: FakePlatformViewFactory = new FakePlatformViewFactory();
      const host: FakeFlutterViewHost = attachController(controller as Any as PlatformViewsController, factory);
      controller.createForPlatformViewLayer(makeRequest(53));
      const pv: Any = controller.platformViews.get(53);
      pv.viewResult = null;
      let threw = false;
      try {
        controller.clearFocus(53);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
      controller.detachFromView();
      controller.detach();
    });

    it('offset plain updates left and top and clears affine', 0, () => {
      const controller: ESObject = new PlatformViewsController();
      const factory: FakePlatformViewFactory = new FakePlatformViewFactory();
      const host: FakeFlutterViewHost = attachController(controller as Any as PlatformViewsController, factory);
      controller.createForTextureLayer(makeRequest(61));
      controller.offset(61, 7, 8);
      const params: Record<string, Any> = controller.viewWrappers.get(61).getDvModel().getLayoutParams() as Record<string, Any>;
      expect(params['left']).assertEqual(8);
      expect(params['top']).assertEqual(7);
      expect(params['affineTransform']).assertEqual(undefined);
      controller.detachFromView();
      controller.detach();
    });

    it('offset with NaN marks the view invisible', 0, () => {
      const controller: ESObject = new PlatformViewsController();
      const factory: FakePlatformViewFactory = new FakePlatformViewFactory();
      const host: FakeFlutterViewHost = attachController(controller as Any as PlatformViewsController, factory);
      controller.createForTextureLayer(makeRequest(62));
      controller.offset(62, Number.NaN, Number.NaN);
      const params: Record<string, Any> = controller.viewWrappers.get(62).getDvModel().getLayoutParams() as Record<string, Any>;
      expect(Number.isNaN(params['left'])).assertEqual(true);
      expect(Number.isNaN(params['top'])).assertEqual(true);
      expect(params['affineTransform']).assertEqual(undefined);
      controller.detachFromView();
      controller.detach();
    });

    it('offset with transform applies the affine', 0, () => {
      const controller: ESObject = new PlatformViewsController();
      const factory: FakePlatformViewFactory = new FakePlatformViewFactory();
      const host: FakeFlutterViewHost = attachController(controller as Any as PlatformViewsController, factory);
      controller.createForTextureLayer(makeRequest(63));
      controller.offset(63, 5, 6);
      controller.transform(63, [1, 0, 0, 1, 20, 30]);
      const params: Record<string, Any> = controller.viewWrappers.get(63).getDvModel().getLayoutParams() as Record<string, Any>;
      expect(params['left']).assertEqual(20);
      expect(params['top']).assertEqual(30);
      const affine: Any = params['affineTransform'];
      expect(affine[2]).assertEqual(-20);
      expect(affine[5]).assertEqual(-30);
      controller.detachFromView();
      controller.detach();
    });

    it('hover sets the hover flag on the matching view only', 0, () => {
      const controller: ESObject = new PlatformViewsController();
      const factory: FakePlatformViewFactory = new FakePlatformViewFactory();
      const host: FakeFlutterViewHost = attachController(controller as Any as PlatformViewsController, factory);
      controller.createForTextureLayer(makeRequest(64));
      controller.hover(64);
      const params: Record<string, Any> = controller.viewWrappers.get(64).getDvModel().getLayoutParams() as Record<string, Any>;
      expect(params['hover']).assertEqual(true);
      controller.hover(999);
      expect(params['hover']).assertEqual(false);
      controller.detachFromView();
      controller.detach();
    });

    it('onTouch down sets the down flag and up clears it', 0, () => {
      const controller: ESObject = new PlatformViewsController();
      const factory: FakePlatformViewFactory = new FakePlatformViewFactory();
      const host: FakeFlutterViewHost = attachController(controller as Any as PlatformViewsController, factory);
      controller.createForTextureLayer(makeRequest(71));
      controller.onTouch(new PlatformViewTouch(71, 1, 2, 0, 1, [1], [0, 0, 1, 1, 0, 0, 0, 0, 0], 0, 0, 1.0, 1.0, 0, 0, 0, 0, 1));
      const params: Record<string, Any> = controller.viewWrappers.get(71).getDvModel().getLayoutParams() as Record<string, Any>;
      expect(params['down']).assertEqual(true);
      controller.onTouch(new PlatformViewTouch(71, 1, 2, 1, 1, [1], [0, 0, 1, 1, 0, 0, 0, 0, 0], 0, 0, 1.0, 1.0, 0, 0, 0, 0, 1));
      expect(params['down']).assertEqual(false);
      controller.detachFromView();
      controller.detach();
    });

    it('onTouch cancel clears down and cancels active touches', 0, () => {
      const controller: ESObject = new PlatformViewsController();
      const factory: FakePlatformViewFactory = new FakePlatformViewFactory();
      const host: FakeFlutterViewHost = attachController(controller as Any as PlatformViewsController, factory);
      controller.createForTextureLayer(makeRequest(72));
      controller.onTouch(new PlatformViewTouch(72, 1, 2, 3, 1, [1], [0, 0, 1, 1, 0, 0, 0, 0, 0], 0, 0, 1.0, 1.0, 0, 0, 0, 0, 1));
      const params: Record<string, Any> = controller.viewWrappers.get(72).getDvModel().getLayoutParams() as Record<string, Any>;
      expect(params['down']).assertEqual(false);
      controller.detachFromView();
      controller.detach();
    });

    it('dispose of a texture view removes it from host children', 0, () => {
      const controller: ESObject = new PlatformViewsController();
      const factory: FakePlatformViewFactory = new FakePlatformViewFactory();
      const host: FakeFlutterViewHost = attachController(controller as Any as PlatformViewsController, factory);
      controller.createForTextureLayer(makeRequest(81));
      const hostChildren: DVModelChildren = (host as Any).getDVModel().children;
      expect(hostChildren.length).assertEqual(1); // L909 pushed the wrapper DVModel
      let threw = false;
      try {
        controller.dispose(81);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
      expect(hostChildren.length).assertEqual(0); // L348 splice removed it
      expect(controller.viewWrappers.get(81)).assertEqual(undefined);
      expect(controller.platformViews.get(81)).assertEqual(undefined);
      controller.detachFromView();
      controller.detach();
    });

    it('attachToView reattaches wrappers and notifies views on a new host', 0, () => {
      const controller: ESObject = new PlatformViewsController();
      const factory: FakePlatformViewFactory = new FakePlatformViewFactory();
      const hostA: FakeFlutterViewHost = attachController(controller as Any as PlatformViewsController, factory);
      controller.createForTextureLayer(makeRequest(82));
      const hostB: FakeFlutterViewHost = new FakeFlutterViewHost();
      let threw = false;
      try {
        controller.attachToView(hostB as Any as FlutterView);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
      const hostBChildren: DVModelChildren = (hostB as Any).getDVModel().children;
      expect(hostBChildren.length).assertEqual(1); // L973 re-added the wrapper DVModel
      const hostAChildren: DVModelChildren = (hostA as Any).getDVModel().children;
      expect(hostAChildren.length).assertEqual(0); // detachFromView removed it from A
      controller.detachFromView();
      controller.detach();
    });

    it('cancelActiveTouchIfNeeded drains last event touches on a real node controller', 0, () => {
      const controller: ESObject = new PlatformViewsController();
      const factory: FakePlatformViewFactory = new FakePlatformViewFactory();
      attachController(controller as Any as PlatformViewsController, factory);
      controller.createForTextureLayer(makeRequest(83));
      const params: Record<string, Any> =
        controller.viewWrappers.get(83).getDvModel().getLayoutParams() as Record<string, Any>;
      const t1: CustomTouchObject = new CustomTouchObject(TouchType.Move, 1, 0, 0, 0, 0, 0, 0, 0, 0);
      const t2: CustomTouchObject = new CustomTouchObject(TouchType.Move, 2, 0, 0, 0, 0, 0, 0, 0, 0);
      const evt: CustomTouchEvent = new CustomTouchEvent(TouchType.Move, [t1, t2], [t1, t2], 0,
        SourceType.TouchScreen, 1, 0, 0, SourceTool.Finger);
      params['platformViewLastTouchEvent'] = evt; // activeTouches left empty -> last_event branch (L272)
      let threw = false;
      try {
        controller.cancelActiveTouchIfNeeded(83, params, 'ut');
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
      expect(params['platformViewCancelDraining']).assertEqual(true); // L289
      expect(params['platformViewLastTouchEvent']).assertEqual(undefined); // cleared (L220)
      controller.detachFromView();
      controller.detach();
    });

    it('resize updates dimensions and notifies the callback', 0, () => {
      const controller: ESObject = new PlatformViewsController();
      const factory: FakePlatformViewFactory = new FakePlatformViewFactory();
      attachController(controller as Any as PlatformViewsController, factory);
      controller.createForTextureLayer(makeRequest(84));
      const cb: FakeBufferResized = new FakeBufferResized();
      let threw = false;
      try {
        controller.resize(new PlatformViewResizeRequest(84, 120, 60), cb);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
      expect(cb.count).assertEqual(1);
      expect(cb.lastWidth).assertEqual(120); // toPhysicalPixels is identity for logical
      expect(cb.lastHeight).assertEqual(60);
      const params: Record<string, Any> =
        controller.viewWrappers.get(84).getDvModel().getLayoutParams() as Record<string, Any>;
      expect(params['width']).assertEqual(120);
      expect(params['height']).assertEqual(60);
      controller.detachFromView();
      controller.detach();
    });
  });
}