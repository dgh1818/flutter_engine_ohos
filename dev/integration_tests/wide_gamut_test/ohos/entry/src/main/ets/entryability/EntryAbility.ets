
import {
  FlutterAbility,
  FlutterEngine,
  MethodChannel,
  MethodCallHandler,
  MethodResult,
  MethodCall,
} from '@ohos/flutter_ohos';
import { GeneratedPluginRegistrant } from '../plugins/GeneratedPluginRegistrant';
import window from '@ohos.window';
import image from '@ohos.multimedia.image';
import common from '@ohos.app.ability.common';
import { hilog } from '@kit.PerformanceAnalysisKit';

const DOMAIN = 0x0001;
const TAG = 'ScreenshotHandler';

class ScreenshotHandler implements MethodCallHandler {
  private context: common.UIAbilityContext;

  constructor(context: common.UIAbilityContext) {
    this.context = context;
  }

  onMethodCall(call: MethodCall, result: MethodResult): void {
    if (call.method === 'test') {
      this.captureScreenshot(result);
    } else {
      result.notImplemented();
    }
  }

  private async captureScreenshot(result: MethodResult): Promise<void> {
    let pixelMap: image.PixelMap | null = null;
    try {
      await new Promise<void>((resolve: () => void) => setTimeout(resolve, 300));
      const win = await window.getLastWindow(this.context);
      pixelMap = await win.snapshot();
      const info = await pixelMap.getImageInfo();
      const width = info.size.width;
      const height = info.size.height;

      let formatStr = 'UNKNOWN';
      if (info.pixelFormat === image.PixelMapFormat.RGBA_8888) {
        formatStr = 'RGBA_8888';
      } else if (info.pixelFormat === image.PixelMapFormat.BGRA_8888) {
        formatStr = 'BGRA_8888';
      } else {
        await pixelMap.convertPixelFormat(image.PixelMapFormat.RGBA_8888);
        formatStr = 'RGBA_8888';
      }

      const bytesNumber = pixelMap.getPixelBytesNumber();
      const buffer = new ArrayBuffer(bytesNumber);
      await pixelMap.readPixelsToBuffer(buffer);

      const pixels = new Uint8Array(buffer);
      result.success([width, height, formatStr, pixels]);
    } catch (e) {
      hilog.error(DOMAIN, TAG, `screenshot error: ${e}`);
      result.error('screenshot_error', `${e}`, null);
    } finally {
      if (pixelMap !== null) {
        pixelMap.release();
      }
    }
  }
}

export default class EntryAbility extends FlutterAbility {
  configureFlutterEngine(flutterEngine: FlutterEngine) {
    super.configureFlutterEngine(flutterEngine)
    GeneratedPluginRegistrant.registerWith(flutterEngine)

    const channel = new MethodChannel(flutterEngine.dartExecutor, 'flutter/screenshot');
    channel.setMethodCallHandler(new ScreenshotHandler(this.context));
  }
}
