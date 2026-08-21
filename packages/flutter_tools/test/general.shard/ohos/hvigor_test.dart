// Copyright 2014 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:flutter_tools/src/base/file_system.dart';
import 'package:flutter_tools/src/build_info.dart';
import 'package:flutter_tools/src/convert.dart';
import 'package:flutter_tools/src/globals.dart' as globals;
import 'package:flutter_tools/src/ohos/hvigor.dart';
import 'package:flutter_tools/src/project.dart';
import 'package:test/fake.dart';

import '../../src/common.dart';
import '../../src/context.dart';

void main() {
  group('setHcppEnableFlag', () {
    testUsingContext('updates an existing enable_ohos_hybrid_composition entry', () async {
      final _BuildInfoHarness harness = _BuildInfoHarness.create();
      harness.writeBuildInfo(<Map<String, Object?>>[
        <String, Object?>{'name': 'enable_impeller', 'value': 'true'},
        <String, Object?>{'name': 'enable_ohos_hybrid_composition', 'value': 'false'},
      ]);

      await setHcppEnableFlag(harness.project, _ohosBuildInfo(enableHcpp: true));

      expect(harness.flags(), <String, Object?>{
        'enable_impeller': 'true',
        'enable_ohos_hybrid_composition': 'true',
      });
    });

    testUsingContext('appends enable_ohos_hybrid_composition when the entry is missing', () async {
      final _BuildInfoHarness harness = _BuildInfoHarness.create();
      harness.writeBuildInfo(<Map<String, Object?>>[
        <String, Object?>{'name': 'enable_impeller', 'value': 'true'},
      ]);

      await setHcppEnableFlag(harness.project, _ohosBuildInfo(enableHcpp: true));

      expect(harness.flags(), <String, Object?>{
        'enable_impeller': 'true',
        'enable_ohos_hybrid_composition': 'true',
      });
    });

    testUsingContext('writes false when enableHcppFlag is false', () async {
      final _BuildInfoHarness harness = _BuildInfoHarness.create();
      harness.writeBuildInfo(<Map<String, Object?>>[
        <String, Object?>{'name': 'enable_ohos_hybrid_composition', 'value': 'true'},
      ]);

      await setHcppEnableFlag(harness.project, _ohosBuildInfo(enableHcpp: false));

      expect(harness.flags(), <String, Object?>{'enable_ohos_hybrid_composition': 'false'});
    });

    testUsingContext('throws when buildinfo.json5 is missing', () async {
      final _BuildInfoHarness harness = _BuildInfoHarness.create();

      await expectLater(
        setHcppEnableFlag(harness.project, _ohosBuildInfo(enableHcpp: true)),
        throwsA(
          predicate<Exception>(
            (Exception e) => e.toString().contains('Failed to find buildinfo.json5'),
          ),
        ),
      );
    });
  });

  group('setImpellerEnableFlag', () {
    testUsingContext('updates an existing enable_impeller entry', () async {
      final _BuildInfoHarness harness = _BuildInfoHarness.create();
      harness.writeBuildInfo(<Map<String, Object?>>[
        <String, Object?>{'name': 'enable_impeller', 'value': 'false'},
      ]);

      await setImpellerEnableFlag(harness.project, _ohosBuildInfo(enableImpeller: true));

      expect(harness.flags(), <String, Object?>{'enable_impeller': 'true'});
    });

    testUsingContext('does not append enable_impeller when the entry is missing', () async {
      final _BuildInfoHarness harness = _BuildInfoHarness.create();
      harness.writeBuildInfo(<Map<String, Object?>>[
        <String, Object?>{'name': 'enable_ohos_hybrid_composition', 'value': 'false'},
      ]);

      await setImpellerEnableFlag(harness.project, _ohosBuildInfo(enableImpeller: true));

      expect(harness.flags(), <String, Object?>{'enable_ohos_hybrid_composition': 'false'});
    });
  });
}

OhosBuildInfo _ohosBuildInfo({bool? enableHcpp, bool? enableImpeller}) {
  return OhosBuildInfo(
    BuildInfo.debug,
    enableHcppFlag: enableHcpp,
    enableImpellerFlag: enableImpeller,
  );
}

/// `_setBuildInfoFlag` reads/writes through [globals.localFileSystem], so the
/// module directory must live on the real disk rather than a MemoryFileSystem.
class _BuildInfoHarness {
  _BuildInfoHarness._(this.moduleDir);

  factory _BuildInfoHarness.create() {
    final Directory moduleDir = globals.fs.systemTempDirectory.createTempSync(
      'flutter_hvigor_test.',
    );
    addTearDown(() => tryToDelete(moduleDir));
    return _BuildInfoHarness._(moduleDir);
  }

  final Directory moduleDir;

  OhosProject get project => _FakeOhosProject(moduleDir);

  File get buildInfoFile {
    return globals.localFileSystem.file(
      globals.fs.path.join(moduleDir.path, BUILD_INFO_JSON_DES_PATH),
    );
  }

  void writeBuildInfo(List<Map<String, Object?>> stringEntries) {
    buildInfoFile.createSync(recursive: true);
    buildInfoFile.writeAsStringSync(jsonEncode(<String, Object?>{'string': stringEntries}));
  }

  Map<String, Object?> flags() {
    final jsonMap = jsonDecode(buildInfoFile.readAsStringSync()) as Map<String, dynamic>;
    final List<Map<String, dynamic>> stringList = (jsonMap['string'] as List<dynamic>)
        .cast<Map<String, dynamic>>();
    return <String, Object?>{
      for (final Map<String, dynamic> item in stringList) item['name'] as String: item['value'],
    };
  }
}

class _FakeOhosProject extends Fake implements OhosProject {
  _FakeOhosProject(this.flutterModuleDirectory);

  @override
  final Directory flutterModuleDirectory;
}
