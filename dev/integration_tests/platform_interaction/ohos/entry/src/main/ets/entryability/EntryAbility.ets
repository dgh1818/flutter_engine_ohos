/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { BasicMessageChannel, FlutterAbility, FlutterEngine, StringCodec } from '@ohos/flutter_ohos';
import { GeneratedPluginRegistrant } from '../plugins/GeneratedPluginRegistrant';

export default class EntryAbility extends FlutterAbility {
  private channel: BasicMessageChannel<string> | null = null;

  configureFlutterEngine(flutterEngine: FlutterEngine) {
    super.configureFlutterEngine(flutterEngine)
    GeneratedPluginRegistrant.registerWith(flutterEngine)
    this.channel = new BasicMessageChannel<string>(
      flutterEngine.getDartExecutor(),
      'navigation-test',
      StringCodec.INSTANCE,
    );
  }

  cleanUpFlutterEngine(flutterEngine: FlutterEngine) {
    super.cleanUpFlutterEngine(flutterEngine)
    this.channel = null
  }

  popSystemNavigator(): boolean {
    this.channel?.send('ping');
    return true;
  }
}
