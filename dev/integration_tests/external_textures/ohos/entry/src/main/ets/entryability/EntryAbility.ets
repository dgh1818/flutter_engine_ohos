import {
  FlutterAbility,
  FlutterEngine,
  MethodChannel,
  MethodCallHandler,
  MethodResult,
  MethodCall,
  TextureRegistry,
  SurfaceTextureRegistryEntry,
} from '@ohos/flutter_ohos';
import { GeneratedPluginRegistrant } from '../plugins/GeneratedPluginRegistrant';
import display from '@ohos.display';
import nativerender from 'libnativerender.so';

const TEXTURE_ID = 0;

class TextureHandler implements MethodCallHandler {
  private textureEntry: SurfaceTextureRegistryEntry | null = null;
  private textureRegistry: TextureRegistry | null = null;
  private producerTimer: number = -1;
  private consumerTimer: number = -1;
  private framesProduced: number = 0;
  private framesConsumed: number = 0;
  private startTime: number = 0;
  private endTime: number = 0;

  constructor(textureRegistry: TextureRegistry) {
    this.textureRegistry = textureRegistry;
    this.textureEntry = textureRegistry.registerTexture(TEXTURE_ID) as SurfaceTextureRegistryEntry;
  }

  onMethodCall(call: MethodCall, result: MethodResult): void {
    switch (call.method) {
      case 'start': {
        this.framesProduced = 0;
        this.framesConsumed = 0;
        this.startTime = Date.now();
        nativerender.resetFrameCount();
        const fps: number = call.args as number;
        if (this.producerTimer !== -1) {
          clearInterval(this.producerTimer);
          this.producerTimer = -1;
        }
        this.producerTimer = setInterval(() => {
          const nativeWindowPtr = this.textureEntry!.getNativeWindowPtr();
          if (nativeWindowPtr !== BigInt(0)) {
            nativerender.renderFrame(nativeWindowPtr);
            this.framesProduced++;
          }
        }, 1000 / fps);

        let refreshRate = 60;
        try {
          const displayInfo = display.getDefaultDisplaySync();
          refreshRate = displayInfo.refreshRate;
        } catch (e) {
        }
        if (this.consumerTimer !== -1) {
          clearInterval(this.consumerTimer);
          this.consumerTimer = -1;
        }
        this.consumerTimer = setInterval(() => {
          if (this.framesProduced > this.framesConsumed) {
            this.framesConsumed++;
          }
        }, 1000 / refreshRate);

        result.success(null);
        break;
      }
      case 'stop': {
        if (this.producerTimer !== -1) {
          clearInterval(this.producerTimer);
          this.producerTimer = -1;
        }
        if (this.consumerTimer !== -1) {
          clearInterval(this.consumerTimer);
          this.consumerTimer = -1;
        }
        this.endTime = Date.now();
        result.success(null);
        break;
      }
      case 'getProducedFrameRate': {
        const producedRate = this.framesProduced * 1000 / (this.endTime - this.startTime);
        result.success(producedRate);
        break;
      }
      case 'getConsumedFrameRate': {
        const consumedRate = this.framesConsumed * 1000 / (this.endTime - this.startTime);
        result.success(consumedRate);
        break;
      }
      default:
        result.notImplemented();
    }
  }

  destroy(): void {
    if (this.producerTimer !== -1) {
      clearInterval(this.producerTimer);
      this.producerTimer = -1;
    }
    if (this.consumerTimer !== -1) {
      clearInterval(this.consumerTimer);
      this.consumerTimer = -1;
    }
    this.textureRegistry?.unregisterTexture(TEXTURE_ID);
    this.textureEntry = null;
  }
}

export default class EntryAbility extends FlutterAbility {
  private handler: TextureHandler | null = null;

  configureFlutterEngine(flutterEngine: FlutterEngine) {
    super.configureFlutterEngine(flutterEngine);
    GeneratedPluginRegistrant.registerWith(flutterEngine);

    const textureRegistry: TextureRegistry = flutterEngine.getFlutterRenderer();
    this.handler = new TextureHandler(textureRegistry);
    const channel = new MethodChannel(flutterEngine.dartExecutor, 'texture');
    channel.setMethodCallHandler(this.handler);
  }

  onDestroy() {
    this.handler?.destroy();
    this.handler = null;
    super.onDestroy();
  }
}
