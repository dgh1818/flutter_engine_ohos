// Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE_HW file.

import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';
import 'package:flutter/src/widgets/split_view_config.dart';
import 'package:flutter/src/widgets/split_view_manager.dart';
import 'package:flutter/widgets.dart';
import 'package:flutter_test/flutter_test.dart';

/// Helper to build a simple page widget with identifiable text.
/// The body text is 'Page $id' (used for find.text assertions).
Widget _buildPage(String id) {
  return Container(
    alignment: Alignment.center,
    color: const Color(0xFFFFFFFF),
    child: Text('Page $id', style: const TextStyle(fontSize: 32)),
  );
}

/// Wraps a child with the minimal dependencies needed by [Navigator]:
/// [MediaQuery] and [Directionality].
///
/// This avoids importing Material (which [MaterialApp] would require),
/// keeping the test in the Widgets library scope.
class _TestDependencies extends StatelessWidget {
  const _TestDependencies({required this.child});

  final Widget child;

  @override
  Widget build(BuildContext context) {
    return MediaQuery(
      data: MediaQueryData.fromView(View.of(context)),
      child: Directionality(textDirection: TextDirection.ltr, child: child),
    );
  }
}

/// Builds a minimal split-view test app using pure Widgets-layer APIs.
///
/// [routes] maps route names to page builders. [initialRoute] is the
/// starting route. The app uses [Navigator] with [onGenerateRoute] instead
/// of [MaterialApp], so no Material import is needed.
Widget _buildSplitViewApp({String? initialRoute, required Map<String, WidgetBuilder> routes}) {
  return _TestDependencies(
    child: Navigator(
      initialRoute: initialRoute,
      onGenerateRoute: (RouteSettings settings) {
        final WidgetBuilder? builder = routes[settings.name];
        if (builder != null) {
          return PageRouteBuilder<void>(
            settings: settings,
            pageBuilder: (BuildContext context, Animation<double> _, Animation<double> _) =>
                builder(context),
          );
        }
        return null;
      },
    ),
  );
}

/// Helper that configures [SplitViewConfig] for testing.
void _configureSplitView({
  String? homePage,
  List<String> fullScreenPages = const <String>[],
  bool enableWideWindowSplit = true,
  bool enableSquareWindowSplit = false,
}) {
  final config = SplitViewConfig();
  config.reset();
  config.setEnableWideWindowSplit(enableWideWindowSplit);
  config.setEnableSquareWindowSplit(enableSquareWindowSplit);
  config.setHomePage(homePage);
  config.setFullScreenPages(fullScreenPages);
}

/// Sets up the test environment for split-view testing:
/// - Configures SplitViewConfig
/// - Sets a wide screen size (1200x900 logical pixels)
/// - Sets SplitViewManager realHomePage
///
/// Platform override must be handled by the caller via
/// `variant: TargetPlatformVariant.only(TargetPlatform.ohos)`.
void _setupSplitViewEnvironment(
  WidgetTester tester, {
  String? homePage,
  List<String> fullScreenPages = const <String>[],
  bool enableWideWindowSplit = true,
  bool enableSquareWindowSplit = false,
  Size screenSize = const Size(1200, 900),
}) {
  _configureSplitView(
    homePage: homePage,
    fullScreenPages: fullScreenPages,
    enableWideWindowSplit: enableWideWindowSplit,
    enableSquareWindowSplit: enableSquareWindowSplit,
  );
  SplitViewManager().reset();
  SplitViewManager().setRealHomePage(homePage);
  SplitViewManager().initDefaultPlaceholderBuilder();

  // Set a wide screen to trigger split-view.
  tester.view.physicalSize = screenSize * tester.view.devicePixelRatio;
  addTearDown(tester.view.reset);
  addTearDown(_resetSplitViewSingletons);
}

/// Resets split-view singletons (but NOT debugDefaultTargetPlatformOverride,
/// which is managed by [TargetPlatformVariant]).
///
/// [SplitViewManager.reset] intentionally does NOT clear `realHomePage`
/// (it is determined by the app layer), so we clear it explicitly here to
/// ensure test isolation when tests run in randomized order.
void _resetSplitViewSingletons() {
  SplitViewConfig().reset();
  SplitViewManager().reset();
  SplitViewManager().setRealHomePage(null);
  SplitViewConfigLoader().reset();
}

void main() {
  // ─── SplitViewConfig tests ──────────────────────────────────────────────

  group('SplitViewConfig', () {
    setUp(() {
      SplitViewConfig().reset();
    });

    test('defaults are all disabled', () {
      final config = SplitViewConfig();
      expect(config.enableWideWindowSplit, isFalse);
      expect(config.enableSquareWindowSplit, isFalse);
      expect(config.isEnabled, isFalse);
      expect(config.homePage, isNull);
      expect(config.fullScreenPages, isEmpty);
      expect(config.enableReducedContainerSize, isTrue);
      expect(config.supportLandscapeFullscreen, isTrue);
    });

    test('setters update values correctly', () {
      final config = SplitViewConfig();
      config.setEnableWideWindowSplit(true);
      config.setEnableSquareWindowSplit(true);
      config.setHomePage('MyHomePage');
      config.setFullScreenPages(<String>['VideoPlayer', 'ImageViewer']);
      config.setEnableReducedContainerSize(false);
      config.setSupportLandscapeFullscreen(false);

      expect(config.enableWideWindowSplit, isTrue);
      expect(config.enableSquareWindowSplit, isTrue);
      expect(config.isEnabled, isTrue);
      expect(config.homePage, 'MyHomePage');
      expect(config.fullScreenPages, <String>['VideoPlayer', 'ImageViewer']);
      expect(config.enableReducedContainerSize, isFalse);
      expect(config.supportLandscapeFullscreen, isFalse);
    });

    test('isEnabled is true when only wide window split is enabled', () {
      final config = SplitViewConfig();
      config.setEnableWideWindowSplit(true);
      expect(config.isEnabled, isTrue);
    });

    test('isEnabled is true when only square window split is enabled', () {
      final config = SplitViewConfig();
      config.setEnableSquareWindowSplit(true);
      expect(config.isEnabled, isTrue);
    });

    test('isForceFullscreenRoute returns true for listed pages', () {
      final config = SplitViewConfig();
      config.setFullScreenPages(<String>['VideoPlayer']);
      expect(config.isForceFullscreenRoute('VideoPlayer'), isTrue);
      expect(config.isForceFullscreenRoute('OtherPage'), isFalse);
      expect(config.isForceFullscreenRoute(null), isFalse);
      expect(config.isForceFullscreenRoute(''), isFalse);
    });

    test('reset restores all defaults', () {
      final config = SplitViewConfig();
      config.setEnableWideWindowSplit(true);
      config.setEnableSquareWindowSplit(true);
      config.setHomePage('Home');
      config.setFullScreenPages(<String>['FullScreen']);
      config.setEnableReducedContainerSize(false);
      config.setSupportLandscapeFullscreen(false);
      config.reset();
      expect(config.enableWideWindowSplit, isFalse);
      expect(config.enableSquareWindowSplit, isFalse);
      expect(config.homePage, isNull);
      expect(config.fullScreenPages, isEmpty);
      expect(config.enableReducedContainerSize, isTrue);
      expect(config.supportLandscapeFullscreen, isTrue);
    });
  });

  // ─── SplitViewManager tests ─────────────────────────────────────────────

  group('SplitViewManager', () {
    setUp(() {
      SplitViewManager().reset();
      // reset() intentionally does NOT clear realHomePage (it is set by the
      // app layer), so clear it explicitly for test isolation.
      SplitViewManager().setRealHomePage(null);
    });

    test('defaults are all false', () {
      final manager = SplitViewManager();
      expect(manager.isSplitViewActive, isFalse);
      expect(manager.isForceFullscreen, isFalse);
      expect(manager.isForcedLandscape, isFalse);
      expect(manager.realHomePage, isNull);
    });

    test('setSplitViewActive notifies listeners', () {
      final manager = SplitViewManager();
      var notified = false;
      void listener() => notified = true;
      manager.addListener(listener);
      addTearDown(() => manager.removeListener(listener));

      manager.setSplitViewActive(true);
      expect(notified, isTrue);
      expect(manager.isSplitViewActive, isTrue);

      notified = false;
      // Setting same value should not notify.
      manager.setSplitViewActive(true);
      expect(notified, isFalse);
    });

    test('setForceFullscreen notifies listeners', () {
      final manager = SplitViewManager();
      var notified = false;
      void listener() => notified = true;
      manager.addListener(listener);
      addTearDown(() => manager.removeListener(listener));

      manager.setForceFullscreen(true);
      expect(notified, isTrue);
      expect(manager.isForceFullscreen, isTrue);
    });

    test('setLandscapeFullscreen notifies listeners', () {
      final manager = SplitViewManager();
      var notified = false;
      void listener() => notified = true;
      manager.addListener(listener);
      addTearDown(() => manager.removeListener(listener));

      manager.setLandscapeFullscreen(true);
      expect(notified, isTrue);
      expect(manager.isForcedLandscape, isTrue);
    });

    test('setRealHomePage updates value', () {
      final manager = SplitViewManager();
      manager.setRealHomePage('/home');
      expect(manager.realHomePage, '/home');
    });

    test('reset clears navigator-related state but not realHomePage', () {
      final manager = SplitViewManager();
      manager.setRealHomePage('/home');
      manager.setSplitViewActive(true);
      manager.setForceFullscreen(true);
      manager.setLandscapeFullscreen(true);

      manager.reset();
      expect(manager.isSplitViewActive, isFalse);
      expect(manager.isForceFullscreen, isFalse);
      expect(manager.isForcedLandscape, isFalse);
      // realHomePage is NOT reset by reset() — it's determined by app layer.
      expect(manager.realHomePage, '/home');
    });

    test('initDefaultPlaceholderBuilder creates a builder', () {
      final manager = SplitViewManager();
      // reset() does not clear placeholderBuilder (it is set by the app layer
      // via WidgetsApp), so call initDefaultPlaceholderBuilder to ensure a
      // known state, then verify it produces a non-null builder.
      manager.initDefaultPlaceholderBuilder();
      expect(manager.placeholderBuilder, isNotNull);
    });
  });

  // ─── SplitViewNavigatorPolicy integration tests ─────────────────────────

  group('SplitViewNavigatorPolicy', () {
    test('isSplitViewEnabled static getter reflects config', () {
      debugDefaultTargetPlatformOverride = TargetPlatform.ohos;
      addTearDown(() => debugDefaultTargetPlatformOverride = null);
      addTearDown(_resetSplitViewSingletons);

      _configureSplitView();
      expect(SplitViewNavigatorPolicy.isSplitViewEnabled, isTrue);

      _configureSplitView(enableWideWindowSplit: false);
      expect(SplitViewNavigatorPolicy.isSplitViewEnabled, isFalse);

      _configureSplitView(enableSquareWindowSplit: true);
      expect(SplitViewNavigatorPolicy.isSplitViewEnabled, isTrue);
    });

    testWidgets('split view activates on wide screen', (WidgetTester tester) async {
      _setupSplitViewEnvironment(tester, homePage: '/home');

      await tester.pumpWidget(
        _buildSplitViewApp(
          initialRoute: '/home',
          routes: <String, WidgetBuilder>{
            '/home': (BuildContext context) => _buildPage('Home'),
            '/detail': (BuildContext context) => _buildPage('Detail'),
          },
        ),
      );
      await tester.pumpAndSettle();

      // SplitViewManager should report active split view.
      expect(SplitViewManager().isSplitViewActive, isTrue);
    }, variant: TargetPlatformVariant.only(TargetPlatform.ohos));

    testWidgets(
      'split view does NOT activate on narrow screen',
      (WidgetTester tester) async {
        _setupSplitViewEnvironment(tester, homePage: '/home', screenSize: const Size(400, 800));

        await tester.pumpWidget(
          _buildSplitViewApp(
            initialRoute: '/home',
            routes: <String, WidgetBuilder>{
              '/home': (BuildContext context) => _buildPage('Home'),
              '/detail': (BuildContext context) => _buildPage('Detail'),
            },
          ),
        );
        await tester.pumpAndSettle();

        expect(SplitViewManager().isSplitViewActive, isFalse);
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    testWidgets(
      'split view does NOT activate on non-ohos platform',
      (WidgetTester tester) async {
        _configureSplitView();
        SplitViewManager().reset();
        SplitViewManager().initDefaultPlaceholderBuilder();
        addTearDown(_resetSplitViewSingletons);

        tester.view.physicalSize = const Size(1200, 900) * tester.view.devicePixelRatio;
        addTearDown(tester.view.reset);

        await tester.pumpWidget(
          _buildSplitViewApp(
            initialRoute: '/home',
            routes: <String, WidgetBuilder>{'/home': (BuildContext context) => _buildPage('Home')},
          ),
        );
        await tester.pumpAndSettle();

        expect(SplitViewManager().isSplitViewActive, isFalse);
      },
      variant: TargetPlatformVariant.only(TargetPlatform.android),
    );

    testWidgets(
      'home page is preserved on left when detail is pushed',
      (WidgetTester tester) async {
        _setupSplitViewEnvironment(tester, homePage: '/home');

        await tester.pumpWidget(
          _buildSplitViewApp(
            initialRoute: '/home',
            routes: <String, WidgetBuilder>{
              '/home': (BuildContext context) => _buildPage('Home'),
              '/detail': (BuildContext context) => _buildPage('Detail'),
            },
          ),
        );
        await tester.pumpAndSettle();

        // Home page should be visible.
        expect(find.text('Page Home'), findsOneWidget);

        // Push detail page from home.
        final BuildContext homeContext = tester.element(find.text('Page Home'));
        Navigator.pushNamed(homeContext, '/detail');
        await tester.pumpAndSettle();

        // Both home and detail should be visible in split view.
        expect(find.text('Page Home'), findsOneWidget);
        expect(find.text('Page Detail'), findsOneWidget);
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    testWidgets(
      'pushing from home clears previous detail pages',
      (WidgetTester tester) async {
        _setupSplitViewEnvironment(tester, homePage: '/home');

        await tester.pumpWidget(
          _buildSplitViewApp(
            initialRoute: '/home',
            routes: <String, WidgetBuilder>{
              '/home': (BuildContext context) => _buildPage('Home'),
              '/detail1': (BuildContext context) => _buildPage('Detail1'),
              '/detail2': (BuildContext context) => _buildPage('Detail2'),
            },
          ),
        );
        await tester.pumpAndSettle();

        // Push detail1 from home.
        final BuildContext homeContext = tester.element(find.text('Page Home'));
        Navigator.pushNamed(homeContext, '/detail1');
        await tester.pumpAndSettle();
        expect(find.text('Page Detail1'), findsOneWidget);

        // Push detail2 from home — should clear detail1.
        final BuildContext homeContext2 = tester.element(find.text('Page Home'));
        Navigator.pushNamed(homeContext2, '/detail2');
        await tester.pumpAndSettle();

        // Detail1 should be replaced by Detail2.
        expect(find.text('Page Detail1'), findsNothing);
        expect(find.text('Page Detail2'), findsOneWidget);
        // Home should still be visible.
        expect(find.text('Page Home'), findsOneWidget);
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    testWidgets(
      'pop from home page is blocked in split view',
      (WidgetTester tester) async {
        _setupSplitViewEnvironment(tester, homePage: '/home');

        await tester.pumpWidget(
          _buildSplitViewApp(
            initialRoute: '/home',
            routes: <String, WidgetBuilder>{
              '/home': (BuildContext context) => _buildPage('Home'),
              '/detail': (BuildContext context) => _buildPage('Detail'),
            },
          ),
        );
        await tester.pumpAndSettle();

        final BuildContext homeContext = tester.element(find.text('Page Home'));
        Navigator.pushNamed(homeContext, '/detail');
        await tester.pumpAndSettle();

        expect(find.text('Page Detail'), findsOneWidget);

        // Pop the detail page from detail context.
        final BuildContext detailContext = tester.element(find.text('Page Detail'));
        final bool poppedDetail = await Navigator.maybePop(detailContext);
        await tester.pumpAndSettle();

        // Detail should be popped, home should remain.
        expect(poppedDetail, isTrue);
        expect(find.text('Page Detail'), findsNothing);
        expect(find.text('Page Home'), findsOneWidget);

        // Now only home page remains. maybePop should bubble up (return false)
        // because the home page must remain visible on the left side.
        final BuildContext homeContext2 = tester.element(find.text('Page Home'));
        final bool poppedHome = await Navigator.maybePop(homeContext2);
        await tester.pumpAndSettle();

        // Pop should be blocked (home page should not pop).
        expect(poppedHome, isFalse);
        // Home should still be visible.
        expect(find.text('Page Home'), findsOneWidget);
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    testWidgets('pop from detail page works normally', (WidgetTester tester) async {
      _setupSplitViewEnvironment(tester, homePage: '/home');

      await tester.pumpWidget(
        _buildSplitViewApp(
          initialRoute: '/home',
          routes: <String, WidgetBuilder>{
            '/home': (BuildContext context) => _buildPage('Home'),
            '/detail': (BuildContext context) => _buildPage('Detail'),
          },
        ),
      );
      await tester.pumpAndSettle();

      final BuildContext homeContext = tester.element(find.text('Page Home'));
      Navigator.pushNamed(homeContext, '/detail');
      await tester.pumpAndSettle();

      expect(find.text('Page Detail'), findsOneWidget);

      // Pop from detail page context.
      final BuildContext detailContext = tester.element(find.text('Page Detail'));
      final bool popped = await Navigator.maybePop(detailContext);
      await tester.pumpAndSettle();

      expect(popped, isTrue);
      // Detail should be gone, home should remain.
      expect(find.text('Page Detail'), findsNothing);
      expect(find.text('Page Home'), findsOneWidget);
    }, variant: TargetPlatformVariant.only(TargetPlatform.ohos));

    testWidgets(
      'force fullscreen route disables split view visibility',
      (WidgetTester tester) async {
        _setupSplitViewEnvironment(
          tester,
          homePage: '/home',
          fullScreenPages: <String>['/fullscreen'],
        );

        await tester.pumpWidget(
          _buildSplitViewApp(
            initialRoute: '/home',
            routes: <String, WidgetBuilder>{
              '/home': (BuildContext context) => _buildPage('Home'),
              '/fullscreen': (BuildContext context) => _buildPage('Fullscreen'),
            },
          ),
        );
        await tester.pumpAndSettle();

        expect(SplitViewManager().isForceFullscreen, isFalse);

        final BuildContext homeContext = tester.element(find.text('Page Home'));
        Navigator.pushNamed(homeContext, '/fullscreen');
        await tester.pumpAndSettle();

        // After pushing a fullscreen route, force fullscreen should be true.
        expect(SplitViewManager().isForceFullscreen, isTrue);
        expect(find.text('Page Fullscreen'), findsOneWidget);
        // Home page should be hidden — fullscreen route covers the entire screen.
        expect(find.text('Page Home'), findsNothing);
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    testWidgets(
      'pushing new home page from detail clears entire stack',
      (WidgetTester tester) async {
        _setupSplitViewEnvironment(tester, homePage: '/home');

        await tester.pumpWidget(
          _buildSplitViewApp(
            initialRoute: '/home',
            routes: <String, WidgetBuilder>{
              '/home': (BuildContext context) => _buildPage('Home'),
              '/detail': (BuildContext context) => _buildPage('Detail'),
            },
          ),
        );
        await tester.pumpAndSettle();

        final BuildContext homeContext = tester.element(find.text('Page Home'));
        Navigator.pushNamed(homeContext, '/detail');
        await tester.pumpAndSettle();

        expect(find.text('Page Detail'), findsOneWidget);

        // Push home page again from detail context.
        final BuildContext detailContext = tester.element(find.text('Page Detail'));
        Navigator.pushNamed(detailContext, '/home');
        await tester.pumpAndSettle();

        // Should show only home page (stack was cleared).
        expect(find.text('Page Home'), findsOneWidget);
        expect(find.text('Page Detail'), findsNothing);
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    testWidgets(
      'placeholder is shown on right when no detail page',
      (WidgetTester tester) async {
        _setupSplitViewEnvironment(tester, homePage: '/home');

        await tester.pumpWidget(
          _buildSplitViewApp(
            initialRoute: '/home',
            routes: <String, WidgetBuilder>{
              '/home': (BuildContext context) => _buildPage('Home'),
              '/detail': (BuildContext context) => _buildPage('Detail'),
            },
          ),
        );
        await tester.pumpAndSettle();

        // Home page should be visible on the left.
        expect(find.text('Page Home'), findsOneWidget);
        // No detail page should be visible.
        expect(find.text('Page Detail'), findsNothing);
        // Placeholder (default ColoredBox with 0xFFf1f3f5) should be rendered.
        expect(
          find.byWidgetPredicate(
            (Widget w) => w is ColoredBox && w.color == const Color(0xFFf1f3f5),
          ),
          findsWidgets,
        );
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    testWidgets(
      'placeholder shows app icon when icon data is set',
      (WidgetTester tester) async {
        _setupSplitViewEnvironment(tester, homePage: '/home');

        // Inject a small 1x1 transparent PNG as placeholder icon data.
        const testIconBase64 =
            'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==';
        SplitViewConfig().setPlaceholderIconData(testIconBase64);
        // Re-initialize the placeholder builder so it picks up the new icon.
        SplitViewManager().initDefaultPlaceholderBuilder();

        await tester.pumpWidget(
          _buildSplitViewApp(
            initialRoute: '/home',
            routes: <String, WidgetBuilder>{
              '/home': (BuildContext context) => _buildPage('Home'),
              '/detail': (BuildContext context) => _buildPage('Detail'),
            },
          ),
        );
        await tester.pumpAndSettle();

        // Home page should be visible on the left.
        expect(find.text('Page Home'), findsOneWidget);
        // Placeholder should contain an Image widget (from Image.memory).
        expect(find.byType(Image), findsWidgets);
        // The Image (placeholder icon) should be positioned on the right half
        // of the screen, not on the left where the home page is.
        final double screenWidth = tester.view.physicalSize.width / tester.view.devicePixelRatio;
        final Offset imageCenter = tester.getCenter(find.byType(Image).first);
        expect(imageCenter.dx, greaterThan(screenWidth / 2));
        // The icon should be displayed at a fixed 48x48 logical pixels.
        final Size imageSize = tester.getSize(find.byType(Image).first);
        expect(imageSize.width, closeTo(48, 1));
        expect(imageSize.height, closeTo(48, 1));
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    testWidgets(
      'square window split activates on near-square screen',
      (WidgetTester tester) async {
        _setupSplitViewEnvironment(
          tester,
          homePage: '/home',
          enableWideWindowSplit: false,
          enableSquareWindowSplit: true,
          screenSize: const Size(700, 680),
        );

        await tester.pumpWidget(
          _buildSplitViewApp(
            initialRoute: '/home',
            routes: <String, WidgetBuilder>{
              '/home': (BuildContext context) => _buildPage('Home'),
              '/detail': (BuildContext context) => _buildPage('Detail'),
            },
          ),
        );
        await tester.pumpAndSettle();

        expect(SplitViewManager().isSplitViewActive, isTrue);
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    testWidgets(
      'square window split does NOT activate when disabled',
      (WidgetTester tester) async {
        _setupSplitViewEnvironment(
          tester,
          homePage: '/home',
          enableWideWindowSplit: false,
          screenSize: const Size(700, 680),
        );

        await tester.pumpWidget(
          _buildSplitViewApp(
            initialRoute: '/home',
            routes: <String, WidgetBuilder>{'/home': (BuildContext context) => _buildPage('Home')},
          ),
        );
        await tester.pumpAndSettle();

        expect(SplitViewManager().isSplitViewActive, isFalse);
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    testWidgets('forced landscape disables split view', (WidgetTester tester) async {
      _setupSplitViewEnvironment(tester, homePage: '/home');

      await tester.pumpWidget(
        _buildSplitViewApp(
          initialRoute: '/home',
          routes: <String, WidgetBuilder>{'/home': (BuildContext context) => _buildPage('Home')},
        ),
      );
      await tester.pumpAndSettle();

      expect(SplitViewManager().isSplitViewActive, isTrue);

      // In split view, home page occupies roughly half the screen width.
      final double screenWidth = tester.view.physicalSize.width / tester.view.devicePixelRatio;
      final RenderBox homeBoxBefore = tester.renderObject(find.byType(Container).first);
      expect(homeBoxBefore.size.width, closeTo(screenWidth / 2, screenWidth * 0.1));

      // Simulate forced landscape.
      SplitViewManager().setLandscapeFullscreen(true);
      await tester.pumpAndSettle();

      // Split view should be disabled due to forced landscape.
      expect(SplitViewManager().isForcedLandscape, isTrue);
      // Home page should now expand to full screen width.
      final RenderBox homeBoxAfter = tester.renderObject(find.byType(Container).first);
      expect(homeBoxAfter.size.width, closeTo(screenWidth, screenWidth * 0.1));
    }, variant: TargetPlatformVariant.only(TargetPlatform.ohos));

    testWidgets(
      'canPop returns false when only home page remains',
      (WidgetTester tester) async {
        _setupSplitViewEnvironment(tester, homePage: '/home');

        await tester.pumpWidget(
          _buildSplitViewApp(
            initialRoute: '/home',
            routes: <String, WidgetBuilder>{
              '/home': (BuildContext context) => _buildPage('Home'),
              '/detail': (BuildContext context) => _buildPage('Detail'),
            },
          ),
        );
        await tester.pumpAndSettle();

        final NavigatorState navigator = tester.state(find.byType(Navigator));
        expect(navigator.canPop(), isFalse);

        final BuildContext homeContext = tester.element(find.text('Page Home'));
        Navigator.pushNamed(homeContext, '/detail');
        await tester.pumpAndSettle();

        expect(navigator.canPop(), isTrue);
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    testWidgets(
      'multiple pushes and pops maintain home on left',
      (WidgetTester tester) async {
        _setupSplitViewEnvironment(tester, homePage: '/home');

        await tester.pumpWidget(
          _buildSplitViewApp(
            initialRoute: '/home',
            routes: <String, WidgetBuilder>{
              '/home': (BuildContext context) => _buildPage('Home'),
              '/d1': (BuildContext context) => _buildPage('D1'),
              '/d2': (BuildContext context) => _buildPage('D2'),
              '/d3': (BuildContext context) => _buildPage('D3'),
            },
          ),
        );
        await tester.pumpAndSettle();

        // Push d1 from home.
        final BuildContext homeContext = tester.element(find.text('Page Home'));
        Navigator.pushNamed(homeContext, '/d1');
        await tester.pumpAndSettle();
        expect(find.text('Page Home'), findsOneWidget);
        expect(find.text('Page D1'), findsOneWidget);

        // Push d2 from d1.
        final BuildContext d1Context = tester.element(find.text('Page D1'));
        Navigator.pushNamed(d1Context, '/d2');
        await tester.pumpAndSettle();
        expect(find.text('Page Home'), findsOneWidget);
        expect(find.text('Page D2'), findsOneWidget);
        expect(find.text('Page D1'), findsNothing);

        // Pop d2 → should show d1 again.
        final BuildContext d2Context = tester.element(find.text('Page D2'));
        await Navigator.maybePop(d2Context);
        await tester.pumpAndSettle();
        expect(find.text('Page D1'), findsOneWidget);
        expect(find.text('Page D2'), findsNothing);

        // Home should still be visible throughout.
        expect(find.text('Page Home'), findsOneWidget);
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );
  });

  // ─── SplitViewConfigLoader tests ────────────────────────────────────────

  group('SplitViewConfigLoader', () {
    setUp(() {
      SplitViewConfigLoader().reset();
      SplitViewConfig().reset();
    });

    tearDown(() {
      SplitViewConfigLoader().reset();
      SplitViewConfig().reset();
    });

    test('getRawConfig returns null initially', () {
      expect(SplitViewConfigLoader().getRawConfig(), isNull);
    });

    testWidgets('setupSystemChannel parses config from platform message', (
      WidgetTester tester,
    ) async {
      SplitViewConfigLoader().setupSystemChannel();
      await tester.binding.defaultBinaryMessenger.handlePlatformMessage(
        'flutter/split_view_config_system',
        const StringCodec().encodeMessage(
          '{"splitOptions":{"homePage":"/home","enableWideWindowSplit":true,'
          '"enableSquareWindowSplit":true,"fullScreenPages":["VideoPlayer","ImageViewer"],'
          '"enableReducedContainerSize":false,"supportLandscapeFullscreen":false}}',
        ),
        (ByteData? _) {},
      );
      // After handlePlatformMessage, parseConfigSync has been called and
      // rawConfig is cleared. Verify the parsed SplitViewConfig instead.
      expect(SplitViewConfig().homePage, '/home');
      expect(SplitViewConfig().enableWideWindowSplit, isTrue);
      expect(SplitViewConfig().enableSquareWindowSplit, isTrue);
      expect(SplitViewConfig().fullScreenPages, <String>['VideoPlayer', 'ImageViewer']);
      expect(SplitViewConfig().enableReducedContainerSize, isFalse);
      expect(SplitViewConfig().supportLandscapeFullscreen, isFalse);
      // rawConfig is cleared by parseConfigSync after parsing.
      expect(SplitViewConfigLoader().getRawConfig(), isNull);
    });

    testWidgets('parseConfigSync handles non-bool enable flags', (WidgetTester tester) async {
      SplitViewConfigLoader().setupSystemChannel();
      await tester.binding.defaultBinaryMessenger.handlePlatformMessage(
        'flutter/split_view_config_system',
        const StringCodec().encodeMessage(
          '{"splitOptions":{"enableWideWindowSplit":"yes",'
          '"enableSquareWindowSplit":42}}',
        ),
        (ByteData? _) {},
      );
      // Non-bool values should fall back to false.
      expect(SplitViewConfig().enableWideWindowSplit, isFalse);
      expect(SplitViewConfig().enableSquareWindowSplit, isFalse);
      expect(SplitViewConfig().isEnabled, isFalse);
    });

    testWidgets('parseConfigSync handles non-string and empty homePage', (
      WidgetTester tester,
    ) async {
      SplitViewConfigLoader().setupSystemChannel();
      await tester.binding.defaultBinaryMessenger.handlePlatformMessage(
        'flutter/split_view_config_system',
        const StringCodec().encodeMessage('{"splitOptions":{"homePage":123}}'),
        (ByteData? _) {},
      );
      // Non-string homePage should fall back to null.
      expect(SplitViewConfig().homePage, isNull);

      // Reset and test empty string.
      SplitViewConfig().reset();
      SplitViewConfigLoader().setupSystemChannel();
      await tester.binding.defaultBinaryMessenger.handlePlatformMessage(
        'flutter/split_view_config_system',
        const StringCodec().encodeMessage('{"splitOptions":{"homePage":""}}'),
        (ByteData? _) {},
      );
      // Empty string homePage should fall back to null.
      expect(SplitViewConfig().homePage, isNull);
    });

    testWidgets('parseConfigSync filters non-string and empty items in fullScreenPages', (
      WidgetTester tester,
    ) async {
      SplitViewConfigLoader().setupSystemChannel();
      await tester.binding.defaultBinaryMessenger.handlePlatformMessage(
        'flutter/split_view_config_system',
        const StringCodec().encodeMessage(
          '{"splitOptions":{"fullScreenPages":["ValidPage","",123,null,"AnotherPage"]}}',
        ),
        (ByteData? _) {},
      );
      // Only valid non-empty strings should remain.
      expect(SplitViewConfig().fullScreenPages, <String>['ValidPage', 'AnotherPage']);
    });

    testWidgets('parseConfigSync handles non-list fullScreenPages', (WidgetTester tester) async {
      SplitViewConfigLoader().setupSystemChannel();
      await tester.binding.defaultBinaryMessenger.handlePlatformMessage(
        'flutter/split_view_config_system',
        const StringCodec().encodeMessage('{"splitOptions":{"fullScreenPages":"not-a-list"}}'),
        (ByteData? _) {},
      );
      // Non-list value should fall back to empty list.
      expect(SplitViewConfig().fullScreenPages, isEmpty);
    });

    testWidgets(
      'parseConfigSync falls back to true for non-bool reducedContainerSize and landscape',
      (WidgetTester tester) async {
        SplitViewConfigLoader().setupSystemChannel();
        await tester.binding.defaultBinaryMessenger.handlePlatformMessage(
          'flutter/split_view_config_system',
          const StringCodec().encodeMessage(
            '{"splitOptions":{"enableReducedContainerSize":"false",'
            '"supportLandscapeFullscreen":0}}',
          ),
          (ByteData? _) {},
        );
        // Non-bool values should fall back to true (default).
        expect(SplitViewConfig().enableReducedContainerSize, isTrue);
        expect(SplitViewConfig().supportLandscapeFullscreen, isTrue);
      },
    );

    testWidgets('parseConfigSync handles invalid JSON gracefully', (WidgetTester tester) async {
      SplitViewConfigLoader().setupSystemChannel();
      await tester.binding.defaultBinaryMessenger.handlePlatformMessage(
        'flutter/split_view_config_system',
        const StringCodec().encodeMessage('not-valid-json'),
        (ByteData? _) {},
      );
      // Invalid JSON should not throw; config should remain at defaults.
      expect(SplitViewConfig().enableWideWindowSplit, isFalse);
      expect(SplitViewConfig().homePage, isNull);
      expect(SplitViewConfig().fullScreenPages, isEmpty);
    });
  });

  // ─── OrientationChangeNotifier tests ───────────────────────────────────

  group('OrientationChangeNotifier', () {
    setUp(() {
      // Reset the singleton by setting to false.
      OrientationChangeNotifier().notifyLandscapeChange(false);
    });

    test('defaults to not forced landscape', () {
      expect(OrientationChangeNotifier().isForcedLandscape, isFalse);
    });

    test('notifyLandscapeChange updates value and notifies listeners', () {
      final notifier = OrientationChangeNotifier();
      var notified = false;
      void listener() => notified = true;
      notifier.addListener(listener);
      addTearDown(() => notifier.removeListener(listener));

      notifier.notifyLandscapeChange(true);
      expect(notified, isTrue);
      expect(notifier.isForcedLandscape, isTrue);
    });

    test('notifyLandscapeChange does not notify when value is same', () {
      final notifier = OrientationChangeNotifier();
      notifier.notifyLandscapeChange(false); // ensure false
      var notified = false;
      void listener() => notified = true;
      notifier.addListener(listener);
      addTearDown(() => notifier.removeListener(listener));

      notifier.notifyLandscapeChange(false);
      expect(notified, isFalse);
    });
  });

  // ─── MediaQueryData.enableSplitView tests ──────────────────────────────

  group('MediaQueryData.enableSplitView', () {
    testWidgets(
      'size is halved when enableSplitView is true on ohos',
      (WidgetTester tester) async {
        const originalSize = Size(800, 600);
        await tester.pumpWidget(
          MediaQuery(
            data: const MediaQueryData(size: originalSize, enableSplitView: true),
            child: Builder(
              builder: (BuildContext context) {
                final MediaQueryData mediaQuery = MediaQuery.of(context);
                return Container(
                  key: Key('size-${mediaQuery.size.width}-${mediaQuery.size.height}'),
                );
              },
            ),
          ),
        );

        // Width should be halved: 800/2 = 400, height stays 600.
        expect(
          (tester.widget<Container>(find.byType(Container)).key! as ValueKey<String>).value,
          'size-400.0-600.0',
        );
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    testWidgets(
      'size is NOT halved when enableSplitView is false',
      (WidgetTester tester) async {
        const originalSize = Size(800, 600);
        await tester.pumpWidget(
          MediaQuery(
            data: const MediaQueryData(size: originalSize),
            child: Builder(
              builder: (BuildContext context) {
                final MediaQueryData mediaQuery = MediaQuery.of(context);
                return Container(
                  key: Key('size-${mediaQuery.size.width}-${mediaQuery.size.height}'),
                );
              },
            ),
          ),
        );

        // Size should be original.
        expect(
          (tester.widget<Container>(find.byType(Container)).key! as ValueKey<String>).value,
          'size-800.0-600.0',
        );
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    testWidgets(
      'size is NOT halved on non-ohos platform even with enableSplitView',
      (WidgetTester tester) async {
        const originalSize = Size(800, 600);
        await tester.pumpWidget(
          MediaQuery(
            data: const MediaQueryData(size: originalSize, enableSplitView: true),
            child: Builder(
              builder: (BuildContext context) {
                final MediaQueryData mediaQuery = MediaQuery.of(context);
                return Container(
                  key: Key('size-${mediaQuery.size.width}-${mediaQuery.size.height}'),
                );
              },
            ),
          ),
        );

        // On non-ohos, size should be original even with enableSplitView.
        expect(
          (tester.widget<Container>(find.byType(Container)).key! as ValueKey<String>).value,
          'size-800.0-600.0',
        );
      },
      variant: TargetPlatformVariant.only(TargetPlatform.android),
    );
  });
}
