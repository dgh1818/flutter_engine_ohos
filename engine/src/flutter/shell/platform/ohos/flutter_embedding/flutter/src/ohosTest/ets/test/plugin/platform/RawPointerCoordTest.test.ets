/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
import { describe, expect, it } from '@ohos/hypium';
import { RawPointerCoords } from '../../../../../main/ets/plugin/platform/RawPointerCoord';

const orientation: number = 100;
const pressure: number = 100;
const size: number = 100;
const toolMajor: number = 100;
const toolMinor: number = 100;
const touchMajor: number = 100;
const touchMinor: number = 100;
const x: number = 100;
const y: number = 100;
const rawPointerCoords: RawPointerCoords =
  new RawPointerCoords(orientation, pressure, size, toolMajor, toolMinor, touchMajor, touchMinor, x, y);

export default function RawPointerCoordsTest() {
  describe('RawPointerCoordsTest', () => {
    it('TestRawPointerCoordsGetX', 0, async (done: Function) => {
      const pointX = rawPointerCoords.getX();
      expect(pointX).not().assertNull();
      done();
    })

    it('TestRawPointerCoordsGetY', 0, async (done: Function) => {
      const pointY = rawPointerCoords.getY();
      expect(pointY).not().assertNull();
      done();
    })
  })
}