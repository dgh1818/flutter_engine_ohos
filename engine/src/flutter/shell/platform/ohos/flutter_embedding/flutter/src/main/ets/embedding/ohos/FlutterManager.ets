/*
* Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd. All rights reserved.
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE_KHZG file.
*/


import { FlutterView } from '../../view/FlutterView';
import UIAbility from '@ohos.app.ability.UIAbility';
import window from '@ohos.window';
import display from '@ohos.display';
import Log from '../../util/Log';
import HashMap from '@ohos.util.HashMap';
import List from '@ohos.util.List';
import { deviceInfo } from '@kit.BasicServicesKit';
import { common } from '@kit.AbilityKit';
import PiPVisibilityBridge from './PiPVisibilityBridge';
import FlutterEngine from '../engine/FlutterEngine';
import FlutterNapi from '../engine/FlutterNapi';
import {
  WindowHostContext,
  FlutterWindowHost,
  WindowHostRegistry,
  WindowArchetype,
  archetypeFromInt,
  SubWindowHost,
  DialogWindowHost,
  TooltipWindowHost,
  PopupWindowHost,
  AnchoredWindowHost,
  AbilityWindowHost,
  AbilityHostMode,
} from './windowing/Windowing';

const TAG = "FlutterManager"

/** Duck-typed view of FlutterAbility's public hostedViewId field — lets the
 * manager stamp it in registerViewAbility without importing FlutterAbility
 * (which imports this module; a cycle would break HAR compilation).
 * Optional member keeps the cast below comparable under strict ArkTS (which
 * forbids unknown and non-overlapping casts). */
interface HostedViewIdHolder {
  hostedViewId?: number;
}

/**
 * Singleton manager for Flutter views and UI abilities.
 * This class manages the lifecycle of FlutterView instances and their association with UI abilities.
 *
 * Multi-window: per-window host behavior lives in the window/ host classes;
 * this manager keeps the shared bookkeeping and implements WindowHostContext.
 */
export default class FlutterManager implements WindowHostContext {
  private static instance: FlutterManager;

  /**
   * Gets the singleton instance of FlutterManager.
   * @returns The singleton FlutterManager instance
   */
  static getInstance(): FlutterManager {
    if (FlutterManager.instance == null) {
      FlutterManager.instance = new FlutterManager();
    }
    return FlutterManager.instance;
  }

  private flutterViewList = new Map<String, FlutterView>();
  private flutterViewIndex = 1;
  private uiAbilityList = new Array<UIAbility>();
  private windowStageList = new Map<UIAbility, window.WindowStage>();
  private mFullScreenListener: FullScreenListener = new DefaultFullScreenListener();
  private windowStageEventCallbacks = new Map<UIAbility, (event: window.WindowStageEventType) => void>();
  private viewToAbility = new Map<string, UIAbility>();
  // viewId (string) -> window host; the registry IS the host-existence record.
  private windowHosts = new WindowHostRegistry();
  private spawnedAbilityName: string = 'RegularWindowAbility';
  private subWindowPageName: string = 'pages/FlutterWindowPage';
  // Per-parent windowRectChange listener (parent moved → re-anchor its
  private parentRectListeners = new Map<UIAbility, Callback<window.RectChangeOptions>>();
  // Same, for SUB-WINDOW parents (dialog → anchored children): keyed by the
  // parent's view id, released in removeHost when that host dies.
  private subWindowRectListeners = new Map<number, Callback<window.RectChangeOptions>>();
  // ~20Hz throttle: windowRectChange streams during a drag; sub-window moves are unanimated.
  private static readonly kParentMoveThrottleMs = 50;
  // Leading-edge timestamp PER parent key (`ability@<serial>` /
  // `subWindow@<viewId>`); a single shared stamp would starve every other
  // parent's moves while one is being dragged.
  private lastMoveForward = new Map<string, number>();
  // Parents with a trailing-edge re-anchor scheduled (the final resting
  // position after a drag burst must not be dropped by the throttle).
  private trailingReanchors = new Set<string>();
  // Stable serial per UIAbility for the string-keyed throttle map above.
  private abilityKeys = new Map<UIAbility, number>();
  private nextAbilityKey = 1;
  // Main (EntryAbility) window's view id — implicit view 0, an "oh_flutter_<n>" id set
  // once in registerViewAbility; adopted (bindEntryAbilityToView), never rebound.
  private mainViewId: string | undefined = undefined;

  private dragEnterCbId: number = 1;
  private dragMoveCbId: number = 1;
  private dragLeaveCbId: number = 1;
  private dropCbId: number = 1;

  private dragEnterCbs: HashMap<number, DragDropCallback> = new HashMap();
  private dragMoveCbs: HashMap<number, DragDropCallback> = new HashMap();
  private dragLeaveCbs: HashMap<number, DragDropCallback> = new HashMap();
  private dropCbs: HashMap<number, DragDropCallback> = new HashMap();

  private getValuesFromMap(map: HashMap<number, DragDropCallback>): List<DragDropCallback> {
    let list: List<DragDropCallback> = new List();
    map.forEach((value, key) => {
      list.add(value);
    });
    return list;
  }

  /**
   * Gets all drag enter callbacks.
   * @returns A list of drag enter callbacks
   */
  getDragEnterCbs(): List<DragDropCallback> {
    return this.getValuesFromMap(this.dragEnterCbs);
  }

  /**
   * Gets all drag move callbacks.
   * @returns A list of drag move callbacks
   */
  getDragMoveCbs(): List<DragDropCallback> {
    return this.getValuesFromMap(this.dragMoveCbs);
  }

  /**
   * Gets all drag leave callbacks.
   * @returns A list of drag leave callbacks
   */
  getDragLeaveCbs(): List<DragDropCallback> {
    return this.getValuesFromMap(this.dragLeaveCbs);
  }

  /**
   * Gets all drop callbacks.
   * @returns A list of drop callbacks
   */
  getDropCbs(): List<DragDropCallback> {
    return this.getValuesFromMap(this.dropCbs);
  }

  /**
   * Adds a drag enter callback.
   * @param callback - The drag enter callback
   * @returns The callback ID
   */
  addDragEnterCb(callback: DragDropCallback): number {
    this.dragEnterCbs.set(this.dragEnterCbId, callback);
    return this.dragEnterCbId++;
  }

  /**
   * Adds a drag move callback.
   * @param callback - The drag move callback
   * @returns The callback ID
   */
  addDragMoveCb(callback: DragDropCallback): number {
    this.dragMoveCbs.set(this.dragMoveCbId, callback);
    return this.dragMoveCbId++;
  }

  /**
   * Adds a drag leave callback.
   * @param callback - The drag leave callback
   * @returns The callback ID
   */
  addDragLeaveCb(callback: DragDropCallback): number {
    this.dragLeaveCbs.set(this.dragLeaveCbId, callback);
    return this.dragLeaveCbId++;
  }

  /**
   * Adds a drop callback.
   * @param callback - The drop callback
   * @returns The callback ID
   */
  addDropCb(callback: DragDropCallback): number {
    this.dropCbs.set(this.dropCbId, callback);
    return this.dropCbId++;
  }

  /**
   * Removes a drag enter callback.
   * @param id - The callback ID to remove
   */
  removeDragEnterCb(id: number) {
    this.dragEnterCbs.remove(id);
  }

  /**
   * Removes a drag move callback.
   * @param id - The callback ID to remove
   */
  removeDragMoveCb(id: number) {
    this.dragMoveCbs.remove(id);
  }

  /**
   * Removes a drag leave callback.
   * @param id - The callback ID to remove
   */
  removeDragLeaveCb(id: number) {
    this.dragLeaveCbs.remove(id);
  }

  /**
   * Removes a drop callback.
   * @param id - The callback ID to remove
   */
  removeDropCb(id: number) {
    this.dropCbs.remove(id);
  }

  /**
   * Pushes a UIAbility to the list.
   * @param uiAbility - The UIAbility to add
   */
  pushUIAbility(uiAbility: UIAbility) {
    this.uiAbilityList.push(uiAbility);
  }

  /**
   * Removes a UIAbility from the list.
   * @param uiAbility - The UIAbility to remove
   */
  popUIAbility(uiAbility: UIAbility) {
    let index = this.uiAbilityList.findIndex((item: UIAbility) => item == uiAbility)
    if (index >= 0) {
      this.uiAbilityList.splice(index, 1)
    }
  }

  /**
   * Associates a WindowStage with a UIAbility.
   * @param uiAbility - The UIAbility
   * @param windowStage - The WindowStage to associate
   */
  pushWindowStage(uiAbility: UIAbility, windowStage: window.WindowStage) {
    this.windowStageList.set(uiAbility, windowStage)
    this.startPipWindowStageListener(uiAbility, windowStage);
  }

  /**
   * Removes the WindowStage association for a UIAbility.
   * @param uiAbility - The UIAbility
   */
  popWindowStage(uiAbility: UIAbility) {
    this.stopPipWindowStageListener(uiAbility);
    // Ability death: release its main-window rect listener too — the
    // singleton-held closure would otherwise pin the destroyed ability.
    this.releaseParentRectListener(uiAbility);
    this.windowStageList.delete(uiAbility)
  }

  private startPipWindowStageListener(uiAbility: UIAbility, windowStage: window.WindowStage): void {
    if (this.windowStageEventCallbacks.has(uiAbility)) {
      return;
    }
    const callback = (event: window.WindowStageEventType): void => {
      if (event === window.WindowStageEventType.HIDDEN) {
        Log.d(TAG, 'PiP: WindowStageEvent HIDDEN, start pip polling');
        PiPVisibilityBridge.getInstance().updateAbilityShown(false);
      } else if (event === window.WindowStageEventType.SHOWN) {
        Log.d(TAG, 'PiP: WindowStageEvent SHOWN, stop pip polling');
        PiPVisibilityBridge.getInstance().updateAbilityShown(true);
      }
    };
    windowStage.on('windowStageEvent', callback);
    this.windowStageEventCallbacks.set(uiAbility, callback);
    Log.d(TAG, 'PiP: WindowStageEvent listener registered for ability');
  }

  private stopPipWindowStageListener(uiAbility: UIAbility): void {
    const callback = this.windowStageEventCallbacks.get(uiAbility);
    if (callback == null) {
      return;
    }
    try {
      const windowStage = this.windowStageList.get(uiAbility);
      windowStage?.off('windowStageEvent', callback);
    } catch (err) {
      Log.w(TAG, 'PiP: windowStageEvent off failed, window may already be destroyed: ' + JSON.stringify(err));
    } finally {
      this.windowStageEventCallbacks.delete(uiAbility);
    }
    Log.d(TAG, 'PiP: WindowStageEvent listener removed for ability');
  }

  /**
   * Gets the WindowStage for a UIAbility.
   * @param uiAbility - The UIAbility
   * @returns The associated WindowStage
   */
  getWindowStage(uiAbility: UIAbility): window.WindowStage {
    return this.windowStageList.get(uiAbility)!!
  }

  /**
   * Gets a UIAbility by context, or returns the first one if no context is provided.
   * @param context - Optional context to search for
   * @returns The UIAbility instance
   */
  getUIAbility(context?: Context): UIAbility {
    if (!context && this.uiAbilityList.length > 0) {
      return this.uiAbilityList[0];
    }
    return this.uiAbilityList.find((item: UIAbility) => item.context == context)!!
  }

  /**
   * Checks if a FlutterView exists with the given ID.
   * @param viewId - The view ID to check
   * @returns True if the view exists, false otherwise
   */
  hasFlutterView(viewId: string): boolean {
    return this.flutterViewList.has(viewId);
  }

  /**
   * Gets a FlutterView by ID.
   * @param viewId - The view ID
   * @returns The FlutterView instance, or null if not found
   */
  getFlutterView(viewId: string): FlutterView | null {
    return this.flutterViewList.get(viewId) ?? null;
  }

  /**
   * Gets all FlutterView instances.
   * @returns A map of all FlutterView instances by ID
   */
  getFlutterViewList(): Map<String, FlutterView> {
    return this.flutterViewList;
  }

  /**
   * Stores or removes a FlutterView in the list.
   * @param viewId - The view ID
   * @param flutterView - The FlutterView instance, or undefined to remove
   */
  private putFlutterView(viewId: string, flutterView?: FlutterView): void {
    if (flutterView != null) {
      this.flutterViewList.set(viewId, flutterView);
    } else {
      this.flutterViewList.delete(viewId);
    }
  }

  /**
   * Creates a new FlutterView instance.
   * It's suggested to keep 'oh_flutter_' as the prefix for xcomponent_id.
   * Otherwise it might affect the performance.
   * @param context - The context for creating the view
   * @returns A new FlutterView instance
   */
  createFlutterView(context: Context): FlutterView {
    let flutterView = new FlutterView(`oh_flutter_${this.flutterViewIndex++}`, context);
    this.putFlutterView(flutterView.getId(), flutterView);
    return flutterView;
  }

  /**
   * Registers a FlutterView by explicit id (multi-window sub-views use the
   * decimal engine view_id).
   */
  registerFlutterView(viewId: string, flutterView: FlutterView): void {
    this.putFlutterView(viewId, flutterView);
  }

  /**
   * Multi-window: records which UIAbility hosts the view `viewId` (so a
   * sub-window can parent on the right WindowStage).
   */
  registerViewAbility(viewId: string, ability: UIAbility): void {
    this.viewToAbility.set(viewId, ability);
    // First "oh_flutter_<n>" registration = the EntryAbility's implicit view 0.
    if (this.mainViewId === undefined && viewId.startsWith('oh_flutter_')) {
      this.mainViewId = viewId;
    }
    // Spawned RegularWindowAbility path (decimal view id): stamp the numeric
    // id onto the ability so its onDestroy OS-close notification block runs.
    // FlutterAbility.onWindowStageCreate's own stamp is unreachable here (the
    // spawned template overrides it without the base view-creation segment),
    // and without the stamp a title-bar close of a cached-engine secondary
    // window never reaches Dart/native (ghost view + leaked host entry).
    // Duck-typed to avoid the FlutterAbility import cycle.
    const numeric = Number(viewId);
    if (!Number.isNaN(numeric) && numeric > 0) {
      const holder = ability as HostedViewIdHolder;
      if (holder != null && typeof holder.hostedViewId === 'number' && holder.hostedViewId === 0) {
        holder.hostedViewId = numeric;
      }
    }
  }

  /**
   * Multi-window (SubWindow path): creates the per-archetype window host for
   * an auxiliary, parent-bound view and attaches it.
   */
  createSubWindow(viewId: number,
                  parentViewId: number,
                  width: number,
                  height: number,
                  archetype: number): void {
    // Dialog here = modal BY DEFINITION. NEVER gate on parentViewId — the main window
    // IS view 0 (modal-to-main has parentViewId===0); has_parent (C++) is authoritative.
    const type = archetypeFromInt(archetype);
    let host: SubWindowHost;
    switch (type) {
      case WindowArchetype.DIALOG:
        host = new DialogWindowHost(this, viewId, parentViewId, width, height);
        break;
      case WindowArchetype.TOOLTIP:
        host = new TooltipWindowHost(this, viewId, parentViewId, width, height);
        break;
      case WindowArchetype.POPUP:
        host = new PopupWindowHost(this, viewId, parentViewId, width, height);
        break;
      default:
        // Regular (never routed here in practice) / unknown: generic host.
        host = new SubWindowHost(this, viewId, parentViewId, type, width, height);
        break;
    }
    this.windowHosts.register(host);
    host.attach();
  }

  // ---- WindowHostContext: service surface for the window/ host classes ----

  resolveParentAbility(parentViewId: number): UIAbility | undefined {
    if (parentViewId === 0) {
      // Implicit (main) view: locate it by its "oh_flutter_" id prefix.
      const mainId = this.findMainFlutterViewId();
      return mainId ? this.viewToAbility.get(mainId) : undefined;
    }
    return this.viewToAbility.get(String(parentViewId));
  }

  windowStageOf(ability: UIAbility): window.WindowStage | undefined {
    return this.windowStageList.get(ability);
  }

  mainFlutterViewId(): string | undefined {
    return this.findMainFlutterViewId();
  }

  sharedEngine(): FlutterEngine | null {
    const mainId = this.findMainFlutterViewId();
    const mainFv = mainId ? this.getFlutterView(mainId) : null;
    return mainFv ? mainFv.getFlutterEngine() : null;
  }

  setViewAbility(viewId: string, ability: UIAbility): void {
    this.viewToAbility.set(viewId, ability);
  }

  getViewAbility(viewId: string): UIAbility | undefined {
    return this.viewToAbility.get(viewId);
  }

  deleteViewAbility(viewId: string): void {
    this.viewToAbility.delete(viewId);
  }

  subWindowOfView(viewId: string): window.Window | undefined {
    return this.windowHosts.subWindowOf(viewId);
  }

  /** The window host owning a view, or undefined (view 0 / no host yet). */
  getWindowHostOf(viewId: string): FlutterWindowHost | undefined {
    return this.windowHosts.get(viewId);
  }

  densityPixels(): number {
    try {
      return display.getDefaultDisplaySync().densityPixels || 1.0;
    } catch (_) {
      return 1.0;
    }
  }

  ensureParentRectListener(ability: UIAbility, stage: window.WindowStage): void {
    if (this.parentRectListeners.has(ability)) {
      return;
    }
    try {
      const parentMain = stage.getMainWindowSync();
      const cb = (options: window.RectChangeOptions): void => {
        this.reanchorChildrenOf(ability);
      };
      parentMain.on('windowRectChange', cb);
      this.parentRectListeners.set(ability, cb);
    } catch (e) {
      Log.w(TAG, `windowRectChange on() failed: ` +
        `${JSON.stringify(e)} — anchored children will not follow parent moves`);
    }
  }

  /**
   * Sub-window parent (dialog) moved → re-anchor ITS anchored children.
   * The main-window listener above never fires for a dialog drag; without
   * this, a tooltip anchored inside a dialog cannot follow the dialog.
   */
  ensureSubWindowRectListener(parentViewId: number): void {
    if (this.subWindowRectListeners.has(parentViewId)) {
      return;
    }
    const parentSub = this.windowHosts.subWindowOf(String(parentViewId));
    if (!parentSub) {
      return;
    }
    try {
      const cb = (options: window.RectChangeOptions): void => {
        this.reanchorChildrenOfParentView(parentViewId);
      };
      parentSub.on('windowRectChange', cb);
      this.subWindowRectListeners.set(parentViewId, cb);
    } catch (e) {
      Log.w(TAG, `subWindow windowRectChange on() failed for parent ${parentViewId}: ` +
        `${JSON.stringify(e)} — anchored children will not follow its moves`);
    }
  }

  /** Releases the main-window listener of a dying ability (popWindowStage). */
  private releaseParentRectListener(ability: UIAbility): void {
    const cb = this.parentRectListeners.get(ability);
    if (!cb) {
      return;
    }
    const stage = this.windowStageList.get(ability);
    try {
      const parentMain = stage ? stage.getMainWindowSync() : undefined;
      if (parentMain) {
        parentMain.off('windowRectChange', cb);
      }
    } catch (e) {
      Log.w(TAG, `windowRectChange off() failed: ${JSON.stringify(e)}`);
    }
    const serial = this.abilityKeys.get(ability);
    this.parentRectListeners.delete(ability);
    this.abilityKeys.delete(ability);
    if (serial !== undefined) {
      // A trailing-edge re-anchor keyed to the old serial may still fire
      // once; it iterates anchoredHostsOf(ability), which is empty by then.
      this.lastMoveForward.delete(`ability@${serial}`);
      this.trailingReanchors.delete(`ability@${serial}`);
    }
  }

  /** Releases the sub-window listener of a dying parent host (removeHost). */
  private releaseSubWindowRectListener(parentViewId: number): void {
    const cb = this.subWindowRectListeners.get(parentViewId);
    if (!cb) {
      return;
    }
    const parentSub = this.windowHosts.subWindowOf(String(parentViewId));
    try {
      if (parentSub) {
        parentSub.off('windowRectChange', cb);
      }
    } catch (e) {
      Log.w(TAG, `subWindow windowRectChange off() failed for parent ${parentViewId}: ${JSON.stringify(e)}`);
    }
    this.subWindowRectListeners.delete(parentViewId);
    this.lastMoveForward.delete(`subWindow@${parentViewId}`);
    this.trailingReanchors.delete(`subWindow@${parentViewId}`);
  }

  removeHost(viewIdStr: string): void {
    // Host death: if OTHER views anchored to this one, release its
    // sub-window rect listener before the registry entry goes away.
    const numericViewId = Number(viewIdStr);
    if (!Number.isNaN(numericViewId)) {
      this.releaseSubWindowRectListener(numericViewId);
    }
    this.windowHosts.unregister(viewIdStr);
  }

  /**
   * UIAbility class name spawned for Regular/modeless-Dialog hosts. An app MUST set
   * its own name first: an undeclared name fails startAbility silently (never shown).
   */
  regularWindowAbilityName(): string {
    return this.spawnedAbilityName;
  }

  setRegularWindowAbilityName(name: string): void {
    if (name && name.length > 0) {
      this.spawnedAbilityName = name;
    }
  }

  /**
   * ArkUI page loaded into every SubWindow host — APP code, not in the HAR. An app
   * MUST set its own page first: unresolvable → loadContent fails → window reported closed.
   */
  subWindowPage(): string {
    return this.subWindowPageName;
  }

  setSubWindowPageName(name: string): void {
    if (name && name.length > 0) {
      this.subWindowPageName = name;
    }
  }

  private reanchorChildrenOf(parentAbility: UIAbility): void {
    // Per-ability leading timestamp + trailing edge: a drag streams events,
    // the last pose must land, and one parent's burst must not starve
    // another parent's moves (the old global timestamp did).
    const run = (): void => {
      this.windowHosts.anchoredHostsOf(parentAbility).forEach((host: AnchoredWindowHost) => {
        host.reanchor();
      });
    };
    this.throttledReanchor(`ability@${this.abilityKey(parentAbility)}`, run);
  }

  /** Sub-window parent (dialog) variant: children are matched by
   * parentViewId, not by ability (several sub-windows share one ability). */
  private reanchorChildrenOfParentView(parentViewId: number): void {
    const run = (): void => {
      this.windowHosts.anchoredHostsOfParentView(parentViewId).forEach((host: AnchoredWindowHost) => {
        host.reanchor();
      });
    };
    this.throttledReanchor(`subWindow@${parentViewId}`, run);
  }

  /** Stable serial per UIAbility (objects cannot key the string throttle map). */
  private abilityKey(ability: UIAbility): number {
    let k = this.abilityKeys.get(ability);
    if (k === undefined) {
      k = this.nextAbilityKey++;
      this.abilityKeys.set(ability, k);
    }
    return k;
  }

  /**
   * Leading-edge throttle with a per-key trailing edge: events within
   * [kParentMoveThrottleMs] are dropped EXCEPT one deferred run, so the
   * final resting position after a drag burst is always applied.
   */
  private throttledReanchor(key: string, run: () => void): void {
    const now = Date.now();
    const last = this.lastMoveForward.get(key) ?? 0;
    const elapsed = now - last;
    if (elapsed >= FlutterManager.kParentMoveThrottleMs) {
      this.lastMoveForward.set(key, now);
      run();
      return;
    }
    if (this.trailingReanchors.has(key)) {
      return;
    }
    this.trailingReanchors.add(key);
    setTimeout(() => {
      this.trailingReanchors.delete(key);
      this.lastMoveForward.set(key, Date.now());
      run();
    }, FlutterManager.kParentMoveThrottleMs - elapsed);
  }

  // ---- Runtime window-property mutations ----------------------------------
  // OHOSWindowController::Set* (Dart FFI → napi), delegated to the per-view
  // host (LOGICAL px ×DPR → PHYSICAL px); fire-and-forget, host logs errors.

  setWindowSize(viewId: number, width: number, height: number): void {
    const host = this.windowHosts.get(String(viewId));
    if (!host) {
      Log.e(TAG, `setWindowSize: no window for viewId=${viewId}`);
      return;
    }
    host.setWindowSize(width, height);
  }

  setWindowTitle(viewId: number, title: string): void {
    const host = this.windowHosts.get(String(viewId));
    if (!host) {
      Log.e(TAG, `setWindowTitle: no window for viewId=${viewId}`);
      return;
    }
    host.setWindowTitle(title);
  }

  setWindowMaximized(viewId: number, maximized: boolean): void {
    const host = this.windowHosts.get(String(viewId));
    if (!host) {
      Log.e(TAG, `setWindowMaximized: no window for viewId=${viewId}`);
      return;
    }
    host.setWindowMaximized(maximized);
  }

  setWindowMinimized(viewId: number, minimized: boolean): void {
    const host = this.windowHosts.get(String(viewId));
    if (!host) {
      Log.e(TAG, `setWindowMinimized: no window for viewId=${viewId}`);
      return;
    }
    host.setWindowMinimized(minimized);
  }

  setWindowFullscreen(viewId: number, fullscreen: boolean): void {
    const host = this.windowHosts.get(String(viewId));
    if (!host) {
      Log.e(TAG, `setWindowFullscreen: no window for viewId=${viewId}`);
      return;
    }
    host.setWindowFullscreen(fullscreen);
  }

  // LOGICAL-px runtime constraints (max 0 == unbounded): the host clamps
  // subsequent setWindowSize calls and pushes to the WM when supported.
  setWindowConstraints(viewId: number,
                       minWidth: number,
                       maxWidth: number,
                       minHeight: number,
                       maxHeight: number): void {
    const host = this.windowHosts.get(String(viewId));
    if (!host) {
      Log.e(TAG, `setWindowConstraints: no window for viewId=${viewId}`);
      return;
    }
    host.setWindowConstraints(minWidth, maxWidth, minHeight, maxHeight);
  }

  // Brings the window to front (Dart Window.activate).
  activateWindow(viewId: number): void {
    const host = this.windowHosts.get(String(viewId));
    if (!host) {
      Log.e(TAG, `activateWindow: no window for viewId=${viewId}`);
      return;
    }
    host.activate();
  }

  /**
   * Per-window delete (Dart destroy() → napi destroyWindowHost): host runs the
   * synchronous native detach first; shared engine NOT freed. View 0 is
   * guarded out — `viewId` is numeric here (a Dart-created window id ≥ 1), so
   * the guard is the value, not a string prefix.
   */
  destroyWindowHost(viewId: number): void {
    const viewIdStr = String(viewId);
    if (viewId <= 0) {
      Log.w(TAG, `destroyWindowHost: skip implicit main view ${viewIdStr}`);
      return;
    }
    const host = this.windowHosts.get(viewIdStr);
    if (!host) {
      Log.w(TAG, `destroyWindowHost: no host for ${viewIdStr} (already closed?)`);
      return;
    }
    host.destroy();
  }

  /**
   * WMS does not reliably return focus to the parent after a child SubWindow closes
   * (2-in-1: app drops to the launcher); showWindow() recovers. Best-effort, never throws.
   */
  restoreParentForeground(parentAbility: UIAbility | undefined, closedViewId: string): void {
    if (!parentAbility) {
      return;
    }
    const windowStage = this.windowStageList.get(parentAbility);
    if (!windowStage) {
      Log.w(TAG, `restoreParentForeground: no windowStage for parent (closed ${closedViewId})`);
      return;
    }
    try {
      const mainWindow = windowStage.getMainWindowSync();
      mainWindow.showWindow()
        .catch((e: Error) => {
          Log.w(TAG, `restoreParentForeground: showWindow failed after ${closedViewId}: ${JSON.stringify(e)}`);
        });
    } catch (e) {
      Log.w(TAG, `restoreParentForeground: getMainWindowSync/showWindow threw after ${closedViewId}: ${JSON.stringify(e)}`);
    }
  }

  /**
   * Focus push for sub-window hosts (WindowHostContext): windowEvent
   * WINDOW_ACTIVE↔WINDOW_INACTIVE translated into the view's activation flag
   * and handed to native. Native caches it for the Dart `isActivated` query
   * and fires the view's notify_listeners on CHANGE so Dart dependents
   * rebuild. Native no-ops for untracked views — safe at any lifecycle point.
   */
  notifyWindowActivated(viewId: number, activated: boolean): void {
    try {
      FlutterNapi.nativeNotifyWindowActivated(viewId, activated);
    } catch (e) {
      Log.w(TAG, `notifyWindowActivated(${viewId}, ${activated}) napi failed: ${JSON.stringify(e)}`);
    }
  }

  /**
   * Close child windows BEFORE the parent frees the shared engine (survivors
   * dangle); closeForCascade's synchronous detach is the guarantee; idempotent.
   * @param parentAbility - The host that owns and is about to destroy the engine.
   */
  cascadeCloseChildren(parentAbility: UIAbility): void {
    const mainId = this.findMainFlutterViewId();
    // Only the engine-owning (main-host) ability cascades; unknown ownership
    // still cascades (so the crash-safety detach runs).
    const mainAbility = mainId ? this.viewToAbility.get(mainId) : undefined;
    if (mainAbility !== undefined && mainAbility !== parentAbility) {
      Log.i(TAG, `cascadeCloseChildren: ${parentAbility.constructor.name}` +
        ` does not own the shared engine; leaving its windows open`);
      return;
    }
    // Snapshot the hosts first — closeForCascade unregisters them below.
    // The ADOPTED main host is exempted EXPLICITLY: its window IS the dying
    // ability's own window (FlutterAbility.onDestroy already tears it down),
    // and an id-format check ('oh_flutter_…' vs its decimal '0') would never
    // match it.
    const hosts = this.windowHosts.snapshot().filter((host: FlutterWindowHost) => {
      if (host instanceof AbilityWindowHost && host.isAdopted()) {
        return false;
      }
      return host.viewIdStr !== mainId;
    });
    if (hosts.length === 0) {
      return;
    }

    for (const host of hosts) {
      // closeForCascade: synchronous native detach first (crash-safety),
      // then best-effort async OS close.
      host.closeForCascade(parentAbility);
    }
  }

  /**
   * UIAbility path: spawns a SEPARATE RegularWindowAbility instance (own task-center
   * card, same process, cached engine); it binds its view from these want.parameters.
   *
   * @param viewId    - The engine numeric view_id to host in this window
   * @param requestId - AbilityStage onAcceptWant instance key → one ability per window
   * @param width     - Preferred window width (0 = use default)
   * @param height    - Preferred window height (0 = use default)
   * @param title     - Window title (empty = no title set)
   */
  createRegularAbilityWindow(
    viewId: number,
    requestId: number,
    width: number,
    height: number,
    title: string,
  ): void {
    const host = new AbilityWindowHost(this, viewId, AbilityHostMode.SPAWNED,
      requestId, width, height, title);
    this.windowHosts.register(host);
    host.attach();
  }

  /**
   * ADOPT path, view 0 only (C++ CreateWindow routes it here): adopt the
   * EntryAbility's main window as the FIRST Dart Regular window's surface.
   *
   * @param viewId - The engine numeric view id (always 0 here)
   * @param width  - Preferred window width in LOGICAL px (0 = leave as-is)
   * @param height - Preferred window height in LOGICAL px (0 = leave as-is)
   * @param title  - Window title (empty = no title set)
   */
  bindEntryAbilityToView(viewId: number,
                         width: number,
                         height: number,
                         title: string): void {
    const host = new AbilityWindowHost(this, viewId, AbilityHostMode.ADOPTED,
      0, width, height, title);
    this.windowHosts.register(host);
    host.attach();
  }

  /**
   * The main (EntryAbility) window's view id (implicit view 0): mainViewId,
   * with a defensive prefix-scan fallback.
   */
  private findMainFlutterViewId(): string | undefined {
    if (this.mainViewId !== undefined) {
      return this.mainViewId;
    }
    for (const id of this.viewToAbility.keys()) {
      if (id.startsWith('oh_flutter_')) {
        return id;
      }
    }
    return undefined;
  }

  /**
   * Main-window close = quit: terminateSelf() runs the full shutdown
   * (cascadeCloseChildren → engine.destroy). NOT exit(0) — it bypasses the Ability lifecycle.
   */
  exitApplication(): void {
    const mainId = this.findMainFlutterViewId();
    const mainAbility = mainId ? this.viewToAbility.get(mainId) : undefined;
    if (!mainAbility) {
      Log.e(TAG, `exitApplication: no main (EntryAbility) host found — cannot terminate`);
      return;
    }
    try {
      (mainAbility.context as common.UIAbilityContext)
        .terminateSelf()
        .catch((e: Error) => {
          Log.e(TAG, `exitApplication: terminateSelf failed: ${JSON.stringify(e)}`);
        });
    } catch (e) {
      Log.e(TAG, `exitApplication: terminateSelf threw: ${JSON.stringify(e)}`);
    }
  }

  /**
   * Gets the next FlutterView ID that will be used.
   * @param idOffset - Optional offset to add to the index
   * @returns The next FlutterView ID string
   */
  getNextFlutterViewId(idOffset: number = 0): string {
    return `oh_flutter_${this.flutterViewIndex + idOffset}`;
  }

  /**
   * Clears all FlutterView instances.
   */
  clear(): void {
    this.flutterViewList.clear();
  }

  /**
   * Sets the full screen listener.
   * @param listener - The FullScreenListener instance
   */
  setFullScreenListener(listener: FullScreenListener) {
    this.mFullScreenListener = listener
  }

  /**
   * Gets the full screen listener.
   * @returns The FullScreenListener instance
   */
  getFullScreenListener(): FullScreenListener {
    return this.mFullScreenListener;
  }

  /**
   * Sets whether to use full screen mode.
   * @param use - Whether to use full screen
   * @param context - Optional context
   */
  setUseFullScreen(use: boolean, context?: Context | null | undefined) {
    this.mFullScreenListener.setUseFullScreen(use, context);
  }

  /**
   * Checks if full screen mode is enabled.
   * @returns True if full screen is enabled, false otherwise
   */
  useFullScreen(): boolean {
    return this.mFullScreenListener.useFullScreen();
  }

  /**
   * Sets whether to use full screen mode specifically for free multi-window.
   * @param useFullScreen - Whether to use full screen
   * @param context - Optional context
   */
  setUseFullScreenForFreeWindow(useFullScreen: boolean, context?: Context | null | undefined): void {
    this.mFullScreenListener.setUseFullScreenForFreeWindow(useFullScreen, context);
  }

  /**
   * Checks if the isInFreeWindowMode API is supported on the current device.
   * @param context - Optional context, defaults to first UIAbility
   * @returns True if the isInFreeWindowMode API is available, false otherwise
   */
  isInFreeWindowModeSupported(context?: Context | null | undefined): boolean {
    // API 22+ required for isInFreeWindowMode
    if (deviceInfo.sdkApiVersion < 22) {
      return false;
    }
    const mainWindow = this.getWindowStage(this.getUIAbility(context ?? undefined))?.getMainWindowSync();
    if (!mainWindow) {
      return false;
    }
    try {
      // Avoid compilation errors: isInFreeWindowMode is an API22 interface
      // Check if isInFreeWindowMode API is available
      const result = typeof (mainWindow as ESObject)?.isInFreeWindowMode();
      return result == 'boolean';
    } catch (exception) {
      Log.i(TAG, `Failed to check isInFreeWindowMode support. Cause code: ${exception.code}, message: ${exception.message}`);
      return false;
    }
  }

  /**
   * Checks if the current device is in free free-window mode.
   * @param context - Optional context, defaults to first UIAbility
   * @returns True if the device is in free multi-window mode, false otherwise
   */
  isInFreeMultiWindowMode(context?: Context | null | undefined): boolean {
    if (this.isInFreeWindowModeSupported(context)) {
      const mainWindow = this.getWindowStage(this.getUIAbility(context ?? undefined))?.getMainWindowSync();
      if (!mainWindow) {
        return false;
      }
      try {
        return mainWindow.isInFreeWindowMode();
      } catch (exception) {
        Log.e(TAG, `isInFreeMultiWindowMode failed. Cause code: ${exception.code}, message: ${exception.message}`);
        return false;
      }
    }
    return false;
  }

  /**
   * Deletes a FlutterView from the list.
   * @param viewId - The view ID
   * @param flutterView - Optional FlutterView instance to verify
   */
  deleteFlutterView(viewId: string, flutterView?: FlutterView): void {
    if (flutterView != null) {
      this.flutterViewList.delete(viewId);
    }
  }

  /**
   * Get window ID for notifyPageChanged.
   * @param context The context to get UIAbility
   * @returns The window ID, or 0 if failed
   */
  getWindowId(context: common.Context): number {
    try {
      const uiAbility = this.getUIAbility(context);
      if (uiAbility == null) {
        Log.e(TAG, "getWindowId: uiAbility is null");
        return 0;
      }
      const windowStage = this.getWindowStage(uiAbility);
      if (windowStage == null) {
        Log.e(TAG, "getWindowId: windowStage is null");
        return 0;
      }
      const mainWindow = windowStage.getMainWindowSync();
      if (mainWindow == null) {
        Log.e(TAG, "getWindowId: mainWindow is null");
        return 0;
      }
      // get windowID for notifyPageChanged.
      const windowId = mainWindow.getWindowProperties()?.id ?? 0;
      return windowId;
    } catch (error) {
      Log.e(TAG, "getWindowId failed: " + JSON.stringify(error));
    }
    return 0;
  }

  /**
   * Sets the visibility of a specific system bar and applies safe area padding.
   * When hiding a system bar, automatically applies padding to avoid content being covered.
   * @param flutterViewId - The ID of the FlutterView to apply padding
   * @param specificSystemBar - System bar type: 'status' for status bar, 'navigation' for three-key navigation bar, 'navigationIndicator' for gesture navigation bar
   * @param isVisible - True to show, false to hide
   * @param enableAnimation - Whether to enable animation effect, optional
   * @param context - Optional context, uses the first UIAbility if not provided
   */
  setSpecificSystemBarEnabled(flutterViewId: string, specificSystemBar: 'status' | 'navigation' | 'navigationIndicator', isVisible: boolean, enableAnimation?: boolean, context?: Context): void {
    const flutterView: FlutterView | null = this.getFlutterView(flutterViewId);
    if (!flutterView) {
      Log.e(TAG, `setSpecificSystemBarEnabled failed: flutterView not found for id ${flutterViewId}`);
      return;
    }

    try {
      const windowStage = this.getWindowStage(this.getUIAbility(context));
      if (!windowStage) {
        Log.e(TAG, 'setSpecificSystemBarEnabled failed: windowStage is null');
        return;
      }

      const mainWindow = windowStage.getMainWindowSync();
      if (!mainWindow) {
        Log.e(TAG, 'setSpecificSystemBarEnabled failed: mainWindow is null');
        return;
      }
      mainWindow.setSpecificSystemBarEnabled(specificSystemBar, isVisible, enableAnimation);

      switch (specificSystemBar) {
        case 'status':
          // Apply top padding when hiding status bar to avoid content being covered
          const statusBarHeight = mainWindow.getWindowAvoidArea(window.AvoidAreaType.TYPE_SYSTEM).topRect.height;
          if (statusBarHeight > 0) {
            flutterView.setPaddingTop(statusBarHeight);
          }
          break;
        case 'navigation':
        case 'navigationIndicator':
          // Apply bottom padding when hiding navigation bar to avoid content being covered
          const navHeight = mainWindow.getWindowAvoidArea(window.AvoidAreaType.TYPE_NAVIGATION_INDICATOR).bottomRect.height;
          if (navHeight > 0) {
            flutterView.setPaddingBottom(navHeight);
          }
          break;
      }

      Log.i(TAG, `setSpecificSystemBarEnabled: ${specificSystemBar} = ${isVisible}, animation = ${enableAnimation} success`);
    } catch (error) {
      Log.e(TAG, `setSpecificSystemBarEnabled error: ${JSON.stringify(error)}`);
    }
  }

  /**
   * Adjusts FlutterView top padding to avoid window decoration overlap.
   * @param flutterViewId - The FlutterView ID
   * @param context - Optional context, defaults to first UIAbility
   */
  handleWindowDecorSafeArea(flutterViewId: string, context?: Context | null | undefined): void {
    if (!this.isWindowDecorSafeAreaAvoidSupported(context)) {
      return;
    }
    const flutterView = this.getFlutterView(flutterViewId);
    if (!flutterView) {
      return;
    }
    const mainWindow = this.getWindowStage(this.getUIAbility(context ?? undefined))?.getMainWindowSync();
    if (!mainWindow) {
      return;
    }

    try {
      // Get current window status
      const currentStatus: window.WindowStatusType = mainWindow.getWindowStatus();
      // In MAXIMIZE mode, skip setting padding because the window menu bar is temporarily displayed
      if (currentStatus == window.WindowStatusType.MAXIMIZE) {
        return;
      }
    } catch (e) {
      Log.e(TAG, "Failed to get window status: " + JSON.stringify(e));
      return;
    }

    // Clear padding if window decoration is visible
    if (mainWindow.getWindowDecorVisible() ) {
      flutterView.setPaddingTop(0);
      return;
    }
    // Clear padding if title buttons are hidden
    const titleButtonRect = mainWindow.getTitleButtonRect();
    if (!titleButtonRect || titleButtonRect.height <= 0) {
      flutterView.setPaddingTop(0);
      return;
    }
    // Set padding to match title button height
    flutterView.setPaddingTop(vp2px(titleButtonRect.height));
  }

  /**
   * Checks if window decoration safe area avoidance is supported on the current device.
   * Returns true only when the device is in freeform multi-window mode and supports window decoration.
   * @param context - Optional context, defaults to first UIAbility
   * @returns True if window decoration safe area avoidance is supported, false otherwise
   */
  isWindowDecorSafeAreaAvoidSupported(context?: Context | null | undefined): boolean {
    // API 18+ required for window decoration APIs
    if (deviceInfo.sdkApiVersion < 18) {
      return false;
    }
    const mainWindow = this.getWindowStage(this.getUIAbility(context ?? undefined))?.getMainWindowSync();
    if (!mainWindow) {
      return false;
    }
    try {
      // Avoid compilation errors: getWindowDecorVisible is an API18 interface
      // Check if getWindowDecorVisible API is available
      const result = typeof (mainWindow as ESObject)?.getWindowDecorVisible();
      return result == 'boolean';
    } catch (exception) {
      Log.i(TAG, `Failed to check window decor visibility support. Cause code: ${exception.code}, message: ${exception.message}`);
      return false;
    }
  }
}

/**
 * Interface for drag and drop callbacks.
 */
export interface DragDropCallback {
  /**
   * Handles a drag and drop event.
   * @param event - The drag event
   * @param extraParams - Additional parameters
   */
  do(event: DragEvent, extraParams: string): void;
}

/**
 * Interface for full screen state management.
 */
export interface FullScreenListener {
  /**
   * Checks if full screen mode is enabled.
   * @returns True if full screen is enabled, false otherwise
   */
  useFullScreen(): boolean;

  /**
   * Sets whether to use full screen mode.
   * @param useFullScreen - Whether to use full screen
   * @param context - Optional context
   */
  setUseFullScreen(useFullScreen: boolean, context?: Context | null | undefined): void;

  /**
   * Sets whether to use full screen mode specifically for free multi-window.
   * @param useFullScreen - Whether to use full screen
   * @param context - Optional context
   */
  setUseFullScreenForFreeWindow(useFullScreen: boolean, context?: Context | null | undefined): void;

  /**
   * Called when the screen state changes.
   * @param data - The window status type
   */
  onScreenStateChanged(data: window.WindowStatusType): void;
}

/**
 * Default implementation of FullScreenListener.
 */
export class DefaultFullScreenListener implements FullScreenListener {
  private fullScreen: boolean = true;
  private skipCheck: boolean = false;

  /**
   * Checks if full screen mode is enabled.
   * @returns True if full screen is enabled, false otherwise
   */
  useFullScreen(): boolean {
    return this.fullScreen;
  }

  /**
   * Sets whether to use full screen mode.
   * @param useFullScreen - Whether to use full screen
   * @param context - Optional context
   */
  setUseFullScreen(useFullScreen: boolean, context?: Context | null | undefined): void {
    this.fullScreen = useFullScreen;
    this.skipCheck = true;

    context = context??getContext(this);
    const currentWindow = FlutterManager.getInstance()
      .getWindowStage(FlutterManager.getInstance().getUIAbility(context));

    currentWindow.getMainWindowSync().setWindowLayoutFullScreen(useFullScreen);

    if (deviceInfo.deviceType === '2in1' && deviceInfo.sdkApiVersion >= 14 && useFullScreen) {
      currentWindow.getMainWindowSync().maximize(window.MaximizePresentation.ENTER_IMMERSIVE);
    }

     Log.i(TAG, "WindowLayoutFullScreen is on")
  }

  /**
   * Sets whether to use full screen mode specifically for free multi-window.
   * @param useFullScreen - Whether to use full screen
   * @param context - Optional context
   */
  setUseFullScreenForFreeWindow(useFullScreen: boolean, context?: Context | null | undefined): void {
    this.fullScreen = useFullScreen;
    context = context??getContext(this);
    const currentWindow = FlutterManager.getInstance()
      .getWindowStage(FlutterManager.getInstance().getUIAbility(context));

    currentWindow.getMainWindowSync().setWindowLayoutFullScreen(useFullScreen);

    if (deviceInfo.sdkApiVersion >= 14 && useFullScreen) {
      currentWindow.getMainWindowSync().maximize(window.MaximizePresentation.ENTER_IMMERSIVE);
    }
    Log.i(TAG, 'WindowLayoutFullScreen is on')
  }

  /**
   * Called when the screen state changes.
   * @param data - The window status type
   */
  onScreenStateChanged(data: window.WindowStatusType): void {
    if (this.skipCheck) {
      Log.i(TAG, "onScreenStateChanged: skipCheck is on, WindowStatusType = " + data)
      return;
    }
    switch (data) {
      case window.WindowStatusType.MINIMIZE:
        // In the minimized mode of the APP window, the original state is maintained to avoid the problem of evasive calculation errors caused by the lock screen
        break;
      case window.WindowStatusType.FULL_SCREEN:
      case window.WindowStatusType.SPLIT_SCREEN:
      case window.WindowStatusType.FLOATING:
      case window.WindowStatusType.MAXIMIZE:
        this.fullScreen = true;
        Log.i(TAG, "onScreenStateChanged: fullScreen = true")
        break;
      default:
        this.fullScreen = false;
        Log.i(TAG, "onScreenStateChanged: fullScreen = false")
        break;
    }
  }
}
