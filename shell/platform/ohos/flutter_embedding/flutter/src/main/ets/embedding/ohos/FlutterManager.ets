/*
* Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd.
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/


import { FlutterView } from '../../view/FlutterView';
import UIAbility from '@ohos.app.ability.UIAbility';
import window from '@ohos.window';

export default class FlutterManager {
  private static instance: FlutterManager;

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

  pushUIAbility(uiAbility: UIAbility) {
    this.uiAbilityList.push(uiAbility);
  }

  popUIAbility(uiAbility: UIAbility) {
    let index = this.uiAbilityList.findIndex((item: UIAbility) => item == uiAbility)
    if (index > 0) {
      this.uiAbilityList.splice(index, 1)
    }
  }

  pushWindowStage(uiAbility: UIAbility, windowStage: window.WindowStage) {
    this.windowStageList.set(uiAbility, windowStage)
  }

  popWindowStage(uiAbility: UIAbility) {
    this.windowStageList.delete(uiAbility)
  }

  getWindowStage(uiAbility: UIAbility): window.WindowStage {
    return this.windowStageList.get(uiAbility)!!
  }

  getUIAbility(context: Context): UIAbility {
    return this.uiAbilityList.find((item: UIAbility) => item.context == context)!!
  }

  hasFlutterView(viewId: string): boolean {
    return this.flutterViewList.has(viewId);
  }

  getFlutterView(viewId: string): FlutterView | null {
    return this.flutterViewList.get(viewId) ?? null;
  }

  getFlutterViewList(): Map<String, FlutterView> {
    return this.flutterViewList;
  }

  private putFlutterView(viewId: string, flutterView?: FlutterView): void {
    if (flutterView != null) {
      this.flutterViewList.set(viewId, flutterView);
    } else {
      this.flutterViewList.delete(viewId);
    }
  }

  /**
   * It's suggested to keep 'oh_flutter_' as the prefix for xcomponent_id.
   * Otherwise it might affect the performance.
   */
  createFlutterView(context: Context): FlutterView {
    let flutterView = new FlutterView(`oh_flutter_${this.flutterViewIndex++}`, context);
    this.putFlutterView(flutterView.getId(), flutterView);
    return flutterView;
  }

  clear(): void {
    this.flutterViewList.clear();
  }

}