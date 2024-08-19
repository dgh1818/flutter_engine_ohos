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

import Log from '../../util/Log';
import { FlutterView } from '../../view/FlutterView';
import FlutterManager from './FlutterManager';
import { DVModel, DVModelChildren, DynamicView } from '../../view/DynamicView/dynamicView';
import Any from '../../plugin/common/Any';

const TAG = "FlutterPage";
export const OHOS_FLUTTER_PAGE_UPDATE = "ohos_flutter_page_update";

/**
 * 基础page组件，承载XComponent组件
 */
@Component
export struct FlutterPage {
  @Prop viewId: string = ""
  @Prop xComponentType: XComponentType = XComponentType.SURFACE

  @Builder
  doNothingBuilder() {
  }

  @BuilderParam splashScreenView: () => void = this.doNothingBuilder;
  @State showSplashScreen: boolean = true;
  @State checkFullScreen: boolean = true;
  @State checkKeyboard: boolean = true;
  @State checkGesture: boolean = true;
  @State checkAiBar: boolean = true;
  @StorageLink('nodeWidth') storageLinkWidth: number = 0;
  @StorageLink('nodeHeight') storageLinkHeight: number = 0;
  @State rootDvModel: DVModelChildren | undefined = undefined

  @State isNeedUpdate: boolean = false;

  private flutterView?: FlutterView | null
  private lastArea?: Area;
  private panOption: PanGestureOptions = new PanGestureOptions({ direction: PanDirection.Up | PanDirection.Down });

  aboutToAppear() {
    this.flutterView = FlutterManager.getInstance().getFlutterView(this.viewId);
    this.flutterView?.addFirstFrameListener(this)
    this.flutterView?.addFirstPreloadFrameListener(this)

    this.flutterView?.setCheckFullScreen(this.checkFullScreen)
    this.flutterView?.setCheckKeyboard(this.checkKeyboard)
    this.flutterView?.setCheckGesture(this.checkGesture)
    this.flutterView?.setCheckAiBar(this.checkGesture)

    this.rootDvModel = this.flutterView!!.getDVModel().children
    getContext().eventHub.on(OHOS_FLUTTER_PAGE_UPDATE, () => {
      this.isNeedUpdate = true;
    })
  }

  aboutToDisappear() {
    this.flutterView?.removeFirstFrameListener(this);
    this.flutterView?.removeFirstPreloadFrameListener(this)
    getContext().eventHub.off(OHOS_FLUTTER_PAGE_UPDATE)
  }

  onFirstFrame() {
    this.showSplashScreen = false;
  }

  onFirstPreloadFrame() {
  }

  build() {
    Stack() {
      ForEach(this.rootDvModel!!, (child: Any) => {
        DynamicView({
          model: child as DVModel,
          params: child.params,
          events: child.events,
          children: child.children,
          customBuilder: child.builder
        })
      }, (child: ESObject) => `${child.id_}`)


      Text('')
        .id('emptyFocusText' + this.viewId)
        .size({ width: 0, height: 0 })
        .opacity(0)
        .focusable(true)

      XComponent({ id: this.viewId, type: this.xComponentType, libraryname: 'flutter' })
        .id(this.viewId)
        .focusable(true)
        .onLoad((context) => {
          this.flutterView?.onSurfaceCreated()
          Log.d(TAG, "XComponent onLoad ");
        })
        .onDestroy(() => {
          Log.d(TAG, "XComponent onDestroy ");
          this.flutterView?.onSurfaceDestroyed()
        })
        .backgroundColor(Color.Transparent)

      if (this.showSplashScreen) {
        this.splashScreenView();
      }
    }
    .defaultFocus(true)
    .onAreaChange((oldValue: Area, newValue: Area) => {
      if (this.isNeedUpdate || !this.lastArea || oldValue.width != newValue.width
        || oldValue.height != newValue.height) {
        Log.d(TAG, "onAreaChange, old=" + JSON.stringify(oldValue));
        Log.d(TAG, "onAreaChange, new=" + JSON.stringify(newValue));
        this.lastArea = newValue;
        this.flutterView?.onAreaChange(newValue)
        this.isNeedUpdate = false;
      }
    })
    .onKeyEvent((event: KeyEvent) => {
      Log.d(TAG, "onKeyEvent " + event.type);
      this.flutterView?.onKeyEvent(event)
    })
    .defaultFocus(true)
    .gesture(
      PanGesture(this.panOption)
        .onActionStart((event: GestureEvent) => {
          this.flutterView?.onMouseWheel("actionStart", event);
        })
        .onActionUpdate((event: GestureEvent) => {
          this.flutterView?.onMouseWheel("actionUpdate", event);
        })
        .onActionEnd((event: GestureEvent) => {
          this.flutterView?.onMouseWheel("actionEnd", event);
        })
    )
  }
}
