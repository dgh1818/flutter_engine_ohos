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
import { FlutterWindowHost } from '../../../../../../main/ets/embedding/ohos/windowing/FlutterWindowHost';
import { DialogWindowCreationRequest, PopupWindowCreationRequest,
  RegularWindowCreationRequest, TooltipWindowCreationRequest,
  WindowConstraints, WindowSizeRequest } from '../../../../../../main/ets/embedding/ohos/windowing/WindowingUtils';
import { NO_PARENT_VIEW_ID } from '../../../../../../main/ets/embedding/ohos/windowing/WindowingUtils';
import { FlutterView } from '../../../../../../main/ets/view/FlutterView';
import FlutterEngine from '../../../../../../main/ets/embedding/engine/FlutterEngine';

/** Waits for pending microtasks/promises (attach chains are async). */
export function sleep(time: number): Promise<void> {
  return new Promise<void>((resolve: Function) => setTimeout(resolve, time));
}

/** Typed request makers, one per archetype (w/h > 0 means a preferred
 * size). */
/** Preferred-size inner object (its fields are mutable; the wrapper's are
 * readonly, so titled/parented requests are built via the copy ctor). */
function preferredSizeOf(width: number, height: number): WindowSizeRequest {
  const preferred = new WindowSizeRequest();
  preferred.hasPreferredViewSize = width > 0 && height > 0;
  preferred.preferredViewWidth = width;
  preferred.preferredViewHeight = height;
  return preferred;
}

/** Mutable init shape for the (readonly) request wrappers — the copy ctors
 * read these field names (ArkTS: no object literals). */
class WindowRequestInit {
  preferredSize: WindowSizeRequest = new WindowSizeRequest();
  preferredConstraints: WindowConstraints = new WindowConstraints();
  title: string = '';
  parentViewId: number = NO_PARENT_VIEW_ID;
}

export function regularRequest(width: number, height: number,
  title: string): RegularWindowCreationRequest {
  const raw = new WindowRequestInit();
  raw.preferredSize = preferredSizeOf(width, height);
  raw.title = title;
  return new RegularWindowCreationRequest(raw as ESObject as RegularWindowCreationRequest);
}

export function dialogRequest(parentViewId: number, width: number, height: number,
  title: string): DialogWindowCreationRequest {
  const raw = new WindowRequestInit();
  raw.preferredSize = preferredSizeOf(width, height);
  raw.title = title;
  raw.parentViewId = parentViewId;
  return new DialogWindowCreationRequest(raw as ESObject as DialogWindowCreationRequest);
}

export function tooltipRequest(parentViewId: number): TooltipWindowCreationRequest {
  const raw = new WindowRequestInit();
  raw.parentViewId = parentViewId;
  return new TooltipWindowCreationRequest(raw as ESObject as TooltipWindowCreationRequest);
}

export function popupRequest(parentViewId: number): PopupWindowCreationRequest {
  const raw = new WindowRequestInit();
  raw.parentViewId = parentViewId;
  return new PopupWindowCreationRequest(raw as ESObject as PopupWindowCreationRequest);
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
  /** Invalid display id: the size funnel's display lookup throws on it and
   * falls back to scale 1.0 — deterministic in UT regardless of device. */
  displayId: number = -1;
  windowRect: window.Rect = new FakeRect(0, 0, 1000, 800) as ESObject as window.Rect;
  drawableRect: window.Rect = new FakeRect(0, 0, 1000, 800) as ESObject as window.Rect;

  setWindowRect(left: number, top: number, width: number, height: number): void {
    this.windowRect = new FakeRect(left, top, width, height) as ESObject as window.Rect;
  }

  /** Simulate a decorated window (e.g. 2in1 title bar): outer frame inset of
   * (w, h) around the 1000x800 content. */
  setDecorInsets(w: number, h: number): void {
    this.windowRect = new FakeRect(0, 0, 1000 + w, 800 + h) as ESObject as window.Rect;
    this.drawableRect = new FakeRect(0, 0, 1000, 800) as ESObject as window.Rect;
  }
}

/** window.AvoidArea fake: system/navigation avoidance rects (height 0 default). */
class FakeAvoidAreaRect {
  height: number = 0;
}

class FakeAvoidArea {
  topRect: FakeAvoidAreaRect = new FakeAvoidAreaRect();
  bottomRect: FakeAvoidAreaRect = new FakeAvoidAreaRect();
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
  fullScreenLayoutCalls: Array<boolean> = new Array<boolean>();
  windowStatus: window.WindowStatusType = window.WindowStatusType.UNDEFINED;
  decorVisible: boolean = false;
  titleButtonRect: window.Rect | null = null;
  inFreeWindowMode: boolean = false;
  avoidArea: FakeAvoidArea = new FakeAvoidArea();
  minimizeCount: number = 0;
  restoreCount: number = 0;
  fullscreenArgs: Array<boolean> = new Array<boolean>();
  constraintCalls: Array<Array<number>> = new Array<Array<number>>();
  backgroundColor: string = '';
  cornerRadius: number = -1;
  shadowRadius: number = -1;
  focusable: boolean = true;
  titleMoveEnabled: boolean = false;
  /** getWindowDecorHeight() source in vp (getDecorHeightPx multiplies by dpr). */
  decorHeightVp: number = 0;
  /** setWindowTitleButtonVisible(isMax, isMin, isClose) args, 1/0-coded. */
  titleButtonCalls: Array<Array<number>> = new Array<Array<number>>();
  titleButtonThrows: boolean = false;
  modalCalls: Array<Array<number>> = new Array<Array<number>>();
  dragEnabled: boolean = false;
  windowEventCb: ((event: window.WindowEventType) => void) | undefined = undefined;
  rectChangeCb: ((options: window.RectChangeOptions) => void) | undefined = undefined;
  /** windowSizeChange on() callback (state-driven surface size bridge). */
  sizeChangeCb: ((size: window.Size) => void) | undefined = undefined;
  destroyCount: number = 0;
  loadContentCount: number = 0;
  loadContentPage: string = '';
  /** windowRectChange on() registration count (idempotency probe). */
  rectOnCount: number = 0;
  /** getWindowProperties() call count (reanchor/positionAt probe). */
  propsReads: number = 0;
  offThrows: boolean = false;
  /** getWindowProperties() throws synchronously. */
  propsThrows: boolean = false;

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
    this.reflectOuterSize(w, h);
    return Promise.resolve();
  }

  /** Official WMS-applied variant (TooltipWindowHost uses this one); same
   * recording bucket as resize. */
  resizeAsync(w: number, h: number): Promise<void> {
    return this.resize(w, h);
  }

  /** WMS applying a resize: the OUTER frame takes the new size while the
   * simulated decoration inset (windowRect − drawableRect) is kept — a real
   * window's properties reflect the change (fireParentResize models the
   * same for parent resizes). The funnel's skip-if-already-at-target check
   * needs this: a props-static fake would swallow every second resize. */
  reflectOuterSize(w: number, h: number): void {
    const outer = this.props.windowRect;
    const drawable = this.props.drawableRect;
    const insetW = Math.max(0, Math.round(outer.width - drawable.width));
    const insetH = Math.max(0, Math.round(outer.height - drawable.height));
    this.props.setWindowRect(outer.left, outer.top, w, h);
    this.props.drawableRect = new FakeRect(drawable.left, drawable.top,
      Math.max(0, w - insetW), Math.max(0, h - insetH)) as ESObject as window.Rect;
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

  setWindowLayoutFullScreen(useFullScreen: boolean): void {
    this.fullScreenLayoutCalls.push(useFullScreen);
  }

  getWindowStatus(): window.WindowStatusType {
    return this.windowStatus;
  }

  getWindowDecorVisible(): boolean {
    return this.decorVisible;
  }

  getTitleButtonRect(): window.Rect {
    return this.titleButtonRect as ESObject as window.Rect;
  }

  isInFreeWindowMode(): boolean {
    return this.inFreeWindowMode;
  }

  setSpecificSystemBarEnabled(specificSystemBar: string, isVisible: boolean, enableAnimation?: boolean): void {
  }

  getWindowAvoidArea(type: window.AvoidAreaType): window.AvoidArea {
    return this.avoidArea as ESObject as window.AvoidArea;
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

  setWindowLimits(limits: window.WindowLimits): Promise<window.WindowLimits> {
    // Recorded as [minWidth, maxWidth, minHeight, maxHeight] scalars.
    this.constraintCalls.push([
      limits.minWidth ?? -1,
      limits.maxWidth ?? -1,
      limits.minHeight ?? -1,
      limits.maxHeight ?? -1,
    ]);
    if (this.rejectConstraints) {
      return Promise.reject(new Error('constraints-rejected'));
    }
    // Real API resolves with the EFFECTIVE limits; echo the input.
    return Promise.resolve(limits);
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

  getWindowDecorHeight(): number {
    return this.decorHeightVp;
  }

  setWindowTitleMoveEnabled(enabled: boolean): void {
    if (this.titleMoveThrows) {
      throw new Error('titlemove-throws');
    }
    this.titleMoveEnabled = enabled;
  }

  setWindowTitleButtonVisible(isMaximizeButtonVisible: boolean,
                              isMinimizeButtonVisible: boolean,
                              isCloseButtonVisible?: boolean): void {
    this.titleButtonCalls.push([
      isMaximizeButtonVisible ? 1 : 0,
      isMinimizeButtonVisible ? 1 : 0,
      isCloseButtonVisible === undefined ? -1 : (isCloseButtonVisible ? 1 : 0),
    ]);
    if (this.titleButtonThrows) {
      throw new Error('titlebutton-throws');
    }
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
    if (this.propsThrows) {
      throw new Error('props-throws');
    }
    this.propsReads += 1;
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
      this.rectOnCount += 1;
      this.rectChangeCb = callback as (options: window.RectChangeOptions) => void;
    } else if (event === 'windowSizeChange') {
      this.sizeChangeCb = callback as (size: window.Size) => void;
    } else {
      this.windowEventCb = callback as (event: window.WindowEventType) => void;
    }
  }

  off(event: string, callback: Function): void {
    if (this.offThrows) {
      throw new Error('off-throws');
    }
    if (event === 'windowRectChange') {
      this.rectChangeCb = undefined;
    } else if (event === 'windowSizeChange') {
      this.sizeChangeCb = undefined;
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

/** Dialog-API probe-missing variant: no setWindowTitleMoveEnabled /
 * setSubWindowModal / enableDrag. */
export class FakeWindowNoDialogApi {
}

/** Title-button probe-missing variant: no setWindowTitleButtonVisible
 * (covers the API-14-unavailable branch of decorateTitleButtons). */
export class FakeWindowNoTitleButton {
  resize(_w: number, _h: number): Promise<void> {
    return Promise.resolve();
  }
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
    // The sub-window hosts read err.code unguarded, so the success arm must still
    // hand back a code-bearing object (the real API never passes undefined here).
    const err = this.subWindowErr ?? { code: 0 } as ESObject as BusinessError;
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
  stage: window.WindowStage | undefined = undefined;
  subWindowPageName: string = 'pages/FlutterWindowPage';
  /** WindowHostContext.spawnedAbilityName() source (per-test configurable). */
  spawnedAbilityNameValue: string = 'RegularWindowAbility';
  restoredForegrounds: Array<string> = new Array<string>();
  activated: Array<Array<number>> = new Array<Array<number>>();
  removedHosts: Array<number> = new Array<number>();
  deletedAbilities: Array<number> = new Array<number>();
  deletedViews: Array<string> = new Array<string>();
  flutterViews: Map<string, FlutterView> = new Map<string, FlutterView>();
  /** getHostWindowByView() source: view-id string → host (each host carries
   * its own OS-window handle, so tests seed hosts, not bare windows). */
  hosts: Map<string, FlutterWindowHost> = new Map<string, FlutterWindowHost>();
  /** getAbilityByView() source: view-id string → backing UIAbility. */
  abilities: Map<string, UIAbility> = new Map<string, UIAbility>();

  getWindowStage(ability: UIAbility): window.WindowStage | undefined {
    return this.stage;
  }

  getAbilityByView(viewId: number): UIAbility | undefined {
    return this.abilities.get(String(viewId));
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

  getHostWindowByView(viewId: number): FlutterWindowHost | undefined {
    return this.hosts.get(String(viewId));
  }

  notifyWindowActivated(viewId: number, activated: boolean): void {
    this.activated.push([viewId, activated ? 1 : 0]);
  }

  removeHost(viewId: number): void {
    this.removedHosts.push(viewId);
  }

  deleteViewAbility(viewId: number): void {
    this.deletedAbilities.push(viewId);
  }

  subWindowPage(): string {
    return this.subWindowPageName;
  }

  spawnedAbilityName(): string {
    return this.spawnedAbilityNameValue;
  }
}
