// Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE_HW file.

import 'dart:convert';
import 'dart:developer' as developer;

import '../widgets/split_view_config.dart';
import 'message_codecs.dart';
import 'platform_channel.dart';

/// Split view configuration loader.
///
/// Reads split view configuration from OhOS platform via SystemChannel.
/// The ETS engine pushes config to Dart through flutter/split_view_config_system
/// channel immediately after Dart isolate starts, which is the fastest way
/// to get config without blocking the first frame.
class SplitViewConfigLoader {
  /// Returns the singleton [SplitViewConfigLoader] instance.
  factory SplitViewConfigLoader() => _instance;

  SplitViewConfigLoader._internal();
  static final SplitViewConfigLoader _instance = SplitViewConfigLoader._internal();

  static const String _systemChannelName = 'flutter/split_view_config_system';

  Map<String, dynamic>? _rawConfig;

  /// Returns the raw config map received from the platform, or null.
  Map<String, dynamic>? getRawConfig() => _rawConfig;

  static Map<String, dynamic> _extractSplitOptions(Map<String, dynamic> fullConfig) {
    final dynamic splitOptions = fullConfig['splitOptions'];
    if (splitOptions != null && splitOptions is Map) {
      return Map<String, dynamic>.from(splitOptions);
    }
    return fullConfig;
  }

  /// Registers the platform channel handler that receives the config pushed
  /// by the ETS engine on isolate start.
  void setupSystemChannel() {
    const channel = BasicMessageChannel<String>(_systemChannelName, StringCodec());
    channel.setMessageHandler((String? message) async {
      if (message != null && message.isNotEmpty) {
        try {
          final fullConfig = jsonDecode(message) as Map<String, dynamic>;
          final Map<String, dynamic> config = _extractSplitOptions(fullConfig);
          _rawConfig = config;

          SplitViewConfig().parseConfigSync();
          final dynamic iconData = fullConfig['placeholderIconData'];
          if (iconData != null && iconData is String) {
            SplitViewConfig().setPlaceholderIconData(iconData);
          }
        } catch (e) {
          developer.log('SystemChannel config parse failed: $e', name: 'SplitViewConfig');
        }
      }
      return '';
    });
  }

  /// Clears the raw config and resets the loader state.
  void reset() {
    _rawConfig = null;
  }

  /// Clears the cached raw config (called after [SplitViewConfig] parses it).
  void clearRawConfig() {
    _rawConfig = null;
  }
}
