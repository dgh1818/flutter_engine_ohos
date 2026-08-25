
import { FlutterAbility, FlutterEngine } from '@ohos/flutter_ohos';
import { GeneratedPluginRegistrant } from '../plugins/GeneratedPluginRegistrant';
import { NativeViewFactory } from './NativeViewFactory';
import { NativeInputFactory } from './NativeInputFactory';
import window from '@ohos.window';

export default class EntryAbility extends FlutterAbility {
  configureFlutterEngine(flutterEngine: FlutterEngine) {
    super.configureFlutterEngine(flutterEngine)
    GeneratedPluginRegistrant.registerWith(flutterEngine)
    flutterEngine.getPlatformViewsController()?.getRegistry().registerViewFactory(
      'split_view/native_view',
      new NativeViewFactory()
    );
    flutterEngine.getPlatformViewsController()?.getRegistry().registerViewFactory(
      'split_view/native_input',
      new NativeInputFactory()
    );
  }

  onWindowStageCreate(windowStage: window.WindowStage) {
    super.onWindowStageCreate(windowStage);

    const mainWindow = windowStage.getMainWindowSync();
    mainWindow.setWindowSystemBarEnable([]);
    mainWindow.setWindowLayoutFullScreen(true);
  }
}
