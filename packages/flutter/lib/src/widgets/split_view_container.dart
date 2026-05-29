// Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE_HW file.

import 'dart:async';

import 'package:flutter/scheduler.dart';
import 'package:flutter/widgets.dart';
import 'split_view_config.dart';
import 'split_view_manager.dart';

/// Split screen container - displays left and right side pages.
///
/// This container accepts the same parameters as Navigator and passes them
/// to two internal Navigators.
class SplitViewContainer extends StatefulWidget {
  /// Initial route for the app - passed to internal Navigator.
  final String? initialRoute;

  /// Initial route generation function - passed to internal Navigator.
  final RouteListFactory onGenerateInitialRoutes;

  /// Route generation function for the app - passed to internal Navigator.
  final RouteFactory? onGenerateRoute;

  /// Unknown route handler - passed to internal Navigator.
  final RouteFactory? onUnknownRoute;

  /// Whether to report route updates to engine - passed to internal Navigator.
  final bool reportsRouteUpdateToEngine;

  /// Clip behavior - passed to internal Navigator.
  final Clip clipBehavior;

  /// App-level Navigator observers - passed to left and right internal Navigators.
  final List<NavigatorObserver>? navigatorObservers;

  /// Whether to request focus - passed to internal Navigator.
  final bool requestFocus;

  /// Restoration scope ID - passed to internal Navigator.
  final String? restorationScopeId;

  /// Route traversal edge behavior - passed to internal Navigator.
  final TraversalEdgeBehavior routeTraversalEdgeBehavior;

  /// Navigator key - points to left _ProxyNavigator, allows app code to access NavigatorState.
  /// When app calls navigatorKey.currentState?.push(), it forwards to left or right Navigator.
  final GlobalKey<NavigatorState>? navigatorKey;

  /// Creates a split screen container with parameters fully compatible with Navigator.
  const SplitViewContainer({
    Key? key,
    this.initialRoute,
    this.onGenerateInitialRoutes = Navigator.defaultGenerateInitialRoutes,
    this.onGenerateRoute,
    this.onUnknownRoute,
    this.reportsRouteUpdateToEngine = false,
    this.clipBehavior = Clip.hardEdge,
    this.navigatorObservers,
    this.requestFocus = true,
    this.restorationScopeId,
    this.routeTraversalEdgeBehavior = kDefaultRouteTraversalEdgeBehavior,
    this.navigatorKey,
  }) : super(key: key);

  @override
  State<SplitViewContainer> createState() => _SplitViewContainerState();
}

class _SplitViewContainerState extends State<SplitViewContainer>
    with WidgetsBindingObserver {
  late SplitViewManager _manager;
  late bool isForceFullscreen;
  late bool isSplitViewActive;
  bool _rightSideShowsPlaceholder = true;
  late final GlobalKey<_ProxyNavigatorState> _leftNavKey;
  late final GlobalKey<_RightSideNavigatorState> _rightNavKey;

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addObserver(this);
    _manager = SplitViewManager();
    _manager.addListener(_onManagerChanged);
    // Check screen size - if physicalSize is invalid (Size.zero), defaults to false
    // and will be corrected when didChangeMetrics triggers
    isSplitViewActive = _checkScreenSizeAndSetSplitScreen();
    _manager.setSplitViewActive(isSplitViewActive);
    isForceFullscreen = _manager.isForceFullscreen;
    _leftNavKey = GlobalKey<_ProxyNavigatorState>();
    _rightNavKey = GlobalKey<_RightSideNavigatorState>();
  }

  @override
  void dispose() {
    WidgetsBinding.instance.removeObserver(this);
    _manager.removeListener(_onManagerChanged);
    super.dispose();
  }

  /// System screen change event - orientation switch, foldable screen open/close, etc.
  @override
  void didChangeMetrics() {
    super.didChangeMetrics();

    final bool shouldActiveSplitScreen = _checkScreenSizeAndSetSplitScreen();
    setState(() {
      _manager.setSplitViewActive(shouldActiveSplitScreen);
      isSplitViewActive = _manager.isSplitViewActive;
    });
  }

  bool _checkScreenSizeAndSetSplitScreen() {
    final Size physicalSize = WidgetsBinding.instance.window.physicalSize;
    final double devicePixelRatio =
        WidgetsBinding.instance.window.devicePixelRatio;

    // Return false if window is not yet initialized (Size.zero)
    // Will be corrected when didChangeMetrics triggers after window is ready
    if (physicalSize == Size.zero || devicePixelRatio == 0.0) {
      return false;
    }

    final double logicalWidth = physicalSize.width / devicePixelRatio;
    final double logicalHeight = physicalSize.height / devicePixelRatio;
    const double splitScreenWidthThreshold = 600;

    if (logicalWidth < splitScreenWidthThreshold ||
        logicalHeight < splitScreenWidthThreshold) {
      return false;
    }

    final double widthToHeightRatio = logicalWidth / logicalHeight;
    final double heightToWidthRatio = logicalHeight / logicalWidth;
    final config = SplitViewConfig();

    if (widthToHeightRatio <= 1.2 && heightToWidthRatio <= 1.2) {
      return config.enableSquareWindowSplit;
    }

    if (!config.enableWideWindowSplit) {
      return false;
    }

    return logicalWidth > logicalHeight;
  }

  @override
  Future<bool> didPopRoute() async {
    final rightNavigator = _manager.rightNavigator;
    final leftNavigator = _manager.leftNavigator;
    if (rightNavigator == null || leftNavigator == null) {
      return false;
    }

    if (_manager.poprouteOnScreen) {
      if (!_manager.rightSideShowsPlaceholder) {
        rightNavigator.pop();
      } else {
        leftNavigator.pop();
      }
      return true;
    }

    if (rightNavigator.canPop()) {
      rightNavigator.pop();
      return true;
    }

    if (leftNavigator.canPop()) {
      leftNavigator.pop();
      return true;
    }

    return false;
  }

  void _onManagerChanged() {
    if (SchedulerBinding.instance.schedulerPhase == SchedulerPhase.persistentCallbacks) {
      return;
    }
    setState(() {});
  }

  @override
  Widget build(BuildContext context) {
    isForceFullscreen = _manager.isForceFullscreen;
    isSplitViewActive = _manager.isSplitViewActive;
    _rightSideShowsPlaceholder = _manager.rightSideShowsPlaceholder;

    // When app requests forced landscape (only landscapeLeft/landscapeRight),
    // and supportLandscapeFullscreen is enabled, disable split view
    final bool isForcedLandscape = _manager.isForcedLandscape &&
        SplitViewConfig().supportLandscapeFullscreen;

    final bool splitScreenOnThisPage =
        !isForceFullscreen && !isForcedLandscape && isSplitViewActive;

    // When split screen is active and enableReducedContainerSize is true,
    // wrap with MediaQuery to provide half-width size
    final bool shouldReduceSize = splitScreenOnThisPage &&
        SplitViewConfig().enableReducedContainerSize;
    final MediaQueryData mediaQueryData = MediaQuery.of(context);
    final MediaQueryData updatedMediaQueryData = mediaQueryData.copyWith(
      enableSplitView: shouldReduceSize,
    );

    return MediaQuery(
      data: updatedMediaQueryData,
      child: Row(
        children: <Widget>[
          Expanded(
            flex: splitScreenOnThisPage ? 50 : (_rightSideShowsPlaceholder ? 100 : 0),
            child: Semantics(
              container: true,
              explicitChildNodes: true,
              child: Visibility(
                visible: splitScreenOnThisPage || _rightSideShowsPlaceholder,
                maintainState: true,
                child: SizedBox(
                  width: splitScreenOnThisPage || _rightSideShowsPlaceholder
                      ? null
                      : 0,
                  height: splitScreenOnThisPage || _rightSideShowsPlaceholder
                      ? null
                      : 0,
                  child: _ProxyNavigator(
                    key: widget.navigatorKey ?? _leftNavKey,
                    initialRoute: widget.initialRoute,
                    onGenerateInitialRoutes: widget.onGenerateInitialRoutes,
                    onGenerateRoute: widget.onGenerateRoute,
                    onUnknownRoute: widget.onUnknownRoute,
                    reportsRouteUpdateToEngine: widget.reportsRouteUpdateToEngine,
                    clipBehavior: widget.clipBehavior,
                    requestFocus: widget.requestFocus,
                    restorationScopeId: widget.restorationScopeId,
                    routeTraversalEdgeBehavior: widget.routeTraversalEdgeBehavior,
                  ),
                ),
              ),
            ),
          ),
        Visibility(
          visible: splitScreenOnThisPage,
          child: Container(
            width: 1,
            color: const Color(0x33000000),
          ),
        ),
        Expanded(
          flex: splitScreenOnThisPage ? 50 : (_rightSideShowsPlaceholder ? 0 : 100),
          child: ClipRect(
            child: Semantics(
              container: true,
              explicitChildNodes: true,
              child: Visibility(
                visible: splitScreenOnThisPage || !_rightSideShowsPlaceholder,
                maintainState: true,
                child: SizedBox(
                  width: splitScreenOnThisPage || !_rightSideShowsPlaceholder
                      ? null
                      : 0,
                  height: splitScreenOnThisPage || !_rightSideShowsPlaceholder
                      ? null
                      : 0,
                  child: _RightSideNavigator(
                    key: _rightNavKey,
                    initialRoute: SplitStartPage.routeName,
                    onGenerateRoute: widget.onGenerateRoute,
                    onUnknownRoute: widget.onUnknownRoute,
                    reportsRouteUpdateToEngine: false,
                    clipBehavior: widget.clipBehavior,
                    navigatorObservers: widget.navigatorObservers,
                    requestFocus: widget.requestFocus,
                    restorationScopeId: 'nav-right',
                    routeTraversalEdgeBehavior: widget.routeTraversalEdgeBehavior,
                  ),
                ),
              ),
            ),
          ),
        ),
      ],
    ),
    );
  }
}

class _ProxyNavigator extends StatefulWidget {
  final String? initialRoute;
  final RouteListFactory onGenerateInitialRoutes;
  final RouteFactory? onGenerateRoute;
  final RouteFactory? onUnknownRoute;
  final bool reportsRouteUpdateToEngine;
  final Clip clipBehavior;
  final bool requestFocus;
  final String? restorationScopeId;
  final TraversalEdgeBehavior routeTraversalEdgeBehavior;

  const _ProxyNavigator({
    Key? key,
    this.initialRoute,
    this.onGenerateInitialRoutes = Navigator.defaultGenerateInitialRoutes,
    this.onGenerateRoute,
    this.onUnknownRoute,
    this.reportsRouteUpdateToEngine = false,
    this.clipBehavior = Clip.hardEdge,
    this.requestFocus = true,
    this.restorationScopeId,
    this.routeTraversalEdgeBehavior = kDefaultRouteTraversalEdgeBehavior,
  }) : super(key: key);

  @override
  State<_ProxyNavigator> createState() => _ProxyNavigatorState();
}

class _ProxyNavigatorState extends State<_ProxyNavigator> {
  late SplitViewManager _manager;
  late _ProxyNavigatorObserver _observer;
  late GlobalKey<_ProxyNavigatorStateImpl> _navigatorKey;
  Widget? _cachedChild;

  @override
  void initState() {
    super.initState();
    _manager = SplitViewManager();
    _observer = _ProxyNavigatorObserver(_manager);
    _navigatorKey = GlobalKey<_ProxyNavigatorStateImpl>();

    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (_navigatorKey.currentState != null) {
        _manager.setLeftNavigator(_navigatorKey.currentState!);
      }
    });
  }

  @override
  void dispose() {
    // Clear navigator reference to prevent memory leak
    if (_navigatorKey.currentState != null &&
        _manager.leftNavigator == _navigatorKey.currentState) {
      _manager.setLeftNavigator(null);
    }
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return _cachedChild ??= _ProxyNavigatorImpl(
      key: _navigatorKey,
      initialRoute: widget.initialRoute,
      onGenerateInitialRoutes: widget.onGenerateInitialRoutes,
      onGenerateRoute: (RouteSettings settings) {
        if (widget.onGenerateRoute != null) {
          final Route<dynamic>? route = widget.onGenerateRoute!(settings);
          if (route != null) {
            return route;
          }
        }
        return null;
      },
      onUnknownRoute: widget.onUnknownRoute,
      reportsRouteUpdateToEngine: widget.reportsRouteUpdateToEngine,
      clipBehavior: widget.clipBehavior,
      observers: <NavigatorObserver>[
        _observer,
      ],
      requestFocus: widget.requestFocus,
      restorationScopeId: widget.restorationScopeId,
      routeTraversalEdgeBehavior: widget.routeTraversalEdgeBehavior,
    );
  }
}

class _ProxyNavigatorImpl extends Navigator {
  _ProxyNavigatorImpl({
    required Key key,
    String? initialRoute,
    RouteListFactory onGenerateInitialRoutes = Navigator.defaultGenerateInitialRoutes,
    RouteFactory? onGenerateRoute,
    RouteFactory? onUnknownRoute,
    bool reportsRouteUpdateToEngine = false,
    Clip clipBehavior = Clip.hardEdge,
    required List<NavigatorObserver> observers,
    bool requestFocus = true,
    String? restorationScopeId,
    TraversalEdgeBehavior routeTraversalEdgeBehavior = kDefaultRouteTraversalEdgeBehavior,
  }) : super(
          key: key,
          pages: const <Page<dynamic>>[],
          initialRoute: initialRoute,
          onGenerateInitialRoutes: onGenerateInitialRoutes,
          onGenerateRoute: onGenerateRoute,
          onUnknownRoute: onUnknownRoute,
          reportsRouteUpdateToEngine: reportsRouteUpdateToEngine,
          clipBehavior: clipBehavior,
          observers: observers,
          requestFocus: requestFocus,
          restorationScopeId: restorationScopeId,
          routeTraversalEdgeBehavior: routeTraversalEdgeBehavior,
        );

  @override
  _ProxyNavigatorStateImpl createState() => _ProxyNavigatorStateImpl();
}

class _ProxyNavigatorStateImpl extends NavigatorState {
  final SplitViewManager _manager = SplitViewManager();
  final Map<Route<dynamic>, Route<dynamic>> _popupToBarrier = <Route<dynamic>, Route<dynamic>>{};

  @override
  Future<T?> push<T extends Object?>(Route<T> route) {
    final manager = SplitViewManager();

    // Handle PopupRoute (dialog, menu, etc.)
    // Note: If right side shows placeholder, don't forward popup to right, let it display on left
    if (_manager.isSplitViewActive &&
        route is PopupRoute &&
        !_manager.hasProcessedPopupRoute(route)) {
      if (manager.rightNavigator != null) {
        // Mark route as processed to prevent right side from processing again
        _manager.addProcessedPopupRoute(route);

        // Create barrier on left side
        final barrierRoute = _createBarrierRoute(route as PopupRoute);
        _popupToBarrier[route] = barrierRoute;
        _manager.addProcessedPopupRoute(barrierRoute);
        _manager.setPoprouteOnScreen(true);
        if (!_manager.rightSideShowsPlaceholder) {
          final FocusNode? previousFocus = WidgetsBinding.instance.focusManager.primaryFocus;
          super.push(barrierRoute);
          final rightFuture = manager.rightNavigator!.push(route);
          rightFuture.then((value) {
            final associatedBarrier = _popupToBarrier.remove(route);
            if (associatedBarrier != null && super.canPop()) {
              super.pop();
              _manager.removeProcessedPopupRoute(associatedBarrier);
            }
            _manager.removeProcessedPopupRoute(route);
            _manager.setPoprouteOnScreen(false);
            if (previousFocus != null && previousFocus.context != null) {
              previousFocus.requestFocus();
            }
          });
          return rightFuture;
        } else {
          final FocusNode? previousFocus = WidgetsBinding.instance.focusManager.primaryFocus;
          manager.rightNavigator!.push(barrierRoute);
          final leftFuture = super.push(route);
          leftFuture.then((value) {
            final associatedBarrier = _popupToBarrier.remove(route);
            if (associatedBarrier != null &&
                manager.rightNavigator != null &&
                manager.rightNavigator!.canPop()) {
              (manager.rightNavigator! as NavigatorState).pop();
              _manager.removeProcessedPopupRoute(associatedBarrier);
            }
            _manager.removeProcessedPopupRoute(route);
            _manager.setPoprouteOnScreen(false);
            if (previousFocus != null && previousFocus.context != null) {
              previousFocus.requestFocus();
            }
          });
          return leftFuture;
        }
      }
    }

    if (manager.rightNavigator != null &&
        route is! PopupRoute &&
        manager.isHomePageReady) {
      manager.setRightSideShowsPlaceholder(false);
      final config = SplitViewConfig();
      if (config.isForceFullscreenRoute(route.settings.name)) {
        manager.setForceFullscreen(true);
      } else {
        manager.setForceFullscreen(false);
      }
      return manager.pushToRightAndClear(route);
    }

    return super.push(route);
  }

  @override
  Future<T?> pushNamed<T extends Object?>(
    String routeName, {
    Object? arguments,
  }) {
    return super.pushNamed(routeName, arguments: arguments);
  }

  @override
  Future<T?> pushReplacementNamed<T extends Object?, TO extends Object?>(
    String routeName, {
    TO? result,
    Object? arguments,
  }) {
    if (_manager.isHomePageReady) {
      return pushNamed(routeName, arguments: arguments);
    }
    return super.pushReplacementNamed<T, TO>(routeName, result: result, arguments: arguments);
  }

  @override
  Future<T?> pushAndRemoveUntil<T extends Object?>(
    Route<T> newRoute,
    RoutePredicate predicate,
  ) {
    if (_manager.rightNavigator != null &&
        newRoute is! PopupRoute &&
        _manager.isHomePageReady) {
      _manager.setRightSideShowsPlaceholder(false);

      final config = SplitViewConfig();
      if (config.isForceFullscreenRoute(newRoute.settings.name)) {
        _manager.setForceFullscreen(true);
      } else {
        _manager.setForceFullscreen(false);
      }

      return _manager.pushAndRemoveUntilToRight(newRoute, predicate);
    }

    return super.pushAndRemoveUntil<T>(newRoute, predicate);
  }

  @override
  bool canPop() {
    if (_manager.isHomePageReady && !_manager.poprouteOnScreen) {
      final rightNavigator = _manager.rightNavigator;
      return rightNavigator != null && rightNavigator.canPop();
    }

    return super.canPop();
  }

  @override
  Future<bool> maybePop<T extends Object?>([T? result]) async {
    final rightNavigator = _manager.rightNavigator;
    // When popup route is on screen (barrier visible), only handle left side pop.
    // Don't forward to right navigator, because right side only has barrier route.
    // The real popup is on left side, so pop should happen on left.
    if (_manager.poprouteOnScreen) {
      return super.maybePop(result);
    }
    if (rightNavigator != null && rightNavigator.canPop()) {
      final didPop = await rightNavigator.maybePop(result);
      if (didPop) {
        return true;
      }
    }

    if (_manager.isHomePageReady && !_manager.poprouteOnScreen) {
      return false;
    }
  
    return super.maybePop(result);
  }

  @override
  void pop<T extends Object?>([T? result]) {
    if (_manager.isHomePageReady && !_manager.poprouteOnScreen) {
      return;
    }
    super.pop(result);
  }

  Route<T> _createBarrierRoute<T>(PopupRoute route) {
    return _BarrierOnlyRoute<T>(
      barrierColor: route.barrierColor,
      barrierDismissible: false,
      barrierLabel: route.barrierLabel,
    );
  }
}

/// Proxy navigator observer - monitors left navigator events.
/// Note: PopupRoute already handled by _ProxyNavigatorStateImpl.push()
class _ProxyNavigatorObserver extends NavigatorObserver {
  final SplitViewManager manager;

  _ProxyNavigatorObserver(this.manager);

  @override
  void didPop(Route<dynamic> route, Route<dynamic>? previousRoute) {
    if (manager.realHomePage == previousRoute?.settings.name) {
      manager.markHomePageReady();
    }
    manager.setLeftCurrentRoute(previousRoute ?? route);

    // Notify app's globalRouteObservers to trigger RouteAware's didPop/didPopNext callbacks
    for (final observer in manager.globalRouteObservers) {
      observer.didPop(route, previousRoute);
    }
  }

  @override
  void didPush(Route<dynamic> route, Route<dynamic>? previousRoute) {
    // Home page display logic:
    // Case 1: Has intermediate routes (splash/login pages, etc.)
    //   - Initialization: push '/' first (previousRoute=null), then '/splash' etc.
    //   - Real home display: when navigating back to home, previousRoute is not null
    // Case 2: No intermediate routes, directly display home '/'
    //   - Initialization: only push '/' (previousRoute=null), single route in stack
    //   - Then navigator.canPop() == false

    if (manager.realHomePage == route.settings.name) {
      if (previousRoute != null) {
        // previousRoute is not null, navigated from other route, mark home page directly
        manager.markHomePageReady();
      } else {
        // previousRoute is null, may be initial root route
        // Delayed check: if only one route in stack, this is initial home page
        WidgetsBinding.instance.addPostFrameCallback((_) {
          final canPop = navigator?.canPop() ?? false;
          if (!canPop) {
            // Single route in stack, this is initial home page
            manager.markHomePageReady();
          }
        });
      }
    }
    manager.setLeftCurrentRoute(route);

    for (final observer in manager.globalRouteObservers) {
      observer.didPush(route, previousRoute);
    }
  }

  @override
  void didReplace({Route<dynamic>? newRoute, Route<dynamic>? oldRoute}) {
    // Observer callback is called during Navigator _debugLocked, no route forwarding here
    // Forwarding logic should be in _ProxyNavigatorStateImpl's push/pushReplacementNamed methods
    if (manager.realHomePage == newRoute?.settings.name) {
      manager.markHomePageReady();
    }
    if (newRoute != null) {
      manager.setLeftCurrentRoute(newRoute);
    }

    for (final observer in manager.globalRouteObservers) {
      observer.didReplace(newRoute: newRoute, oldRoute: oldRoute);
    }
  }
}

class _RightSideNavigator extends StatefulWidget {
  final String? initialRoute;
  final RouteListFactory onGenerateInitialRoutes;
  final RouteFactory? onGenerateRoute;
  final RouteFactory? onUnknownRoute;
  final bool reportsRouteUpdateToEngine;
  final Clip clipBehavior;
  final List<NavigatorObserver>? navigatorObservers;
  final bool requestFocus;
  final String? restorationScopeId;
  final TraversalEdgeBehavior routeTraversalEdgeBehavior;

  const _RightSideNavigator({
    GlobalKey<_RightSideNavigatorState>? key,
    this.initialRoute,
    this.onGenerateInitialRoutes = Navigator.defaultGenerateInitialRoutes,
    this.onGenerateRoute,
    this.onUnknownRoute,
    this.reportsRouteUpdateToEngine = false,
    this.clipBehavior = Clip.hardEdge,
    this.navigatorObservers,
    this.requestFocus = true,
    this.restorationScopeId,
    this.routeTraversalEdgeBehavior = kDefaultRouteTraversalEdgeBehavior,
  }) : super(key: key);

  @override
  State<_RightSideNavigator> createState() => _RightSideNavigatorState();
}

class _RightSideNavigatorState extends State<_RightSideNavigator> {
  late SplitViewManager _manager;
  late _RightSideNavigatorObserver _observer;
  late final GlobalKey<_RightSideNavigatorStateImpl> _navigatorKey;
  Widget? _cachedChild;

  @override
  void initState() {
    super.initState();
    _manager = SplitViewManager();
    _navigatorKey = GlobalKey<_RightSideNavigatorStateImpl>();
    _observer = _RightSideNavigatorObserver();
    if (widget.navigatorObservers != null &&
        widget.navigatorObservers!.isNotEmpty) {
      for (final observer in widget.navigatorObservers!) {
        if (observer is RouteObserver<ModalRoute<dynamic>>) {
          _manager.addGlobalRouteObserver(observer);
        }
      }
    }
  }

  @override
  void dispose() {
    // Clear navigator reference to prevent memory leak
    if (_navigatorKey.currentState != null &&
        _manager.rightNavigator == _navigatorKey.currentState) {
      _manager.setRightNavigator(null);
    }
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    // Use HeroControllerScope.none to prevent multiple Navigators from sharing HeroController
    // This avoids Hero animation conflicts between outer and inner Navigators
    return _cachedChild ??= HeroControllerScope.none(
      child: _RightSideNavigatorImpl(
        key: _navigatorKey,
        initialRoute: widget.initialRoute, // Use passed initialRoute (already '/split_start_page')
        // Custom onGenerateInitialRoutes: only generate target route, no intermediate paths
        // This makes right side stack bottom directly SplitStartPage, not '/'
        onGenerateInitialRoutes: (navigator, initialRouteName) {
          final route = navigator.widget.onGenerateRoute?.call(RouteSettings(name: initialRouteName));
          if (route != null) {
            return [route];
          }
          return [PageRouteBuilder(
            settings: RouteSettings(name: SplitStartPage.routeName),
            pageBuilder: (_, __, ___) => const SplitStartPage(),
          )];
        },
        observers: <NavigatorObserver>[
          _observer,
          if (widget.navigatorObservers != null) ...widget.navigatorObservers!,
        ],
        reportsRouteUpdateToEngine: widget.reportsRouteUpdateToEngine,
        clipBehavior: widget.clipBehavior,
        requestFocus: widget.requestFocus,
        restorationScopeId: widget.restorationScopeId,
        routeTraversalEdgeBehavior: widget.routeTraversalEdgeBehavior,
        onGenerateRoute: (RouteSettings settings) {
          if (settings.name == SplitStartPage.routeName) {
            return PageRouteBuilder(
              settings: settings,
              pageBuilder: (context, animation, secondaryAnimation) => const SplitStartPage(),
            );
          }
          if (widget.onGenerateRoute != null) {
            final Route<dynamic>? route = widget.onGenerateRoute!(settings);
            if (route != null) {
              return route;
            }
          }
          return null;
        },
        onUnknownRoute: widget.onUnknownRoute,
      ),
    );
  }

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    if (_navigatorKey.currentState != null) {
      _manager.setRightNavigator(_navigatorKey.currentState!);
    } else {
      WidgetsBinding.instance.addPostFrameCallback((_) {
        if (_navigatorKey.currentState != null && mounted) {
          _manager.setRightNavigator(_navigatorKey.currentState!);
        }
      });
    }
  }
}

class _RightSideNavigatorImpl extends Navigator {
  _RightSideNavigatorImpl({
    required Key key,
    String? initialRoute,
    RouteListFactory onGenerateInitialRoutes = Navigator.defaultGenerateInitialRoutes,
    RouteFactory? onGenerateRoute,
    RouteFactory? onUnknownRoute,
    bool reportsRouteUpdateToEngine = false,
    Clip clipBehavior = Clip.hardEdge,
    required List<NavigatorObserver> observers,
    bool requestFocus = true,
    String? restorationScopeId,
    TraversalEdgeBehavior routeTraversalEdgeBehavior = kDefaultRouteTraversalEdgeBehavior,
  }) : super(
          key: key,
          pages: const <Page<dynamic>>[],
          initialRoute: initialRoute,
          onGenerateInitialRoutes: onGenerateInitialRoutes,
          onGenerateRoute: onGenerateRoute,
          onUnknownRoute: onUnknownRoute,
          reportsRouteUpdateToEngine: reportsRouteUpdateToEngine,
          clipBehavior: clipBehavior,
          observers: observers,
          requestFocus: requestFocus,
          restorationScopeId: restorationScopeId,
          routeTraversalEdgeBehavior: routeTraversalEdgeBehavior,
        );

  @override
  _RightSideNavigatorStateImpl createState() => _RightSideNavigatorStateImpl();
}

class _RightSideNavigatorStateImpl extends NavigatorState {
  final SplitViewManager _manager = SplitViewManager();
  final Map<Route<dynamic>, Route<dynamic>> _popupToBarrier = <Route<dynamic>, Route<dynamic>>{};

  @override
  Future<T?> push<T extends Object?>(Route<T> route) {
    if (_manager.isHomePageReady) {
      final routeName = route.settings.name;
      if (routeName == _manager.realHomePage) {
        _manager.setRightSideShowsPlaceholder(true);
        return pushAndRemoveUntil(
          PageRouteBuilder(
            settings: const RouteSettings(name: SplitStartPage.routeName),
            pageBuilder: (context, animation, secondaryAnimation) => const SplitStartPage(),
          ),
          (route) => false,
        );
      }
    }

    if (_manager.isSplitViewActive &&
        route is PopupRoute &&
        !_manager.hasProcessedPopupRoute(route)) {
      final manager = SplitViewManager();

      if (manager.leftNavigator != null) {
        _manager.addProcessedPopupRoute(route);
        final FocusNode? previousFocus = WidgetsBinding.instance.focusManager.primaryFocus;

        final barrierRoute = _createBarrierRoute(route as PopupRoute);
        _popupToBarrier[route] = barrierRoute;
        _manager.addProcessedPopupRoute(barrierRoute);
        manager.leftNavigator!.push(barrierRoute);
        _manager.setPoprouteOnScreen(true);
        final rightFuture = super.push(route);
        rightFuture.then((value) {
          final associatedBarrier = _popupToBarrier.remove(route);
          if (associatedBarrier != null &&
              manager.leftNavigator != null &&
              manager.leftNavigator!.canPop()) {
            (manager.leftNavigator! as NavigatorState).pop();
            _manager.removeProcessedPopupRoute(associatedBarrier);
          }
          _manager.removeProcessedPopupRoute(route);
          _manager.setPoprouteOnScreen(false);
          if (previousFocus != null && previousFocus.context != null) {
            previousFocus.requestFocus();
          }
        });

        return rightFuture;
      }
    }
    return super.push(route);
  }

  @override
  Future<T?> pushAndRemoveUntil<T extends Object?>(Route<T> newRoute, RoutePredicate predicate) {
    if (_manager.isHomePageReady) {
      final routeName = newRoute.settings.name;
      if (routeName == _manager.realHomePage) {
        _manager.setRightSideShowsPlaceholder(true);
        return super.pushAndRemoveUntil<T>(
          PageRouteBuilder(
            settings: const RouteSettings(name: SplitStartPage.routeName),
            pageBuilder: (context, animation, secondaryAnimation) => const SplitStartPage(),
          ),
          predicate,
        );
      }
    }
    return super.pushAndRemoveUntil<T>(newRoute, predicate);
  }

  @override
  Future<bool> maybePop<T extends Object?>([T? result]) async {
    final canPopResult = super.canPop();

    if (canPopResult) {
      return super.maybePop(result);
    }
    return false;
  }

  @override
  bool pop<T extends Object?>([T? result]) {
    final canPopResult = super.canPop();
    if (canPopResult) {
      super.pop(result);
      return true;
    }
    return false;
  }

  Route<T> _createBarrierRoute<T>(PopupRoute route) {
    return _BarrierOnlyRoute<T>(
      barrierColor: route.barrierColor,
      barrierDismissible: false,
      barrierLabel: route.barrierLabel,
    );
  }
}

class _RightSideNavigatorObserver extends NavigatorObserver {
  Route<dynamic>? _currentRoute;

  @override
  void didPush(Route<dynamic> route, Route<dynamic>? previousRoute) {
    if (previousRoute != null &&
        SplitStartPage.isSplitStartPageRoute(previousRoute.settings) &&
        !SplitStartPage.isSplitStartPageRoute(route.settings) && route is! PopupRoute) {
      _notifyLeftSideDidPushNext();
    }

    final manager = SplitViewManager();
    if (route is! PopupRoute) {
      manager.setRightSideShowsPlaceholder(
          SplitStartPage.isSplitStartPageRoute(route.settings));
    }
    final config = SplitViewConfig();
    if (config.isForceFullscreenRoute(route.settings.name)) {
      manager.setForceFullscreen(true);
    } else {
      manager.setForceFullscreen(false);
    }
    _currentRoute = route;
  }

  @override
  void didPop(Route<dynamic> route, Route<dynamic>? previousRoute) {
    final manager = SplitViewManager();
    if (route is! PopupRoute) {
      manager.setRightSideShowsPlaceholder(
          SplitStartPage.isSplitStartPageRoute(previousRoute?.settings));
    }

    if (!SplitStartPage.isSplitStartPageRoute(route.settings) && route is! PopupRoute) {
      if (previousRoute == null ||
          (previousRoute != null && SplitStartPage.isSplitStartPageRoute(previousRoute.settings))) {
        _notifyLeftSideDidPopNext();
      }
    }

    final config = SplitViewConfig();
    if (previousRoute == null ||
        !config.isForceFullscreenRoute(previousRoute?.settings.name)) {
      manager.setForceFullscreen(false);
    } else {
      manager.setForceFullscreen(true);
    }

    _currentRoute = previousRoute;
  }

  @override
  void didReplace({Route<dynamic>? newRoute, Route<dynamic>? oldRoute}) {
    final manager = SplitViewManager();
    if (newRoute is! PopupRoute) {
      manager.setRightSideShowsPlaceholder(
          SplitStartPage.isSplitStartPageRoute(newRoute?.settings));
    }
    final config = SplitViewConfig();
    if (newRoute == null ||
        !config.isForceFullscreenRoute(newRoute?.settings.name)) {
      manager.setForceFullscreen(false);
    } else {
      manager.setForceFullscreen(true);
    }

    _currentRoute = newRoute;
  }

  void _notifyLeftSideDidPushNext() {
    final manager = SplitViewManager();
    final observers = manager.globalRouteObservers;
    final leftCurrentRoute = manager.leftCurrentRoute;

    if (observers.isEmpty || leftCurrentRoute == null) {
      return;
    }

    final virtualRoute = _VirtualSplitScreenRoute();

    for (final observer in observers) {
      try {
        observer.didPush(virtualRoute, leftCurrentRoute);
      } catch (e) {
        debugPrint('SplitScreen: Error notifying left side didPushNext: $e');
      }
    }
  }

  /// Notify left side RouteAware's didPopNext is triggered
  void _notifyLeftSideDidPopNext() {
    final manager = SplitViewManager();
    final observers = manager.globalRouteObservers;
    final leftCurrentRoute = manager.leftCurrentRoute;

    if (observers.isEmpty || leftCurrentRoute == null) {
      return;
    }

    final virtualRoute = _VirtualSplitScreenRoute();

    for (final observer in observers) {
      try {
        observer.didPop(virtualRoute, leftCurrentRoute);
      } catch (e) {
        debugPrint('SplitScreen: Error notifying left side didPopNext: $e');
      }
    }
  }

  Route<dynamic>? getCurrentRoute() => _currentRoute;
}

class _VirtualSplitScreenRoute extends ModalRoute<void> {
  @override
  Color? get barrierColor => null;

  @override
  String? get barrierLabel => null;

  @override
  bool get barrierDismissible => false;

  @override
  bool get maintainState => false;

  @override
  bool get opaque => false;

  @override
  Duration get transitionDuration => Duration.zero;

  @override
  Widget buildPage(BuildContext context, Animation<double> animation,
      Animation<double> secondaryAnimation) {
    return const SizedBox.shrink();
  }

  @override
  Widget buildTransitions(BuildContext context, Animation<double> animation,
      Animation<double> secondaryAnimation, Widget child) {
    return const SizedBox.shrink();
  }
}

/// Barrier-only Route - used to disable interaction on left side, displays no actual content
class _BarrierOnlyRoute<T> extends PopupRoute<T> {
  final Color? barrierColor;
  final bool barrierDismissible;
  final String? barrierLabel;
  final VoidCallback? onBarrierDismissed;

  _BarrierOnlyRoute({
    this.barrierColor = const Color(0x80000000),
    this.barrierDismissible = true,
    this.barrierLabel,
    this.onBarrierDismissed,
  }) : super(requestFocus: false);

  @override
  Color? get barrierColorValue => barrierColor;

  @override
  bool get opaque => false;

  @override
  bool get maintainState => false;

  @override
  Duration get transitionDuration => Duration.zero;

  @override
  Widget buildPage(
    BuildContext context,
    Animation<double> animation,
    Animation<double> secondaryAnimation,
  ) {
    return const SizedBox.shrink();
  }

  @override
  Widget buildTransitions(
    BuildContext context,
    Animation<double> animation,
    Animation<double> secondaryAnimation,
    Widget child,
  ) {
    return child;
  }

  @override
  void didPopNext(Route nextRoute) {
    // When right side dialog is closed, also close left side barrier
    super.didPopNext(nextRoute);
  }

  @override
  bool didPop(T? result) {
    // When barrier is closed, no need to actively pop right side
    // Because right side dialog is already in its own pop process
    // Left side barrier will close automatically when right dialog closes (handled by then callback in push)
    return super.didPop(result);
  }
}

/// Split start page - displayed in right panel while waiting for left navigation
/// This is a globally unique Widget, can be used to check if current page is split start page
class SplitStartPage extends StatelessWidget {
  static const String routeName = '/split_start_page';

  const SplitStartPage({Key? key}) : super(key: key);

  static bool isSplitStartPage(Widget? widget) {
    return widget is SplitStartPage;
  }

  static bool isSplitStartPageRoute(RouteSettings? settings) {
    return settings?.name == routeName;
  }

  @override
  Widget build(BuildContext context) {
    final bytes = SplitViewConfig().placeholderIconBytes;

    Widget content;
    if (bytes != null) {
      content = Image.memory(
        bytes,
        width: 48,
        height: 48,
        fit: BoxFit.contain,
      );
    } else {
      content = const Text(
        'Split Start Page',
        style: TextStyle(
          fontSize: 16,
          color: Color(0xFF000000),
          decoration: TextDecoration.none,
        ),
      );
    }

    return Container(
      color: const Color(0xFFf1f3f5),
      child: Center(
        child: content,
      ),
    );
  }
}
