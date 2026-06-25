// Copyright 2014 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:io' show Platform;
import 'dart:typed_data';

import 'package:flutter/material.dart';

import 'src/basic_messaging.dart';
import 'src/method_calls.dart';
import 'src/pair.dart';
import 'src/test_step.dart';

void main() {
  runApp(const TestApp());
}

class TestApp extends StatefulWidget {
  const TestApp({super.key});

  @override
  State<TestApp> createState() => _TestAppState();
}

class _TestAppState extends State<TestApp> {
  static final dynamic anUnknownValue = DateTime.fromMillisecondsSinceEpoch(1520777802314);
  static final List<dynamic> aList = <dynamic>[
    false,
    0,
    0.0,
    'hello',
    <dynamic>[
      <String, dynamic>{'key': 42},
    ],
  ];
  static final Map<String, dynamic> aMap = <String, dynamic>{
    'a': false,
    'b': 0,
    'c': 0.0,
    'd': 'hello',
    'e': <dynamic>[
      <String, dynamic>{'key': 42},
    ],
  };
  static final Uint8List someUint8s = Uint8List.fromList(<int>[0xBA, 0x5E, 0xBA, 0x11]);
  static final Int32List someInt32s = Int32List.fromList(<int>[-0x7fffffff - 1, 0, 0x7fffffff]);
  static final Int64List someInt64s = Int64List.fromList(<int>[
    -0x7fffffffffffffff - 1,
    0,
    0x7fffffffffffffff,
  ]);
  static final Float32List someFloat32s = Float32List.fromList(<double>[
    double.nan,
    double.negativeInfinity,
    -double.maxFinite,
    -double.minPositive,
    -0.0,
    0.0,
    double.minPositive,
    double.maxFinite,
    double.infinity,
  ]);
  static final Float64List someFloat64s = Float64List.fromList(<double>[
    double.nan,
    double.negativeInfinity,
    -double.maxFinite,
    -double.minPositive,
    -0.0,
    0.0,
    double.minPositive,
    double.maxFinite,
    double.infinity,
  ]);
  static final dynamic aCompoundUnknownValue = <dynamic>[
    anUnknownValue,
    Pair(anUnknownValue, aList),
  ];
  static final List<TestStep> steps = <TestStep>[
    () => methodCallJsonSuccessHandshake(null),
    () => methodCallJsonSuccessHandshake(true),
    () => methodCallJsonSuccessHandshake(7),
    () => methodCallJsonSuccessHandshake('world'),
    () => methodCallJsonSuccessHandshake(aList),
    () => methodCallJsonSuccessHandshake(aMap),
    () => methodCallJsonNotImplementedHandshake(),
    () => methodCallStandardSuccessHandshake(null),
    () => methodCallStandardSuccessHandshake(true),
    () => methodCallStandardSuccessHandshake(7),
    () => methodCallStandardSuccessHandshake('world'),
    () => methodCallStandardSuccessHandshake(aList),
    () => methodCallStandardSuccessHandshake(aMap),
    () => methodCallStandardSuccessHandshake(anUnknownValue),
    () => methodCallStandardSuccessHandshake(aCompoundUnknownValue),
    // TODO(engine): JSONMethodCodec.decodeEnvelope catches
    // FlutterException and wraps it in Error, and IncomingResultHandler uses
    // e.getMessage() instead of e.message.
    // () => methodCallJsonErrorHandshake(null),
    // () => methodCallJsonErrorHandshake('world'),
    // () => methodCallStandardErrorHandshake(null),
    // () => methodCallStandardErrorHandshake('world'),
    // TODO(engine): IncomingResultHandler.reply uses e.getMessage() and
    // IncomingMethodCallHandler.onMessage catch block uses e.getMessage().
    // () => methodCallStandardNotImplementedHandshake(),
    // TODO(engine): BinaryCodec.decodeMessage(null) returns ArrayBuffer(0) instead of null.
    // () => basicBinaryHandshake(null),
    // TODO(engine): BasicMessageChannel with BinaryCodec does not round-trip correctly.
    // if (!Platform.isMacOS)
    //   // Note, it was decided that this will function differently on macOS. See
    //   // also: https://github.com/flutter/flutter/issues/110865.
    //   () => basicBinaryHandshake(ByteData(0)),
    // () => basicBinaryHandshake(ByteData(4)..setUint32(0, 0x12345678)),
    () => basicStringHandshake('hello, world'),
    () => basicStringHandshake('hello \u263A \u{1f602} unicode'),
    // TODO(engine): BasicMessageChannel does not round-trip empty messages correctly.
    // if (!Platform.isMacOS)
    //   // Note, it was decided that this will function differently on macOS. See
    //   // also: https://github.com/flutter/flutter/issues/110865.
    //   () => basicStringHandshake(''),
    // TODO(engine): StringCodec.decodeMessage(null) does not round-trip null correctly.
    // () => basicStringHandshake(null),
    // TODO(engine): JSONMessageCodec.decodeMessage(null) does not round-trip null correctly.
    // () => basicJsonHandshake(null),
    () => basicJsonHandshake(true),
    () => basicJsonHandshake(false),
    () => basicJsonHandshake(0),
    () => basicJsonHandshake(-7),
    () => basicJsonHandshake(7),
    // TODO(engine): JSON large integer precision loss on ohos.
    // () => basicJsonHandshake(1 << 32),
    // () => basicJsonHandshake(1 << 56),
    () => basicJsonHandshake(0.0),
    () => basicJsonHandshake(-7.0),
    () => basicJsonHandshake(7.0),
    () => basicJsonHandshake(''),
    () => basicJsonHandshake('hello, world'),
    () => basicJsonHandshake('hello, "world"'),
    () => basicJsonHandshake('hello \u263A \u{1f602} unicode'),
    () => basicJsonHandshake(<dynamic>[]),
    () => basicJsonHandshake(aList),
    () => basicJsonHandshake(<String, dynamic>{}),
    () => basicJsonHandshake(aMap),
    // TODO(engine): StandardMessageCodec null round-trip issue.
    // () => basicStandardHandshake(null),
    () => basicStandardHandshake(true),
    () => basicStandardHandshake(false),
    () => basicStandardHandshake(0),
    () => basicStandardHandshake(-7),
    () => basicStandardHandshake(7),
    () => basicStandardHandshake(1 << 32),
    () => basicStandardHandshake(1 << 64),
    () => basicStandardHandshake(1 << 128),
    () => basicStandardHandshake(0.0),
    () => basicStandardHandshake(-7.0),
    () => basicStandardHandshake(7.0),
    () => basicStandardHandshake(''),
    () => basicStandardHandshake('hello, world'),
    () => basicStandardHandshake('hello \u263A \u{1f602} unicode'),
    () => basicStandardHandshake(someUint8s),
    () => basicStandardHandshake(someInt32s),
    () => basicStandardHandshake(someInt64s),
    () => basicStandardHandshake(someFloat32s),
    () => basicStandardHandshake(someFloat64s),
    () => basicStandardHandshake(<dynamic>[]),
    () => basicStandardHandshake(aList),
    () => basicStandardHandshake(<String, dynamic>{}),
    () => basicStandardHandshake(<dynamic, dynamic>{7: true, false: -7}),
    () => basicStandardHandshake(aMap),
    () => basicStandardHandshake(anUnknownValue),
    () => basicStandardHandshake(aCompoundUnknownValue),
    () => basicBinaryMessageToUnknownChannel(),
    () => basicStringMessageToUnknownChannel(),
    () => basicJsonMessageToUnknownChannel(),
    () => basicStandardMessageToUnknownChannel(),
    if (Platform.isIOS || Platform.isAndroid || Platform.isMacOS || Platform.isOhos)
      () => basicBackgroundStandardEcho(123),
  ];
  Future<TestStepResult>? _result;
  int _step = 0;

  void _executeNextStep() {
    setState(() {
      if (_step < steps.length) {
        final int stepIndex = _step;
        _result = steps[_step++]().timeout(
          const Duration(seconds: 20),
          onTimeout: () => TestStepResult(
            'Timeout',
            'Step $stepIndex timed out after 20 seconds',
            TestStatus.failed,
          ),
        );
      } else {
        _result = Future<TestStepResult>.value(TestStepResult.complete);
      }
    });
  }

  Widget _buildTestResultWidget(BuildContext context, AsyncSnapshot<TestStepResult> snapshot) {
    return TestStepResult.fromSnapshot(snapshot).asWidget(context);
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Channels Test',
      home: Scaffold(
        appBar: AppBar(title: const Text('Channels Test')),
        body: Padding(
          padding: const EdgeInsets.all(20.0),
          child: FutureBuilder<TestStepResult>(future: _result, builder: _buildTestResultWidget),
        ),
        floatingActionButton: FloatingActionButton(
          key: const ValueKey<String>('step'),
          onPressed: _executeNextStep,
          child: const Icon(Icons.navigate_next),
        ),
      ),
    );
  }
}
