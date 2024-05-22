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

export class RawPointerCoords {
  private orientation: number = 0;
  private pressure: number = 0;
  private size: number = 0;
  private toolMajor: number = 0;
  private toolMinor: number = 0;
  private touchMajor: number = 0;
  private touchMinor: number = 0;
  private x: number = 0;
  private y: number = 0;

  constructor(orientation: number, pressure: number, size: number, toolMajor: number, toolMinor: number,
              touchMajor: number, touchMinor: number, x: number, y: number) {
    this.orientation = orientation;
    this.pressure = pressure;
    this.size = size;
    this.toolMajor = toolMajor;
    this.toolMinor = toolMinor;
    this.touchMajor = touchMajor;
    this.touchMinor = touchMinor;
    this.x = x;
    this.y = y;
  }

  getX(): number {
    return this.x;
  }

  getY(): number {
    return this.y;
  }

}