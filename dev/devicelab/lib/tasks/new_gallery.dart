// Copyright 2014 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import '../framework/utils.dart';
import 'perf_tests.dart';

class NewGalleryPerfTest extends PerfTest {
  NewGalleryPerfTest({
    String timelineFileName = 'transitions',
    super.dartDefine = '',
    super.enableImpeller,
    super.timeoutSeconds,
    super.forceOpenGLES,
    super.createPlatforms = const <String>['android', 'ios', 'web'],
  }) : super(
         '${flutterDirectory.path}/dev/integration_tests/new_gallery',
         'test_driver/transitions_perf.dart',
         timelineFileName,
         enableMergedPlatformThread: true,
       );
}
