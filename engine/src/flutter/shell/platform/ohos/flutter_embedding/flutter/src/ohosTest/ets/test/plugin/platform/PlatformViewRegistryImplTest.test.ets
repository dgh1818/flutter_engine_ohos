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
import common from '@ohos.app.ability.common';
import Any from '../../../../../main/ets/plugin/common/Any';
import StringCodec from '../../../../../main/ets/plugin/common/StringCodec';
import PlatformView, { Params } from '../../../../../main/ets/plugin/platform/PlatformView';
import PlatformViewFactory from '../../../../../main/ets/plugin/platform/PlatformViewFactory';
import PlatformViewRegistryImpl from '../../../../../main/ets/plugin/platform/PlatformViewRegistryImpl';

class StubPlatformView extends PlatformView {
  getView(): WrappedBuilder<[Params]> {
    return (new Object()) as WrappedBuilder<[Params]>;
  }

  dispose(): void {
  }
}

class MockFactory extends PlatformViewFactory {
  constructor() {
    super(StringCodec.INSTANCE);
  }

  create(_context: common.Context, _viewId: number, _args: Any): PlatformView {
    return new StubPlatformView();
  }
}

const platformViewRegistryImpl: PlatformViewRegistryImpl = new PlatformViewRegistryImpl();

export default function PlatformViewRegistryImplTest() {
  describe('PlatformViewRegistryImplTest', () => {
    it('getFactory_unregisteredId_returnsUndefined', 0, () => {
      const factory = platformViewRegistryImpl.getFactory('missing');
      expect(factory).assertUndefined();
    })

    it('registerViewFactory_newId_returnsTrue', 0, () => {
      const result = platformViewRegistryImpl.registerViewFactory('reg', new MockFactory());
      expect(result).assertTrue();
    })

    it('TestPlatformViewRegistryImplRegisterViewFactoryTwiceReturnsFalse', 0, async (done: Function) => {
      const platformViewFactory: PlatformViewFactory = platformViewRegistryImpl.getFactory('test2');
      expect(platformViewFactory).not().assertNull();
      // First registration should succeed
      const result1 = platformViewRegistryImpl.registerViewFactory('test2', platformViewFactory);
      expect(result1).assertTrue();
      // Second registration of same type should fail (covers if branch at line 36)
      const result2 = platformViewRegistryImpl.registerViewFactory('test2', platformViewFactory);
      expect(result2).assertEqual(false);
      done();
    })

  })
}
