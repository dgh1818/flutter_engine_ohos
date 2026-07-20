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

import { FlutterMutatorsStack } from '../../../../../../main/ets/embedding/engine/mutatorsstack/FlutterMutatorsStack';
import { describe, expect, it } from '@ohos/hypium';
export default function FlutterMutatorsStackTest() {
  describe('FlutterMutatorsStackTest', () => {
    it('TestPushClipPath', 0, async (done: Function) => {
      let flutterMutatorStack = new FlutterMutatorsStack();
      const width = 100;
      const height = 200;
      const command = 'M 10 10 L 90 90';
      flutterMutatorStack.pushClipPath(width, height, command);
      expect(flutterMutatorStack.getMutators().length).assertEqual(1);
      expect(flutterMutatorStack.getFinalClippingPaths().length).assertEqual(1);
      done();
    })

    it('TestPushClipRect1', 0, async (done: Function) => {
      let flutterMutatorStack = new FlutterMutatorsStack();
      const width = 100;
      const height = 200;
      const radius = 10;
      flutterMutatorStack.pushClipRect(width, height, radius);
      expect(flutterMutatorStack.getMutators().length).assertEqual(1);
      done();
    })

    it('TestPushClipRect2', 0, async (done: Function) => {
      let flutterMutatorStack = new FlutterMutatorsStack();
      const width = 100;
      const height = 200;
      const radius = 10;
      flutterMutatorStack.pushClipRect(width, height, radius);
      expect(flutterMutatorStack.getFinalClippingRects().length).assertEqual(1);
      done();
    })

    it('TestPushTransform1', 0, async (done: Function) => {
      const values = [1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1];
      let flutterMutatorStack = new FlutterMutatorsStack();
      flutterMutatorStack.pushTransform(values);
      expect(flutterMutatorStack.getMutators().length).assertEqual(1);
      done();
    })

    it('TestPushTransform2', 0, async (done: Function) => {
      const values = [1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1];
      let flutterMutatorStack = new FlutterMutatorsStack();
      flutterMutatorStack.pushTransform(values);
      expect(flutterMutatorStack.getMutators().length).assertEqual(1);
      done();
    })

    it('TestFlutterMutatorsStack1', 0, async (done: Function) => {
      let flutterMutatorStack = new FlutterMutatorsStack();
      expect(flutterMutatorStack.getMutators().length).assertEqual(0);
      done();
    })

    it('TestFlutterMutatorsStack2', 0, async (done: Function) => {
      let flutterMutatorStack = new FlutterMutatorsStack();
      expect(flutterMutatorStack.getFinalClippingPaths().length).assertEqual(0);
      done();
    })

    it('TestFlutterMutatorsStack3', 0, async (done: Function) => {
      let flutterMutatorStack = new FlutterMutatorsStack();
      expect(flutterMutatorStack.getFinalClippingRects().length).assertEqual(0);
      done();
    })
  })
}
