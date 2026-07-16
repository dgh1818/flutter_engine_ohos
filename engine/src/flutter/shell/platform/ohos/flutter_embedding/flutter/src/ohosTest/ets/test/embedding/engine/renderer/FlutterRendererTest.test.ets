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
import { FlutterRenderer } from '../../../../../../main/ets/embedding/engine/renderer/FlutterRenderer';
import FlutterNapi from '../../../../../../main/ets/embedding/engine/FlutterNapi';
export default function FlutterRendererTest() {
  describe('FlutterRendererTest', () => {
    it('TestCreateSurfaceTexture', 0, async (done: Function) => {
      let flutterNapi: FlutterNapi = new FlutterNapi();
      let flutterRenderer = new FlutterRenderer(flutterNapi);
      let surfaceTextureEntry = flutterRenderer.createSurfaceTexture();
      expect(surfaceTextureEntry).not().assertNull();;
      done();
    })

    it('TestGetTextureId', 0, async (done: Function) => {
      let flutterNapi: FlutterNapi = new FlutterNapi();
      let flutterRenderer = new FlutterRenderer(flutterNapi);
      let textureId = flutterRenderer.getTextureId();
      expect(textureId == 0).assertFalse();
      done();
    })

    it('TestRegisterPixelMap', 0, async (done: Function) => {
      let flutterNapi: FlutterNapi = new FlutterNapi();
      let flutterRenderer = new FlutterRenderer(flutterNapi);
      const pixelMap = {} as PixelMap;
      let textureId = flutterRenderer.registerPixelMap(pixelMap);
      expect(textureId).not().assertNull();
      done();
    })

    it('TestRegisterPixelMapMore', 0, async (done: Function) => {
      let flutterNapi: FlutterNapi = new FlutterNapi();
      let flutterRenderer = new FlutterRenderer(flutterNapi);
      const pixelMap1 = {} as PixelMap;
      const pixelMap2 = {} as PixelMap;
      const textureId1 = flutterRenderer.registerPixelMap(pixelMap1);
      const textureId2 = flutterRenderer.registerPixelMap(pixelMap2);
      expect(textureId1).not().assertNull(); // 第一次调用
      expect(textureId2).not().assertNull(); // 第二次调用
      done();
    })
  })
}
