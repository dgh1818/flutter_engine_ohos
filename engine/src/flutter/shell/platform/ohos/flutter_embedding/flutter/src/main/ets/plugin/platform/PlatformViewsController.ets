/*
* Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd. All rights reserved.
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE_KHZG file.
*
* Based on PlatformViewsController.java originally written by
* Copyright (C) 2013 The Flutter Authors.
*
*/

import PlatformViewsChannel, {
  PlatformViewBufferResized,
  PlatformViewCreationRequest,
  PlatformViewResizeRequest,
  PlatformViewsHandler,
  PlatformViewTouch,
  PlatformViewBufferSize
} from '../../../ets/embedding/engine/systemchannels/PlatformViewsChannel';
import PlatformView, { Params } from './PlatformView';
import { DVModelParameters, } from '../../view/DynamicView/dynamicView';
import { createDVModelFromJson } from '../../view/DynamicView/dynamicViewJson';
import display from '@ohos.display';
import { FlutterView } from '../../view/FlutterView';
import { TextureRegistry } from '../../view/TextureRegistry';
import TextInputPlugin from '../editing/TextInputPlugin';
import { PlatformViewWrapper } from './PlatformViewWrapper';
import { FlutterOverlaySurface } from '../../embedding/engine/FlutterOverlaySurface';
import HashSet from '@ohos.util.HashSet';
import PlatformViewRegistry from './PlatformViewRegistry';
import PlatformViewRegistryImpl from './PlatformViewRegistryImpl';
import DartExecutor from '../../embedding/engine/dart/DartExecutor';
import { FlutterMutatorView } from '../../embedding/engine/mutatorsstack/FlutterMutatorView';
import Log from '../../util/Log'
import PlatformViewFactory from './PlatformViewFactory'
import { ByteBuffer } from '../../util/ByteBuffer';
import Any from '../common/Any';
import { ArrayList, Stack } from '@kit.ArkTS';
import { CustomTouchEvent, CustomTouchObject } from './CustomTouchEvent';
import { NodeRenderType } from '@kit.ArkUI';
import { PlatformViewInfo } from '../../embedding/ohos/PlatformViewInfo';
import { EmbeddingNodeController } from '../../embedding/ohos/EmbeddingNodeController';

/**
 * JSON representation of a DynamicView model.
 */
class DVModelJson {
  /** The component type. */
  compType: string
  /** Array of child components. */
  children: Array<Any>
  /** Component attributes. */
  attributes: Any
  /** Component events. */
  events: Any
  /** Optional build function. */
  build: Any

  /**
   * Constructs a new DVModelJson instance.
   * @param compType - The component type
   * @param children - Array of child components
   * @param attributes - Component attributes
   * @param events - Component events
   * @param build - Optional build function
   */
  constructor(compType: string, children: Array<Any>, attributes: Any, events: Any, build?: Any) {
    this.compType = compType
    this.children = children
    this.attributes = attributes
    this.events = events;
    this.build = build;
  }
}
/**
 * Enumeration of touch event types.
 */
enum TouchEventType {
  /** Action code for when a primary pointer touched the screen. */
  ACTION_DOWN = 0,
  /** Action code for when a primary pointer stopped touching the screen. */
  ACTION_UP = 1,
  /** Action code for when the event only includes information about pointer movement. */
  ACTION_MOVE = 2,
  /** Action code for when a motion event has been canceled. */
  ACTION_CANCEL = 3,
  /** Action code for when a secondary pointer touched the screen. */
  ACTION_POINTER_DOWN = 5,
  /** Action code for when a secondary pointer stopped touching the screen. */
  ACTION_POINTER_UP = 6,
}

const TAG = "PlatformViewsController"

/**
 * Controller for managing platform views in Flutter applications.
 * This class handles the creation, lifecycle, and interaction of native views embedded in Flutter.
 */
export default class PlatformViewsController implements PlatformViewsHandler {
  private registry: PlatformViewRegistryImpl;
  private context: Context | null = null;
  private flutterView: FlutterView | null = null;
  private textureRegistry: TextureRegistry | null = null;
  private textInputPlugin: TextInputPlugin | null = null;
  private platformViewsChannel: PlatformViewsChannel | null = null;
  private nextOverlayLayerId: number = 0;
  private focusViewId: number = -1;
  private platformViews: Map<number, PlatformView>;
  private viewIdWithTextureId: Map<number, number>;
  private viewIdWithNodeController: Map<number, EmbeddingNodeController>;
  private viewWrappers: Map<number, PlatformViewWrapper>;
  private currentFrameUsedOverlayLayerIds: HashSet<number>;
  private currentFrameUsedPlatformViewIds: HashSet<number>;

  /**
   * Constructs a new PlatformViewsController instance.
   */
  constructor() {
    this.registry = new PlatformViewRegistryImpl();
    this.currentFrameUsedOverlayLayerIds = new HashSet();
    this.currentFrameUsedPlatformViewIds = new HashSet();
    this.viewWrappers = new Map();
    this.platformViews = new Map();
    this.viewIdWithTextureId = new Map();
    this.viewIdWithNodeController = new Map();
  }

  /**
   * Creates a platform view for hybrid composition mode.
   * @param request - The platform view creation request
   */
  createForPlatformViewLayer(request: PlatformViewCreationRequest): void {
    Log.i(TAG, "Enter createForPlatformViewLayer");
    this.ensureValidRequest(request);

    let platformView: PlatformView = this.createPlatformView(request);

    this.configureForHybridComposition(platformView, request);
  }

  /**
   * Disposes a platform view and releases all associated resources.
   * @param viewId - The ID of the platform view to dispose
   */
  dispose(viewId: number): void {
    let platformView: PlatformView | null = this.platformViews.get(viewId) || null;
    if (platformView == null) {
      Log.e(TAG, "Disposing unknown platform view with id: " + viewId);
      return;
    }
    if (this.focusViewId == viewId) {
      this.clearFocus(viewId);
      this.focusViewId = -1;
    }
    this.platformViews.delete(viewId);
    let textureId = this.viewIdWithTextureId.get(viewId);

    if (textureId != undefined) {
      this.textureRegistry!.unregisterTexture(textureId);
    }

    this.viewIdWithNodeController.get(viewId)?.disposeFrameNode()
    this.viewIdWithNodeController.delete(viewId);

    let viewWrapper: PlatformViewWrapper | null = this.viewWrappers.get(viewId) || null;
    if (viewWrapper != null && this.flutterView) {
      let index = this.flutterView.getDVModel().children.indexOf(viewWrapper.getDvModel()!);
      if (index > -1) {
        this.flutterView.getDVModel().children.splice(index, 1);
        platformView.onFlutterViewDetached();
      }
    }
    this.viewWrappers.delete(viewId);

    try {
      platformView.dispose();
    } catch (err) {
      Log.e(TAG, "Disposing platform view threw an exception", err);
    }
  }

  /**
   * Sets a parameter value in the DVModelParameters.
   * @param params - The parameters object to modify
   * @param key - The parameter key
   * @param element - The value to set
   */
  setParams: (params: DVModelParameters, key: string, element: Any) => void =
    (params: DVModelParameters, key: string, element: Any): void => {
      let params2 = params as Record<string, Any>;
      params2[key] = element;
    }

  /**
   * Gets a parameter value from the DVModelParameters.
   * @param params - The parameters object to read from
   * @param key - The parameter key
   * @returns The parameter value as a number
   */
  getParams: (params: DVModelParameters, key: string) => number = (params: DVModelParameters, key: string): number => {
    let params2 = params as Record<string, Any>;
    return params2[key];
  }

  /**
   * Resizes a platform view.
   * @param request - The resize request containing new dimensions
   * @param onComplete - Callback to invoke when resize is complete
   */
  resize(request: PlatformViewResizeRequest, onComplete: PlatformViewBufferResized): void {
    let physicalWidth: number = this.toPhysicalPixels(request.newLogicalWidth);
    let physicalHeight: number = this.toPhysicalPixels(request.newLogicalHeight);
    let viewId: number = request.viewId;
    Log.i(TAG,
      `Resize viewId ${viewId}, pw:${physicalWidth}, ph:${physicalHeight},lw:${request.newLogicalWidth}, lh:${request.newLogicalHeight}`);

    let viewWrapper = this.viewWrappers.get(request.viewId)
    let params: DVModelParameters | undefined = viewWrapper?.getDvModel()!.params
     
    this.setParams(params!, "width", physicalWidth);
    this.setParams(params!, "height", physicalHeight);

    let textureId = this.viewIdWithTextureId.get(viewId);
    if (textureId != undefined) {
      let density = this.getDisplayDensity();
      this.textureRegistry?.notifyTextureResizing(textureId, request.newLogicalWidth * density, request.newLogicalHeight * density);
    }

    onComplete.run(new PlatformViewBufferSize(physicalWidth, physicalHeight));
  }

  /**
   * Updates the offset position of a platform view.
   * @param viewId - The ID of the platform view
   * @param top - The top offset
   * @param left - The left offset
   */
  offset(viewId: number, top: number, left: number): void {
    Log.i(TAG, `Offset is id${viewId}, t:${top}, l:${left}`);

    let viewWrapper = this.viewWrappers.get(viewId)
    if (viewWrapper != undefined) {
      let params: DVModelParameters | undefined = viewWrapper?.getDvModel()!.params
      this.setParams(params!, "left", left);
      this.setParams(params!, "top", top);
    }
  }

  /**
   * Sets the hover state for platform views.
   * @param viewId - The ID of the platform view to set hover state for
   */
  hover(viewId: number) {
    for (let key of this.viewWrappers.keys()) {
      let viewWrapper: undefined | PlatformViewWrapper = this.viewWrappers.get(key);
      let dvModel = viewWrapper?.getDvModel();
      let params = dvModel?.getLayoutParams() as Record<string, Any>;
      if (key == viewId) {
        params["hover"] = true;
      } else {
        params["hover"] = false;
      }
    }
  }

  /**
   * Handles touch events for platform views.
   * @param touch - The touch event information
   */
  onTouch(touch: PlatformViewTouch): void {
    let viewWrapper: undefined | PlatformViewWrapper = this.viewWrappers.get(touch.viewId)
    this.focusViewId = touch.viewId;
    if (viewWrapper != undefined) {
      let dvModel = viewWrapper.getDvModel()
      let params = dvModel.getLayoutParams() as Record<string, Any>;
      // When receiving a DOWN action
      if (touch.action === TouchEventType.ACTION_DOWN) {
        // Set the current touch state to true
        params['down'] = true
        // When first receiving a touch DOWN event, dispatch all events stored in the list
        let touchEventArray: Array<CustomTouchEvent> | undefined = params['touchEvent'] as Array<CustomTouchEvent>
        if (touchEventArray !== undefined) {
          let nodeController = params['nodeController'] as EmbeddingNodeController;
          for (let it of touchEventArray) {
            nodeController.postEvent(it)
          }
          // Clear the list after first dispatch
          params['touchEvent'] = undefined
        }

        // When first receiving a mouse PRESS event, dispatch all events stored in the list
        let mouseEventArray: Array<MouseEvent> | undefined = params['mouseEvent'] as Array<MouseEvent>
        if (mouseEventArray !== undefined) {
          let nodeController = params['nodeController'] as EmbeddingNodeController;
          for (let it of mouseEventArray) {
            nodeController.postMouseEvent(it)
          }
          // Clear the list after first dispatch
          params['mouseEvent'] = undefined
        }
        // When receiving an UP action
      } else if (touch.action === TouchEventType.ACTION_UP || touch.action === TouchEventType.ACTION_CANCEL) {
        // Set the touch state to false after finger is lifted. When multiple fingers are lifted suddenly,
        // the final state returned is also ACTION_UP, so we use the UP state to indicate the user
        // is no longer touching the platform view
        params['down'] = false
      }
    }
  }

  /**
   * Sets the text direction for a platform view.
   * @param viewId - The ID of the platform view
   * @param direction - The text direction to set
   */
  setDirection(viewId: number, direction: Direction): void {
    let nodeController = this.viewIdWithNodeController.get(viewId)
    if (nodeController != undefined) {
      nodeController?.setRenderOption(this.flutterView!.getPlatformView()!, this.flutterView!.getSurfaceId(),
        NodeRenderType.RENDER_TYPE_TEXTURE, direction)
      nodeController?.rebuild()
    }
  }

  /**
   * Validates if a direction value is valid.
   * @param direction - The direction value to validate
   * @returns True if the direction is valid, false otherwise
   */
  validateDirection(direction: number): boolean {
    return direction == Direction.Ltr || direction == Direction.Rtl || direction == Direction.Auto;
  }

  /**
   * Clears focus from a platform view.
   * @param viewId - The ID of the platform view
   */
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
    // Make the Xcomponent gain focus.
    focusControl.requestFocus("unfocus-xcomponent-node");
  }

  /**
   * Synchronizes the native view hierarchy.
   * @param yes - Whether to synchronize
   * @throws Error as this method is not implemented
   */
  synchronizeToNativeViewHierarchy(yes: boolean): void {
    throw new Error('Method not implemented.');
  }

  /**
   * Creates a platform view for texture layer composition mode.
   * @param request - The platform view creation request
   * @returns The texture ID for the created view
   */
  public createForTextureLayer(request: PlatformViewCreationRequest): number {
    Log.i(TAG, "Enter createForTextureLayer");
    this.ensureValidRequest(request);

    let platformView: PlatformView = this.createPlatformView(request);
    let textureId = this.configureForTextureLayerComposition(platformView, request);
    this.viewIdWithTextureId.set(request.viewId, textureId);
    return textureId;
  }

  /**
   * Ensures that a platform view creation request is valid.
   * @param request - The request to validate
   * @throws Error if the request is invalid
   * @private
   */
  private ensureValidRequest(request: PlatformViewCreationRequest): void {
    if (!this.validateDirection(request.direction)) {
      throw new Error("Trying to create a view with unknown direction value: "
        + request.direction
        + "(view id: "
        + request.viewId
        + ")")
    }
  }

  /**
   * Creates a platform view instance from a factory.
   * @param request - The platform view creation request
   * @returns The created PlatformView instance
   * @throws Error if the factory is not found or creation fails
   * @private
   */
  private createPlatformView(request: PlatformViewCreationRequest): PlatformView {
    Log.i(TAG, "begin createPlatformView");
    const viewFactory: PlatformViewFactory = this.registry.getFactory(request.viewType);
    if (viewFactory == null) {
      throw new Error("Trying to create a platform view of unregistered type: " + request.viewType)
    }

    let createParams: Any = null;
    if (request.params != null) {
      let byteParas: ByteBuffer = request.params as ByteBuffer;
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

  /**
   * Configures the view for Hybrid Composition mode.
   * @param platformView - The platform view to configure
   * @param request - The creation request
   * @private
   */
  private configureForHybridComposition(platformView: PlatformView, request: PlatformViewCreationRequest): void {
    Log.i(TAG, "Using hybrid composition for platform view: " + request.viewId);
  }

  /**
   * Configures the view for Texture Layer Composition mode.
   * @param platformView - The platform view to configure
   * @param request - The creation request
   * @returns The texture ID for the view
   * @private
   */
  private configureForTextureLayerComposition(platformView: PlatformView,
    request: PlatformViewCreationRequest): number {
    Log.i(TAG, "Hosting view in view hierarchy for platform view: " + request.viewId);
    let surfaceId: string = '0';
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
    let physicalWidth: number = this.toPhysicalPixels(request.logicalWidth);
    let physicalHeight: number = this.toPhysicalPixels(request.logicalHeight);

    let nodeController = new EmbeddingNodeController();
    nodeController.setRenderOption(platformView, surfaceId, NodeRenderType.RENDER_TYPE_TEXTURE, request.direction);
    this.viewIdWithNodeController.set(request.viewId, nodeController);

    let dvModel = createDVModelFromJson(new DVModelJson("NodeContainer",
      [],
      {
        "width": physicalWidth,
        "height": physicalHeight,
        "nodeController": nodeController,
        "left": request.logicalLeft,
        "top": request.logicalTop
      },
      {},
      undefined));
    let viewWrapper: PlatformViewWrapper = new PlatformViewWrapper();
    viewWrapper.addDvModel(dvModel);
    this.viewWrappers.set(request.viewId, viewWrapper);
    this.flutterView?.getDVModel().children.push(viewWrapper.getDvModel())
    platformView.onFlutterViewAttached(this.flutterView!.getDVModel());
    Log.i(TAG, "Create platform view success");
    return textureId;
  }

  /**
   * Attaches the controller to a context, texture registry, and Dart executor.
   * @param context - The application context
   * @param textureRegistry - The texture registry for managing textures
   * @param dartExecutor - The Dart executor for communication
   */
  public attach(context: Context, textureRegistry: TextureRegistry | null, dartExecutor: DartExecutor): void {
    this.context = context;
    this.textureRegistry = textureRegistry;
    this.platformViewsChannel = new PlatformViewsChannel(dartExecutor);
    this.platformViewsChannel.setPlatformViewsHandler(this);
  }

  /**
   * Detaches the controller and cleans up resources.
   */
  public detach(): void {
    if (this.platformViewsChannel != null) {
      this.platformViewsChannel.setPlatformViewsHandler(null);
    }
    this.destroyOverlaySurfaces();
    this.platformViewsChannel = null;
    this.context = null;
    this.textureRegistry = null;
  }

  /**
   * Attaches the controller to a FlutterView.
   * @param newFlutterView - The FlutterView to attach to
   */
  public attachToView(newFlutterView: FlutterView) {
    this.flutterView = newFlutterView;
  }

  /**
   * Detaches the controller from the FlutterView.
   */
  public detachFromView(): void {
    this.destroyOverlaySurfaces();
    this.removeOverlaySurfaces();
    this.flutterView = null;
  }

  /**
   * Gets the current FlutterView.
   * @returns The FlutterView instance, or null if not attached
   */
  public getFlutterView(): FlutterView | null {
    return this.flutterView;
  }

  /**
   * Attaches a text input plugin to this controller.
   * @param textInputPlugin - The TextInputPlugin instance
   */
  public attachTextInputPlugin(textInputPlugin: TextInputPlugin): void {
    this.textInputPlugin = textInputPlugin;
  }

  /**
   * Detaches the text input plugin from this controller.
   */
  public detachTextInputPlugin(): void {
    this.textInputPlugin = null;
  }

  /**
   * Gets the platform view registry.
   * @returns The PlatformViewRegistry instance
   */
  public getRegistry(): PlatformViewRegistry {
    return this.registry;
  }

  /**
   * Called when the controller is detached from NAPI.
   * Disposes all platform views.
   */
  public onDetachedFromNapi(): void {
    this.diposeAllViews();
  }

  /**
   * Called before the Flutter engine restarts.
   * Disposes all platform views.
   */
  public onPreEngineRestart(): void {
    this.diposeAllViews();
  }

  /**
   * Gets the display density.
   * @returns The display density value
   * @private
   */
  private getDisplayDensity(): number {
    return display.getDefaultDisplaySync().densityPixels;
  }

  /**
   * Converts logical pixels to physical pixels.
   * @param logicalPixels - The logical pixel value
   * @returns The physical pixel value
   * @private
   */
  private toPhysicalPixels(logicalPixels: number): number {
    return Math.round(px2vp(logicalPixels * this.getDisplayDensity()));
  }

  /**
   * Converts physical pixels to logical pixels using a specific density.
   * @param physicalPixels - The physical pixel value
   * @param displayDensity - The display density to use
   * @returns The logical pixel value
   * @private
   */
  private toLogicalPixelsByDensity(physicalPixels: number, displayDensity: number): number {
    return Math.round(physicalPixels / displayDensity);
  }

  /**
   * Converts physical pixels to logical pixels using the current display density.
   * @param physicalPixels - The physical pixel value
   * @returns The logical pixel value
   * @private
   */
  private toLogicalPixels(physicalPixels: number): number {
    return this.toLogicalPixelsByDensity(physicalPixels, this.getDisplayDensity());
  }

  /**
   * Disposes all platform views.
   * @private
   */
  private diposeAllViews(): void {
    let viewKeys = this.platformViews.keys();
    for (let viewId of viewKeys) {
      this.dispose(viewId);
    }
  }

  /**
   * Initializes the root image view if needed.
   * @private
   */
  private initializeRootImageViewIfNeeded(): void {
  }

  /**
   * Called when an overlay surface should be displayed.
   * @param id - The overlay surface ID
   * @param x - The X position
   * @param y - The Y position
   * @param width - The width
   * @param height - The height
   */
  public onDisplayOverlaySurface(id: number, x: number, y: number, width: number, height: number): void {
  }

  /**
   * Called at the beginning of each frame.
   * Clears the sets of used overlay layers and platform views.
   */
  public onBeginFrame(): void {
    this.currentFrameUsedOverlayLayerIds.clear();
    this.currentFrameUsedPlatformViewIds.clear();
  }

  /**
   * Called at the end of each frame.
   */
  public onEndFrame(): void {
  }

  /**
   * Finishes frame rendering.
   * @param isFrameRenderedUsingImageReaders - Whether the frame was rendered using image readers
   * @private
   */
  private finishFrame(isFrameRenderedUsingImageReaders: boolean): void {
  }

  /**
   * Creates a new overlay surface.
   * @returns A new FlutterOverlaySurface instance
   */
  public createOverlaySurface(): FlutterOverlaySurface {
    return new FlutterOverlaySurface(this.nextOverlayLayerId++);
  }

  /**
   * Destroys all overlay surfaces.
   * @private
   */
  private destroyOverlaySurfaces(): void {
  }

  /**
   * Removes overlay surfaces from the view hierarchy.
   * @private
   */
  private removeOverlaySurfaces(): void {
    if (!(this.flutterView instanceof FlutterView)) {
      return;
    }
  }
  
  /**
   * Renders a platform view with the specified dimensions and position.
   * @param surfaceId - The surface ID
   * @param platformView - The platform view to render
   * @param width - The width in pixels
   * @param height - The height in pixels
   * @param left - The left position in pixels
   * @param top - The top position in pixels
   */
  public render(surfaceId: number, platformView: PlatformView,
    width: number, height: number, left: number, top: number) {

    let wrapper = this.viewWrappers.get(surfaceId);
    if (wrapper != null) {
      let params: DVModelParameters | undefined = wrapper?.getDvModel()!.params

      this.setParams(params!, "width", width);
      this.setParams(params!, "height", height);
      this.setParams(params!, "left", left);
      this.setParams(params!, "top", top);
      return;
    }

    this.flutterView!.setSurfaceId(surfaceId.toString());
    let wrappedBuilder: WrappedBuilder<[Params]> = platformView.getView();
    this.flutterView?.setWrappedBuilder(wrappedBuilder);
    this.flutterView?.setPlatformView(platformView);

    let nodeController = new EmbeddingNodeController();

    nodeController.setRenderOption(platformView, surfaceId.toString(), NodeRenderType.RENDER_TYPE_TEXTURE,
      Direction.Auto);
    this.viewIdWithNodeController.set(surfaceId, nodeController);

    let dvModel = createDVModelFromJson(new DVModelJson("NodeContainer",
      [],
      {
        "width": width,
        "height": height,
        "nodeController": nodeController,
        "left": left,
        "top": top
      },
      {},
      undefined));

    let viewWrapper: PlatformViewWrapper = new PlatformViewWrapper();
    viewWrapper.addDvModel(dvModel);
    this.viewWrappers.set(surfaceId, viewWrapper);
    this.flutterView?.getDVModel().children.push(viewWrapper.getDvModel());
    platformView.onFlutterViewAttached(this.flutterView!.getDVModel());
    this.platformViews.set(surfaceId, platformView!);
  }
}