/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import {
  FlutterPlugin,
  FlutterPluginBinding,
  MethodChannel,
  MethodCallHandler,
  MethodCall,
  MethodResult,
  TextureRegistry
} from '@ohos/flutter_ohos';
import { image } from '@kit.ImageKit';

export default class TexturePlugin implements FlutterPlugin, MethodCallHandler {
  private channel: MethodChannel | null = null;
  private textureRegistry: TextureRegistry | null = null;
  private textureId: number = -1;
  private textureWidth: number = 0;
  private textureHeight: number = 0;
  private pixelMap: image.PixelMap | null = null;

  getUniqueClassName(): string {
    return 'TexturePlugin';
  }

  onAttachedToEngine(binding: FlutterPluginBinding): void {
    this.textureRegistry = binding.getTextureRegistry();
    this.channel = new MethodChannel(binding.getBinaryMessenger(), 'samples.flutter.io/texture');
    this.channel.setMethodCallHandler(this);
  }

  onDetachedFromEngine(binding: FlutterPluginBinding): void {
    this.channel?.setMethodCallHandler(null);
    this.channel = null;
    if (this.textureId >= 0) {
      this.textureRegistry?.unregisterTexture(this.textureId);
      this.textureId = -1;
    }
    this.pixelMap?.release();
    this.pixelMap = null;
    this.textureRegistry = null;
  }

  onMethodCall(call: MethodCall, result: MethodResult): void {
    if (call.method === 'create') {
      this.handleCreate(call, result);
    } else if (call.method === 'setColor') {
      this.handleSetColor(call, result);
    } else {
      result.notImplemented();
    }
  }

  private async handleCreate(call: MethodCall, result: MethodResult): Promise<void> {
    if (this.textureId >= 0) {
      result.error('Error', 'texture already created', null);
      return;
    }

    const args = call.args as Array<number>;
    if (!args || args.length !== 2) {
      result.error('Invalid args', 'Invalid create args', null);
      return;
    }

    this.textureWidth = args[0];
    this.textureHeight = args[1];

    if (this.textureWidth <= 0 || this.textureHeight <= 0) {
      result.error('Invalid args', 'Texture dimensions must be positive', null);
      return;
    }

    try {
      this.pixelMap = image.createPixelMapSync({
        size: { height: this.textureHeight, width: this.textureWidth },
        pixelFormat: image.PixelMapFormat.RGBA_8888,
        editable: true
      });
      await this.writeColor(0x05, 0x53, 0xb1);
      if (!this.textureRegistry || !this.pixelMap) {
        result.error('Error', 'Texture registry or pixelMap not available', null);
        return;
      }
      this.textureId = this.textureRegistry.registerPixelMap(this.pixelMap);
      result.success(this.textureId);
    } catch (e) {
      if (this.pixelMap) {
        this.pixelMap.release();
        this.pixelMap = null;
      }
      result.error('Error', 'Failed to create texture', e);
    }
  }

  private async handleSetColor(call: MethodCall, result: MethodResult): Promise<void> {
    if (this.textureId < 0 || !this.pixelMap) {
      result.error('Error', 'Texture not created', null);
      return;
    }

    const args = call.args as Array<number>;
    if (!args || args.length !== 3) {
      result.error('Invalid args', 'Invalid setColor args', null);
      return;
    }

    try {
      await this.writeColor(args[0], args[1], args[2]);
      if (!this.textureRegistry || !this.pixelMap) {
        result.error('Error', 'Texture registry or pixelMap not available', null);
        return;
      }
      this.textureRegistry.setTextureBackGroundPixelMap(this.textureId, this.pixelMap);
      result.success(null);
    } catch (e) {
      result.error('Error', 'Failed to set color', e);
    }
  }

  private async writeColor(r: number, g: number, b: number): Promise<void> {
    const bufferSize = this.textureWidth * this.textureHeight * 4;
    const buffer = new ArrayBuffer(bufferSize);
    const view = new Uint8Array(buffer);
    for (let i = 0; i < this.textureWidth * this.textureHeight; i++) {
      view[i * 4] = r;
      view[i * 4 + 1] = g;
      view[i * 4 + 2] = b;
      view[i * 4 + 3] = 255;
    }
    await this.pixelMap!.writeBufferToPixels(buffer);
  }
}
