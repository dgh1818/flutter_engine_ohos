/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import window from '@ohos.window';
import Log from '../../../util/Log';
import { WindowArchetype } from './WindowArchetype';
import { WindowHostContext } from './WindowHostContext';
import { AnchoredWindowHost } from './AnchoredWindowHost';

const TAG = 'TooltipWindowHost'

/** Tooltip host: positioner-anchored, NEVER takes focus. */
export class TooltipWindowHost extends AnchoredWindowHost {
  constructor(ctx: WindowHostContext,
              viewId: number,
              parentViewId: number,
              width: number,
              height: number) {
    super(ctx, viewId, parentViewId, WindowArchetype.TOOLTIP, width, height);
  }

  /** Smaller default launch height than popup/dialog hosts. */
  protected defaultLogicalSize(): number[] {
    return [400, 200];
  }

  protected async prepareShow(subWindow: window.Window): Promise<void> {
    try {
      await subWindow.setWindowFocusable(false);
    } catch (e) {
      Log.w(TAG, `subWindow setWindowFocusable failed: ${JSON.stringify(e)}`);
    }
  }
}
