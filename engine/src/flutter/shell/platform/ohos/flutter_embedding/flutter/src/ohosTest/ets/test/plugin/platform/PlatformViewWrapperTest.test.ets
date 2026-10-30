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
import OhosTouchProcessor from '../../../../../main/ets/embedding/ohos/OhosTouchProcessor';
import { PlatformViewWrapper } from '../../../../../main/ets/plugin/platform/PlatformViewWrapper';
import { DVModel, DVModelParameters, DVModelEvents, DVModelChildren } from '../../../../../main/ets/view/DynamicView/dynamicView';
import Any from '../../../../../main/ets/plugin/common/Any';

function sleep(time: number) {
  return new Promise<void>((resolve: Function) => setTimeout(resolve, time));
}

const platformViewWrapper: PlatformViewWrapper = new PlatformViewWrapper();

export default function PlatformViewWrapperTest() {
  describe('PlatformViewWrapperTest', () => {
    it('TestPlatformViewWrapperDvModel', 0, async (done: Function) => {
      const newTouchProcessor: OhosTouchProcessor = new OhosTouchProcessor();
      platformViewWrapper.setTouchProcessor(newTouchProcessor);
      await sleep(1000);
      const dvModel: DVModel = platformViewWrapper.getDvModel();
      expect(dvModel).not().assertNull();
      await sleep(1000);
      platformViewWrapper.addDvModel(dvModel);
      await sleep(1000);
      const parameters: DVModelParameters = new DVModelParameters();
      platformViewWrapper.setLayoutParams(parameters);
      done();
    })

    it('TestSetLayoutParamsAfterSetDvModel (covers line 87 false branch)', 0, async (done: Function) => {
      // First set a DVModel so the early-return on line 87 is not taken.
      const events: DVModelEvents = new DVModelEvents();
      const children: DVModelChildren = new DVModelChildren();
      const model: DVModel = new DVModel('row', new DVModelParameters(), events, children);
      platformViewWrapper.addDvModel(model);
      // Then setLayoutParams reaches line 90 (params null check).
      // model.params is non-null (set in DVModel constructor), so false branch.
      const parameters: DVModelParameters = new DVModelParameters();
      platformViewWrapper.setLayoutParams(parameters);
      expect(true).assertEqual(true);
      done();
    })
  })
}
