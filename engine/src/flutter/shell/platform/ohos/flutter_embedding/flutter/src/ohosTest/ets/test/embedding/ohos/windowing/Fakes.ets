/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import window from '@ohos.window';
import UIAbility from '@ohos.app.ability.UIAbility';
import { common, Want } from '@kit.AbilityKit';
import { BusinessError } from '@ohos.base';
import { WindowHostContext } from '../../../../../../main/ets/embedding/ohos/windowing/WindowHostContext';
import { FlutterView } from '../../../../../../main/ets/view/FlutterView';
import FlutterEngine from '../../../../../../main/ets/embedding/engine/FlutterEngine';

/** Waits for pending microtasks/promises (attach chains are async). */
export function sleep(time: number): Promise<void> {
  return new Promise<void>((resolve: Function) => setTimeout(resolve, time));
}

/** Simple rect with the fields windowRect/drawableRect consumers read. */
export class FakeRect {
  left: number;
  top: number;
  width: number;
  height: number;

  constructor(left: number, top: number, width: number, height: number) {
    this.left = left;
    this.top = top;
    this.width = width;
    this.height = height;
  }
}

/** window.WindowProperties fake carrying just windowRect + drawableRect. */
export class FakeWindowProperties {
  windowRect: window.Rect = new FakeRect(0, 0, 1000, 800) as ESObject as window.Rect;
  drawableRect: window.Rect = new FakeRect(0, 0, 1000, 800) as ESObject as window.Rect;

  setWindowRect(left: number, top: number, width: number, height: number): void {
    this.windowRect = new FakeRect(left, top, width, height) as ESObject as window.Rect;
  }
}

/** Full fake of window.Window: every method the hosts can call, with
 * per-call recording and controllable failures. Cast to window.Window at
 * use sites via `fake as ESObject as window.Window`. */
export class FakeWindow {
  viewId: string = '';
  resizeCalls: Array<Array<number>> = new Array<Array<number>>();
  showCount: number = 0;
  moveCalls: Array<Array<number>> = new Array<Array<number>>();
  titles: Array<string> = new Array<string>();
  maximizeCount: number = 0;
  minimizeCount: number = 0;
  restoreCount: number = 0;
  fullscreenArgs: Array<boolean> = new Array<boolean>();
  constraintCalls: Array<Array<number>> = new Array<Array<number>>();
  backgroundColor: string = '';
  cornerRadius: number = -1;
  shadowRadius: number = -1;
  focusable: boolean = true;
  titleMoveEnabled: boolean = false;
  modalCalls: Array<Array<number>> = new Array<Array<number>>();
  dragEnabled: boolean = false;
  windowEventCb: ((event: window.WindowEventType) => void) | undefined = undefined;
  rectChangeCb: ((options: window.RectChangeOptions) => void) | undefined = undefined;
  destroyCount: number = 0;
  loadContentCount: number = 0;
  loadContentPage: string = '';

  props: FakeWindowProperties = new FakeWindowProperties();
  destroyErr: BusinessError | undefined = undefined;
  loadContentErr: BusinessError | undefined = undefined;
  rejectResize: boolean = false;
  rejectShow: boolean = false;
  rejectModal: boolean = false;
  rejectDrag: boolean = false;
  rejectMove: boolean = false;
  rejectTitle: boolean = false;
  rejectMaximize: boolean = false;
  rejectMinimize: boolean = false;
  rejectRestore: boolean = false;
  rejectFullscreen: boolean = false;
  rejectConstraints: boolean = false;
  destroyThrows: boolean = false;
  onThrows: boolean = false;
  bgThrows: boolean = false;
  shadowThrows: boolean = false;
  rejectCornerRadius: boolean = false;
  rejectFocusable: boolean = false;
  titleMoveThrows: boolean = false;

  resize(w: number, h: number): Promise<void> {
    this.resizeCalls.push([w, h]);
    if (this.rejectResize) {
      return Promise.reject(new Error('resize-rejected'));
    }
    return Promise.resolve();
  }

  showWindow(): Promise<void> {
    this.showCount += 1;
    if (this.rejectShow) {
      return Promise.reject(new Error('show-rejected'));
    }
    return Promise.resolve();
  }

  setWindowTitle(title: string): Promise<void> {
    this.titles.push(title);
    if (this.rejectTitle) {
      return Promise.reject(new Error('title-rejected'));
    }
    return Promise.resolve();
  }

  maximize(): Promise<void> {
    this.maximizeCount += 1;
    if (this.rejectMaximize) {
      return Promise.reject(new Error('maximize-rejected'));
    }
    return Promise.resolve();
  }

  minimize(): Promise<void> {
    this.minimizeCount += 1;
    if (this.rejectMinimize) {
      return Promise.reject(new Error('minimize-rejected'));
    }
    return Promise.resolve();
  }

  restore(): Promise<void> {
    this.restoreCount += 1;
    if (this.rejectRestore) {
      return Promise.reject(new Error('restore-rejected'));
    }
    return Promise.resolve();
  }

  setFullScreen(fullscreen: boolean): Promise<void> {
    this.fullscreenArgs.push(fullscreen);
    if (this.rejectFullscreen) {
      return Promise.reject(new Error('fullscreen-rejected'));
    }
    return Promise.resolve();
  }

  setWindowConstraints(minWidth: number, maxWidth: number, minHeight: number,
    maxHeight: number): Promise<void> {
    this.constraintCalls.push([minWidth, maxWidth, minHeight, maxHeight]);
    if (this.rejectConstraints) {
      return Promise.reject(new Error('constraints-rejected'));
    }
    return Promise.resolve();
  }

  setWindowBackgroundColor(color: string): void {
    if (this.bgThrows) {
      throw new Error('bg-throws');
    }
    this.backgroundColor = color;
  }

  setWindowCornerRadius(cornerRadius: number): Promise<void> {
    if (this.rejectCornerRadius) {
      return Promise.reject(new Error('corner-rejected'));
    }
    this.cornerRadius = cornerRadius;
    return Promise.resolve();
  }

  setWindowShadowRadius(radius: number): void {
    if (this.shadowThrows) {
      throw new Error('shadow-throws');
    }
    this.shadowRadius = radius;
  }

  setWindowFocusable(focusable: boolean): Promise<void> {
    if (this.rejectFocusable) {
      return Promise.reject(new Error('focusable-rejected'));
    }
    this.focusable = focusable;
    return Promise.resolve();
  }

  setWindowTitleMoveEnabled(enabled: boolean): void {
    if (this.titleMoveThrows) {
      throw new Error('titlemove-throws');
    }
    this.titleMoveEnabled = enabled;
  }

  setSubWindowModal(isModal: boolean, modalityType: number): Promise<void> {
    this.modalCalls.push([isModal ? 1 : 0, modalityType]);
    if (this.rejectModal) {
      return Promise.reject(new Error('modal-rejected'));
    }
    return Promise.resolve();
  }

  enableDrag(enable: boolean): Promise<void> {
    this.dragEnabled = enable;
    if (this.rejectDrag) {
      return Promise.reject(new Error('drag-rejected'));
    }
    return Promise.resolve();
  }

  getWindowProperties(): window.WindowProperties {
    return this.props as ESObject as window.WindowProperties;
  }

  moveWindowToAsync(x: number, y: number): Promise<void> {
    this.moveCalls.push([x, y]);
    if (this.rejectMove) {
      return Promise.reject(new Error('move-rejected'));
    }
    return Promise.resolve();
  }

  loadContent(path: string, storage: LocalStorage, callback: Function): void {
    this.loadContentCount += 1;
    this.loadContentPage = path;
    const err = this.loadContentErr;
    callback(err);
  }

  on(event: string, callback: Function): void {
    if (this.onThrows) {
      throw new Error('on-throws');
    }
    if (event === 'windowRectChange') {
      this.rectChangeCb = callback as (options: window.RectChangeOptions) => void;
    } else {
      this.windowEventCb = callback as (event: window.WindowEventType) => void;
    }
  }

  off(event: string, callback: Function): void {
    if (event === 'windowRectChange') {
      this.rectChangeCb = undefined;
    } else {
      this.windowEventCb = undefined;
    }
  }

  destroyWindow(callback: Function): void {
    this.destroyCount += 1;
    if (this.destroyThrows) {
      throw new Error('destroy-throws');
    }
    const err = this.destroyErr;
    callback(err);
  }
}

/** styleAfterResize probe-missing variant: no setWindowShadowRadius. */
export class FakeWindowNoShadow {
  background: string = '';
  cornerRadius: number = -1;

  setWindowBackgroundColor(color: string): void {
    this.background = color;
  }

  setWindowCornerRadius(cornerRadius: number): Promise<void> {
    this.cornerRadius = cornerRadius;
    return Promise.resolve();
  }
}

/** Dialog-API probe-missing variant: no setWindowTitleMoveEnabled /
 * setSubWindowModal / enableDrag. */
export class FakeWindowNoDialogApi {
}

/** Dialog-API partial variant: has setSubWindowModal but no enableDrag
 * (covers the drag probe-absent branch of configureAfterShow). */
export class FakeWindowNoDrag {
  modalCalls: Array<Array<number>> = new Array<Array<number>>();
  rejectModal: boolean = false;

  setWindowTitleMoveEnabled(enabled: boolean): void {
  }

  setSubWindowModal(isModal: boolean, modalityType: number): Promise<void> {
    this.modalCalls.push([isModal ? 1 : 0, modalityType]);
    if (this.rejectModal) {
      return Promise.reject(new Error('modal-rejected'));
    }
    return Promise.resolve();
  }
}

/** window.WindowStage fake: controllable createSubWindow + main window. */
export class FakeWindowStage {
  subWindow: window.Window | undefined = undefined;
  subWindowErr: BusinessError | undefined = undefined;
  mainWindow: window.Window | undefined = undefined;
  createSubWindowCount: number = 0;
  createSubWindowName: string = '';
  getMainThrows: boolean = false;

  createSubWindow(name: string, callback: Function): void {
    this.createSubWindowCount += 1;
    this.createSubWindowName = name;
    const err = this.subWindowErr;
    callback(err, this.subWindow);
  }

  getMainWindowSync(): window.Window {
    if (this.getMainThrows) {
      throw new Error('getMain-throws');
    }
    return this.mainWindow as ESObject as window.Window;
  }

  windowStageEventCb: ((event: window.WindowStageEventType) => void) | undefined = undefined;

  on(event: string, callback: Function): void {
    if (event === 'windowStageEvent') {
      this.windowStageEventCb = callback as (event: window.WindowStageEventType) => void;
    }
  }

  off(event: string, callback: Function): void {
    if (event === 'windowStageEvent') {
      this.windowStageEventCb = undefined;
    }
  }
}

/** Minimal abilityInfo payload (ArkTS: no object literals). */
class FakeAbilityInfo {
  bundleName: string;

  constructor(bundleName: string) {
    this.bundleName = bundleName;
  }
}

/** common.UIAbilityContext fake: abilityInfo + startAbility + terminateSelf. */
export class FakeAbilityContext {
  bundleName: string = 'com.example.flutter_tester';
  startAbilityCount: number = 0;
  terminateCount: number = 0;
  rejectStart: boolean = false;
  rejectTerminate: boolean = false;
  terminateThrows: boolean = false;
  startedWants: Array<Want> = new Array<Want>();

  abilityInfo: Object = new FakeAbilityInfo(this.bundleName);

  startAbility(want: Want): Promise<void> {
    this.startAbilityCount += 1;
    this.startedWants.push(want);
    if (this.rejectStart) {
      return Promise.reject(new Error('startAbility-rejected'));
    }
    return Promise.resolve();
  }

  terminateSelf(): Promise<void> {
    this.terminateCount += 1;
    if (this.terminateThrows) {
      throw new Error('terminate-throws');
    }
    if (this.rejectTerminate) {
      return Promise.reject(new Error('terminate-rejected'));
    }
    return Promise.resolve();
  }
}

/** UIAbility fake. `context` is swapped per-test (real getContext() for
 * FlutterView construction, FakeAbilityContext for start/terminate). */
export class FakeAbility {
  stage: window.WindowStage | undefined = undefined;
  context: common.UIAbilityContext = getContext() as common.UIAbilityContext;
  windowStageForTest: window.WindowStage | undefined = undefined;
}

/** FlutterView fake (ESObject-cast to FlutterView): engine attach bookkeeping. */
export class FakeFlutterView {
  attached: boolean = false;
  attachCalls: number = 0;
  detachCalls: number = 0;
  detachThrows: boolean = false;

  isAttachedToFlutterEngine(): boolean {
    return this.attached;
  }

  attachToFlutterEngine(engine: FlutterEngine): void {
    this.attachCalls += 1;
    this.attached = true;
  }

  detachFromFlutterEngine(): void {
    this.detachCalls += 1;
    if (this.detachThrows) {
      throw new Error('detach-throws');
    }
    this.attached = false;
  }
}

/** Fake WindowHostContext backed by public, per-test-configurable fields. */
export class FakeHostContext implements WindowHostContext {
  parentAbilityFor: UIAbility | undefined = undefined;
  stage: window.WindowStage | undefined = undefined;
  mainViewId: string | undefined = undefined;
  engine: FlutterEngine | null = null;
  dpr: number = 2.0;
  regularAbilityName: string = 'RegularWindowAbility';
  subWindowPageName: string = 'pages/FlutterWindowPage';
  parentRectListenerCalls: number = 0;
  subWindowRectListenerCalls: number = 0;
  restoredForegrounds: Array<string> = new Array<string>();
  activated: Array<Array<number>> = new Array<Array<number>>();
  removedHosts: Array<string> = new Array<string>();
  deletedViews: Array<string> = new Array<string>();
  deletedAbilities: Array<string> = new Array<string>();
  flutterViews: Map<string, FlutterView> = new Map<string, FlutterView>();
  viewAbilities: Map<string, UIAbility> = new Map<string, UIAbility>();
  subWindows: Map<string, window.Window> = new Map<string, window.Window>();

  resolveParentAbility(parentViewId: number): UIAbility | undefined {
    return this.parentAbilityFor;
  }

  windowStageOf(ability: UIAbility): window.WindowStage | undefined {
    return this.stage;
  }

  mainFlutterViewId(): string | undefined {
    return this.mainViewId;
  }

  sharedEngine(): FlutterEngine | null {
    return this.engine;
  }

  getFlutterView(viewId: string): FlutterView | null {
    const v = this.flutterViews.get(viewId);
    return v !== undefined ? v : null;
  }

  registerFlutterView(viewId: string, flutterView: FlutterView): void {
    this.flutterViews.set(viewId, flutterView);
  }

  deleteFlutterView(viewId: string, flutterView?: FlutterView): void {
    this.deletedViews.push(viewId);
    this.flutterViews.delete(viewId);
  }

  setViewAbility(viewId: string, ability: UIAbility): void {
    this.viewAbilities.set(viewId, ability);
  }

  getViewAbility(viewId: string): UIAbility | undefined {
    return this.viewAbilities.get(viewId);
  }

  deleteViewAbility(viewId: string): void {
    this.deletedAbilities.push(viewId);
    this.viewAbilities.delete(viewId);
  }

  subWindowOfView(viewId: string): window.Window | undefined {
    return this.subWindows.get(viewId);
  }

  densityPixels(): number {
    return this.dpr;
  }

  ensureParentRectListener(ability: UIAbility, stage: window.WindowStage): void {
    this.parentRectListenerCalls += 1;
  }

  ensureSubWindowRectListener(parentViewId: number): void {
    this.subWindowRectListenerCalls += 1;
  }

  restoreParentForeground(ability: UIAbility | undefined, closedViewId: string): void {
    this.restoredForegrounds.push(closedViewId);
  }

  notifyWindowActivated(viewId: number, activated: boolean): void {
    this.activated.push([viewId, activated ? 1 : 0]);
  }

  removeHost(viewIdStr: string): void {
    this.removedHosts.push(viewIdStr);
  }

  regularWindowAbilityName(): string {
    return this.regularAbilityName;
  }

  subWindowPage(): string {
    return this.subWindowPageName;
  }
}
