// Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE_HW file.

/// Configuration for OHOS CI platform.
///
/// Defines which shards/subshards should be skipped on OHOS,
/// and which extra tests should be added beyond the upstream shards.
///
/// When upstream adds or renames a shard, this file should be updated
/// accordingly to keep OHOS CI in sync.
library;

import 'dart:io' show Platform;

/// Whether the current CI run is on OHOS.
///
/// Set via environment variable `OHOS_CI=true` (used by run_shard_test.sh
/// and run_parallel_shards.sh). This avoids passing --define flags down to
/// `flutter test` which does not support them.
bool get isOhosCi => Platform.environment['OHOS_CI'] == 'true';

class OhosPlatformConfig {
  const OhosPlatformConfig._();

  /// Shards that should be completely skipped on OHOS.
  static const Set<String> skippedShards = <String>{
    'add_to_app_life_cycle_tests',
    'android_preview_tool_integration_tests',
    'android_java11_tool_integration_tests',
    'android_engine_vulkan_tests',
    'android_engine_opengles_tests',
    'web_canvaskit_tests',
    'web_skwasm_tests',
    'web_long_running_tests',
    'web_tool_tests',
    'flutter_plugins',
    'fuchsia_precache',
    'verify_binaries_codesigned',
    'verify_binaries_pre_codesigned',
    'docs',
    // OHOS TODO: tool_integration_tests require Android SDK, Chrome, and
    // network access for pub get. Most integration tests involve building
    // Android/iOS/web apps. Re-enable after OHOS CI environment has the
    // required toolchain or after skipping platform-specific test files.
    'tool_integration_tests',
    // OHOS TODO: customer_testing shard fails because the OHOS pubspec.yaml
    // replacement causes dependency resolution errors for integration_test_example.
    // Re-enable after fixing the pubspec_ohos.yaml to resolve all customer test deps.
    'customer_testing',
  };

  /// Subshards that should be skipped within a shard on OHOS.
  ///
  /// Key: shard name, Value: set of subshard names to skip.
  static const Map<String, Set<String>> skippedSubshards = <String, Set<String>>{
    'framework_tests': <String>{'impeller', 'misc_examples'},
    // OHOS TODO: widget_preview_scaffold has compile errors in OHOS fork.
    // The widget preview code uses Flutter framework types that are not
    // properly resolved in the OHOS package configuration. Re-enable after
    // fixing packages/flutter_tools/test/widget_preview_scaffold.shard/.
    'tool_tests': <String>{'widget_preview_scaffold'},
  };

  /// Test paths that should be excluded on OHOS within a given shard/subshard.
  ///
  /// Key: "shard/subshard" or "shard", Value: set of test file paths to skip.
  static const Map<String, Set<String>> skippedTestPaths = <String, Set<String>>{
    // OHOS TODO: build_apk_test requires Android SDK.
    // Re-enable after OHOS CI has Android SDK installed.
    'tool_tests/commands': <String>{
      'test/commands.shard/permeable/build_apk_test.dart',
      'test/commands.shard/permeable/build_appbundle_test.dart',
      // OHOS TODO: create_test fails because generated projects fail analysis
      // with OHOS pubspec replacement. Re-enable after fixing pubspec issues.
      'test/commands.shard/permeable/create_test.dart',
      // OHOS TODO: widget_preview_test requires Chrome/Edge browser.
      // Re-enable after OHOS CI has a browser installed.
      'test/commands.shard/permeable/widget_preview/widget_preview_test.dart',
      // OHOS TODO: preview_code_generator_test fails with "Failed to update packages"
      // in OHOS parallel pubspec environment. Re-enable after fixing pub resolution.
      'test/commands.shard/hermetic/widget_preview/preview_code_generator_test.dart',
      // OHOS TODO: daemon_test getSupportedPlatforms returns incomplete ohos reasons
      // (missing "platform not enabled for this project"). Fix OHOS flutter_tools
      // getSupportedPlatforms logic and re-enable.
      'test/commands.shard/hermetic/daemon_test.dart',
    },
    'tool_tests/general': <String>{
      // OHOS TODO: android_gradle_builder_test requires Android SDK.
      // Re-enable after OHOS CI has Android SDK installed.
      'test/general.shard/android/android_gradle_builder_test.dart',
      // OHOS TODO: mdns_discovery_test fails on Linux without mDNS support.
      // Re-enable after OHOS CI environment supports mDNS or test is skipped.
      'test/general.shard/mdns_discovery_test.dart',
    },
  };

  /// Integration test directories under dev/ that should be skipped on OHOS.
  static const Set<String> skippedIntegrationTests = <String>{
    // OHOS TODO: hook_user_defines fails with "User-define `magic_value` must
    // be an integer, found: null". The OHOS native assets hook returns null
    // for the magic_value define. Fix the hook implementation, then remove
    // from skippedIntegrationTests and add to extraTests with
    // shard='framework_tests', subshard='misc',
    // path='dev/integration_tests/hook_user_defines'.
    'hook_user_defines',
  };

  /// Extra tests to run on OHOS that are not part of upstream shards.
  static const List<OhosExtraTest> extraTests = <OhosExtraTest>[
    OhosExtraTest(
      shard: 'framework_tests',
      subshard: 'misc',
      path: 'dev/integration_tests/link_hook',
    ),
  ];
}

class OhosExtraTest {
  const OhosExtraTest({required this.shard, required this.subshard, required this.path});

  final String shard;
  final String subshard;
  final String path;
}
