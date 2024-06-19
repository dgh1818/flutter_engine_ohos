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
import { SurfaceTextureWrapper } from './SurfaceTextureWrapper';
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
    Log.i(TAG, "getTextureId: ", this.nextTextureId )
    return this.nextTextureId;
  }

  registerTexture(textureId: number): SurfaceTextureEntry {
    let surfaceTextureRegistryEntry = new SurfaceTextureRegistryEntry(this.nextTextureId);
    let surfaceId = this.flutterNapi.registerTexture(textureId);
    Log.i(TAG, "registerTexture, surfaceId=" + surfaceId);
    surfaceTextureRegistryEntry.setSurfaceId(surfaceId);
    return surfaceTextureRegistryEntry;
  }


  registerPixelMap(pixelMap: PixelMap): number {
    this.nextTextureId = this.nextTextureId + 1;
    this.flutterNapi.registerPixelMap(this.nextTextureId, pixelMap);
    return this.nextTextureId;
  }

  unregisterTexture(textureId: number): void {
    this.flutterNapi.unregisterTexture(textureId);
  }
  onTrimMemory(level: number) {
    throw new Error('Method not implemented.');
  }

  private getImageReceiver(): image.ImageReceiver {
    let receiver: image.ImageReceiver = image.createImageReceiver(640, 480, 4, 8);
    if (receiver !== undefined) {
      console.info('[camera test] ImageReceiver is ok');
    } else {
      console.info('[camera test] ImageReceiver is not ok');
    }
    return receiver;
  }
}

export class SurfaceTextureRegistryEntry implements SurfaceTextureEntry {
  private textureId: number = 0;
  private surfaceId: number = 0;
  private surfaceTextureWrapper: SurfaceTextureWrapper | null = null;
  private released: boolean = false;

  constructor(id: number) {
    this.textureId = id;
  }

  getImageReceiver(): image.ImageReceiver {
    return this.surfaceTextureWrapper!.getImageReceiver();
  }

  setImageReceiver(receiver: image.ImageReceiver): void {
    this.surfaceTextureWrapper = new SurfaceTextureWrapper(receiver);
  }

  getTextureId(): number {
    return this.textureId;
  }

  getSurfaceId(): number {
    return this.surfaceId;
  }

  setSurfaceId(surfaceId: number): void {
    this.surfaceId = surfaceId;
  }

  textureWrapper(): SurfaceTextureWrapper {
    return this.surfaceTextureWrapper!;
  }
  release() {
    throw new Error('Method not implemented.');
  }
}