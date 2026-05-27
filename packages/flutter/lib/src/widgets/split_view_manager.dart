// Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE_HW file.

import 'dart:async';
import 'package:flutter/foundation.dart';
import 'package:flutter/widgets.dart';
import 'package:flutter/services.dart';
import 'split_view_config.dart';

/// Route name constant for split view start page.
/// Used to avoid circular dependency (SplitStartPage defined in split_view_container.dart).
const String kSplitStartPageRouteName = '/split_start_page';

class SplitViewManager extends ChangeNotifier {
  static final SplitViewManager _instance = SplitViewManager._internal();

  factory SplitViewManager() {
    return _instance;
  }

  SplitViewManager._internal() {
    // Listen to orientation changes from SystemChrome
    OrientationChangeNotifier().addListener(_onOrientationChange);
  }

  /// Handle orientation change notifications from OrientationChangeNotifier.
  void _onOrientationChange() {
    final bool isForcedLandscape = OrientationChangeNotifier().isForcedLandscape;
    final config = SplitViewConfig();
    if (config.supportLandscapeFullscreen) {
      setLandscapeFullscreen(isForcedLandscape);
    }
  }

  /// Final determined home page route name.
  String? _realHomePage;

  String? get realHomePage => _realHomePage;

  void setRealHomePage(String? homePage) {
    _realHomePage = homePage;
  }

  /// Whether home page has been navigated to.
  bool _homePageReady = false;

  /// Get whether left side page is ready.
  bool get isHomePageReady => _homePageReady;

  /// Mark home page as ready (navigated to specified home page).
  void markHomePageReady() {
    _homePageReady = true;
  }

  /// Reference to left navigator state.
  NavigatorState? _leftNavigator;

  NavigatorState? get leftNavigator => _leftNavigator;

  void setLeftNavigator(NavigatorState? navigator) {
    _leftNavigator = navigator;
  }

  /// Reference to right navigator state.
  NavigatorState? _rightNavigator;

  NavigatorState? get rightNavigator => _rightNavigator;

  void setRightNavigator(NavigatorState? navigator) {
    _rightNavigator = navigator;
  }

  /// Global RouteObservers - notify left side RouteAware's didPushNext/didPopNext.
  final List<RouteObserver<ModalRoute<dynamic>>> _globalRouteObservers =
      <RouteObserver<ModalRoute<dynamic>>>[];

  List<RouteObserver<ModalRoute<dynamic>>> get globalRouteObservers => _globalRouteObservers;

  void addGlobalRouteObserver(RouteObserver<ModalRoute<dynamic>> observer) {
    _globalRouteObservers.add(observer);
  }

  /// Current route on left side - used to notify left side RouteAware when placeholder page pops on right side.
  Route<dynamic>? _leftCurrentRoute;

  /// Get current route on left side - called by _RightSideNavigatorObserver.
  Route<dynamic>? get leftCurrentRoute => _leftCurrentRoute;

  /// Set current route on left side - called by _ProxyNavigatorObserver.
  void setLeftCurrentRoute(Route<dynamic> route) {
    _leftCurrentRoute = route;
  }

  /// Whether split screen is active on current page.
  bool _isSplitViewActive = false;

  bool get isSplitViewActive => _isSplitViewActive;

  void setSplitViewActive(bool value) {
    if (_isSplitViewActive == value) {
      return;
    }
    _isSplitViewActive = value;
    notifyListeners();
  }

  /// Determines whether right side shows split screen start page based on resolution and orientation.
  bool _rightSideShowsPlaceholder = true;

  bool get rightSideShowsPlaceholder => _rightSideShowsPlaceholder;

  void setRightSideShowsPlaceholder(bool value) {
    if (_rightSideShowsPlaceholder == value) {
      return;
    }
    _rightSideShowsPlaceholder = value;
    notifyListeners();
  }

  /// Whether there is a popup route on screen.
  bool _isPoprouteOnScreen = false;

  bool get poprouteOnScreen => _isPoprouteOnScreen;

  void setPoprouteOnScreen(bool value) {
    if (_isPoprouteOnScreen == value) {
      return;
    }
    _isPoprouteOnScreen = value;
    notifyListeners();
  }

  /// Whether current page is forced fullscreen.
  bool _isForceFullscreen = false;

  bool get isForceFullscreen => _isForceFullscreen;

  void setForceFullscreen(bool value) {
    if (_isForceFullscreen == value) {
      return;
    }
    _isForceFullscreen = value;
    notifyListeners();
  }

  /// Whether app requested forced landscape mode (landscapeLeft/landscapeRight only).
  /// When true, split view should be disabled.
  bool _isForcedLandscape = false;

  bool get isForcedLandscape => _isForcedLandscape;

  void setLandscapeFullscreen(bool value) {
    if (_isForcedLandscape == value) {
      return;
    }
    _isForcedLandscape = value;
    notifyListeners();
  }

  /// Set of processed PopupRoutes to prevent duplicate processing on both sides.
  final Set<Route<dynamic>> _processedPopupRoutes = <Route<dynamic>>{};

  Set<Route<dynamic>> get processedPopupRoutes => _processedPopupRoutes;

  void addProcessedPopupRoute(Route<dynamic> route) {
    _processedPopupRoutes.add(route);
  }

  void removeProcessedPopupRoute(Route<dynamic> route) {
    _processedPopupRoutes.remove(route);
  }

  bool hasProcessedPopupRoute(Route<dynamic> route) {
    return _processedPopupRoutes.contains(route);
  }

  /// Forward navigation to right navigator - pushNamed operation (auto clears right stack).
  Future<T?> pushNamedToRightAndClear<T extends Object?>(
    String routeName, {
    Object? arguments,
  }) {
    clearRightStack();
    if (_rightNavigator != null) {
      return _rightNavigator!.pushNamed(routeName, arguments: arguments);
    }
    return Future.value(null);
  }

  /// Clear right route stack and navigate to new page.
  Future<T?> pushToRightAndClear<T extends Object?>(Route<T> route) {
    clearRightStack();
    final completer = Completer<T?>();
    WidgetsBinding.instance.addPostFrameCallback((_) {
      pushToRight<T>(route).then(completer.complete);
    });
    return completer.future;
  }

  /// Forward navigation to right navigator - push operation.
  Future<T?> pushToRight<T extends Object?>(Route<T> route) {
    if (_rightNavigator == null) {
      return Future.value(null);
    }
    return _rightNavigator!.push(route);
  }

  /// Forward navigation to right navigator - pushAndRemoveUntil operation.
  Future<T?> pushAndRemoveUntilToRight<T extends Object?>(
    Route<T> newRoute,
    RoutePredicate predicate,
  ) {
    if (_rightNavigator == null) {
      return Future.value(null);
    }
    return _rightNavigator!.pushAndRemoveUntil(newRoute, predicate);
  }

  /// Clear right route stack, keep only split screen start page.
  void clearRightStack() {
    if (_rightNavigator != null) {
      _rightNavigator!.popUntil((route) =>
          route.settings.name == kSplitStartPageRouteName || route.isFirst);
    }
  }

  /// Reset manager to initial state.
  /// Only call when completely exiting split screen structure (e.g., user logout, root widget disposed, test teardown).
  void reset() {
    // 1. Remove listener to avoid memory leaks and invalid calls during reset
    OrientationChangeNotifier().removeListener(_onOrientationChange);

    // 2. Clear strong references to prevent memory leaks
    _leftNavigator = null;
    _rightNavigator = null;
    _globalRouteObservers.clear();
    _leftCurrentRoute = null;

    // 3. Clear internal collections
    _processedPopupRoutes.clear();

    // 4. Restore config flags to initial defaults
    _realHomePage = null;
    _homePageReady = false;
    _isSplitViewActive = false;
    _rightSideShowsPlaceholder = true;
    _isForceFullscreen = false;
    _isForcedLandscape = false;

    // 5. Notify all listeners that environment has been reset
    notifyListeners();

    // 6. Re-add listener to restore functionality
    OrientationChangeNotifier().addListener(_onOrientationChange);
  }
}
