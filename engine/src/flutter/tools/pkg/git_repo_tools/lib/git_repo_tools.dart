// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:io' as io show Directory, File, Platform, stdout;

import 'package:path/path.dart' as path;
import 'package:process/process.dart';
import 'package:process_runner/process_runner.dart';

/// The default base branch to diff against when computing changed files.
///
/// OHOS fork uses 'oh-3.44.9-dev' as the main branch instead of 'main'.
/// In CI the [kTargetBranchEnv] environment variable (set by the runner from
/// `.ci_ohos.yaml`'s `target_branch`) takes precedence so the branch name is
/// defined in a single place; local development falls back to this default.
const String kDefaultBaseBranch = 'oh-3.44.9-dev';

/// Environment variable that overrides [kDefaultBaseBranch] in CI.
const String kTargetBranchEnv = 'TARGET_BRANCH';

/// Utility methods for working with a git repository.
final class GitRepo {
  /// The git repository rooted at `root`.
  GitRepo.fromRoot(
    this.root, {
    this.verbose = false,
    StringSink? logSink,
    ProcessManager processManager = const LocalProcessManager(),
  }) : _processManager = processManager,
       logSink = logSink ?? io.stdout;

  /// Whether to produce verbose log output.
  ///
  /// If true, output of git commands will be printed to [logSink].
  final bool verbose;

  /// Where to send verbose log output.
  ///
  /// Defaults to [io.stdout].
  final StringSink logSink;

  /// The root of the git repo.
  final io.Directory root;

  /// The delegate to use for running processes.
  final ProcessManager _processManager;

  /// Returns a list of all non-deleted files which differ from the nearest
  /// merge-base with `main`. If it can't find a fork point, uses the default
  /// merge-base.
  ///
  /// This is only computed once and cached. Subsequent invocations of the
  /// getter will return the same result.
  late final Future<List<io.File>> changedFiles = _changedFiles();

  /// Returns the SHA of the current HEAD commit.
  Future<String> headSha({bool short = false}) async {
    final ProcessRunnerResult result = await ProcessRunner(
      defaultWorkingDirectory: root,
      processManager: _processManager,
    ).runProcess(<String>['git', 'rev-parse', if (short) '--short', 'HEAD']);
    return result.stdout.trim();
  }

  Future<List<io.File>> _changedFiles() async {
    final processRunner = ProcessRunner(
      defaultWorkingDirectory: root,
      processManager: _processManager,
    );
    await _fetch(processRunner);
    // Find the merge base between the current branch and the branch that was
    // checked out at the time of the last fetch. The merge base is the common
    // ancestor of the two branches, and the output is the hash of the merge
    // base.
    ProcessRunnerResult mergeBaseResult = await processRunner.runProcess(<String>[
      'git',
      'merge-base',
      '--fork-point',
      'FETCH_HEAD',
      'HEAD',
    ], failOk: true);
    if (mergeBaseResult.exitCode != 0) {
      if (verbose) {
        logSink.writeln('git merge-base --fork-point failed, using default merge-base');
        logSink.writeln('Output:\n${mergeBaseResult.stdout}');
      }
      mergeBaseResult = await processRunner.runProcess(<String>[
        'git',
        'merge-base',
        'FETCH_HEAD',
        'HEAD',
      ]);
    }
    final String mergeBase = mergeBaseResult.stdout.trim();
    final ProcessRunnerResult masterResult = await processRunner.runProcess(<String>[
      'git',
      'diff',
      '--name-only',
      '--diff-filter=ACMRT', // Added, copied, modified, renamed, or type-changed.
      '--relative', // _gitOutputToList will prepend the CWD so we don't want duplicate sub-paths
      mergeBase,
    ]);
    return _gitOutputToList(masterResult);
  }

  /// Returns a list of non-deleted files which differ between the HEAD
  /// commit and its parent.
  ///
  /// This is only computed once and cached. Subsequent invocations of the
  /// getter will return the same result.
  late final Future<List<io.File>> changedFilesAtHead = _changedFilesAtHead();

  Future<List<io.File>> _changedFilesAtHead() async {
    final processRunner = ProcessRunner(
      defaultWorkingDirectory: root,
      processManager: _processManager,
    );
    await _fetch(processRunner);
    final ProcessRunnerResult diffTreeResult = await processRunner.runProcess(<String>[
      'git',
      'diff-tree',
      '--no-commit-id',
      '--name-only',
      '--diff-filter=ACMRT', // Added, copied, modified, renamed, or type-changed.
      '--relative', // _gitOutputToList will prepend the CWD so we don't want duplicate sub-paths
      '-r',
      'HEAD',
    ]);
    return _gitOutputToList(diffTreeResult);
  }

  Future<void> _fetch(ProcessRunner processRunner) async {
    final String baseBranch = io.Platform.environment[kTargetBranchEnv] ?? kDefaultBaseBranch;
    // Try remotes in priority order so that a misconfigured `upstream` (e.g.
    // pointing at the official Flutter repo, which lacks the OHOS base branch)
    // does not crash local tools: if a remote is not configured or the fetch
    // fails, the next candidate is attempted instead. The candidate list
    // mirrors `bin/format.dart`'s `_getDiffBaseRevision`.
    const candidates = <String>['upstream', 'gitcode', 'origin'];
    ProcessRunnerResult? lastFetchResult;
    String? lastRemote;
    for (final remote in candidates) {
      final ProcessRunnerResult remoteResult = await processRunner.runProcess(<String>[
        'git',
        'remote',
        'get-url',
        remote,
      ], failOk: true);
      if (remoteResult.exitCode != 0) {
        continue;
      }
      lastRemote = remote;
      lastFetchResult = await processRunner.runProcess(<String>[
        'git',
        'fetch',
        remote,
        baseBranch,
      ], failOk: true);
      if (lastFetchResult.exitCode == 0) {
        return;
      }
      if (verbose) {
        logSink.writeln('git fetch $remote $baseBranch failed:');
        logSink.writeln(lastFetchResult.stderr);
      }
    }
    throw ProcessRunnerException(
      'Failed to fetch $baseBranch from any configured remote '
      '(last tried: $lastRemote).',
      result: lastFetchResult,
    );
  }

  List<io.File> _gitOutputToList(ProcessRunnerResult result) {
    final String diffOutput = result.stdout.trim();
    if (verbose) {
      logSink.writeln('git diff output:\n$diffOutput');
    }
    final resultMap = <String>{};
    resultMap.addAll(diffOutput.split('\n').where((String str) => str.isNotEmpty));
    return resultMap
        .map<io.File>((String filePath) => io.File(path.join(root.path, filePath)))
        .toList();
  }
}
