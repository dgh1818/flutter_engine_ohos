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
import { DVModel, DVModelParameters } from '../../../../../main/ets/view/DynamicView/dynamicView';

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
  })
}
