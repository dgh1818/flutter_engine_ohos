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

export interface TextureRegistry {

  getTextureId(): number;

  registerTexture(textureId: number): SurfaceTextureEntry;

  registerPixelMap(pixelMap: PixelMap): number;

  setTextureBackGroundPixelMap(textureId: number, pixelMap: PixelMap): void;

  setTextureBufferSize(textureId: number, width: number, height: number): void;

  setExternalNativeImage(textureId: number, native_image: number): boolean;

  resetExternalTexture(textureId: number, need_surfaceId: boolean): number;

  unregisterTexture(textureId: number): void;

  onTrimMemory(level: number): void;
}

export interface SurfaceTextureEntry {
  getTextureId(): number;

  getSurfaceId(): number;

  /*
  * This return value is OHNativeWindow* in native code.
  * Once converted to OHNativeWindow*, it can be used to create an EGLSurface or VkSurface for rendering.
  * This OHNativeWindow* needn't be released when invoking unregisterTexture.
  */
  getNativeWindowId(): number;

  release(): void;
}

export interface OnFrameConsumedListener {
  onFrameConsumed(): void;
}

export interface OnTrimMemoryListener {
  onTrimMemory(level: number): void;
}