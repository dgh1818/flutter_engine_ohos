// Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE_HW file.

import 'package:flutter_devicelab/framework/devices.dart';
import 'package:flutter_devicelab/framework/framework.dart';
import 'package:flutter_devicelab/framework/utils.dart';
import 'package:flutter_devicelab/tasks/perf_tests.dart';

Future<void> main() async {
  deviceOperatingSystem = DeviceOperatingSystem.ohos;

  final projectPath = '${flutterDirectory.path}/examples/image_list';
  final String bundleName = ohosBundleName(projectPath);

  await task(
    ReportedDurationTest(
      ReportedDurationTestFlavor.release,
      projectPath,
      'lib/main.dart',
      bundleName,
      RegExp(r'===image_list=== all loaded in ([\d]+)ms.'),
    ).run,
  );
}
