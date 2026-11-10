/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import window from '@ohos.window';
import Log from '../../../util/Log';

const TAG = 'WindowingUtils';

/**
 * Shared helpers for the windowing hosts — the extension point for
 * cross-host utilities: add future helpers here instead of new files.
 */

/** LOGICAL-px rect a window is born with — the ETS counterpart of win32's
 * HostWindow::GetInitialRect result (host_window_regular.cc /
 * host_window_dialog.cc). Origin 0,0 == "system decides" (win32
 * CW_USEDEFAULT); width/height 0 == "no size preference" (system default
 * size). */
export class InitialWindowRect {
  /** Logical-px left edge; 0 means "system decides" the origin. */
  left: number = 0;
  /** Logical-px top edge; 0 means "system decides" the origin. */
  top: number = 0;
  /** Logical-px width; 0 means "no size preference". */
  width: number = 0;
  /** Logical-px height; 0 means "no size preference". */
  height: number = 0;

  /**
   * Constructs an InitialWindowRect.
   * @param left - Logical-px left edge
   * @param top - Logical-px top edge
   * @param width - Logical-px width
   * @param height - Logical-px height
   */
  constructor(left: number, top: number, width: number, height: number) {
    this.left = left;
    this.top = top;
    this.width = width;
    this.height = height;
  }
}

/** C++ kNoParentViewId (ohos_window_types.h) — -1 can never be a real view
 * id (ids are the implicit 0 or allocated from 1 up), so it means "no
 * parent"; 0 IS a legitimate parent (the adopted main window). */
export const NO_PARENT_VIEW_ID: number = -1;

/** The implicit main (EntryAbility) view id — the one id that is NOT
 * allocated by the engine (every other id starts at 1). */
export const IMPLICIT_VIEW_ID: number = 0;

export const NESTED_DIALOG_CASCADE_PX: number = 20;

/** C++ WindowSizeRequest. */
export class WindowSizeRequest {
  /** Whether the creator expressed any size preference at all. */
  hasPreferredViewSize: boolean = false;
  /** Preferred content width in logical px (valid when set). */
  preferredViewWidth: number = 0;
  /** Preferred content height in logical px (valid when set). */
  preferredViewHeight: number = 0;

  /** Defensive copy-constructor: defaults every field when raw is undefined. */
  constructor(raw?: WindowSizeRequest) {
    this.hasPreferredViewSize = raw?.hasPreferredViewSize ?? false;
    this.preferredViewWidth = raw?.preferredViewWidth ?? 0;
    this.preferredViewHeight = raw?.preferredViewHeight ?? 0;
  }

  /** Debug rendering: `set(WxH)` or `none`. */
  toString(): string {
    return this.hasPreferredViewSize
      ? `set(${this.preferredViewWidth}x${this.preferredViewHeight})`
      : 'none';
  }
}

/** C++ WindowConstraints. */
export class WindowConstraints {
  /** Whether the creator expressed any constraint at all. */
  hasViewConstraints: boolean = false;
  /** Minimum content width in logical px (0 == unbounded). */
  viewMinWidth: number = 0;
  /** Minimum content height in logical px (0 == unbounded). */
  viewMinHeight: number = 0;
  /** Maximum content width in logical px (0 == unbounded). */
  viewMaxWidth: number = 0;
  /** Maximum content height in logical px (0 == unbounded). */
  viewMaxHeight: number = 0;

  /** Defensive copy-constructor: defaults every field when raw is undefined. */
  constructor(raw?: WindowConstraints) {
    this.hasViewConstraints = raw?.hasViewConstraints ?? false;
    this.viewMinWidth = raw?.viewMinWidth ?? 0;
    this.viewMinHeight = raw?.viewMinHeight ?? 0;
    this.viewMaxWidth = raw?.viewMaxWidth ?? 0;
    this.viewMaxHeight = raw?.viewMaxHeight ?? 0;
  }

  /** Debug rendering: `set[minW,minH..maxW,maxH]` or `none`. */
  toString(): string {
    return this.hasViewConstraints
      ? `set[${this.viewMinWidth},${this.viewMinHeight}..${this.viewMaxWidth},${this.viewMaxHeight}]`
      : 'none';
  }
}

export class BoxConstraints {
  private readonly smallestSize: window.Size;
  private readonly biggestSize: window.Size;

  /** @param smallest - Min size, or undefined for 0x0
   *  @param biggest - Max size, or undefined for unbounded */
  constructor(smallest: window.Size | undefined, biggest: window.Size | undefined) {
    this.smallestSize = smallest !== undefined ? smallest : { width: 0, height: 0 };
    this.biggestSize = biggest !== undefined ? biggest :
      { width: Number.POSITIVE_INFINITY, height: Number.POSITIVE_INFINITY };
  }

  smallest(): window.Size {
    return this.smallestSize;
  }

  biggest(): window.Size {
    return this.biggestSize;
  }

  /** Whether |size| lies within [smallest, biggest]. */
  isSatisfiedBy(size: window.Size): boolean {
    return this.smallest().width <= size.width &&
      size.width <= this.biggest().width &&
      this.smallest().height <= size.height &&
      size.height <= this.biggest().height;
  }

  static fromWindowConstraints(c: WindowConstraints): BoxConstraints {
    let smallest: window.Size | undefined = undefined;
    let biggest: window.Size | undefined = undefined;
    if (c.hasViewConstraints) {
      smallest = { width: c.viewMinWidth, height: c.viewMinHeight };
      if (c.viewMaxWidth > 0 && c.viewMaxHeight > 0) {
        biggest = { width: c.viewMaxWidth, height: c.viewMaxHeight };
      }
    }
    return new BoxConstraints(smallest, biggest);
  }
}

/** C++ RegularWindowCreationRequest. */
export class RegularWindowCreationRequest {
  /** Birth size preference, or none. */
  readonly preferredSize: WindowSizeRequest;
  /** Runtime min/max size constraints, or none. */
  readonly preferredConstraints: WindowConstraints;
  /** Window title (may be empty). */
  readonly title: string;
  readonly sizedToContent: boolean;
  readonly resizable: boolean;

  /** Defensive copy-constructor: defaults every field when raw is undefined. */
  constructor(raw?: RegularWindowCreationRequest) {
    this.preferredSize = new WindowSizeRequest(raw?.preferredSize);
    this.preferredConstraints = new WindowConstraints(raw?.preferredConstraints);
    this.title = raw?.title ?? '';
    this.sizedToContent = raw?.sizedToContent ?? false;
    this.resizable = raw?.resizable ?? true;
  }
}

/** C++ DialogWindowCreationRequest: parentViewId != NO_PARENT_VIEW_ID =
 * modal; sentinel = modeless (UIAbility spawn path). */
export class DialogWindowCreationRequest {
  /** Birth size preference, or none. */
  readonly preferredSize: WindowSizeRequest;
  /** Runtime min/max size constraints, or none. */
  readonly preferredConstraints: WindowConstraints;
  /** Window title (may be empty). */
  readonly title: string;
  readonly sizedToContent: boolean;
  readonly resizable: boolean;
  /** Owning view; NO_PARENT_VIEW_ID routes to the modeless spawn path. */
  readonly parentViewId: number;

  /** Defensive copy-constructor: defaults every field when raw is undefined. */
  constructor(raw?: DialogWindowCreationRequest) {
    this.preferredSize = new WindowSizeRequest(raw?.preferredSize);
    this.preferredConstraints = new WindowConstraints(raw?.preferredConstraints);
    this.title = raw?.title ?? '';
    this.sizedToContent = raw?.sizedToContent ?? false;
    this.resizable = raw?.resizable ?? true;
    this.parentViewId = raw?.parentViewId ?? NO_PARENT_VIEW_ID;
  }
}

/** C++ TooltipWindowCreationRequest (no preferred size — sized to the
 * content constraints; no title). */
export class TooltipWindowCreationRequest {
  /** Min/max constraints that size the content-sized tooltip. */
  readonly preferredConstraints: WindowConstraints;
  /** Owning (anchor) view. */
  readonly parentViewId: number;

  /** Defensive copy-constructor: defaults every field when raw is undefined. */
  constructor(raw?: TooltipWindowCreationRequest) {
    this.preferredConstraints = new WindowConstraints(raw?.preferredConstraints);
    this.parentViewId = raw?.parentViewId ?? NO_PARENT_VIEW_ID;
  }
}

/** C++ PopupWindowCreationRequest (mirrors the tooltip shape; win32 has no
 * popup counterpart). */
export class PopupWindowCreationRequest {
  /** Min/max constraints that size the content-sized popup. */
  readonly preferredConstraints: WindowConstraints;
  /** Owning (positioner) view. */
  readonly parentViewId: number;

  /** Defensive copy-constructor: defaults every field when raw is undefined. */
  constructor(raw?: PopupWindowCreationRequest) {
    this.preferredConstraints = new WindowConstraints(raw?.preferredConstraints);
    this.parentViewId = raw?.parentViewId ?? NO_PARENT_VIEW_ID;
  }
}

export enum WindowArchetype {
  /** Top-level window with its own task-center card (UIAbility host). */
  REGULAR = 0,
  /** Dialog: modal (has parent) = SubWindow host; modeless = UIAbility host. */
  DIALOG = 1,
  /** Positioner-anchored, non-focusable SubWindow. */
  TOOLTIP = 2,
  /** Menu-ish popup: tooltip mechanics with click-grab dismissal semantics. */
  POPUP = 3,
}
/** View-id string → numeric registry key. 'oh_flutter_<n>' ids are the
 * EntryAbility's implicit main view (0); anything else parses as a number
 * (unknown ids yield NaN, which simply never hits the registry). */
export function numericViewId(id: string): number {
  return id.startsWith('oh_flutter_') ? 0 : Number(id);
}
