/*
* Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd.
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

import { PlatformViewsAccessibilityDelegate } from './PlatformViewsAccessibilityDelegate';
import PlatformViewsChannel, {
  PlatformViewBufferResized,
  PlatformViewCreationRequest,
  PlatformViewResizeRequest,
  PlatformViewsHandler, PlatformViewTouch, PlatformViewBufferSize
} from '../../../ets/embedding/engine/systemchannels/PlatformViewsChannel';
import PlatformView, { Params } from './PlatformView';
import { DVModel, DVModelContainer, DVModelParameters, DynamicView } from '../../view/DynamicView/dynamicView';
import display from '@ohos.display';
import { FlutterView } from '../../view/FlutterView';
import { TextureRegistry } from '../../view/TextureRegistry';
import TextInputPlugin from '../editing/TextInputPlugin';
import { PlatformOverlayView } from './PlatformOverlayView';
import { PlatformViewWrapper } from './PlatformViewWrapper';
import { FlutterOverlaySurface } from '../../embedding/engine/FlutterOverlaySurface';
import HashSet from '@ohos.util.HashSet';
import PlatformViewRegistry from './PlatformViewRegistry';
import PlatformViewRegistryImpl from './PlatformViewRegistryImpl';
import DartExecutor from '../../embedding/engine/dart/DartExecutor';
import { AccessibilityEventsDelegate } from './AccessibilityEventsDelegate';
import AccessibilityBridge from '../../view/AccessibilityBridge';
import { FlutterMutatorView } from '../../embedding/engine/mutatorsstack/FlutterMutatorView';
import common from '@ohos.app.ability.common';
import Log from '../../util/Log'
import OhosTouchProcessor from '../../embedding/ohos/OhosTouchProcessor'
import PlatformViewFactory from './PlatformViewFactory'
import { ByteBuffer } from '../../util/ByteBuffer';
import Any from '../common/Any';
import { RawPointerCoords } from './RawPointerCoord';
import { ArrayList } from '@kit.ArkTS';
import { CustomTouchEvent, CustomTouchObject } from './CustomTouchEvent';
import { NodeRenderType } from '@kit.ArkUI';

const TAG = "PlatformViewsController"
let timeStamp: number = 52086377781000
export default class PlatformViewsController implements PlatformViewsAccessibilityDelegate, PlatformViewsHandler {
  private registry: PlatformViewRegistryImpl;
  private context: Context | null = null;
  private flutterView: FlutterView | null = null;
  private textureRegistry: TextureRegistry | null = null;
  private textInputPlugin: TextInputPlugin | null = null;
  private platformViewsChannel: PlatformViewsChannel | null = null;
  private accessibilityEventsDelegate: AccessibilityEventsDelegate;
  private nextOverlayLayerId: number = 0;
  private usesSoftwareRendering: boolean = false;

  private platformViews: Map<number, PlatformView>;
  private viewIdWithTextureId: Map<number, number>;
  private overlayLayerViews: Map<number, PlatformOverlayView>;
  private viewWrappers: Map<number, PlatformViewWrapper>;
  private currentFrameUsedOverlayLayerIds: HashSet<number>;
  private currentFrameUsedPlatformViewIds: HashSet<number>;
  private platformViewParent: Map<number, FlutterMutatorView>;

  constructor() {
    this.registry = new PlatformViewRegistryImpl();
    this.accessibilityEventsDelegate = new AccessibilityEventsDelegate();
    this.overlayLayerViews = new Map<number, PlatformOverlayView>();
    this.currentFrameUsedOverlayLayerIds = new HashSet();
    this.currentFrameUsedPlatformViewIds = new HashSet();
    this.viewWrappers = new Map();
    this.platformViews = new Map();
    this.viewIdWithTextureId = new Map();
    this.platformViewParent = new Map();
  }


  getPlatformViewById(viewId: number): Object {
    throw new Error('Method not implemented.');
  }

  usesVirtualDisplay(id: number): boolean {
    throw new Error('Method not implemented.');
  }

  attachAccessibilityBridge(accessibilityBridge: AccessibilityBridge): void {
    throw new Error('Method not implemented.');
  }

  detachAccessibilityBridge(): void {
    throw new Error('Method not implemented.');
  }

  createForPlatformViewLayer(request: PlatformViewCreationRequest): void {
    Log.i(TAG, "Enter createForPlatformViewLayer");
    this.ensureValidRequest(request);

    let platformView: PlatformView = this.createPlatformView(request);

    this.configureForHybridComposition(platformView, request);
  }

  dispose(viewId: number): void {
    let platformView: PlatformView | null = this.platformViews.get(viewId) || null;
    if (platformView == null) {
      Log.e(TAG, "Disposing unknown platform view with id: " + viewId);
      return;
    }
    this.platformViews.delete(viewId);
    let textureId = this.viewIdWithTextureId.get(viewId);

    if (textureId != undefined) {
      this.textureRegistry!.unregisterTexture(textureId);
    }

    try {
      platformView.dispose();
    } catch (err) {
      Log.e(TAG, "Disposing platform view threw an exception", err);
    }


  }

  setParams: (params: DVModelParameters, key: string, element: Any ) => void = (params: DVModelParameters, key: string, element: Any): void => {
    let params2 = params as Record<string, Any>;
    params2[key] =element;
  }

  resize(request: PlatformViewResizeRequest, onComplete: PlatformViewBufferResized): void {
    let physicalWidth: number = this.toPhysicalPixels(request.newLogicalWidth);
    let physicalHeight: number = this.toPhysicalPixels(request.newLogicalHeight);
    let viewId: number = request.viewId;
    Log.i(TAG, `Resize viewId ${viewId}, pw:${physicalWidth}, ph:${physicalHeight},lw:${request.newLogicalWidth}, lh:${request.newLogicalHeight}`);
 
    let nodeWidthLink: SubscribedAbstractProperty<number> = AppStorage.link('nodeWidth');
    let nodeHeightLink: SubscribedAbstractProperty<number> = AppStorage.link('nodeHeight');
    let oldNodeWidth: number = AppStorage.get('nodeWidth')!
    let oldNodeHeight: number = AppStorage.get('nodeHeight')!
    if (oldNodeWidth == physicalWidth) {
      onComplete.run(new PlatformViewBufferSize(physicalWidth, physicalHeight));
      return;
    }
    nodeWidthLink.set(physicalWidth);
    nodeHeightLink.set(physicalHeight);
    this.flutterView!.getEmbeddingNodeController().rebuild();

    onComplete.run(new PlatformViewBufferSize(physicalWidth, physicalHeight));
  }

  offset(viewId: number, top: number, left: number): void {
    Log.i(TAG, `Offset is id${viewId}, t:${top}, l:${left}`);
  }

  onTouch(touch: PlatformViewTouch): void {
    console.log("nodeController onTouch:")
    let pointX = 0;
    let pointY = 0;
    let rawPointerCoords1: Array<ESObject> = touch.rawPointerCoords as Array<ESObject> ;
    rawPointerCoords1.forEach((item: ESObject) => {
      let rawPoints: ArrayList<ESObject> = item as ArrayList<ESObject>;
      let length = rawPoints.length
      pointX = rawPoints[length - 2];
      pointY = rawPoints[length - 1];
    })


    timeStamp = timeStamp + 10000;
    let touches1: CustomTouchObject = new CustomTouchObject(touch.action, 0, pointX, pointY, pointX, pointY, pointX, pointY, pointX, pointY);
    let changedTouches1: CustomTouchObject = new CustomTouchObject(touch.action, 0, pointX, pointY, pointX, pointY, pointX, pointY, pointX, pointY);
    let customTouchEvent1: CustomTouchEvent = new CustomTouchEvent(touch.action, [touches1], [changedTouches1], timeStamp, 2, 1, 0, 0, 1)

    if (this.flutterView == null) {
      Log.e(TAG, "flutterView is null")
    }
    this.flutterView?.getEmbeddingNodeController().postEvent(customTouchEvent1)
  }

  setDirection(viewId: number, direction: Direction): void {
    this.flutterView!.getEmbeddingNodeController().setRenderOption(this.flutterView!.getPlatformView()!, this.flutterView!.getSurfaceId(), NodeRenderType.RENDER_TYPE_TEXTURE, direction);
    this.flutterView!.getEmbeddingNodeController().rebuild();
  }

  validateDirection(direction:number):boolean {
    return direction == Direction.Ltr || direction == Direction.Rtl || direction == Direction.Auto;
  }

  clearFocus(viewId: number): void {
    const platformView = this.platformViews.get(viewId);
    if (platformView == null) {
      Log.e(TAG, "Setting direction to an unknown view with id: " + viewId);
      return;
    }
    const embeddedView = platformView.getView();
    if (embeddedView == null) {
      Log.e(TAG, "Setting direction to a null view with id: " + viewId);
      return;
    }
    focusControl.requestFocus('emptyFocusText' + this.flutterView?.getId());
  }
  synchronizeToNativeViewHierarchy(yes: boolean): void {
    throw new Error('Method not implemented.');
  }

  public createForTextureLayer(request: PlatformViewCreationRequest): number {
    Log.i(TAG, "Enter createForTextureLayer");
    this.ensureValidRequest(request);

    let platformView: PlatformView = this.createPlatformView(request);
    let textureId = this.configureForTextureLayerComposition(platformView, request);
    this.viewIdWithTextureId.set(request.viewId, textureId);
    return textureId;
  }

  private ensureValidRequest(request: PlatformViewCreationRequest): void {
    if (!this.validateDirection(request.direction)) {
      throw new Error("Trying to create a view with unknown direction value: "
        + request.direction
        + "(view id: "
        + request.viewId
        + ")")
    }
  }

  private createPlatformView(request: PlatformViewCreationRequest): PlatformView {
    Log.i(TAG, "begin createPlatformView");
    const viewFactory: PlatformViewFactory = this.registry.getFactory(request.viewType);
    if (viewFactory == null) {
      throw new Error("Trying to create a platform view of unregistered type: " + request.viewType)
    }

    let createParams: Any = null;
    if (request.params != null) {
      let byteParas : ByteBuffer = request.params as ByteBuffer;
      createParams = viewFactory.getCreateArgsCodec().decodeMessage(byteParas.buffer);
    }

    if (this.context == null) {
      throw new Error('PlatformView#context is null.');
    }
    let platformView = viewFactory.create(this.context, request.viewId, createParams);

    let embeddedView: WrappedBuilder<[Params]> = platformView.getView();
    if (embeddedView == null) {
      throw new Error("PlatformView#getView() returned null, but an WrappedBuilder reference was expected.");
    }


    this.platformViews.set(request.viewId, platformView);
    return platformView;
  }

  // Configures the view for Hybrid Composition mode.
  private configureForHybridComposition(platformView: PlatformView, request: PlatformViewCreationRequest): void {
    Log.i(TAG, "Using hybrid composition for platform view: " + request.viewId);
  }

  private configureForTextureLayerComposition(platformView: PlatformView, request: PlatformViewCreationRequest): number {
    Log.i(TAG, "Hosting view in view hierarchy for platform view: " + request.viewId);
    let surfaceId: string  = '0';
    let textureId: number = 0;
    if (this.textureRegistry != null) {
      textureId = this.textureRegistry!.getTextureId();
      surfaceId = this.textureRegistry!.registerTexture(textureId).getSurfaceId().toString();
      Log.i(TAG, "nodeController getSurfaceId: " + surfaceId);
      this.flutterView!.setSurfaceId(surfaceId);
    }

    let wrappedBuilder: WrappedBuilder<[Params]> = platformView.getView();
    this.flutterView?.setWrappedBuilder(wrappedBuilder);
    this.flutterView?.setPlatformView(platformView);
    let nodeWidthLink: SubscribedAbstractProperty<number> = AppStorage.link('nodeWidth');
    let nodeHeightLink: SubscribedAbstractProperty<number> = AppStorage.link('nodeHeight');
    let physicalWidth: number = this.toPhysicalPixels(request.logicalWidth);
    let physicalHeight: number = this.toPhysicalPixels(request.logicalHeight);
    nodeWidthLink.set(physicalWidth);
    nodeHeightLink.set(physicalHeight);
    this.flutterView!.getEmbeddingNodeController().setRenderOption(platformView, surfaceId, NodeRenderType.RENDER_TYPE_TEXTURE, request.direction);
    this.flutterView!.getEmbeddingNodeController().rebuild();
    Log.i(TAG, "Create platform view success");

    return textureId;
  }

  public attach(context: Context, textureRegistry: TextureRegistry | null, dartExecutor: DartExecutor): void {
    if (this.context != null) {

    }
    this.context = context;
    this.textureRegistry = textureRegistry;
    this.platformViewsChannel = new PlatformViewsChannel(dartExecutor);
    this.platformViewsChannel.setPlatformViewsHandler(this);
  }

  public detach(): void {
    if (this.platformViewsChannel != null) {
      this.platformViewsChannel.setPlatformViewsHandler(null);
    }
    this.destroyOverlaySurfaces();
    this.platformViewsChannel = null;
    this.context = null;
    this.textureRegistry = null;
  }

  public attachToView(newFlutterView : FlutterView) {
    this.flutterView = newFlutterView;
    for (let wrapper of this.viewWrappers.values()) {
      this.flutterView?.getDVModel().children.push(wrapper.getDvModel()!);
    }
    for (let mutator of this.platformViewParent.values()) {
      this.flutterView?.getDVModel().children.push(mutator.getDvModel()!);
    }
    for (let platformView of this.platformViews.values()) {
      platformView.onFlutterViewAttached(this.flutterView?.getDVModel());
    }
  }

  public detachFromView(): void {
    for (let index = 0; index < this.viewWrappers.size; index++) {
      this.flutterView?.getDVModel().children.pop();
    }
    for (let index = 0; index < this.platformViewParent.size; index++) {
      this.flutterView?.getDVModel().children.pop();
    }
    this.destroyOverlaySurfaces();
    this.removeOverlaySurfaces();
    this.flutterView = null;

    for (let platformView of this.platformViews.values()) {
      platformView.onFlutterViewDetached();
    }
  }

  public getFlutterView(): FlutterView | null {
    return this.flutterView;
  }

  public attachTextInputPlugin(textInputPlugin: TextInputPlugin): void {
    this.textInputPlugin = textInputPlugin;
  }

  public detachTextInputPlugin(): void {
    this.textInputPlugin = null;
  }

  public getRegistry(): PlatformViewRegistry {
    return this.registry;
  }

  public onDetachedFromNapi(): void {
    this.diposeAllViews();
  }

  public onPreEngineRestart(): void {
    this.diposeAllViews();
  }

  private getDisplayDensity(): number {
    return display.getDefaultDisplaySync().densityPixels;
  }
  private toPhysicalPixels(logicalPixels: number): number {
    return Math.round(px2vp(logicalPixels * this.getDisplayDensity()));
  }

  private toLogicalPixelsByDensity(physicalPixels: number, displayDensity: number): number {
    return Math.round(physicalPixels / displayDensity);
  }

  private toLogicalPixels(physicalPixels: number): number {
    return this.toLogicalPixelsByDensity(physicalPixels, this.getDisplayDensity());
  }


  private diposeAllViews(): void {
  }

  private initializeRootImageViewIfNeeded(): void {
  }

  public onDisplayOverlaySurface(id: number, x: number, y: number, width: number, height: number): void {
  }

  public onBeginFrame(): void {
    this.currentFrameUsedOverlayLayerIds.clear();
    this.currentFrameUsedPlatformViewIds.clear();
  }

  public onEndFrame(): void {
  }

  private finishFrame(isFrameRenderedUsingImageReaders: boolean): void {
  }

  public createOverlaySurfaceByPlatformOverlayView(imageView: PlatformOverlayView) {
    let id = this.nextOverlayLayerId++;
    this.overlayLayerViews.set(id, imageView);
    return new FlutterOverlaySurface(this.nextOverlayLayerId++);
  }

  public createOverlaySurface(): FlutterOverlaySurface {
    return new FlutterOverlaySurface(this.nextOverlayLayerId++);
  }

  private destroyOverlaySurfaces(): void {
  }

  private removeOverlaySurfaces(): void {
    if (!(this.flutterView instanceof FlutterView)) {
      return;
    }
  }
}