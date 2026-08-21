/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import window from '@ohos.window';
import UIAbility from '@ohos.app.ability.UIAbility';
import { FlutterWindowHost } from './FlutterWindowHost';
import { SubWindowHost } from './SubWindowHost';
import { AnchoredWindowHost } from './AnchoredWindowHost';

/**
 * viewId → window-host registry, owned by FlutterManager. Membership IS
 * the record that a host exists; the host object carries its own kind.
 */
export class WindowHostRegistry {
  private readonly hosts = new Map<string, FlutterWindowHost>();

  register(host: FlutterWindowHost): void {
    this.hosts.set(host.viewIdStr, host);
  }

  unregister(viewIdStr: string): void {
    this.hosts.delete(viewIdStr);
  }

  get(viewIdStr: string): FlutterWindowHost | undefined {
    return this.hosts.get(viewIdStr);
  }

  /** Undefined when the view has no SubWindow host or it has not
   * materialized yet. */
  subWindowOf(viewIdStr: string): window.Window | undefined {
    const host = this.hosts.get(viewIdStr);
    if (host instanceof SubWindowHost) {
      return host.subWindow;
    }
    return undefined;
  }

  /** All live hosts (snapshot; safe to mutate the registry while iterating). */
  snapshot(): FlutterWindowHost[] {
    const out: FlutterWindowHost[] = [];
    this.hosts.forEach((host) => {
      out.push(host);
    });
    return out;
  }

  /** Anchored sub-windows on `parentAbility` whose re-anchor loop has run
   * at least once — the ones that must follow the parent when it MOVES
   * (the loop itself only repositions on the child's own size change). */
  anchoredHostsOf(parentAbility: UIAbility): AnchoredWindowHost[] {
    const out: AnchoredWindowHost[] = [];
    this.hosts.forEach((host) => {
      if (host instanceof AnchoredWindowHost && host.anchored &&
          host.parentAbilityOf() === parentAbility) {
        out.push(host);
      }
    });
    return out;
  }

  /** Same, keyed by the PARENT VIEW id — the sub-window-parent variant:
   * children of a dialog are matched by parentViewId, not by ability
   * (several sub-windows share one ability). */
  anchoredHostsOfParentView(parentViewId: number): AnchoredWindowHost[] {
    const out: AnchoredWindowHost[] = [];
    this.hosts.forEach((host) => {
      if (host instanceof AnchoredWindowHost && host.anchored &&
          host.parentViewId === parentViewId) {
        out.push(host);
      }
    });
    return out;
  }
}
