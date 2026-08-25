/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import PlatformView, { Params } from '@ohos/flutter_ohos/src/main/ets/plugin/platform/PlatformView';
import common from '@ohos.app.ability.common';

/**
 * Tracks whether the user has touched the native TextInput inside a
 * PlatformView. The engine calls shouldTriggerTextInputHandoffOnTouch()
 * on every DOWN gesture; only when this flag is true do we report that
 * the framework should hand off text-input ownership to the native side.
 */
const editorTouchDownByViewId: Map<number, boolean> = new Map();

/**
 * Custom dialog component shown when the native button is tapped.
 */
@CustomDialog
struct NativeCustomDialog {
  controller: CustomDialogController;

  build() {
    Column() {
      Text('Native Dialog')
        .fontSize(24)
        .fontWeight(FontWeight.Bold)
        .margin({ bottom: 12 })
      Text('This is an OHOS native dialog')
        .fontSize(16)
        .margin({ bottom: 20 })
      Button('Close')
        .width(120)
        .height(40)
        .backgroundColor(0xFF2196F3)
        .fontColor(Color.White)
        .onClick(() => {
          this.controller.close();
        })
    }
    .width(280)
    .padding(24)
    .backgroundColor(Color.White)
    .borderRadius(16)
  }
}

/**
 * ArkUI component rendered as a native platform view inside Flutter.
 *
 * Displays a coloured container with a label and a button that opens a
 * native OHOS custom dialog, so that integration tests can verify both
 * the native view embedding and native dialog behaviour in split view.
 */
@Component
struct NativeViewComponent {
  @State inputText: string = '';
  @Prop params: Params;
  private dialogController: CustomDialogController = new CustomDialogController({
    builder: NativeCustomDialog(),
    autoCancel: true,
    alignment: DialogAlignment.Center,
    offset: { dx: 0, dy: 0 },
    customStyle: true,
  });

  build() {
    Column() {
      Text('Native OHOS View')
        .fontSize(28)
        .fontColor(Color.White)
        .fontWeight(FontWeight.Bold)
      Text('Embedded via OhosView')
        .fontSize(16)
        .fontColor(Color.White)
        .margin({ top: 8 })
      TextInput({ text: this.inputText, placeholder: 'Native input in view' })
        .onTouch(() => {
          const viewId = (this.params.platformView as NativeViewPage).getViewId();
          editorTouchDownByViewId.set(viewId, true);
        })
        .width('80%')
        .height(44)
        .margin({ top: 16 })
        .fontSize(16)
        .backgroundColor(Color.White)
        .borderRadius(8)
        .id('native_view_text_input')
        .onChange((value: string) => {
          this.inputText = value;
        })
      Button('Show Native Dialog')
        .width(220)
        .height(44)
        .margin({ top: 20 })
        .backgroundColor(0xFF2196F3)
        .fontColor(Color.White)
        .fontSize(16)
        .onClick(() => {
          this.dialogController.open();
        })
    }
    .width('100%')
    .height('100%')
    .justifyContent(FlexAlign.Center)
    .backgroundColor(0xFF4CAF50)
  }
}

@Builder
function nativeViewBuilder(params: Params) {
  NativeViewComponent({ params: params });
}

/**
 * ArkUI component that contains a native TextInput, rendered as a
 * platform view inside Flutter via OhosView with viewType
 * `split_view/native_input`.
 *
 * Used by integration tests to verify that the native input field
 * receives focus and the soft keyboard appears when tapped.
 */
@Component
struct NativeInputComponent {
  @State inputText: string = '';
  @Prop params: Params;

  build() {
    Column() {
      TextInput({ text: this.inputText, placeholder: 'Native OHOS input' })
        .onTouch(() => {
          const viewId = (this.params.platformView as NativeInputPage).getViewId();
          editorTouchDownByViewId.set(viewId, true);
        })
        .width('100%')
        .height(48)
        .fontSize(16)
        .backgroundColor(Color.White)
        .borderRadius(8)
        .id('native_home_text_input')
        .onChange((value: string) => {
          this.inputText = value;
        })
    }
    .width('100%')
    .height('100%')
    .justifyContent(FlexAlign.Center)
    .backgroundColor(0xFFFF9800)
  }
}

@Builder
function nativeInputBuilder(params: Params) {
  NativeInputComponent({ params: params });
}

/**
 * PlatformView wrapper that exposes [NativeViewComponent] to Flutter
 * through the `OhosView` widget with viewType `split_view/native_view`.
 */
export class NativeViewPage extends PlatformView {
  private readonly viewId: number;

  constructor(context: common.Context, viewId: number, args: Object) {
    super()
    this.viewId = viewId;
  }

  /**
   * Returns the view ID assigned by the engine.
   * Used by the ArkUI component to mark editor touch-down events.
   */
  getViewId(): number {
    return this.viewId;
  }

  /**
   * This PlatformView contains a native TextInput, so it should
   * participate in the Flutter->native text-input handoff path.
   */
  shouldParticipateInTextInputHandoff(): boolean {
    return true;
  }

  /**
   * Only trigger the handoff when the user actually touched the
   * native TextInput inside this view. The flag is set by the
   * TextInput's .onTouch callback and consumed here.
   */
  shouldTriggerTextInputHandoffOnTouch(): boolean {
    const shouldTrigger = editorTouchDownByViewId.get(this.viewId) ?? false;
    editorTouchDownByViewId.delete(this.viewId);
    return shouldTrigger;
  }

  getView(): WrappedBuilder<[Params]> {
    return new WrappedBuilder(nativeViewBuilder)
  }

  dispose(): void {
    editorTouchDownByViewId.delete(this.viewId);
  }
}

/**
 * PlatformView wrapper that exposes [NativeInputComponent] to Flutter
 * through the `OhosView` widget with viewType `split_view/native_input`.
 *
 * This is a lightweight view — no MethodChannel is needed. It simply
 * renders a native ArkUI TextInput so integration tests can verify
 * native input focus and soft keyboard behaviour.
 */
export class NativeInputPage extends PlatformView {
  private readonly viewId: number;

  constructor(context: common.Context, viewId: number, args: Object) {
    super()
    this.viewId = viewId;
  }

  /**
   * Returns the view ID assigned by the engine.
   * Used by the ArkUI component to mark editor touch-down events.
   */
  getViewId(): number {
    return this.viewId;
  }

  /**
   * This PlatformView contains a native TextInput, so it should
   * participate in the Flutter->native text-input handoff path.
   */
  shouldParticipateInTextInputHandoff(): boolean {
    return true;
  }

  /**
   * Only trigger the handoff when the user actually touched the
   * native TextInput inside this view. The flag is set by the
   * TextInput's .onTouch callback and consumed here.
   */
  shouldTriggerTextInputHandoffOnTouch(): boolean {
    const shouldTrigger = editorTouchDownByViewId.get(this.viewId) ?? false;
    editorTouchDownByViewId.delete(this.viewId);
    return shouldTrigger;
  }

  getView(): WrappedBuilder<[Params]> {
    return new WrappedBuilder(nativeInputBuilder)
  }

  dispose(): void {
    editorTouchDownByViewId.delete(this.viewId);
  }
}
