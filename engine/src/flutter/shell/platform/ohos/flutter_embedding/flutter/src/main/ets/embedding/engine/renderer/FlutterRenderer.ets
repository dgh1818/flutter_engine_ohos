/*
* Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd.
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

import image from '@ohos.multimedia.image';
import { BusinessError } from '@ohos.base';
import { SurfaceTextureEntry, TextureRegistry } from '../../../view/TextureRegistry';
import { FlutterAbility } from '../../ohos/FlutterAbility';
import FlutterNapi from '../FlutterNapi';
import Log from '../../../util/Log';

const TAG = "FlutterRenderer"

export class FlutterRenderer implements TextureRegistry {
  private nextTextureId: number = 0;
  private flutterNapi: FlutterNapi;

  constructor(flutterNapi: FlutterNapi) {
    this.flutterNapi = flutterNapi;
  }

  getTextureId(): number {
    this.nextTextureId = this.nextTextureId + 1;
    Log.i(TAG, "getTextureId: ", this.nextTextureId)
    return this.nextTextureId;
  }

  registerTexture(textureId: number): SurfaceTextureEntry {
    let surfaceTextureRegistryEntry = new SurfaceTextureRegistryEntry(this.nextTextureId);
    let surfaceId = this.flutterNapi.registerTexture(textureId);
    Log.i(TAG, "registerTexture, surfaceId=" + surfaceId);
    surfaceTextureRegistryEntry.setSurfaceId(surfaceId);
    let nativeWindowId = this.flutterNapi.getTextureNativeWindowId(textureId);
    surfaceTextureRegistryEntry.setNativeWindowId(nativeWindowId);
    return surfaceTextureRegistryEntry;
  }


  registerPixelMap(pixelMap: PixelMap): number {
    this.nextTextureId = this.nextTextureId + 1;
    this.flutterNapi.registerPixelMap(this.nextTextureId, pixelMap);
    return this.nextTextureId;
  }

  setTextureBackGroundPixelMap(textureId: number, pixelMap: PixelMap): void {
    this.flutterNapi.setTextureBackGroundPixelMap(textureId, pixelMap);
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