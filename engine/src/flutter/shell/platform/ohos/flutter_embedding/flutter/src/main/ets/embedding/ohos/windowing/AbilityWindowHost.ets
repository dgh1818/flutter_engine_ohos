/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import window from '@ohos.window';
import UIAbility from '@ohos.app.ability.UIAbility';
import Log from '../../../util/Log';
import { common, Want } from '@kit.AbilityKit';
import { WindowArchetype } from './WindowArchetype';
import { WindowHostContext } from './WindowHostContext';
import { FlutterWindowHost } from './FlutterWindowHost';

const TAG = 'AbilityWindowHost'

export enum AbilityHostMode {
  /** The FIRST top-level window ADOPTS the EntryAbility's pre-created main
   * window + implicit view 0 instead of spawning a sibling UIAbility. */
  ADOPTED,
  /** A separate UIAbility instance (RegularWindowAbility) is launched. */
  SPAWNED,
}

/**
 * UIAbility-backed host (Regular / modeless Dialog): own task-center card.
 * The launched ability registers its FlutterView / viewToAbility entry
 * itself (onWindowStageCreate); this host drives creation and teardown.
 */
export class AbilityWindowHost extends FlutterWindowHost {
  private readonly mode: AbilityHostMode;
  private readonly requestId: number;
  private readonly title: string;

  constructor(ctx: WindowHostContext,
              viewId: number,
              mode: AbilityHostMode,
              requestId: number,
              width: number,
              height: number,
              title: string) {
    super(ctx, viewId, 0, WindowArchetype.REGULAR, width, height);
    this.mode = mode;
    this.requestId = requestId;
    this.title = title;
  }

  attach(): void {
    if (this.mode === AbilityHostMode.ADOPTED) {
      this.attachAdopted();
    } else {
      this.attachSpawned();
    }
  }

  /** True when this host ADOPTED the EntryAbility's main window (implicit
   * view 0) instead of spawning a UIAbility — cascade close must skip it:
   * the dying ability's own teardown already covers that window. */
  isAdopted(): boolean {
    return this.mode === AbilityHostMode.ADOPTED;
  }

  /** ADOPT: view 0's engine + XComponent (`pages/Index`) already exist —
   * only preferred size + title are applied (a new FlutterView / stage
   * reload would retire the adopted surface). */
  private attachAdopted(): void {
    const mainId = this.ctx.mainFlutterViewId();
    const mainAbility = mainId ? this.ctx.getViewAbility(mainId) : undefined;
    if (!mainAbility) {
      Log.e(TAG, `bindEntryAbilityToView: no main ability registered yet` +
        ` (EntryAbility onWindowStageCreate not done?) — cannot adopt`);
      return;
    }
    const windowStage = this.ctx.windowStageOf(mainAbility);
    if (!windowStage) {
      Log.e(TAG, `bindEntryAbilityToView: no windowStage for main ability`);
      return;
    }

    // Preferred size/title for the EntryAbility's main window; LOGICAL px
    // ×DPR → the PHYSICAL px Window.resize() takes.
    const mainWin = windowStage.getMainWindowSync();
    if (this.requestedWidth > 0 && this.requestedHeight > 0) {
      try {
        const dpr = this.ctx.densityPixels();
        mainWin.resize(Math.round(this.requestedWidth * dpr), Math.round(this.requestedHeight * dpr));
      } catch (e) {
        Log.w(TAG, `bindEntryAbilityToView: resize failed: ${JSON.stringify(e)}`);
      }
    }
    if (this.title && this.title.length > 0) {
      try {
        mainWin.setWindowTitle(this.title);
      } catch (e) {
        Log.w(TAG, `bindEntryAbilityToView: setWindowTitle failed: ${JSON.stringify(e)}`);
      }
    }
  }

  /**
   * SPAWN: launch a separate RegularWindowAbility (own task-center card);
   * same process + cached engine keeps single-engine multi-view. The
   * ability parses these want.parameters in onCreate.
   */
  private attachSpawned(): void {
    const mainId = this.ctx.mainFlutterViewId();
    const mainAbility = mainId ? this.ctx.getViewAbility(mainId) : undefined;
    const ctx: common.UIAbilityContext | undefined =
      mainAbility?.context as common.UIAbilityContext;
    if (!ctx) {
      Log.e(TAG, `createRegularAbilityWindow: no launch context for viewId=${this.viewId}` +
        ` (main ability not registered yet?)`);
      this.failAttach('attachSpawned: no launch context');
      return;
    }
    const bundleName = ctx.abilityInfo?.bundleName;
    if (!bundleName) {
      Log.e(TAG, `createRegularAbilityWindow: cannot resolve bundleName`);
      this.failAttach('attachSpawned: no bundleName');
      return;
    }
    // The ability class is APP code (cannot ship in the HAR): default is the
    // sample's name; apps with their own class call
    // FlutterManager.setRegularWindowAbilityName (unconfigured → reject).
    const want: Want = {
      bundleName: bundleName,
      abilityName: this.ctx.regularWindowAbilityName(),
      parameters: {
        'instanceKey': String(this.requestId),
        'flutter_view_id': String(this.viewId),
        'flutter_window_request_id': String(this.requestId),
        'flutter_preferred_width': String(this.requestedWidth),
        'flutter_preferred_height': String(this.requestedHeight),
        'flutter_window_title': this.title,
      } as Record<string, string>,
    };
    ctx.startAbility(want).catch((err: Error) => {
      Log.e(TAG, `createRegularAbilityWindow: startAbility FAILED viewId=${this.viewId}:`
        + ` ${JSON.stringify(err)}`);
      this.failAttach('attachSpawned: startAbility rejected');
    });
  }

  /** View 0 is keyed by its "oh_flutter_<n>" XComponent id, NOT "0" —
   * resolve via the main view id; spawned abilities use the decimal id. */
  resolveWindow(): window.Window | null {
    let ability: UIAbility | undefined;
    if (this.viewId === 0) {
      const mainId = this.ctx.mainFlutterViewId();
      ability = mainId ? this.ctx.getViewAbility(mainId) : undefined;
    } else {
      ability = this.ctx.getViewAbility(this.viewIdStr);
    }
    if (ability) {
      const stage = this.ctx.windowStageOf(ability);
      if (stage) {
        try {
          return stage.getMainWindowSync();
        } catch (e) {
          Log.w(TAG, `resolveWindow: getMainWindowSync failed viewId=${this.viewId}: ${JSON.stringify(e)}`);
        }
      }
    }
    return null;
  }

  /** Terminate the hosting UIAbility (else a blank shell lingers). The
   * adopted view 0 never comes down this path — its lifecycle is the
   * EntryAbility's. */
  protected closeHost(): void {
    this.terminateHostAbility('destroyWindowHost');
  }

  protected closeHostForCascade(parentAbility: UIAbility): void {
    this.terminateHostAbility('cascadeCloseChildren', parentAbility);
  }

  private terminateHostAbility(where: string, notThisAbility?: UIAbility): void {
    const ability = this.ctx.getViewAbility(this.viewIdStr);
    if (ability && ability !== notThisAbility) {
      try {
        (ability.context as common.UIAbilityContext)
          .terminateSelf()
          .catch((e: Error) => {
            Log.w(TAG, `${where}: terminateSelf failed for ${this.viewIdStr}: ${JSON.stringify(e)}`);
          });
      } catch (e) {
        Log.w(TAG, `${where}: terminateSelf threw for ${this.viewIdStr}: ${JSON.stringify(e)}`);
      }
    } else {
      Log.w(TAG, `${where}: no host ability for ${this.viewIdStr} (already closed?)`);
    }
  }
}
