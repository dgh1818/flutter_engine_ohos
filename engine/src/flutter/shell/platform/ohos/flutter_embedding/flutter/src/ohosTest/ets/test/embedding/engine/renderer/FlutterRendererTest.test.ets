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
import { FlutterRenderer, SurfaceTextureRegistryEntry } from '../../../../../../main/ets/embedding/engine/renderer/FlutterRenderer';
import FlutterNapi from '../../../../../../main/ets/embedding/engine/FlutterNapi';

class CountingNapi extends FlutterNapi {
  unregisterCalls: number = 0;
  lastTextureId: number = -1;
  unregisterTexture(textureId: number): void {
    this.unregisterCalls += 1;
    this.lastTextureId = textureId;
  }
}

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

  describe('SurfaceTextureRegistryEntryReleaseTest', () => {
    it('TestReleaseWithoutNapiIsNoOp', 0, async (done: Function) => {
      let entry = new SurfaceTextureRegistryEntry(9);
      entry.release(); // 未注入 napi:静默 no-op,不应抛出异常
      expect(entry.getTextureId()).assertEqual(9);
      done();
    })

    it('TestReleaseForwardsUnregisterOnce', 0, async (done: Function) => {
      let napi = new CountingNapi();
      let entry = new SurfaceTextureRegistryEntry(7, napi);
      entry.release();
      expect(napi.unregisterCalls).assertEqual(1);
      expect(napi.lastTextureId).assertEqual(7);
      done();
    })

    it('TestSecondReleaseIsIdempotent', 0, async (done: Function) => {
      let napi = new CountingNapi();
      let entry = new SurfaceTextureRegistryEntry(8, napi);
      entry.release();
      entry.release(); // 第二次调用应被防重入挡住
      expect(napi.unregisterCalls).assertEqual(1);
      done();
    })

    it('TestReleaseWithRealNapiWithoutHolder', 0, async (done: Function) => {
      let napi = new FlutterNapi(); // nativeShellHolderId 为 null
      let entry = new SurfaceTextureRegistryEntry(11, napi);
      entry.release(); // 底层应安全 return,不应抛出异常
      expect(entry.getTextureId()).assertEqual(11);
      done();
    })

    it('TestGettersAfterRelease', 0, async (done: Function) => {
      let napi = new CountingNapi();
      let entry = new SurfaceTextureRegistryEntry(12, napi);
      entry.release();
      expect(entry.getTextureId()).assertEqual(12);
      expect(entry.getSurfaceId()).assertEqual(0);
      done();
    })
  })
}
