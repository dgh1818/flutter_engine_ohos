// Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE_HW file.

import 'dart:convert';
import 'dart:typed_data';

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
  static final SplitViewConfig _instance = SplitViewConfig._internal();

  factory SplitViewConfig() => _instance;

  SplitViewConfig._internal();

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

  bool get enableWideWindowSplit => _enableWideWindowSplit;
  bool get enableSquareWindowSplit => _enableSquareWindowSplit;
  bool get isEnabled => _enableWideWindowSplit || _enableSquareWindowSplit;
  String? get homePage => _homePage;
  List<String> get fullScreenPages => _fullScreenPages;
  bool get enableReducedContainerSize => _enableReducedContainerSize;
  bool get supportLandscapeFullscreen => _supportLandscapeFullscreen;
  String? get placeholderIconData => _placeholderIconData;

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
    final Map<String, dynamic>? config =
        SplitViewConfigLoader().getRawConfig();

    if (config == null) {
      return;
    }

    // Type-safe parsing for boolean fields
    _enableWideWindowSplit =
        (config['enableWideWindowSplit'] is bool)
            ? config['enableWideWindowSplit'] as bool
            : false;
    _enableSquareWindowSplit =
        (config['enableSquareWindowSplit'] is bool)
            ? config['enableSquareWindowSplit'] as bool
            : false;

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
            'SplitViewConfig: Some fullScreenPages items are not strings or empty and were skipped');
      }
      _fullScreenPages = validPages;
    } else {
      _fullScreenPages = <String>[];
    }

    // Type-safe parsing for remaining boolean fields
    _enableReducedContainerSize =
        (config['enableReducedContainerSize'] is bool)
            ? config['enableReducedContainerSize'] as bool
            : true;
    _supportLandscapeFullscreen =
        (config['supportLandscapeFullscreen'] is bool)
            ? config['supportLandscapeFullscreen'] as bool
            : true;

    // Clear temporary data in SplitViewConfigLoader after parsing
    SplitViewConfigLoader().clearRawConfig();
  }

  void setEnableWideWindowSplit(bool enable) {
    _enableWideWindowSplit = enable;
  }

  void setEnableSquareWindowSplit(bool enable) {
    _enableSquareWindowSplit = enable;
  }

  void setHomePage(String? homePage) {
    _homePage = homePage;
  }

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

  void setEnableReducedContainerSize(bool enable) {
    _enableReducedContainerSize = enable;
  }

  void setSupportLandscapeFullscreen(bool enable) {
    _supportLandscapeFullscreen = enable;
  }

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
