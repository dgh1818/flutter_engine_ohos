/**
 * Copyright 2014 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

import {
  AbilityAware,
  AbilityPluginBinding,
  BinaryMessenger,
  FlutterPlugin,
  FlutterPluginBinding,
  Log,
  MethodCall,
  MethodCallHandler,
  MethodChannel,
  MethodResult,
} from '@ohos/flutter_ohos';
import UIAbility from '@ohos.app.ability.UIAbility';
import window from '@ohos.window';
import image from '@ohos.multimedia.image';

const TAG = "IntegrationTestPlugin";

export default class IntegrationTestPlugin implements MethodCallHandler, FlutterPlugin, AbilityAware {
  private static CHANNEL: string = "plugins.flutter.io/integration_test";
  private methodChannel: MethodChannel | null = null;
  private flutterAbility: UIAbility | null = null;
  private static testResultsSetter: Set<Map<String, String>> =
    new Set<Map<String, String>>();

  getUniqueClassName(): string {
    return 'IntegrationTestPlugin';
  }

  private attachedToEngine(unusedApplicationContext: Context, messenger: BinaryMessenger): void {
    Log.d(TAG, 'attachedToEngine');
    this.methodChannel = new MethodChannel(messenger, IntegrationTestPlugin.CHANNEL);
    this.methodChannel.setMethodCallHandler(this);
  }

  onAttachedToEngine(binding: FlutterPluginBinding) {
    this.attachedToEngine(binding.getApplicationContext(), binding.getBinaryMessenger());
  }

  onMethodCall(call: MethodCall, result: MethodResult) {
    Log.d(TAG, 'onMethodCall: ' + call.method);
    switch (call.method) {
      case "allTestsFinished":
        IntegrationTestPlugin.testResultsSetter.add(call.argument("results"));
        result.success(null);
        return;
      case "captureScreenshot":
        this.captureScreenshot(result);
        return;
      case "convertFlutterSurfaceToImage":
      case "revertFlutterImage":
        result.success(null);
        return;
      default:
        result.notImplemented();
    }
  }

  private async captureScreenshot(result: MethodResult): Promise<void> {
    try {
      const ability: UIAbility | null = this.flutterAbility;
      if (!ability || !ability.context) {
        result.error('Could not capture screenshot', 'Ability or context is null', null);
        return;
      }
      const win: window.Window = await window.getLastWindow(ability.context);
      const pixelMap: image.PixelMap = await win.snapshot();
      try {
        const packer: image.ImagePacker = image.createImagePacker();
        try {
          const buffer: ArrayBuffer = await packer.packToData(pixelMap, {
            format: 'image/png',
            quality: 100,
          });
          result.success(new Uint8Array(buffer));
        } finally {
          packer.release().catch((e: Object): void => Log.e(TAG, 'packer release failed: ' + JSON.stringify(e)));
        }
      } finally {
        pixelMap.release().catch((e: Object): void => Log.e(TAG, 'pixelMap release failed: ' + JSON.stringify(e)));
      }
    } catch (err) {
      result.error('Could not capture screenshot', JSON.stringify(err), null);
    }
  }

  onDetachedFromEngine(binding: FlutterPluginBinding) {
    Log.d(TAG, 'onDetachedFromEngine');
    this.methodChannel?.setMethodCallHandler(null);
    this.methodChannel = null;
  }

  onAttachedToAbility(binding: AbilityPluginBinding) {
    Log.d(TAG, 'onAttachedToAbility');
    this.flutterAbility = binding.getAbility();
  }

  onDetachedFromAbility() {
    Log.d(TAG, 'onDetachedFromAbility');
    this.flutterAbility = null;
  }
}
