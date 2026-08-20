/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, expect, it } from '@ohos/hypium';
import common from '@ohos.app.ability.common';
import Any from '../../../../../main/ets/plugin/common/Any';
import StringCodec from '../../../../../main/ets/plugin/common/StringCodec';
import DartExecutor from '../../../../../main/ets/embedding/engine/dart/DartExecutor';
import FlutterNapi from '../../../../../main/ets/embedding/engine/FlutterNapi';
import {
  PlatformViewCreationRequest,
  PlatformViewTouch,
  RequestedDisplayMode
} from '../../../../../main/ets/embedding/engine/systemchannels/PlatformViewsChannel';
import PlatformView, { Params } from '../../../../../main/ets/plugin/platform/PlatformView';
import PlatformViewFactory from '../../../../../main/ets/plugin/platform/PlatformViewFactory';
import PlatformViewRegistryImpl from '../../../../../main/ets/plugin/platform/PlatformViewRegistryImpl';
import PlatformViewsController from '../../../../../main/ets/plugin/platform/PlatformViewsController';
import PlatformViewsControllerHybrid from '../../../../../main/ets/plugin/platform/PlatformViewsControllerHybrid';
import { ByteBuffer } from '../../../../../main/ets/util/ByteBuffer';
import { FlutterView } from '../../../../../main/ets/view/FlutterView';
import {
  DVModel,
  DVModelChildren,
  DVModelEvents,
  DVModelParameters
} from '../../../../../main/ets/view/DynamicView/dynamicView';

const VIEW_TYPE: string = 'hcpp/legacy_route_view';
const NON_TEXTURE_FALLBACK: number = -2;

class FakeBinaryMessenger {
  send(channel: String, message: ArrayBuffer | null, callback?: Object | null): void {
  }

  setMessageHandler(channel: String, handler: Object | null): void {
  }

  makeBackgroundTaskQueue(): Object {
    return new Object();
  }
}

class FakePlatformView extends PlatformView {
  disposed: boolean = false;

  getView(): WrappedBuilder<[Params]> {
    return (new Object()) as WrappedBuilder<[Params]>;
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

  create(context: common.Context, viewId: number, args: Any): PlatformView {
    this.created++;
    return new FakePlatformView();
  }
}

class FakeFlutterViewHost {
  private dVModel: DVModel =
    new DVModel('Stack', new DVModelParameters(), new DVModelEvents(), new DVModelChildren(), null);

  getDVModel(): DVModel {
    return this.dVModel;
  }

  getId(): string {
    return 'hcpp-legacy-host';
  }
}

/** Per-case HCPP mock: toggle without native shell / buildinfo. */
class RecordingHybridNapi {
  enabled: boolean = false;

  isHybridCompositionEnabled(): boolean {
    return this.enabled;
  }
}

class RecordingHybridController {
  createCalls: number = 0;
  disposeCalls: number = 0;
  touchCalls: number = 0;
  directionCalls: number = 0;
  clearFocusCalls: number = 0;
  attachCalls: number = 0;
  detachCalls: number = 0;
  lastCreate: PlatformViewCreationRequest | null = null;
  ownedIds: Set<number> = new Set();

  createForPlatformViewLayer(request: PlatformViewCreationRequest): void {
    this.createCalls++;
    this.lastCreate = request;
    this.ownedIds.add(request.viewId);
  }

  dispose(viewId: number): void {
    this.disposeCalls++;
    this.ownedIds.delete(viewId);
  }

  onTouch(_touch: PlatformViewTouch): void {
    this.touchCalls++;
  }

  setDirection(_viewId: number, _direction: Direction): void {
    this.directionCalls++;
  }

  clearFocus(_viewId: number): void {
    this.clearFocusCalls++;
  }

  owns(viewId: number): boolean {
    return this.ownedIds.has(viewId);
  }

  attachToView(_flutterView: FlutterView): void {
    this.attachCalls++;
  }

  detachFromView(_detachingFlutterView?: FlutterView): void {
    this.detachCalls++;
  }
}

function emptyParams(): ByteBuffer {
  return ByteBuffer.from(new ArrayBuffer(0));
}

function makeRequest(viewId: number, displayMode?: RequestedDisplayMode): PlatformViewCreationRequest {
  return new PlatformViewCreationRequest(
    viewId, VIEW_TYPE, 0, 0, 40, 20, Direction.Ltr, emptyParams(), displayMode);
}

function makeTouch(viewId: number): PlatformViewTouch {
  return new PlatformViewTouch(
    viewId, 1, 2, 0, 1, [1], [0, 0, 1, 1, 0, 0, 0, 0, 0],
    0, 0, 1.0, 1.0, 0, 0, 0, 0, 1);
}

function attachTextureController(controller: PlatformViewsController,
  factory: FakePlatformViewFactory): FakeFlutterViewHost {
  const registry: PlatformViewRegistryImpl = controller.getRegistry() as PlatformViewRegistryImpl;
  registry.registerViewFactory(VIEW_TYPE, factory);
  const messenger: FakeBinaryMessenger = new FakeBinaryMessenger();
  controller.attach(getContext() as common.Context, null, messenger as Any as DartExecutor);
  const host: FakeFlutterViewHost = new FakeFlutterViewHost();
  controller.attachToView(host as Any as FlutterView);
  return host;
}

function platformViewCount(controller: PlatformViewsController): number {
  const anyC: Any = controller as Any;
  const views: Map<number, PlatformView> = anyC.platformViews as Map<number, PlatformView>;
  return views.size;
}

export default function PlatformViewsControllerHcppTest() {
  describe('PlatformViewsControllerHcpp', () => {
    it('setHybridController_keeps_empty_platform_views', 0, () => {
      const controller: PlatformViewsController = new PlatformViewsController();
      expect(platformViewCount(controller)).assertEqual(0);
      controller.setHybridController(null, null);
      expect(platformViewCount(controller)).assertEqual(0);
    })

    it('createForPlatformViewLayer_forwards_when_hcpp_enabled', 0, () => {
      const controller: PlatformViewsController = new PlatformViewsController();
      const factory: FakePlatformViewFactory = new FakePlatformViewFactory();
      attachTextureController(controller, factory);
      const hybrid: RecordingHybridController = new RecordingHybridController();
      const napi: RecordingHybridNapi = new RecordingHybridNapi();
      napi.enabled = true;
      controller.setHybridController(hybrid as Any as PlatformViewsControllerHybrid, napi as Any as FlutterNapi);

      controller.createForPlatformViewLayer(makeRequest(401));
      expect(hybrid.createCalls).assertEqual(1);
      expect(hybrid.lastCreate!.viewId).assertEqual(401);
      expect(factory.created).assertEqual(0);
      expect(platformViewCount(controller)).assertEqual(0);
      controller.detachFromView();
      controller.detach();
    })

    it('createForPlatformViewLayer_falls_back_when_hcpp_disabled', 0, () => {
      const controller: PlatformViewsController = new PlatformViewsController();
      const factory: FakePlatformViewFactory = new FakePlatformViewFactory();
      attachTextureController(controller, factory);
      const hybrid: RecordingHybridController = new RecordingHybridController();
      const napi: RecordingHybridNapi = new RecordingHybridNapi();
      napi.enabled = false;
      controller.setHybridController(hybrid as Any as PlatformViewsControllerHybrid, napi as Any as FlutterNapi);

      controller.createForPlatformViewLayer(makeRequest(402));
      expect(hybrid.createCalls).assertEqual(0);
      expect(factory.created).assertEqual(1);
      expect(platformViewCount(controller)).assertEqual(1);
      controller.detachFromView();
      controller.detach();
    })

    it('createForPlatformViewLayer_falls_back_when_hybrid_controller_null', 0, () => {
      const controller: PlatformViewsController = new PlatformViewsController();
      const factory: FakePlatformViewFactory = new FakePlatformViewFactory();
      attachTextureController(controller, factory);
      const napi: RecordingHybridNapi = new RecordingHybridNapi();
      napi.enabled = true;
      controller.setHybridController(null, napi as Any as FlutterNapi);

      controller.createForPlatformViewLayer(makeRequest(403));
      expect(factory.created).assertEqual(1);
      controller.detachFromView();
      controller.detach();
    })

    it('dispose_onTouch_setDirection_clearFocus_forward_owned_views', 0, () => {
      const controller: PlatformViewsController = new PlatformViewsController();
      const factory: FakePlatformViewFactory = new FakePlatformViewFactory();
      attachTextureController(controller, factory);
      const hybrid: RecordingHybridController = new RecordingHybridController();
      hybrid.ownedIds.add(410);
      controller.setHybridController(hybrid as Any as PlatformViewsControllerHybrid, null);

      controller.onTouch(makeTouch(410));
      controller.setDirection(410, Direction.Rtl);
      controller.clearFocus(410);
      expect(hybrid.touchCalls).assertEqual(1);
      expect(hybrid.directionCalls).assertEqual(1);
      expect(hybrid.clearFocusCalls).assertEqual(1);

      // dispose removes ownership; later events must not forward.
      controller.dispose(410);
      expect(hybrid.disposeCalls).assertEqual(1);
      controller.onTouch(makeTouch(410));
      controller.setDirection(410, Direction.Rtl);
      controller.clearFocus(410);
      expect(hybrid.touchCalls).assertEqual(1);
      expect(hybrid.directionCalls).assertEqual(1);
      expect(hybrid.clearFocusCalls).assertEqual(1);

      controller.dispose(999);
      controller.onTouch(makeTouch(999));
      controller.setDirection(999, Direction.Ltr);
      controller.clearFocus(999);
      expect(hybrid.disposeCalls).assertEqual(1);
      expect(hybrid.touchCalls).assertEqual(1);
      controller.detachFromView();
      controller.detach();
    })

    it('createForTextureLayer_routes_hybrid_fallback_when_hcpp_on', 0, () => {
      const controller: PlatformViewsController = new PlatformViewsController();
      const factory: FakePlatformViewFactory = new FakePlatformViewFactory();
      attachTextureController(controller, factory);
      const hybrid: RecordingHybridController = new RecordingHybridController();
      const napi: RecordingHybridNapi = new RecordingHybridNapi();
      napi.enabled = true;
      controller.setHybridController(hybrid as Any as PlatformViewsControllerHybrid, napi as Any as FlutterNapi);

      const textureId: number = controller.createForTextureLayer(
        makeRequest(420, RequestedDisplayMode.TEXTURE_WITH_HYBRID_FALLBACK));
      expect(textureId).assertEqual(NON_TEXTURE_FALLBACK);
      expect(hybrid.createCalls).assertEqual(1);
      expect(factory.created).assertEqual(0);
      controller.detachFromView();
      controller.detach();
    })

    it('createForTextureLayer_skips_hybrid_when_display_mode_or_flag_off', 0, () => {
      const controller: PlatformViewsController = new PlatformViewsController();
      const factory: FakePlatformViewFactory = new FakePlatformViewFactory();
      attachTextureController(controller, factory);
      const hybrid: RecordingHybridController = new RecordingHybridController();
      const napi: RecordingHybridNapi = new RecordingHybridNapi();
      napi.enabled = true;
      controller.setHybridController(hybrid as Any as PlatformViewsControllerHybrid, napi as Any as FlutterNapi);

      try {
        controller.createForTextureLayer(
          makeRequest(421, RequestedDisplayMode.TEXTURE_WITH_VIRTUAL_FALLBACK));
      } catch (_err) {
      }
      expect(hybrid.createCalls).assertEqual(0);

      napi.enabled = false;
      try {
        controller.createForTextureLayer(
          makeRequest(422, RequestedDisplayMode.TEXTURE_WITH_HYBRID_FALLBACK));
      } catch (_err) {
      }
      expect(hybrid.createCalls).assertEqual(0);

      controller.setHybridController(null, napi as Any as FlutterNapi);
      napi.enabled = true;
      try {
        controller.createForTextureLayer(
          makeRequest(423, RequestedDisplayMode.TEXTURE_WITH_HYBRID_FALLBACK));
      } catch (_err) {
      }
      expect(hybrid.createCalls).assertEqual(0);
      controller.detachFromView();
      controller.detach();
    })

    it('attachToView_and_detachFromView_forward_to_hybrid', 0, () => {
      const controller: PlatformViewsController = new PlatformViewsController();
      const hybrid: RecordingHybridController = new RecordingHybridController();
      controller.setHybridController(hybrid as Any as PlatformViewsControllerHybrid, null);
      const host: FakeFlutterViewHost = new FakeFlutterViewHost();
      controller.attachToView(host as Any as FlutterView);
      expect(hybrid.attachCalls).assertEqual(1);
      controller.detachFromView(host as Any as FlutterView);
      expect(hybrid.detachCalls).assertEqual(1);
    })

    it('createPlatformView_decodes_params_via_util', 0, () => {
      const controller: PlatformViewsController = new PlatformViewsController();
      const factory: FakePlatformViewFactory = new FakePlatformViewFactory();
      attachTextureController(controller, factory);
      const encoded: ArrayBuffer = StringCodec.INSTANCE.encodeMessage('legacy-args') as ArrayBuffer;
      const request: PlatformViewCreationRequest = new PlatformViewCreationRequest(
        430, VIEW_TYPE, 0, 0, 10, 10, Direction.Ltr, ByteBuffer.from(encoded));
      controller.createForPlatformViewLayer(request);
      expect(factory.created).assertEqual(1);
      controller.detachFromView();
      controller.detach();
    })
  })
}
