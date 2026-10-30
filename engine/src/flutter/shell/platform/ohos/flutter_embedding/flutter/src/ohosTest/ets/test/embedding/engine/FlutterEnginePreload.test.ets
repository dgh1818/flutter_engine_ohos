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

import { afterAll, describe, expect, it } from '@ohos/hypium';
import FlutterEnginePreload from '../../../../../main/ets/embedding/engine/FlutterEnginePreload';
import FlutterEngine from '../../../../../main/ets/embedding/engine/FlutterEngine';
import FlutterEngineCache from '../../../../../main/ets/embedding/engine/FlutterEngineCache';
import FlutterAbilityLaunchConfigs from '../../../../../main/ets/embedding/ohos/FlutterAbilityLaunchConfigs';
import FlutterInjector from '../../../../../main/ets/FlutterInjector';
import FlutterLoader from '../../../../../main/ets/embedding/engine/loader/FlutterLoader';
import PlatformViewsController from '../../../../../main/ets/plugin/platform/PlatformViewsController';
import common from '@ohos.app.ability.common';
import { ViewportMetrics } from '../../../../../main/ets/view/FlutterView';
import {
  pierceNativeShellHolderId,
  restorePierceGlobals,
  seedPreloadFlutterNapi,
  stubPreloadNativeOps
} from './CoveragePierceHelpers';
import Any from '../../../../../main/ets/plugin/common/Any';

function ensureLoaderInitialized(context: common.Context): FlutterLoader {
  const loader: FlutterLoader = FlutterInjector.getInstance().getFlutterLoader();
  if (!loader.initialized) {
    loader.startInitialization(context);
    loader.ensureInitializationComplete([]);
  }
  return loader;
}

function buildCornerRadiusViewportMetrics(): ViewportMetrics {
  const viewportMetrics = new ViewportMetrics();
  viewportMetrics.physicalWidth = 1080;
  viewportMetrics.physicalHeight = 2340;
  viewportMetrics.devicePixelRatio = 3.0;
  viewportMetrics.physicalTouchSlop = 3.0;
  viewportMetrics.displayCornerRadiusTopLeft = 24;
  viewportMetrics.displayCornerRadiusTopRight = 24;
  viewportMetrics.displayCornerRadiusBottomRight = 8;
  viewportMetrics.displayCornerRadiusBottomLeft = 8;
  return viewportMetrics;
}

export default function FlutterEnginePreloadTest() {
  let context: common.UIAbilityContext;

  describe('FlutterEnginePreloadTest', () => {

    // This suite is the one that pierces global state (pierceNativeShellHolderId /
    // seedPreloadFlutterNapi pollute the static FlutterNapi.hasInit flag and the
    // FlutterInjector singleton); restore here so later suites see real code.
    afterAll(() => {
      restorePierceGlobals();
    })

    it('static_methods_exist', 0, () => {
      console.info("uitest: static_methods_exist begin");
      expect(typeof FlutterEnginePreload.preloadEngine).assertEqual('function');
      expect(typeof FlutterEnginePreload.predrawEngine).assertEqual('function');
      expect(typeof FlutterEnginePreload.preLoadFlutterNapi).assertEqual('function');
    })

    it('preloadEngine_with_empty_params', 0, async (done: Function) => {
      console.info("uitest: preloadEngine_with_empty_params begin");
      context = getContext() as common.UIAbilityContext;
      try {
        await FlutterEnginePreload.preloadEngine(context, {});
      } catch (e) {
        console.info("uitest: preloadEngine_with_empty_params error: " + JSON.stringify(e));
      }
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
        'viewport_metrics': viewportMetrics
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
      await result?.detachFromNativeAndReleaseResourcesAsync();
      done();
    })

    it('preLoadFlutterNapi_with_cached_engine_id', 0, async (done: Function) => {
      console.info("uitest: preLoadFlutterNapi_with_cached_engine_id begin");
      context = getContext() as common.UIAbilityContext;
      let getFlutterNapiCalls = 0;
      const fakeNapi: ESObject = {
        isAttached: (): boolean => true,
        dispatchPlatformMessage: (): void => {},
        runBundleAndSnapshotFromLibrary: (): void => {},
      };
      const mockEngine: Any = {
        getFlutterNapi: (): ESObject => {
          getFlutterNapiCalls++;
          return fakeNapi;
        },
      };
      const cache = FlutterEngineCache.getInstance();
      cache.put('test_cached_engine', mockEngine as Any as FlutterEngine);
      const params: Record<string, Object> = {
        'cached_engine_id': 'test_cached_engine'
      };
      const result = FlutterEnginePreload.preLoadFlutterNapi(context, 'test_bundle', params);
      expect(getFlutterNapiCalls).assertEqual(1);
      expect((result as ESObject) === fakeNapi).assertEqual(true);
      cache.remove('test_cached_engine');
      done();
    })

    it('preLoadFlutterNapi_with_cached_engine_group_id', 0, async (done: Function) => {
      console.info("uitest: preLoadFlutterNapi_with_cached_engine_group_id begin");
      context = getContext() as common.UIAbilityContext;
      const params: Record<string, Object> = {
        'cached_engine_group_id': 'test_cached_engine_group'
      };
      const result = FlutterEnginePreload.preLoadFlutterNapi(context, 'test_bundle', params);
      await result?.detachFromNativeAndReleaseResourcesAsync();
      done();
    })

    it('preloadEngine_with_corner_radius_viewport_metrics', 0, async (done: Function) => {
      console.info("uitest: preloadEngine_with_corner_radius_viewport_metrics begin");
      context = getContext() as common.UIAbilityContext;
      const viewportMetrics = new ViewportMetrics();
      viewportMetrics.physicalWidth = 1080;
      viewportMetrics.physicalHeight = 2340;
      viewportMetrics.devicePixelRatio = 3.0;
      viewportMetrics.physicalTouchSlop = 3.0;
      viewportMetrics.displayCornerRadiusTopLeft = 24;
      viewportMetrics.displayCornerRadiusTopRight = 24;
      viewportMetrics.displayCornerRadiusBottomRight = 0;
      viewportMetrics.displayCornerRadiusBottomLeft = 0;
      const params: Record<string, Object> = {
        'viewport_metrics': viewportMetrics
      };
      await FlutterEnginePreload.preloadEngine(context, params, 'corner_radii_preload_view');
      done();
    })

    it('predrawEngine_with_corner_radius_viewport_metrics', 0, async (done: Function) => {
      console.info("uitest: predrawEngine_with_corner_radius_viewport_metrics begin");
      context = getContext() as common.UIAbilityContext;
      const engine = new FlutterEngine(context, null, null, new PlatformViewsController());
      const viewportMetrics = buildCornerRadiusViewportMetrics();
      const params: Record<string, Object> = {
        'viewport_metrics': viewportMetrics
      };
      FlutterEnginePreload.predrawEngine(engine, params, 'corner_radii_predraw_view');
      engine.destroy();
      done();
    })

    it('preLoadFlutterNapi_with_initialized_loader', 0, async (done: Function) => {
      console.info("uitest: preLoadFlutterNapi_with_initialized_loader begin");
      context = getContext() as common.UIAbilityContext;
      const loader = ensureLoaderInitialized(context);
      const napi = FlutterEnginePreload.preLoadFlutterNapi(context, loader.findAppBundlePath(), {});
      expect(napi).not().assertNull();
      await napi?.detachFromNativeAndReleaseResourcesAsync();
      done();
    })

    it('preloadEngine_corner_radii_after_loader_init', 0, async (done: Function) => {
      console.info("uitest: preloadEngine_corner_radii_after_loader_init begin");
      context = getContext() as common.UIAbilityContext;
      ensureLoaderInitialized(context);
      const params: Record<string, Object> = {
        'viewport_metrics': buildCornerRadiusViewportMetrics()
      };
      await FlutterEnginePreload.preloadEngine(context, params, 'corner_radii_preload_init_view');
      done();
    })

    it('predrawEngine_repeated_corner_radius_updates', 0, async (done: Function) => {
      console.info("uitest: predrawEngine_repeated_corner_radius_updates begin");
      context = getContext() as common.UIAbilityContext;
      const engine = new FlutterEngine(context, null, null, new PlatformViewsController());
      const params: Record<string, Object> = {
        'viewport_metrics': buildCornerRadiusViewportMetrics()
      };
      FlutterEnginePreload.predrawEngine(engine, params, 'corner_radii_predraw_a');
      FlutterEnginePreload.predrawEngine(engine, params, 'corner_radii_predraw_b');
      engine.destroy();
      done();
    })

    it('preloadEngine_pierced_preload_napi_corner_radii', 0, async (done: Function) => {
      console.info("uitest: preloadEngine_pierced_preload_napi_corner_radii begin");
      context = getContext() as common.UIAbilityContext;
      try {
        ensureLoaderInitialized(context);
        const cachedEngineId = 'pierced_preload_engine_ut';
        const engine = new FlutterEngine(context, null, null, new PlatformViewsController());
        pierceNativeShellHolderId(engine.getFlutterNapi(), 1);
        stubPreloadNativeOps(engine.getFlutterNapi());
        FlutterEngineCache.getInstance().put(cachedEngineId, engine);
        const params: Record<string, Object> = {};
        params[FlutterAbilityLaunchConfigs.EXTRA_CACHED_ENGINE_ID] = cachedEngineId;
        params[FlutterAbilityLaunchConfigs.PRELOAD_VIEWPORT_METRICS_KEY] = buildCornerRadiusViewportMetrics();
        await FlutterEnginePreload.preloadEngine(context, params, 'pierced_preload_corner_view');
        FlutterEngineCache.getInstance().remove(cachedEngineId);
      } catch (e) {
        console.info("uitest: preloadEngine_pierced error: " + JSON.stringify(e));
      }
      done();
    })

    it('preloadEngine_mock_run_bundle_corner_radii', 0, async (done: Function) => {
      console.info("uitest: preloadEngine_mock_run_bundle_corner_radii begin");
      context = getContext() as common.UIAbilityContext;
      try {
        ensureLoaderInitialized(context);
        const loader = FlutterInjector.getInstance().getFlutterLoader();
        const napi = seedPreloadFlutterNapi(1);
        stubPreloadNativeOps(napi);
        const preloadParams: Record<string, Object> = {};
        preloadParams[FlutterAbilityLaunchConfigs.PRELOAD_VIEWPORT_METRICS_KEY] = buildCornerRadiusViewportMetrics();
        FlutterEnginePreload.preLoadFlutterNapi(context, loader.findAppBundlePath(), preloadParams);
        await FlutterEnginePreload.preloadEngine(context, preloadParams, 'mock_run_bundle_preload_view');
      } catch (e) {
        console.info("uitest: preloadEngine_mock_run_bundle error: " + JSON.stringify(e));
      }
      done();
    })

    it('preloadEngine_cached_engine_corner_radii', 0, async (done: Function) => {
      console.info("uitest: preloadEngine_cached_engine_corner_radii begin");
      context = getContext() as common.UIAbilityContext;
      try {
        ensureLoaderInitialized(context);
        const cachedEngineId = 'corner_radii_cached_engine_ut';
        const engine = new FlutterEngine(context, null, null, new PlatformViewsController());
        pierceNativeShellHolderId(engine.getFlutterNapi(), 1);
        stubPreloadNativeOps(engine.getFlutterNapi());
        FlutterEngineCache.getInstance().put(cachedEngineId, engine);
        const params: Record<string, Object> = {};
        params[FlutterAbilityLaunchConfigs.EXTRA_CACHED_ENGINE_ID] = cachedEngineId;
        params[FlutterAbilityLaunchConfigs.PRELOAD_VIEWPORT_METRICS_KEY] = buildCornerRadiusViewportMetrics();
        await FlutterEnginePreload.preloadEngine(context, params, 'cached_engine_preload_view');
        FlutterEngineCache.getInstance().remove(cachedEngineId);
      } catch (e) {
        console.info("uitest: preloadEngine_cached_engine error: " + JSON.stringify(e));
      }
      done();
    })

    it('predrawEngine static method exists', 0, () => {
      expect(typeof FlutterEnginePreload.predrawEngine).assertEqual('function');
    });
  })
}