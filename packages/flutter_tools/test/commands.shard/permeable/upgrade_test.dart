// Copyright 2014 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:args/command_runner.dart';
import 'package:file/memory.dart';
import 'package:flutter_tools/src/base/file_system.dart';
import 'package:flutter_tools/src/base/io.dart';
import 'package:flutter_tools/src/base/logger.dart';
import 'package:flutter_tools/src/base/platform.dart';
import 'package:flutter_tools/src/base/time.dart';
import 'package:flutter_tools/src/cache.dart';
import 'package:flutter_tools/src/commands/upgrade.dart';
import 'package:flutter_tools/src/context_runner.dart';
import 'package:flutter_tools/src/convert.dart';
import 'package:flutter_tools/src/globals.dart' as globals;
import 'package:flutter_tools/src/ohos/ohos_upgrade.dart';
import 'package:flutter_tools/src/persistent_tool_state.dart';
import 'package:flutter_tools/src/runner/flutter_command.dart';
import 'package:flutter_tools/src/version.dart';

import '../../src/common.dart';
import '../../src/context.dart';
import '../../src/fake_process_manager.dart';
import '../../src/fakes.dart';
import '../../src/test_flutter_command_runner.dart';

void main() {
  group('UpgradeCommandRunner', () {
    final jan12026 = DateTime.utc(2026);

    late FakeUpgradeCommandRunner fakeCommandRunner;
    late UpgradeCommandRunner realCommandRunner;
    late FakeProcessManager processManager;
    late FakePlatform fakePlatform;
    const gitTagVersion = GitTagVersion(
      x: 1,
      y: 2,
      z: 3,
      hotfix: 4,
      commits: 5,
      hash: 'asd',
      gitTag: '1.2.3+hotfix.5.pre.5',
    );

    setUp(() {
      fakeCommandRunner = FakeUpgradeCommandRunner()..clock = SystemClock.fixed(jan12026);
      realCommandRunner = UpgradeCommandRunner()
        ..workingDirectory = getFlutterRoot()
        ..clock = SystemClock.fixed(jan12026);
      processManager = FakeProcessManager.empty();
      fakeCommandRunner.willHaveUncommittedChanges = false;
      fakePlatform = FakePlatform()
        ..environment = Map<String, String>.unmodifiable(<String, String>{
          'ENV1': 'irrelevant',
          'ENV2': 'irrelevant',
        });
    });

    testUsingContext(
      'throws on unknown tag, official branch,  noforce',
      () async {
        final flutterVersion = FakeFlutterVersion(branch: 'beta');
        const upstreamRevision = '';
        final latestVersion = FakeFlutterVersion(frameworkRevision: upstreamRevision);
        fakeCommandRunner.remoteVersion = latestVersion;

        final Future<FlutterCommandResult> result = fakeCommandRunner.runCommand(
          const UpgradePhase.firstHalf(),
          force: false,
          testFlow: false,
          gitTagVersion: const GitTagVersion.unknown(),
          flutterVersion: flutterVersion,
          verifyOnly: false,
        );
        expect(result, throwsToolExit());
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{Platform: () => fakePlatform},
    );

    testUsingContext(
      'throws tool exit with uncommitted changes',
      () async {
        final flutterVersion = FakeFlutterVersion(branch: 'beta');
        const upstreamRevision = '';
        final latestVersion = FakeFlutterVersion(frameworkRevision: upstreamRevision);
        fakeCommandRunner.remoteVersion = latestVersion;
        fakeCommandRunner.willHaveUncommittedChanges = true;

        final Future<FlutterCommandResult> result = fakeCommandRunner.runCommand(
          const UpgradePhase.firstHalf(),
          force: false,
          testFlow: false,
          gitTagVersion: gitTagVersion,
          flutterVersion: flutterVersion,
          verifyOnly: false,
        );
        expect(result, throwsToolExit());
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{Platform: () => fakePlatform},
    );

    testUsingContext(
      "Doesn't continue on known tag, beta branch, no force, already up-to-date",
      () async {
        const revision = 'abc123';
        final latestVersion = FakeFlutterVersion(frameworkRevision: revision);
        final flutterVersion = FakeFlutterVersion(branch: 'beta', frameworkRevision: revision);
        fakeCommandRunner.alreadyUpToDate = true;
        fakeCommandRunner.remoteVersion = latestVersion;

        final Future<FlutterCommandResult> result = fakeCommandRunner.runCommand(
          const UpgradePhase.firstHalf(),
          force: false,
          testFlow: false,
          gitTagVersion: gitTagVersion,
          flutterVersion: flutterVersion,
          verifyOnly: false,
        );
        expect(await result, FlutterCommandResult.success());
        expect(testLogger.statusText, contains('Flutter is already up to date'));
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        ProcessManager: () => processManager,
        Platform: () => fakePlatform,
      },
    );

    testUsingContext(
      'starts the upgrade operation and passes now as --continue-started-at <iso-date>',
      () async {
        const revision = 'abc123';
        const upstreamRevision = 'def456';
        const version = '1.2.3';
        const upstreamVersion = '4.5.6';

        final flutterVersion = FakeFlutterVersion(
          branch: 'beta',
          frameworkRevision: revision,
          frameworkRevisionShort: revision,
          frameworkVersion: version,
        );

        final latestVersion = FakeFlutterVersion(
          frameworkRevision: upstreamRevision,
          frameworkRevisionShort: upstreamRevision,
          frameworkVersion: upstreamVersion,
        );

        final DateTime now = DateTime.now().subtract(const Duration(minutes: 25));
        fakeCommandRunner.remoteVersion = latestVersion;
        fakeCommandRunner.clock = SystemClock.fixed(now);

        processManager.addCommands(<FakeCommand>[
          FakeCommand(
            command: <String>[
              globals.fs.path.join('bin', 'flutter'),
              'upgrade',
              '--continue',
              '--continue-started-at',
              now.toIso8601String(),
              '--no-version-check',
            ],
          ),
        ]);

        final Future<FlutterCommandResult> result = fakeCommandRunner.runCommand(
          const UpgradePhase.firstHalf(),
          force: false,
          testFlow: false,
          gitTagVersion: gitTagVersion,
          flutterVersion: flutterVersion,
          verifyOnly: false,
        );
        expect(await result, FlutterCommandResult.success());
        expect(testLogger.statusText, isNot(contains('Took ')));
      },
      overrides: <Type, Generator>{
        ProcessManager: () => processManager,
        Platform: () => fakePlatform,
      },
    );

    testUsingContext(
      'finishes the upgrade operation and prints minutes the operation took',
      () async {
        const revision = 'abc123';
        const upstreamRevision = 'def456';
        const version = '1.2.3';
        const upstreamVersion = '4.5.6';

        final flutterVersion = FakeFlutterVersion(
          branch: 'beta',
          frameworkRevision: revision,
          frameworkRevisionShort: revision,
          frameworkVersion: version,
        );

        final latestVersion = FakeFlutterVersion(
          frameworkRevision: upstreamRevision,
          frameworkRevisionShort: upstreamRevision,
          frameworkVersion: upstreamVersion,
        );

        fakeCommandRunner.remoteVersion = latestVersion;

        final now = DateTime.now();
        final DateTime before = now.subtract(const Duration(minutes: 25));
        fakeCommandRunner.clock = SystemClock.fixed(now);

        processManager.addCommands(<FakeCommand>[
          FakeCommand(
            command: <String>[
              globals.fs.path.join('bin', 'flutter'),
              '--no-color',
              '--no-version-check',
              'precache',
            ],
          ),
        ]);

        final Future<FlutterCommandResult> result = fakeCommandRunner.runCommand(
          UpgradePhase.secondHalf(upgradeStartedAt: before),
          force: false,
          testFlow: false,
          gitTagVersion: gitTagVersion,
          flutterVersion: flutterVersion,
          verifyOnly: false,
        );
        expect(await result, FlutterCommandResult.success());
        expect(testLogger.statusText, contains('Took 25.0m'));
      },
      overrides: <Type, Generator>{
        ProcessManager: () => processManager,
        Platform: () => fakePlatform,
      },
    );

    testUsingContext(
      'correctly provides upgrade version on verify only',
      () async {
        const revision = 'abc123';
        const upstreamRevision = 'def456';
        const version = '1.2.3';
        const upstreamVersion = '4.5.6';

        final flutterVersion = FakeFlutterVersion(
          branch: 'beta',
          frameworkRevision: revision,
          frameworkRevisionShort: revision,
          frameworkVersion: version,
        );

        final latestVersion = FakeFlutterVersion(
          frameworkRevision: upstreamRevision,
          frameworkRevisionShort: upstreamRevision,
          frameworkVersion: upstreamVersion,
        );

        fakeCommandRunner.alreadyUpToDate = false;
        fakeCommandRunner.remoteVersion = latestVersion;

        final Future<FlutterCommandResult> result = fakeCommandRunner.runCommand(
          const UpgradePhase.firstHalf(),
          force: false,
          testFlow: false,
          gitTagVersion: gitTagVersion,
          flutterVersion: flutterVersion,
          verifyOnly: true,
        );
        expect(await result, FlutterCommandResult.success());
        expect(testLogger.statusText, contains('A new version of Flutter is available'));
        expect(testLogger.statusText, contains('The latest version: 4.5.6 (revision def456)'));
        expect(testLogger.statusText, contains('Your current version: 1.2.3 (revision abc123)'));
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        ProcessManager: () => processManager,
        Platform: () => fakePlatform,
      },
    );

    testUsingContext(
      'fetchLatestVersion returns version if git succeeds',
      () async {
        const revision = 'abc123';
        const version = '1.2.3';

        processManager.addCommands(<FakeCommand>[
          const FakeCommand(command: <String>['git', 'fetch', '--tags']),
          const FakeCommand(
            command: <String>['git', 'rev-parse', '--verify', '@{upstream}'],
            stdout: revision,
          ),
          const FakeCommand(
            command: <String>['git', 'tag', '--points-at', revision],
            stdout: version,
          ),
        ]);

        final FlutterVersion updateVersion = await realCommandRunner.fetchLatestVersion(
          localVersion: FakeFlutterVersion(),
        );

        expect(updateVersion.frameworkVersion, version);
        expect(updateVersion.frameworkRevision, revision);
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        ProcessManager: () => processManager,
        Platform: () => fakePlatform,
      },
      skip: true, // OHOS not supported
    );

    testUsingContext(
      'fetchLatestVersion returns latest version if the upstream revision has multiple tags',
      () async {
        const latestVersion = '3.4.5';
        const tags = <String>['1.2.3', '1.2.4', latestVersion];
        const revision = 'abc123';

        processManager.addCommands(<FakeCommand>[
          const FakeCommand(command: <String>['git', 'fetch', '--tags']),
          const FakeCommand(
            command: <String>['git', 'rev-parse', '--verify', '@{upstream}'],
            stdout: revision,
          ),
          FakeCommand(
            command: const <String>['git', 'tag', '--points-at', revision],
            stdout: tags.join('\n'),
          ),
        ]);

        final FlutterVersion updateVersion = await realCommandRunner.fetchLatestVersion(
          localVersion: FakeFlutterVersion(),
        );

        expect(updateVersion.frameworkVersion, latestVersion);
        expect(updateVersion.frameworkRevision, revision);
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        ProcessManager: () => processManager,
        Platform: () => fakePlatform,
      },
      skip: true, // OHOS not supported
    );

    testUsingContext(
      'fetchLatestVersion throws toolExit if HEAD is detached',
      () async {
        processManager.addCommands(const <FakeCommand>[
          FakeCommand(command: <String>['git', 'fetch', '--tags']),
          FakeCommand(
            command: <String>['git', 'rev-parse', '--verify', '@{upstream}'],
            exception: ProcessException('git', <String>[
              'rev-parse',
              '--verify',
              '@{upstream}',
            ], 'fatal: HEAD does not point to a branch'),
          ),
        ]);

        await expectLater(
          () async => realCommandRunner.fetchLatestVersion(localVersion: FakeFlutterVersion()),
          throwsToolExit(
            message:
                'Unable to upgrade Flutter: Your Flutter checkout '
                'is currently not on a release branch.\n'
                'Use "flutter channel" to switch to an official channel, and retry. '
                'Alternatively, re-install Flutter by going to https://flutter.dev/setup.',
          ),
        );
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        ProcessManager: () => processManager,
        Platform: () => fakePlatform,
      },
      skip: true, // OHOS not supported
    );

    testUsingContext(
      'fetchLatestVersion throws toolExit if no upstream configured',
      () async {
        processManager.addCommands(const <FakeCommand>[
          FakeCommand(command: <String>['git', 'fetch', '--tags']),
          FakeCommand(
            command: <String>['git', 'rev-parse', '--verify', '@{upstream}'],
            exception: ProcessException('git', <String>[
              'rev-parse',
              '--verify',
              '@{upstream}',
            ], 'fatal: no upstream configured for branch'),
          ),
        ]);

        await expectLater(
          () async => realCommandRunner.fetchLatestVersion(localVersion: FakeFlutterVersion()),
          throwsToolExit(
            message:
                'Unable to upgrade Flutter: The current Flutter '
                'branch/channel is not tracking any remote repository.\n'
                'Re-install Flutter by going to https://flutter.dev/setup.',
          ),
        );
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        ProcessManager: () => processManager,
        Platform: () => fakePlatform,
      },
      skip: true, // OHOS not supported
    );

    testUsingContext(
      'git exception during attemptReset throwsToolExit',
      () async {
        const revision = 'abc123';
        const errorMessage = 'fatal: Could not parse object ´$revision´';
        processManager.addCommand(
          const FakeCommand(
            command: <String>['git', 'reset', '--hard', revision],
            exception: ProcessException('git', <String>['reset', '--hard', revision], errorMessage),
          ),
        );

        await expectLater(
          () async => realCommandRunner.attemptReset(revision),
          throwsToolExit(message: errorMessage),
        );
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        ProcessManager: () => processManager,
        Platform: () => fakePlatform,
      },
    );

    testUsingContext(
      'flutterUpgradeContinue passes env variables to child process',
      () async {
        processManager.addCommand(
          FakeCommand(
            command: <String>[
              globals.fs.path.join('bin', 'flutter'),
              'upgrade',
              '--continue',
              '--continue-started-at',
              '2026-01-01T00:00:00.000Z',
              '--no-version-check',
            ],
            environment: <String, String>{
              'FLUTTER_ALREADY_LOCKED': 'true',
              ...fakePlatform.environment,
            },
          ),
        );
        await realCommandRunner.flutterUpgradeContinue(startedAt: jan12026);
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        ProcessManager: () => processManager,
        Platform: () => fakePlatform,
      },
    );

    testUsingContext(
      'Show current version to the upgrade message.',
      () async {
        const revision = 'abc123';
        const upstreamRevision = 'def456';
        const version = '1.2.3';
        const upstreamVersion = '4.5.6';

        final flutterVersion = FakeFlutterVersion(
          branch: 'beta',
          frameworkRevision: revision,
          frameworkVersion: version,
        );
        final latestVersion = FakeFlutterVersion(
          frameworkRevision: upstreamRevision,
          frameworkVersion: upstreamVersion,
        );

        fakeCommandRunner.alreadyUpToDate = false;
        fakeCommandRunner.remoteVersion = latestVersion;
        fakeCommandRunner.workingDirectory = 'workingDirectory/aaa/bbb';

        final Future<FlutterCommandResult> result = fakeCommandRunner.runCommand(
          const UpgradePhase.firstHalf(),
          force: true,
          testFlow: true,
          gitTagVersion: gitTagVersion,
          flutterVersion: flutterVersion,
          verifyOnly: false,
        );
        expect(await result, FlutterCommandResult.success());
        expect(
          testLogger.statusText,
          contains('Upgrading Flutter to 4.5.6 from 1.2.3 in workingDirectory/aaa/bbb...'),
        );
      },
      overrides: <Type, Generator>{
        ProcessManager: () => processManager,
        Platform: () => fakePlatform,
      },
    );

    testUsingContext(
      'Can upgrade to a newer version with the same framework Git revision',
      () async {
        const revision = 'abc123';
        const version = '1.2.3';
        const upstreamVersion = '4.5.6';

        final flutterVersion = FakeFlutterVersion(
          frameworkRevision: revision,
          frameworkVersion: version,
          gitTagVersion: GitTagVersion.parse(version),
        );
        final latestVersion = FakeFlutterVersion(
          frameworkRevision: revision,
          frameworkVersion: upstreamVersion,
          gitTagVersion: GitTagVersion.parse(upstreamVersion),
        );

        fakeCommandRunner.alreadyUpToDate = false;
        fakeCommandRunner.remoteVersion = latestVersion;
        fakeCommandRunner.workingDirectory = 'workingDirectory/aaa/bbb';

        final Future<FlutterCommandResult> result = fakeCommandRunner.runCommand(
          const UpgradePhase.firstHalf(),
          force: true,
          testFlow: true,
          gitTagVersion: gitTagVersion,
          flutterVersion: flutterVersion,
          verifyOnly: false,
        );
        expect(await result, FlutterCommandResult.success());
        expect(
          testLogger.statusText,
          contains('Upgrading Flutter to 4.5.6 from 1.2.3 in workingDirectory/aaa/bbb...'),
        );
      },
      overrides: <Type, Generator>{
        ProcessManager: () => processManager,
        Platform: () => fakePlatform,
      },
    );

    testUsingContext(
      'always regenerates flutter.version.json',
      () async {
        const revision = 'abc123';
        const version = '3.38.1';
        const upstreamVersion = '3.38.2';
        const engineRevision = 'zyxw';

        final flutterVersion = FakeFlutterVersion(
          frameworkRevision: revision,
          frameworkVersion: version,
          engineRevision: engineRevision,
          gitTagVersion: GitTagVersion.parse(version),
        );

        final latestVersion = FakeFlutterVersion(
          frameworkRevision: revision,
          frameworkVersion: upstreamVersion,
          engineRevision: engineRevision,
          gitTagVersion: GitTagVersion.parse(upstreamVersion),
        );

        final CommandRunner<void> runner = createTestCommandRunner(
          UpgradeCommand(verboseHelp: false, commandRunner: fakeCommandRunner),
        );

        fakeCommandRunner.alreadyUpToDate = false;
        fakeCommandRunner.remoteVersion = latestVersion;
        fakeCommandRunner.workingDirectory = 'workingDirectory/aaa/bbb';

        await runInContext(
          () async {
            await runner.run(<String>['upgrade', '--force']);
            // Verify that flutter.version.json is deleted before running phase two of the
            // upgrade command. This should cause the version file to be regenerated based on the
            // current branch state.
            expect(flutterVersion.didDeleteVersionFile, true);
          },
          overrides: <Type, Generator>{
            FlutterVersion: () => flutterVersion,
            ProcessManager: () => FakeProcessManager.any(),
            Platform: () => fakePlatform,
          },
        );

        await runInContext(() async {
          await runner.run(<String>['upgrade', '--continue']);
          // Verify that ensureVersionFile() was invoked, which will create flutter.version.json
          // if it doesn't exist.
          expect(latestVersion.didEnsureVersionFile, true);
          expect(latestVersion.didDeleteVersionFile, false);
        }, overrides: {FlutterVersion: () => latestVersion});
      },
      skip: true, // OHOS not supported
      overrides: <Type, Generator>{
        ProcessManager: () => FakeProcessManager.any(),
        Platform: () => fakePlatform,
      },
    );

    testUsingContext(
      'Does not confuse an older release with the same revision as an available update',
      () async {
        const revision = 'abc123';
        const version = '4.5.6';
        final GitTagVersion currentTag = GitTagVersion.parse(version);
        const upstreamVersion = '1.2.3';
        final GitTagVersion upstreamTag = GitTagVersion.parse(upstreamVersion);

        final flutterVersion = FakeFlutterVersion(
          branch: 'stable',
          frameworkRevision: revision,
          frameworkVersion: version,
          gitTagVersion: currentTag,
        );
        final latestVersion = FakeFlutterVersion(
          branch: 'stable',
          frameworkRevision: revision,
          frameworkVersion: upstreamVersion,
          gitTagVersion: upstreamTag,
        );

        fakeCommandRunner.alreadyUpToDate = false;
        fakeCommandRunner.remoteVersion = latestVersion;
        fakeCommandRunner.workingDirectory = 'workingDirectory/aaa/bbb';

        final Future<FlutterCommandResult> result = fakeCommandRunner.runCommand(
          const UpgradePhase.firstHalf(),
          force: true,
          testFlow: true,
          gitTagVersion: currentTag,
          flutterVersion: flutterVersion,
          verifyOnly: false,
        );
        expect(await result, FlutterCommandResult.success());
        expect(testLogger.statusText, contains('Flutter is already up to date on channel stable'));
      },
      overrides: <Type, Generator>{
        ProcessManager: () => processManager,
        Platform: () => fakePlatform,
      },
    );

    testUsingContext(
      'precacheArtifacts passes env variables to child process',
      () async {
        processManager.addCommand(
          FakeCommand(
            command: <String>[
              globals.fs.path.join('bin', 'flutter'),
              '--no-color',
              '--no-version-check',
              'precache',
            ],
            environment: <String, String>{
              'FLUTTER_ALREADY_LOCKED': 'true',
              ...fakePlatform.environment,
            },
          ),
        );
        await precacheArtifacts();
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        ProcessManager: () => processManager,
        Platform: () => fakePlatform,
      },
    );

    group('runs upgrade', () {
      setUp(() {
        processManager.addCommand(
          FakeCommand(
            command: <String>[
              globals.fs.path.join('bin', 'flutter'),
              'upgrade',
              '--continue',
              '--continue-started-at',
              '2026-01-01T00:00:00.000Z',
              '--no-version-check',
            ],
          ),
        );
      });

      testUsingContext(
        'does not throw on unknown tag, official branch, force',
        () async {
          fakeCommandRunner.remoteVersion = FakeFlutterVersion(frameworkRevision: '1234');
          final flutterVersion = FakeFlutterVersion(branch: 'beta');

          final Future<FlutterCommandResult> result = fakeCommandRunner.runCommand(
            const UpgradePhase.firstHalf(),
            force: true,
            testFlow: false,
            gitTagVersion: const GitTagVersion.unknown(),
            flutterVersion: flutterVersion,
            verifyOnly: false,
          );
          expect(await result, FlutterCommandResult.success());
          expect(processManager, hasNoRemainingExpectations);
        },
        overrides: <Type, Generator>{
          ProcessManager: () => processManager,
          Platform: () => fakePlatform,
        },
      );

      testUsingContext(
        'does not throw tool exit with uncommitted changes and force',
        () async {
          final flutterVersion = FakeFlutterVersion(branch: 'beta');
          fakeCommandRunner.remoteVersion = FakeFlutterVersion(frameworkRevision: '1234');
          fakeCommandRunner.willHaveUncommittedChanges = true;

          final Future<FlutterCommandResult> result = fakeCommandRunner.runCommand(
            const UpgradePhase.firstHalf(),
            force: true,
            testFlow: false,
            gitTagVersion: gitTagVersion,
            flutterVersion: flutterVersion,
            verifyOnly: false,
          );
          expect(await result, FlutterCommandResult.success());
          expect(processManager, hasNoRemainingExpectations);
        },
        overrides: <Type, Generator>{
          ProcessManager: () => processManager,
          Platform: () => fakePlatform,
        },
      );

      testUsingContext(
        "Doesn't throw on known tag, beta branch, no force",
        () async {
          final flutterVersion = FakeFlutterVersion(branch: 'beta');
          fakeCommandRunner.remoteVersion = FakeFlutterVersion(frameworkRevision: '1234');

          final Future<FlutterCommandResult> result = fakeCommandRunner.runCommand(
            const UpgradePhase.firstHalf(),
            force: true,
            testFlow: false,
            gitTagVersion: gitTagVersion,
            flutterVersion: flutterVersion,
            verifyOnly: false,
          );
          expect(await result, FlutterCommandResult.success());
          expect(processManager, hasNoRemainingExpectations);
        },
        overrides: <Type, Generator>{
          ProcessManager: () => processManager,
          Platform: () => fakePlatform,
        },
      );

      group('full command', () {
        late FakeProcessManager fakeProcessManager;
        late Directory tempDir;
        late File flutterToolState;
        late FileSystem fs;

        setUp(() {
          Cache.disableLocking();
          fakeProcessManager = FakeProcessManager.list(<FakeCommand>[
            const FakeCommand(command: <String>['git', 'tag', '--points-at', 'HEAD']),
            const FakeCommand(
              command: <String>['git', 'describe', '--match', '*.*.*', '--long', '--tags', 'HEAD'],
              stdout: 'v1.12.16-19-gb45b676af',
            ),
          ]);
          fs = MemoryFileSystem.test();
          tempDir = fs.systemTempDirectory.createTempSync('flutter_upgrade_test.');
          flutterToolState = tempDir.childFile('.flutter_tool_state');
        });

        tearDown(() {
          Cache.enableLocking();
          tryToDelete(tempDir);
        });

        testUsingContext(
          'upgrade continue prints welcome message',
          () async {
            fakeProcessManager = FakeProcessManager.any();
            final upgradeCommand = UpgradeCommand(
              verboseHelp: false,
              commandRunner: fakeCommandRunner,
            );

            await createTestCommandRunner(upgradeCommand).run(<String>[
              'upgrade',
              '--continue',
              '--continue-started-at',
              DateTime.now().toIso8601String(),
            ]);

            expect(
              json.decode(flutterToolState.readAsStringSync()),
              containsPair('redisplay-welcome-message', true),
            );
          },
          overrides: <Type, Generator>{
            FileSystem: () => fs,
            FlutterVersion: () => FakeFlutterVersion(),
            ProcessManager: () => fakeProcessManager,
            PersistentToolState: () =>
                PersistentToolState.test(directory: tempDir, logger: testLogger),
          },
          skip: true, // OHOS not supported
        );
      });
    });
  });
  group('parseStableTag', () {
    test('parses a stable tag with the -ohos- spelling', () {
      expect(OhosUpgrade.parseStableTag('3.35.8-ohos-1.0.4'), <int>[3, 35, 0, 8, 1, 0, 4, 1]);
    });

    test('parses a stable tag with the +ohos- spelling', () {
      expect(OhosUpgrade.parseStableTag('3.44.9+ohos-1.0.0'), <int>[3, 44, 1, 9, 1, 0, 0, 1]);
      expect(OhosUpgrade.parseStableTag('3.41.9+ohos-1.0.2'), <int>[3, 41, 1, 9, 1, 0, 2, 1]);
    });

    test('rejects pre-release suffixes', () {
      expect(OhosUpgrade.parseStableTag('3.35.8-ohos-1.0.4-beta'), isNull);
      expect(OhosUpgrade.parseStableTag('3.35.8-ohos-1.0.2-debug'), isNull);
      expect(OhosUpgrade.parseStableTag('3.44.9+ohos-0.0.1-canary1'), isNull);
    });

    test('rejects tags without an ohos version segment', () {
      expect(OhosUpgrade.parseStableTag('3.7.12-ohos'), isNull);
      expect(OhosUpgrade.parseStableTag('3.35.8'), isNull);
      expect(OhosUpgrade.parseStableTag('github.com/flutter/flutter/3.22.0'), isNull);
    });
  });

  group('parseLooseVersion', () {
    test('parses a pre-release version ignoring its suffix', () {
      expect(OhosUpgrade.parseLooseVersion('3.44.9+ohos-0.0.1-canary1'), <int>[
        3,
        44,
        1,
        9,
        0,
        0,
        1,
        0,
      ]);
      expect(OhosUpgrade.parseLooseVersion('3.35.8-ohos-1.0.4-beta'), <int>[
        3,
        35,
        0,
        8,
        1,
        0,
        4,
        0,
      ]);
    });

    test('returns null for a version without an ohos segment', () {
      expect(OhosUpgrade.parseLooseVersion('3.47.2'), isNull);
      expect(OhosUpgrade.parseLooseVersion('3.35.8'), isNull);
      expect(OhosUpgrade.parseLooseVersion('3.35.8-ohos-1.0.4'), <int>[3, 35, 0, 8, 1, 0, 4, 1]);
    });

    test('returns null for empty or unrecognized input', () {
      expect(OhosUpgrade.parseLooseVersion(''), isNull);
      expect(OhosUpgrade.parseLooseVersion('unknown'), isNull);
    });

    test('returns null for the unknown framework version', () {
      expect(OhosUpgrade.parseLooseVersion('0.0.0-unknown'), isNull);
    });
  });

  group('compareVersions', () {
    test('compares flutter segments before ohos segments', () {
      expect(
        OhosUpgrade.compareVersions(
          <int>[3, 41, 0, 10, 1, 0, 1, 1],
          <int>[3, 44, 1, 9, 0, 0, 1, 0],
        ),
        isNegative,
      );
      expect(
        OhosUpgrade.compareVersions(<int>[3, 35, 0, 8, 2, 0, 0, 1], <int>[3, 35, 0, 8, 1, 9, 9, 1]),
        isPositive,
      );
      expect(
        OhosUpgrade.compareVersions(<int>[3, 35, 0, 8, 1, 0, 4, 1], <int>[3, 35, 0, 8, 1, 0, 4, 1]),
        isZero,
      );
      // A stable release outranks its own pre-releases.
      expect(
        OhosUpgrade.compareVersions(<int>[3, 44, 1, 9, 0, 0, 1, 1], <int>[3, 44, 1, 9, 0, 0, 1, 0]),
        isPositive,
      );
    });

    test('prefers the current +ohos naming over the legacy -ohos one', () {
      // 3.41.9+ohos-1.0.2 (current naming) outranks 3.41.10-ohos-1.0.1
      // (legacy naming) at the same major.minor, even though the legacy
      // flutter patch number is larger.
      expect(
        OhosUpgrade.compareVersions(
          <int>[3, 41, 0, 10, 1, 0, 1, 1],
          <int>[3, 41, 1, 9, 1, 0, 2, 1],
        ),
        isNegative,
      );
      expect(
        OhosUpgrade.compareVersions(<int>[3, 22, 0, 4, 1, 1, 5, 1], <int>[3, 22, 1, 3, 1, 1, 6, 1]),
        isNegative,
      );
      // Different major.minor lines still compare by the flutter version.
      expect(
        OhosUpgrade.compareVersions(
          <int>[3, 41, 0, 10, 1, 0, 1, 1],
          <int>[3, 44, 1, 9, 0, 0, 1, 1],
        ),
        isNegative,
      );
    });

    test('still compares the flutter patch within the same naming', () {
      expect(
        OhosUpgrade.compareVersions(<int>[3, 41, 1, 9, 1, 0, 2, 1], <int>[3, 41, 1, 9, 1, 0, 3, 1]),
        isNegative,
      );
      expect(
        OhosUpgrade.compareVersions(<int>[3, 35, 0, 7, 1, 0, 5, 1], <int>[3, 35, 0, 8, 1, 0, 4, 1]),
        isNegative,
      );
    });
  });

  group('fetchLatestStableTag', () {
    late FakeProcessManager processManager;

    setUp(() {
      processManager = FakeProcessManager.empty();
    });

    testUsingContext(
      'returns the newest stable tag, preferring the peeled revision',
      () async {
        processManager.addCommands(<FakeCommand>[
          const FakeCommand(
            command: <String>['git', 'ls-remote', '--tags', 'origin'],
            stdout:
                'aaa100\trefs/tags/3.35.8-ohos-1.0.4\n'
                'aaa101\trefs/tags/3.35.8-ohos-1.0.4^{}\n'
                'bbb200\trefs/tags/3.41.10-ohos-1.0.0\n'
                'bbb201\trefs/tags/3.41.10-ohos-1.0.0^{}\n'
                'ccc300\trefs/tags/3.41.10-ohos-1.0.1\n'
                'ccc301\trefs/tags/3.41.10-ohos-1.0.1^{}\n'
                'ddd400\trefs/tags/3.44.9+ohos-0.0.1-canary1\n'
                'eee500\trefs/tags/github.com/flutter/flutter/3.22.0\n',
          ),
        ]);

        final ({String tag, String revision}) latest = await OhosUpgrade().fetchLatestStableTag(
          workingDirectory: '/flutter',
        );

        expect(latest.tag, '3.41.10-ohos-1.0.1');
        expect(latest.revision, 'ccc301');
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        ProcessManager: () => processManager,
        Platform: () => FakePlatform(),
      },
    );

    testUsingContext(
      'prefers the +ohos naming over the -ohos one when both are stable',
      () async {
        processManager.addCommands(<FakeCommand>[
          const FakeCommand(
            command: <String>['git', 'ls-remote', '--tags', 'origin'],
            stdout:
                'ccc300\trefs/tags/3.41.10-ohos-1.0.1\n'
                'ccc301\trefs/tags/3.41.10-ohos-1.0.1^{}\n'
                'fff600\trefs/tags/3.41.9+ohos-1.0.2\n'
                'fff601\trefs/tags/3.41.9+ohos-1.0.2^{}\n',
          ),
        ]);

        final ({String tag, String revision}) latest = await OhosUpgrade().fetchLatestStableTag(
          workingDirectory: '/flutter',
        );

        expect(latest.tag, '3.41.9+ohos-1.0.2');
        expect(latest.revision, 'fff601');
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        ProcessManager: () => processManager,
        Platform: () => FakePlatform(),
      },
    );

    testUsingContext(
      'throws tool exit when the remote cannot be queried',
      () async {
        processManager.addCommands(<FakeCommand>[
          const FakeCommand(
            command: <String>['git', 'ls-remote', '--tags', 'origin'],
            exception: ProcessException('git', <String>[
              'ls-remote',
              '--tags',
              'origin',
            ], 'fatal: unable to access'),
          ),
        ]);

        await expectLater(
          () => OhosUpgrade().fetchLatestStableTag(workingDirectory: '/flutter'),
          throwsToolExit(),
        );
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        ProcessManager: () => processManager,
        Platform: () => FakePlatform(),
      },
    );

    testUsingContext(
      'throws tool exit when the remote has no stable tag',
      () async {
        processManager.addCommands(<FakeCommand>[
          const FakeCommand(command: <String>['git', 'ls-remote', '--tags', 'origin']),
        ]);

        await expectLater(
          () => OhosUpgrade().fetchLatestStableTag(workingDirectory: '/flutter'),
          throwsToolExit(
            message: 'Unable to upgrade Flutter: no OHOS stable release tag found on the remote.',
          ),
        );
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        ProcessManager: () => processManager,
        Platform: () => FakePlatform(),
      },
    );
  });

  group('currentVersion', () {
    late FakeProcessManager processManager;

    setUp(() {
      processManager = FakeProcessManager.empty();
    });

    testUsingContext(
      'uses the OHOS release tag pointing at HEAD',
      () async {
        processManager.addCommands(<FakeCommand>[
          const FakeCommand(
            command: <String>['git', 'tag', '--points-at', 'HEAD'],
            stdout: '3.35.8-ohos-1.0.4\n',
          ),
        ]);

        final List<int>? version = await OhosUpgrade().currentVersion(
          workingDirectory: '/flutter',
          localVersion: FakeFlutterVersion(),
        );

        expect(version, <int>[3, 35, 0, 8, 1, 0, 4, 1]);
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        ProcessManager: () => processManager,
        Platform: () => FakePlatform(),
      },
    );

    testUsingContext(
      'falls back to the cached framework version when HEAD has no tag',
      () async {
        processManager.addCommands(<FakeCommand>[
          const FakeCommand(command: <String>['git', 'tag', '--points-at', 'HEAD']),
        ]);

        final List<int>? version = await OhosUpgrade().currentVersion(
          workingDirectory: '/flutter',
          localVersion: FakeFlutterVersion(frameworkVersion: '3.41.10-ohos-1.0.1'),
        );

        expect(version, <int>[3, 41, 0, 10, 1, 0, 1, 1]);
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        ProcessManager: () => processManager,
        Platform: () => FakePlatform(),
      },
    );
  });

  group('fetchLatestVersion', () {
    late FakeProcessManager processManager;

    setUp(() {
      processManager = FakeProcessManager.empty();
    });

    testUsingContext(
      'returns the local version without fetching when the target is not newer',
      () async {
        // The latest stable tag on the remote is 3.41.10-ohos-1.0.1 while the
        // current checkout is a newer pre-release (3.44.9+ohos-0.0.1-canary1);
        // the SDK must not downgrade and must not fetch the older tag.
        processManager.addCommands(<FakeCommand>[
          const FakeCommand(
            command: <String>['git', 'remote', 'get-url', 'origin'],
            stdout: 'https://gitcode.com/CPF-Flutter/flutter_flutter.git',
          ),
          const FakeCommand(
            command: <String>['git', 'ls-remote', '--tags', 'origin'],
            stdout:
                'ccc300\trefs/tags/3.41.10-ohos-1.0.1\n'
                'ccc301\trefs/tags/3.41.10-ohos-1.0.1^{}\n',
          ),
          const FakeCommand(
            command: <String>['git', 'tag', '--points-at', 'HEAD'],
            stdout: '3.44.9+ohos-0.0.1-canary1\n',
          ),
        ]);
        final localVersion = FakeFlutterVersion(frameworkVersion: '3.44.9+ohos-0.0.1-canary1');

        final FlutterVersion version = await OhosUpgrade().fetchLatestVersion(
          workingDirectory: '/flutter',
          localVersion: localVersion,
        );

        expect(version, same(localVersion));
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        ProcessManager: () => processManager,
        Platform: () => FakePlatform(),
      },
    );

    testUsingContext(
      'upgrades a pre-release checkout to the stable tag with the same version',
      () async {
        // The current checkout is 3.44.9+ohos-0.0.1-canary1 while the newest
        // stable tag is its own promotion, 3.44.9+ohos-0.0.1; the stable tag
        // must outrank the pre-release instead of reporting up to date.
        const revision = 'bbb4567abc';
        processManager.addCommands(<FakeCommand>[
          const FakeCommand(
            command: <String>['git', 'remote', 'get-url', 'origin'],
            stdout: 'https://gitcode.com/CPF-Flutter/flutter_flutter.git',
          ),
          const FakeCommand(
            command: <String>['git', 'ls-remote', '--tags', 'origin'],
            stdout: 'bbb4567abc\trefs/tags/3.44.9+ohos-0.0.1\n',
          ),
          const FakeCommand(
            command: <String>['git', 'tag', '--points-at', 'HEAD'],
            stdout: '3.44.9+ohos-0.0.1-canary1\n',
          ),
          const FakeCommand(
            command: <String>['git', 'fetch', 'origin', 'tag', '3.44.9+ohos-0.0.1', '--no-tags'],
          ),
          const FakeCommand(
            command: <String>['git', 'tag', '--points-at', revision],
            stdout: '3.44.9+ohos-0.0.1\n',
          ),
        ]);
        final localVersion = FakeFlutterVersion(frameworkVersion: '3.44.9+ohos-0.0.1-canary1');

        final FlutterVersion version = await OhosUpgrade().fetchLatestVersion(
          workingDirectory: '/flutter',
          localVersion: localVersion,
        );

        expect(version.frameworkRevision, revision);
        expect(version.frameworkVersion, '3.44.9+ohos-0.0.1');
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        ProcessManager: () => processManager,
        Platform: () => FakePlatform(),
      },
    );

    testUsingContext(
      'fetchLatestVersion fetches the target tag and returns the new version',
      () async {
        const revision = 'adaf911c35';
        processManager.addCommands(<FakeCommand>[
          const FakeCommand(
            command: <String>['git', 'remote', 'get-url', 'origin'],
            stdout: 'https://gitcode.com/CPF-Flutter/flutter_flutter.git',
          ),
          const FakeCommand(
            command: <String>['git', 'ls-remote', '--tags', 'origin'],
            stdout:
                'tagobj01\trefs/tags/3.41.10-ohos-1.0.1\n'
                'adaf911c35\trefs/tags/3.41.10-ohos-1.0.1^{}\n',
          ),
          const FakeCommand(
            command: <String>['git', 'tag', '--points-at', 'HEAD'],
            stdout: '3.35.8-ohos-1.0.4\n',
          ),
          const FakeCommand(
            command: <String>['git', 'fetch', 'origin', 'tag', '3.41.10-ohos-1.0.1', '--no-tags'],
          ),
          const FakeCommand(
            command: <String>['git', 'tag', '--points-at', revision],
            stdout: '3.41.10-ohos-1.0.1\n',
          ),
        ]);
        final upgrade = OhosUpgrade();
        final localVersion = FakeFlutterVersion(frameworkVersion: '3.35.8-ohos-1.0.4');

        final FlutterVersion version = await upgrade.fetchLatestVersion(
          workingDirectory: '/flutter',
          localVersion: localVersion,
        );

        expect(version.frameworkRevision, revision);
        expect(version.frameworkVersion, '3.41.10-ohos-1.0.1');
        expect(upgrade.latestTag, '3.41.10-ohos-1.0.1');
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        ProcessManager: () => processManager,
        Platform: () => FakePlatform(),
      },
    );

    testUsingContext(
      'throws tool exit when the target tag cannot be fetched',
      () async {
        processManager.addCommands(<FakeCommand>[
          const FakeCommand(
            command: <String>['git', 'remote', 'get-url', 'origin'],
            stdout: 'https://gitcode.com/CPF-Flutter/flutter_flutter.git',
          ),
          const FakeCommand(
            command: <String>['git', 'ls-remote', '--tags', 'origin'],
            stdout: 'adaf911c35\trefs/tags/3.41.10-ohos-1.0.1\n',
          ),
          const FakeCommand(
            command: <String>['git', 'tag', '--points-at', 'HEAD'],
            stdout: '3.35.8-ohos-1.0.4\n',
          ),
          const FakeCommand(
            command: <String>['git', 'fetch', 'origin', 'tag', '3.41.10-ohos-1.0.1', '--no-tags'],
            exception: ProcessException('git', <String>[
              'fetch',
              'origin',
              'tag',
              '3.41.10-ohos-1.0.1',
            ], 'fatal: unable to access'),
          ),
        ]);
        final localVersion = FakeFlutterVersion(frameworkVersion: '3.35.8-ohos-1.0.4');

        await expectLater(
          () => OhosUpgrade().fetchLatestVersion(
            workingDirectory: '/flutter',
            localVersion: localVersion,
          ),
          throwsToolExit(),
        );
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        ProcessManager: () => processManager,
        Platform: () => FakePlatform(),
      },
    );

    testUsingContext(
      'throws tool exit when the origin remote is not a standard OHOS remote',
      () async {
        processManager.addCommand(
          const FakeCommand(
            command: <String>['git', 'remote', 'get-url', 'origin'],
            stdout: 'https://example.com/flutter.git',
          ),
        );

        await expectLater(
          () => OhosUpgrade().fetchLatestVersion(
            workingDirectory: '/flutter',
            localVersion: FakeFlutterVersion(frameworkVersion: '3.35.8-ohos-1.0.4'),
          ),
          throwsToolExit(),
        );
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        ProcessManager: () => processManager,
        Platform: () => FakePlatform(),
      },
    );

    final logger = BufferLogger.test();

    testUsingContext(
      'flutterUpgradeContinue prints guidance when the follow-up fails',
      () async {
        processManager.addCommand(
          FakeCommand(
            command: <String>[
              globals.fs.path.join('bin', 'flutter'),
              'upgrade',
              '--continue',
              '--continue-started-at',
              '2026-01-01T00:00:00.000Z',
              '--no-version-check',
            ],
            exitCode: 1,
          ),
        );
        await (UpgradeCommandRunner()..workingDirectory = '/flutter').flutterUpgradeContinue(
          startedAt: DateTime.utc(2026),
        );

        expect(logger.statusText, contains('Run "flutter precache"'));
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        ProcessManager: () => processManager,
        Platform: () => FakePlatform(),
        Logger: () => logger,
      },
    );
  });
}

class FakeUpgradeCommandRunner extends UpgradeCommandRunner {
  bool willHaveUncommittedChanges = false;
  bool alreadyUpToDate = false;

  late FlutterVersion remoteVersion;

  @override
  Future<FlutterVersion> fetchLatestVersion({FlutterVersion? localVersion}) async => remoteVersion;

  @override
  Future<bool> hasUncommittedChanges() async => willHaveUncommittedChanges;

  @override
  Future<void> attemptReset(String newRevision) async {}

  @override
  Future<void> updatePackages(FlutterVersion flutterVersion) async {}

  @override
  Future<void> runDoctor() async {}
}
