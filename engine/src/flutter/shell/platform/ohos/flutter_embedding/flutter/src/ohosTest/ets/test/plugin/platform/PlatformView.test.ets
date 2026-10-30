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
      expect(platformView.ohosHcppViewId).assertEqual(-1);
      expect(platformView.touchDispatcher).assertNull();
      expect(platformView.axisDispatcher).assertNull();
      expect(platformView.injectedTimestamps.size).assertEqual(0);
      platformView.ohosHcppViewId = 7;
      platformView.touchDispatcher = (_id: number, _event: TouchEvent): void => {
      };
      platformView.axisDispatcher = (_id: number, _event: AxisEvent): void => {
      };
      platformView.injectedTimestamps.set(1, 2);
      expect(platformView.ohosHcppViewId).assertEqual(7);
      expect(platformView.touchDispatcher).not().assertNull();
      expect(platformView.axisDispatcher).not().assertNull();
      expect(platformView.injectedTimestamps.get(1)).assertEqual(2);
    })

    it('getPlatformViewVisibleAreaEventOptions returns object', 0, () => {
      const opts = platformView.getPlatformViewVisibleAreaEventOptions();
      expect(opts).not().assertNull();
    });

    it('shouldParticipateInTextInputHandoff returns boolean', 0, () => {
      const r = platformView.shouldParticipateInTextInputHandoff();
      expect(typeof r).assertEqual('boolean');
    });

    it('shouldTriggerTextInputHandoffOnTouch returns boolean', 0, () => {
      const r = platformView.shouldTriggerTextInputHandoffOnTouch();
      expect(typeof r).assertEqual('boolean');
    });

    it('onInputConnectionLocked does not throw', 0, () => {
      let threw = false;
      try { platformView.onInputConnectionLocked(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('onInputConnectionUnlocked does not throw', 0, () => {
      let threw = false;
      try { platformView.onInputConnectionUnlocked(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('onInactive does not throw', 0, () => {
      let threw = false;
      try { platformView.onInactive(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('onActive does not throw', 0, () => {
      let threw = false;
      try { platformView.onActive(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('onFlutterViewDetached does not throw', 0, () => {
      let threw = false;
      try { platformView.onFlutterViewDetached(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });
  })
}