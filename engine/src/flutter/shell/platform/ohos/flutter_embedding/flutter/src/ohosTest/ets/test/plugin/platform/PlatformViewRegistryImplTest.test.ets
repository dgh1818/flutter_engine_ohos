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
import PlatformViewFactory from '../../../../../main/ets/plugin/platform/PlatformViewFactory';
import PlatformViewRegistryImpl from '../../../../../main/ets/plugin/platform/PlatformViewRegistryImpl';

const platformViewRegistryImpl: PlatformViewRegistryImpl = new PlatformViewRegistryImpl();

export default function PlatformViewRegistryImplTest() {
  describe('PlatformViewRegistryImplTest', () => {
    it('TestPlatformViewRegistryImplGetFactory', 0, async (done: Function) => {
      const platformViewFactory: PlatformViewFactory = platformViewRegistryImpl.getFactory('test');
      expect(platformViewFactory).not().assertNull();
      done();
    })

    it('TestPlatformViewRegistryImplRegisterViewFactory', 0, async (done: Function) => {
      const platformViewFactory: PlatformViewFactory = platformViewRegistryImpl.getFactory('test');
      expect(platformViewFactory).not().assertNull();
      const result = platformViewRegistryImpl.registerViewFactory('test', platformViewFactory);
      expect(result).assertTrue();
      done();
    })
  })
}