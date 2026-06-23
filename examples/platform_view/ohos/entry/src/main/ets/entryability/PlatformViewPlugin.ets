import { FlutterPlugin, FlutterPluginBinding, MethodCallHandler, MethodChannel, MethodCall, MethodResult, AbilityAware, AbilityPluginBinding, Log } from '@ohos/flutter_ohos';
import router from '@ohos.router';
import UIAbility from '@ohos.app.ability.UIAbility';
import { BusinessError } from '@ohos.base';

const TAG = "PlatformViewPlugin";
const CHANNEL = "samples.flutter.io/platform_view";
const METHOD_SWITCH_VIEW = "switchView";
const EVENT_RETURN = "RETURN_FROM_COUNT_PAGE";

export default class PlatformViewPlugin implements FlutterPlugin, MethodCallHandler, AbilityAware {
  private channel: MethodChannel | null = null;
  private pendingResult: MethodResult | null = null;
  private ability: UIAbility | null = null;
  private eventCallback: ((counter: number) => void) | null = null;

  getUniqueClassName(): string {
    return 'PlatformViewPlugin';
  }

  onAttachedToEngine(binding: FlutterPluginBinding): void {
    this.channel = new MethodChannel(binding.getBinaryMessenger(), CHANNEL);
    this.channel.setMethodCallHandler(this);
  }

  onDetachedFromEngine(binding: FlutterPluginBinding): void {
    if (this.channel != null) {
      this.channel.setMethodCallHandler(null);
      this.channel = null;
    }
  }

  onAttachedToAbility(binding: AbilityPluginBinding): void {
    this.ability = binding.getAbility();
    this.eventCallback = (counter: number) => {
      Log.i(TAG, 'Received return event with counter=' + counter);
      if (this.pendingResult != null) {
        this.pendingResult.success(counter);
        this.pendingResult = null;
      }
    };
    this.ability.context.eventHub.on(EVENT_RETURN, this.eventCallback);
  }

  onDetachedFromAbility(): void {
    if (this.pendingResult != null) {
      this.pendingResult.error('ACTIVITY_DESTROYED', 'Ability was destroyed before result was returned', null);
      this.pendingResult = null;
    }
    if (this.ability != null && this.eventCallback != null) {
      this.ability.context.eventHub.off(EVENT_RETURN, this.eventCallback);
      this.eventCallback = null;
      this.ability = null;
    }
  }

  onMethodCall(call: MethodCall, result: MethodResult): void {
    if (call.method === METHOD_SWITCH_VIEW) {
      this.pendingResult = result;
      const counter = call.args as number;
      router.pushUrl({
        url: 'pages/CountPage',
        params: { counter: counter }
      }).catch((error: BusinessError) => {
        Log.e(TAG, 'Failed to launch CountPage: ' + error.message);
        if (this.pendingResult != null) {
          this.pendingResult.error('LAUNCH_FAILURE', error.message, null);
          this.pendingResult = null;
        }
      });
    } else {
      result.notImplemented();
    }
  }
}
