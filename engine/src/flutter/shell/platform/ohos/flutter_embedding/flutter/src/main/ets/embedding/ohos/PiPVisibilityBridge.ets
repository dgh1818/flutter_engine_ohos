/*
* Copyright (c) 2026 Hunan OpenValley Digital Industry Development Co., Ltd. All rights reserved.
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE_KHZG file.
*/

import common from '@ohos.app.ability.common';
import UIAbility from '@ohos.app.ability.UIAbility';
import window from '@ohos.window';
import Log from '../../util/Log';
import FlutterManager from './FlutterManager';

const TAG = "PiPVisibilityBridge";
const POLL_INTERVAL_MS = 120;
const START_GRACE_MS = 2000;

type PiPVisibilityListener = (visible: boolean) => void;

interface AbilityVisibilityState {
  listeners: Set<PiPVisibilityListener>;
  isShown: boolean;
  pipVisible: boolean;
  lastHiddenAt: number;
  pollTimerId?: number;
  refreshInFlight: boolean;
}

export default class PiPVisibilityBridge {
  private static instance: PiPVisibilityBridge;

  static getInstance(): PiPVisibilityBridge {
    if (PiPVisibilityBridge.instance == null) {
      PiPVisibilityBridge.instance = new PiPVisibilityBridge();
    }
    return PiPVisibilityBridge.instance;
  }

  private states = new Map<UIAbility, AbilityVisibilityState>();

  register(context: common.Context | undefined, listener: PiPVisibilityListener): void {
    const state = this.resolveState(context);
    if (state == null) {
      return;
    }

    state.listeners.add(listener);
    listener(state.pipVisible);
  }

  unregister(context: common.Context | undefined, listener: PiPVisibilityListener): void {
    const uiAbility = this.resolveUIAbility(context);
    if (uiAbility == null) {
      return;
    }

    const state = this.states.get(uiAbility);
    if (state == null) {
      return;
    }

    state.listeners.delete(listener);
    if (state.listeners.size == 0) {
      this.stopPolling(state);
      this.states.delete(uiAbility);
    }
  }

  updateAbilityShown(context: common.Context | undefined, shown: boolean): void {
    const state = this.resolveState(context);
    if (state == null) {
      return;
    }

    if (shown) {
      state.isShown = true;
      state.lastHiddenAt = 0;
      this.stopPolling(state);
      this.updateVisibility(state, false);
      return;
    }

    state.isShown = false;
    state.lastHiddenAt = Date.now();
    this.stopPolling(state);
    this.refreshVisibility(state);
  }

  private resolveState(context: common.Context | undefined): AbilityVisibilityState | null {
    const uiAbility = this.resolveUIAbility(context);
    if (uiAbility == null) {
      return null;
    }

    let state = this.states.get(uiAbility);
    if (state != null) {
      return state;
    }

    state = {
      listeners: new Set<PiPVisibilityListener>(),
      isShown: false,
      pipVisible: false,
      lastHiddenAt: 0,
      refreshInFlight: false
    };
    this.states.set(uiAbility, state);
    return state;
  }

  private scheduleNextPoll(state: AbilityVisibilityState): void {
    if (state.pollTimerId != null || state.isShown || state.pipVisible ||
        !this.isWithinStartGraceWindow(state)) {
      return;
    }
    state.pollTimerId = setTimeout((): void => {
      state.pollTimerId = undefined;
      this.refreshVisibility(state);
    }, POLL_INTERVAL_MS) as number;
  }

  private stopPolling(state: AbilityVisibilityState): void {
    if (state.pollTimerId == null) {
      return;
    }
    clearTimeout(state.pollTimerId);
    state.pollTimerId = undefined;
  }

  private refreshVisibility(state: AbilityVisibilityState): void {
    if (state.refreshInFlight) {
      this.scheduleNextPoll(state);
      return;
    }
    state.refreshInFlight = true;
    this.queryBackgroundVisible(state).then((visible: boolean) => {
      // Foreground wins over stale async PiP results that arrive late.
      this.updateVisibility(state, !state.isShown && visible);
    }).catch((_error: Object) => {
    }).finally(() => {
      state.refreshInFlight = false;
      if (this.shouldStopPolling(state)) {
        this.stopPolling(state);
      } else {
        this.scheduleNextPoll(state);
      }
    });
  }

  private async queryBackgroundVisible(state: AbilityVisibilityState): Promise<boolean> {
    if (state.isShown) {
      return false;
    }

    try {
      const mode = await window.getGlobalWindowMode();
      return (mode & window.GlobalWindowMode.PIP) !== 0;
    } catch (error) {
      Log.w(TAG, "getGlobalWindowMode failed: " + JSON.stringify(error));
      return false;
    }
  }

  private isWithinStartGraceWindow(state: AbilityVisibilityState): boolean {
    return state.lastHiddenAt > 0 && (Date.now() - state.lastHiddenAt) <= START_GRACE_MS;
  }

  private shouldStopPolling(state: AbilityVisibilityState): boolean {
    return state.isShown || state.pipVisible || !this.isWithinStartGraceWindow(state);
  }

  private updateVisibility(state: AbilityVisibilityState, visible: boolean): void {
    if (state.pipVisible === visible) {
      return;
    }

    state.pipVisible = visible;
    const listeners = Array.from(state.listeners);
    listeners.forEach((listener: PiPVisibilityListener): void => {
      listener(visible);
    });
  }

  private resolveUIAbility(context: common.Context | undefined): UIAbility | null {
    if (context == null) {
      return null;
    }
    const uiAbilityContext = context as common.UIAbilityContext;
    if (uiAbilityContext.abilityInfo == null) {
      Log.w(TAG, "resolveUIAbility skipped for non-UIAbility context");
      return null;
    }
    try {
      return FlutterManager.getInstance().getUIAbility(context);
    } catch (error) {
      Log.w(TAG, "resolveUIAbility failed: " + JSON.stringify(error));
      return null;
    }
  }
}
