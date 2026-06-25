import { FlutterAbility, FlutterEngine } from '@ohos/flutter_ohos';
import { GeneratedPluginRegistrant } from '../plugins/GeneratedPluginRegistrant';
import { DummyPlatformViewFactory } from './DummyPlatformViewFactory';

export default class EntryAbility extends FlutterAbility {
  configureFlutterEngine(flutterEngine: FlutterEngine) {
    super.configureFlutterEngine(flutterEngine)
    GeneratedPluginRegistrant.registerWith(flutterEngine)
    flutterEngine.getPlatformViewsController()?.getRegistry().registerViewFactory(
      "benchmarks/platform_views_layout/DummyPlatformView",
      new DummyPlatformViewFactory()
    );
  }
}
