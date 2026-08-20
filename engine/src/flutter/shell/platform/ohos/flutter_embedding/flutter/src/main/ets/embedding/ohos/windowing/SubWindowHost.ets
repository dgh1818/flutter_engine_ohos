/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import window from '@ohos.window';
import UIAbility from '@ohos.app.ability.UIAbility';
import Log from '../../../util/Log';
import { FlutterView } from '../../../view/FlutterView';
import FlutterNapi from '../../engine/FlutterNapi';
import { WindowArchetype } from './WindowArchetype';
import { WindowHostContext } from './WindowHostContext';
import { FlutterWindowHost } from './FlutterWindowHost';

const TAG = 'SubWindowHost'

/**
 * Sub-window host for a parent-bound auxiliary view (dialog / tooltip /
 * popup); the page's XComponent (decimal viewId) drives the native surface
 * callback routed to PlatformViewOHOSNapi::NotifyCreateForView.
 *
 * Subclasses override the hook methods below, NEVER attach() — the
 * operation order is load-bearing:
 *   createSubWindow → register view + attach to shared engine →
 *   loadContent → resize → styleAfterResize → placeInitial → prepareShow →
 *   showWindow → WINDOW_DESTROYED listener → configureAfterShow
 */
export class SubWindowHost extends FlutterWindowHost {
  subWindow: window.Window | undefined;

  constructor(ctx: WindowHostContext,
              viewId: number,
              parentViewId: number,
              archetype: WindowArchetype,
              width: number,
              height: number) {
    super(ctx, viewId, parentViewId, archetype, width, height);
  }

  attach(): void {
    const parentAbility = this.ctx.resolveParentAbility(this.parentViewId);
    if (!parentAbility) {
      Log.e(TAG, `attach: no parent ability for parentViewId=${this.parentViewId}`);
      this.failAttach('attach: no parent ability');
      return;
    }
    const windowStage = this.ctx.windowStageOf(parentAbility);
    if (!windowStage) {
      Log.e(TAG, `attach: no windowStage for parent ability`);
      this.failAttach('attach: no windowStage');
      return;
    }
    this.parentAbility = parentAbility;
    // ArkTS drops type narrowing in async callbacks; capture as const.
    const parentAbilityResolved: UIAbility = parentAbility;
    // Window name must be unique; use the viewId.
    windowStage.createSubWindow('flutter-' + this.viewIdStr, (err, subWindow) => {
      if (err.code || !subWindow) {
        Log.e(TAG, `createSubWindow failed: ${JSON.stringify(err)}`);
        this.failAttach('createSubWindow');
        return;
      }
      this.subWindow = subWindow;
      // Register BEFORE loadContent so the page's XComponent.onLoad finds
      // it; the id must be the DECIMAL viewId (not "oh_flutter_") for C++.
      let fv = this.ctx.getFlutterView(this.viewIdStr);
      if (!fv) {
        fv = new FlutterView(this.viewIdStr, parentAbilityResolved.context);
        this.ctx.registerFlutterView(this.viewIdStr, fv);
      }
      // Attach to the SAME engine as the parent (single-engine, multi-view):
      // unattached, the XComponent surface callbacks never reach
      // NotifyCreateForView — no ViewportMetrics, nothing renders.
      if (!fv.isAttachedToFlutterEngine()) {
        const mainEngine = this.ctx.sharedEngine();
        if (mainEngine) {
          fv.attachToFlutterEngine(mainEngine);
        } else {
          Log.e(TAG, `attach: main engine not available for sub-view ${this.viewIdStr}`);
          this.failAttach('attach: no shared engine');
          return;
        }
      }
      // A sub-window's viewToAbility entry points at the PARENT ability.
      this.ctx.setViewAbility(this.viewIdStr, parentAbilityResolved);
      // Anchored sub-windows must also follow parent MOVES (the child's own
      // size loop does not reposition on parent movement). One listener per
      // parent ability; it re-anchors only that parent's anchored children.
      this.ctx.ensureParentRectListener(parentAbilityResolved, windowStage);

      const storage = new LocalStorage();
      storage.setOrCreate('viewId', this.viewIdStr);
      storage.setOrCreate('parentViewId', String(this.parentViewId));
      subWindow.loadContent(this.ctx.subWindowPage(), storage, async (errLoad) => {
        if (errLoad && errLoad.code) {
          Log.e(TAG, `subWindow loadContent failed: ${JSON.stringify(errLoad)}`);
          this.failAttach('loadContent');
          return;
        }
        // Sub-windows are born hidden and zero-size: loadContent alone does
        // not lay the page out or surface the XComponent's RS node. The
        // explicit resize + showWindow does — that is what raises the native
        // OnSurfaceCreated (→ NotifyCreateForView) and starts rendering.
        try {
          const dpr = this.ctx.densityPixels();
          // Requested LOGICAL size, ×DPR → the PHYSICAL px resize() expects.
          // NOT the full display — a display-sized host would swallow the
          // whole screen for tooltip/popup/dialog.
          let logicalW = this.requestedWidth;
          let logicalH = this.requestedHeight;
          if (logicalW <= 0 || logicalH <= 0) {
            // Birth-size default so the host is small, not full-display.
            const def = this.defaultLogicalSize();
            logicalW = def[0];
            logicalH = def[1];
          }
          const physW = Math.round(logicalW * dpr);
          const physH = Math.round(logicalH * dpr);
          await subWindow.resize(physW, physH);
          // Per-archetype styling that must land BEFORE the first show.
          await this.styleAfterResize(subWindow);
          // Per-archetype placement; after resize so it anchors the final
          // launch size.
          await this.placeInitial(subWindow, logicalW, logicalH, physW, physH);
          // Per-archetype config; must precede showWindow.
          await this.prepareShow(subWindow);
          await subWindow.showWindow();
          // OS-initiated teardown: fire the same Dart onWindowDestroyed +
          // ETS cleanup as a Dart-initiated destroy(); safe either way
          // (native no-ops when the Dart path already ran; idempotent).
          try {
            subWindow.on('windowEvent', (event: window.WindowEventType) => {
              if (event === window.WindowEventType.WINDOW_DESTROYED) {
                this.handleOsClosed();
              }
            });
          } catch (e) {
            Log.w(TAG, `subWindow on('windowEvent') register failed for ${this.viewIdStr}: ${JSON.stringify(e)}`);
          }
          // Post-show config (dialog: setSubWindowModal + border-drag
          // resize). AFTER showWindow: showWindow excludes already-modal
          // child windows.
          await this.configureAfterShow(subWindow);
        } catch (e) {
          Log.e(TAG, `subWindow resize/show failed for viewId=${this.viewIdStr}: ${JSON.stringify(e)}`);
          this.failAttach('subWindow resize/show');
        }
      });
    });
  }

  /**
   * Late failures (load/resize/show) may leave an OS sub-window behind —
   * destroy it so no hidden, never-rendering host lingers; early failures
   * have no window yet and take the base path only.
   */
  protected failAttach(where: string): void {
    if (this.subWindow) {
      this.detachView(where);
      this.closeHost();
    }
    super.failAttach(where);
  }

  resolveWindow(): window.Window | null {
    return this.subWindow ?? null;
  }

  // ---- Hook methods (subclass extension points) ---------------------------

  /** Default LOGICAL launch size; tooltip overrides. */
  protected defaultLogicalSize(): number[] {
    return [400, 320];
  }

  /** Host styling applied right after the initial resize (base: none). */
  protected async styleAfterResize(subWindow: window.Window): Promise<void> {
  }

  /** Initial placement after resize/styling (base: OS default). */
  protected async placeInitial(subWindow: window.Window,
                               logicalW: number,
                               logicalH: number,
                               physW: number,
                               physH: number): Promise<void> {
  }

  /** Configuration that must precede showWindow (base: none). */
  protected async prepareShow(subWindow: window.Window): Promise<void> {
  }

  /** Configuration applied after showWindow (base: none). */
  protected async configureAfterShow(subWindow: window.Window): Promise<void> {
  }

  // ---- Teardown -----------------------------------------------------------

  handleOsClosed(): void {
    try {
      FlutterNapi.nativeHandleOsWindowClosed(this.viewId);
    } catch (e) {
      Log.w(TAG, `nativeHandleOsWindowClosed (sub) failed for ${this.viewIdStr}: ${JSON.stringify(e)}`);
    }
    super.handleOsClosed();
  }

  protected closeHost(): void {
    const sub = this.subWindow;
    if (!sub) {
      Log.w(TAG, `destroyWindowHost: no sub-window host for ${this.viewIdStr} (already closed?)`);
      return;
    }
    // Capture the PARENT ability NOW: destroy() deletes the viewToAbility
    // entry synchronously, before this async destroyWindow callback runs.
    const parentAbility = this.parentAbility;
    try {
      sub.destroyWindow((err) => {
        if (err && err.code) {
          Log.w(TAG, `destroyWindowHost: destroyWindow failed for ${this.viewIdStr}: ${JSON.stringify(err)}`);
          return;
        }
        // WMS does not reliably restore the parent main window to the
        // foreground after destroying a (modal) child SubWindow; re-show it.
        this.ctx.restoreParentForeground(parentAbility, this.viewIdStr);
      });
    } catch (e) {
      Log.w(TAG, `destroyWindowHost: destroyWindow threw for ${this.viewIdStr}: ${JSON.stringify(e)}`);
    }
  }

  protected closeHostForCascade(parentAbility: UIAbility): void {
    const sub = this.subWindow;
    if (!sub) {
      return;
    }
    // Best-effort async close so the child vanishes with its parent stage.
    try {
      sub.destroyWindow((err) => {
        if (err && err.code) {
          Log.w(TAG, `cascadeCloseChildren: destroyWindow failed for ${this.viewIdStr}: ${JSON.stringify(err)}`);
        }
      });
    } catch (e) {
      Log.w(TAG, `cascadeCloseChildren: destroyWindow threw for ${this.viewIdStr}: ${JSON.stringify(e)}`);
    }
  }
}
