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
import FlutterEnginePreload from '../../../../../main/ets/embedding/engine/FlutterEnginePreload';
import common from '@ohos.app.ability.common';
import { ViewportMetrics } from '../../../../../main/ets/view/FlutterView';

export default function FlutterEnginePreloadTest() {
  let context: common.UIAbilityContext;

  describe('FlutterEnginePreloadTest', () => {

    it('static_methods_exist', 0, () => {
      console.info("uitest: static_methods_exist begin");
      expect(typeof FlutterEnginePreload.preloadEngine).assertEqual('function');
      expect(typeof FlutterEnginePreload.predrawEngine).assertEqual('function');
      expect(typeof FlutterEnginePreload.preLoadFlutterNapi).assertEqual('function');
    })

    it('preloadEngine_with_empty_params', 0, async (done: Function) => {
      console.info("uitest: preloadEngine_with_empty_params begin");
      context = getContext() as common.UIAbilityContext;
      await FlutterEnginePreload.preloadEngine(context, {});
      done();
    })

    it('preloadEngine_with_viewport_metrics', 0, async (done: Function) => {
      console.info("uitest: preloadEngine_with_viewport_metrics begin");
      context = getContext() as common.UIAbilityContext;
      const viewportMetrics = new ViewportMetrics();
      viewportMetrics.physicalWidth = 1080;
      viewportMetrics.physicalHeight = 1920;
      viewportMetrics.devicePixelRatio = 3.0;
      const params: Record<string, Object> = {
        'preloadViewportMetrics': viewportMetrics
      };
      await FlutterEnginePreload.preloadEngine(context, params, 'test_view_id');
      done();
    })

    it('preLoadFlutterNapi_with_params', 0, async (done: Function) => {
      console.info("uitest: preLoadFlutterNapi_with_params begin");
      context = getContext() as common.UIAbilityContext;
      const params: Record<string, Object> = {
        'dartEntrypoint': 'main',
        'initialRoute': '/home',
        'dartEntrypointArgs': ['--debug']
      };
      const result = FlutterEnginePreload.preLoadFlutterNapi(context, 'test_bundle', params);
      done();
    })

    it('preLoadFlutterNapi_with_cached_engine_id', 0, async (done: Function) => {
      console.info("uitest: preLoadFlutterNapi_with_cached_engine_id begin");
      context = getContext() as common.UIAbilityContext;
      const params: Record<string, Object> = {
        'cachedEngineId': 'test_cached_engine'
      };
      const result = FlutterEnginePreload.preLoadFlutterNapi(context, 'test_bundle', params);
      done();
    })

    it('preLoadFlutterNapi_with_cached_engine_group_id', 0, async (done: Function) => {
      console.info("uitest: preLoadFlutterNapi_with_cached_engine_group_id begin");
      context = getContext() as common.UIAbilityContext;
      const params: Record<string, Object> = {
        'cachedEngineGroupId': 'test_cached_engine_group'
      };
      const result = FlutterEnginePreload.preLoadFlutterNapi(context, 'test_bundle', params);
      done();
    })
  })
}