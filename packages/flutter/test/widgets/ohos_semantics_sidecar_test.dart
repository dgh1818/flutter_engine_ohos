// Copyright 2026 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:convert';
import 'dart:ui' as ui;

import 'package:flutter/rendering.dart';
import 'package:flutter/widgets.dart';
import 'package:flutter_test/flutter_test.dart';

class _FakeView extends TestFlutterView {
  _FakeView(ui.FlutterView view, this.viewId)
    : super(
        view: view,
        platformDispatcher: view.platformDispatcher as TestPlatformDispatcher,
        display: view.display as TestDisplay,
      );

  @override
  final int viewId;

  @override
  void render(ui.Scene scene, {Size? size}) {}

  @override
  void updateSemantics(ui.SemanticsUpdate update) {}
}

class _StyleSemantics extends SingleChildRenderObjectWidget {
  const _StyleSemantics({required this.fontSize, required this.color, super.child});

  final double? fontSize;
  final int? color;

  @override
  RenderObject createRenderObject(BuildContext context) => _StyleRender(fontSize, color);

  @override
  void updateRenderObject(BuildContext context, _StyleRender renderObject) {
    renderObject.setStyle(fontSize, color);
  }
}

class _StyleRender extends RenderProxyBox {
  _StyleRender(this.fontSize, this.color);
  double? fontSize;
  int? color;

  void setStyle(double? size, int? background) {
    if (fontSize != size || color != background) {
      fontSize = size;
      color = background;
      markNeedsSemanticsUpdate();
    }
  }

  @override
  void describeSemanticsConfiguration(SemanticsConfiguration config) {
    super.describeSemanticsConfiguration(config);
    config
      ..isSemanticBoundary = true
      ..label = 'child'
      ..textDirection = TextDirection.ltr
      ..fontSize = fontSize
      ..backgroundColor = color;
  }
}

void main() {
  testWidgets('OHOS sends JSON dirty attribute batches to its private channel', (
    WidgetTester tester,
  ) async {
    debugOhosSemanticsEnabledOverride = true;
    addTearDown(() => debugOhosSemanticsEnabledOverride = null);
    final messages = <Map<String, Object?>>[];
    tester.binding.defaultBinaryMessenger.setMockMessageHandler(
      'flutter/ohos_inspector_sidecar_experiment',
      (data) async {
        messages.add(jsonDecode(utf8.decode(data!.buffer.asUint8List())) as Map<String, Object?>);
        return null;
      },
    );
    addTearDown(
      () => tester.binding.defaultBinaryMessenger.setMockMessageHandler(
        'flutter/ohos_inspector_sidecar_experiment',
        null,
      ),
    );
    final SemanticsHandle handle = tester.ensureSemantics();
    await tester.pumpWidget(
      const _StyleSemantics(
        fontSize: 18,
        color: 0xff112233,
        child: SizedBox(width: 10, height: 10),
      ),
    );
    expect(messages, isNotEmpty);
    expect(messages.first['version'], 1);
    expect(messages.first['viewId'], tester.view.viewId);
    expect(messages.first['epoch'], isA<int>());
    expect(messages.first['sequence'], isA<int>());
    expect(messages.first['nodes'], contains(containsPair('fontSize', 18.0)));
    final Map<String, Object?> first = messages.last;
    final firstEpoch = first['epoch']! as int;
    final firstSequence = first['sequence']! as int;
    await tester.pumpWidget(
      const _StyleSemantics(fontSize: null, color: null, child: SizedBox(width: 10, height: 10)),
    );
    expect(messages.last['epoch'], firstEpoch);
    expect(messages.last['sequence'], firstSequence + 1);
    expect(
      messages.last['nodes'],
      contains(allOf(containsPair('fontSize', null), containsPair('backgroundColor', null))),
    );
    handle.dispose();
    final replacement = SemanticsOwner(onSemanticsUpdate: (_) {});
    expect(replacement.ohosSemanticsEpoch, greaterThan(firstEpoch));
    expect(replacement.nextOhosSemanticsSequence(), 1);
    replacement.dispose();
  });

  testWidgets('OHOS batches contain merged nodes and their view IDs', (WidgetTester tester) async {
    debugOhosSemanticsEnabledOverride = true;
    addTearDown(() => debugOhosSemanticsEnabledOverride = null);
    final batches = <(int, Map<int, (double?, int?)>)>[];
    SemanticsBinding.instance.onOhosSemanticsAttributes =
        (int viewId, Map<int, (double?, int?)> nodes) {
          batches.add((viewId, Map<int, (double?, int?)>.of(nodes)));
        };
    addTearDown(() => SemanticsBinding.instance.onOhosSemanticsAttributes = null);
    final SemanticsHandle handle = tester.ensureSemantics();
    final second = _FakeView(tester.view, 202);

    Widget view(ui.FlutterView view, double? size, int? color) => View(
      view: view,
      child: MergeSemantics(
        child: _StyleSemantics(
          fontSize: size,
          color: color,
          child: const SizedBox(width: 10, height: 10),
        ),
      ),
    );

    await tester.pumpWidget(
      wrapWithView: false,
      ViewCollection(
        views: <Widget>[view(tester.view, 18, 0xff112233), view(second, 29, 0xff445566)],
      ),
    );
    final (int, Map<int, (double?, int?)>) firstBatch = batches.singleWhere(
      (batch) => batch.$1 == tester.view.viewId,
    );
    final (int, Map<int, (double?, int?)>) secondBatch = batches.singleWhere(
      (batch) => batch.$1 == 202,
    );
    final MapEntry<int, (double?, int?)> firstEntry = firstBatch.$2.entries.singleWhere(
      (entry) => entry.value == (18.0, 0xff112233),
    );
    expect(secondBatch.$2.values, contains((29.0, 0xff445566)));
    final SemanticsNode root = tester.binding.renderViews
        .singleWhere((renderView) => renderView.flutterView == tester.view)
        .owner!
        .semanticsOwner!
        .rootSemanticsNode!;
    SemanticsNode? merged;
    bool findMerged(SemanticsNode node) {
      if (node.mergeAllDescendantsIntoThisNode) {
        merged = node;
      }
      return true;
    }

    root.visitChildren(findMerged);
    expect(firstEntry.key, merged!.id);

    batches.clear();
    await tester.pumpWidget(
      wrapWithView: false,
      ViewCollection(views: <Widget>[view(tester.view, null, null), view(second, 29, 0xff445566)]),
    );
    expect(batches.singleWhere((batch) => batch.$1 == tester.view.viewId).$2[firstEntry.key], (
      null,
      null,
    ));
    handle.dispose();
    debugOhosSemanticsEnabledOverride = null;
  });

  testWidgets('observer and transport exceptions do not block semantics updates', (
    WidgetTester tester,
  ) async {
    debugOhosSemanticsEnabledOverride = true;
    addTearDown(() => debugOhosSemanticsEnabledOverride = null);
    SemanticsBinding.instance.onOhosSemanticsAttributes = (_, _) => throw StateError('observer');
    addTearDown(() => SemanticsBinding.instance.onOhosSemanticsAttributes = null);
    tester.binding.defaultBinaryMessenger.setMockMessageHandler(
      'flutter/ohos_inspector_sidecar_experiment',
      (_) => throw StateError('transport'),
    );
    addTearDown(
      () => tester.binding.defaultBinaryMessenger.setMockMessageHandler(
        'flutter/ohos_inspector_sidecar_experiment',
        null,
      ),
    );
    final SemanticsHandle handle = tester.ensureSemantics();
    await tester.pumpWidget(
      Directionality(
        textDirection: TextDirection.ltr,
        child: Semantics(label: 'still works', child: const SizedBox(width: 10, height: 10)),
      ),
    );
    expect(find.bySemanticsLabel('still works'), findsOneWidget);
    expect(tester.takeException(), isNull);
    handle.dispose();
  });

  testWidgets('non-OHOS does not emit attribute batches', (WidgetTester tester) async {
    debugOhosSemanticsEnabledOverride = false;
    addTearDown(() => debugOhosSemanticsEnabledOverride = null);
    var called = false;
    SemanticsBinding.instance.onOhosSemanticsAttributes = (_, _) => called = true;
    addTearDown(() => SemanticsBinding.instance.onOhosSemanticsAttributes = null);
    final SemanticsHandle handle = tester.ensureSemantics();
    await tester.pumpWidget(
      const _StyleSemantics(
        fontSize: 12,
        color: 0xff000000,
        child: SizedBox(width: 10, height: 10),
      ),
    );
    expect(called, isFalse);
    handle.dispose();
  });
}
