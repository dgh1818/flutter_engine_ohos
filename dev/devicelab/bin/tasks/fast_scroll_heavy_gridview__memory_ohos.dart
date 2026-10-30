// Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE_HW file.

import 'package:flutter_devicelab/framework/devices.dart';
import 'package:flutter_devicelab/framework/framework.dart';
import 'package:flutter_devicelab/framework/utils.dart';
import 'package:flutter_devicelab/tasks/perf_tests.dart';

class FastScrollHeavyGridViewOhosMemoryTest extends MemoryTest {
  FastScrollHeavyGridViewOhosMemoryTest()
    : super(
        '${flutterDirectory.path}/dev/benchmarks/macrobenchmarks',
        'test_memory/heavy_gridview.dart',
        ohosBundleName('${flutterDirectory.path}/dev/benchmarks/macrobenchmarks'),
      );

  @override
  OhosDevice? get device => super.device as OhosDevice?;

  @override
  int get iterationCount => 5;

  @override
  Future<void> useMemory() async {
    await launchApp();
    await recordStart();
    // OHOS `uiInput swipe` 5th arg is speed (px/s); derive it from a target duration (ms).
    const durationMs = 50;
    const int speed = (1500 - 50) * 1000 ~/ durationMs;
    const swipeArgs = '50 1500 50 50 $speed';
    await device!.hdcShellExec('uitest', <String>['uiInput', 'swipe', swipeArgs]);
    await Future<void>.delayed(const Duration(milliseconds: 1500));
    await device!.hdcShellExec('uitest', <String>['uiInput', 'swipe', swipeArgs]);
    await Future<void>.delayed(const Duration(milliseconds: 1500));
    await device!.hdcShellExec('uitest', <String>['uiInput', 'swipe', swipeArgs]);
    await Future<void>.delayed(const Duration(milliseconds: 1500));
    await recordEnd();
  }
}

Future<void> main() async {
  deviceOperatingSystem = DeviceOperatingSystem.ohos;
  await task(FastScrollHeavyGridViewOhosMemoryTest().run);
}
