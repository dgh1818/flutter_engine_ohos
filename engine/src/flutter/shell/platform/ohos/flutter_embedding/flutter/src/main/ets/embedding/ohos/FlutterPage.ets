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
import Any from '../../plugin/common/Any';
import Log from '../../util/Log';
import { DVModel, DVModelChildren, DynamicView } from '../../view/DynamicView/dynamicView';
import { FlutterView } from '../../view/FlutterView';
import FlutterManager from './FlutterManager';

const TAG = "FlutterPage";

/**
 * 基础page组件，承载XComponent组件
 */
@Component
export struct FlutterPage {
  @State rootDvModel: DVModelChildren | undefined = undefined
  @Prop viewId: string = ""
  @Prop xComponentType: XComponentType = XComponentType.TEXTURE

  @Builder doNothingBuilder() {}
  @BuilderParam splashScreenView: () => void = this.doNothingBuilder;
  @State showSplashScreen: boolean = true;

  private flutterView?: FlutterView | null

  aboutToAppear() {
    this.flutterView = FlutterManager.getInstance().getFlutterView(this.viewId);
    this.rootDvModel = this.flutterView!!.getDVModel().children
    this.flutterView?.addFirstFrameListener(this)
  }

  aboutToDisappear() {
    this.flutterView?.removeFirstFrameListener(this);
  }

  onFirstFrame() {
    this.showSplashScreen = false;
  }

  build() {
    Stack() {
      XComponent({ id: this.viewId, type: this.xComponentType, libraryname: 'flutter' })
        .focusable(true)
        .focusOnTouch(true)
        .onLoad((context) => {
          this.flutterView?.onSurfaceCreated()
          Log.d(TAG, "XComponent onLoad ");
        })
        .onDestroy(() => {
          Log.d(TAG, "XComponent onDestroy ");
          this.flutterView?.onSurfaceDestroyed()
        })
        .backgroundColor(Color.Transparent)

      ForEach(this.rootDvModel!!, (child: Any) => {
        DynamicView({
          model: child as DVModel,
          params: child.params,
          events: child.events,
          children: child.children,
          customBuilder: child.builder
        })
      },(child: Any) => `${child.id_}`)

      if (this.showSplashScreen) {
        this.splashScreenView();
      }
    }
    .onAreaChange((oldValue: Area, newValue: Area) => {
      Log.d(TAG, "onAreaChange ");
      this.flutterView?.onAreaChange(newValue)
    })
    .onKeyEvent((event: KeyEvent) => {
      Log.d(TAG, "onKeyEvent " + event.type);
      this.flutterView?.onKeyEvent(event)
    })
  }
}
