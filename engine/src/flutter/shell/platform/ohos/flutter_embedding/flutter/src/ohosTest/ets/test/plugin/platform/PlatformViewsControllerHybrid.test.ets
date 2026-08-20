/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, expect, it } from '@ohos/hypium';
import common from '@ohos.app.ability.common';
import display from '@ohos.display';
import Any from '../../../../../main/ets/plugin/common/Any';
import StringCodec from '../../../../../main/ets/plugin/common/StringCodec';
import DartExecutor from '../../../../../main/ets/embedding/engine/dart/DartExecutor';
import {
  PlatformViewCreationRequest,
  PlatformViewTouch
} from '../../../../../main/ets/embedding/engine/systemchannels/PlatformViewsChannel';
import FlutterNapi from '../../../../../main/ets/embedding/engine/FlutterNapi';
import PlatformView, { Params } from '../../../../../main/ets/plugin/platform/PlatformView';
import PlatformViewFactory from '../../../../../main/ets/plugin/platform/PlatformViewFactory';
import PlatformViewRegistryImpl from '../../../../../main/ets/plugin/platform/PlatformViewRegistryImpl';
import PlatformViewsControllerHybrid from '../../../../../main/ets/plugin/platform/PlatformViewsControllerHybrid';
import { ByteBuffer } from '../../../../../main/ets/util/ByteBuffer';
import { FlutterView } from '../../../../../main/ets/view/FlutterView';
import {
  DVModel,
  DVModelChildren,
  DVModelEvents,
  DVModelParameters
} from '../../../../../main/ets/view/DynamicView/dynamicView';

const VIEW_TYPE: string = 'hcpp/ut_fake_view';

/** Channel attach only needs BinaryMessenger-shaped methods at runtime. */
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
  attachedCount: number = 0;
  detachedCount: number = 0;

  getView(): WrappedBuilder<[Params]> {
    // createPlatformView only checks non-null; EmbeddingNodeController stores the reference.
    return (new Object()) as WrappedBuilder<[Params]>;
  }

  onFlutterViewAttached(dvModel: DVModel): void {
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
  lastViewId: number = -1;
  lastArgs: Any = undefined;
  created: FakePlatformView | null = null;

  constructor() {
    super(StringCodec.INSTANCE);
  }

  create(context: common.Context, viewId: number, args: Any): PlatformView {
    this.lastViewId = viewId;
    this.lastArgs = args;
    this.created = new FakePlatformView();
    return this.created;
  }
}

/**
 * Minimal FlutterView stand-in: create/dispose only call getDVModel().children.
 * Avoids FlutterManager / WindowStage which fail outside a full Flutter page.
 */
class FakeFlutterViewHost {
  private dVModel: DVModel =
    new DVModel('Stack', new DVModelParameters(), new DVModelEvents(), new DVModelChildren(), null);

  getDVModel(): DVModel {
    return this.dVModel;
  }

  // FlutterView.getId(): onEndFrameHybrid / removeOverlayRectsFromStorage 用它做
  // AppStorage overlay map 的 viewKey，缺方法会让 ?. 链路抛 undefined is not callable。
  getId(): string {
    return 'fake-flutter-view';
  }
}

function makeMutatorsPureTranslate(dpr: number): number[] {
  // Layout mirrors PlatformViewsControllerHybrid private constants:
  // [0]=opacity, [1..16]=matrix, [17]=clipRectsCount; missing trailing counts → 0.
  const data: number[] = [];
  data.push(1); // opacity
  for (let i = 0; i < 16; i++) {
    data.push(0);
  }
  data[1] = dpr; // m00 scaleX
  data[6] = dpr; // m11 scaleY (MATRIX_START+5)
  data.push(0); // clipRectsCount at index 17
  return data;
}

function makeMutatorsWithClipRect(dpr: number): number[] {
  const data: number[] = makeMutatorsPureTranslate(dpr);
  data[0] = 1.5; // invalid opacity → clamped to 1
  data[17] = 1; // clipRectsCount
  data.push(10, 20, 100, 80); // clip rect x,y,w,h
  data.push(0); // clipRRectCount
  data.push(0); // clipPathCount
  return data;
}

function makeMutatorsRotated(dpr: number): number[] {
  const data: number[] = makeMutatorsPureTranslate(dpr);
  // ~90° rotation in column-major 4x4: m00=0 m01=-dpr m10=dpr m11=0
  data[1] = 0;
  data[2] = -dpr;
  data[5] = dpr;
  data[6] = 0;
  data.push(0); // clipRRectCount after clipRectsCount=0 → index 18
  data.push(0); // clipPathCount
  return data;
}

/** Uniform scale (no shear): hasRotation=false, isPureTranslate=false when scaleMul!==1. */
function makeMutatorsUniformScale(dpr: number, scaleMul: number): number[] {
  const data: number[] = [];
  data.push(1); // opacity
  for (let i = 0; i < 16; i++) {
    data.push(0);
  }
  data[1] = dpr * scaleMul; // m00 scaleX
  data[6] = dpr * scaleMul; // m11 scaleY
  data.push(0); // clipRectsCount
  return data;
}

/**
 * Uniform scale + inset clipRect. Scale-only tracks viewX/viewY to the new bbox.
 * Inset clip overflows the 100x50 display shape, so source writes a clip path
 * (empty ∩ would be 'M0 0 L0 0 Z'; no overflow would omit pathCommands).
 */
function makeMutatorsUniformScaleWithClipRect(dpr: number, scaleMul: number,
  nx: number, ny: number, nw: number, nh: number): number[] {
  const data: number[] = makeMutatorsUniformScale(dpr, scaleMul);
  data[17] = 1;
  data.push(nx + 10, ny + 10, Math.max(nw - 20, 20), Math.max(nh - 20, 20));
  data.push(0); // rrects
  data.push(0); // clipPath
  return data;
}

/** rrectsCount=1 → round-rect clip params (with/without displayOffsets). */
function makeMutatorsWithRRect(dpr: number): number[] {
  const data: number[] = makeMutatorsPureTranslate(dpr);
  // clipRectsCount already 0 at [17]
  data.push(1); // rrectsCount
  data.push(0, 0, 344, 200); // bounds
  data.push(12); // radius
  data.push(0); // clipPathCount
  return data;
}

/**
 * Pure-translate + clipRect + polygon clipPath that overflows viewport → SH clip.
 * node origin assumed at (nx, ny); triangle spans far outside a small clip rect.
 */
function makeMutatorsClipPathOverflow(dpr: number, nx: number, ny: number): number[] {
  const data: number[] = makeMutatorsPureTranslate(dpr);
  data[17] = 1; // clipRectsCount
  // small viewport inside node
  data.push(nx + 10, ny + 10, 30, 30);
  data.push(0); // rrectsCount
  data.push(1); // clipPathCount
  // path header: bounds + cmdLen
  data.push(nx, ny, 200, 200);
  // MOVE + LINE + LINE + CLOSE → cmdLen = 3+3+3+1 = 10
  data.push(10);
  // PathVerb.MOVE=1, LINE=2, CLOSE=0
  data.push(1, nx - 50, ny - 50);
  data.push(2, nx + 250, ny - 50);
  data.push(2, nx + 100, ny + 250);
  data.push(0);
  return data;
}

/** Polygon entirely outside viewport → SH yields <3 verts → clipHidden. */
function makeMutatorsClipPathFullyOutside(dpr: number, nx: number, ny: number): number[] {
  const data: number[] = makeMutatorsPureTranslate(dpr);
  data[17] = 1;
  data.push(nx + 100, ny + 100, 40, 40); // viewport far from triangle
  data.push(0);
  data.push(1);
  data.push(nx - 200, ny - 200, 50, 50);
  data.push(10);
  data.push(1, nx - 200, ny - 200);
  data.push(2, nx - 150, ny - 200);
  data.push(2, nx - 175, ny - 150);
  data.push(0);
  return data;
}

/** clipPath with QUAD+CUBIC (skips SH, covers curve verb branches). */
function makeMutatorsClipPathCurves(dpr: number, nx: number, ny: number): number[] {
  const data: number[] = makeMutatorsPureTranslate(dpr);
  data[17] = 0;
  data.push(0); // rrects
  data.push(1); // clipPathCount
  data.push(nx, ny, 100, 100);
  // MOVE(3) + QUAD(1+4=5) + CUBIC(1+6=7) + CLOSE(1) = 16
  data.push(16);
  data.push(1, nx, ny);
  data.push(3, nx + 20, ny + 10, nx + 40, ny); // QUAD
  data.push(4, nx + 50, ny + 5, nx + 60, ny + 15, nx + 80, ny); // CUBIC
  data.push(0);
  return data;
}

/**
 * clipRectsCount>0, clipPathCount=0, intersecting offset clip → hasClipRectOffset path.
 */
function makeMutatorsClipRectOffset(dpr: number, nx: number, ny: number, nw: number, nh: number): number[] {
  const data: number[] = makeMutatorsPureTranslate(dpr);
  data[17] = 1;
  // clip inset from left/top → clipRectX/Y > 0.5
  data.push(nx + 40, ny + 20, nw - 40, nh - 20);
  data.push(0); // rrects
  data.push(0); // clipPath
  return data;
}

/** Same origin as node but shorter height → size-narrow branch (no path offset). */
function makeMutatorsClipRectSizeOnly(dpr: number, nx: number, ny: number, nw: number, nh: number): number[] {
  const data: number[] = makeMutatorsPureTranslate(dpr);
  data[17] = 1;
  data.push(nx, ny, nw, nh / 2);
  data.push(0);
  data.push(0);
  return data;
}

/**
 * clipRect inset + rrectsCount with radius → hasClipRectOffset && rrect flatten path.
 * Triggers rrectToPolyVerts + sutherlandHodgmanClip (viewport ∩ rounded rect).
 */
function makeMutatorsClipRectOffsetWithRRect(dpr: number, nx: number, ny: number,
  nw: number, nh: number, radius: number): number[] {
  const data: number[] = makeMutatorsPureTranslate(dpr);
  data[17] = 1; // clipRectsCount
  // inset from left/top → clipRectX/Y > 0.5 → hasClipRectOffset
  data.push(nx + 40, ny + 20, nw - 40, nh - 20);
  data.push(1); // rrectsCount
  data.push(0, 0, nw, nh); // bounds (node size used when displayOffsets exist)
  data.push(radius); // layer-tree points (=vp), becomes radius*dpr px inside controller
  data.push(0); // clipPathCount
  return data;
}

/**
 * clipRect viewport + QUAD/CUBIC clipPath overflowing it → flattenQuad/Cubic then SH.
 */
function makeMutatorsClipPathCurvesOverflow(dpr: number, nx: number, ny: number): number[] {
  const data: number[] = makeMutatorsPureTranslate(dpr);
  data[17] = 1; // clipRectsCount
  data.push(nx + 10, ny + 10, 40, 40); // small viewport
  data.push(0); // rrectsCount
  data.push(1); // clipPathCount
  data.push(nx, ny, 200, 200);
  // MOVE(3) + QUAD(5) + CUBIC(7) + CLOSE(1) = 16
  data.push(16);
  data.push(1, nx - 30, ny - 30);
  data.push(3, nx + 80, ny - 20, nx + 160, ny - 30); // QUAD far outside
  data.push(4, nx + 180, ny + 40, nx + 200, ny + 100, nx + 120, ny + 180); // CUBIC
  data.push(0);
  return data;
}

/** Rotated matrix + inset clipRect. Clip uses current bbox origin (off.x/off.y). */
function makeMutatorsRotatedWithClipRect(dpr: number, nx: number, ny: number, nw: number, nh: number): number[] {
  const data: number[] = [];
  data.push(1); // opacity
  for (let i = 0; i < 16; i++) {
    data.push(0);
  }
  // ~90° rotation
  data[1] = 0;
  data[2] = -dpr;
  data[5] = dpr;
  data[6] = 0;
  data.push(1); // clipRectsCount
  data.push(nx + 10, ny + 10, nw - 20, nh - 20);
  data.push(0); // rrects
  data.push(0); // clipPath
  return data;
}

/**
 * Polygon fully inside viewport → enters SH gate but overflows=false → svgRaw path.
 */
function makeMutatorsClipPathNoOverflow(dpr: number, nx: number, ny: number): number[] {
  const data: number[] = makeMutatorsPureTranslate(dpr);
  data[17] = 1;
  data.push(nx - 50, ny - 50, 400, 400); // large viewport containing triangle
  data.push(0);
  data.push(1);
  data.push(nx + 10, ny + 10, 80, 80);
  data.push(10);
  data.push(1, nx + 20, ny + 20);
  data.push(2, nx + 80, ny + 20);
  data.push(2, nx + 50, ny + 70);
  data.push(0);
  return data;
}

/** Two clipRects whose intersection is empty / degenerate → maxX<=minX skips SH apply. */
function makeMutatorsClipPathDegenerateViewport(dpr: number, nx: number, ny: number): number[] {
  const data: number[] = makeMutatorsPureTranslate(dpr);
  data[17] = 2;
  data.push(nx, ny, 40, 40);
  data.push(nx + 100, ny + 100, 40, 40); // disjoint → empty intersection
  data.push(0);
  data.push(1);
  data.push(nx, ny, 200, 200);
  data.push(10);
  data.push(1, nx - 10, ny - 10);
  data.push(2, nx + 210, ny - 10);
  data.push(2, nx + 100, ny + 210);
  data.push(0);
  return data;
}

/**
 * Axis-aligned rectangle polygon with vertical edge on clip left → intersectSH denom===0.
 */
function makeMutatorsClipPathVerticalEdgeOnClip(dpr: number, nx: number, ny: number): number[] {
  const data: number[] = makeMutatorsPureTranslate(dpr);
  data[17] = 1;
  // viewport left = nx+20; rectangle left edge sits exactly on that line
  data.push(nx + 20, ny + 20, 40, 40);
  data.push(0);
  data.push(1);
  data.push(nx, ny, 100, 100);
  // MOVE+3 LINE+CLOSE: rectangle overflowing left of viewport
  data.push(13); // 1+2 + 3*(1+2) + 1 = 13
  data.push(1, nx, ny); // left edge x=nx < clip left
  data.push(2, nx + 20, ny); // top-right of left edge at clip boundary
  data.push(2, nx + 20, ny + 80);
  data.push(2, nx, ny + 80);
  data.push(0);
  return data;
}

/** clipPath with MOVE+CLOSE only → polyVerts.length < 3, SH gate skipped. */
function makeMutatorsClipPathTooFewVerts(dpr: number, nx: number, ny: number): number[] {
  const data: number[] = makeMutatorsPureTranslate(dpr);
  data[17] = 1;
  data.push(nx, ny, 50, 50);
  data.push(0);
  data.push(1);
  data.push(nx, ny, 10, 10);
  data.push(4); // MOVE(3)+CLOSE(1)
  data.push(1, nx + 1, ny + 1);
  data.push(0);
  return data;
}

/** clipRects without prior onDisplay (nodeW=0) → hasClipRectOffset stays false. */
function makeMutatorsClipRectOnly(dpr: number): number[] {
  const data: number[] = makeMutatorsPureTranslate(dpr);
  data[17] = 1;
  data.push(0, 0, 50, 50);
  data.push(0);
  data.push(0);
  return data;
}

function makePlatformViewTouch(viewId: number, action: number,
  props?: number[][], coords?: number[][]): PlatformViewTouch {
  return new PlatformViewTouch(
    viewId, 1, 2, action, 1,
    props !== undefined ? props : [[0, 0]],
    coords !== undefined ? coords : [[0, 1, 0, 0, 0, 0, 0, 15, 25]],
    0, 0, 1, 1, 0, 0, 0, 0, 7);
}

class ThrowingDisposePlatformView extends PlatformView {
  getView(): WrappedBuilder<[Params]> {
    return (new Object()) as WrappedBuilder<[Params]>;
  }

  dispose(): void {
    throw new Error('dispose boom');
  }
}

class ThrowingDisposeFactory extends PlatformViewFactory {
  constructor() {
    super(StringCodec.INSTANCE);
  }

  create(context: common.Context, viewId: number, args: Any): PlatformView {
    return new ThrowingDisposePlatformView();
  }
}

/** Mock native HCPP as on; ohosTest buildinfo does not enable HCPP globally. */
class FakeFlutterNapi {
  touchPackets: number = 0;
  lastPackets: Object[] = [];

  isHybridCompositionEnabled(): boolean {
    return true;
  }

  dispatchTouchToEngine(touchData: Object[]): void {
    this.touchPackets++;
    this.lastPackets = touchData;
  }
}

function makeControllerWithFactory(): [PlatformViewsControllerHybrid, FakePlatformViewFactory, PlatformViewRegistryImpl] {
  const registry: PlatformViewRegistryImpl = new PlatformViewRegistryImpl();
  const factory: FakePlatformViewFactory = new FakePlatformViewFactory();
  registry.registerViewFactory(VIEW_TYPE, factory);
  const controller: PlatformViewsControllerHybrid = new PlatformViewsControllerHybrid(registry);
  return [controller, factory, registry];
}

function attachContextAndView(controller: PlatformViewsControllerHybrid): FakeFlutterViewHost {
  const context: common.Context = getContext() as common.Context;
  const messenger: FakeBinaryMessenger = new FakeBinaryMessenger();
  controller.attach(context, messenger as Any as DartExecutor);
  const fakeView: FakeFlutterViewHost = new FakeFlutterViewHost();
  controller.attachToView(fakeView as Any as FlutterView);
  return fakeView;
}

function makeCreateRequest(viewId: number, width: number = 80, height: number = 40): PlatformViewCreationRequest {
  const emptyParams: ByteBuffer = ByteBuffer.from(new ArrayBuffer(0));
  return new PlatformViewCreationRequest(
    viewId,
    VIEW_TYPE,
    10,
    20,
    width,
    height,
    Direction.Ltr,
    emptyParams);
}

export default function PlatformViewsControllerHybridTest() {
  describe('PlatformViewsControllerHybrid', () => {
    it('getRegistry_returns_injected_registry', 0, () => {
      const registry: PlatformViewRegistryImpl = new PlatformViewRegistryImpl();
      const controller: PlatformViewsControllerHybrid = new PlatformViewsControllerHybrid(registry);
      expect(controller.getRegistry()).assertEqual(registry);
    });

    it('owns_unknown_view_id_returns_false', 0, () => {
      const controller: PlatformViewsControllerHybrid =
        new PlatformViewsControllerHybrid(new PlatformViewRegistryImpl());
      expect(controller.owns(1001)).assertEqual(false);
    });

    it('isSurfaceControlEnabled_returns_true', 0, () => {
      const controller: PlatformViewsControllerHybrid =
        new PlatformViewsControllerHybrid(new PlatformViewRegistryImpl());
      expect(controller.isSurfaceControlEnabled()).assertEqual(true);
    });

    it('setOverscrollOffset_does_not_throw', 0, () => {
      const controller: PlatformViewsControllerHybrid =
        new PlatformViewsControllerHybrid(new PlatformViewRegistryImpl());
      controller.setOverscrollOffset(7, -12.5);
      controller.setOverscrollOffset(7, -12.505);
      expect(controller.owns(7)).assertEqual(false);
    });

    it('detach_without_attach_is_safe', 0, () => {
      const controller: PlatformViewsControllerHybrid =
        new PlatformViewsControllerHybrid(new PlatformViewRegistryImpl());
      controller.detach();
      expect(controller.owns(1)).assertEqual(false);
    });

    it('onPreEngineRestart_with_empty_views_is_safe', 0, () => {
      const controller: PlatformViewsControllerHybrid =
        new PlatformViewsControllerHybrid(new PlatformViewRegistryImpl());
      controller.onPreEngineRestart();
      expect(controller.owns(1)).assertEqual(false);
    });

    it('onDetachedFromNapi_with_empty_views_is_safe', 0, () => {
      const controller: PlatformViewsControllerHybrid =
        new PlatformViewsControllerHybrid(new PlatformViewRegistryImpl());
      controller.onDetachedFromNapi();
      expect(controller.owns(1)).assertEqual(false);
    });

    it('setDirection_unknown_view_is_noop', 0, () => {
      const controller: PlatformViewsControllerHybrid =
        new PlatformViewsControllerHybrid(new PlatformViewRegistryImpl());
      controller.setDirection(999, Direction.Rtl);
      expect(controller.owns(999)).assertEqual(false);
    });

    it('hidePlatformViewHybrid_unknown_view_is_noop', 0, () => {
      const controller: PlatformViewsControllerHybrid =
        new PlatformViewsControllerHybrid(new PlatformViewRegistryImpl());
      controller.hidePlatformViewHybrid(123);
      expect(controller.owns(123)).assertEqual(false);
    });

    it('onBeginFrameHybrid_and_onEndFrameHybrid_empty_safe', 0, () => {
      const controller: PlatformViewsControllerHybrid =
        new PlatformViewsControllerHybrid(new PlatformViewRegistryImpl());
      controller.onBeginFrameHybrid();
      controller.onEndFrameHybrid();
      expect(controller.owns(1)).assertEqual(false);
    });

    it('onDisplayPlatformViewHybrid_unknown_view_is_noop', 0, () => {
      const controller: PlatformViewsControllerHybrid =
        new PlatformViewsControllerHybrid(new PlatformViewRegistryImpl());
      controller.onDisplayPlatformViewHybrid(55, 0, 0, 100, 100, 100, 100);
      expect(controller.owns(55)).assertEqual(false);
    });

    it('onDisplayOverlayHybrid_unknown_safe', 0, () => {
      const controller: PlatformViewsControllerHybrid =
        new PlatformViewsControllerHybrid(new PlatformViewRegistryImpl());
      controller.onDisplayOverlayHybrid(1, 0, 0, 10, 10);
      expect(controller.owns(1)).assertEqual(false);
    });

    it('showOverlaySurfaceHybrid_and_hide_safe', 0, () => {
      const controller: PlatformViewsControllerHybrid =
        new PlatformViewsControllerHybrid(new PlatformViewRegistryImpl());
      controller.showOverlaySurfaceHybrid();
      controller.hideOverlaySurfaceHybrid();
      expect(controller.isSurfaceControlEnabled()).assertEqual(true);
    });

    it('createForPlatformViewLayer_without_view_throws', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const context: common.Context = getContext() as common.Context;
      const messenger: FakeBinaryMessenger = new FakeBinaryMessenger();
      controller.attach(context, messenger as Any as DartExecutor);
      let threw: boolean = false;
      try {
        controller.createForPlatformViewLayer(makeCreateRequest(201));
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
      expect(controller.owns(201)).assertEqual(false);
    });

    it('createForPlatformViewLayer_unregistered_type_throws', 0, () => {
      const registry: PlatformViewRegistryImpl = new PlatformViewRegistryImpl();
      const controller: PlatformViewsControllerHybrid = new PlatformViewsControllerHybrid(registry);
      attachContextAndView(controller);
      const emptyParams: ByteBuffer = ByteBuffer.from(new ArrayBuffer(0));
      const request: PlatformViewCreationRequest = new PlatformViewCreationRequest(
        202, 'hcpp/missing', 0, 0, 10, 10, Direction.Ltr, emptyParams);
      let threw: boolean = false;
      try {
        controller.createForPlatformViewLayer(request);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
      expect(controller.owns(202)).assertEqual(false);
      controller.detachFromView();
      controller.detach();
    });

    it('create_display_hide_dispose_lifecycle', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const factory: FakePlatformViewFactory = pair[1];
      const fakeView: FakeFlutterViewHost = attachContextAndView(controller);
      const viewId: number = 301;

      controller.createForPlatformViewLayer(makeCreateRequest(viewId, 120, 60));
      expect(controller.owns(viewId)).assertEqual(true);
      expect(factory.lastViewId).assertEqual(viewId);
      expect(factory.created).not().assertNull();
      expect(factory.created!.viewId).assertEqual(viewId);
      expect(factory.created!.touchDispatcher).not().assertNull();
      expect(factory.created!.axisDispatcher).not().assertNull();
      expect(fakeView.getDVModel().children.length).assertEqual(1);

      controller.setOverscrollOffset(viewId, -8);
      controller.onDisplayPlatformViewHybrid(viewId, 30, 40, 200, 100, 200, 100);
      const paramsAfterDisplay: Record<string, Any> =
        fakeView.getDVModel().children[0].params as Record<string, Any>;
      expect(paramsAfterDisplay['width'] !== undefined).assertEqual(true);
      expect(paramsAfterDisplay['height'] !== undefined).assertEqual(true);
      // overscrollOffset=-8 → top = px2vp(40) - (-8) = px2vp(40)+8
      const expectedTop: number = px2vp(40) + 8;
      expect(Math.abs((paramsAfterDisplay['top'] as number) - expectedTop) < 0.01).assertEqual(true);
      expect(Math.abs((paramsAfterDisplay['left'] as number) - px2vp(30)) < 0.01).assertEqual(true);

      const dpr: number = display.getDefaultDisplaySync().densityPixels;
      controller.onDisplayMutatorsHybrid(viewId, makeMutatorsPureTranslate(dpr));
      expect((fakeView.getDVModel().children[0].params as Record<string, Any>)['opacity'])
        .assertEqual(1);

      controller.onDisplayOverlayHybrid(viewId, 0, 0, 50, 50);
      controller.onBeginFrameHybrid();
      controller.onEndFrameHybrid();

      controller.hidePlatformViewHybrid(viewId);
      const paramsHidden: Record<string, Any> =
        fakeView.getDVModel().children[0].params as Record<string, Any>;
      expect(paramsHidden['width']).assertEqual(0);
      expect(paramsHidden['height']).assertEqual(0);

      controller.dispose(viewId);
      expect(controller.owns(viewId)).assertEqual(false);
      expect(factory.created!.disposed).assertEqual(true);
      expect(fakeView.getDVModel().children.length).assertEqual(0);

      controller.detachFromView();
      controller.detach();
    });

    it('onDisplayMutatorsHybrid_truncated_data_is_safe', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      attachContextAndView(controller);
      const viewId: number = 302;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      controller.onDisplayMutatorsHybrid(viewId, [1, 0, 0]);
      expect(controller.owns(viewId)).assertEqual(true);
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    it('dispose_unknown_view_is_noop', 0, () => {
      const controller: PlatformViewsControllerHybrid =
        new PlatformViewsControllerHybrid(new PlatformViewRegistryImpl());
      controller.dispose(404);
      expect(controller.owns(404)).assertEqual(false);
    });

    it('attach_detach_reattach_to_same_view_is_idempotent', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const fakeView: FakeFlutterViewHost = attachContextAndView(controller);
      controller.attachToView(fakeView as Any as FlutterView);
      controller.createForPlatformViewLayer(makeCreateRequest(303));
      expect(controller.owns(303)).assertEqual(true);
      controller.dispose(303);
      controller.detachFromView(fakeView as Any as FlutterView);
      controller.detach();
      expect(controller.owns(303)).assertEqual(false);
    });

    it('onDisplayMutatorsHybrid_clip_rect_and_invalid_opacity', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const fakeView: FakeFlutterViewHost = attachContextAndView(controller);
      const viewId: number = 310;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      controller.onDisplayPlatformViewHybrid(viewId, 0, 0, 200, 100, 200, 100);
      const dpr: number = display.getDefaultDisplaySync().densityPixels;
      controller.onDisplayMutatorsHybrid(viewId, makeMutatorsWithClipRect(dpr));
      const params: Record<string, Any> =
        fakeView.getDVModel().children[0].params as Record<string, Any>;
      expect(params['opacity']).assertEqual(1);
      expect(controller.owns(viewId)).assertEqual(true);
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    it('onDisplayMutatorsHybrid_rotated_matrix_path', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const fakeView: FakeFlutterViewHost = attachContextAndView(controller);
      const viewId: number = 311;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      // width/height != viewWidth/viewHeight → rotated displayOffsets branch
      controller.onDisplayPlatformViewHybrid(viewId, 10, 20, 240, 180, 200, 100);
      const dpr: number = display.getDefaultDisplaySync().densityPixels;
      controller.onDisplayMutatorsHybrid(viewId, makeMutatorsRotated(dpr));
      const params: Record<string, Any> =
        fakeView.getDVModel().children[0].params as Record<string, Any>;
      expect(params['rotate'] !== undefined).assertEqual(true);
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    it('onTouch_unknown_view_is_noop', 0, () => {
      const controller: PlatformViewsControllerHybrid =
        new PlatformViewsControllerHybrid(new PlatformViewRegistryImpl());
      controller.onTouch(makePlatformViewTouch(404, 0));
      expect(controller.owns(404)).assertEqual(false);
    });

    it('onTouch_down_up_on_created_view', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const fakeView: FakeFlutterViewHost = attachContextAndView(controller);
      const viewId: number = 312;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      const params: Record<string, Any> =
        fakeView.getDVModel().children[0].getLayoutParams() as Record<string, Any>;
      try {
        controller.onTouch(makePlatformViewTouch(viewId, 0)); // ACTION_DOWN
      } catch (e) {
        // postEvent may throw without real BuilderNode; down flag is set first
      }
      expect(params['down']).assertEqual(true);
      try {
        controller.onTouch(makePlatformViewTouch(viewId, 1)); // ACTION_UP
      } catch (e) {
      }
      expect(params['down']).assertEqual(false);
      expect(controller.owns(viewId)).assertEqual(true);
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    it('dispatchTouchToEngine_without_napi_or_offset_is_safe', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      attachContextAndView(controller);
      const viewId: number = 313;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      // no napi → early return
      controller.dispatchTouchToEngine(viewId, {
        type: TouchType.Down,
        timestamp: 1000,
        changedTouches: [{ id: 1, windowX: 10, windowY: 20 }]
      } as TouchEvent);
      const fakeNapi: FakeFlutterNapi = new FakeFlutterNapi();
      controller.setFlutterNapi(fakeNapi as Any as FlutterNapi);
      // still no displayOffsets → early return
      controller.dispatchTouchToEngine(viewId, {
        type: TouchType.Down,
        timestamp: 1000,
        changedTouches: [{ id: 1, windowX: 10, windowY: 20 }]
      } as TouchEvent);
      expect(fakeNapi.touchPackets).assertEqual(0);
      controller.onDisplayPlatformViewHybrid(viewId, 0, 0, 100, 50, 100, 50);
      controller.dispatchTouchToEngine(viewId, {
        type: TouchType.Down,
        timestamp: 2000000,
        changedTouches: [{ id: 1, windowX: 10, windowY: 20 }]
      } as TouchEvent);
      expect(fakeNapi.touchPackets).assertEqual(1);
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    it('onDisplay_scale_bbox_mutators_track_viewXY', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const fakeView: FakeFlutterViewHost = attachContextAndView(controller);
      const viewId: number = 314;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      const dpr: number = display.getDefaultDisplaySync().densityPixels;
      // First frame at (5,6). Scale-only (no shear) must not freeze viewX/viewY.
      controller.onDisplayPlatformViewHybrid(viewId, 5, 6, 100, 50, 100, 50);
      controller.onDisplayMutatorsHybrid(viewId, makeMutatorsUniformScale(dpr, 1.4));
      // Move to a far bbox. If viewX stayed at 5, clip ∩ node is empty →
      // pathCommands='M0 0 L0 0 Z'. Tracking to (200,220) keeps ∩ non-empty.
      // Inset clip still overflows the 100x50 display shape, so a clip path is written.
      controller.onDisplayPlatformViewHybrid(viewId, 200, 220, 140, 80, 100, 50);
      controller.onDisplayMutatorsHybrid(viewId,
        makeMutatorsUniformScaleWithClipRect(dpr, 1.4, 200, 220, 140, 80));
      const params: Record<string, Any> =
        fakeView.getDVModel().children[0].params as Record<string, Any>;
      expect(params['pathCommands'] !== undefined).assertEqual(true);
      expect(params['pathCommands']).not().assertEqual('M0 0 L0 0 Z');
      expect((params['scaleX'] as number) > 1).assertEqual(true);
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    it('onDisplay_rotated_mutators_clips_against_current_bbox', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const fakeView: FakeFlutterViewHost = attachContextAndView(controller);
      const viewId: number = 344;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      const dpr: number = display.getDefaultDisplaySync().densityPixels;
      controller.onDisplayPlatformViewHybrid(viewId, 5, 6, 100, 50, 100, 50);
      controller.onDisplayMutatorsHybrid(viewId, makeMutatorsPureTranslate(dpr));
      // Rotation freezes viewX/viewY at (5,6), but clip/layout anchors on the
      // current bbox origin (200,220) + centerOffset. Clip at the new bbox
      // therefore intersects the laid-out node — not empty ∩ ('M0 0 L0 0 Z').
      // Inset clip + 90° rotation overflows the display shape → SH clip path.
      controller.onDisplayPlatformViewHybrid(viewId, 200, 220, 140, 80, 100, 50);
      controller.onDisplayMutatorsHybrid(viewId,
        makeMutatorsRotatedWithClipRect(dpr, 200, 220, 140, 80));
      const params: Record<string, Any> =
        fakeView.getDVModel().children[0].params as Record<string, Any>;
      expect(params['pathCommands'] !== undefined).assertEqual(true);
      expect(params['pathCommands']).not().assertEqual('M0 0 L0 0 Z');
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    it('detachFromView_mismatched_view_is_noop', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const fakeView: FakeFlutterViewHost = attachContextAndView(controller);
      const other: FakeFlutterViewHost = new FakeFlutterViewHost();
      controller.detachFromView(other as Any as FlutterView);
      controller.createForPlatformViewLayer(makeCreateRequest(315));
      expect(controller.owns(315)).assertEqual(true);
      controller.dispose(315);
      controller.detachFromView(fakeView as Any as FlutterView);
      controller.detach();
    });

    it('onPreEngineRestart_disposes_created_views', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      attachContextAndView(controller);
      controller.createForPlatformViewLayer(makeCreateRequest(316));
      expect(controller.owns(316)).assertEqual(true);
      controller.onPreEngineRestart();
      expect(controller.owns(316)).assertEqual(false);
      controller.detachFromView();
      controller.detach();
    });

    it('clearFocus_and_dispatchAxis_without_napi_safe', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      attachContextAndView(controller);
      const viewId: number = 317;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      try {
        controller.clearFocus(viewId);
      } catch (e) {
      }
      // no napi → early return
      controller.dispatchAxisToEngine(viewId, {
        action: 2,
        windowX: 1,
        windowY: 2,
        timestamp: 1000,
        getHorizontalAxisValue: (): number => 0,
        getVerticalAxisValue: (): number => 3
      } as AxisEvent);
      const fakeNapi: FakeFlutterNapi = new FakeFlutterNapi();
      controller.setFlutterNapi(fakeNapi as Any as FlutterNapi);
      controller.dispatchAxisToEngine(viewId, {
        action: 1, // not UPDATE
        windowX: 1,
        windowY: 2,
        timestamp: 1000,
        getHorizontalAxisValue: (): number => 1,
        getVerticalAxisValue: (): number => 1
      } as AxisEvent);
      expect(fakeNapi.touchPackets).assertEqual(0);
      // zero scroll skipped
      controller.dispatchAxisToEngine(viewId, {
        action: 2,
        windowX: 1,
        windowY: 2,
        timestamp: 1000,
        getHorizontalAxisValue: (): number => 0,
        getVerticalAxisValue: (): number => 0
      } as AxisEvent);
      expect(fakeNapi.touchPackets).assertEqual(0);
      // non-zero scroll injected
      controller.dispatchAxisToEngine(viewId, {
        action: 2,
        windowX: 3,
        windowY: 4,
        timestamp: 2000000,
        getHorizontalAxisValue: (): number => 1.5,
        getVerticalAxisValue: (): number => -2
      } as AxisEvent);
      expect(fakeNapi.touchPackets).assertEqual(1);
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    it('mutators_rrect_with_and_without_display_offset', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const fakeView: FakeFlutterViewHost = attachContextAndView(controller);
      const viewId: number = 320;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      const dpr: number = display.getDefaultDisplaySync().densityPixels;
      // no displayOffsets yet → rrect bounds fallback
      controller.onDisplayMutatorsHybrid(viewId, makeMutatorsWithRRect(dpr));
      controller.onDisplayPlatformViewHybrid(viewId, 0, 0, 200, 100, 200, 100);
      controller.onDisplayMutatorsHybrid(viewId, makeMutatorsWithRRect(dpr));
      const params: Record<string, Any> =
        fakeView.getDVModel().children[0].params as Record<string, Any>;
      expect(params['rectRadius']).assertEqual(12);
      expect((params['rectWidth'] as number) > 0).assertEqual(true);
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    it('mutators_clipPath_polygon_triggers_sutherland_hodgman', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const fakeView: FakeFlutterViewHost = attachContextAndView(controller);
      const viewId: number = 321;
      const nx: number = 50;
      const ny: number = 60;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      controller.onDisplayPlatformViewHybrid(viewId, nx, ny, 200, 200, 200, 200);
      const dpr: number = display.getDefaultDisplaySync().densityPixels;
      controller.onDisplayMutatorsHybrid(viewId, makeMutatorsClipPathOverflow(dpr, nx, ny));
      const params: Record<string, Any> =
        fakeView.getDVModel().children[0].params as Record<string, Any>;
      expect(params['pathCommands'] !== undefined).assertEqual(true);
      expect(params['rectWidth']).assertEqual(0);
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    it('mutators_clipPath_fully_outside_hides_path', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const fakeView: FakeFlutterViewHost = attachContextAndView(controller);
      const viewId: number = 322;
      const nx: number = 0;
      const ny: number = 0;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      controller.onDisplayPlatformViewHybrid(viewId, nx, ny, 300, 300, 300, 300);
      const dpr: number = display.getDefaultDisplaySync().densityPixels;
      controller.onDisplayMutatorsHybrid(viewId, makeMutatorsClipPathFullyOutside(dpr, nx, ny));
      const params: Record<string, Any> =
        fakeView.getDVModel().children[0].params as Record<string, Any>;
      expect(params['pathWidth']).assertEqual(0);
      expect(params['pathHeight']).assertEqual(0);
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    it('mutators_clipPath_quad_cubic_verbs', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const fakeView: FakeFlutterViewHost = attachContextAndView(controller);
      const viewId: number = 323;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      controller.onDisplayPlatformViewHybrid(viewId, 0, 0, 120, 120, 120, 120);
      const dpr: number = display.getDefaultDisplaySync().densityPixels;
      controller.onDisplayMutatorsHybrid(viewId, makeMutatorsClipPathCurves(dpr, 0, 0));
      const params: Record<string, Any> =
        fakeView.getDVModel().children[0].params as Record<string, Any>;
      const cmds: string = params['pathCommands'] as string;
      expect(cmds.indexOf('Q') >= 0).assertEqual(true);
      expect(cmds.indexOf('C') >= 0).assertEqual(true);
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    it('mutators_clipRect_offset_and_size_only', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const fakeView: FakeFlutterViewHost = attachContextAndView(controller);
      const viewId: number = 324;
      const nx: number = 100;
      const ny: number = 80;
      const nw: number = 200;
      const nh: number = 100;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      controller.onDisplayPlatformViewHybrid(viewId, nx, ny, nw, nh, nw, nh);
      const dpr: number = display.getDefaultDisplaySync().densityPixels;
      controller.onDisplayMutatorsHybrid(viewId, makeMutatorsClipRectOffset(dpr, nx, ny, nw, nh));
      let params: Record<string, Any> =
        fakeView.getDVModel().children[0].params as Record<string, Any>;
      expect(params['pathCommands'] !== undefined).assertEqual(true);
      controller.onDisplayMutatorsHybrid(viewId, makeMutatorsClipRectSizeOnly(dpr, nx, ny, nw, nh));
      params = fakeView.getDVModel().children[0].params as Record<string, Any>;
      expect((params['rectHeight'] as number) > 0).assertEqual(true);
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    it('onTouch_action_variants_and_convert_failures', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const fakeView: FakeFlutterViewHost = attachContextAndView(controller);
      const viewId: number = 325;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      const params: Record<string, Any> =
        fakeView.getDVModel().children[0].getLayoutParams() as Record<string, Any>;

      try {
        controller.onTouch(makePlatformViewTouch(viewId, 0));
      } catch (e) {
      }
      expect(params['down']).assertEqual(true);

      try {
        controller.onTouch(makePlatformViewTouch(viewId, 2)); // MOVE
      } catch (e) {
      }
      expect(params['down']).assertEqual(true);

      try {
        controller.onTouch(makePlatformViewTouch(viewId, 5)); // POINTER_DOWN
      } catch (e) {
      }
      expect(params['down']).assertEqual(true);

      try {
        controller.onTouch(makePlatformViewTouch(viewId, 6)); // POINTER_UP
      } catch (e) {
      }
      expect(params['down']).assertEqual(true);

      try {
        controller.onTouch(makePlatformViewTouch(viewId, 3)); // CANCEL
      } catch (e) {
      }
      expect(params['down']).assertEqual(false);

      try {
        controller.onTouch(makePlatformViewTouch(viewId, 99)); // unknown → convert null
      } catch (e) {
      }
      expect(controller.owns(viewId)).assertEqual(true);

      try {
        controller.onTouch(makePlatformViewTouch(viewId, 0, [], [])); // empty arrays
      } catch (e) {
      }
      expect(controller.owns(viewId)).assertEqual(true);
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    it('touch_and_axis_dispatcher_lambdas', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const factory: FakePlatformViewFactory = pair[1];
      attachContextAndView(controller);
      const viewId: number = 326;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      controller.onDisplayPlatformViewHybrid(viewId, 0, 0, 100, 50, 100, 50);
      const fakeNapi: FakeFlutterNapi = new FakeFlutterNapi();
      controller.setFlutterNapi(fakeNapi as Any as FlutterNapi);
      expect(factory.created!.touchDispatcher).not().assertNull();
      expect(factory.created!.axisDispatcher).not().assertNull();
      try {
        factory.created!.touchDispatcher!(viewId, {
          type: TouchType.Down,
          timestamp: 3000000,
          changedTouches: [{ id: 1, windowX: 5, windowY: 6 }]
        } as TouchEvent);
      } catch (e) {
      }
      expect(fakeNapi.touchPackets).assertEqual(1);
      try {
        factory.created!.axisDispatcher!(viewId, {
          action: 2,
          windowX: 1,
          windowY: 2,
          timestamp: 4000000,
          getHorizontalAxisValue: (): number => 2,
          getVerticalAxisValue: (): number => 0
        } as AxisEvent);
      } catch (e) {
      }
      expect(fakeNapi.touchPackets).assertEqual(2);
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    it('attachToView_reattach_and_detachFromView_with_wrappers', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const factory: FakePlatformViewFactory = pair[1];
      const first: FakeFlutterViewHost = attachContextAndView(controller);
      controller.createForPlatformViewLayer(makeCreateRequest(327));
      expect(first.getDVModel().children.length).assertEqual(1);
      expect(factory.created!.attachedCount).assertEqual(1);
      const second: FakeFlutterViewHost = new FakeFlutterViewHost();
      controller.attachToView(second as Any as FlutterView);
      expect(second.getDVModel().children.length).assertEqual(1);
      expect(factory.created!.attachedCount).assertEqual(2);
      controller.detachFromView(second as Any as FlutterView);
      expect(factory.created!.detachedCount).assertEqual(1);
      expect(controller.getRegistry()).not().assertNull();
      // recreate after detachFromView cleared flutterView
      attachContextAndView(controller);
      expect(controller.owns(327)).assertEqual(true);
      expect(factory.created!.attachedCount).assertEqual(3);
      controller.dispose(327);
      controller.detachFromView();
      controller.detach();
    });

    it('dispose_swallows_platform_view_dispose_throw', 0, () => {
      const registry: PlatformViewRegistryImpl = new PlatformViewRegistryImpl();
      registry.registerViewFactory(VIEW_TYPE, new ThrowingDisposeFactory());
      const controller: PlatformViewsControllerHybrid = new PlatformViewsControllerHybrid(registry);
      attachContextAndView(controller);
      controller.createForPlatformViewLayer(makeCreateRequest(328));
      expect(controller.owns(328)).assertEqual(true);
      controller.dispose(328); // should not throw
      expect(controller.owns(328)).assertEqual(false);
      controller.detachFromView();
      controller.detach();
    });

    it('create_with_string_params_and_setDirection', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const factory: FakePlatformViewFactory = pair[1];
      attachContextAndView(controller);
      const encoded: ArrayBuffer = StringCodec.INSTANCE.encodeMessage('pv-args') as ArrayBuffer;
      const request: PlatformViewCreationRequest = new PlatformViewCreationRequest(
        329, VIEW_TYPE, 0, 0, 40, 20, Direction.Ltr, ByteBuffer.from(encoded));
      controller.createForPlatformViewLayer(request);
      expect(factory.lastArgs).assertEqual('pv-args');
      try {
        controller.setDirection(329, Direction.Rtl);
      } catch (e) {
      }
      controller.onDetachedFromNapi();
      expect(controller.owns(329)).assertEqual(false);
      controller.detachFromView();
      controller.detach();
    });

    it('overlay_end_frame_flushes_pending_rects', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      attachContextAndView(controller);
      controller.createForPlatformViewLayer(makeCreateRequest(330));
      controller.onBeginFrameHybrid();
      controller.onDisplayOverlayHybrid(330, 10, 20, 30, 40);
      controller.onDisplayOverlayHybrid(330, 0, 0, 0, 0); // invisible
      controller.onEndFrameHybrid();
      controller.onEndFrameHybrid(); // empty flush
      controller.dispose(330);
      controller.detachFromView();
      controller.detach();
    });

    it('negative_opacity_clamped_and_pure_translate_clears_rotate', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const fakeView: FakeFlutterViewHost = attachContextAndView(controller);
      const viewId: number = 331;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      controller.onDisplayPlatformViewHybrid(viewId, 0, 0, 100, 50, 100, 50);
      const dpr: number = display.getDefaultDisplaySync().densityPixels;
      controller.onDisplayMutatorsHybrid(viewId, makeMutatorsRotated(dpr));
      const data: number[] = makeMutatorsPureTranslate(dpr);
      data[0] = -0.5; // clamp
      controller.onDisplayMutatorsHybrid(viewId, data);
      const params: Record<string, Any> =
        fakeView.getDVModel().children[0].params as Record<string, Any>;
      expect(params['opacity']).assertEqual(1);
      expect(params['rotate']).assertEqual(0);
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    it('mutators_rotated_with_clipRect_uses_viewXY', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const fakeView: FakeFlutterViewHost = attachContextAndView(controller);
      const viewId: number = 332;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      // rotated bbox larger than viewW/viewH
      controller.onDisplayPlatformViewHybrid(viewId, 10, 20, 140, 120, 100, 80);
      const dpr: number = display.getDefaultDisplaySync().densityPixels;
      controller.onDisplayMutatorsHybrid(viewId, makeMutatorsRotatedWithClipRect(dpr, 10, 20, 140, 120));
      const params: Record<string, Any> =
        fakeView.getDVModel().children[0].params as Record<string, Any>;
      expect(params['rotate'] !== undefined).assertEqual(true);
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    it('mutators_clipPath_inside_viewport_skips_sh_apply', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const fakeView: FakeFlutterViewHost = attachContextAndView(controller);
      const viewId: number = 333;
      const nx: number = 0;
      const ny: number = 0;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      controller.onDisplayPlatformViewHybrid(viewId, nx, ny, 300, 300, 300, 300);
      const dpr: number = display.getDefaultDisplaySync().densityPixels;
      controller.onDisplayMutatorsHybrid(viewId, makeMutatorsClipPathNoOverflow(dpr, nx, ny));
      const params: Record<string, Any> =
        fakeView.getDVModel().children[0].params as Record<string, Any>;
      const cmds: string = params['pathCommands'] as string;
      expect(cmds.indexOf('M') >= 0).assertEqual(true);
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    it('mutators_clipPath_degenerate_and_few_verts_and_vertical_edge', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      attachContextAndView(controller);
      const viewId: number = 334;
      const nx: number = 0;
      const ny: number = 0;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      controller.onDisplayPlatformViewHybrid(viewId, nx, ny, 300, 300, 300, 300);
      const dpr: number = display.getDefaultDisplaySync().densityPixels;
      controller.onDisplayMutatorsHybrid(viewId, makeMutatorsClipPathDegenerateViewport(dpr, nx, ny));
      controller.onDisplayMutatorsHybrid(viewId, makeMutatorsClipPathTooFewVerts(dpr, nx, ny));
      controller.onDisplayMutatorsHybrid(viewId, makeMutatorsClipPathVerticalEdgeOnClip(dpr, nx, ny));
      expect(controller.owns(viewId)).assertEqual(true);
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    it('mutators_clipRect_before_display_and_hide_created', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const fakeView: FakeFlutterViewHost = attachContextAndView(controller);
      const viewId: number = 335;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      const dpr: number = display.getDefaultDisplaySync().densityPixels;
      // no displayOffsets yet → nodeW=0 branch
      controller.onDisplayMutatorsHybrid(viewId, makeMutatorsClipRectOnly(dpr));
      controller.hidePlatformViewHybrid(viewId);
      const params: Record<string, Any> =
        fakeView.getDVModel().children[0].params as Record<string, Any>;
      expect(params['width']).assertEqual(0);
      expect(params['height']).assertEqual(0);
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    it('dispatchTouchToEngine_all_touch_types_and_unknown', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      attachContextAndView(controller);
      const viewId: number = 336;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      controller.onDisplayPlatformViewHybrid(viewId, 0, 0, 100, 50, 100, 50);
      const fakeNapi: FakeFlutterNapi = new FakeFlutterNapi();
      controller.setFlutterNapi(fakeNapi as Any as FlutterNapi);
      const types: TouchType[] = [TouchType.Down, TouchType.Move, TouchType.Up, TouchType.Cancel];
      for (let i = 0; i < types.length; i++) {
        controller.dispatchTouchToEngine(viewId, {
          type: types[i],
          timestamp: 1000000 * (i + 1),
          changedTouches: [{ id: i + 1, windowX: 2, windowY: 3 }]
        } as TouchEvent);
      }
      expect(fakeNapi.touchPackets).assertEqual(4);
      // unknown type → early return, empty touches → no dispatch
      controller.dispatchTouchToEngine(viewId, {
        type: 99 as TouchType,
        timestamp: 9000000,
        changedTouches: [{ id: 9, windowX: 1, windowY: 1 }]
      } as TouchEvent);
      controller.dispatchTouchToEngine(viewId, {
        type: TouchType.Down,
        timestamp: 9000001,
        changedTouches: [] as Any
      } as Any as TouchEvent);
      expect(fakeNapi.touchPackets).assertEqual(4);
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    it('onTouch_encoded_pointer_index_and_focus_clear_on_dispose', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const fakeView: FakeFlutterViewHost = attachContextAndView(controller);
      const viewId: number = 337;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      const params: Record<string, Any> =
        fakeView.getDVModel().children[0].getLayoutParams() as Record<string, Any>;
      const twoProps: number[][] = [[0, 0], [1, 0]];
      const twoCoords: number[][] = [
        [0, 1, 0, 0, 0, 0, 0, 10, 20],
        [0, 1, 0, 0, 0, 0, 0, 30, 40]
      ];
      try {
        controller.onTouch(makePlatformViewTouch(viewId, 0, twoProps, twoCoords)); // DOWN
      } catch (e) {
      }
      expect(params['down']).assertEqual(true);
      try {
        // action = POINTER_DOWN | (1 << 8) = 261; MASK keeps down=true
        controller.onTouch(makePlatformViewTouch(viewId, 261, twoProps, twoCoords));
        controller.onTouch(makePlatformViewTouch(viewId, 5, twoProps, twoCoords));
        controller.onTouch(makePlatformViewTouch(viewId, 262, twoProps, twoCoords)); // POINTER_UP index 1
      } catch (e) {
      }
      expect(params['down']).assertEqual(true);
      controller.clearFocus(viewId);
      controller.dispose(viewId); // focusViewId clear branch
      expect(controller.owns(viewId)).assertEqual(false);
      controller.detachFromView();
      controller.detach();
    });

    it('setOverscrollOffset_significant_change_and_zero_size_create', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      attachContextAndView(controller);
      controller.setOverscrollOffset(338, 0);
      controller.setOverscrollOffset(338, 1.5); // >0.01 delta
      controller.setOverscrollOffset(338, 1.5); // same → skip
      const req: PlatformViewCreationRequest = makeCreateRequest(338, 0, 0);
      controller.createForPlatformViewLayer(req); // bootstrap w/h = 0
      expect(controller.owns(338)).assertEqual(true);
      controller.dispose(338);
      controller.detachFromView();
      controller.detach();
    });

    it('mutators_two_clipRects_intersection_offset', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const fakeView: FakeFlutterViewHost = attachContextAndView(controller);
      const viewId: number = 339;
      const nx: number = 0;
      const ny: number = 0;
      const nw: number = 200;
      const nh: number = 100;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      controller.onDisplayPlatformViewHybrid(viewId, nx, ny, nw, nh, nw, nh);
      const dpr: number = display.getDefaultDisplaySync().densityPixels;
      const data: number[] = makeMutatorsPureTranslate(dpr);
      data[17] = 2; // two clip rects → intersection
      data.push(nx + 10, ny, nw - 10, nh);      // inset left
      data.push(nx, ny + 10, nw, nh - 10);      // inset top
      data.push(0); // rrects
      data.push(0); // clipPath
      controller.onDisplayMutatorsHybrid(viewId, data);
      const params: Record<string, Any> =
        fakeView.getDVModel().children[0].params as Record<string, Any>;
      // offset clip → pathCommands or rect shrink
      expect(params['pathCommands'] !== undefined ||
        (params['rectWidth'] as number) > 0).assertEqual(true);
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    it('mutators_empty_clip_intersection_clears_path', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const fakeView: FakeFlutterViewHost = attachContextAndView(controller);
      const viewId: number = 340;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      controller.onDisplayPlatformViewHybrid(viewId, 0, 0, 100, 100, 100, 100);
      const dpr: number = display.getDefaultDisplaySync().densityPixels;
      const data: number[] = makeMutatorsPureTranslate(dpr);
      data[17] = 2;
      data.push(0, 0, 40, 40);
      data.push(80, 80, 40, 40); // disjoint → no valid intersection with node
      data.push(0);
      data.push(0);
      controller.onDisplayMutatorsHybrid(viewId, data);
      const params: Record<string, Any> =
        fakeView.getDVModel().children[0].params as Record<string, Any>;
      // hasClipRectOffset false → path cleared
      // 交集为空时产品故意设 1x1 零面积 path 隐藏（避免 fallback 全尺寸闪现）
      expect(params['pathWidth']).assertEqual(1);
      expect(params['pathHeight']).assertEqual(1);
      expect(params['pathCommands']).assertEqual('M0 0 L0 0 Z');
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    it('display_after_hide_restores_size', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const fakeView: FakeFlutterViewHost = attachContextAndView(controller);
      const viewId: number = 341;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId, 90, 60));
      controller.onDisplayPlatformViewHybrid(viewId, 1, 2, 90, 60, 90, 60);
      controller.hidePlatformViewHybrid(viewId);
      let params: Record<string, Any> =
        fakeView.getDVModel().children[0].params as Record<string, Any>;
      expect(params['width']).assertEqual(0);
      controller.onDisplayPlatformViewHybrid(viewId, 1, 2, 90, 60, 90, 60);
      params = fakeView.getDVModel().children[0].params as Record<string, Any>;
      expect((params['width'] as number) > 0).assertEqual(true);
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    // Covers rrectToPolyVerts: needs hasClipRectOffset + rrectsCount + radius*dpr>0.5.
    // Existing mutators_rrect_* only sets rrects (no clipRect inset) → never enters this path.
    it('mutators_clipRect_offset_with_rrect_calls_rrectToPolyVerts', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const fakeView: FakeFlutterViewHost = attachContextAndView(controller);
      const viewId: number = 342;
      const nx: number = 100;
      const ny: number = 80;
      const nw: number = 200;
      const nh: number = 120;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      controller.onDisplayPlatformViewHybrid(viewId, nx, ny, nw, nh, nw, nh);
      const dpr: number = display.getDefaultDisplaySync().densityPixels;
      controller.onDisplayMutatorsHybrid(viewId,
        makeMutatorsClipRectOffsetWithRRect(dpr, nx, ny, nw, nh, 12));
      const params: Record<string, Any> =
        fakeView.getDVModel().children[0].params as Record<string, Any>;
      const cmds: string = params['pathCommands'] as string;
      expect(cmds !== undefined && cmds.length > 0).assertEqual(true);
      // rrectHandled clears rect clip fields and uses Path (flattened arc polyline).
      expect(params['rectWidth']).assertEqual(0);
      expect(params['rectHeight']).assertEqual(0);
      expect(params['rectRadius']).assertEqual(0);
      expect(cmds.indexOf('M') >= 0).assertEqual(true);
      expect(cmds.indexOf('L') >= 0).assertEqual(true);
      expect(cmds.indexOf('Z') >= 0).assertEqual(true);
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    // Covers flattenQuad + flattenCubic via clipPath QUAD/CUBIC overflowing viewport.
    it('mutators_clipPath_curves_overflow_flattens_quad_cubic', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const fakeView: FakeFlutterViewHost = attachContextAndView(controller);
      const viewId: number = 343;
      const nx: number = 50;
      const ny: number = 60;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      controller.onDisplayPlatformViewHybrid(viewId, nx, ny, 200, 200, 200, 200);
      const dpr: number = display.getDefaultDisplaySync().densityPixels;
      controller.onDisplayMutatorsHybrid(viewId, makeMutatorsClipPathCurvesOverflow(dpr, nx, ny));
      const params: Record<string, Any> =
        fakeView.getDVModel().children[0].params as Record<string, Any>;
      expect(params['pathCommands'] !== undefined).assertEqual(true);
      expect(params['rectWidth']).assertEqual(0);
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    it('dispatchHoverToEngine_without_napi_is_safe', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      attachContextAndView(controller);
      const viewId: number = 345;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      controller.dispatchHoverToEngine(viewId, true, {
        windowX: 10,
        windowY: 20,
        timestamp: 1000
      } as HoverEvent);
      controller.dispatchHoverToEngine(viewId, false, {
        windowX: 10,
        windowY: 20,
        timestamp: 2000
      } as HoverEvent);
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    it('dispatchHoverToEngine_exit_without_enter_is_dropped', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      attachContextAndView(controller);
      const viewId: number = 346;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      const fakeNapi: FakeFlutterNapi = new FakeFlutterNapi();
      controller.setFlutterNapi(fakeNapi as Any as FlutterNapi);
      controller.dispatchHoverToEngine(viewId, false, {
        windowX: 4,
        windowY: 5,
        timestamp: 3000
      } as HoverEvent);
      expect(fakeNapi.touchPackets).assertEqual(0);
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    it('dispatchHoverToEngine_enter_then_exit_injects_hover_and_remove', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      attachContextAndView(controller);
      const viewId: number = 347;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      const fakeNapi: FakeFlutterNapi = new FakeFlutterNapi();
      controller.setFlutterNapi(fakeNapi as Any as FlutterNapi);
      controller.dispatchHoverToEngine(viewId, true, {
        windowX: 8,
        windowY: 9,
        timestamp: 4000000
      } as HoverEvent);
      expect(fakeNapi.touchPackets).assertEqual(1);
      const enterItem: Record<string, Any> = fakeNapi.lastPackets[0] as Record<string, Any>;
      expect(enterItem['change']).assertEqual(3); // PointerChange.HOVER
      controller.dispatchHoverToEngine(viewId, false, {
        windowX: 8,
        windowY: 9,
        timestamp: 5000000
      } as HoverEvent);
      expect(fakeNapi.touchPackets).assertEqual(2);
      const exitItem: Record<string, Any> = fakeNapi.lastPackets[0] as Record<string, Any>;
      expect(exitItem['change']).assertEqual(2); // PointerChange.REMOVE
      controller.dispose(viewId);
      controller.detachFromView();
      controller.detach();
    });

    it('dispatchHoverToEngine_restart_resets_hover_pairing', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      attachContextAndView(controller);
      const viewId: number = 348;
      controller.createForPlatformViewLayer(makeCreateRequest(viewId));
      const fakeNapi: FakeFlutterNapi = new FakeFlutterNapi();
      controller.setFlutterNapi(fakeNapi as Any as FlutterNapi);
      controller.dispatchHoverToEngine(viewId, true, {
        windowX: 1,
        windowY: 1,
        timestamp: 6000
      } as HoverEvent);
      expect(fakeNapi.touchPackets).assertEqual(1);
      controller.onPreEngineRestart();
      controller.dispatchHoverToEngine(viewId, false, {
        windowX: 1,
        windowY: 1,
        timestamp: 7000
      } as HoverEvent);
      expect(fakeNapi.touchPackets).assertEqual(1);
      controller.detachFromView();
      controller.detach();
    });

    it('detachFromView_clears_own_overlay_slice_keeps_others', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      attachContextAndView(controller);
      const seed: Map<string, Object[]> = new Map();
      const otherSlice: Object[] = [99 as Object];
      seed.set('other-engine', otherSlice);
      AppStorage.setOrCreate('hcpp_overlay_rects_map', seed);
      controller.createForPlatformViewLayer(makeCreateRequest(349));
      controller.onBeginFrameHybrid();
      controller.onDisplayOverlayHybrid(349, 10, 20, 30, 40);
      controller.onEndFrameHybrid();
      const flushed: Map<string, Object[]> =
        AppStorage.get('hcpp_overlay_rects_map') as Map<string, Object[]>;
      expect(flushed.has('fake-flutter-view')).assertEqual(true);
      expect(flushed.has('other-engine')).assertEqual(true);
      controller.dispose(349);
      controller.detachFromView();
      const afterDetach: Map<string, Object[]> =
        AppStorage.get('hcpp_overlay_rects_map') as Map<string, Object[]>;
      expect(afterDetach.has('fake-flutter-view')).assertEqual(false);
      expect(afterDetach.has('other-engine')).assertEqual(true);
      controller.detach();
    });

    it('disposeAllViews_via_any_clears_created_views', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      attachContextAndView(controller);
      controller.createForPlatformViewLayer(makeCreateRequest(350));
      expect(controller.owns(350)).assertEqual(true);
      const anyC: Any = controller as Any;
      anyC.disposeAllViews();
      expect(controller.owns(350)).assertEqual(false);
      controller.detachFromView();
      controller.detach();
    });

    it('removeOverlayRectsFromStorage_via_any_drops_own_key', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      attachContextAndView(controller);
      const seed: Map<string, Object[]> = new Map();
      seed.set('fake-flutter-view', [1 as Object]);
      seed.set('other-engine', [99 as Object]);
      AppStorage.setOrCreate('hcpp_overlay_rects_map', seed);
      const anyC: Any = controller as Any;
      anyC.removeOverlayRectsFromStorage();
      const after: Map<string, Object[]> =
        AppStorage.get('hcpp_overlay_rects_map') as Map<string, Object[]>;
      expect(after.has('fake-flutter-view')).assertEqual(false);
      expect(after.has('other-engine')).assertEqual(true);
      controller.detachFromView();
      controller.detach();
    });

    it('applyCompositionOrder_reorders_two_displayed_views', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const fakeView: FakeFlutterViewHost = attachContextAndView(controller);
      controller.createForPlatformViewLayer(makeCreateRequest(351));
      controller.createForPlatformViewLayer(makeCreateRequest(352));
      controller.onDisplayPlatformViewHybrid(351, 0, 0, 40, 20, 40, 20);
      controller.onDisplayPlatformViewHybrid(352, 50, 0, 40, 20, 40, 20);
      controller.onEndFrameHybrid();
      controller.onDisplayPlatformViewHybrid(352, 50, 0, 40, 20, 40, 20);
      controller.onDisplayPlatformViewHybrid(351, 0, 0, 40, 20, 40, 20);
      controller.onEndFrameHybrid();
      expect(fakeView.getDVModel().children.length >= 2).assertEqual(true);
      controller.dispose(351);
      controller.dispose(352);
      controller.detachFromView();
      controller.detach();
    });

    it('onEndFrameHybrid_without_flutterView_drops_pending_rects', 0, () => {
      const controller: PlatformViewsControllerHybrid =
        new PlatformViewsControllerHybrid(new PlatformViewRegistryImpl());
      const anyC: Any = controller as Any;
      controller.onDisplayOverlayHybrid(1, 0, 0, 10, 10);
      expect((anyC.pendingOverlayRects as Object[]).length).assertEqual(1);
      controller.onEndFrameHybrid();
      expect((anyC.pendingOverlayRects as Object[]).length).assertEqual(0);
    });

    it('createPlatformView_throws_when_context_null', 0, () => {
      const pair = makeControllerWithFactory();
      const controller: PlatformViewsControllerHybrid = pair[0];
      const fakeView: FakeFlutterViewHost = new FakeFlutterViewHost();
      controller.attachToView(fakeView as Any as FlutterView);
      let threw: boolean = false;
      try {
        controller.createForPlatformViewLayer(makeCreateRequest(360));
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });

    it('createPlatformView_throws_when_getView_null', 0, () => {
      const registry: PlatformViewRegistryImpl = new PlatformViewRegistryImpl();
      registry.registerViewFactory(VIEW_TYPE, new NullViewFactory());
      const controller: PlatformViewsControllerHybrid = new PlatformViewsControllerHybrid(registry);
      attachContextAndView(controller);
      let threw: boolean = false;
      try {
        controller.createForPlatformViewLayer(makeCreateRequest(361));
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
      controller.detachFromView();
      controller.detach();
    });
  });
}

class NullEmbeddedView extends PlatformView {
  getView(): WrappedBuilder<[Params]> {
    return null as Any as WrappedBuilder<[Params]>;
  }

  dispose(): void {
  }
}

class NullViewFactory extends PlatformViewFactory {
  constructor() {
    super(StringCodec.INSTANCE);
  }

  create(context: common.Context, viewId: number, args: Any): PlatformView {
    return new NullEmbeddedView();
  }
}
