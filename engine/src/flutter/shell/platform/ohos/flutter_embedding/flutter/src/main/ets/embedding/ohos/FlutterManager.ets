/*
* Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd. All rights reserved.
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE_KHZG file.
*/


import { FlutterView } from '../../view/FlutterView';
import UIAbility from '@ohos.app.ability.UIAbility';
import window from '@ohos.window';
import Log from '../../util/Log';
import HashMap from '@ohos.util.HashMap';
import List from '@ohos.util.List';
import { deviceInfo } from '@kit.BasicServicesKit';

const TAG = "FlutterManager"

/**
 * Singleton manager for Flutter views and UI abilities.
 * This class manages the lifecycle of FlutterView instances and their association with UI abilities.
 */
export default class FlutterManager {
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
  }

  /**
   * Removes the WindowStage association for a UIAbility.
   * @param uiAbility - The UIAbility
   */
  popWindowStage(uiAbility: UIAbility) {
    this.windowStageList.delete(uiAbility)
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
   * Deletes a FlutterView from the list.
   * @param viewId - The view ID
   * @param flutterView - Optional FlutterView instance to verify
   */
  deleteFlutterView(viewId: string, flutterView?: FlutterView): void {
    if (flutterView != null) {
      this.flutterViewList.delete(viewId);
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
   * Called when the screen state changes.
   * @param data - The window status type
   */
  onScreenStateChanged(data: window.WindowStatusType): void {
    if (this.skipCheck) {
      Log.i(TAG, "onScreenStateChanged: skipCheck is on, WindowStatusType = " + data)
      return;
    }
    switch (data) {
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
