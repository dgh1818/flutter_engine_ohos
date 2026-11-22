// Copyright 2026 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is exercised as part of the OHOS a11y automation-test attributes
// feature: fontSize / backgroundColor collected into SemanticsConfiguration
// and merged by SemanticsConfiguration.absorb.
//
// These tests deliberately avoid initializing the widget/tester binding:
// render objects are constructed directly and asked to describe their
// semantics configuration, which keeps them runnable against any cached
// engine artifact.

import 'package:flutter/rendering.dart';
import 'package:flutter/widgets.dart';
import 'package:flutter_test/flutter_test.dart';

// Existing render-object tests exercise their semantics configuration directly.
// ignore_for_file: invalid_use_of_protected_member

void main() {
  setUp(() => debugOhosSemanticsEnabledOverride = true);
  tearDown(() => debugOhosSemanticsEnabledOverride = null);

  group('SemanticsConfiguration.absorb merges a11y automation attributes', () {
    test('parent keeps own value, takes child value when unset', () {
      final parent = SemanticsConfiguration()..backgroundColor = 0xFF112233;
      final child = SemanticsConfiguration()..fontSize = 24.0;
      parent.absorb(child);
      expect(parent.backgroundColor, 0xFF112233);
      expect(parent.fontSize, 24.0);
    });

    test('parent fontSize wins when both parent and child set it', () {
      final parent = SemanticsConfiguration()..fontSize = 10.0;
      parent.absorb(SemanticsConfiguration()..fontSize = 24.0);
      expect(parent.fontSize, 10.0);
    });

    test('copy retains automation attributes', () {
      final original = SemanticsConfiguration()
        ..fontSize = 24.0
        ..backgroundColor = 0xFFFFFFFF;
      final SemanticsConfiguration copy = original.copy();
      expect(copy.fontSize, 24.0);
      expect(copy.backgroundColor, 0xFFFFFFFF);
    });
  });

  group('SemanticsNode merges a11y automation attributes', () {
    SemanticsNode node({double? fontSize, int? backgroundColor}) {
      final config = SemanticsConfiguration();
      if (fontSize != null) {
        config.fontSize = fontSize;
      }
      if (backgroundColor != null) {
        config.backgroundColor = backgroundColor;
      }
      return SemanticsNode()..updateWith(config: config);
    }

    SemanticsNode parent(List<SemanticsNode> children, {bool merge = true}) {
      return SemanticsNode()..updateWith(
        config: SemanticsConfiguration()
          ..isSemanticBoundary = true
          ..isMergingSemanticsOfDescendants = merge,
        childrenInInversePaintOrder: children,
      );
    }

    test('uses merged descendant values when parent values are absent', () {
      final SemanticsNode child = node(fontSize: 24, backgroundColor: 0xFF28A87A);
      final SemanticsData data = parent(<SemanticsNode>[child]).getSemanticsData();
      expect(data.fontSize, 24);
      expect(data.backgroundColor, 0xFF28A87A);
    });

    test('keeps parent values when descendants also provide values', () {
      final SemanticsNode child = node(fontSize: 24, backgroundColor: 0xFF28A87A);
      final root = SemanticsNode()
        ..updateWith(
          config: SemanticsConfiguration()
            ..isSemanticBoundary = true
            ..isMergingSemanticsOfDescendants = true
            ..fontSize = 18
            ..backgroundColor = 0xFFFFFFFF,
          childrenInInversePaintOrder: <SemanticsNode>[child],
        );
      final SemanticsData data = root.getSemanticsData();
      expect(data.fontSize, 18);
      expect(data.backgroundColor, 0xFFFFFFFF);
    });

    test('uses the first non-null value in descendant traversal order', () {
      final SemanticsNode first = node(fontSize: 20);
      final SemanticsNode second = node(fontSize: 24, backgroundColor: 0xFF28A87A);
      final SemanticsNode third = node(backgroundColor: 0xFF112233);
      final SemanticsData data = parent(<SemanticsNode>[first, second, third]).getSemanticsData();
      expect(data.fontSize, 20);
      expect(data.backgroundColor, 0xFF28A87A);
    });

    test('merges fields independently from nested descendants', () {
      final SemanticsNode grandchild = node(fontSize: 24, backgroundColor: 0xFF28A87A);
      final SemanticsNode child = parent(<SemanticsNode>[grandchild], merge: false);
      final root = SemanticsNode()
        ..updateWith(
          config: SemanticsConfiguration()
            ..isSemanticBoundary = true
            ..isMergingSemanticsOfDescendants = true
            ..fontSize = 18,
          childrenInInversePaintOrder: <SemanticsNode>[child],
        );
      final SemanticsData data = root.getSemanticsData();
      expect(data.fontSize, 18);
      expect(data.backgroundColor, 0xFF28A87A);
    });

    test('leaves absent fields unset', () {
      final SemanticsData data = parent(<SemanticsNode>[node()]).getSemanticsData();
      expect(data.fontSize, isNull);
      expect(data.backgroundColor, isNull);
    });

    test('does not merge descendants when merging is disabled', () {
      final SemanticsNode child = node(fontSize: 24, backgroundColor: 0xFF28A87A);
      final SemanticsData data = parent(<SemanticsNode>[child], merge: false).getSemanticsData();
      expect(data.fontSize, isNull);
      expect(data.backgroundColor, isNull);
    });

    test('reflects updates to a merged descendant', () {
      final SemanticsNode child = node(fontSize: 24, backgroundColor: 0xFF28A87A);
      final SemanticsNode root = parent(<SemanticsNode>[child]);
      expect(root.getSemanticsData().fontSize, 24);
      expect(root.getSemanticsData().backgroundColor, 0xFF28A87A);
      child.updateWith(
        config: SemanticsConfiguration()
          ..fontSize = 30
          ..backgroundColor = 0xFF112233,
      );
      expect(root.getSemanticsData().fontSize, 30);
      expect(root.getSemanticsData().backgroundColor, 0xFF112233);
    });

    test('semantics data equality includes both attributes', () {
      final SemanticsNode root = node(fontSize: 24, backgroundColor: 0xFF28A87A);
      final SemanticsData original = root.getSemanticsData();
      final SemanticsData same = root.getSemanticsData();
      expect(same, original);
      expect(same.hashCode, original.hashCode);

      root.updateWith(
        config: SemanticsConfiguration()
          ..fontSize = 30
          ..backgroundColor = 0xFF28A87A,
      );
      expect(root.getSemanticsData(), isNot(original));
      root.updateWith(
        config: SemanticsConfiguration()
          ..fontSize = 24
          ..backgroundColor = 0xFF112233,
      );
      expect(root.getSemanticsData(), isNot(original));
    });
  });

  test('RenderParagraph reports explicit font size', () {
    final paragraph = RenderParagraph(
      const TextSpan(text: 'Hello', style: TextStyle(fontSize: 24.0)),
      textDirection: TextDirection.ltr,
    );
    final config = SemanticsConfiguration();
    paragraph.describeSemanticsConfiguration(config);
    expect(config.fontSize, 24.0);
  });

  test('RenderParagraph does not report font size outside OHOS', () {
    debugOhosSemanticsEnabledOverride = false;
    final paragraph = RenderParagraph(
      const TextSpan(text: 'Hello', style: TextStyle(fontSize: 24.0)),
      textDirection: TextDirection.ltr,
    );
    final config = SemanticsConfiguration();
    paragraph.describeSemanticsConfiguration(config);
    expect(config.fontSize, isNull);
  });

  test('RenderParagraph leaves fontSize unset without an explicit style', () {
    final paragraph = RenderParagraph(
      const TextSpan(text: 'Hello'),
      textDirection: TextDirection.ltr,
    );
    final config = SemanticsConfiguration();
    paragraph.describeSemanticsConfiguration(config);
    expect(config.fontSize, isNull);
  });

  group('RenderEditable reports a11y automation font size', () {
    RenderEditable buildEditable(TextStyle? style) {
      return RenderEditable(
        text: TextSpan(text: 'Hello', style: style),
        textDirection: TextDirection.ltr,
        startHandleLayerLink: LayerLink(),
        endHandleLayerLink: LayerLink(),
        offset: ViewportOffset.zero(),
        textSelectionDelegate: _NoopTextSelectionDelegate(),
      );
    }

    test('explicit font size is reported', () {
      final config = SemanticsConfiguration();
      buildEditable(const TextStyle(fontSize: 24.0)).describeSemanticsConfiguration(config);
      expect(config.fontSize, 24.0);
    });

    test('ambient style without explicit size falls back to the framework default', () {
      final config = SemanticsConfiguration();
      buildEditable(
        const TextStyle(color: Color(0xFF000000)),
      ).describeSemanticsConfiguration(config);
      expect(config.fontSize, 14.0);
    });

    test('no style at all leaves fontSize unreported', () {
      final config = SemanticsConfiguration();
      buildEditable(null).describeSemanticsConfiguration(config);
      expect(config.fontSize, isNull);
    });

    test('does not report font size outside OHOS', () {
      debugOhosSemanticsEnabledOverride = false;
      final config = SemanticsConfiguration();
      buildEditable(const TextStyle(fontSize: 24.0)).describeSemanticsConfiguration(config);
      expect(config.fontSize, isNull);
    });
  });

  test('RenderDecoratedBox reports solid background color', () {
    final box = RenderDecoratedBox(decoration: const BoxDecoration(color: Color(0xFF445566)));
    final config = SemanticsConfiguration();
    box.describeSemanticsConfiguration(config);
    expect(config.backgroundColor, 0xFF445566);
  });

  test('RenderDecoratedBox does not report background color outside OHOS', () {
    debugOhosSemanticsEnabledOverride = false;
    final box = RenderDecoratedBox(decoration: const BoxDecoration(color: Color(0xFF445566)));
    final config = SemanticsConfiguration();
    box.describeSemanticsConfiguration(config);
    expect(config.backgroundColor, isNull);
  });

  test('gradient decorations are not reported as background color', () {
    final box = RenderDecoratedBox(
      decoration: const BoxDecoration(
        gradient: LinearGradient(colors: <Color>[Color(0xFF000000), Color(0xFFFFFFFF)]),
      ),
    );
    final config = SemanticsConfiguration();
    box.describeSemanticsConfiguration(config);
    expect(config.backgroundColor, isNull);
  });

  // The cases below go through the widget pipeline and need the test binding,
  // so they only run against an engine built from this branch (Linux build
  // machine flow).
  testWidgets('ColoredBox reports solid background color (widget pipeline)', (
    WidgetTester tester,
  ) async {
    await tester.pumpWidget(
      const ColoredBox(color: Color(0xFF112233), child: SizedBox(width: 10, height: 10)),
    );
    final RenderObject box = tester.renderObject(find.byType(ColoredBox));
    final config = SemanticsConfiguration();
    box.describeSemanticsConfiguration(config);
    expect(config.backgroundColor, 0xFF112233);
  });

  testWidgets('ColoredBox does not report background color outside OHOS', (
    WidgetTester tester,
  ) async {
    debugOhosSemanticsEnabledOverride = false;
    await tester.pumpWidget(
      const ColoredBox(color: Color(0xFF112233), child: SizedBox(width: 10, height: 10)),
    );
    final RenderObject box = tester.renderObject(find.byType(ColoredBox));
    final config = SemanticsConfiguration();
    box.describeSemanticsConfiguration(config);
    expect(config.backgroundColor, isNull);
  });

  testWidgets('changing background colors updates semantics', (WidgetTester tester) async {
    final SemanticsHandle handle = tester.ensureSemantics();
    Widget buildColor(Color color, {required bool decorated}) {
      final Widget box = decorated
          ? DecoratedBox(
              decoration: BoxDecoration(color: color),
              child: const Text('content'),
            )
          : ColoredBox(color: color, child: const Text('content'));
      return Directionality(
        textDirection: TextDirection.ltr,
        child: Semantics(key: const ValueKey<String>('container'), container: true, child: box),
      );
    }

    try {
      for (final decorated in <bool>[false, true]) {
        await tester.pumpWidget(buildColor(const Color(0xFF112233), decorated: decorated));
        expect(
          tester
              .getSemantics(find.byKey(const ValueKey<String>('container')))
              .getSemanticsData()
              .backgroundColor,
          0xFF112233,
        );
        await tester.pumpWidget(buildColor(const Color(0xFF778899), decorated: decorated));
        expect(
          tester
              .getSemantics(find.byKey(const ValueKey<String>('container')))
              .getSemanticsData()
              .backgroundColor,
          0xFF778899,
        );
        await tester.pumpWidget(const SizedBox.shrink());
      }
    } finally {
      handle.dispose();
    }
  });

  testWidgets('Text falls back to the ambient default font size', (WidgetTester tester) async {
    await tester.pumpWidget(
      const Directionality(textDirection: TextDirection.ltr, child: Text('Hello')),
    );
    final RenderObject paragraph = tester.renderObject(find.byType(RichText));
    final config = SemanticsConfiguration();
    paragraph.describeSemanticsConfiguration(config);
    expect(config.fontSize, 14.0);
  });
}

class _NoopTextSelectionDelegate with TextSelectionDelegate {
  @override
  TextEditingValue textEditingValue = TextEditingValue.empty;

  @override
  void userUpdateTextEditingValue(TextEditingValue value, SelectionChangedCause cause) {}

  @override
  void hideToolbar([bool hideHandles = true]) {}

  @override
  void bringIntoView(TextPosition position) {}

  @override
  void cutSelection(SelectionChangedCause cause) {}

  @override
  Future<void> pasteText(SelectionChangedCause cause) async {}

  @override
  void selectAll(SelectionChangedCause cause) {}

  @override
  void copySelection(SelectionChangedCause cause) {}
}
