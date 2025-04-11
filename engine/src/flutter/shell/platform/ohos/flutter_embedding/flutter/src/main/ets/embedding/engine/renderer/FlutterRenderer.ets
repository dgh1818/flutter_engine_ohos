/*
* Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd. All rights reserved.
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE_KHZG file.
*/

import image from '@ohos.multimedia.image';
import { BusinessError } from '@ohos.base';
import { SurfaceTextureEntry, TextureRegistry } from '../../../view/TextureRegistry';
import { FlutterAbility } from '../../ohos/FlutterAbility';
import FlutterNapi from '../FlutterNapi';
import Log from '../../../util/Log';

const TAG = "FlutterRenderer"

export class FlutterRenderer implements TextureRegistry {
  private flutterNapi: FlutterNapi;
  private static globalTextureId: number = 0;

  constructor(flutterNapi: FlutterNapi) {
    this.flutterNapi = flutterNapi;
  }

  getTextureId(): number {
    let nextTextureId: number = FlutterRenderer.globalTextureId + 1;
    FlutterRenderer.globalTextureId = FlutterRenderer.globalTextureId + 1;
    Log.i(TAG, "getTextureId: " + nextTextureId)
    return nextTextureId;
  }

  registerTexture(textureId: number): SurfaceTextureEntry {
    let surfaceTextureRegistryEntry = new SurfaceTextureRegistryEntry(textureId);
    let surfaceId = this.flutterNapi.registerTexture(textureId);
    Log.i(TAG, "registerTexture, surfaceId=" + surfaceId);
    surfaceTextureRegistryEntry.setSurfaceId(surfaceId);
    let nativeWindowId = this.flutterNapi.getTextureNativeWindowId(textureId);
    surfaceTextureRegistryEntry.setNativeWindowId(nativeWindowId);
    return surfaceTextureRegistryEntry;
  }


  registerPixelMap(pixelMap: PixelMap): number {
    let nextTextureId: number = this.getTextureId();
    this.flutterNapi.registerPixelMap(nextTextureId, pixelMap);
    return nextTextureId;
  }

  setTextureBackGroundPixelMap(textureId: number, pixelMap: PixelMap): void {
    this.flutterNapi.setTextureBackGroundPixelMap(textureId, pixelMap);
  }

  /**
   * @deprecated since 3.7
   */
  setTextureBackGroundColor(textureId: number, color: number): void {
    this.flutterNapi.setTextureBackGroundColor(textureId, color);
  }

  setTextureBufferSize(textureId: number, width: number, height: number): void {
    this.flutterNapi.setTextureBufferSize(textureId, width, height);
  }

  notifyTextureResizing(textureId: number, width: number, height: number): void {
    this.flutterNapi.notifyTextureResizing(textureId, width, height);
  }

  setExternalNativeImage(textureId: number, native_image: number): boolean {
    return this.flutterNapi.setExternalNativeImage(textureId, native_image);
  }

  resetExternalTexture(textureId: number, need_surfaceId: boolean): number {
    return this.flutterNapi.resetExternalTexture(textureId, need_surfaceId);
  }

  unregisterTexture(textureId: number): void {
    this.flutterNapi.unregisterTexture(textureId);
  }

  onTrimMemory(level: number) {
    throw new Error('Method not implemented.');
  }
}

export class SurfaceTextureRegistryEntry implements SurfaceTextureEntry {
  private textureId: number = 0;
  private surfaceId: number = 0;
  private nativeWindowId: number = 0;
  private released: boolean = false;

  constructor(id: number) {
    this.textureId = id;
  }

  getTextureId(): number {
    return this.textureId;
  }

  getSurfaceId(): number {
    return this.surfaceId;
  }

  getNativeWindowId(): number {
    return this.nativeWindowId;
  }

  setSurfaceId(surfaceId: number): void {
    this.surfaceId = surfaceId;
  }

  setNativeWindowId(nativeWindowId: number): void {
    this.nativeWindowId = nativeWindowId;
  }

  release() {
    throw new Error('Method not implemented.');
  }
}