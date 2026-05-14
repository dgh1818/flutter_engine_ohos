// Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE_HW file.

import 'dart:async';

import 'package:flutter/foundation.dart';
import 'message_codec.dart';
import 'platform_channel.dart';
import '../widgets/split_view_config.dart';

/// Split view configuration loader.
///
/// Reads split view configuration from OhOS platform.
/// Uses OptionalMethodChannel, so if platform doesn't implement handler,
/// app startup is not affected.
class SplitViewConfigLoader {
  static final SplitViewConfigLoader _instance = SplitViewConfigLoader._internal();

  factory SplitViewConfigLoader() => _instance;

  SplitViewConfigLoader._internal();

  /// Raw configuration data read from platform.
  Map<String, dynamic>? _rawConfig;

  /// Completer for config loading completion.
  final Completer<void> _configReadyCompleter = Completer<void>();

  Map<String, dynamic>? getRawConfig() => _rawConfig;

  /// Wait for config to be ready.
  ///
  /// Returns immediately if config is already ready.
  /// Waits for loading to complete if config is being loaded.
  Future<void> waitForConfig() => _configReadyCompleter.future;

  /// Load config with timeout (blocking style).
  ///
  /// Used during binding initialization to block until config is loaded.
  /// After timeout, returns and SplitViewConfig uses member defaults.
  ///
  /// Possible results:
  /// 1. Normal load → _rawConfig has data
  /// 2. Config file not exists (platform returns null) → _rawConfig is null
  /// 3. Platform handler not implemented → _rawConfig is null
  /// 4. Communication timeout → _rawConfig is null (exception, prints timeout warning)
  ///
  /// [timeout] Timeout duration, default 500ms (suitable for blocking before first frame).
  Future<void> loadConfigWithTimeout({
    Duration timeout = const Duration(milliseconds: 500),
  }) async {
    bool isTimeout = false;

    try {
      const MethodChannel channel = OptionalMethodChannel('flutter/split_view_config');

      final Map<dynamic, dynamic>? platformConfig = await channel
          .invokeMethod<Map<dynamic, dynamic>>('getSplitViewConfig')
          .timeout(timeout, onTimeout: () {
            isTimeout = true;
            return null;
          });

      if (platformConfig != null) {
        final Map<String, dynamic> fullConfig = Map<String, dynamic>.from(platformConfig);
        final dynamic splitOptions = fullConfig['splitOptions'];
        if (splitOptions != null && splitOptions is Map) {
          _rawConfig = Map<String, dynamic>.from(splitOptions);
        } else {
          _rawConfig = fullConfig;
        }
        // Parse config to SplitViewConfig immediately after loading
        SplitViewConfig().parseConfigSync();
      } else if (isTimeout) {
        debugPrint('Warning: SplitView config load timeout (${timeout.inMilliseconds}ms)');
      }
    } on MissingPluginException catch (_) {
      // Platform handler not implemented
    } catch (e) {
      debugPrint('Warning: SplitView config load failed: $e');
    } finally {
      if (!_configReadyCompleter.isCompleted) {
        _configReadyCompleter.complete();
      }
    }
  }

  /// Start config loading (fire-and-forget style).
  ///
  /// This method doesn't block. Completer completes automatically after config loads.
  Future<void> startLoad() async {
    await loadConfigWithTimeout(
      timeout: const Duration(milliseconds: 500),
    );
  }

  void reset() {
    _rawConfig = null;
  }

  void clearRawConfig() {
    _rawConfig = null;
  }
}
