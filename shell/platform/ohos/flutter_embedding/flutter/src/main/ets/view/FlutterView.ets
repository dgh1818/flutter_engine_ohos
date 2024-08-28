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
import FlutterEngine from '../embedding/engine/FlutterEngine';
import Log from '../util/Log';
import { DVModel, DVModelChildren, DVModelEvents, DVModelParameters } from './DynamicView/dynamicView';
import { display } from '@kit.ArkUI'
import FlutterManager from '../embedding/ohos/FlutterManager';
import window from '@ohos.window';
import KeyboardManager from '../embedding/ohos/KeyboardManager';
import MouseCursorPlugin from '../plugin/mouse/MouseCursorPlugin';
import Settings from '../embedding/ohos/Settings';
import ArrayList from '@ohos.util.ArrayList';
import PlatformView, { Params } from '../plugin/platform/PlatformView';
import hiTraceMeter from '@ohos.hiTraceMeter'
import { JSON } from '@kit.ArkTS';
import TextInputPlugin from '../plugin/editing/TextInputPlugin';
import { BusinessError } from '@kit.BasicServicesKit';
import { accessibility } from '@kit.AccessibilityKit';

const TAG = "FlutterViewTag";

export class ViewportMetrics {
  devicePixelRatio: number = 1.0;
  physicalWidth: number = 0;
  physicalHeight: number = 0;
  physicalViewPaddingTop: number = 0;
  physicalViewPaddingRight: number = 0;
  physicalViewPaddingBottom: number = 0;
  physicalViewPaddingLeft: number = 0;
  physicalViewInsetTop: number = 0;
  physicalViewInsetRight: number = 0;
  physicalViewInsetBottom: number = 0;
  physicalViewInsetLeft: number = 0;
  systemGestureInsetTop: number = 0;
  systemGestureInsetRight: number = 0;
  systemGestureInsetBottom: number = 0;
  systemGestureInsetLeft: number = 0;
  physicalTouchSlop: number = -1;
  displayFeatures: Array<DisplayFeature> = [];

  clone(): ViewportMetrics {
    const copy = new ViewportMetrics();
    copy.devicePixelRatio = this.devicePixelRatio;
    copy.physicalWidth = this.physicalWidth;
    copy.physicalHeight = this.physicalHeight;
    copy.physicalViewPaddingTop = this.physicalViewPaddingTop;
    copy.physicalViewPaddingRight = this.physicalViewPaddingRight;
    copy.physicalViewPaddingBottom = this.physicalViewPaddingBottom;
    copy.physicalViewPaddingLeft = this.physicalViewPaddingLeft;
    copy.physicalViewInsetTop = this.physicalViewInsetTop;
    copy.physicalViewInsetRight = this.physicalViewInsetRight;
    copy.physicalViewInsetBottom = this.physicalViewInsetBottom;
    copy.physicalViewInsetLeft = this.physicalViewInsetLeft;
    copy.systemGestureInsetTop = this.systemGestureInsetTop;
    copy.systemGestureInsetRight = this.systemGestureInsetRight;
    copy.systemGestureInsetBottom = this.systemGestureInsetBottom;
    copy.systemGestureInsetLeft = this.systemGestureInsetLeft;
    copy.physicalTouchSlop = this.physicalTouchSlop;
    copy.displayFeatures = this.displayFeatures;
    return copy;
  }

  isEqual(other: ViewportMetrics): boolean {
    return this.devicePixelRatio === other.devicePixelRatio &&
      this.physicalWidth === other.physicalWidth &&
      this.physicalHeight === other.physicalHeight &&
      this.physicalViewPaddingTop === other.physicalViewPaddingTop &&
      this.physicalViewPaddingRight === other.physicalViewPaddingRight &&
      this.physicalViewPaddingBottom === other.physicalViewPaddingBottom &&
      this.physicalViewPaddingLeft === other.physicalViewPaddingLeft &&
      this.physicalViewInsetTop === other.physicalViewInsetTop &&
      this.physicalViewInsetRight === other.physicalViewInsetRight &&
      this.physicalViewInsetBottom === other.physicalViewInsetBottom &&
      this.physicalViewInsetLeft === other.physicalViewInsetLeft &&
      this.systemGestureInsetTop === other.systemGestureInsetTop &&
      this.systemGestureInsetRight === other.systemGestureInsetRight &&
      this.systemGestureInsetBottom === other.systemGestureInsetBottom &&
      this.systemGestureInsetLeft === other.systemGestureInsetLeft &&
      this.physicalTouchSlop === other.physicalTouchSlop &&
      this.displayFeatures === other.displayFeatures;
  }
}

export class FlutterView {

  private flutterEngine: FlutterEngine | null = null
  private id: string = ""
  private dVModel: DVModel =
    new DVModel("Stack", new DVModelParameters(), new DVModelEvents(), new DVModelChildren(), null);
  private wrapBuilder: WrappedBuilder<[Params]> | undefined = undefined;
  private platformView: PlatformView | undefined = undefined;
  private isSurfaceAvailableForRendering: boolean = false
  private viewportMetrics = new ViewportMetrics();
  private displayInfo?: display.Display;
  private keyboardManager: KeyboardManager | null = null;
  private mainWindow: window.Window | null = null;
  private mouseCursorPlugin?: MouseCursorPlugin;
  private textInputPlugin?: TextInputPlugin;
  private uiContext?: UIContext | undefined;
  private settings?: Settings;
  private mFirstFrameListeners: ArrayList<FirstFrameListener>;
  private mFirstPreloadFrameListeners: ArrayList<FirstPreloadFrameListener>;
  private isFlutterUiDisplayed: boolean = false;
  private isFlutterUiPreload: boolean = false;
  private surfaceId: string = "0";
  private checkFullScreen: boolean = true;
  private checkKeyboard: boolean = true;
  private checkGesture: boolean = true;
  private checkAiBar: boolean = true;
  private systemAvoidArea: window.AvoidArea;
  private navigationAvoidArea: window.AvoidArea;
  private gestureAvoidArea: window.AvoidArea;
  private keyboardAvoidArea: window.AvoidArea;
  private needSetViewport: boolean = false;
  private windowPosition: window.Rect | null = null;
  private displayFeaturesCache: Map<display.FoldDisplayMode, Array<DisplayFeature>> =
    new Map<display.FoldDisplayMode, Array<DisplayFeature>>();

  constructor(viewId: string, context: Context) {
    this.id = viewId
    this.displayInfo = display.getDefaultDisplaySync();
    this.viewportMetrics.devicePixelRatio = this.displayInfo?.densityPixels;
    this.viewportMetrics.physicalTouchSlop = 1.0 * this.displayInfo?.densityPixels;

    this.mainWindow = FlutterManager.getInstance()
      .getWindowStage(FlutterManager.getInstance().getUIAbility(context))
      .getMainWindowSync();
    this.mFirstFrameListeners = new ArrayList<FirstFrameListener>();
    this.mFirstPreloadFrameListeners = new ArrayList<FirstPreloadFrameListener>();

    this.mainWindow?.on('windowSizeChange', this.windowSizeChangeCallback);
    this.mainWindow?.on('avoidAreaChange', this.avoidAreaChangeCallback);
    this.mainWindow?.on('windowStatusChange', this.windowStatusChangeCallback);
    this.mainWindow?.on('keyboardHeightChange', this.keyboardHeightChangeCallback);
    this.mainWindow?.on('windowRectChange', this.windowRectChangeCallback);
    //监听系统无障碍服务状态改变
    accessibility.on('accessibilityStateChange', this.accessibilityStateChangeCallback);

    this.systemAvoidArea = this.mainWindow?.getWindowAvoidArea(window.AvoidAreaType.TYPE_SYSTEM);
    this.navigationAvoidArea = this.mainWindow?.getWindowAvoidArea(window.AvoidAreaType.TYPE_NAVIGATION_INDICATOR);
    this.gestureAvoidArea = this.mainWindow?.getWindowAvoidArea(window.AvoidAreaType.TYPE_SYSTEM_GESTURE);
    this.keyboardAvoidArea = this.mainWindow?.getWindowAvoidArea(window.AvoidAreaType.TYPE_KEYBOARD);

    // Subscribes to display changes. Example: event that the display size is changed.
    try {
      display.on("change", this.displayChangeCallback);
    } catch (e) {
      Log.e(TAG, "displayInfo error" + JSON.stringify(e));
    }

  }

  private avoidAreaChangeCallback = (data: window.AvoidAreaOptions) => {
    Log.i(TAG, "avoidAreaChangeCallback type:" + data.type);
    if (data.type == window.AvoidAreaType.TYPE_SYSTEM) { //0
      this.systemAvoidArea = data.area;
    } else if (data.type == window.AvoidAreaType.TYPE_SYSTEM_GESTURE) { //2
      this.gestureAvoidArea = data.area;
    } else if (data.type == window.AvoidAreaType.TYPE_KEYBOARD) { //3
      this.keyboardAvoidArea = data.area;
    } else if (data.type == window.AvoidAreaType.TYPE_NAVIGATION_INDICATOR) { //4
      this.navigationAvoidArea = data.area;
    }
    if (this.isAttachedToFlutterEngine()) {
      this.onAreaChange(null);
    }
  }

  private  windowSizeChangeCallback = (data: window.Size) => {
    Log.i(TAG, "windowSizeChangeCallback w:" + data.width + ", h:" + data.height);
    if (this.isAttachedToFlutterEngine()) {
      this.onAreaChange(null);
    }
  }

  private windowStatusChangeCallback = (data: window.WindowStatusType) => {
    Log.i(TAG, "windowStatusChangeCallback " + data);
    if (this.isAttachedToFlutterEngine()) {
      FlutterManager.getInstance().getFullScreenListener().onScreenStateChanged(data);
    }
  };

  private displayChangeCallback = async (data: number) => {
    this.displayInfo = display.getDefaultDisplaySync();
    this.flutterEngine?.getTextInputChannel()?.setDevicePixelRatio(this.displayInfo.densityPixels);
    let devicePixelRatio: number = this.displayInfo?.densityPixels;
    let physicalTouchSlop: number = 1.0 * this.displayInfo?.densityPixels;
    let displayFeatures: Array<DisplayFeature> = await this.getDisplayFeature();

    Log.i(TAG, "Display on: " + JSON.stringify(this.displayInfo) + ". Display id:" + JSON.stringify(data))
    if (devicePixelRatio != this.viewportMetrics.devicePixelRatio ||
      physicalTouchSlop != this.viewportMetrics.physicalTouchSlop ||
      displayFeatures != this.viewportMetrics.displayFeatures) {
      this.viewportMetrics.devicePixelRatio = devicePixelRatio;
      this.viewportMetrics.physicalTouchSlop = physicalTouchSlop;
      this.viewportMetrics.displayFeatures = displayFeatures;
      this.needSetViewport = true;
      this.onAreaChange(null);
    }
  }

  private keyboardHeightChangeCallback = (data: number) => {
    Log.i(TAG, "keyboardHeightChangeCallback " + data);
    this.keyboardAvoidArea.bottomRect.height = data;
    this.onAreaChange(null);
  };

  private windowRectChangeCallback = (data: window.RectChangeOptions) => {
    Log.i(TAG, "windowRectChangeCallback " + data);
    this.windowPosition = data.rect as window.Rect;
    this.flutterEngine?.getTextInputChannel()?.setWindowPosition(this.windowPosition);
  }

  private accessibilityStateChangeCallback = (data: boolean) => {
    Log.i(TAG, `subscribe accessibility state change, result: ${JSON.stringify(data)}`);
    this.flutterEngine?.getFlutterNapi()?.accessibilityStateChange(data);
  }

  private async getDisplayFeature(): Promise<Array<DisplayFeature>> {
    let displayMode: display.FoldDisplayMode = display.getFoldDisplayMode();
    if (this.displayFeaturesCache.has(displayMode)) {
      return this.displayFeaturesCache.get(displayMode)!;
    }
    let displayFeatures: Array<DisplayFeature> = [];
    // Folded crease areas of screen
    let foldCreaseRegion: display.FoldCreaseRegion = display.getCurrentFoldCreaseRegion();
    if ((displayMode == display.FoldDisplayMode.FOLD_DISPLAY_MODE_FULL ||
      displayMode == display.FoldDisplayMode.FOLD_DISPLAY_MODE_COORDINATION) &&
      foldCreaseRegion != undefined) {
      let type: DisplayFeatureType = DisplayFeatureType.FOLD;
      let state: DisplayFeatureState = DisplayFeatureState.UNKNOWN;
      // FULL when internal screen is displayed
      // COORDINATION when both the internal and external screen are displayed
      if (displayMode == display.FoldDisplayMode.FOLD_DISPLAY_MODE_FULL ||
        displayMode == display.FoldDisplayMode.FOLD_DISPLAY_MODE_COORDINATION) {
        state = DisplayFeatureState.POSTURE_FLAT;
      // MAIN or SUB when external screen is displayed in large-screen or small-screen folding products
      } else if (displayMode == display.FoldDisplayMode.FOLD_DISPLAY_MODE_MAIN ||
        displayMode == display.FoldDisplayMode.FOLD_DISPLAY_MODE_SUB) {
        state = DisplayFeatureState.POSTURE_HALF_OPENED;
      }
      foldCreaseRegion.creaseRects.forEach((rect: display.Rect) => {
        displayFeatures.push(new DisplayFeature(rect, type, state));
      });
    }

    let cutoutInfo: display.CutoutInfo | undefined = await this.displayInfo?.getCutoutInfo();
    if (cutoutInfo != undefined) {
      // Unusable areas such as diggings and bangs
      cutoutInfo.boundingRects.forEach((bounding: display.Rect) => {
        displayFeatures.push(new DisplayFeature(bounding, DisplayFeatureType.CUTOUT));
      });
      // Curved areas of waterfall screen
      let waterFallRects: display.WaterfallDisplayAreaRects = cutoutInfo.waterfallDisplayAreaRects;
      if (!this.isEmptyRect(waterFallRects.left)) {
        displayFeatures.push(new DisplayFeature(waterFallRects.left, DisplayFeatureType.FOLD));
      }
      if (!this.isEmptyRect(waterFallRects.right)) {
        displayFeatures.push(new DisplayFeature(waterFallRects.right, DisplayFeatureType.FOLD));
      }
      if (!this.isEmptyRect(waterFallRects.top)) {
        displayFeatures.push(new DisplayFeature(waterFallRects.top, DisplayFeatureType.FOLD));
      }
      if (!this.isEmptyRect(waterFallRects.bottom)) {
        displayFeatures.push(new DisplayFeature(waterFallRects.bottom, DisplayFeatureType.FOLD));
      }
    }

    this.displayFeaturesCache.set(displayMode, displayFeatures);
    return displayFeatures;
  }

  getId(): string {
    return this.id;
  }

  setSurfaceId(surfaceId: string): void {
    this.surfaceId = surfaceId;
  }

  getSurfaceId(): string {
    return this.surfaceId;
  }

  setWrappedBuilder(wrappedBuilder: WrappedBuilder<[Params]>) {
    this.wrapBuilder = wrappedBuilder;
  }

  getWrappedBuilder(): WrappedBuilder<[Params]> | undefined {
    return this.wrapBuilder;
  }

  setPlatformView(platformView: PlatformView) {
    this.platformView = platformView;
  }

  getPlatformView(): PlatformView | undefined {
    return this.platformView;
  }

  getDVModel() {
    return this.dVModel;
  }

  getKeyboardHeight() {
    return this.keyboardAvoidArea.bottomRect.height
  }

  onDestroy() {
    try {
      this.mainWindow?.off('windowSizeChange', this.windowSizeChangeCallback);
      this.mainWindow?.off('avoidAreaChange', this.avoidAreaChangeCallback);
      this.mainWindow?.off('windowStatusChange', this.windowStatusChangeCallback);
      this.mainWindow?.off('keyboardHeightChange', this.keyboardHeightChangeCallback);
      this.mainWindow?.off('windowRectChange', this.windowRectChangeCallback);
      accessibility.off('accessibilityStateChange', this.accessibilityStateChangeCallback);
    } catch (e) {
      Log.e(TAG, "mainWindow off error: " + JSON.stringify(e));
    }
    this.mainWindow = null;

    try {
      display.off("change", this.displayChangeCallback);
    } catch (e) {
      Log.e(TAG, "displayInfo off error" + JSON.stringify(e));
    }
  }

  attachToFlutterEngine(flutterEngine: FlutterEngine): void {
    hiTraceMeter.startTrace("attachToFlutterEngine", 0);
    if (this.isAttachedToFlutterEngine()) {
      if (flutterEngine == this.flutterEngine) {
        Log.i(TAG, "Already attached to this engine. Doing nothing.");
        return;
      }
      // Detach from a previous FlutterEngine so we can attach to this new one.f
      Log.i(
        TAG,
        "Currently attached to a different engine. Detaching and then attaching"
          + " to new engine.");
      this.detachFromFlutterEngine();
    }
    Log.i(TAG, "attachToFlutterEngine");
    this.flutterEngine = flutterEngine;
    this.flutterEngine?.getFlutterNapi().xComponentAttachFlutterEngine(this.id)
    this.flutterEngine?.getFlutterNapi()?.updateRefreshRate(this.displayInfo!.refreshRate)
    this.flutterEngine?.getFlutterNapi()?.updateSize(this.displayInfo!.width, this.displayInfo!.height)
    this.flutterEngine?.getFlutterNapi()?.updateDensity(this.displayInfo!.densityPixels)
    flutterEngine.getPlatformViewsController()?.attachToView(this);

    let newArea: Area | null = {
      width: px2vp(this.displayInfo!.width),
      height: px2vp(this.displayInfo!.height),
      position: { x: 0, y: 0 },
      globalPosition: { x: 0, y: 0 }
    };
    if (this.viewportMetrics.physicalWidth != 0 || this.viewportMetrics.physicalHeight != 0) {
      newArea = null;
    }
    this.onAreaChange(newArea, true);

    let windowId = this.mainWindow?.getWindowProperties()?.id ?? 0
    this.mouseCursorPlugin = new MouseCursorPlugin(windowId, this.flutterEngine?.getMouseCursorChannel()!);
    this.textInputPlugin = new TextInputPlugin(this.flutterEngine?.getTextInputChannel()!);
    this.keyboardManager = new KeyboardManager(flutterEngine, this.textInputPlugin!);
    this.settings = new Settings(this.flutterEngine.getSettingsChannel()!);
    this.sendSettings();
    this.isFlutterUiDisplayed = this.flutterEngine.getFlutterNapi().isDisplayingFlutterUi;
    this.isFlutterUiPreload = this.flutterEngine.getFlutterNapi().isPreloadedFlutterUi;
    if (this.isFlutterUiPreload) {
      this.onFirstFrame(1);
    }
    if (this.isFlutterUiDisplayed) {
      this.onFirstFrame();
    }
    if (this.isSurfaceAvailableForRendering) {
      this.flutterEngine?.processPendingMessages();
    }
    hiTraceMeter.finishTrace("attachToFlutterEngine", 0);
  }

  preDraw(width: number = 0, height: number = 0): void {
    if (this.isAttachedToFlutterEngine()) {
      if (width == 0 || height == 0) {
        width = this.displayInfo!.width;
        height = this.displayInfo!.height;
      }
      this.flutterEngine?.getFlutterNapi().xComponentPreDraw(this.id, width, height);
    }
  }

  detachFromFlutterEngine(): void {
    Log.i(TAG, "detachFromFlutterEngine");
    if (!this.isAttachedToFlutterEngine()) {
      Log.d(TAG, "FlutterView not attached to an engine. Not detaching.");
      return;
    }
    if (this.isSurfaceAvailableForRendering) {
      this.flutterEngine!!.getFlutterNapi().xComponentDetachFlutterEngine(this.id)
    }
    this.flutterEngine?.getPlatformViewsController()?.detachFromView();
    this.flutterEngine = null;
    this.keyboardManager = null;
    this.textInputPlugin?.destroy();
  }

  onWindowCreated() {
    Log.d(TAG, "received onwindowCreated.");
    let _UIContext = this.mainWindow?.getUIContext();
    this.uiContext = _UIContext;
    this.sendSettings();
    Log.d(TAG, "uiContext init and sendSettings finished.");
  }

  sendSettings(): void {
    if (this.uiContext != undefined && this.isAttachedToFlutterEngine()) {
      this.settings?.sendSettings(this.uiContext.getMediaQuery());
    } else {
      Log.e(TAG, "UIContext is null, cannot send Settings!");
    }
  }

  onSurfaceCreated() {
    this.isSurfaceAvailableForRendering = true;
    this.flutterEngine?.processPendingMessages();
  }

  onSurfaceDestroyed() {
    this.isSurfaceAvailableForRendering = false;
    if (this.isAttachedToFlutterEngine()) {
      this.flutterEngine!!.getFlutterNapi().xComponentDetachFlutterEngine(this.id)
    }
  }

  onAreaChange(newArea: Area | null, setFullScreen: boolean = false) {
    const originalMetrics = this.viewportMetrics.clone();
    if (newArea != null) {
      this.viewportMetrics.physicalWidth = vp2px(newArea.width as number);
      this.viewportMetrics.physicalHeight = vp2px(newArea.height as number);
    }
    let fullScreen = false
    // 根据是否全屏显示，设置标题栏高度
    if (this.checkFullScreen &&
      (setFullScreen || FlutterManager.getInstance().getFullScreenListener().useFullScreen())) { // 全屏显示
      fullScreen = true
      this.viewportMetrics.physicalViewPaddingTop = this.systemAvoidArea!.topRect.height
      this.viewportMetrics.physicalViewPaddingBottom = this.systemAvoidArea!.bottomRect.height
    } else { // 非全屏显示
      this.viewportMetrics.physicalViewPaddingTop = 0;
      this.viewportMetrics.physicalViewPaddingBottom = 0;
    }

    this.viewportMetrics.physicalViewPaddingLeft = this.systemAvoidArea!.leftRect.width
    this.viewportMetrics.physicalViewPaddingRight = this.systemAvoidArea!.rightRect.width

    this.onKeyboardAreaChange(fullScreen)
    this.onAiBarAreaChange(fullScreen)
    this.onGestureAreaChange(fullScreen)
    if (!this.viewportMetrics.isEqual(originalMetrics) || this.needSetViewport) {
      if (!this.updateViewportMetrics()) {
        this.needSetViewport = true;
      } else {
        this.needSetViewport = false;
      }
    }
  }

  // At the bottom of the single-frame phone is an AI assistant wake-up bar
  // The area can lead to confusing gestures in repetitive areas without handled properly
  // physicalViewPaddingBottom is modified by this function which needs to be called after onKeyboardAreaChange
  private onAiBarAreaChange(fullScreen: boolean = false) {
    if (this.checkAiBar && this.navigationAvoidArea != null && fullScreen) {
      this.viewportMetrics.physicalViewPaddingBottom =
        Math.max(this.navigationAvoidArea.bottomRect.height, this.viewportMetrics.physicalViewPaddingBottom)
    }
  }

  private onKeyboardAreaChange(fullScreen: boolean = false) {
    if (this.checkKeyboard && fullScreen) {
      this.viewportMetrics.physicalViewInsetTop = this.keyboardAvoidArea!.topRect.height
      this.viewportMetrics.physicalViewInsetLeft = this.keyboardAvoidArea!.leftRect.width
      this.viewportMetrics.physicalViewInsetBottom = this.keyboardAvoidArea!.bottomRect.height
      this.viewportMetrics.physicalViewInsetRight = this.keyboardAvoidArea!.rightRect.width
    } else {
      this.viewportMetrics.physicalViewInsetTop = 0
      this.viewportMetrics.physicalViewInsetLeft = 0
      this.viewportMetrics.physicalViewInsetBottom = 0
      this.viewportMetrics.physicalViewInsetRight = 0
    }
  }

  private onGestureAreaChange(fullScreen: boolean = false) {
    if (this.checkGesture && fullScreen) {
      this.viewportMetrics.systemGestureInsetTop = this.gestureAvoidArea!.topRect.height
      this.viewportMetrics.systemGestureInsetLeft = this.gestureAvoidArea!.leftRect.width
      this.viewportMetrics.systemGestureInsetBottom =
        Math.max(this.navigationAvoidArea.bottomRect.height, this.gestureAvoidArea!.bottomRect.height)
      this.viewportMetrics.systemGestureInsetRight = this.gestureAvoidArea!.rightRect.width
    } else {
      this.viewportMetrics.systemGestureInsetTop = 0
      this.viewportMetrics.systemGestureInsetLeft = 0
      this.viewportMetrics.systemGestureInsetBottom = 0
      this.viewportMetrics.systemGestureInsetRight = 0
    }
  }

  public isAttachedToFlutterEngine(): boolean {
    return this.flutterEngine != null
  }

  public isSameEngineShellHolderId(id: number): boolean {
    if (this.flutterEngine) {
      let flutterNapi = this.flutterEngine.getFlutterNapi();
      if (flutterNapi.nativeShellHolderId == id && id != 0) {
        return true;
      }
    }
    return false;
  }

  private updateViewportMetrics(): boolean {
    if (this.isAttachedToFlutterEngine()) {
      const displayFeature = this.viewportMetrics.displayFeatures;
      let displayFeaturesBounds: number[] = new Array(displayFeature.length * 4);
      let displayFeaturesType: number[] = new Array(displayFeature.length);
      let displayFeaturesState: number[] = new Array(displayFeature.length);

      for (let i = 0; i < displayFeature.length; i++) {
        displayFeaturesBounds[4*i] = displayFeature[i].bounds.left;
        displayFeaturesBounds[4*i + 1] = displayFeature[i].bounds.top;
        displayFeaturesBounds[4*i + 2] = displayFeature[i].bounds.width;
        displayFeaturesBounds[4*i + 3] = displayFeature[i].bounds.height;
        displayFeaturesType[i] = displayFeature[i].type;
        displayFeaturesState[i] = displayFeature[i].state;
      }
      this?.flutterEngine?.getFlutterNapi()?.setViewportMetrics(this.viewportMetrics.devicePixelRatio,
        this.viewportMetrics.physicalWidth,
        this.viewportMetrics.physicalHeight,
        this.viewportMetrics.physicalViewPaddingTop,
        this.viewportMetrics.physicalViewPaddingRight,
        this.viewportMetrics.physicalViewPaddingBottom,
        this.viewportMetrics.physicalViewPaddingLeft,
        this.viewportMetrics.physicalViewInsetTop,
        this.viewportMetrics.physicalViewInsetRight,
        this.viewportMetrics.physicalViewInsetBottom,
        this.viewportMetrics.physicalViewInsetLeft,
        this.viewportMetrics.systemGestureInsetTop,
        this.viewportMetrics.systemGestureInsetRight,
        this.viewportMetrics.systemGestureInsetBottom,
        this.viewportMetrics.systemGestureInsetLeft,
        this.viewportMetrics.physicalTouchSlop,
        displayFeaturesBounds,
        displayFeaturesType,
        displayFeaturesState)
      return true;
    }
    return false;
  }

  onKeyPreIme(event: KeyEvent) : boolean {
    return this.keyboardManager?.onKeyPreIme(event) ?? false;
  }

  onKeyEvent(event: KeyEvent) : boolean {
    return this.keyboardManager?.onKeyEvent(event) ?? false;
  }

  onMouseWheel(eventType: string, event: PanGestureEvent) {
    this.flutterEngine?.getFlutterNapi()?.xComponentDisPatchMouseWheel(this.id, eventType, event);
  }

  addFirstFrameListener(listener: FirstFrameListener) {
    this.mFirstFrameListeners.add(listener);
  }

  removeFirstFrameListener(listener: FirstFrameListener) {
    this.mFirstFrameListeners.remove(listener);
  }

  addFirstPreloadFrameListener(listener: FirstPreloadFrameListener) {
    this.mFirstPreloadFrameListeners.add(listener);
  }

  removeFirstPreloadFrameListener(listener: FirstPreloadFrameListener) {
    this.mFirstPreloadFrameListeners.remove(listener);
  }

  hasRenderedFirstFrame(): boolean {
    return this.isFlutterUiDisplayed;
  }

  onFirstFrame(isPreload: number = 0) {
    if (isPreload) {
      let listeners = this.mFirstPreloadFrameListeners.clone();
      listeners.forEach((listener) => {
        listener.onFirstPreloadFrame();
      })
    } else {
      let listeners = this.mFirstFrameListeners.clone();
      listeners.forEach((listener) => {
        listener.onFirstFrame();
      })
    }
  }

  setCheckFullScreen(check: boolean) {
    this.checkFullScreen = check;
  }

  setCheckKeyboard(check: boolean) {
    this.checkKeyboard = check
  }

  setCheckGesture(check: boolean) {
    this.checkGesture = check
  }

  setCheckAiBar(check: boolean) {
    this.checkAiBar = check
  }

  isEmptyRect(rect: display.Rect | null | undefined) {
    if (rect == null || rect == undefined) {
      return true;
    }
    return rect.left == 0 && rect.top == 0 && rect.width == 0 && rect.height == 0;
  }
}

export interface FirstFrameListener {
  onFirstFrame(): void;
}

export interface FirstPreloadFrameListener {
  onFirstPreloadFrame(): void;
}

export class DisplayFeature {
  bounds: display.Rect;
  type: DisplayFeatureType;
  state: DisplayFeatureState;

  constructor(bounds: display.Rect, type: DisplayFeatureType, state: DisplayFeatureState = DisplayFeatureState.UNKNOWN) {
    this.bounds = bounds;
    this.type = type;
    this.state = state;
  }
}

// Unusual types of areas in screen such as occlusion, folding or curved areas
export enum DisplayFeatureType {
  UNKNOWN = 0,
  FOLD = 1,
  HINGE = 2,
  CUTOUT = 3,
}

// State of screen
export enum DisplayFeatureState {
  UNKNOWN = 0,
  POSTURE_FLAT = 1,
  POSTURE_HALF_OPENED = 2,
}
