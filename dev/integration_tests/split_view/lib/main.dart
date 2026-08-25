// Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE_HW file.

import 'dart:io' show Platform;

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

/// Entry point for the split view integration test app.
void main() {
  runApp(const SplitViewTestApp());
}

/// A test app that exercises the OHOS split view (parallel vision) feature.
///
/// Uses `home + onGenerateRoute` routing (方案3 style):
/// - [home] directly provides the home page widget.
/// - [onGenerateRoute] dynamically handles all other routes.
///
/// Covered feature areas:
/// 1. Navigation: push (named/unnamed), multi-level push, pop, popUntil
/// 2. Return: pop, popUntil home
/// 3. Dialogs: showDialog, AlertDialog, nested dialog
/// 4. Input method: TextField focus and text input
/// 5. Orientation switch: forced landscape / restore all orientations
/// 6. Fullscreen: config-based fullscreen route + force-landscape fullscreen
class SplitViewTestApp extends StatelessWidget {
  const SplitViewTestApp({super.key});

  @override
  Widget build(BuildContext context) {
    return const MaterialApp(
      title: 'Split View Integration Test',
      debugShowCheckedModeBanner: false,
      home: _HomePage(),
      onGenerateRoute: _generateRoute,
    );
  }

  /// Dynamically generates routes for all non-home pages.
  static Route<dynamic>? _generateRoute(RouteSettings settings) {
    switch (settings.name) {
      case '/detail1':
        return MaterialPageRoute<void>(
          settings: settings,
          builder: (_) => const _DetailPage(pageIndex: 1),
        );
      case '/detail2':
        return MaterialPageRoute<void>(
          settings: settings,
          builder: (_) => const _DetailPage(pageIndex: 2),
        );
      case '/detail3':
        return MaterialPageRoute<void>(
          settings: settings,
          builder: (_) => const _DetailPage(pageIndex: 3),
        );
      case '/fullscreen':
        return MaterialPageRoute<void>(settings: settings, builder: (_) => const _FullScreenPage());
      case '/mediaquery':
        return MaterialPageRoute<void>(settings: settings, builder: (_) => const _MediaQueryPage());
      case '/native':
        return MaterialPageRoute<void>(settings: settings, builder: (_) => const _NativeViewPage());
      default:
        return MaterialPageRoute<void>(
          settings: settings,
          builder: (_) => _UnknownRoutePage(routeName: settings.name),
        );
    }
  }
}

// ---------------------------------------------------------------------------
// Home page
// ---------------------------------------------------------------------------

/// Home page with buttons exercising different navigation and feature APIs.
class _HomePage extends StatelessWidget {
  const _HomePage();

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Home Title')),
      body: SafeArea(
        child: SingleChildScrollView(
          padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 24),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: <Widget>[
              const Text('Page Home', key: Key('home-text'), style: TextStyle(fontSize: 32)),
              const SizedBox(height: 24),
              _buildSectionTitle('Input'),
              const Row(
                children: <Widget>[
                  Text('Home Dart Input Label'),
                  SizedBox(width: 8),
                  Expanded(
                    child: TextField(
                      key: Key('home-dart-input'),
                      decoration: InputDecoration(
                        hintText: 'Dart input on home',
                        border: OutlineInputBorder(),
                      ),
                    ),
                  ),
                ],
              ),
              const SizedBox(height: 12),
              SizedBox(
                key: const Key('home-native-input-container'),
                width: 300,
                height: 60,
                child: Platform.isOhos
                    ? const OhosView(viewType: 'split_view/native_input')
                    : const Placeholder(),
              ),
              const SizedBox(height: 24),
              _buildSectionTitle('Navigation'),
              _buildButton(
                key: 'push-native',
                label: 'Push Native View Page',
                onTap: () => Navigator.of(context).pushNamed('/native'),
              ),
              _buildButton(
                key: 'push-detail1-named',
                label: 'Push Detail 1 (named)',
                onTap: () => Navigator.of(context).pushNamed('/detail1'),
              ),
              _buildButton(
                key: 'push-detail1-unnamed',
                label: 'Push Detail 1 (unnamed)',
                onTap: () => Navigator.of(
                  context,
                ).push(MaterialPageRoute<void>(builder: (_) => const _DetailPage(pageIndex: 1))),
              ),
              _buildButton(
                key: 'push-fullscreen',
                label: 'Push Fullscreen (config)',
                onTap: () => Navigator.of(context).pushNamed('/fullscreen'),
              ),
              _buildButton(
                key: 'push-mediaquery',
                label: 'Push MediaQuery Page',
                onTap: () => Navigator.of(context).pushNamed('/mediaquery'),
              ),
              const SizedBox(height: 16),
              _buildSectionTitle('Dialogs'),
              _buildButton(
                key: 'show-dialog',
                label: 'Show AlertDialog',
                onTap: () => _showAlertDialog(context),
              ),
              _buildButton(
                key: 'show-nested-dialog',
                label: 'Show Nested Dialog',
                onTap: () => _showNestedDialog(context),
              ),
              const SizedBox(height: 16),
              _buildSectionTitle('Orientation'),
              _buildButton(
                key: 'force-landscape',
                label: 'Force Landscape (fullscreen)',
                onTap: () => SystemChrome.setPreferredOrientations(<DeviceOrientation>[
                  DeviceOrientation.landscapeLeft,
                  DeviceOrientation.landscapeRight,
                ]),
              ),
              _buildButton(
                key: 'restore-orientation',
                label: 'Restore All Orientations',
                onTap: () => SystemChrome.setPreferredOrientations(<DeviceOrientation>[
                  DeviceOrientation.portraitUp,
                  DeviceOrientation.portraitDown,
                  DeviceOrientation.landscapeLeft,
                  DeviceOrientation.landscapeRight,
                ]),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildSectionTitle(String title) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 8),
      child: Text(title, style: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
    );
  }

  Widget _buildButton({required String key, required String label, required VoidCallback onTap}) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 8),
      child: ElevatedButton(key: Key(key), onPressed: onTap, child: Text(label)),
    );
  }

  void _showAlertDialog(BuildContext context) {
    showDialog<void>(
      context: context,
      builder: (BuildContext context) => AlertDialog(
        title: const Text('Test Dialog'),
        content: const Text('Dialog in split view'),
        actions: <Widget>[
          TextButton(
            key: const Key('close-dialog'),
            onPressed: () => Navigator.of(context).pop(),
            child: const Text('Close'),
          ),
        ],
      ),
    );
  }

  void _showNestedDialog(BuildContext context) {
    showDialog<void>(
      context: context,
      builder: (BuildContext outerContext) => AlertDialog(
        title: const Text('Outer Dialog'),
        content: const Text('Tap to open inner dialog'),
        actions: <Widget>[
          TextButton(
            key: const Key('close-outer-dialog'),
            onPressed: () => Navigator.of(outerContext).pop(),
            child: const Text('Close'),
          ),
          ElevatedButton(
            key: const Key('open-inner-dialog'),
            onPressed: () {
              showDialog<void>(
                context: outerContext,
                builder: (BuildContext innerContext) => AlertDialog(
                  title: const Text('Inner Dialog'),
                  content: const Text('Nested dialog in split view'),
                  actions: <Widget>[
                    TextButton(
                      key: const Key('close-inner-dialog'),
                      onPressed: () {
                        Navigator.of(innerContext).pop();
                        Navigator.of(outerContext).pop();
                      },
                      child: const Text('Close All'),
                    ),
                  ],
                ),
              );
            },
            child: const Text('Open Inner'),
          ),
        ],
      ),
    );
  }
}

// ---------------------------------------------------------------------------
// Detail page (multi-level: detail1 → detail2 → detail3)
// ---------------------------------------------------------------------------

/// Detail page that supports multi-level navigation (detail1 → detail2 → detail3).
class _DetailPage extends StatelessWidget {
  const _DetailPage({required this.pageIndex});

  final int pageIndex;

  String get _nextRoute => switch (pageIndex) {
    1 => '/detail2',
    2 => '/detail3',
    _ => '/detail1',
  };

  int get _nextIndex => pageIndex == 3 ? 1 : pageIndex + 1;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: Text('Detail Title $pageIndex')),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: <Widget>[
            Text(
              'Page Detail $pageIndex',
              key: Key('detail-text-$pageIndex'),
              style: const TextStyle(fontSize: 32),
            ),
            const SizedBox(height: 24),
            ElevatedButton(
              key: const Key('push-next'),
              onPressed: () => Navigator.of(context).pushNamed(_nextRoute),
              child: Text('Push Detail $_nextIndex'),
            ),
            const SizedBox(height: 12),
            ElevatedButton(
              key: const Key('pop-detail'),
              onPressed: () => Navigator.of(context).pop(),
              child: const Text('Pop'),
            ),
            const SizedBox(height: 12),
            ElevatedButton(
              key: const Key('pop-until-home'),
              onPressed: () =>
                  Navigator.of(context).popUntil((Route<dynamic> route) => route.isFirst),
              child: const Text('Pop Until Home'),
            ),
          ],
        ),
      ),
    );
  }
}

// ---------------------------------------------------------------------------
// Fullscreen page (config-based)
// ---------------------------------------------------------------------------

/// Fullscreen page that should cover the entire screen, hiding the home page.
class _FullScreenPage extends StatelessWidget {
  const _FullScreenPage();

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Fullscreen Title')),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: <Widget>[
            const Text(
              'Page Fullscreen',
              key: Key('fullscreen-text'),
              style: TextStyle(fontSize: 32),
            ),
            const SizedBox(height: 24),
            ElevatedButton(
              key: const Key('pop-fullscreen'),
              onPressed: () => Navigator.of(context).pop(),
              child: const Text('Pop'),
            ),
          ],
        ),
      ),
    );
  }
}

// ---------------------------------------------------------------------------
// MediaQuery page
// ---------------------------------------------------------------------------

/// Page that displays [MediaQuery] size info, used to verify that
/// [MediaQueryData.enableSplitView] reduces the reported width by half
/// when split view is active.
class _MediaQueryPage extends StatelessWidget {
  const _MediaQueryPage();

  @override
  Widget build(BuildContext context) {
    final Size mqSize = MediaQuery.of(context).size;
    return Scaffold(
      appBar: AppBar(title: const Text('MediaQuery Title')),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: <Widget>[
            const Text(
              'Page MediaQuery',
              key: Key('mediaquery-text'),
              style: TextStyle(fontSize: 32),
            ),
            const SizedBox(height: 24),
            Text(
              'width: ${mqSize.width.toStringAsFixed(0)}',
              key: const Key('mediaquery-width'),
              style: const TextStyle(fontSize: 20),
            ),
            const SizedBox(height: 8),
            Text(
              'height: ${mqSize.height.toStringAsFixed(0)}',
              key: const Key('mediaquery-height'),
              style: const TextStyle(fontSize: 20),
            ),
            const SizedBox(height: 24),
            ElevatedButton(
              key: const Key('pop-mediaquery'),
              onPressed: () => Navigator.of(context).pop(),
              child: const Text('Pop'),
            ),
          ],
        ),
      ),
    );
  }
}

// ---------------------------------------------------------------------------
// Unknown route page
// ---------------------------------------------------------------------------

/// Fallback page for unknown routes.
class _UnknownRoutePage extends StatelessWidget {
  const _UnknownRoutePage({this.routeName});

  final String? routeName;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Unknown Route')),
      body: Center(child: Text('Unknown route: $routeName', key: const Key('unknown-route-text'))),
    );
  }
}

// ---------------------------------------------------------------------------
// Native view page (embedded OHOS platform view)
// ---------------------------------------------------------------------------

/// Detail page that embeds a native OHOS view via [OhosView].
///
/// This verifies that platform views render correctly inside the right
/// half of the screen when split view is active.
class _NativeViewPage extends StatelessWidget {
  const _NativeViewPage();

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Native View Title')),
      body: SafeArea(
        child: SingleChildScrollView(
          padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 24),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: <Widget>[
              const Text(
                'Page Native View',
                key: Key('native-text'),
                style: TextStyle(fontSize: 32),
              ),
              const SizedBox(height: 24),
              const Row(
                children: <Widget>[
                  Text('Native Dart Input Label'),
                  SizedBox(width: 8),
                  Expanded(
                    child: TextField(
                      key: Key('native-dart-input'),
                      decoration: InputDecoration(
                        hintText: 'Dart input on native page',
                        border: OutlineInputBorder(),
                      ),
                    ),
                  ),
                ],
              ),
              const SizedBox(height: 12),
              SizedBox(
                key: const Key('native-view-container'),
                width: 300,
                height: 200,
                child: Platform.isOhos
                    ? const OhosView(viewType: 'split_view/native_view')
                    : const Placeholder(),
              ),
              const SizedBox(height: 24),
              ElevatedButton(
                key: const Key('pop-native'),
                onPressed: () => Navigator.of(context).pop(),
                child: const Text('Pop'),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
