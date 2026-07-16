import router from '@ohos.router';
import common from '@ohos.app.ability.common';

const EVENT_RETURN = "RETURN_FROM_COUNT_PAGE";

@Entry
@Component
struct CountPage {
  private context = getContext(this) as common.UIAbilityContext
  @State counter: number = 0

  aboutToAppear(): void {
    const params = router.getParams() as Record<string, Object>
    if (params != null && params['counter'] != null) {
      this.counter = params['counter'] as number
    }
  }

  incrementCounter(): void {
    this.counter++
  }

  returnToFlutterView(): void {
    this.context.eventHub.emit(EVENT_RETURN, this.counter)
    router.back()
  }

  onBackPress(): boolean {
    this.returnToFlutterView()
    return true
  }

  build() {
    Stack({ alignContent: Alignment.BottomEnd }) {
      Column() {
        Row() {
          Text('Platform View')
            .fontSize(20)
            .fontColor(Color.White)
            .fontWeight(FontWeight.Bold)
        }
        .width('100%')
        .height(56)
        .backgroundColor('#1976D2')
        .padding({ left: 16 })
        .alignItems(VerticalAlign.Center)

        Column() {
          Text(`Button tapped ${this.counter} time${this.counter == 1 ? '' : 's'}.`)
            .fontSize(17)
            .padding(18)

          Button('Continue in Flutter view')
            .type(ButtonType.Normal)
            .backgroundColor(Color.White)
            .fontColor(Color.Black)
            .borderRadius(4)
            .onClick(() => this.returnToFlutterView())
        }
        .layoutWeight(1)
        .justifyContent(FlexAlign.Center)
        .alignItems(HorizontalAlign.Center)

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
      .onClick(() => this.incrementCounter())
    }
    .width('100%')
    .height('100%')
  }
}
