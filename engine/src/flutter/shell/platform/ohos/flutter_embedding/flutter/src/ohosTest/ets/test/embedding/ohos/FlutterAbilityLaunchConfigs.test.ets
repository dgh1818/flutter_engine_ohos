import { describe, it, expect, Level } from '@ohos/hypium';
import FlutterAbilityLaunchConfigs from '../../../../../main/ets/embedding/ohos/FlutterAbilityLaunchConfigs';

export default function flutterAbilityLaunchConfigsTest() {
  describe('FlutterAbilityLaunchConfigs.metadataKeys', () => {
    it('DART_ENTRYPOINT_META_DATA_KEY equals io.flutter.Entrypoint', Level.LEVEL0, () => {
      expect(FlutterAbilityLaunchConfigs.DART_ENTRYPOINT_META_DATA_KEY).assertEqual('io.flutter.Entrypoint');
    });

    it('DART_ENTRYPOINT_URI_META_DATA_KEY equals io.flutter.EntrypointUri', Level.LEVEL0, () => {
      expect(FlutterAbilityLaunchConfigs.DART_ENTRYPOINT_URI_META_DATA_KEY).assertEqual('io.flutter.EntrypointUri');
    });

    it('INITIAL_ROUTE_META_DATA_KEY equals io.flutter.InitialRoute', Level.LEVEL0, () => {
      expect(FlutterAbilityLaunchConfigs.INITIAL_ROUTE_META_DATA_KEY).assertEqual('io.flutter.InitialRoute');
    });

    it('SPLASH_SCREEN_META_DATA_KEY equals io.flutter.embedding.android.SplashScreenDrawable', Level.LEVEL0, () => {
      expect(FlutterAbilityLaunchConfigs.SPLASH_SCREEN_META_DATA_KEY).assertEqual('io.flutter.embedding.android.SplashScreenDrawable');
    });

    it('NORMAL_THEME_META_DATA_KEY equals io.flutter.embedding.android.NormalTheme', Level.LEVEL0, () => {
      expect(FlutterAbilityLaunchConfigs.NORMAL_THEME_META_DATA_KEY).assertEqual('io.flutter.embedding.android.NormalTheme');
    });

    it('HANDLE_DEEPLINKING_META_DATA_KEY equals flutter_deeplinking_enabled', Level.LEVEL0, () => {
      expect(FlutterAbilityLaunchConfigs.HANDLE_DEEPLINKING_META_DATA_KEY).assertEqual('flutter_deeplinking_enabled');
    });
  });

  describe('FlutterAbilityLaunchConfigs.wantExtras', () => {
    it('EXTRA_DART_ENTRYPOINT equals dart_entrypoint', Level.LEVEL0, () => {
      expect(FlutterAbilityLaunchConfigs.EXTRA_DART_ENTRYPOINT).assertEqual('dart_entrypoint');
    });

    it('EXTRA_DART_ENTRYPOINT_LIBRARY_URI equals dart_entrypoint_library_uri', Level.LEVEL0, () => {
      expect(FlutterAbilityLaunchConfigs.EXTRA_DART_ENTRYPOINT_LIBRARY_URI).assertEqual('dart_entrypoint_library_uri');
    });

    it('EXTRA_INITIAL_ROUTE equals route', Level.LEVEL0, () => {
      expect(FlutterAbilityLaunchConfigs.EXTRA_INITIAL_ROUTE).assertEqual('route');
    });

    it('EXTRA_BACKGROUND_MODE equals background_mode', Level.LEVEL0, () => {
      expect(FlutterAbilityLaunchConfigs.EXTRA_BACKGROUND_MODE).assertEqual('background_mode');
    });

    it('EXTRA_CACHED_ENGINE_ID equals cached_engine_id', Level.LEVEL0, () => {
      expect(FlutterAbilityLaunchConfigs.EXTRA_CACHED_ENGINE_ID).assertEqual('cached_engine_id');
    });

    it('EXTRA_DART_ENTRYPOINT_ARGS equals dart_entrypoint_args', Level.LEVEL0, () => {
      expect(FlutterAbilityLaunchConfigs.EXTRA_DART_ENTRYPOINT_ARGS).assertEqual('dart_entrypoint_args');
    });

    it('EXTRA_CACHED_ENGINE_GROUP_ID equals cached_engine_group_id', Level.LEVEL0, () => {
      expect(FlutterAbilityLaunchConfigs.EXTRA_CACHED_ENGINE_GROUP_ID).assertEqual('cached_engine_group_id');
    });

    it('EXTRA_DESTROY_ENGINE_WITH_ACTIVITY equals destroy_engine_with_activity', Level.LEVEL0, () => {
      expect(FlutterAbilityLaunchConfigs.EXTRA_DESTROY_ENGINE_WITH_ACTIVITY).assertEqual('destroy_engine_with_activity');
    });

    it('EXTRA_ENABLE_STATE_RESTORATION equals enable_state_restoration', Level.LEVEL0, () => {
      expect(FlutterAbilityLaunchConfigs.EXTRA_ENABLE_STATE_RESTORATION).assertEqual('enable_state_restoration');
    });
  });

  describe('FlutterAbilityLaunchConfigs.defaults', () => {
    it('DEFAULT_DART_ENTRYPOINT equals main', Level.LEVEL0, () => {
      expect(FlutterAbilityLaunchConfigs.DEFAULT_DART_ENTRYPOINT).assertEqual('main');
    });

    it('DEFAULT_INITIAL_ROUTE equals slash', Level.LEVEL0, () => {
      expect(FlutterAbilityLaunchConfigs.DEFAULT_INITIAL_ROUTE).assertEqual('/');
    });

    it('DEFAULT_BACKGROUND_MODE equals 0 opaque', Level.LEVEL0, () => {
      expect(FlutterAbilityLaunchConfigs.DEFAULT_BACKGROUND_MODE).assertEqual(0);
    });
  });

  describe('FlutterAbilityLaunchConfigs.preload', () => {
    it('PRELOAD_VIEWPORT_METRICS_KEY equals viewport_metrics', Level.LEVEL0, () => {
      expect(FlutterAbilityLaunchConfigs.PRELOAD_VIEWPORT_METRICS_KEY).assertEqual('viewport_metrics');
    });
  });

  describe('FlutterAbilityLaunchConfigs.BackgroundMode', () => {
    it('DEFAULT_BACKGROUND_MODE is 0 representing opaque', Level.LEVEL0, () => {
      expect(FlutterAbilityLaunchConfigs.DEFAULT_BACKGROUND_MODE).assertEqual(0);
    });

    it('opaque and transparent have different values', Level.LEVEL1, () => {
      const opaque: number = FlutterAbilityLaunchConfigs.DEFAULT_BACKGROUND_MODE;
      const transparent: number = opaque + 1;
      expect(opaque !== transparent).assertEqual(true);
    });

    it('opaque is falsy and transparent is truthy', Level.LEVEL1, () => {
      const opaque: number = FlutterAbilityLaunchConfigs.DEFAULT_BACKGROUND_MODE;
      expect(!opaque).assertEqual(true);
    });
  });
}
