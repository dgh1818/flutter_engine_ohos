// Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE_HW file.

import 'dart:math' as math;

import 'package:flutter_devicelab/framework/devices.dart';
import 'package:flutter_devicelab/framework/framework.dart';
import 'package:flutter_devicelab/framework/utils.dart';
import 'package:flutter_devicelab/tasks/perf_tests.dart';

class ComplexLayoutScrollPerfOhosMemoryTest extends MemoryTest {
  ComplexLayoutScrollPerfOhosMemoryTest()
    : super(
        '${flutterDirectory.path}/dev/benchmarks/complex_layout',
        'test_memory/scroll_perf.dart',
        ohosBundleName('${flutterDirectory.path}/dev/benchmarks/complex_layout'),
        requiresTapToStart: true,
      );

  @override
  OhosDevice? get device => super.device as OhosDevice?;

  @override
  math.Point<int> get tapLocation => const math.Point<int>(500, 500);
}

Future<void> main() async {
  deviceOperatingSystem = DeviceOperatingSystem.ohos;
  await task(ComplexLayoutScrollPerfOhosMemoryTest().run);
}
