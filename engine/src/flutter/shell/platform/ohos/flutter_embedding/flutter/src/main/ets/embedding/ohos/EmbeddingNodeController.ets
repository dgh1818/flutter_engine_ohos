/*
* Copyright (c) 2024 Hunan OpenValley Digital Industry Development Co., Ltd.
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
import { BuilderNode, FrameNode, NodeController, NodeRenderType } from '@kit.ArkUI';
import Any from '../../plugin/common/Any';
import PlatformView, { Params } from '../../plugin/platform/PlatformView';
import Log from '../../util/Log';
import { DVModel, DVModelChildren, DynamicView } from '../../view/DynamicView/dynamicView';


declare class nodeControllerParams {
  surfaceId : string
  type : string
  renderType : NodeRenderType
  embedId : string
  width : number
  height : number
}

@Component
struct ButtonComponent {
  @Prop params: Params
  @State bkColor: Color = Color.Red

  build() {
    Column() {
      Button("test button")
        .border({ width: 2, color: Color.Red})
        .backgroundColor(this.bkColor)
    }
  }
}

@Builder
function ButtonBuilder(params: Params) {
  ButtonComponent({ params: params })
    .backgroundColor(Color.Green)
}

const TAG = 'EmbeddingNodeController'
export class EmbeddingNodeController extends NodeController {
  private rootNode: BuilderNode<[Params]> | undefined | null = null;
  private wrappedBuilder: WrappedBuilder<[Params]> | null = null;
  private platformView: PlatformView | undefined = undefined;
  private embedId : string = "";
  private surfaceId : string = "";
  private renderType :NodeRenderType = NodeRenderType.RENDER_TYPE_DISPLAY;
  private direction: Direction = Direction.Auto;
  private isDestroy : boolean = false;

  setRenderOption(platformView: PlatformView, surfaceId: string, renderType: NodeRenderType, direction: Direction) {
    if (platformView == undefined) {
      Log.e(TAG, "platformView undefined");
    } else {
      this.wrappedBuilder = platformView.getView();
    }
    this.platformView = platformView;
    this.surfaceId = surfaceId;
    this.renderType = renderType;
    this.direction = direction;
  }

  makeNode(uiContext: UIContext): FrameNode | null{
    this.rootNode = new BuilderNode(uiContext, { surfaceId: this.surfaceId, type: this.renderType});
    if (this.wrappedBuilder == null) {
      this.rootNode.build(wrapBuilder(ButtonBuilder), {});
    } else {
      this.rootNode.build(this.wrappedBuilder, {direction: this.direction, platformView: this.platformView});
    }
    return this.rootNode.getFrameNode();
  }

  setBuilderNode(rootNode: BuilderNode<Params[]> | null): void{
    this.rootNode = rootNode;
  }

  getBuilderNode(): BuilderNode<[Params]> | undefined | null{
    return this.rootNode;
  }

  updateNode(arg: Object): void {
    this.rootNode?.update(arg);
  }
  getEmbedId() : string {
    return this.embedId;
  }

  setDestroy(isDestroy : boolean) : void {
    this.isDestroy = isDestroy;
    if (this.isDestroy) {
      this.rootNode = null;
    }
  }

  postEvent(event: TouchEvent | undefined) : boolean {
    if (event != undefined) {
      let changedTouchLen = event.changedTouches.length;
      for (let i = 0; i< changedTouchLen; i++) {
        event.changedTouches[i].displayX = vp2px(event.changedTouches[i].displayX);
        event.changedTouches[i].displayY = vp2px(event.changedTouches[i].displayY);
        event.changedTouches[i].windowX = vp2px(event.changedTouches[i].windowX);
        event.changedTouches[i].windowY = vp2px(event.changedTouches[i].windowY);
        event.changedTouches[i].screenX = vp2px(event.changedTouches[i].screenX);
        event.changedTouches[i].screenY = vp2px(event.changedTouches[i].screenY);
        event.changedTouches[i].x = vp2px(event.changedTouches[i].x);
        event.changedTouches[i].y = vp2px(event.changedTouches[i].y);
      }
      return this.rootNode?.postTouchEvent(event) as boolean
    } else {
      return false;
    }
  }
}