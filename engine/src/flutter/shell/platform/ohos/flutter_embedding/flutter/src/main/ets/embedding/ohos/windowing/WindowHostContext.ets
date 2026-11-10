/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import UIAbility from '@ohos.app.ability.UIAbility';
import window from '@ohos.window';
import { FlutterView } from '../../../view/FlutterView';
import { FlutterWindowHost } from './FlutterWindowHost';

/**
 * The slice of FlutterManager the window-host classes need. Hosts depend
 * on this INTERFACE (not the manager class) so the manager can import the
 * host classes without an import cycle. ETS counterpart of the C++
 * OHOSWindowController accessors the OHOSWindow hierarchy uses.
 */
export interface WindowHostContext {
  /** The WindowStage of an ability, or undefined if it has none registered. */
  getWindowStage(ability: UIAbility): window.WindowStage | undefined;

  /** The UIAbility registered for a (numeric) view id, or undefined when the
   * view has no ability of its own (sub-window views, or not SPAWNED yet). */
  getAbilityByView(viewId: number): UIAbility | undefined;

  /** The FlutterView of a (string-id) view, or null before the view's surface
   * was created / after teardown. */
  getFlutterView(viewId: string): FlutterView | null;

  /** Records a FlutterView and feeds it to a materialized host (if any). */
  registerFlutterView(viewId: string, flutterView: FlutterView): void;

  /** Drops a FlutterView record (idempotent); the exact-view overload guards
   * against dropping a re-registered successor. */
  deleteFlutterView(viewId: string, flutterView?: FlutterView): void;

  /** The UIAbility class name SPAWNED regular hosts launch (configurable
   * by the app; the ability must be declared in its module.json5 with
   * launchType "specified"). Same config-surface pattern as subWindowPage. */
  spawnedAbilityName(): string;

  /** The window host of a view (numeric id; 0 = the implicit main view), or
   * undefined when its host has not materialized (or already tore down) —
   * each host carries its own OS-window handle (getWindow). */
  getHostWindowByView(viewId: number): FlutterWindowHost | undefined;

  /** Reports an OS-initiated window destruction to native (Dart `destroyed`
   * flips and unregister runs). Routed through this seam so window-host
   * modules need no runtime import of the napi bridge (import-cycle guard:
   * FlutterNapi → FlutterManager → hosts → FlutterWindowHost). */
  notifyOsWindowClosed(viewId: number): void;

  /** Clears the native sized-to-content reflow flag after a resize lands.
   * Routed through this seam (import-cycle guard: FlutterNapi →
   * FlutterManager → host subclasses → FlutterHostWindowSized). */
  setViewSizedToContent(viewId: number, sizedToContent: boolean): void;

  /** Focus push for a sub-window host: windowEvent WINDOW_ACTIVE↔
   * WINDOW_INACTIVE translated to the view's activation flag and handed to
   * native (cached for the Dart `isActivated` query; a CHANGED flag also
   * fires the view's notify_listeners so Dart dependents rebuild). */
  notifyWindowActivated(viewId: number, activated: boolean): void;

  /** Removes a host from the registry (idempotent). */
  removeHost(viewId: number): void;

  /** Drops the view's ability record so destroyed UIAbilities are not held
   * (idempotent; sub-window views without an ability are a no-op). */
  deleteViewAbility(viewId: number): void;

  /** The ArkUI page loaded into SubWindow hosts. App-configurable because
   * the page is APP code; must satisfy the FlutterManager.subWindowPage
   * contract (LocalStorage 'viewId' key etc.). */
  subWindowPage(): string;
}
