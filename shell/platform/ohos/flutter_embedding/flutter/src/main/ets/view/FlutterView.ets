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
import { EmbeddingNodeController } from '../embedding/ohos/EmbeddingNodeController';
import PlatformView, { Params } from '../plugin/platform/PlatformView';

const TAG = "FlutterView";

class ViewportMetrics {
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
  physicalTouchSlop = -1;
}

export class PlatformViewParas {
  width: number = 0.0;
  height: number = 0.0;
  top: number = 0.0;
  left: number = 0.0;
  direction: Direction = Direction.Auto;

  SetValue(width: number, height: number, top: number, left: number): void {
    this.width = width;
    this.height = height;
    this.top = top;
    this.left = left;
  }

  SetOffset(top: number, left: number): void {
    this.top = top;
    this.left = left;
  }
}

export class FlutterView {
  private flutterEngine: FlutterEngine | null = null
  private id: string = ""
  private dVModel: DVModel = new DVModel("Stack", new DVModelParameters(), new DVModelEvents(), new DVModelChildren(), null);
  private wrapBuilder: WrappedBuilder<[Params]> | undefined = undefined;
  private platformView: PlatformView | undefined = undefined;
  private isSurfaceAvailableForRendering: boolean = false
  private viewportMetrics = new ViewportMetrics();
  private displayInfo?: display.Display;
  private keyboardManager: KeyboardManager | null = null;
  private mainWindow: window.Window
  private mouseCursorPlugin?: MouseCursorPlugin;
  private uiContext?: UIContext | undefined;
  private settings?: Settings;
  private mFirstFrameListeners: ArrayList<FirstFrameListener>;
  private isFlutterUiDisplayed: boolean = false;
  private surfaceId: string = "0";
  private nodeController: EmbeddingNodeController = new EmbeddingNodeController();
  private platformViewSize: PlatformViewParas = new PlatformViewParas();
  private checkFullScreen: boolean = true;
  private checkKeyboard: boolean = true;
  private checkGesture: boolean = true;

  constructor(viewId: string, context: Context) {
    this.id = viewId
    this.displayInfo = display.getDefaultDisplaySync();
    this.viewportMetrics.devicePixelRatio = this.displayInfo?.densityPixels
    this.mainWindow = FlutterManager.getInstance()
      .getWindowStage(FlutterManager.getInstance().getUIAbility(context))
      .getMainWindowSync();
    this.mFirstFrameListeners = new ArrayList<FirstFrameListener>();

    this.mainWindow?.on('windowSizeChange', this.windowSizeChangeCallback);
    this.mainWindow?.on('avoidAreaChange', this.avoidAreaChangeCallback);
    this.mainWindow?.on('keyboardHeightChange', this.keyboardHeightChangeCallback);
    this.mainWindow?.on('windowStatusChange', (data: window.WindowStatusType) => {
      Log.i(TAG, "windowStatusChangeCallback " + JSON.stringify(data));
      FlutterManager.getInstance().getFullScreenListener().onScreenStateChanged(data);
      this.onAreaChange(null);
    });
  }

  private  windowSizeChangeCallback= (data: window.Size) => {
    Log.i(TAG, "windowSizeChangeCallback");
    this.onAreaChange(null);
  }
  private avoidAreaChangeCallback = (data: Any) => {
    Log.i(TAG, "avoidAreaChangeCallback");
    this.onAreaChange(null);
  }
  private keyboardHeightChangeCallback = (data: number) => {
    Log.i(TAG, "keyboardHeightChangeCallback");
    this.onAreaChange(null);
  }

  getId(): string {
    return this.id
  }

  setSurfaceId(surfaceId: string): void {
    this.surfaceId = surfaceId;
  }

  getSurfaceId(): string {
    return this.surfaceId;
  }

  getEmbeddingNodeController(): EmbeddingNodeController {
    return this.nodeController;
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

  getPlatformViewSize(): PlatformViewParas {
    return this.platformViewSize;
  }

  getDVModel() {
    return this.dVModel
  }

  onDestroy() {
    try {
      this.mainWindow?.off('windowSizeChange', this.windowSizeChangeCallback);
      this.mainWindow?.off('avoidAreaChange', this.avoidAreaChangeCallback);
      this.mainWindow?.off('keyboardHeightChange', this.keyboardHeightChangeCallback);
      this.mainWindow?.off('windowStatusChange');
    } catch (e) {
    }
  }

  attachToFlutterEngine(flutterEngine: FlutterEngine): void {
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
    if (this.isSurfaceAvailableForRendering) {
      this.flutterEngine.getFlutterNapi().xComponentAttachFlutterEngine(this.id)
    }
    this.flutterEngine?.getFlutterNapi()?.updateRefreshRate(this.displayInfo!.refreshRate)
    this.flutterEngine?.getFlutterNapi()?.updateSize(this.displayInfo!.width, this.displayInfo!.height)
    this.flutterEngine?.getFlutterNapi()?.updateDensity(this.displayInfo!.densityPixels)
    flutterEngine.getPlatformViewsController()?.attachToView(this);
    this.updateViewportMetrics()
    let windowId = this.mainWindow?.getWindowProperties()?.id
    this.mouseCursorPlugin = new MouseCursorPlugin(windowId, this.flutterEngine?.getMouseCursorChannel()!);
    this.settings = new Settings(this.flutterEngine.getSettingsChannel()!);
    this.sendSettings();
    this.isFlutterUiDisplayed = this.flutterEngine.getFlutterNapi().isDisplayingFlutterUi;
    if (this.isFlutterUiDisplayed) {
      this.onFirstFrame();
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
    if (this.isAttachedToFlutterEngine()) {
      this.flutterEngine!!.getFlutterNapi().xComponentAttachFlutterEngine(this.id)
    }
  }

  onSurfaceDestroyed() {
    this.isSurfaceAvailableForRendering = false;
    if (this.isAttachedToFlutterEngine()) {
      this.flutterEngine!!.getFlutterNapi().xComponentDetachFlutterEngine(this.id)
    }
  }

  onAreaChange(newArea: Area | null) {
    let systemAvoidArea = this.mainWindow?.getWindowAvoidArea(window.AvoidAreaType.TYPE_SYSTEM);
    let navigationAvoidArea = this.mainWindow?.getWindowAvoidArea(window.AvoidAreaType.TYPE_NAVIGATION_INDICATOR);

    if (newArea != null) {
      this.viewportMetrics.physicalWidth = vp2px(newArea.width as number);
      this.viewportMetrics.physicalHeight = vp2px(newArea.height as number);
    }

    // 根据是否全屏显示，设置标题栏高度
    if (this.checkFullScreen && FlutterManager.getInstance().getFullScreenListener().useFullScreen()) { // 全屏显示
      this.viewportMetrics.physicalViewPaddingTop = systemAvoidArea!.topRect.height
      this.viewportMetrics.physicalViewPaddingBottom = navigationAvoidArea!.bottomRect.height
    } else { // 非全屏显示
      this.viewportMetrics.physicalViewPaddingTop = 0;
      this.viewportMetrics.physicalViewPaddingBottom = 0;
    }

    this.viewportMetrics.physicalViewPaddingLeft = systemAvoidArea!.leftRect.width
    this.viewportMetrics.physicalViewPaddingRight = systemAvoidArea!.rightRect.width

    this.onKeyboardAreaChange()
    this.onGestureAreaChange()
    this.updateViewportMetrics()
  }

  private onKeyboardAreaChange() {
    if (this.checkKeyboard) {
      let keyboardAvoidArea = this.mainWindow?.getWindowAvoidArea(window.AvoidAreaType.TYPE_KEYBOARD);
      this.viewportMetrics.physicalViewInsetTop = keyboardAvoidArea!.topRect.height
      this.viewportMetrics.physicalViewInsetLeft = keyboardAvoidArea!.leftRect.width
      this.viewportMetrics.physicalViewInsetBottom = keyboardAvoidArea!.bottomRect.height
      this.viewportMetrics.physicalViewInsetRight = keyboardAvoidArea!.rightRect.width
    } else {
      this.viewportMetrics.physicalViewInsetTop = 0
      this.viewportMetrics.physicalViewInsetLeft = 0
      this.viewportMetrics.physicalViewInsetBottom = 0
      this.viewportMetrics.physicalViewInsetRight = 0
    }
  }

  private onGestureAreaChange() {
    if (this.checkGesture) {
      let gestureAvoidArea = this.mainWindow?.getWindowAvoidArea(window.AvoidAreaType.TYPE_SYSTEM_GESTURE);
      this.viewportMetrics.systemGestureInsetTop = gestureAvoidArea!.topRect.height
      this.viewportMetrics.systemGestureInsetLeft = gestureAvoidArea!.leftRect.width
      this.viewportMetrics.systemGestureInsetBottom = gestureAvoidArea!.bottomRect.height
      this.viewportMetrics.systemGestureInsetRight = gestureAvoidArea!.rightRect.width
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

  private updateViewportMetrics() {
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
    }
  }

  onKeyEvent(event: KeyEvent) {
    this.keyboardManager?.handleKeyEvent(event)
  }

  addFirstFrameListener(listener: FirstFrameListener) {
    this.mFirstFrameListeners.add(listener);
  }

  removeFirstFrameListener(listener: FirstFrameListener) {
    this.mFirstFrameListeners.remove(listener);
  }

  hasRenderedFirstFrame(): boolean {
    return this.isFlutterUiDisplayed;
  }

  onFirstFrame() {
    let listeners = this.mFirstFrameListeners.clone();
    listeners.forEach((listener) => {
      listener.onFirstFrame();
    })
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
}

export interface FirstFrameListener {
  onFirstFrame(): void;
}
