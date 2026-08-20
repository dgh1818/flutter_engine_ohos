/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import window from '@ohos.window';
import Log from '../../../util/Log';
import { WindowArchetype } from './WindowArchetype';
import { WindowHostContext } from './WindowHostContext';
import { SubWindowHost } from './SubWindowHost';

const TAG = 'DialogWindowHost'

/** @ohos.window symbols newer than the API-12 typedef this embedding
 * compiles against (dynamic @since 14, static @since 23). A direct
 * reference breaks SDK-12 builds and crashes on older devices, so use a
 * custom interface + typeof probe. */
interface DialogSubWindowApi {
  setWindowTitleMoveEnabled(enabled: boolean): void;
  setSubWindowModal(isModal: boolean, modalityType: number): Promise<void>;
  enableDrag(enable: boolean): Promise<void>;
}

/** ModalityType.WINDOW_MODALITY (dynamic @since 14, static @since 23): the
 * enum namespace is not on the API-12 typedef, so use its literal value. */
const WINDOW_MODALITY: number = 0;

/**
 * MODAL dialog host (SubWindow path; modeless dialogs resolve to a
 * UIAbility host on the C++ side). Blocks the parent via
 * setSubWindowModal(WINDOW_MODALITY) after showWindow.
 *
 * Do NOT "help" the modality with setWindowTouchable(false) on the parent:
 * on PC multi-window builds a non-touchable window passes clicks through
 * to the window underneath instead of blocking them. setSubWindowModal
 * alone is the modality mechanism; WMS enforces it.
 */
export class DialogWindowHost extends SubWindowHost {
  constructor(ctx: WindowHostContext,
              viewId: number,
              parentViewId: number,
              width: number,
              height: number) {
    super(ctx, viewId, parentViewId, WindowArchetype.DIALOG, width, height);
  }

  /**
   * Center on the parent view's OWN window (not the ability's main window,
   * which would stack nested modals on top of each other); when the parent
   * is itself a sub-window, add a left-up cascade per nesting level. All
   * values are PHYSICAL px — no DPR scaling here.
   */
  protected async placeInitial(subWindow: window.Window,
                                        logicalW: number,
                                        logicalH: number,
                                        physW: number,
                                        physH: number): Promise<void> {
    const parentAbility = this.parentAbility;
    if (!parentAbility) {
      return;
    }
    const windowStage = this.ctx.windowStageOf(parentAbility);
    if (!windowStage) {
      return;
    }
    try {
      let parentRect: window.Rect = windowStage.getMainWindowSync().getWindowProperties().windowRect;
      let cascade = 0;
      const parentSub = this.ctx.subWindowOfView(String(this.parentViewId));
      if (parentSub !== undefined) {
        parentRect = parentSub.getWindowProperties().windowRect;
        cascade = 40; // phys px, left-up per nesting level
      }
      const px = parentRect.left + Math.round((parentRect.width - physW) / 2) - cascade;
      const py = parentRect.top + Math.round((parentRect.height - physH) / 2) - cascade;
      await subWindow.moveWindowToAsync(px, py);
    } catch (e) {
      Log.w(TAG, `subWindow modal center failed for ${this.viewIdStr}: ${JSON.stringify(e)}`);
    }
  }

  /** Enable title-bar drag; probed (dynamic @since 14) — throws logged, not
   * fatal (the page's startMoving drag strip covers builds without one). */
  protected async prepareShow(subWindow: window.Window): Promise<void> {
    const api = subWindow as DialogSubWindowApi;
    if (typeof api.setWindowTitleMoveEnabled !== 'function') {
      return;
    }
    try {
      api.setWindowTitleMoveEnabled(true);
    } catch (e) {
      Log.w(TAG, `subWindow setWindowTitleMoveEnabled failed for ${this.viewIdStr}: ${JSON.stringify(e)}`);
    }
  }

  /** Block the parent: setSubWindowModal(true, WINDOW_MODALITY) (dynamic
   * @since 14), necessarily AFTER showWindow (showWindow excludes
   * already-modal child windows); enableDrag(true) (dynamic @since 14,
   * public @since 20) for border-drag resize. Both probed — on builds
   * without them the dialog simply runs modeless (logged). */
  protected async configureAfterShow(subWindow: window.Window): Promise<void> {
    const api = subWindow as DialogSubWindowApi;
    if (typeof api.setSubWindowModal === 'function') {
      try {
        await api.setSubWindowModal(true, WINDOW_MODALITY);
        Log.i(TAG, `subWindow set modal (WINDOW_MODALITY) for viewId=${this.viewIdStr}`);
      } catch (e) {
        Log.w(TAG, `subWindow setSubWindowModal failed for ${this.viewIdStr}: ${JSON.stringify(e)}`);
      }
    } else {
      Log.w(TAG, `setSubWindowModal unavailable (dynamic API 14+) for ${this.viewIdStr}: dialog is NOT modal`);
    }
    if (typeof api.enableDrag === 'function') {
      try {
        await api.enableDrag(true);
      } catch (e) {
        Log.w(TAG, `subWindow enableDrag failed for ${this.viewIdStr}: ${JSON.stringify(e)}`);
      }
    }
  }
}
