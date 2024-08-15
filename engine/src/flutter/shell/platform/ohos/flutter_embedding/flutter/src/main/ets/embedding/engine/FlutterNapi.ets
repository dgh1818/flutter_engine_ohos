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

import flutter from 'libflutter.so';
import common from '@ohos.app.ability.common';
import Log from '../../util/Log';
import resourceManager from '@ohos.resourceManager';
import { PlatformMessageHandler } from './dart/PlatformMessageHandler';
import { FlutterCallbackInformation } from '../../view/FlutterCallbackInformation';
import image from '@ohos.multimedia.image';
import { EngineLifecycleListener } from './FlutterEngine';
import { ByteBuffer } from '../../util/ByteBuffer';
import { Action } from '../../view/AccessibilityBridge'
import LocalizationPlugin from '../../plugin/localization/LocalizationPlugin';
import i18n from '@ohos.i18n';
import Any from '../../plugin/common/Any';
import FlutterManager from '../ohos/FlutterManager';
import deviceInfo from '@ohos.deviceInfo';
import TouchEventProcessor from '../ohos/TouchEventProcessor';
import { EmbeddingNodeController } from '../ohos/EmbeddingNodeController';

const TAG = "FlutterNapi";

enum ContextType {
  APP_LIFECYCLE = 0,
  JS_PAGE_LIFECYCLE,
}

interface PendingMessage {
  channel: string;
  message: ArrayBuffer;
  replyId: number;
  messageData: number;
}

/**
 * 提供arkTs的flutterNAPI接口
 */
export default class FlutterNapi {
  private static hasInit: boolean = false;
  //是否已实现
  hasImplemented: boolean = false;
  nativeShellHolderId: number | null = null;
  platformMessageHandler: PlatformMessageHandler | null = null;
  private engineLifecycleListeners = new Set<EngineLifecycleListener>();
  accessibilityDelegate: AccessibilityDelegate | null = null;
  localizationPlugin: LocalizationPlugin | null = null;
  isDisplayingFlutterUi: boolean = false;
  isPreloadedFlutterUi: boolean = false;
  isRunningDart: boolean = false;
  private nextSpawnNapi: FlutterNapi | null = null;
  private pendingMessages: PendingMessage[] = [];
  private readyForHandleMessage: boolean = false;

  /**
   * 更新刷新率
   * @param rate
   */
  updateRefreshRate(refreshRateFPS: number) {
    flutter.nativeUpdateRefreshRate(refreshRateFPS);
  }

  updateSize(width: number, height: number) {
    flutter.nativeUpdateSize(width, height);
  }

  updateDensity(densityPixels: number) {
    flutter.nativeUpdateDensity(densityPixels);
  }

  init(context: common.Context,
    args: Array<string>,
    bundlePath: string,
    appStoragePath: string,
    engineCachesPath: string,
    initTimeMillis: number) {
    if (FlutterNapi.hasInit) {
      Log.e(TAG, "the engine has init");
      return;
    }
    Log.w(TAG, JSON.stringify({
      "name": "init, initTimeMillis=" + initTimeMillis,
      "bundlePath": bundlePath,
      "appStoragePath": appStoragePath,
      "engineCachesPath": engineCachesPath,
      "args": args,
    }));
    let code: number | null = flutter.nativeInit(context, args, bundlePath, appStoragePath,
      engineCachesPath, initTimeMillis, deviceInfo.productModel);
    FlutterNapi.hasInit = code == 0;
    Log.i(TAG, "init code=" + code + ", FlutterNapi.hasInit" + FlutterNapi.hasInit);
  }

  static prefetchDefaultFontManager(): void {
    flutter.nativePrefetchDefaultFontManager();
  }

  attachToNative(): void {
    if (!FlutterNapi.hasInit) {
      Log.e(TAG, "attachToNative fail, FlutterNapi.hasInit=" + FlutterNapi.hasInit);
      return;
    }
    if (this.nativeShellHolderId == null) {
      this.nativeShellHolderId = flutter.nativeAttach(this);
    }
    Log.w(TAG, "nativeShellHolderId=" + this.nativeShellHolderId);
  }

  runBundleAndSnapshotFromLibrary(
    bundlePath: string,
    entrypointFunctionName: string,
    pathToEntrypointFunction: string,
    assetManager: resourceManager.ResourceManager,
    entrypointArgs: Array<string>) {
    if (!FlutterNapi.hasInit) {
      Log.e(TAG, "runBundleAndSnapshotFromLibrary fail, FlutterNapi.hasInit=" + FlutterNapi.hasInit);
      return;
    }
    Log.i(TAG, "init: bundlePath=" + bundlePath + "  entrypointFunctionName=" + entrypointFunctionName +
      "  pathToEntrypointFunction=" + pathToEntrypointFunction + "  entrypointArgs=" + JSON.stringify(entrypointArgs))
    flutter.nativeRunBundleAndSnapshotFromLibrary(this.nativeShellHolderId!, bundlePath, entrypointFunctionName,
      pathToEntrypointFunction, assetManager, entrypointArgs);
    this.isRunningDart = true;
    this.isDisplayingFlutterUi = false;
    this.isPreloadedFlutterUi = false;
  };

  /**
   * 当前so方法是否都实现
   * @returns
   */
  checkImplemented(methodName: string = ""): boolean {
    if (!this.hasImplemented) {
      Log.e(TAG, "this method has not implemented -> " + methodName)
    }
    return this.hasImplemented;
  }

  setPlatformMessageHandler(platformMessageHandler: PlatformMessageHandler | null): void {
    this.ensureRunningOnMainThread();
    this.platformMessageHandler = platformMessageHandler;
  }

  private ensureAttachedToNative(): void {
    if (this.nativeShellHolderId == null) {
      throw new Error(
        "Cannot execute operation because FlutterNapi is not attached to native.");
    }
  }

  private nativeNotifyLowMemoryWarning(nativeShellHolderId: number): void {

  }

  static nativeLookupCallbackInformation(handle: number): FlutterCallbackInformation | null {
    return null;
  }

  notifyLowMemoryWarning(): void {
    this.ensureRunningOnMainThread();
    this.ensureAttachedToNative();
    this.nativeNotifyLowMemoryWarning(this.nativeShellHolderId!);
  }

  isAttached(): boolean {
    return this.nativeShellHolderId != null;
  }

  private ensureRunningOnMainThread(): void {

  }

  dispatchEmptyPlatformMessage(channel: String, responseId: number): void {
    this.ensureRunningOnMainThread();
    if (this.isAttached()) {
      flutter.nativeDispatchEmptyPlatformMessage(this.nativeShellHolderId!, channel, responseId);
    } else {
      Log.w(
        TAG,
        "Tried to send a platform message to Flutter, but FlutterNapi was detached from native C++. Could not send. Channel: "
          + channel
          + ". Response ID: "
          + responseId);
    }
  }

  /** Sends a reply {@code message} from HarmonyOS to Flutter over the given {@code channel}. */
  dispatchPlatformMessage(channel: String, message: ArrayBuffer, position: number, responseId: number): void {
    this.ensureRunningOnMainThread();
    if (this.isAttached()) {
      flutter.nativeDispatchPlatformMessage(this.nativeShellHolderId!, channel, message, position, responseId);
    } else {
      Log.w(
        TAG,
        "Tried to send a platform message to Flutter, but FlutterNapi was detached from native C++. Could not send. Channel: "
          + channel
          + ". Response ID: "
          + responseId);
    }
  }

  invokePlatformMessageEmptyResponseCallback(responseId: number): void {
    if (this.isAttached()) {
      flutter.nativeInvokePlatformMessageEmptyResponseCallback(this.nativeShellHolderId!, responseId);
    } else {
      Log.w(
        TAG,
        "Tried to send a platform message response, but FlutterNapi was detached from native C++. Could not send. Response ID: "
          + responseId);
    }
  }

  invokePlatformMessageResponseCallback(responseId: number, message: ArrayBuffer, position: number) {
    if (this.isAttached()) {
      flutter.nativeInvokePlatformMessageResponseCallback(
        this.nativeShellHolderId!, responseId, message, position);
    } else {
      Log.w(
        TAG,
        "Tried to send a platform message response, but FlutterNapi was detached from native C++. Could not send. Response ID: "
          + responseId);
    }
  }

  setViewportMetrics(devicePixelRatio: number, physicalWidth: number
    , physicalHeight: number, physicalPaddingTop: number, physicalPaddingRight: number
    , physicalPaddingBottom: number, physicalPaddingLeft: number, physicalViewInsetTop: number
    , physicalViewInsetRight: number, physicalViewInsetBottom: number, physicalViewInsetLeft: number
    , systemGestureInsetTop: number, systemGestureInsetRight: number, systemGestureInsetBottom: number
    , systemGestureInsetLeft: number, physicalTouchSlop: number, displayFeaturesBounds: Array<number>
    , displayFeaturesType: Array<number>, displayFeaturesState: Array<number>): void {
    if (this.isAttached()) {
      flutter.nativeSetViewportMetrics(this.nativeShellHolderId!, devicePixelRatio,
        physicalWidth,
        physicalHeight,
        physicalPaddingTop,
        physicalPaddingRight,
        physicalPaddingBottom,
        physicalPaddingLeft,
        physicalViewInsetTop,
        physicalViewInsetRight,
        physicalViewInsetBottom,
        physicalViewInsetLeft,
        systemGestureInsetTop,
        systemGestureInsetRight,
        systemGestureInsetBottom,
        systemGestureInsetLeft,
        physicalTouchSlop,
        displayFeaturesBounds,
        displayFeaturesType,
        displayFeaturesState);
    }
  }

  spawn(entrypointFunctionName: string, pathToEntrypointFunction: string, initialRoute: string,
    entrypointArgs: Array<string>): FlutterNapi {
    if (this.nextSpawnNapi) {
      let ret = this.nextSpawnNapi;
      this.nextSpawnNapi = null;
      return ret;
    }
    let flutterNapi = new FlutterNapi();
    let shellHolderId: number =
      flutter.nativeSpawn(this.nativeShellHolderId, entrypointFunctionName, pathToEntrypointFunction, initialRoute,
        entrypointArgs, flutterNapi);
    flutterNapi.nativeShellHolderId = shellHolderId;
    flutterNapi.isRunningDart = this.isRunningDart;
    flutterNapi.isDisplayingFlutterUi = false;
    flutterNapi.isPreloadedFlutterUi = false;
    return flutterNapi;
  }

  preSpawn(entrypointFunctionName: string, pathToEntrypointFunction: string, initialRoute: string,
    entrypointArgs: Array<string>): FlutterNapi {
    if (this.nextSpawnNapi) {
      this.nextSpawnNapi.detachFromNativeAndReleaseResources();
    }
    let flutterNapi = new FlutterNapi();
    let shellHolderId: number =
      flutter.nativeSpawn(this.nativeShellHolderId, entrypointFunctionName, pathToEntrypointFunction, initialRoute,
        entrypointArgs, flutterNapi);
    flutterNapi.nativeShellHolderId = shellHolderId;
    flutterNapi.isRunningDart = this.isRunningDart;
    flutterNapi.isDisplayingFlutterUi = false;
    flutterNapi.isPreloadedFlutterUi = false;
    this.nextSpawnNapi = flutterNapi;
    return flutterNapi;
  }

  addEngineLifecycleListener(engineLifecycleListener: EngineLifecycleListener): void {
    this.engineLifecycleListeners.add(engineLifecycleListener);
  }

  removeEngineLifecycleListener(engineLifecycleListener: EngineLifecycleListener) {
    this.engineLifecycleListeners.delete(engineLifecycleListener);
  }

  //Called by native to respond to a platform message that we sent.
  handlePlatformMessageResponse(replyId: number, reply: ArrayBuffer): void {
    Log.d(TAG, "called handlePlatformMessageResponse Response ID: " + replyId);
    if (this.platformMessageHandler != null) {
      this.platformMessageHandler.handlePlatformMessageResponse(replyId, reply);
    }
  }

  // Called by native on any thread.
  handlePlatformMessage(channel: string, message: ArrayBuffer, replyId: number, messageData: number): void {
    Log.d(TAG, "called handlePlatformMessage Channel: " + channel + ". Response ID: " + replyId);
    if (this.platformMessageHandler != null && this.readyForHandleMessage) {  
      this.platformMessageHandler.handleMessageFromDart(channel, message, replyId, messageData);
    } else {
      const pendingMessage: PendingMessage = { channel, message, replyId, messageData };
      this.pendingMessages.push(pendingMessage);
    }
  }

  processPendingMessages(): void {
    Log.d(TAG, "processPendingMessages len:" + this.pendingMessages.length);
    this.readyForHandleMessage = true;
    while (this.pendingMessages.length > 0 && this.platformMessageHandler) {
      const pendingMessage = this.pendingMessages.shift();
      if (pendingMessage) {
        this.platformMessageHandler.handleMessageFromDart(
          pendingMessage.channel, 
          pendingMessage.message, 
          pendingMessage.replyId, 
          pendingMessage.messageData
        );
      }
    }
  }

  // Called by native to notify first Flutter frame rendered.
  onFirstFrame(isPreload: number): void {
    Log.d(TAG, "called onFirstFrame isPreload:" + isPreload);
    if (isPreload) {
      this.isPreloadedFlutterUi = true;
    } else {
      if (this.isDisplayingFlutterUi) {
        return;
      }
      this.isDisplayingFlutterUi = true;
    }
    FlutterManager.getInstance().getFlutterViewList().forEach((value) => {
      if (this.nativeShellHolderId != null && value.isSameEngineShellHolderId(this.nativeShellHolderId)) {
        value.onFirstFrame(isPreload);
      }
    });
  }

  // Called by native.
  onPreEngineRestart(): void {
    Log.d(TAG, "called onPreEngineRestart")
    this.engineLifecycleListeners.forEach(listener => listener.onPreEngineRestart());
  }

  //  /** Invoked by native to obtain the results of OHOS's locale resolution algorithm. */
  computePlatformResolvedLocale(strings: Array<string>): Array<string> {
    Log.d(TAG, "called computePlatformResolvedLocale " + JSON.stringify(strings))
    return []
  }

  decodeImage(buffer: ArrayBuffer, imageGeneratorAddress: number): void {
    if (buffer) {
      Log.d(TAG, "called decodeImage=" + buffer.byteLength)
      const imageSourceApi = image.createImageSource(buffer);
      let tempPixelMap: image.PixelMap | null = null;
      imageSourceApi.createPixelMap({
        desiredPixelFormat: image.PixelMapFormat.RGBA_8888
      }).then(pixelMap => {
        Log.d(TAG, "called createPixelMap end " + pixelMap.getPixelBytesNumber())
        tempPixelMap = pixelMap
        return pixelMap.getImageInfo()
      }).then(imageInfo => {
        Log.d(TAG,
          `nativeImageHeaderCallback width=${imageInfo.size.width}  height=${imageInfo.size.height} imageGeneratorAddress=${imageGeneratorAddress}`)
        flutter.nativeImageDecodeCallback(imageInfo.size.width, imageInfo.size.height, imageGeneratorAddress,
          tempPixelMap)
      }).catch((error: Any) => {
        Log.d(TAG, "decodeImage error=" + JSON.stringify(error))
        flutter.nativeImageDecodeCallback(0, 0, imageGeneratorAddress, null);
      })
    } else {
      flutter.nativeImageDecodeCallback(0, 0, imageGeneratorAddress, null);
    }
  }

  setSemanticsEnabled(enabled: boolean, responseId: number): void {
    if (this.isAttached()) {
      this.nativeSetSemanticsEnabled(enabled);
    } else {
      Log.w(
        TAG,
        "Tried to send a platform message response, but FlutterNapi was detached from native C++. Could not send. Response ID: "
          + responseId);
    }
  }

  // Send an empty response to a platform message received from Dart.
  nativeSetSemanticsEnabled(enabled: boolean): void {
  }

  setAccessibilityFeatures(accessibilityFeatureFlags: number, responseId: number): void {
    if (this.isAttached()) {
      this.nativeSetAccessibilityFeatures(accessibilityFeatureFlags, responseId);
    } else {
      Log.w(
        TAG,
        "Tried to send a platform message response, but FlutterNapi was detached from native C++. Could not send. Response ID: "
          + responseId);
    }
  }

  nativeSetAccessibilityFeatures(accessibilityFeatureFlags: number, responseId: number): void {
  }

  dispatchSemanticsAction(virtualViewId: number, action: Action, responseId: number): void {
    if (this.isAttached()) {
      this.nativeDispatchSemanticsAction(virtualViewId, action, responseId);
    } else {
      Log.w(
        TAG,
        "Tried to send a platform message response, but FlutterNapi was detached from native C++. Could not send. Response ID: "
          + responseId);
    }
  }

  nativeDispatchSemanticsAction(virtualViewId: number, action: Action, responseId: number): void {
  }

  setAccessibilityDelegate(delegate: AccessibilityDelegate, responseId: number): void {
    if (this.isAttached()) {
      this.accessibilityDelegate = delegate;
    } else {
      Log.w(
        TAG,
        "Tried to send a platform message response, but FlutterNapi was detached from native C++. Could not send. Response ID: "
          + responseId);
    }
  }

  setLocalizationPlugin(localizationPlugin: LocalizationPlugin | null): void {
    this.localizationPlugin = localizationPlugin;
  }

  /**
   * 获取系统语言列表
   * @param rate
   */
  getSystemLanguages() {
    Log.d(TAG, "called getSystemLanguages ")
    let index: number;
    let systemLanguages = i18n.System.getPreferredLanguageList();
    for (index = 0; index < systemLanguages.length; index++) {
      Log.d(TAG, "systemlanguages " + index + ":" + systemLanguages[index]);
    }
    flutter.nativeGetSystemLanguages(this.nativeShellHolderId!, systemLanguages);
  }

  /**
   * xcomponet绑定flutterEngine
   * @param xcomponentId
   */
  xComponentAttachFlutterEngine(xcomponentId: string) {
    flutter.nativeXComponentAttachFlutterEngine(xcomponentId, this.nativeShellHolderId!);
  }

  /**
   * xcomponet预渲染
   * @param xcomponentId
   */
  xComponentPreDraw(xcomponentId: string, width: number, height: number) {
    flutter.nativeXComponentPreDraw(xcomponentId, this.nativeShellHolderId!, width, height);
  }

  /**
   * xcomponet解除绑定flutterEngine
   * @param xcomponentId
   */
  xComponentDetachFlutterEngine(xcomponentId: string) {
    flutter.nativeXComponentDetachFlutterEngine(xcomponentId, this.nativeShellHolderId!);
  }

  detachFromNativeAndReleaseResources() {
    flutter.nativeDestroy(this.nativeShellHolderId!!);
    this.nativeShellHolderId = null;
    this.isRunningDart = false;
    this.isDisplayingFlutterUi = false;
    this.isPreloadedFlutterUi = false;
    this.readyForHandleMessage = false;
  }

  unregisterTexture(textureId: number): void {
    Log.d(TAG, "called unregisterTexture ");
    flutter.nativeUnregisterTexture(this.nativeShellHolderId!, textureId);
  }

  registerPixelMap(textureId: number, pixelMap: PixelMap): void {
    Log.d(TAG, "called registerPixelMap ");
    flutter.nativeRegisterPixelMap(this.nativeShellHolderId!, textureId, pixelMap);
  }

  registerTexture(textureId: number): number {
    Log.d(TAG, "called registerTexture ");
    return flutter.nativeRegisterTexture(this.nativeShellHolderId!, textureId);
  }

  /** Dispatch Touch Event */
  onTouchEvent(strings: Array<string>): void {
    Log.d(TAG, "called onTouchEvent ");
    TouchEventProcessor.getInstance().postTouchEvent(strings);
  }
}

export interface AccessibilityDelegate {
  updateCustomAccessibilityActions(buffer: ByteBuffer, strings: string[]): void;

  updateSemantics(buffer: ByteBuffer, strings: string[], stringAttributeArgs: ByteBuffer[]): void;
}
