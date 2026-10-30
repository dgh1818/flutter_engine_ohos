import { FlutterAbility, FlutterEngine, BasicMessageChannel, StringCodec, Reply, Log } from '@ohos/flutter_ohos';
import { GeneratedPluginRegistrant } from '../plugins/GeneratedPluginRegistrant';

const TAG = "EntryAbility";
const CHANNEL_NAME = "increment";
const PING = "ping";
const PONG = "pong";
const EMPTY_MESSAGE = "";
const EVENT_FLUTTER_INCREMENT = "flutter_increment";
const EVENT_SEND_PING = "send_ping";

export default class EntryAbility extends FlutterAbility {
  private channel: BasicMessageChannel<string> | null = null;
  private pingCallback: () => void = () => {
    this.channel?.send(PING);
  };

  configureFlutterEngine(flutterEngine: FlutterEngine) {
    super.configureFlutterEngine(flutterEngine)
    GeneratedPluginRegistrant.registerWith(flutterEngine)

    this.channel = new BasicMessageChannel<string>(
      flutterEngine.getDartExecutor(),
      CHANNEL_NAME,
      StringCodec.INSTANCE
    );

    this.channel.setMessageHandler({
      onMessage: (message: string, reply: Reply<string>) => {
        Log.i(TAG, "Received from Flutter: " + message);
        if (message === PONG) {
          this.context.eventHub.emit(EVENT_FLUTTER_INCREMENT);
        }
        reply.reply(EMPTY_MESSAGE);
      }
    });

    this.context.eventHub.on(EVENT_SEND_PING, this.pingCallback);
  }

  cleanUpFlutterEngine(flutterEngine: FlutterEngine) {
    this.context.eventHub.off(EVENT_SEND_PING, this.pingCallback);
    this.channel?.setMessageHandler(null);
    this.channel = null;
    super.cleanUpFlutterEngine(flutterEngine);
  }
}
