// Copyright 2014 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:args/args.dart';

import '../base/common.dart';
import '../base/utils.dart';
import '../doctor_validator.dart';
import '../emulator.dart';
import '../globals.dart' as globals;
import '../ohos/ohos_emulators.dart';
import '../runner/flutter_command.dart';

class EmulatorsCommand extends FlutterCommand {
  EmulatorsCommand() {
    argParser.addOption('launch', help: 'The full or partial ID of the emulator to launch.');
    argParser.addFlag(
      'cold',
      help: 'Used with the "--launch" flag to cold boot the emulator instance (Android only).',
      negatable: false,
    );
    argParser.addFlag(
      'create',
      help: 'Creates a new Android emulator based on a Pixel device.',
      negatable: false,
    );
    argParser.addOption(
      'name',
      help: 'Used with the "--create" flag. Specifies a name for the emulator being created.',
    );
    argParser.addOption(
      'launch-ohos-emulator',
      help: 'Launch the emulator instance (Ohos only). Specify the emulator name.',
    );
    argParser.addOption(
      'create-ohos-emulator',
      help: 'Creates a new Ohos emulator. Specify the emulator name.',
    );
    argParser.addOption(
      'devicetype',
      help:
          'Used with "--create-ohos-emulator". Specify the device type (e.g. phone, tablet). '
          'Defaults to "phone".',
    );
    argParser.addOption(
      'osversion',
      help:
          'Used with "--create-ohos-emulator". Specify the OS version (e.g. "HarmonyOS 6.0.0(20)"). '
          'Defaults to the OS version of the downloaded image for the device type.',
    );
    argParser.addFlag('list-ohos-emulator', help: 'Lists all Ohos emulators.', negatable: false);
    argParser.addFlag(
      'print-ohos-emulator',
      help: 'Prints detailed information about Ohos emulators.',
      negatable: false,
    );
  }

  final _ohosEmulators = OhosEmulators();

  @override
  final name = 'emulators';

  @override
  final description = 'List, launch and create emulators.';

  @override
  final String category = FlutterCommandCategory.tools;

  @override
  final aliases = <String>['emulator'];

  @override
  Future<FlutterCommandResult> runCommand() async {
    final ArgResults argumentResults = argResults!;
    // Ohos emulator commands do not depend on the Android/iOS emulator
    // sources checked below, so they must skip that check: machines with
    // only an Ohos SDK installed must still be able to use them.
    final bool ohosEmulatorCommand =
        argumentResults.wasParsed('launch-ohos-emulator') ||
        argumentResults.wasParsed('create-ohos-emulator') ||
        argumentResults.wasParsed('list-ohos-emulator') ||
        argumentResults.wasParsed('print-ohos-emulator');
    if (!ohosEmulatorCommand &&
        globals.doctor!.workflows.every((Workflow w) => !w.canListEmulators)) {
      throwToolExit(
        'Unable to find any emulator sources. Please ensure you have some\n'
        'Android AVD images ${globals.platform.isMacOS ? 'or an iOS Simulator ' : ''}available.',
        exitCode: 1,
      );
    }
    if (argumentResults.wasParsed('launch')) {
      final bool coldBoot = argumentResults.wasParsed('cold');
      await _launchEmulator(stringArg('launch')!, coldBoot: coldBoot);
    } else if (argumentResults.wasParsed('create')) {
      await _createEmulator(name: stringArg('name'));
    } else if (argumentResults.wasParsed('launch-ohos-emulator')) {
      final String? ohosName = stringArg('launch-ohos-emulator');
      if (ohosName == null || ohosName.isEmpty) {
        throwToolExit('--launch-ohos-emulator requires an emulator name.', exitCode: 1);
      }
      // Check if there are remaining positional arguments, which may indicate an unquoted value
      if (argumentResults.rest.isNotEmpty) {
        throwToolExit(
          '--launch-ohos-emulator value may contain spaces. Please wrap the value in double quotes, e.g. --launch-ohos-emulator "Pura X View".',
          exitCode: 1,
        );
      }
      await _ohosEmulators.launchEmulator(ohosName);
    } else if (argumentResults.wasParsed('create-ohos-emulator')) {
      final String? ohosName = stringArg('create-ohos-emulator');
      final String? ohosDeviceType = stringArg('devicetype');
      final String? ohosOsVersion = stringArg('osversion');
      if (ohosName == null || ohosName.isEmpty) {
        throwToolExit('--create-ohos-emulator requires an emulator name.', exitCode: 1);
      }
      // Check if there are remaining positional arguments, which may indicate unquoted value
      if (argumentResults.rest.isNotEmpty) {
        throwToolExit(
          'One of the option values may contain spaces. Please wrap values containing spaces in double quotes, e.g. --create-ohos-emulator "Pura X View" --osversion "HarmonyOS 6.0.0(20)".',
          exitCode: 1,
        );
      }
      await _ohosEmulators.createEmulator(
        name: ohosName,
        deviceType: ohosDeviceType,
        osVersion: ohosOsVersion,
      );
    } else if (argumentResults.wasParsed('list-ohos-emulator')) {
      await _ohosEmulators.listEmulators();
    } else if (argumentResults.wasParsed('print-ohos-emulator')) {
      await _ohosEmulators.printEmulatorDetails();
    } else {
      final String? searchText = argumentResults.rest.isNotEmpty
          ? argumentResults.rest.first
          : null;
      await _listEmulators(searchText);
    }

    return FlutterCommandResult.success();
  }

  Future<void> _launchEmulator(String id, {required bool coldBoot}) async {
    final List<Emulator> emulators = await emulatorManager!.getEmulatorsMatching(id);

    if (emulators.isEmpty) {
      globals.printStatus("No emulator found that matches '$id'.");
    } else if (emulators.length > 1) {
      _printEmulatorList(emulators, "More than one emulator matches '$id':");
    } else {
      await emulators.first.launch(coldBoot: coldBoot);
    }
  }

  Future<void> _createEmulator({String? name}) async {
    final CreateEmulatorResult createResult = await emulatorManager!.createEmulator(name: name);

    if (createResult.success) {
      globals.printStatus("Emulator '${createResult.emulatorName}' created successfully.");
    } else {
      globals.printStatus("Failed to create emulator '${createResult.emulatorName}'.\n");
      final String? error = createResult.error;
      if (error != null) {
        globals.printStatus(error.trim());
      }
      _printAdditionalInfo();
    }
  }

  Future<void> _listEmulators(String? searchText) async {
    final List<Emulator> emulators = searchText == null
        ? await emulatorManager!.getAllAvailableEmulators()
        : await emulatorManager!.getEmulatorsMatching(searchText);

    if (emulators.isEmpty) {
      globals.printStatus('No emulators available.');
      _printAdditionalInfo(showCreateInstruction: true);
    } else {
      _printEmulatorList(
        emulators,
        '${emulators.length} available ${pluralize('emulator', emulators.length)}:',
      );
    }
  }

  void _printEmulatorList(List<Emulator> emulators, String message) {
    globals.printStatus('$message\n');
    Emulator.printEmulators(emulators, globals.logger);
    _printAdditionalInfo(showCreateInstruction: true, showRunInstruction: true);
  }

  void _printAdditionalInfo({bool showRunInstruction = false, bool showCreateInstruction = false}) {
    globals.printStatus('');
    if (showRunInstruction) {
      globals.printStatus("To run an emulator, run 'flutter emulators --launch <emulator id>'.");
    }
    if (showCreateInstruction) {
      globals.printStatus(
        "To create a new emulator, run 'flutter emulators --create [--name xyz]'.",
      );
    }

    if (showRunInstruction || showCreateInstruction) {
      globals.printStatus('');
    }
    // TODO(dantup): Update this link to flutter.dev if/when we have a better page.
    // That page can then link out to these places if required.
    globals.printStatus(
      'You can find more information on managing emulators at the links below:\n'
      '  https://developer.android.com/studio/run/managing-avds\n'
      '  https://developer.android.com/studio/command-line/avdmanager',
    );
  }
}
