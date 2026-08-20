/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import window from '@ohos.window';
import display from '@ohos.display';
import UIAbility from '@ohos.app.ability.UIAbility';
import Log from '../../../util/Log';
import FlutterNapi, { WindowPositionResult } from '../../engine/FlutterNapi';
import { WindowArchetype } from './WindowArchetype';
import { WindowHostContext } from './WindowHostContext';
import { SubWindowHost } from './SubWindowHost';

const TAG = 'AnchoredWindowHost'

/** setWindowShadowRadius — dynamic @since 17, static @since 23; not on the
 * API-12 Window typedef this embedding compiles against. */
interface WindowShadowRadiusApi {
  setWindowShadowRadius(radius: number): void;
}

/**
 * Positioner-anchored sub-window host (tooltip/popup). The window keeps its
 * launch size — OHOS resize is animated and cannot track per-frame content.
 */
export abstract class AnchoredWindowHost extends SubWindowHost {
  /** True once the window has been placed (follows parent moves). */
  anchored: boolean = false;

  constructor(ctx: WindowHostContext,
              viewId: number,
              parentViewId: number,
              archetype: WindowArchetype,
              width: number,
              height: number) {
    super(ctx, viewId, parentViewId, archetype, width, height);
  }

  /** The parent ability this window is anchored under (for re-anchor fan-out). */
  parentAbilityOf(): UIAbility | undefined {
    return this.parentAbility;
  }

  // ---- Launch styling: transparent / shadowless / square -----------------

  /**
   * Kill the "white flash/edge": OHOS windows default to an opaque
   * '#FFF0F0F0' background that shows through the transparent parts of the
   * Flutter content, so the host must be fully transparent
   * (Regular/Dialog/Satellite keep an opaque host).
   */
  protected async styleAfterResize(subWindow: window.Window): Promise<void> {
    try {
      subWindow.setWindowBackgroundColor('#00000000');
    } catch (e) {
      Log.w(TAG, `subWindow setWindowBackgroundColor failed for ${this.viewIdStr}: ${JSON.stringify(e)}`);
    }
    try {
      const shadowApi = subWindow as WindowShadowRadiusApi;
      if (typeof shadowApi.setWindowShadowRadius === 'function') {
        shadowApi.setWindowShadowRadius(0);
      }
    } catch (e) {
      Log.w(TAG, `subWindow setWindowShadowRadius failed for ${this.viewIdStr}: ${JSON.stringify(e)}`);
    }
    try {
      await subWindow.setWindowCornerRadius(0);
    } catch (e) {
      Log.w(TAG, `subWindow setWindowCornerRadius failed for ${this.viewIdStr}: ${JSON.stringify(e)}`);
    }
  }

  // ---- Initial placement: Dart positioner ---------------------------------

  /** Anchors the window to its triggering widget via the Dart positioner. */
  protected async placeInitial(subWindow: window.Window,
                                        logicalW: number,
                                        logicalH: number,
                                        physW: number,
                                        physH: number): Promise<void> {
    this.anchored = true;
    // If the PARENT is itself a sub-window (tooltip in a dialog), its rect
    // listener must watch the dialog's sub-window, not the ability main
    // window — re-anchoring on dialog drags depends on it.
    this.ctx.ensureSubWindowRectListener(this.parentViewId);
    await this.positionAt(logicalW, logicalH);
  }

  // ---- Anchoring -----------------------------------------------------------

  /**
   * Moves the window to the Dart positioner's LOGICAL position
   * (moveWindowTo takes PHYSICAL px ×dpr). The parent's screen geometry
   * lives only here, hence the nativeComputeWindowPosition round trip.
   * Fully guarded — never throws (both call sites rely on that).
   */
  protected async positionAt(logicalW: number, logicalH: number): Promise<void> {
    const subWindow = this.subWindow;
    if (!subWindow) {
      Log.w(TAG, `positionAt: no subWindow for viewId=${this.viewIdStr}`);
      return;
    }
    const parentAbility = this.parentAbility;
    if (!parentAbility) {
      Log.w(TAG, `positionAt: no parent ability for viewId=${this.viewIdStr}`);
      return;
    }
    const windowStage = this.ctx.windowStageOf(parentAbility);
    if (!windowStage) {
      Log.w(TAG, `positionAt: no windowStage for parent of viewId=${this.viewIdStr}`);
      return;
    }
    try {
      const dpr = this.ctx.densityPixels();
      // Parent geometry source: when the parent view is ITSELF a sub-window
      // (tooltip/popup inside a dialog), the dialog's OWN windowRect is the
      // anchor parent — the hosting ability's main window is a different
      // screen rect entirely (same priority as DialogWindowHost.placeInitial).
      let parentWin = windowStage.getMainWindowSync();
      const parentSub = this.ctx.subWindowOfView(String(this.parentViewId));
      if (parentSub !== undefined) {
        parentWin = parentSub;
      }
      const wr = parentWin.getWindowProperties().windowRect; // physical px
      // windowRect is the DECORATED rect (origin above the caption) while
      // Dart's anchorRect origin is the CONTENT top-left — translating by
      // windowRect.origin alone places every anchor one caption-height too
      // high. Use drawableRect (content area relative to the window's
      // top-left, API 11+); fallback: treat the window as undecorated.
      let drLeft = 0;
      let drTop = 0;
      let drW = wr.width;
      let drH = wr.height;
      try {
        const dr = parentWin.getWindowProperties().drawableRect;
        drLeft = dr.left; drTop = dr.top; drW = dr.width; drH = dr.height;
      } catch (_) { /* keep windowRect as-is */ }
      const disp = display.getDefaultDisplaySync();
      const pos = new WindowPositionResult();
      // Invoke the positioner with the WINDOW's logical size so the host
      // lands exactly where the positioner says the window belongs.
      const ret = FlutterNapi.nativeComputeWindowPosition(
        pos, this.viewId,
        logicalW, logicalH,
        (wr.left + drLeft) / dpr, (wr.top + drTop) / dpr, // content-origin (logical)
        drW / dpr, drH / dpr, // parent content size (logical)
        0, 0, disp.width / dpr, disp.height / dpr); // workArea=display (logical)
      if (ret === 0) {
        const px = Math.round(pos.left * dpr);
        const py = Math.round(pos.top * dpr);
        await subWindow.moveWindowToAsync(px, py);
      }
    } catch (e) {
      Log.w(TAG, `subWindow position compute/move failed ` +
        `for viewId=${this.viewIdStr}: ${JSON.stringify(e)}`);
    }
  }

  /** Parent moved → re-anchor at the current size; rate limiting is
   * per-parent in the manager. */
  reanchor(): void {
    const sub = this.subWindow;
    if (!sub) {
      return;
    }
    // Late parent materialization: the placeInitial attempt may have run
    // before the parent dialog's sub-window existed. Cheap map check.
    this.ctx.ensureSubWindowRectListener(this.parentViewId);
    try {
      const dpr = this.ctx.densityPixels();
      const rect = sub.getWindowProperties().windowRect; // physical
      this.positionAt(rect.width / dpr, rect.height / dpr).catch((e: Object) => {
        Log.w(TAG, `reanchor failed viewId=${this.viewIdStr}: ${JSON.stringify(e)}`);
      });
    } catch (e) {
      Log.w(TAG, `reanchor geometry read failed viewId=${this.viewIdStr}: ${JSON.stringify(e)}`);
    }
  }
}
