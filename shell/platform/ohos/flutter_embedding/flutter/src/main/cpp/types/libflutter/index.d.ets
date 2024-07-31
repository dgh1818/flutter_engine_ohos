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

import common from '@ohos.app.ability.common';
import resourceManager from '@ohos.resourceManager';
import image from '@ohos.multimedia.image';
import FlutterNapi from '../../../ets/embedding/engine/FlutterNapi';

export const getContext: (a: number) => napiContext;

export class napiContext {
  onPageShow(): void;

  onPageHide(): void;
}

/**
 * 设置刷新率
 */
export const nativeUpdateRefreshRate: (
  ate: number
) => void;

/*
 * 初始化SkFontMgr::RefDefault()
 */
export const nativePrefetchDefaultFontManager: () => void;

export const nativeCheckAndReloadFont: (nativeShellHolderId: number) => void;

export const nativeUpdateSize: (
  width: number,
  height: number
) => void;

export const nativeUpdateDensity: (
  densityPixels: number
) => void;

/**
 * 初始化dart vm和flutter engine
 */
export const nativeInit: (
  context: common.Context,
  args: Array<string>,
  bundlePath: string,
  appStoragePath: string,
  engineCachesPath: string,
  initTimeMillis: number,
  productModel: string
) => number | null;

export const nativeAttach: (napi: FlutterNapi) => number;

export const nativeSpawn: (
  nativeSpawningShellId: number | null,
  entrypointFunctionName: string,
  pathToEntrypointFunction: string,
  initialRoute: string,
  entrypointArgs: Array<string>,
  napi: FlutterNapi
) => number;

export const nativeRunBundleAndSnapshotFromLibrary: (
  nativeShellHolderId: number,
  bundlePath: string,
  entrypointFunctionName: string,
  pathToEntrypointFunction: string,
  assetManager: resourceManager.ResourceManager,
  entrypointArgs: Array<string>
) => void;

//Send a data-carrying response to a platform message received from Dart.
export const nativeInvokePlatformMessageResponseCallback: (nativeShellHolderId: number, responseId: number, message: ArrayBuffer, position: number) => void;

// Send an empty response to a platform message received from Dart.
export const nativeInvokePlatformMessageEmptyResponseCallback: (nativeShellHolderId: number, responseId: number) => void;

// Send a data-carrying platform message to Dart.
export const nativeDispatchPlatformMessage: (nativeShellHolderId: number, channel: String, message: ArrayBuffer, position: number, responseId: number) => void;

// Send an empty platform message to Dart.
export const nativeDispatchEmptyPlatformMessage: (nativeShellHolderId: number, channel: String, responseId: number) => void;

export const nativeSetViewportMetrics: (nativeShellHolderId: number, devicePixelRatio: number, physicalWidth: number
                                        , physicalHeight: number, physicalPaddingTop: number, physicalPaddingRight: number
                                        , physicalPaddingBottom: number, physicalPaddingLeft: number, physicalViewInsetTop: number
                                        , physicalViewInsetRight: number, physicalViewInsetBottom: number, physicalViewInsetLeft: number
                                        , systemGestureInsetTop: number, systemGestureInsetRight: number, systemGestureInsetBottom: number
                                        , systemGestureInsetLeft: number, physicalTouchSlop: number, displayFeaturesBounds: Array<number>
                                        , displayFeaturesType: Array<number>, displayFeaturesState: Array<number>) => void;

export const nativeImageDecodeCallback: (width: number, height: number, imageGeneratorPointer: number, pixelMap : image.PixelMap | null) => void;

export const nativeGetSystemLanguages: (nativeShellHolderId: number, languages: Array<string>) => void;

export const nativeXComponentAttachFlutterEngine: (xcomponentId: string, nativeShellHolderId: number) => void;

export const nativeXComponentPreDraw: (xcomponentId: string, nativeShellHolderId: number, width: number, height: number) => void;

export const nativeXComponentDetachFlutterEngine: (xcomponentId: string, nativeShellHolderId: number) => void;

export const nativeXComponentDispatchMouseWheel: (nativeShellHolderId: number,
                                                  xcomponentId: string,
                                                  eventType: string,
                                                  fingerId: number,
                                                  globalX: number,
                                                  globalY: number,
                                                  offsetY: number,
                                                  timestamp: number
                                                  ) => void;

/**
 * Detaches flutterNapi和engine之间的关联
 * 这个方法执行前提是flutterNapi已经和engine关联
 */
export const nativeDestroy: (
  nativeShellHolderId: number
) => void;


export const nativeUnregisterTexture: (nativeShellHolderId: number, textureId: number) => void;

export const nativeRegisterPixelMap: (nativeShellHolderId: number, textureId: number, pixelMap: PixelMap) => void;

export const nativeRegisterTexture: (nativeShellHolderId: number, textureId: number) => number;

export const nativeGetTextureWindowId: (nativeShellHolderId: number, textureId: number) => number;