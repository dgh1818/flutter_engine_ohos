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
import { beforeAll, describe, expect, it } from '@ohos/hypium';
import { FlutterOverlaySurface } from '../../../../../main/ets/embedding/engine/FlutterOverlaySurface';

export default function FlutterOverlaySurfaceTest() {
  let flutterOverlaySurface: FlutterOverlaySurface;

  describe('FlutterOverlaySurfaceTest', () => {

    beforeAll(() => {
      const id = 2356;
      flutterOverlaySurface = new FlutterOverlaySurface(id);
    })

    it('getId', 0, async (done: Function) => {
      console.info("uitest: getId begin");
      const id = flutterOverlaySurface.getId();
      expect(id).not().assertNull();
      done();
    })
  })
}