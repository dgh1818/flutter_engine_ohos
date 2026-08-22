// Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE_HW file.

// Host-side (platform-neutral) unit tests for `_window_ohos.dart`.
//
// The `Platform.isOhos` guard and the controller lifecycle require a real
// OHOS embedding and are intentionally NOT covered here. What IS covered:
//
// 1. The feature-flag guard of the `WindowingOwnerOHOS` constructor, which
//    runs before any platform check and behaves identically everywhere.
// 2. The `ohosWindowingSupported` getter and the `_OHOSPlatformInterface`
//    FFI binding it goes through, mocked with a fake `libflutter.so` (see
//    [_FakeLibflutter]).

import 'dart:ffi';
import 'dart:io';

import 'package:flutter/src/foundation/_features.dart' show isWindowingEnabled;
import 'package:flutter/src/widgets/_window_ohos.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  group('WindowingOwnerOHOS', () {
    test('constructor throws UnsupportedError when windowing is disabled', () {
      isWindowingEnabled = false;

      expect(WindowingOwnerOHOS.new, throwsUnsupportedError);
    });
  });

  // Built once, before the groups register, so the skip decisions below are
  // sound. Null on hosts that cannot build the stub (non-Linux, no C
  // compiler); the affected tests are skipped there with a reason.
  final fakeAvailable = _FakeLibflutter.tryCreate() != null;

  group('ohosWindowingSupported (mocked libflutter.so)', () {
    const skipReason =
        'requires Linux and a C compiler (gcc/cc/clang) to build the fake '
        'libflutter.so used to mock the FFI surface';

    if (fakeAvailable) {
      tearDownAll(_FakeLibflutter.instance.dispose);
    }

    test('returns true when OHOS_WindowingSupported reports support', () {
      _FakeLibflutter.instance.flag.value = 1;

      expect(ohosWindowingSupported, isTrue);
    }, skip: fakeAvailable ? false : skipReason);

    test('returns false when OHOS_WindowingSupported reports no support', () {
      _FakeLibflutter.instance.flag.value = 0;

      expect(ohosWindowingSupported, isFalse);
    }, skip: fakeAvailable ? false : skipReason);

    test('re-reads the native symbol on every access', () {
      _FakeLibflutter.instance.flag.value = 1;
      expect(ohosWindowingSupported, isTrue);

      _FakeLibflutter.instance.flag.value = 0;
      expect(ohosWindowingSupported, isFalse);
    }, skip: fakeAvailable ? false : skipReason);
  });
}

/// Builds and preloads a fake `libflutter.so` so that the production code's
/// `DynamicLibrary.open('libflutter.so')` resolves to this stub.
///
/// The trick: the stub is compiled with `-Wl,-soname,libflutter.so` and
/// opened here by absolute path. Once an object with that SONAME is in the
/// process, glibc's `dlopen("libflutter.so")` deduplicates to it instead of
/// searching the filesystem — no LD_LIBRARY_PATH or source changes needed.
///
/// Only `OHOS_WindowingSupported` is exported because it is the only symbol
/// reachable from a host test: every other FFI entry point sits behind the
/// `Platform.isOhos` guard of the controller constructors. The stub reads a
/// global flag that tests poke through FFI to control the reported value.
class _FakeLibflutter {
  _FakeLibflutter._(this._dir, this.flag);

  final Directory _dir;
  bool _disposed = false;

  /// Address of `g_ohos_windowing_supported` inside the stub; write to it
  /// to flip what `OHOS_WindowingSupported` returns.
  final Pointer<Int32> flag;

  static const String _cSource = '''
#include <stdbool.h>

int g_ohos_windowing_supported = 1;

bool OHOS_WindowingSupported(void) { return g_ohos_windowing_supported != 0; }
''';

  static _FakeLibflutter? _instance;
  static bool _built = false;

  /// The stub built by [tryCreate]; throws if none could be built (only
  /// reachable when the calling test was not skipped).
  static _FakeLibflutter get instance {
    final _FakeLibflutter? obj = _instance;
    if (obj == null) {
      throw StateError('fake libflutter.so unavailable');
    }
    return obj;
  }

  /// Compiles and preloads the stub exactly once; returns null when the
  /// host cannot build it (non-Linux, or no C compiler available).
  static _FakeLibflutter? tryCreate() {
    if (!_built) {
      _instance = _build();
      _built = true;
    }
    return _instance;
  }

  static _FakeLibflutter? _build() {
    if (!Platform.isLinux) {
      return null;
    }

    final Directory dir = Directory.systemTemp.createTempSync('flutter_windowing_ohos_test');
    final source = File('${dir.path}/fake.c')..writeAsStringSync(_cSource);
    final libPath = '${dir.path}/fake_libflutter.so';

    for (final compiler in const <String>['gcc', 'cc', 'clang']) {
      final ProcessResult result;
      try {
        result = Process.runSync(compiler, <String>[
          '-shared',
          '-fPIC',
          '-Wl,-soname,libflutter.so',
          '-o',
          libPath,
          source.path,
        ]);
      } on ProcessException {
        continue; // Compiler not installed; try the next candidate.
      }
      if (result.exitCode != 0) {
        continue;
      }

      // Preload by absolute path; registers SONAME `libflutter.so`.
      final lib = DynamicLibrary.open(libPath);
      final Pointer<Int32> flag = lib.lookup<Int32>('g_ohos_windowing_supported');
      return _FakeLibflutter._(dir, flag);
    }

    dir.deleteSync(recursive: true);
    return null;
  }

  void dispose() {
    if (_disposed) {
      return;
    }
    _disposed = true;
    _dir.deleteSync(recursive: true);
  }
}
