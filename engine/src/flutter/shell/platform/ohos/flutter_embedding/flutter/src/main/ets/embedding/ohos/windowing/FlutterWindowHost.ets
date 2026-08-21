/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import window from '@ohos.window';
import UIAbility from '@ohos.app.ability.UIAbility';
import Log from '../../../util/Log';
import FlutterNapi from '../../engine/FlutterNapi';
import { FlutterView } from '../../../view/FlutterView';
import { WindowArchetype } from './WindowArchetype';
import { WindowHostContext } from './WindowHostContext';

const TAG = 'FlutterWindowHost'

/** WM-side constraint push — newer than the API-12 Window typedef this
 * embedding compiles against; probed at runtime, clamp-only fallback.
 * Extends Window so the cast below type-checks under strict ArkTS (which
 * forbids unknown and non-overlapping casts). */
interface WindowConstraintsApi extends window.Window {
  setWindowConstraints(minWidth: number, maxWidth: number, minHeight: number, maxHeight: number): Promise<void>;
}

/**
 * Base class of the per-archetype window hosts — the ETS counterpart of
 * the C++ OHOSWindow hierarchy; each host owns the OS-window lifecycle of
 * ONE engine view. Hosts reach manager services only through
 * WindowHostContext so the manager can import host classes without an
 * import cycle.
 */
export abstract class FlutterWindowHost {
  protected readonly ctx: WindowHostContext;
  readonly viewId: number;
  readonly viewIdStr: string;
  readonly parentViewId: number;
  readonly archetype: WindowArchetype;
  /** Requested LOGICAL size (0 = unconstrained / sized-to-content). */
  protected readonly requestedWidth: number;
  protected readonly requestedHeight: number;
  /** The UIAbility whose WindowStage parents this window (SubWindow path). */
  protected parentAbility: UIAbility | undefined;

  constructor(ctx: WindowHostContext,
              viewId: number,
              parentViewId: number,
              archetype: WindowArchetype,
              width: number,
              height: number) {
    this.ctx = ctx;
    this.viewId = viewId;
    this.viewIdStr = String(viewId);
    this.parentViewId = parentViewId;
    this.archetype = archetype;
    this.requestedWidth = width;
    this.requestedHeight = height;
  }

  /** Materializes the OS window host (async internally). */
  abstract attach(): void;

  /**
   * Attach failed after C++ CreateWindow already handed the view id to
   * Dart (so the Dart controller believes its window exists). Report via
   * the OS-close channel — native fires the same teardown a real OS close
   * does, so Dart `destroyed` flips and unregister runs — then drop this
   * host's bookkeeping (idempotent; native no-ops if Dart already tore it
   * down).
   */
  protected failAttach(where: string): void {
    try {
      FlutterNapi.nativeHandleOsWindowClosed(this.viewId);
    } catch (e) {
      Log.w(TAG, `${where}: nativeHandleOsWindowClosed failed for ${this.viewIdStr}: ${JSON.stringify(e)}`);
    }
    this.cleanupBookkeeping();
  }

  /** The OHOS Window backing this view, or null before the host
   * materializes or after teardown. */
  abstract resolveWindow(): window.Window | null;

  /** The FlutterView registered for this view id, if any. */
  protected currentFlutterView(): FlutterView | null {
    return this.ctx.getFlutterView(this.viewIdStr);
  }

  /** Synchronous native detach (clears the XComponent engine/surface flags
   * inline); idempotent, guarded natively. */
  detachView(where: string): void {
    const fv = this.currentFlutterView();
    if (fv) {
      try {
        fv.detachFromFlutterEngine();
      } catch (e) {
        Log.w(TAG, `${where}: detach failed for ${this.viewIdStr}: ${JSON.stringify(e)}`);
      }
    }
  }

  /** Dart-initiated teardown (controller.destroy() → napi
   * "destroyWindowHost"): detach, then closeHost(), then bookkeeping. */
  destroy(): void {
    this.detachView('destroyWindowHost');
    this.closeHost();
    this.cleanupBookkeeping();
  }

  /** OS-level teardown (Dart-initiated path): SubWindow destroyWindow +
   * parent-foreground restore; Ability terminateSelf. */
  protected abstract closeHost(): void;

  /** Parent closing → children close with it. Detach FIRST (crash-safety:
   * no live XComponent may reference the soon-to-be-freed holder). */
  closeForCascade(parentAbility: UIAbility): void {
    this.detachView('cascadeCloseChildren');
    this.closeHostForCascade(parentAbility);
    this.cleanupBookkeeping();
  }

  /** OS-level teardown for the cascade path (no parent-foreground restore —
   * the parent itself is dying). */
  protected abstract closeHostForCascade(parentAbility: UIAbility): void;

  /**
   * OS-initiated destruction (sub-window WINDOW_DESTROYED): detach +
   * bookkeeping cleanup. SubWindowHost overrides to notify native first;
   * ability hosts never reach this (their close path is
   * FlutterAbility.onDestroy → native directly).
   */
  handleOsClosed(): void {
    this.detachView('handleOsClosed');
    this.cleanupBookkeeping();
  }

  /** Drops this view's manager/registry bookkeeping (idempotent). */
  protected cleanupBookkeeping(): void {
    this.ctx.deleteViewAbility(this.viewIdStr);
    const fv = this.currentFlutterView();
    if (fv != null) {
      this.ctx.deleteFlutterView(this.viewIdStr, fv);
    }
    this.ctx.removeHost(this.viewIdStr);
  }

  // ---- Runtime window-property mutations ----------------------------------
  // width/height are LOGICAL px (Dart space); @ohos.window Window.resize()
  // takes PHYSICAL px, so scale by DPR. Fire-and-forget; errors logged.

  /** Latest LOGICAL-px size constraints (max 0 == unbounded). Seeded empty;
   * setConstraints (Dart Window.setConstraints) updates them live so later
   * setWindowSize calls clamp against the CURRENT range. */
  private constraintMinW: number = 0;
  private constraintMaxW: number = 0;
  private constraintMinH: number = 0;
  private constraintMaxH: number = 0;

  private clampToConstraints(v: number, min: number, max: number): number {
    let r = v;
    if (min > 0 && r < min) {
      r = min;
    }
    if (max > 0 && r > max) {
      r = max;
    }
    return r;
  }

  setWindowSize(width: number, height: number): void {
    const win = this.resolveWindow();
    if (!win) {
      Log.e(TAG, `setWindowSize: no window for viewId=${this.viewId}`);
      return;
    }
    if (width <= 0 || height <= 0) {
      return;
    }
    width = this.clampToConstraints(width, this.constraintMinW, this.constraintMaxW);
    height = this.clampToConstraints(height, this.constraintMinH, this.constraintMaxH);
    const dpr = this.ctx.densityPixels();
    const physW = Math.round(width * dpr);
    const physH = Math.round(height * dpr);
    win.resize(physW, physH).catch((e: Object) => {
      Log.e(TAG, `setWindowSize failed viewId=${this.viewId}: ${JSON.stringify(e)}`);
    });
  }

  /** Runtime SetConstraints (LOGICAL px; max 0 == unbounded). Stores the
   * range for host-side clamping and pushes it to the WM when the symbol
   * exists (newer than the API-12 typedef this embedding compiles against —
   * probe first, degrade to clamp-only otherwise). */
  setWindowConstraints(minWidth: number,
                       maxWidth: number,
                       minHeight: number,
                       maxHeight: number): void {
    this.constraintMinW = minWidth;
    this.constraintMaxW = maxWidth;
    this.constraintMinH = minHeight;
    this.constraintMaxH = maxHeight;
    const win = this.resolveWindow();
    if (!win) {
      Log.e(TAG, `setWindowConstraints: no window for viewId=${this.viewId}`);
      return;
    }
    const api = win as WindowConstraintsApi;
    if (typeof api.setWindowConstraints !== 'function') {
      Log.i(TAG, `setWindowConstraints: WM API unavailable for viewId=${this.viewId}; host-side clamp only`);
      return;
    }
    api.setWindowConstraints(minWidth, maxWidth, minHeight, maxHeight).catch((e: Object) => {
      Log.w(TAG, `setWindowConstraints failed viewId=${this.viewId}: ${JSON.stringify(e)}`);
    });
  }

  /** Brings this window to the front (Dart Window.activate → napi
   * activateWindow). showWindow re-raises an existing window without
   * recreating it; best-effort. */
  activate(): void {
    const win = this.resolveWindow();
    if (!win) {
      Log.e(TAG, `activate: no window for viewId=${this.viewId}`);
      return;
    }
    win.showWindow().catch((e: Object) => {
      Log.w(TAG, `activate failed viewId=${this.viewId}: ${JSON.stringify(e)}`);
    });
  }

  setWindowTitle(title: string): void {
    const win = this.resolveWindow();
    if (!win) {
      Log.e(TAG, `setWindowTitle: no window for viewId=${this.viewId}`);
      return;
    }
    win.setWindowTitle(title).catch((e: Object) => {
      Log.e(TAG, `setWindowTitle failed viewId=${this.viewId}: ${JSON.stringify(e)}`);
    });
  }

  setWindowMaximized(maximized: boolean): void {
    const win = this.resolveWindow();
    if (!win) {
      Log.e(TAG, `setWindowMaximized: no window for viewId=${this.viewId}`);
      return;
    }
    const op = maximized ? win.maximize() : win.restore();
    op.catch((e: Object) => {
      Log.w(TAG, `setWindowMaximized failed viewId=${this.viewId}: ${JSON.stringify(e)}`);
    });
  }

  setWindowMinimized(minimized: boolean): void {
    const win = this.resolveWindow();
    if (!win) {
      Log.e(TAG, `setWindowMinimized: no window for viewId=${this.viewId}`);
      return;
    }
    const op = minimized ? win.minimize() : win.restore();
    op.catch((e: Object) => {
      Log.w(TAG, `setWindowMinimized failed viewId=${this.viewId}: ${JSON.stringify(e)}`);
    });
  }

  setWindowFullscreen(fullscreen: boolean): void {
    const win = this.resolveWindow();
    if (!win) {
      Log.e(TAG, `setWindowFullscreen: no window for viewId=${this.viewId}`);
      return;
    }
    win.setFullScreen(fullscreen).catch((e: Object) => {
      Log.w(TAG, `setWindowFullscreen failed viewId=${this.viewId}: ${JSON.stringify(e)}`);
    });
  }
}
