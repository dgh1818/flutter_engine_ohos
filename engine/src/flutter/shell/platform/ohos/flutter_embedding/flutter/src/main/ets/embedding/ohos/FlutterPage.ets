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
import { PlatformViewWrapper } from '../../plugin/platform/PlatformViewWrapper';
import { RootDvModeManager } from '../../plugin/platform/RootDvModelManager';
import Log from '../../util/Log';
import {
  DVModel,
  DVModelChildren,
  DVModelContainer,
  DVModelEvents,
  DVModelParameters,
  DynamicView
} from '../../view/DynamicView/dynamicView';
import { createDVModelFromJson } from '../../view/DynamicView/dynamicViewJson';
import { FlutterView } from '../../view/FlutterView';
import KeyEventChannel from '../engine/systemchannels/KeyEventChannel';
import FlutterManager from './FlutterManager';

const TAG = "FlutterPage";

/**
 * 基础page组件，承载XComponent组件
 */
@Component
export struct FlutterPage {
  @State rootDvModel: DVModelChildren | undefined = undefined
  @Prop viewId: string = ""
  private flutterView?: FlutterView | null

  aboutToAppear() {
    this.flutterView = FlutterManager.getInstance().getFlutterView(this.viewId);
    this.rootDvModel = this.flutterView!!.getDVModel().children
  }

  build() {
    Stack() {
      XComponent({ id: this.viewId, type: XComponentType.SURFACE, libraryname: 'flutter' })
        .focusable(true)
        .onLoad((context) => {
          this.flutterView?.onSurfaceCreated()
          Log.d(TAG, "XComponent onLoad ");
        })
        .onDestroy(() => {
          Log.d(TAG, "XComponent onDestroy ");
          this.flutterView?.onSurfaceDestroyed()
        })
      ForEach(this.rootDvModel!!, (child:ESObject) =>{
        DynamicView({
          model: child as DVModel,
          params:child.params,
          events:child.events,
          children:child.children,
          customBuilder:child.builder
        })
      })
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