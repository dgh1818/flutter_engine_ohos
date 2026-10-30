import common from '@ohos.app.ability.common';
import { FlutterPage } from '@ohos/flutter_ohos'

let storage = LocalStorage.getShared()
const EVENT_FLUTTER_INCREMENT = "flutter_increment";
const EVENT_SEND_PING = "send_ping";

@Entry(storage)
@Component
struct Index {
  private context = getContext(this) as common.UIAbilityContext
  @LocalStorageLink('viewId') viewId: string = "";
  @State counter: number = 0;

  private flutterIncrementCallback: () => void = () => {
    this.counter++;
  };

  aboutToAppear() {
    this.context.eventHub.on(EVENT_FLUTTER_INCREMENT, this.flutterIncrementCallback);
  }

  aboutToDisappear() {
    this.context.eventHub.off(EVENT_FLUTTER_INCREMENT, this.flutterIncrementCallback);
  }

  build() {
    Column() {
      Column() {
        FlutterPage({ viewId: this.viewId })
      }
      .layoutWeight(1)
      .width('100%')

      Stack({ alignContent: Alignment.BottomEnd }) {
        Column() {
          Text(`Flutter button tapped ${this.counter} time${this.counter == 1 ? '' : 's'}.`)
            .fontSize(17)
            .layoutWeight(1)
            .textAlign(TextAlign.Center)
            .width('100%')

          Row() {
            Text('OHOS')
              .fontSize(30)
              .fontColor('#9E9E9E')
          }
          .width('100%')
          .padding({ left: 5, bottom: 15 })
        }
        .width('100%')
        .height('100%')

        Button({ type: ButtonType.Circle }) {
          Text('+')
            .fontSize(24)
            .fontColor(Color.Black)
        }
        .width(56)
        .height(56)
        .backgroundColor(Color.White)
        .margin({ right: 16, bottom: 16 })
        .shadow({ radius: 6, color: '#99000000', offsetY: 2 })
        .onClick(() => {
          this.context.eventHub.emit(EVENT_SEND_PING);
        })
      }
      .layoutWeight(1)
      .width('100%')
      .backgroundColor('#9E9E9E')
    }
    .width('100%')
    .height('100%')
  }
}
