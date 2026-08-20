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

import { describe, expect, it } from '@ohos/hypium'
import PlatformView, { Params } from '../../../../../main/ets/plugin/platform/PlatformView';

export default function PlatformViewTest() {
  describe('PlatformViewTest', () => {
    class TestPlatformView extends PlatformView {
      getView(): WrappedBuilder<[Params]> {
        throw new Error('Method not implemented.');
      }

      dispose(): void {
        throw new Error('Method not implemented.');
      }
    }

    const platformView: PlatformView = new TestPlatformView()
    it('getType', 0, () => {
      expect(platformView.getType()).assertEqual('default')
    })

    it('hcpp_dispatcher_fields_default_null', 0, () => {
      expect(platformView.viewId).assertEqual(-1);
      expect(platformView.touchDispatcher).assertNull();
      expect(platformView.axisDispatcher).assertNull();
      expect(platformView.injectedTimestamps.size).assertEqual(0);
      platformView.viewId = 7;
      platformView.touchDispatcher = (_id: number, _event: TouchEvent): void => {
      };
      platformView.axisDispatcher = (_id: number, _event: AxisEvent): void => {
      };
      platformView.injectedTimestamps.set(1, 2);
      expect(platformView.viewId).assertEqual(7);
      expect(platformView.touchDispatcher).not().assertNull();
      expect(platformView.axisDispatcher).not().assertNull();
      expect(platformView.injectedTimestamps.get(1)).assertEqual(2);
    })
  })
}