/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, expect, it, Level } from '@ohos/hypium';
import common from '@ohos.app.ability.common';
import {
  FlutterPluginBinding,
  FlutterAssets,
} from '../../../../../../main/ets/embedding/engine/plugins/FlutterPlugin';
import FlutterEngine from '../../../../../../main/ets/embedding/engine/FlutterEngine';
import { BinaryMessenger, TaskQueue, TaskQueueOptions } from '../../../../../../main/ets/plugin/common/BinaryMessenger';
import PlatformViewFactory from '../../../../../../main/ets/plugin/platform/PlatformViewFactory';
import PlatformViewRegistry from '../../../../../../main/ets/plugin/platform/PlatformViewRegistry';
import { TextureRegistry } from '../../../../../../main/ets/view/TextureRegistry';
import StringCodec from '../../../../../../main/ets/plugin/common/StringCodec';
import PlatformView, { Params } from '../../../../../../main/ets/plugin/platform/PlatformView';
import Any from '../../../../../../main/ets/plugin/common/Any';

class FakeMessenger implements BinaryMessenger {
  makeBackgroundTaskQueue(_options?: TaskQueueOptions): TaskQueue {
    return {};
  }

  send(_channel: String, _message: ArrayBuffer | null): void {
  }

  setMessageHandler(_channel: String, _handler: Object | null): void {
  }
}

class FakeAssets implements FlutterAssets {
  getAssetFilePathByName(assetFileName: string, _bundleName?: string): string {
    return 'assets/' + assetFileName;
  }

  getAssetFilePathBySubpath(assetSubpath: string, _bundleName?: string): string {
    return 'assets/' + assetSubpath;
  }
}

class FakeTextures {
  createSurfaceTexture(): Object {
    return new Object();
  }

  getTextureId(): number {
    return 0;
  }

  registerTexture(_textureId: number): Object {
    return new Object();
  }

  registerSurfaceTexture(_receiver: Object): Object {
    return new Object();
  }

  registerPixelMap(_pixelMap: PixelMap): number {
    return 0;
  }

  setTextureBackGroundPixelMap(_textureId: number, _pixelMap: PixelMap): void {
  }

  setTextureBackGroundColor(_textureId: number, _color: number): void {
  }

  setTextureBufferSize(_textureId: number, _width: number, _height: number): void {
  }

  notifyTextureResizing(_textureId: number, _width: number, _height: number): void {
  }

  setExternalNativeImage(_textureId: number, _nativeImage: number): boolean {
    return false;
  }

  setExternalNativeImagePtr(_textureId: number, _nativeImage: bigint): boolean {
    return false;
  }

  resetExternalTexture(_textureId: number, _needSurfaceId: boolean): number {
    return 0;
  }

  unregisterTexture(_textureId: number): void {
  }

  onTrimMemory(_level: number): void {
  }
}

class StubView extends PlatformView {
  getView(): WrappedBuilder<[Params]> {
    return (new Object()) as WrappedBuilder<[Params]>;
  }

  dispose(): void {
  }
}

class StubFactory extends PlatformViewFactory {
  constructor() {
    super(StringCodec.INSTANCE);
  }

  create(_context: common.Context, _viewId: number, _args: Any): PlatformView {
    return new StubView();
  }
}

class RecordingRegistry implements PlatformViewRegistry {
  lastType: string = '';
  registerCount: number = 0;

  registerViewFactory(viewTypeId: string, _factory: PlatformViewFactory): boolean {
    this.lastType = viewTypeId;
    this.registerCount++;
    return true;
  }
}

function makeBinding(registry?: PlatformViewRegistry): FlutterPluginBinding {
  return new FlutterPluginBinding(
    getContext(),
    (new Object()) as FlutterEngine,
    new FakeMessenger(),
    new FakeAssets(),
    new FakeTextures() as ESObject as TextureRegistry,
    registry);
}

export default function flutterPluginBindingTest() {
  describe('FlutterPluginBinding.getters', () => {
    it('returns constructor arguments', Level.LEVEL1, () => {
      const messenger: FakeMessenger = new FakeMessenger();
      const assets: FakeAssets = new FakeAssets();
      const textures: FakeTextures = new FakeTextures();
      const registry: RecordingRegistry = new RecordingRegistry();
      const binding: FlutterPluginBinding = new FlutterPluginBinding(
        getContext(),
        (new Object()) as FlutterEngine,
        messenger,
        assets,
        textures as ESObject as TextureRegistry,
        registry);
      expect(binding.getApplicationContext()).assertEqual(getContext());
      expect(binding.getBinaryMessenger()).assertEqual(messenger);
      expect(binding.getFlutterAssets()).assertEqual(assets);
      expect(binding.getTextureRegistry()).assertEqual(textures as ESObject as TextureRegistry);
      expect(binding.getPlatformViewRegistry()).assertEqual(registry);
      expect(binding.getFlutterEngine()).not().assertNull();
    });

  });

  describe('FlutterPluginBinding.emptyRegistry', () => {
    it('omitted registry always rejects registerViewFactory', Level.LEVEL1, () => {
      const binding: FlutterPluginBinding = makeBinding();
      const registry: PlatformViewRegistry = binding.getPlatformViewRegistry();
      const accepted: boolean = registry.registerViewFactory('any', new StubFactory());
      expect(accepted).assertEqual(false);
    });

    it('custom registry is used when provided', Level.LEVEL1, () => {
      const recording: RecordingRegistry = new RecordingRegistry();
      const binding: FlutterPluginBinding = makeBinding(recording);
      const accepted: boolean = binding.getPlatformViewRegistry().registerViewFactory('custom', new StubFactory());
      expect(accepted).assertEqual(true);
      expect(recording.lastType).assertEqual('custom');
      expect(recording.registerCount).assertEqual(1);
    });
  });
}
