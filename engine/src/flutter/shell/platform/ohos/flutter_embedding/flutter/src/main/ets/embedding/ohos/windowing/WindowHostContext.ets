/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import UIAbility from '@ohos.app.ability.UIAbility';
import window from '@ohos.window';
import { FlutterView } from '../../../view/FlutterView';
import FlutterEngine from '../../engine/FlutterEngine';

/**
 * The slice of FlutterManager the window-host classes need. Hosts depend
 * on this INTERFACE (not the manager class) so the manager can import the
 * host classes without an import cycle. ETS counterpart of the C++
 * OHOSWindowController accessors the OHOSWindow hierarchy uses.
 */
export interface WindowHostContext {
  /** Resolves the UIAbility whose WindowStage owns a parent view's children. */
  resolveParentAbility(parentViewId: number): UIAbility | undefined;

  /** The WindowStage of an ability, or undefined if it has none registered. */
  windowStageOf(ability: UIAbility): window.WindowStage | undefined;

  /** The FlutterView id string of the main (EntryAbility) window = view 0. */
  mainFlutterViewId(): string | undefined;

  /** The shared FlutterEngine of the single-engine, multi-view model. */
  sharedEngine(): FlutterEngine | null;

  getFlutterView(viewId: string): FlutterView | null;

  registerFlutterView(viewId: string, flutterView: FlutterView): void;

  deleteFlutterView(viewId: string, flutterView?: FlutterView): void;

  setViewAbility(viewId: string, ability: UIAbility): void;

  getViewAbility(viewId: string): UIAbility | undefined;

  deleteViewAbility(viewId: string): void;

  /** The sub-window of a view, or undefined when it has no SubWindow host
   * (UIAbility-backed or not materialized yet). */
  subWindowOfView(viewId: string): window.Window | undefined;

  /** The display density (DPR); 1.0 when unavailable. */
  densityPixels(): number;

  /** Registers the per-parent windowRectChange listener (parent moved →
   * re-anchor its anchored sub-windows); idempotent per ability. */
  ensureParentRectListener(ability: UIAbility, stage: window.WindowStage): void;

  /** Same for a SUB-WINDOW parent (dialog moved → re-anchor its anchored
   * children); idempotent per parent view id, no-op while the parent's
   * sub-window has not materialized yet. */
  ensureSubWindowRectListener(parentViewId: number): void;

  /** Re-activates a parent after a child SubWindow closed (WMS does not
   * reliably hand foreground back). Best-effort, logged. */
  restoreParentForeground(ability: UIAbility | undefined, closedViewId: string): void;

  /** Focus push for a sub-window host: windowEvent WINDOW_ACTIVE↔
   * WINDOW_INACTIVE translated to the view's activation flag and handed to
   * native (cached for the Dart `isActivated` query; a CHANGED flag also
   * fires the view's notify_listeners so Dart dependents rebuild). */
  notifyWindowActivated(viewId: number, activated: boolean): void;

  /** Removes a host from the registry (idempotent). */
  removeHost(viewIdStr: string): void;

  /** The UIAbility class name launched for spawned Regular / modeless-Dialog
   * hosts. App-configurable because the ability is APP code — the embedding
   * cannot ship it in the HAR. */
  regularWindowAbilityName(): string;

  /** The ArkUI page loaded into SubWindow hosts. App-configurable because
   * the page is APP code; must satisfy the FlutterManager.subWindowPage
   * contract (LocalStorage 'viewId' key etc.). */
  subWindowPage(): string;
}
