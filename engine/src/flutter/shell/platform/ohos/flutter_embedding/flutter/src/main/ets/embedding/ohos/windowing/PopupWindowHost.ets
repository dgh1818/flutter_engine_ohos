/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { WindowArchetype } from './WindowArchetype';
import { WindowHostContext } from './WindowHostContext';
import { AnchoredWindowHost } from './AnchoredWindowHost';

/** Popup host: like a tooltip but STAYS FOCUSABLE (its content can hold
 * interactive items). Per-type behavior beyond this lands here. */
export class PopupWindowHost extends AnchoredWindowHost {
  constructor(ctx: WindowHostContext,
              viewId: number,
              parentViewId: number,
              width: number,
              height: number) {
    super(ctx, viewId, parentViewId, WindowArchetype.POPUP, width, height);
  }
}
