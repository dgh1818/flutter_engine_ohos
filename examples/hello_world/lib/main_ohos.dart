// Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE_HW file.

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

void main() {
  runApp(const MyApp());

  // Auto-trigger TextInput IME chain after 2 seconds
  // This triggers: setClient -> show -> attach -> listenKeyBoardEvent -> setEditingState -> hide -> clearClient
  Future.delayed(const Duration(seconds: 2), () {
    _triggerTextInputIME();
  });
}

void _triggerTextInputIME() async {
  // 1. setClient + show -> triggers attach() -> listenKeyBoardEvent() -> registers IME callbacks
  await SystemChannels.textInput.invokeMethod('TextInput.setClient', [
    1,
    {
      'inputAction': 'TextInputAction.done',
      'inputType': {
        'name': 'TextInputType.text',
        'signed': false,
        'decimal': false,
      },
    },
  ]);
  await SystemChannels.textInput.invokeMethod('TextInput.show');

  // 2. setEditingState -> triggers updateEditingState / changeSelection
  await SystemChannels.textInput.invokeMethod('TextInput.setEditingState', {
    'text': 'hello world',
    'selectionBase': 0,
    'selectionExtent': 11,
    'composingBase': -1,
    'composingExtent': -1,
  });
  await Future.delayed(const Duration(milliseconds: 500));

  // 3. setEditingState again with different text -> triggers changeSelection with different values
  await SystemChannels.textInput.invokeMethod('TextInput.setEditingState', {
    'text': 'hi',
    'selectionBase': 0,
    'selectionExtent': 2,
    'composingBase': -1,
    'composingExtent': -1,
  });
  await Future.delayed(const Duration(milliseconds: 500));

  // 4. setEditingState with composing region
  await SystemChannels.textInput.invokeMethod('TextInput.setEditingState', {
    'text': 'hello',
    'selectionBase': 0,
    'selectionExtent': 5,
    'composingBase': 0,
    'composingExtent': 3,
  });
  await Future.delayed(const Duration(milliseconds: 500));

  // 5. updateConfig -> triggers configuration update
  await SystemChannels.textInput.invokeMethod('TextInput.updateConfig', {
    'inputAction': 'TextInputAction.go',
    'inputType': {
      'name': 'TextInputType.number',
      'signed': true,
      'decimal': true,
    },
  });
  await Future.delayed(const Duration(milliseconds: 300));

  // 6. setEditableSizeAndTransform
  await SystemChannels.textInput.invokeMethod(
    'TextInput.setEditableSizeAndTransform',
    {
      'width': 100,
      'height': 60,
      'transform': [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 10, 20, 0, 1],
    },
  );
  await Future.delayed(const Duration(milliseconds: 300));

  // 7. setCaretRect
  await SystemChannels.textInput.invokeMethod('TextInput.setCaretRect', {
    'x': 10,
    'y': 20,
    'width': 5,
    'height': 15,
  });
  await Future.delayed(const Duration(milliseconds: 300));

  // 8. setPlatformViewClient
  await SystemChannels.textInput.invokeMethod(
    'TextInput.setPlatformViewClient',
    {'platformViewId': 1, 'usesVirtualDisplay': false},
  );
  await Future.delayed(const Duration(milliseconds: 300));

  // 9. requestAutofill
  await SystemChannels.textInput.invokeMethod('TextInput.setClient', [
    2,
    {
      'inputAction': 'TextInputAction.done',
      'inputType': {
        'name': 'TextInputType.text',
        'signed': false,
        'decimal': false,
      },
      'autofill': {
        'uniqueIdentifier': 'field1',
        'editingValue': {
          'text': '',
          'selectionBase': 0,
          'selectionExtent': 0,
          'composingBase': -1,
          'composingExtent': -1,
        },
        'hints': ['username'],
      },
    },
  ]);
  await SystemChannels.textInput.invokeMethod('TextInput.show');
  await SystemChannels.textInput.invokeMethod('TextInput.requestAutofill');
  await Future.delayed(const Duration(milliseconds: 300));

  // 10. finishAutofillContext
  await SystemChannels.textInput.invokeMethod(
    'TextInput.finishAutofillContext',
    true,
  );
  await Future.delayed(const Duration(milliseconds: 300));

  // 11. setClient with NONE input type
  await SystemChannels.textInput.invokeMethod('TextInput.setClient', [
    3,
    {
      'inputAction': 'TextInputAction.done',
      'inputType': {
        'name': 'TextInputType.none',
        'signed': false,
        'decimal': false,
      },
    },
  ]);
  await Future.delayed(const Duration(milliseconds: 300));

  // 12. sendAppPrivateCommand
  await SystemChannels.textInput.invokeMethod(
    'TextInput.sendAppPrivateCommand',
    '',
  );
  await Future.delayed(const Duration(milliseconds: 300));

  // 13. hide + clearClient
  await SystemChannels.textInput.invokeMethod('TextInput.hide');
  await Future.delayed(const Duration(milliseconds: 300));
  await SystemChannels.textInput.invokeMethod('TextInput.clearClient');

  // 14. New client with composing region to trigger composingRegionMutatedByFramework
  await SystemChannels.textInput.invokeMethod('TextInput.setClient', [
    4,
    {
      'inputAction': 'TextInputAction.done',
      'inputType': {
        'name': 'TextInputType.text',
        'signed': false,
        'decimal': false,
      },
    },
  ]);
  await SystemChannels.textInput.invokeMethod('TextInput.show');

  // 15. setEditingState with composing -> sets mLastOutgoingFrameworkState with composing
  await SystemChannels.textInput.invokeMethod('TextInput.setEditingState', {
    'text': 'hello',
    'selectionBase': 0,
    'selectionExtent': 5,
    'composingBase': 0,
    'composingExtent': 3,
  });
  await Future.delayed(const Duration(milliseconds: 300));

  // 16. setEditingState again with different composing -> triggers composingRegionMutatedByFramework
  await SystemChannels.textInput.invokeMethod('TextInput.setEditingState', {
    'text': 'hello world',
    'selectionBase': 0,
    'selectionExtent': 11,
    'composingBase': 0,
    'composingExtent': 5,
  });
  await Future.delayed(const Duration(milliseconds: 300));

  // 17. setEditingState with -1 selection (no selection)
  await SystemChannels.textInput.invokeMethod('TextInput.setEditingState', {
    'text': 'test',
    'selectionBase': -1,
    'selectionExtent': -1,
    'composingBase': -1,
    'composingExtent': -1,
  });
  await Future.delayed(const Duration(milliseconds: 300));

  // 18. setEditingState with selection in middle
  await SystemChannels.textInput.invokeMethod('TextInput.setEditingState', {
    'text': 'hello world',
    'selectionBase': 2,
    'selectionExtent': 7,
    'composingBase': 2,
    'composingExtent': 5,
  });
  await Future.delayed(const Duration(milliseconds: 300));

  // 19. setClient with visiblePassword type -> triggers hasSecureKeyboardInSwitch
  await SystemChannels.textInput.invokeMethod('TextInput.setClient', [
    5,
    {
      'inputAction': 'TextInputAction.done',
      'inputType': {
        'name': 'TextInputType.visiblePassword',
        'signed': false,
        'decimal': false,
      },
    },
  ]);
  await SystemChannels.textInput.invokeMethod('TextInput.show');
  await Future.delayed(const Duration(milliseconds: 300));

  // 20. setEditingState with empty text
  await SystemChannels.textInput.invokeMethod('TextInput.setEditingState', {
    'text': '',
    'selectionBase': 0,
    'selectionExtent': 0,
    'composingBase': -1,
    'composingExtent': -1,
  });
  await Future.delayed(const Duration(milliseconds: 300));

  // 21. multiple setClient cycles to trigger handleChangeFocus path
  await SystemChannels.textInput.invokeMethod('TextInput.setClient', [
    6,
    {
      'inputAction': 'TextInputAction.go',
      'inputType': {
        'name': 'TextInputType.emailAddress',
        'signed': false,
        'decimal': false,
      },
    },
  ]);
  await SystemChannels.textInput.invokeMethod('TextInput.show');
  await Future.delayed(const Duration(milliseconds: 300));
  await SystemChannels.textInput.invokeMethod('TextInput.setEditingState', {
    'text': 'test@example.com',
    'selectionBase': 0,
    'selectionExtent': 16,
    'composingBase': -1,
    'composingExtent': -1,
  });
  await Future.delayed(const Duration(milliseconds: 300));

  // 22. clearClient then setClient again
  await SystemChannels.textInput.invokeMethod('TextInput.clearClient');
  await Future.delayed(const Duration(milliseconds: 300));
  await SystemChannels.textInput.invokeMethod('TextInput.setClient', [
    7,
    {
      'inputAction': 'TextInputAction.newline',
      'inputType': {
        'name': 'TextInputType.multiline',
        'signed': false,
        'decimal': false,
      },
    },
  ]);
  await SystemChannels.textInput.invokeMethod('TextInput.show');
  await Future.delayed(const Duration(milliseconds: 300));
  await SystemChannels.textInput.invokeMethod('TextInput.setEditingState', {
    'text': 'line1\nline2',
    'selectionBase': 0,
    'selectionExtent': 11,
    'composingBase': -1,
    'composingExtent': -1,
  });
  await Future.delayed(const Duration(milliseconds: 300));

  // 23. Final cleanup
  await SystemChannels.textInput.invokeMethod('TextInput.hide');
  await Future.delayed(const Duration(milliseconds: 300));
  await SystemChannels.textInput.invokeMethod('TextInput.clearClient');

  // 24. Trigger platform channel messages to cover PlatformPlugin
  await SystemChannels.platform.invokeMethod(
    'SystemSound.play',
    'SystemSoundType.click',
  );
  await Future.delayed(const Duration(milliseconds: 200));
  await SystemChannels.platform.invokeMethod(
    'HapticFeedback.vibrate',
    'HapticFeedbackType.standard',
  );
  await Future.delayed(const Duration(milliseconds: 200));
  await SystemChannels.platform.invokeMethod(
    'SystemChrome.setEnabledSystemUIOverlays',
    // The ArkTS handler whitelists "SystemUiOverlay.top"/"SystemUiOverlay.bottom";
    // plain "top"/"bottom" makes it throw and kills the rest of this trigger chain.
    ['SystemUiOverlay.top', 'SystemUiOverlay.bottom'],
  );
  await Future.delayed(const Duration(milliseconds: 200));
  await SystemChannels.platform.invokeMethod(
    'SystemChrome.setEnabledSystemUIMode',
    'SystemUiMode.edgeToEdge',
  );
  await Future.delayed(const Duration(milliseconds: 200));
  await SystemChannels.platform.invokeMethod(
    'SystemChrome.setSystemUIOverlayStyle',
    {'systemNavigationBarColor': '#000000'},
  );
  await Future.delayed(const Duration(milliseconds: 200));
  await SystemChannels.platform.invokeMethod(
    'SystemChrome.setPreferredOrientations',
    ['DeviceOrientation.portraitUp'],
  );
  await Future.delayed(const Duration(milliseconds: 200));
  await SystemChannels.platform.invokeMethod(
    'SystemChrome.setApplicationSwitcherDescription',
    {'label': 'Test', 'primaryColor': '#000000'},
  );
  await Future.delayed(const Duration(milliseconds: 200));
  await SystemChannels.platform.invokeMethod(
    'SystemChrome.restoreSystemUIOverlays',
  );
  await Future.delayed(const Duration(milliseconds: 200));
  await SystemChannels.platform.invokeMethod('Clipboard.setData', {
    'text': 'test clipboard',
  });
  await Future.delayed(const Duration(milliseconds: 200));
  await SystemChannels.platform.invokeMethod('Clipboard.getData', 'text/plain');
  await Future.delayed(const Duration(milliseconds: 200));

  // 25. Navigation channel messages
  await SystemChannels.navigation.invokeMethod('routeInformationUpdated', {
    'location': '/test',
    'state': null,
    'replace': false,
  });
  await Future.delayed(const Duration(milliseconds: 200));
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Flutter Demo',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.deepPurple),
        useMaterial3: true,
      ),
      home: const MyHomePage(title: 'Flutter Demo Home Page'),
    );
  }
}

class MyHomePage extends StatefulWidget {
  const MyHomePage({super.key, required this.title});

  final String title;

  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  final TextEditingController _textController = TextEditingController();
  final FocusNode _focusNode = FocusNode();

  @override
  void dispose() {
    _textController.dispose();
    _focusNode.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      key: const Key('home_scaffold'),
      appBar: AppBar(
        key: const Key('home_appbar'),
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
        title: Text(widget.title),
      ),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: <Widget>[
            SizedBox(
              width: 240,
              child: TextField(
                key: const Key('home_text_field'),
                controller: _textController,
                focusNode: _focusNode,
                decoration: const InputDecoration(
                  border: OutlineInputBorder(),
                  labelText: 'Enter text',
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
