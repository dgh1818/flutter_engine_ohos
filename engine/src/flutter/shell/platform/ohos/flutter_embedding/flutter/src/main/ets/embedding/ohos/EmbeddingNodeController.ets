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
  surfaceId: string
  type: string
  renderType: NodeRenderType
  embedId: string
  width: number
  height: number
}

const TAG = 'EmbeddingNodeController'

export class EmbeddingNodeController extends NodeController {
  private rootNode: FrameNode | null = null;
  private builderNode: BuilderNode<[Params]> | undefined | null = null;
  private wrappedBuilder: WrappedBuilder<[Params]> | null = null;
  private platformView: PlatformView | undefined = undefined;
  private embedId: string = "";
  private surfaceId: string = "";
  private renderType: NodeRenderType = NodeRenderType.RENDER_TYPE_DISPLAY;
  private direction: Direction = Direction.Auto;
  private isDestroy: boolean = false;

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

  makeNode(uiContext: UIContext): FrameNode | null {
    this.rootNode = new FrameNode(uiContext);
    this.builderNode = new BuilderNode(uiContext, { surfaceId: this.surfaceId, type: this.renderType });
    if (this.platformView) {
      this.builderNode.build(this.wrappedBuilder, { direction: this.direction, platformView: this.platformView });
    }
    this.rootNode.appendChild(this.builderNode.getFrameNode())
    return this.rootNode.getFirstChild();
  }

  setBuilderNode(builderNode: BuilderNode<Params[]> | null): void {
    this.builderNode = builderNode;
  }

  getBuilderNode(): BuilderNode<[Params]> | undefined | null {
    return this.builderNode;
  }

  updateNode(arg: Object): void {
    this.builderNode?.update(arg);
  }

  getEmbedId(): string {
    return this.embedId;
  }

  setDestroy(isDestroy: boolean): void {
    this.isDestroy = isDestroy;
    if (this.isDestroy) {
      this.rootNode?.dispose()
      this.rootNode = null;
    }
  }

  disposeFrameNode() {
    if (this.rootNode !== null && this.builderNode !== null) {
      this.rootNode.removeChild(this.builderNode?.getFrameNode());
      this.builderNode?.dispose();

      this.rootNode.dispose();
    }
  }

  removeBuilderNode() {
    const rootRenderNode = this.rootNode!.getRenderNode();
    if (rootRenderNode !== null && this.builderNode !== null && this.builderNode?.getFrameNode() !== null) {
      rootRenderNode.removeChild(this.builderNode!.getFrameNode()!.getRenderNode());
    }
  }

  postEvent(event: TouchEvent | undefined, isPx: boolean = false): boolean {
    if (event == undefined) {
      return false;
    }

    // change vp to px
    if (!isPx) {
      let changedTouchLen = event.changedTouches.length;
      for (let i = 0; i < changedTouchLen; i++) {
        event.changedTouches[i].displayX = vp2px(event.changedTouches[i].displayX);
        event.changedTouches[i].displayY = vp2px(event.changedTouches[i].displayY);
        event.changedTouches[i].windowX = vp2px(event.changedTouches[i].windowX);
        event.changedTouches[i].windowY = vp2px(event.changedTouches[i].windowY);
        event.changedTouches[i].screenX = vp2px(event.changedTouches[i].screenX);
        event.changedTouches[i].screenY = vp2px(event.changedTouches[i].screenY);
        event.changedTouches[i].x = vp2px(event.changedTouches[i].x);
        event.changedTouches[i].y = vp2px(event.changedTouches[i].y);
        Log.d(TAG, "changedTouches[" + i + "] displayX:" + event.changedTouches[i].displayX + " displayY:" +
          event.changedTouches[i].displayY + " x:" + event.changedTouches[i].x + " y:" + event.changedTouches[i].y);
      }
      let touchesLen = event.touches.length;
      for (let i = 0; i< touchesLen; i++) {
        event.touches[i].displayX = vp2px(event.touches[i].displayX);
        event.touches[i].displayY = vp2px(event.touches[i].displayY);
        event.touches[i].windowX = vp2px(event.touches[i].windowX);
        event.touches[i].windowY = vp2px(event.touches[i].windowY);
        event.touches[i].screenX = vp2px(event.touches[i].screenX);
        event.touches[i].screenY = vp2px(event.touches[i].screenY);
        event.touches[i].x = vp2px(event.touches[i].x);
        event.touches[i].y = vp2px(event.touches[i].y);
        Log.d(TAG, "touches[" + i + "] displayX:" + event.touches[i].displayX + " displayY:" +
          event.touches[i].displayY + " x:" + event.touches[i].x + " y:" + event.touches[i].y);
      }
    }

    return this.builderNode?.postTouchEvent(event) as boolean
  }
}