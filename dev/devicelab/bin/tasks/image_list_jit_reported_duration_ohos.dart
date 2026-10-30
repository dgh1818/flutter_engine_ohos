// Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE_HW file.

import 'package:flutter_devicelab/framework/devices.dart';
import 'package:flutter_devicelab/framework/framework.dart';
import 'package:flutter_devicelab/framework/utils.dart';
import 'package:flutter_devicelab/tasks/perf_tests.dart';

Future<void> main() async {
  deviceOperatingSystem = DeviceOperatingSystem.ohos;
  await task(
    ReportedDurationTest(
      ReportedDurationTestFlavor.debug,
      '${flutterDirectory.path}/examples/image_list',
      'lib/main.dart',
      ohosBundleName('${flutterDirectory.path}/examples/image_list'),
      RegExp(r'===image_list=== all loaded in ([\d]+)ms.'),
    ).run,
  );
}
