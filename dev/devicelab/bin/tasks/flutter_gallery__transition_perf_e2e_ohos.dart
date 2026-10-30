// Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE_HW file.

// OHOS note: this task intentionally keeps the legacy drive-based
// transition methodology until OHOS supports the BuildTest flow.
// enableImpeller is passed explicitly here (the Android variant relies
// on engine defaults).

import 'package:flutter_devicelab/framework/devices.dart';
import 'package:flutter_devicelab/framework/framework.dart';
import 'package:flutter_devicelab/tasks/gallery.dart';

Future<void> main() async {
  deviceOperatingSystem = DeviceOperatingSystem.ohos;
  await task(createGalleryTransitionE2ETest(enableImpeller: true));
}
