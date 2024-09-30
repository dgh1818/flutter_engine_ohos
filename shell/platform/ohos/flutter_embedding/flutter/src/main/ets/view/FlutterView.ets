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
import display from '@ohos.display';
import FlutterManager from '../embedding/ohos/FlutterManager';
import window from '@ohos.window';
import KeyboardManager from '../embedding/ohos/KeyboardManager';
import MouseCursorPlugin from '../plugin/mouse/MouseCursorPlugin';
import Any from '../plugin/common/Any';
import Settings from '../embedding/ohos/Settings';
import ArrayList from '@ohos.util.ArrayList';
import PlatformView, { Params } from '../plugin/platform/PlatformView';
import hiTraceMeter from '@ohos.hiTraceMeter'
import { JSON } from '@kit.ArkTS';

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
      this.physicalTouchSlop === other.physicalTouchSlop;
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
    this.systemAvoidArea = this.mainWindow?.getWindowAvoidArea(window.AvoidAreaType.TYPE_SYSTEM);
    this.navigationAvoidArea = this.mainWindow?.getWindowAvoidArea(window.AvoidAreaType.TYPE_NAVIGATION_INDICATOR);
    this.gestureAvoidArea = this.mainWindow?.getWindowAvoidArea(window.AvoidAreaType.TYPE_SYSTEM_GESTURE);
    this.keyboardAvoidArea = this.mainWindow?.getWindowAvoidArea(window.AvoidAreaType.TYPE_KEYBOARD);
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

  getId(): string {
    return this.id
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
    return this.dVModel
  }

  onDestroy() {
    try {
      this.mainWindow?.off('windowSizeChange', this.windowSizeChangeCallback);
      this.mainWindow?.off('avoidAreaChange', this.avoidAreaChangeCallback);
      this.mainWindow?.off('windowStatusChange', this.windowStatusChangeCallback);
    } catch (e) {
      Log.e(TAG, "mainWindow off error: " + JSON.stringify(e));
    }
    this.mainWindow = null;
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
    this.keyboardManager = new KeyboardManager(flutterEngine);
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

    // 根据是否全屏显示，设置标题栏高度
    if (this.checkFullScreen &&
      (setFullScreen || FlutterManager.getInstance().getFullScreenListener().useFullScreen())) { // 全屏显示
      this.viewportMetrics.physicalViewPaddingTop = this.systemAvoidArea!.topRect.height
      this.viewportMetrics.physicalViewPaddingBottom = this.navigationAvoidArea!.bottomRect.height
    } else { // 非全屏显示
      this.viewportMetrics.physicalViewPaddingTop = 0;
      this.viewportMetrics.physicalViewPaddingBottom = 0;
    }

    this.viewportMetrics.physicalViewPaddingLeft = this.systemAvoidArea!.leftRect.width
    this.viewportMetrics.physicalViewPaddingRight = this.systemAvoidArea!.rightRect.width

    this.onKeyboardAreaChange()
    this.onAiBarAreaChange()
    this.onGestureAreaChange()
    if (!this.viewportMetrics.isEqual(originalMetrics) || this.needSetViewport) {
      if (!this.updateViewportMetrics()) {
        this.needSetViewport = true;
      }
    }
  }

  // At the bottom of the single-frame phone is an AI assistant wake-up bar
  // The area can lead to confusing gestures in repetitive areas without handled properly
  private onAiBarAreaChange() {
    if (this.checkAiBar && this.navigationAvoidArea != null) {
      this.viewportMetrics.physicalViewInsetBottom =
        Math.max(this.navigationAvoidArea.bottomRect.height, this.viewportMetrics.physicalViewInsetBottom)
    } else {
      this.viewportMetrics.physicalViewInsetBottom = 0
    }
  }

  private onKeyboardAreaChange() {
    if (this.checkKeyboard) {
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

  private onGestureAreaChange() {
    if (this.checkGesture) {
      this.viewportMetrics.systemGestureInsetTop = this.gestureAvoidArea!.topRect.height
      this.viewportMetrics.systemGestureInsetLeft = this.gestureAvoidArea!.leftRect.width
      this.viewportMetrics.systemGestureInsetBottom = this.gestureAvoidArea!.bottomRect.height
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
        new Array(0),
        new Array(0),
        new Array(0))
      return true
    }
    return false
  }

  onKeyEvent(event: KeyEvent) {
    this.keyboardManager?.handleKeyEvent(event)
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
}

export interface FirstFrameListener {
  onFirstFrame(): void;
}

export interface FirstPreloadFrameListener {
  onFirstPreloadFrame(): void;
}
