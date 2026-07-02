// Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE_HW file.

part of 'navigator.dart';

/// Stores information about a mirror barrier created for a [PopupRoute]
/// in split-view mode.
///
/// When a popup route is displayed on the right side of the split view,
/// a mirror barrier is created and added to the left overlay to prevent
/// user interaction with the home page while the popup is active.
class MirrorBarrierInfo {
  /// Creates a [MirrorBarrierInfo] with the given [mirrorBarrier].
  MirrorBarrierInfo({required this.mirrorBarrier});

  /// The mirror barrier overlay entry.
  final OverlayEntry mirrorBarrier;
}

/// InheritedWidget used to mark that a [Navigator] is already inside a
/// split-view container.
///
/// This is the Navigator-level equivalent of the Router-level
/// `isInsideSplitViewContainer()` check in `split_view_container.dart`.
class _SplitViewScope extends InheritedWidget {
  const _SplitViewScope({required super.child});

  /// Returns `true` if the given [context] is inside a [_SplitViewScope].
  static bool isInsideSplitView(BuildContext context) {
    return context.getElementForInheritedWidgetOfExactType<_SplitViewScope>() != null;
  }

  @override
  bool updateShouldNotify(_SplitViewScope oldWidget) => false;
}

/// Encapsulates all split-view logic for [NavigatorState].
///
/// This policy class is instantiated only on ohos platforms
/// (`defaultTargetPlatform == TargetPlatform.ohos`), so non-ohos platforms
/// have zero overhead. Because it lives in the same file as [NavigatorState],
/// it can access private types like [_RouteEntry], [_History],
/// [_RouteLifecycle], and private members of [NavigatorState].
class SplitViewNavigatorPolicy with WidgetsBindingObserver {
  /// Creates a [SplitViewNavigatorPolicy] with the given [_navigator].
  SplitViewNavigatorPolicy(this._navigator);

  /// Returns whether split-view is enabled via [SplitViewConfig].
  ///
  /// This static getter allows [NavigatorState] to check split-view
  /// enablement before creating a [SplitViewNavigatorPolicy] instance.
  static bool get isSplitViewEnabled => SplitViewConfig().isEnabled;

  final NavigatorState _navigator;

  late GlobalKey<OverlayState> _rightOverlayKey;
  OverlayState? get _rightOverlay => _rightOverlayKey.currentState;

  /// The right overlay state, exposed for external access.
  OverlayState? get rightOverlay => _rightOverlay;

  final Map<Route<dynamic>, MirrorBarrierInfo> _mirrorBarrierRoutes =
      <Route<dynamic>, MirrorBarrierInfo>{};

  bool _isSplitViewActive = false;
  bool _splitViewVisible = false;
  bool _homePageReady = false;

  Route<dynamic>? _callerRoute;

  _RouteEntry? _cachedHomePageEntry;

  /// Whether there is more than one present route entry whose name matches
  /// the home page name in [_history]. Updated by [updateHomePageCache].
  bool _hasOtherHomePage = false;

  /// Whether the home page is ready in split-view mode.
  bool get isHomePageReady => _homePageReady;

  /// Initializes the split-view policy.
  void init() {
    _rightOverlayKey = GlobalKey<OverlayState>();
    WidgetsBinding.instance.addObserver(this);

    SplitViewManager().reset();

    _isSplitViewActive = _checkScreenSizeAndSetSplitScreen();
    SplitViewManager().setSplitViewActive(_isSplitViewActive);
    final manager = SplitViewManager();
    _splitViewVisible =
        !manager.isForceFullscreen &&
        !(manager.isForcedLandscape && SplitViewConfig().supportLandscapeFullscreen) &&
        _isSplitViewActive;
    SplitViewManager().addListener(_onSplitViewManagerChanged);
  }

  /// Disposes the split-view policy.
  void dispose() {
    WidgetsBinding.instance.removeObserver(this);
    SplitViewManager().removeListener(_onSplitViewManagerChanged);
    for (final MirrorBarrierInfo info in _mirrorBarrierRoutes.values) {
      info.mirrorBarrier.remove();
    }
    _mirrorBarrierRoutes.clear();
  }

  /// Resets the overlay keys.
  void resetOverlayKeys() {
    _rightOverlayKey = GlobalKey<OverlayState>();
  }

  /// Sets the caller route for split-view push interception.
  /// This records which route is calling Navigator.of, so that when
  /// a push operation follows, we can determine if it originates from
  /// the home page.
  void setCallerRoute(BuildContext context) {
    _callerRoute = ModalRoute.of(context);
  }

  /// Clears the caller route after a push operation completes.
  void clearCallerRoute() {
    _callerRoute = null;
  }

  /// Returns the split overlay entries for left and right panels.
  ({List<OverlayEntry> left, List<OverlayEntry> right}) splitOverlayEntries() {
    return _splitOverlayEntries();
  }

  /// Rearranges the left and right overlays to reflect the current split-view
  /// state.
  ///
  /// Before rearranging, entries that need to migrate between overlays are
  /// removed from their current overlay. This is required because [rearrange]
  /// uses append semantics rather than replace, and its assert requires
  /// `entry._overlay` to be null or point to the same overlay. Without prior
  /// removal, the assert 'already present in another Overlay' would fire.
  ///
  /// Performance note: `containsEntry` and `remove` are both O(n) List
  /// operations, making the overall complexity O(n²). With typical route stack
  /// sizes (10–60 entries) and since this runs only on navigation operations
  /// (not every build), the overhead is negligible. If future route stacks
  /// grow significantly, consider adding a Set-based batch removal method to
  /// [OverlayState] (e.g. `removeWhereInSet`) to reduce lookups to O(1).
  void rearrangeSplitOverlays() {
    final ({List<OverlayEntry> left, List<OverlayEntry> right}) split = _splitOverlayEntries();
    final OverlayState? leftOverlay = _navigator.overlay;
    final OverlayState? rightOverlay = _rightOverlay;
    if (leftOverlay != null) {
      for (final OverlayEntry entry in split.right) {
        if (leftOverlay.containsEntry(entry)) {
          entry.remove();
        }
      }
    }
    if (rightOverlay != null) {
      for (final OverlayEntry entry in split.left) {
        if (rightOverlay.containsEntry(entry)) {
          entry.remove();
        }
      }
    }
    leftOverlay?.rearrange(split.left);
    rightOverlay?.rearrange(split.right);
  }

  /// Disposes the mirror barrier for the given entry if it is a [PopupRoute].
  void disposeMirrorBarrierFor(_RouteEntry entry) {
    if (entry.route is PopupRoute && _mirrorBarrierRoutes.containsKey(entry.route)) {
      final MirrorBarrierInfo mirrorInfo = _mirrorBarrierRoutes.remove(entry.route)!;
      mirrorInfo.mirrorBarrier.remove();
    }
  }

  /// Returns the home page name if split-view is active and home page is ready.
  /// Returns null otherwise.
  String? getHomePageNameIfReady() {
    return _homePageReady ? SplitViewManager().realHomePage : null;
  }

  /// Whether the given entry is the home page and effective routes should be skipped.
  /// In split-view mode when the home page is on top of the left side,
  /// we skip setting effectiveNextRoute for it.
  bool shouldSkipEffectiveRoutesForEntry(_RouteEntry entry) {
    return _isHomePageTopOnLeft && entry == _cachedHomePageEntry;
  }

  /// Determines whether a pop operation should be blocked because it
  /// originates from the home page in split-view mode.
  ///
  /// This method handles two distinct scenarios:
  ///
  /// **Scenario A — Home page is at the top of the stack (e.g. after
  /// `deduplicateRouteStack` moved it up):**
  /// - If there is another home page entry deeper in the stack, the pop
  ///   is **not blocked** — the top home page is simply removed, and
  ///   the deeper home page naturally takes its place on the left side.
  /// - If the sole home page handles pop internally
  ///   ([Route.willHandlePopInternally] is true), the pop is **not
  ///   blocked** — the standard pop/maybePop flow already handles this
  ///   correctly (popDisposition=doNotPop, popGestureEnabled=false).
  /// - Otherwise (sole home page, willHandlePopInternally=false), the
  ///   pop is **blocked** and [onPopInvokedWithResult] is called to
  ///   notify the route, matching Flutter's official doNotPop behavior.
  ///
  /// **Scenario B — `_callerRoute` is the home page (pop originates from
  /// the home page, e.g. a back button on the left side):**
  /// - The pop is **blocked** because the home page on the left side
  ///   should not pop routes on the right side.
  /// - If the home page handles pop internally, [didPop] is called to
  ///   let it dismiss internal overlays (e.g. bottom sheets). When
  ///   [didPop] succeeds, no further notification is needed.
  /// - Otherwise, [onPopInvokedWithResult] is called to notify the route.
  bool shouldBlockPopFromHomePageByRoute(Object? result) {
    if (!_homePageReady) {
      return false;
    }
    final _RouteEntry? homePageEntry = _cachedHomePageEntry;
    if (homePageEntry == null) {
      return false;
    }

    // Scenario: top entry is a popup → allow pop (popups should close normally).
    final _RouteEntry? topEntry = _navigator._lastRouteEntryWhereOrNull(
      _RouteEntry.isPresentPredicate,
    );
    if (topEntry != null && topEntry.route is PopupRoute) {
      return false;
    }

    // Scenario A: sole home page at top → block pop and notify route.
    if (topEntry != null && topEntry == homePageEntry) {
      if (_hasOtherHomePage || topEntry.route.willHandlePopInternally) {
        return false;
      }
      topEntry.route.onPopInvokedWithResult(false, result);
      return true;
    }

    // Scenario B: pop from home page → block and notify via didPop/onPopInvokedWithResult.
    final Route<dynamic>? callerRoute = _callerRoute;
    if (callerRoute != null && callerRoute == homePageEntry.route) {
      final bool didHandle = callerRoute.willHandlePopInternally && callerRoute.didPop(result);
      if (!didHandle) {
        callerRoute.onPopInvokedWithResult(false, result);
      }
      return true;
    }

    return false;
  }

  /// Determines whether [NavigatorState.maybePop] should bubble up to the
  /// parent navigator instead of attempting a pop that would be blocked.
  ///
  /// Returns true only when:
  /// - Split view is active and the home page is ready.
  /// - The top of the stack is a home page entry.
  /// - There is no other home page entry deeper in the stack.
  /// - The top home page route does NOT handle pop internally
  ///   ([Route.willHandlePopInternally] is false).
  ///
  /// When the top home page handles pop internally (e.g. it has an open
  /// bottom sheet to dismiss first), we should NOT bubble up — instead,
  /// [maybePop] should proceed to [pop], which will be intercepted by
  /// [shouldBlockPopFromHomePageByRoute]. The interception calls
  /// [Route.didPop] so the route can dismiss its internal overlays
  /// (e.g. bottom sheets) before the next back gesture bubbles up.
  bool shouldBubbleUpFromHomePage() {
    if (!_homePageReady) {
      return false;
    }
    final _RouteEntry? topEntry = _navigator._lastRouteEntryWhereOrNull(
      _RouteEntry.isPresentPredicate,
    );
    if (topEntry != null &&
        topEntry == _cachedHomePageEntry &&
        !_hasOtherHomePage &&
        !topEntry.route.willHandlePopInternally) {
      return true;
    }
    return false;
  }

  /// Determines whether a push operation should clear the right-side stack,
  /// based on the [_callerRoute] captured via [Navigator.of] / [Navigator.maybeOf].
  ///
  /// Returns true only when:
  /// - Split view is active and the home page is ready.
  /// - [_callerRoute] matches the home page route (push originates from home).
  /// - There are detail entries after the home page that need to be removed.
  /// - The entry being pushed is NOT a [PopupRoute] (popups should overlay,
  ///   not clear the right side).
  ///
  /// When a popup is pushed from the home page while detail pages are on
  /// the right, the popup should appear on the right side without clearing
  /// the detail pages. The [MirrorBarrier] mechanism ensures the popup's
  /// barrier covers the left side while the popup content displays on right.
  bool shouldClearRightStackOnPushByRoute(_RouteEntry entry) {
    if (!_isPushFromHomePage()) return false;
    // Popup routes should overlay the current layout without clearing
    // the right side. When a popup is pushed from home, it should appear
    // on the right with MirrorBarrier covering the left.
    if (entry.route is PopupRoute) return false;
    return true;
  }

  /// Whether a navigation operation (pushReplacement, pushAndRemoveUntil, etc.)
  /// originating from the home page should be converted to a plain push in
  /// split-view mode.
  ///
  /// Unlike [shouldClearRightStackOnPushByRoute], this method returns true
  /// even when there are no detail entries on the right side. This is
  /// necessary because operations like pushReplacement and pushAndRemoveUntil
  /// would otherwise replace or remove the home page itself — the home page
  /// must always remain on the left side of the split view.
  bool shouldInterceptNavigationFromHomePage() {
    return _isPushFromHomePage();
  }

  /// Whether a push operation should clear all entries in the stack,
  /// including the home page itself. This happens when pushing a new home page
  /// (route name matches the home page name) from a detail page.
  ///
  /// Returns true only when:
  /// - The home page is ready.
  /// - The target entry's route name matches the home page name.
  bool shouldClearAllStackOnPushByRoute(_RouteEntry entry) {
    if (!_homePageReady) {
      return false;
    }
    final _RouteEntry? homePageEntry = _cachedHomePageEntry;
    if (homePageEntry == null) {
      return false;
    }
    return entry.route.settings.name == homePageEntry.route.settings.name;
  }

  /// Removes all detail entries on the right side of the split view by marking
  /// them for pop. Called before a new route is pushed from the home page so
  /// the right side is cleared and the new page can take over.
  ///
  /// Must be called while the navigator is not locked (i.e. outside of
  /// [_flushHistoryUpdates]). The marked entries will be processed together
  /// with the new push entry in a single [_flushHistoryUpdates] call inside
  /// [_pushEntry].
  void clearRightEntriesForPush() {
    assert(!_navigator._debugLocked);
    final _RouteEntry? homePageEntry = _cachedHomePageEntry;
    if (homePageEntry == null) {
      return;
    }
    var pastHomePage = false;
    for (final _RouteEntry entry in _navigator._history) {
      if (entry == homePageEntry) {
        pastHomePage = true;
        continue;
      }
      if (pastHomePage && _RouteEntry.isPresentPredicate(entry) && entry.route is! PopupRoute) {
        entry.pop<dynamic>(null, imperativeRemoval: true);
      }
    }
  }

  /// Clears all present entries in the stack, including the home page.
  /// Called when pushing a new home page from a detail page, so the new home
  /// page becomes the sole entry displayed on the left side.
  ///
  /// Must be called while the navigator is not locked (i.e. outside of
  /// [_flushHistoryUpdates]). Uses `complete` instead of `pop` to skip
  /// exit animations for immediate transition.
  void clearAllEntriesForPush() {
    assert(!_navigator._debugLocked);
    // Clear ALL present entries (including old home page and detail pages).
    // When pushing a new home page, the old home page must be removed so
    // the new home page becomes the sole entry displayed on the left side.
    for (final _RouteEntry entry in _navigator._history) {
      if (_RouteEntry.isPresentPredicate(entry) && entry.route is! PopupRoute) {
        entry.complete<dynamic>(null, isReplaced: false, imperativeRemoval: true);
      }
    }
  }

  /// Updates the force fullscreen state based on the given route settings.
  void updateForceFullscreen(RouteSettings? routeSettings) {
    final config = SplitViewConfig();
    final bool isForceFullscreen = config.isForceFullscreenRoute(routeSettings?.name);
    SplitViewManager().setForceFullscreen(isForceFullscreen);
    final bool isForcedLandscape =
        SplitViewManager().isForcedLandscape && config.supportLandscapeFullscreen;
    final bool newVisible = !isForceFullscreen && !isForcedLandscape && _isSplitViewActive;
    if (_splitViewVisible != newVisible) {
      _splitViewVisible = newVisible;
    }
  }

  /// Updates the home page ready state.
  void updateHomePageReady() {
    if (!_homePageReady && _isHomePageTopOnLeft) {
      _homePageReady = true;
    }
  }

  /// Updates the cached home page entry by traversing history.
  /// Called at the end of [_flushHistoryUpdates] to ensure the cache
  /// reflects the latest history state for subsequent navigation operations.
  void updateHomePageCache() {
    final String? homePageName = SplitViewManager().realHomePage;
    if (homePageName == null) {
      _cachedHomePageEntry = _navigator._firstRouteEntryWhereOrNull(_RouteEntry.isPresentPredicate);
      _hasOtherHomePage = false;
      return;
    }
    _RouteEntry? homePageEntry;
    int homePageCount = 0;
    for (final _RouteEntry entry in _navigator._history) {
      if (entry.willBePresent && entry.route.settings.name == homePageName) {
        homePageEntry = entry;
        homePageCount++;
      }
    }
    _cachedHomePageEntry = homePageEntry;
    _hasOtherHomePage = homePageCount > 1;
  }

  /// Builds the overlay layout widget for split-view.
  Widget buildOverlayLayout() {
    return _buildSplitViewOverlay();
  }

  @override
  void didChangeMetrics() {
    _updateSplitViewActive();
  }

  bool _checkScreenSizeAndSetSplitScreen() {
    if (defaultTargetPlatform != TargetPlatform.ohos) {
      return false;
    }

    final Size physicalSize = WidgetsBinding.instance.window.physicalSize;
    final double devicePixelRatio = WidgetsBinding.instance.window.devicePixelRatio;
    if (physicalSize == Size.zero || devicePixelRatio == 0.0) {
      return false;
    }

    final double logicalWidth = physicalSize.width / devicePixelRatio;
    final double logicalHeight = physicalSize.height / devicePixelRatio;
    const double splitScreenWidthThreshold = 600;

    if (logicalWidth < splitScreenWidthThreshold || logicalHeight < splitScreenWidthThreshold) {
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

  void _updateSplitViewActive() {
    final bool newActive = _checkScreenSizeAndSetSplitScreen();
    SplitViewManager().setSplitViewActive(newActive);

    final manager = SplitViewManager();
    final bool isForceFullscreen = manager.isForceFullscreen;
    final bool isForcedLandscape =
        manager.isForcedLandscape && SplitViewConfig().supportLandscapeFullscreen;
    final bool newVisible = !isForceFullscreen && !isForcedLandscape && newActive;
    if (_isSplitViewActive != newActive || _splitViewVisible != newVisible) {
      _navigator.setState(() {
        _isSplitViewActive = newActive;
        _splitViewVisible = newVisible;
      });
    }
  }

  void _onSplitViewManagerChanged() {
    if (SchedulerBinding.instance.schedulerPhase == SchedulerPhase.persistentCallbacks) {
      return;
    }
    _updateSplitViewActive();
  }

  bool _isPushFromHomePage() {
    if (!_homePageReady) return false;
    final _RouteEntry? homePageEntry = _cachedHomePageEntry;
    if (_callerRoute == null || homePageEntry == null) return false;
    return _callerRoute == homePageEntry.route;
  }

  bool get _isHomePageTopOnLeft {
    if (_homePageReady) return true;
    return _cachedHomePageEntry != null &&
        _cachedHomePageEntry ==
            _navigator._lastRouteEntryWhereOrNull(_RouteEntry.isPresentPredicate);
  }

  bool _hasDetailEntriesAfterHomePage(_RouteEntry homePageEntry) {
    var pastHomePage = false;
    for (final _RouteEntry entry in _navigator._history) {
      if (entry == homePageEntry) {
        pastHomePage = true;
        continue;
      }
      if (pastHomePage && _RouteEntry.isPresentPredicate(entry) && entry.route is! PopupRoute) {
        return true;
      }
    }
    return false;
  }

  ({List<OverlayEntry> left, List<OverlayEntry> right}) _splitOverlayEntries() {
    final left = <OverlayEntry>[];
    final right = <OverlayEntry>[];

    if (!_homePageReady) {
      for (final _RouteEntry entry in _navigator._history) {
        if (entry.route is PopupRoute) {
          _assignPopupRouteEntries(entry.route as PopupRoute, left, right, popupOnLeft: true);
        } else {
          left.addAll(entry.route.overlayEntries);
        }
      }
      return (left: left, right: right);
    }

    final _RouteEntry? homePageEntry = _cachedHomePageEntry;
    if (homePageEntry == null) {
      // Fallback: home page not found (e.g., transitioning between home pages).
      // Put all entries on the left overlay as a safe default.
      for (final _RouteEntry entry in _navigator._history) {
        if (entry.route is PopupRoute) {
          _assignPopupRouteEntries(entry.route as PopupRoute, left, right, popupOnLeft: true);
        } else {
          left.addAll(entry.route.overlayEntries);
        }
      }
      return (left: left, right: right);
    }

    final bool rightHasDetail = _hasDetailEntriesAfterHomePage(homePageEntry);

    var pastHomePage = false;
    for (final _RouteEntry entry in _navigator._history) {
      if (entry == homePageEntry) {
        left.addAll(entry.route.overlayEntries);
        pastHomePage = true;
      } else if (entry.route is PopupRoute) {
        final bool popupOnLeft = !rightHasDetail;
        _assignPopupRouteEntries(entry.route as PopupRoute, left, right, popupOnLeft: popupOnLeft);
      } else if (!pastHomePage) {
        left.addAll(entry.route.overlayEntries);
      } else if (pastHomePage && entry.route.overlayEntries.isNotEmpty) {
        // An exiting home page entry should stay on the left overlay to cover
        // the gap during the first home page's secondaryAnimation recovery.
        if (!_RouteEntry.isPresentPredicate(entry) &&
            entry.route.settings.name == homePageEntry.route.settings.name) {
          left.addAll(entry.route.overlayEntries);
        } else {
          right.addAll(entry.route.overlayEntries);
        }
      }
    }

    return (left: left, right: right);
  }

  void _assignPopupRouteEntries(
    PopupRoute<dynamic> popupRoute,
    List<OverlayEntry> left,
    List<OverlayEntry> right, {
    required bool popupOnLeft,
  }) {
    if (popupOnLeft) {
      left.addAll(popupRoute.overlayEntries);
    } else {
      right.addAll(popupRoute.overlayEntries);
    }

    if (!_mirrorBarrierRoutes.containsKey(popupRoute)) {
      final Color? barrierColor = popupRoute.barrierColor;
      final String? barrierLabel = popupRoute.barrierLabel;
      final mirrorBarrier = OverlayEntry(
        builder: (BuildContext context) {
          Widget barrier;
          if (barrierColor != null && barrierColor.alpha != 0) {
            final Animation<Color?> color = popupRoute.animation!.drive(
              ColorTween(
                begin: barrierColor.withOpacity(0.0),
                end: barrierColor,
              ).chain(CurveTween(curve: popupRoute.barrierCurve)),
            );
            barrier = AnimatedModalBarrier(
              color: color,
              dismissible: false,
              semanticsLabel: barrierLabel,
              barrierSemanticsDismissible: false,
            );
          } else {
            barrier = ModalBarrier(
              dismissible: false,
              semanticsLabel: barrierLabel,
              barrierSemanticsDismissible: false,
            );
          }
          return barrier;
        },
      );
      _mirrorBarrierRoutes[popupRoute] = MirrorBarrierInfo(mirrorBarrier: mirrorBarrier);
    }

    // Use the current popupOnLeft parameter to determine barrier placement,
    // rather than a cached value, since popupOnLeft may change dynamically
    // (e.g., when rightHasDetail changes during the popup's lifetime).
    final MirrorBarrierInfo mirrorInfo = _mirrorBarrierRoutes[popupRoute]!;
    if (popupOnLeft) {
      right.add(mirrorInfo.mirrorBarrier);
    } else {
      left.add(mirrorInfo.mirrorBarrier);
    }
  }

  Widget _buildPlaceholder(BuildContext context) {
    final SplitViewPlaceholderBuilder? builder = SplitViewManager().placeholderBuilder;
    if (builder != null) {
      return builder(context);
    }
    return const ColoredBox(color: Color(0xFFf1f3f5));
  }

  Widget _buildSplitViewOverlay() {
    // Only compute split entries when overlays haven't been created yet
    // (first build). On subsequent builds, initialEntries is ignored by
    // OverlayState, so computing split would be wasted work.
    final bool needSplit = _navigator.overlay == null || _rightOverlay == null;
    final ({List<OverlayEntry> left, List<OverlayEntry> right})? split = needSplit
        ? _splitOverlayEntries()
        : null;

    final bool rightShowsPlaceholder;
    if (!_isHomePageTopOnLeft) {
      rightShowsPlaceholder = true;
    } else {
      rightShowsPlaceholder =
          _cachedHomePageEntry == null || !_hasDetailEntriesAfterHomePage(_cachedHomePageEntry!);
    }

    final bool leftVisible = _splitViewVisible || rightShowsPlaceholder;
    final bool rightVisible = _splitViewVisible || !rightShowsPlaceholder;

    final bool shouldReduceSize = _splitViewVisible && SplitViewConfig().enableReducedContainerSize;
    final MediaQueryData mediaQueryData = MediaQuery.of(_navigator.context);
    final MediaQueryData updatedMediaQueryData = mediaQueryData.copyWith(
      enableSplitView: shouldReduceSize,
    );

    return _SplitViewScope(
      child: MediaQuery(
        data: updatedMediaQueryData,
        child: Row(
          children: <Widget>[
            Expanded(
              flex: _splitViewVisible ? 50 : (rightShowsPlaceholder ? 100 : 0),
              child: ClipRect(
                child: Visibility(
                  visible: leftVisible,
                  maintainState: true,
                  child: SizedBox(
                    width: leftVisible ? null : 0,
                    height: leftVisible ? null : 0,
                    child: Stack(
                      children: <Widget>[
                        const Positioned.fill(child: ColoredBox(color: Color(0xfff1f3f5))),
                        Overlay(
                          key: _navigator._overlayKey,
                          clipBehavior: _navigator.widget.clipBehavior,
                          initialEntries: _navigator.overlay == null
                              ? split!.left
                              : const <OverlayEntry>[],
                        ),
                      ],
                    ),
                  ),
                ),
              ),
            ),
            Visibility(
              visible: _splitViewVisible,
              child: const SizedBox(width: 1, child: ColoredBox(color: Color(0x33000000))),
            ),
            Expanded(
              flex: _splitViewVisible ? 50 : (rightShowsPlaceholder ? 0 : 100),
              child: ClipRect(
                child: Visibility(
                  visible: rightVisible,
                  maintainState: true,
                  child: SizedBox(
                    width: rightVisible ? null : 0,
                    height: rightVisible ? null : 0,
                    child: Stack(
                      children: <Widget>[
                        Positioned.fill(child: _buildPlaceholder(_navigator.context)),
                        Overlay(
                          key: _rightOverlayKey,
                          clipBehavior: _navigator.widget.clipBehavior,
                          initialEntries: _rightOverlay == null
                              ? split!.right
                              : const <OverlayEntry>[],
                        ),
                      ],
                    ),
                  ),
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
