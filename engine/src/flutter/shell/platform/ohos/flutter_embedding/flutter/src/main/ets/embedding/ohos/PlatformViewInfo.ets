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

import PlatformView from '../../plugin/platform/PlatformView';

export class PlatformViewInfo {
  public platformView: PlatformView;
  public surfaceId: string;
  public width: number;
  public height: number;
  public top: number;
  public left: number;
  public direction: Direction;

  constructor(platformView: PlatformView, surfaceId: string, width: number, height: number, top: number, left: number,
    direction: Direction) {
    this.platformView = platformView;
    this.surfaceId = surfaceId;
    this.width = width;
    this.height = height;
    this.top = top;
    this.left = left;
    this.direction = direction;
  }
}
