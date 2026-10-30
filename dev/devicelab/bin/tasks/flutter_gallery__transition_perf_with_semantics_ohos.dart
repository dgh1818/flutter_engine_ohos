// Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE_HW file.

// OHOS note: runs a single semantics-enabled round; the delta_* and
// semantics_* metrics that the Android variant derives from a second,
// non-semantics round are not collected.

import 'package:flutter_devicelab/framework/devices.dart';
import 'package:flutter_devicelab/framework/framework.dart';
import 'package:flutter_devicelab/tasks/gallery.dart';

Future<void> main() async {
  deviceOperatingSystem = DeviceOperatingSystem.ohos;
  await task(createGalleryTransitionTest(semanticsEnabled: true));
}
