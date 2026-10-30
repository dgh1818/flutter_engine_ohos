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
import { TouchEventId, TouchEventTracker } from '../../../../../main/ets/embedding/ohos/TouchEventTracker';
import { TouchEvent } from '@ohos.multimodalInput.touchEvent';

/** Marker carried through the (cast) TouchEvent so pop results can be verified. */
class MarkedTouch {
  v: number = 0;
}

export default function TouchEventTrackerTest() {
  describe('TouchEventTrackerTest', () => {

    it('getInstance returns same singleton on repeated calls', 0, () => {
      const first: TouchEventTracker = TouchEventTracker.getInstance();
      const second: TouchEventTracker = TouchEventTracker.getInstance();
      expect(first).assertEqual(second);
    })

    it('track assigns monotonically increasing ids', 0, () => {
      const tracker: TouchEventTracker = new TouchEventTracker();
      const e: MarkedTouch = new MarkedTouch();
      const id1: TouchEventId = tracker.track(e as ESObject as TouchEvent);
      const id2: TouchEventId = tracker.track(e as ESObject as TouchEvent);
      expect(id2.getId()).assertLarger(id1.getId());
    })

    it('pop returns newest event and discards older ones', 0, () => {
      const tracker: TouchEventTracker = new TouchEventTracker();
      const e1: MarkedTouch = new MarkedTouch();
      e1.v = 1;
      const e2: MarkedTouch = new MarkedTouch();
      e2.v = 2;
      tracker.track(e1 as ESObject as TouchEvent);
      const id2: TouchEventId = tracker.track(e2 as ESObject as TouchEvent);
      const popped: TouchEvent = tracker.pop(id2);
      expect((popped as ESObject as MarkedTouch).v).assertEqual(2);
      const rePop: TouchEvent = tracker.pop(id2);
      expect(rePop).assertUndefined();
    })

    it('pop with untracked id returns undefined', 0, () => {
      const tracker: TouchEventTracker = new TouchEventTracker();
      const popped: TouchEvent = tracker.pop(TouchEventId.from(999));
      expect(popped).assertUndefined();
    })
  })
}
