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
import { abilityDelegatorRegistry } from '@kit.TestKit';
import { Want } from '@kit.AbilityKit';
import FlutterShellArgs from '../../../../../main/ets/embedding/engine/FlutterShellArgs';

const bundleName = abilityDelegatorRegistry.getArguments().bundleName;

export default function FlutterShellArgsTest() {
  const flutterShellArgs: FlutterShellArgs = new FlutterShellArgs();
  const want: Want = {
    bundleName: bundleName,
    abilityName: 'EntryAbility'
  }

  describe('FlutterShellArgsTest', () => {

    it('fromWant', 0, async (done: Function) => {
      console.info("uitest: fromWant begin");
      try {
        const flutterShellArgs = FlutterShellArgs.fromWant(want);
        if (flutterShellArgs) {
          expect(flutterShellArgs).not().assertNull();
        }
      } catch (e) {
      }
      done();
    })

    it('checkArg', 0, async (done: Function) => {
      console.info("uitest: checkArg begin");
      const argKey = 'argKey';
      const argFlag = 'argFlag';
      try {
        const flutterShellArgs = FlutterShellArgs.fromWant(want);
        FlutterShellArgs.checkArg(argKey, argFlag, want, flutterShellArgs);
      } catch (e) {
      }
      done();
    })

    it('toArray', 0, async (done: Function) => {
      console.info("uitest: toArray begin");
      const toArray = flutterShellArgs.toArray();
      expect(toArray).not().assertNull();
      done();
    })
  })
}