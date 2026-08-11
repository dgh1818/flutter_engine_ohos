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
import { PlatformViewWrapper } from '../../../../../main/ets/plugin/platform/PlatformViewWrapper';
import { RootDvModeManager } from '../../../../../main/ets/plugin/platform/RootDvModelManager';
import { DVModel, DVModelContainer } from '../../../../../main/ets/view/DynamicView/dynamicView';

export default function DVModelContainerTest() {
  describe('DVModelContainerTest', () => {
    it('TestDVModelContainerGetRootDvMode', 0, async (done: Function) => {
      const dvModel: DVModelContainer = RootDvModeManager.getRootDvMode();
      expect(dvModel).not().assertNull();
      done();
    })

    it('TestDVModelAddDvModel', 0, async (done: Function) => {
      const platformViewWrapper: PlatformViewWrapper = new PlatformViewWrapper();
      const dvModel: DVModel = platformViewWrapper.getDvModel();
      RootDvModeManager.addDvModel(dvModel);
      expect(dvModel).not().assertNull();
      done();
    })
  })
}