/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import window from '@ohos.window';
import display from '@ohos.display';
import Log from '../../../util/Log';

import { FlutterWindowHost } from './FlutterWindowHost';
import { WindowHostContext } from './WindowHostContext';
import { BoxConstraints, NESTED_DIALOG_CASCADE_PX,
  WindowArchetype, WindowConstraints } from './WindowingUtils';

const TAG = 'FlutterHostWindowSized'

export abstract class FlutterHostWindowSized extends FlutterWindowHost {
  protected readonly sizedToContent: boolean;
  protected readonly resizable: boolean;
  protected readonly boxConstraints: BoxConstraints;
  protected physicalWidth: number = -1;
  protected physicalHeight: number = -1;

  protected constructor(ctx: WindowHostContext,
                        viewId: number,
                        parentViewId: number,
                        title: string,
                        archetype: WindowArchetype,
                        sizedToContent: boolean,
                        resizable: boolean,
                        constraints: WindowConstraints) {
    super(ctx, viewId, parentViewId, title, archetype);
    this.sizedToContent = sizedToContent;
    this.resizable = resizable;
    this.boxConstraints = BoxConstraints.fromWindowConstraints(constraints);
  }

  didUpdateViewSize(width: number, height: number): void {
    if (width === this.physicalWidth && height === this.physicalHeight) {
      return;
    }
    this.physicalWidth = width;
    this.physicalHeight = height;
    this.applyContentSize(width, height);
  }

  protected applyContentSize(widthPx: number, heightPx: number): void {
    const win = this.window;
    if (win === undefined) {
      Log.e(TAG, `applyContentSize: no window for viewId=${String(this.viewId)}`);
      return;
    }
    const scale = this.densityPixels(win);
    const windowSize = this.getWindowSizeForClientSize(
      { width: Math.ceil(widthPx / scale), height: Math.ceil(heightPx / scale) },
      this.boxConstraints.smallest(), this.boxConstraints.biggest(), win);
    if (windowSize === undefined) {
      return;
    }
    windowSize.height += this.getDecorHeightPx(win);
    if (this.resizable) {
      win.resize(windowSize.width, windowSize.height).then(() => {
        // Routed through the context seam: a direct FlutterNapi import here
        // would close the cycle FlutterNapi → FlutterManager → hosts → this class.
        this.ctx.setViewSizedToContent(this.viewId, false);
      }).catch((e: Object) => {
        Log.w(TAG, `applyContentSize: resize failed viewId=${String(this.viewId)}: ${JSON.stringify(e)}`);
        this.physicalWidth = -1;
        this.physicalHeight = -1;
      });
    } else {
      const windowLimit: window.WindowLimits = {
        minWidth: windowSize.width, maxWidth: windowSize.width,
        minHeight: windowSize.height, maxHeight: windowSize.height,
      };
      win.setWindowLimits(windowLimit, true).then(() => {
        return win.resize(windowSize.width, windowSize.height);
      }).catch((e: Object) => {
        Log.w(TAG, `applyContentSize: pin/resize failed viewId=${String(this.viewId)}: ${JSON.stringify(e)}`);
        this.physicalWidth = -1;
        this.physicalHeight = -1;
      });
    }
    this.moveWindow(win, windowSize);
  }

  private moveWindow(win: window.Window, windowSize: window.Size): void {
    const parentWnd = this.ctx.getHostWindowByView(this.parentViewId)?.getWindow() ?? undefined;
    if (parentWnd === undefined) {
      return;
    }
    const cascadePx: number = this.sizedToContent ? 0 : NESTED_DIALOG_CASCADE_PX;
    try {
      const frame = parentWnd.getWindowProperties().windowRect;
      win.moveWindowToAsync(
        Math.round(frame.left + (frame.width - windowSize.width) * 0.5) - cascadePx,
        Math.round(frame.top + (frame.height - windowSize.height) * 0.5) - cascadePx)
        .catch((e: Object) => {
          Log.w(TAG, `applyContentSize: move failed viewId=${String(this.viewId)}: ${JSON.stringify(e)}`);
        });
    } catch (e) {
      Log.w(TAG, `applyContentSize: parent rect unavailable for ${String(this.viewId)}: ${JSON.stringify(e)}`);
    }
  }

  protected async getWorkArea(): Promise<window.Rect> {
    const disp = display.getDefaultDisplaySync();
    try {
      const wa = await disp.getAvailableArea();
      if (wa.width > 0 && wa.height > 0) {
        return wa;
      }
    } catch (e) {
      Log.w(TAG, `getWorkArea: getAvailableArea failed for ${String(this.viewId)}: ` +
        `${JSON.stringify(e)}; constraining against the FULL display instead`);
    }
    return { left: 0, top: 0, width: disp.width, height: disp.height };
  }

  protected setWindowStyleForResizable(): void {
    if (this.resizable) {
      return;
    }
    if (this.window === undefined) {
      return;
    }
    this.window.setResizeByDragEnabled(false, (err) => {
      if (err.code) {
        Log.w(TAG, `setResizeByDragEnabled failed: code=${String(err.code)}, message=${err.message}`);
        return;
      }
    });
  }
}
