// Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE_HW file.

// ignore_for_file: use_setters_to_change_properties

import 'dart:convert';

import 'package:flutter/foundation.dart';
import '../services/split_view_config_loader.dart';

/// Split screen configuration - settings from config file.
///
/// This class automatically preloads configuration from platform at app startup.
/// No manual initialization is required.
///
/// Configuration format (fields map to member variables):
/// ```json
/// {
///   "enableWideWindowSplit": true,
///   "enableSquareWindowSplit": true,
///   "homePage": "HomePage",
///   "fullScreenPages": ["VideoPlayer", "ImageViewer"],
///   "enableReducedContainerSize": true,
///   "supportLandscapeFullscreen": true
/// }
/// ```
class SplitViewConfig {
  /// Returns the singleton [SplitViewConfig] instance.
  factory SplitViewConfig() => _instance;

  SplitViewConfig._internal();
  static final SplitViewConfig _instance = SplitViewConfig._internal();

  /// Wide window split switch, default false.
  bool _enableWideWindowSplit = false;

  /// Square window split switch, default false.
  bool _enableSquareWindowSplit = false;

  /// Home page route name. If not specified, first page is the home page.
  String? _homePage;

  /// Pages that need fullscreen display.
  List<String> _fullScreenPages = <String>[];

  /// Component width based on half screen width, default true.
  bool _enableReducedContainerSize = true;

  /// Fullscreen on landscape orientation request, default true.
  bool _supportLandscapeFullscreen = true;

  /// Placeholder icon data (base64 encoded), displayed in split start page.
  String? _placeholderIconData;
  Uint8List? _placeholderIconBytes;
  bool _bytesDecoded = false;

  /// Whether wide window split is enabled.
  bool get enableWideWindowSplit => _enableWideWindowSplit;

  /// Whether square window split is enabled.
  bool get enableSquareWindowSplit => _enableSquareWindowSplit;

  /// Whether any split mode is enabled.
  bool get isEnabled => _enableWideWindowSplit || _enableSquareWindowSplit;

  /// Home page route name, or null to use the first page.
  String? get homePage => _homePage;

  /// Route names that must display fullscreen.
  List<String> get fullScreenPages => _fullScreenPages;

  /// Whether container width is reduced to half screen.
  bool get enableReducedContainerSize => _enableReducedContainerSize;

  /// Whether fullscreen is allowed for landscape orientation.
  bool get supportLandscapeFullscreen => _supportLandscapeFullscreen;

  /// Placeholder icon data (base64), or null.
  String? get placeholderIconData => _placeholderIconData;

  /// Decoded placeholder icon bytes, lazily decoded on first access.
  Uint8List? get placeholderIconBytes {
    if (!_bytesDecoded) {
      _bytesDecoded = true;
      if (_placeholderIconData != null && _placeholderIconData!.isNotEmpty) {
        try {
          _placeholderIconBytes = base64Decode(_placeholderIconData!);
        } catch (e) {
          debugPrint('SplitViewConfig: Failed to decode placeholder icon: $e');
          _placeholderIconBytes = null;
        }
      }
    }
    return _placeholderIconBytes;
  }

  /// Sets the placeholder icon data (base64 encoded string).
  void setPlaceholderIconData(String? data) {
    _placeholderIconData = data;
    _bytesDecoded = false;
    _placeholderIconBytes = null;
  }

  /// Parse configuration synchronously (called from WidgetsBinding.initInstances).
  ///
  /// Configuration is already loaded via lockEvents in ServicesBinding.
  /// This method parses the loaded configuration from SplitViewConfigLoader.
  /// Uses type-safe parsing to avoid runtime exceptions from malformed config.
  void parseConfigSync() {
    final Map<String, dynamic>? config = SplitViewConfigLoader().getRawConfig();

    if (config == null) {
      return;
    }

    // Type-safe parsing for boolean fields
    _enableWideWindowSplit =
        (config['enableWideWindowSplit'] is bool) && config['enableWideWindowSplit'] as bool;
    _enableSquareWindowSplit =
        (config['enableSquareWindowSplit'] is bool) && config['enableSquareWindowSplit'] as bool;

    // Type-safe parsing for string field, treat empty string as null
    final dynamic homePageValue = config['homePage'];
    if (homePageValue is String && homePageValue.isNotEmpty) {
      _homePage = homePageValue;
    } else {
      _homePage = null;
    }

    // Type-safe parsing for list field with element filtering
    // Also filter out empty strings for consistency
    final dynamic fullScreenPagesValue = config['fullScreenPages'];
    if (fullScreenPagesValue != null && fullScreenPagesValue is List) {
      final List<String> validPages = fullScreenPagesValue
          .whereType<String>()
          .where((s) => s.isNotEmpty)
          .toList();
      if (validPages.length != fullScreenPagesValue.length) {
        debugPrint(
          'SplitViewConfig: Some fullScreenPages items are not strings or empty and were skipped',
        );
      }
      _fullScreenPages = validPages;
    } else {
      _fullScreenPages = <String>[];
    }

    // Type-safe parsing for remaining boolean fields
    _enableReducedContainerSize =
        config['enableReducedContainerSize'] is! bool ||
        config['enableReducedContainerSize'] as bool;
    _supportLandscapeFullscreen =
        config['supportLandscapeFullscreen'] is! bool ||
        config['supportLandscapeFullscreen'] as bool;

    // Clear temporary data in SplitViewConfigLoader after parsing
    SplitViewConfigLoader().clearRawConfig();
  }

  /// Sets whether wide window split is enabled.
  void setEnableWideWindowSplit(bool enable) {
    _enableWideWindowSplit = enable;
  }

  /// Sets whether square window split is enabled.
  void setEnableSquareWindowSplit(bool enable) {
    _enableSquareWindowSplit = enable;
  }

  /// Sets the home page route name.
  void setHomePage(String? homePage) {
    _homePage = homePage;
  }

  /// Sets the route names that must display fullscreen.
  void setFullScreenPages(List<String> pages) {
    _fullScreenPages = pages;
  }

  /// Check if the specified route needs fullscreen display.
  bool isForceFullscreenRoute(String? routeName) {
    if (routeName == null || routeName.isEmpty) {
      return false;
    }
    return _fullScreenPages.contains(routeName);
  }

  /// Sets whether container width is reduced to half screen.
  void setEnableReducedContainerSize(bool enable) {
    _enableReducedContainerSize = enable;
  }

  /// Sets whether fullscreen is allowed for landscape orientation.
  void setSupportLandscapeFullscreen(bool enable) {
    _supportLandscapeFullscreen = enable;
  }

  /// Resets all configuration to defaults.
  void reset() {
    _enableWideWindowSplit = false;
    _enableSquareWindowSplit = false;
    _homePage = null;
    _fullScreenPages = <String>[];
    _enableReducedContainerSize = true;
    _supportLandscapeFullscreen = true;
    SplitViewConfigLoader().reset();
  }
}
