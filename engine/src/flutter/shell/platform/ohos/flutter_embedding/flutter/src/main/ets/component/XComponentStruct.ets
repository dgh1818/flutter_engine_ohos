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
import Any from '../plugin/common/Any';
import ApplicationInfoLoader from '../embedding/engine/loader/ApplicationInfoLoader';

import { BuilderParams, DVModelParameters } from '../view/DynamicView/dynamicView';

@Component
struct XComponentStruct {
  private context: Any;
  private applicationInfo = ApplicationInfoLoader.load(getContext());
  dvModelParams: DVModelParameters = new DVModelParameters();

  build() {
    // todo OS解决默认背景色后可以移除冗余重复代码,仅保留差异的backgroundColor属性条件配置
    if (this.applicationInfo.isDebugMode) {
      XComponent({
        id: (this.dvModelParams as Record<string, Any>)["xComponentId"],
        type: XComponentType.TEXTURE,
        libraryname: 'flutter'
      })
        .onLoad((context) => {
          this.context = context;
        })
        .onDestroy(() => {
        })
        .backgroundColor(Color.White)
    } else {
      XComponent({
        id: (this.dvModelParams as Record<string, Any>)["xComponentId"],
        type: XComponentType.TEXTURE,
        libraryname: 'flutter'
      })
        .onLoad((context) => {
          this.context = context;
        })
        .onDestroy(() => {
        })
    }
  }
}

@Builder
export function BuildXComponentStruct(buildParams: BuilderParams) {
  XComponentStruct({ dvModelParams: buildParams.params });
}