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
import { FlutterOverlaySurface } from '../../../../../main/ets/embedding/engine/FlutterOverlaySurface';
import PlatformViewsController from '../../../../../main/ets/plugin/platform/PlatformViewsController';

const platformViewsController: PlatformViewsController = new PlatformViewsController();
const direction: Direction = 100;

export default function PlatformViewsControllerTest() {
  describe('PlatformViewsControllerTest', () => {

    it('TestPlatformViewRegistryImplGetRegistry', 0, async (done: Function) => {
      const result = platformViewsController.getRegistry();
      expect(result).not().assertNull();
      done();
    })

    it('TestCreateOverlaySurface', 0, async (done: Function) => {
      const result: FlutterOverlaySurface = platformViewsController.createOverlaySurface();
      expect(result).not().assertNull();
      done();
    })
    it('TestPlatformViewRegistryImplValidateDirection', 0, async (done: Function) => {
      const result = platformViewsController.validateDirection(direction);
      expect(result).assertFalse();
      done();
    })

    it('TestPlatformViewRegistryImplGetFlutterView', 0, async (done: Function) => {
      const result = platformViewsController.getFlutterView();
      expect(result).assertNull();
      done();
    })
  })
}