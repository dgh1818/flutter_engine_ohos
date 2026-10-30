/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import Any from '../../../../../main/ets/plugin/common/Any';
import FlutterInjector from '../../../../../main/ets/FlutterInjector';
import FlutterNapi from '../../../../../main/ets/embedding/engine/FlutterNapi';

let savedHasInit: boolean | undefined = undefined;
let savedGetPreloadFlutterNapi: object | undefined = undefined;

/** Marks native init complete so attach/runBundle guards pass in unit tests. */
export function pierceFlutterNapiHasInit(): void {
  if (savedHasInit === undefined) {
    savedHasInit = (FlutterNapi as Any).hasInit as boolean;
  }
  (FlutterNapi as Any).hasInit = true;
}

/** Restores hasInit after pierce tests to avoid polluting later suites. */
export function restoreFlutterNapiHasInit(): void {
  if (savedHasInit !== undefined) {
    (FlutterNapi as Any).hasInit = savedHasInit;
    savedHasInit = undefined;
  }
}

/**
 * Undoes ALL global pollution from the pierce helpers: restores the static
 * FlutterNapi.hasInit flag and the FlutterInjector.getPreloadFlutterNapi
 * singleton method replaced by seedPreloadFlutterNapi.
 *
 * Call this from the afterAll of the suite(s) that actually pierce (i.e. the
 * preload suite), NOT from an earlier suite — registering the restore before
 * the pierce happens makes it a guaranteed no-op and leaks the pollution to
 * every later suite in the process (attachToNative skipping its guard is the
 * native CHECK crash path).
 */
export function restorePierceGlobals(): void {
  if (savedGetPreloadFlutterNapi !== undefined) {
    (FlutterInjector.getInstance() as Any).getPreloadFlutterNapi = savedGetPreloadFlutterNapi;
    savedGetPreloadFlutterNapi = undefined;
  }
  restoreFlutterNapiHasInit();
}

/** Assigns a non-zero shell holder id (0 fails the `!nativeShellHolderId` guard). */
export function pierceNativeShellHolderId(napi: FlutterNapi, shellHolderId: number = 1): void {
  pierceFlutterNapiHasInit();
  napi.nativeShellHolderId = shellHolderId;
}

/**
 * Sets only the shell holder id WITHOUT touching the static hasInit flag.
 * Use for guard-branch tests (id=0 → early return before native); unlike
 * pierceNativeShellHolderId it leaves global state clean for later suites.
 */
export function setShellHolderIdForGuardTest(napi: FlutterNapi, shellHolderId: number): void {
  napi.nativeShellHolderId = shellHolderId;
}

/**
 * Seeds FlutterInjector with a pierced napi for preloadEngine / preLoadFlutterNapi paths.
 * NOTE: assigning the `preloadFlutterNapi` FIELD does not work — getPreloadFlutterNapi()
 * unconditionally overwrites it with `new FlutterNapi()` and preLoadFlutterNapi would
 * attachToNative() that fresh instance (native CHECK crash when nativeInit never ran).
 * Stub the getter method instead so callers always receive the pierced instance.
 */
export function seedPreloadFlutterNapi(shellHolderId: number = 1): FlutterNapi {
  const napi = new FlutterNapi();
  pierceNativeShellHolderId(napi, shellHolderId);
  if (savedGetPreloadFlutterNapi === undefined) {
    savedGetPreloadFlutterNapi = (FlutterInjector.getInstance() as Any).getPreloadFlutterNapi as object;
  }
  (FlutterInjector.getInstance() as Any).getPreloadFlutterNapi = (): FlutterNapi => napi;
  return napi;
}

/** Stubs heavy native ops so preLoadFlutterNapi/preloadEngine can finish in unit tests. */
export function stubPreloadNativeOps(napi: FlutterNapi): void {
  (napi as Any).runBundleAndSnapshotFromLibrary = (_bundlePath: string,
    _entry: string | undefined,
    _path: string | undefined,
    _assetManager: object,
    _args: Array<string>) => {
  };
  (napi as Any).dispatchPlatformMessage = (_channel: String, _message: Object, _position: number,
    _responseId: number) => {
  };
  (napi as Any).dispatchEmptyPlatformMessage = (_channel: String, _responseId: number) => {
  };
  (napi as Any).xComponentPreDraw = (_viewId: string, _width: number, _height: number) => {
  };
  (napi as Any).setPreloading = () => {
  };
  (napi as Any).setViewportMetrics = (_devicePixelRatio: number, _physicalWidth: number,
    _physicalHeight: number, _physicalPaddingTop: number, _physicalPaddingRight: number,
    _physicalPaddingBottom: number, _physicalPaddingLeft: number, _physicalViewInsetTop: number,
    _physicalViewInsetRight: number, _physicalViewInsetBottom: number, _physicalViewInsetLeft: number,
    _systemGestureInsetTop: number, _systemGestureInsetRight: number, _systemGestureInsetBottom: number,
    _systemGestureInsetLeft: number, _physicalTouchSlop: number, _displayFeaturesBounds: Array<number>,
    _displayFeaturesType: Array<number>, _displayFeaturesState: Array<number>,
    _displayCornerRadiusTopLeft: number, _displayCornerRadiusTopRight: number,
    _displayCornerRadiusBottomRight: number, _displayCornerRadiusBottomLeft: number) => {
  };
}

export function invokeSetViewportMetricsWithCornerRadii(napi: FlutterNapi): void {
  napi.setViewportMetrics(
    3.0, 1080, 2340,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    3.0,
    [], [], [],
    12, 34, 56, 78
  );
}
