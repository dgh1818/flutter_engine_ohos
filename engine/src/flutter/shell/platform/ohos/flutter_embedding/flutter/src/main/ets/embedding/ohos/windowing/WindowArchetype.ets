/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

/**
 * Window archetype. The numeric values ARE the wire contract with native
 * (OHOSWindowController::WindowType) — NEVER renumber them.
 */
export enum WindowArchetype {
  /** Top-level window with its own task-center card (UIAbility host). */
  REGULAR = 0,
  /** Dialog: modal (has parent) = SubWindow host; modeless = UIAbility host. */
  DIALOG = 1,
  /** Positioner-anchored, non-focusable SubWindow. */
  TOOLTIP = 2,
  /** Positioner-anchored, focusable SubWindow. */
  POPUP = 3,
}

/** Parses the archetype int from native; unknown values fall back to
 * REGULAR (the generic-host bucket in FlutterManager.createSubWindow). */
export function archetypeFromInt(value: number): WindowArchetype {
  switch (value) {
    case WindowArchetype.DIALOG:
      return WindowArchetype.DIALOG;
    case WindowArchetype.TOOLTIP:
      return WindowArchetype.TOOLTIP;
    case WindowArchetype.POPUP:
      return WindowArchetype.POPUP;
    default:
      return WindowArchetype.REGULAR;
  }
}
