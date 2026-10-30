// Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE_HW file.

import '../base/common.dart';
import '../base/io.dart';
import '../base/process.dart';
import '../globals.dart' as globals;
import '../version.dart';

/// Resolves the `flutter upgrade` target for the OHOS SDK fork.
///
/// The OHOS SDK does not track an upstream channel branch. Stable releases
/// are published as tags on the 'origin' remote, formatted
/// `<flutter version>-ohos-<ohos version>` (newer lines use `+ohos-`); tags
/// carrying a pre-release suffix (for example '-beta' or '-canary1') are
/// not upgrade targets.
class OhosUpgrade {
  /// The latest OHOS stable release tag discovered by [fetchLatestVersion].
  String? latestTag;

  /// Resolves the latest OHOS stable release as the upgrade target.
  ///
  /// Exits the tool if the remote cannot be queried, has no stable tag, or is
  /// not a standard OHOS remote.
  ///
  /// If the newest stable tag is not newer than the current checkout (for
  /// example on a dev branch ahead of the latest release), the local version
  /// is returned unchanged so the upstream revision comparison in
  /// `UpgradeCommandRunner` reports "already up to date" and the SDK never
  /// downgrades.
  Future<FlutterVersion> fetchLatestVersion({
    required String? workingDirectory,
    required FlutterVersion localVersion,
  }) async {
    // Check whether the 'origin' remote is a standard OHOS remote. The OHOS
    // discovery below talks to 'origin' directly, so that is the remote to
    // validate — unlike the upstream validator, which resolves the tracked
    // upstream and rejects detached checkouts (for example a clone of a
    // release tag).
    final String? originUrl = await _remoteUrl(
      remote: 'origin',
      workingDirectory: workingDirectory,
    );
    final String? flutterGit = globals.platform.environment['FLUTTER_GIT_URL'];
    final List<String> allowedRemotes = <String>[
      if (flutterGit != null) flutterGit else ...VersionUpstreamValidator.standardRemotes,
    ].map(_stripDotGit).toList();
    if (originUrl == null || !allowedRemotes.contains(_stripDotGit(originUrl))) {
      throwToolExit(
        'Unable to upgrade Flutter: the "origin" remote'
        '${originUrl == null ? '' : ' ($originUrl)'} is not a standard OHOS remote.\n'
        'Point "origin" at a standard OHOS remote, or set "FLUTTER_GIT_URL" to '
        'the current remote.',
      );
    }
    final ({String tag, String revision}) latest = await fetchLatestStableTag(
      workingDirectory: workingDirectory,
    );
    final List<int>? current = await currentVersion(
      workingDirectory: workingDirectory,
      localVersion: localVersion,
    );
    if (current != null && compareVersions(parseStableTag(latest.tag)!, current) <= 0) {
      latestTag = latest.tag;
      return localVersion;
    }
    try {
      // Fetch only the target tag; a full 'git fetch --tags' would pull every
      // release tag of all OHOS version lines.
      await globals.git.run(
        ['fetch', 'origin', 'tag', latest.tag, '--no-tags'],
        throwOnError: true,
        workingDirectory: workingDirectory,
      );
    } on ProcessException catch (e) {
      throwToolExit(
        'Unable to upgrade Flutter: failed to fetch tag "${latest.tag}".\n'
        'Error: $e.',
      );
    }
    latestTag = latest.tag;
    return FlutterVersion.fromRevision(
      flutterRoot: workingDirectory!,
      frameworkRevision: latest.revision,
      fs: globals.fs,
      git: globals.git,
    );
  }

  /// Returns the URL of [remote], or null when it cannot be resolved.
  Future<String?> _remoteUrl({required String remote, required String? workingDirectory}) async {
    try {
      final RunResult result = await globals.git.run(
        <String>['remote', 'get-url', remote],
        throwOnError: true,
        workingDirectory: workingDirectory,
      );
      final String url = result.stdout.trim();
      return url.isEmpty ? null : url;
    } on ProcessException {
      return null;
    }
  }

  static String _stripDotGit(String url) =>
      url.endsWith('.git') ? url.substring(0, url.length - 4) : url;

  /// OHOS stable release tags: `<flutter version>-ohos-<ohos version>` or
  /// `<flutter version>+ohos-<ohos version>`. Tags with a suffix (for example
  /// '-beta' or '-canary1') do not match and are excluded.
  static final _stableTagPattern = RegExp(r'^(\d+)\.(\d+)\.(\d+)([+-])ohos-(\d+)\.(\d+)\.(\d+)$');

  /// Loose variant of the release tag pattern: matches the version prefix
  /// and ignores any suffix, so pre-release versions (for example
  /// '3.44.9+ohos-0.0.1-canary1') still resolve for comparison purposes.
  /// An ohos segment is always present on OHOS checkouts.
  static final _looseVersionPattern = RegExp(r'^(\d+)\.(\d+)\.(\d+)([+-])ohos-(\d+)\.(\d+)\.(\d+)');

  /// Parses an OHOS stable release tag into eight numeric segments
  /// [flutterMajor, flutterMinor, naming, flutterPatch, ohosMajor, ohosMinor,
  /// ohosPatch, release]. The third segment ranks the separator, so at the
  /// same major.minor the '+' naming ('3.41.9+ohos-x') outranks the legacy
  /// '-' one ('3.41.10-ohos-x'). The last segment is the release rank: 1
  /// for a stable release and 0 for a pre-release, so a stable tag outranks
  /// its own pre-releases. Returns null if [tag] is not a stable release
  /// tag.
  static List<int>? parseStableTag(String tag) {
    final RegExpMatch? match = _stableTagPattern.firstMatch(tag.trim());
    if (match == null) {
      return null;
    }
    return <int>[
      int.parse(match.group(1)!),
      int.parse(match.group(2)!),
      if (match.group(4) == '+') 1 else 0,
      int.parse(match.group(3)!),
      int.parse(match.group(5)!),
      int.parse(match.group(6)!),
      int.parse(match.group(7)!),
      1,
    ];
  }

  /// Parses a version string of the current checkout into eight numeric
  /// segments (see [parseStableTag] for the segment order and ranks). A
  /// pre-release suffix is ignored for the version segments but sets the
  /// release rank (the last segment) to 0, so a stable release outranks its
  /// own pre-releases. Returns null if [version] does not start with an
  /// OHOS version (an ohos segment is required).
  static List<int>? parseLooseVersion(String version) {
    final String trimmed = version.trim();
    if (trimmed.isEmpty || trimmed == kUnknownFrameworkVersion) {
      return null;
    }
    final RegExpMatch? match = _looseVersionPattern.firstMatch(trimmed);
    if (match == null) {
      return null;
    }
    return <int>[
      int.parse(match.group(1)!),
      int.parse(match.group(2)!),
      if (match.group(4) == '+') 1 else 0,
      int.parse(match.group(3)!),
      int.parse(match.group(5)!),
      int.parse(match.group(6)!),
      int.parse(match.group(7)!),
      if (match.end == trimmed.length) 1 else 0,
    ];
  }

  /// Compares two eight-segment versions returned by [parseStableTag].
  /// Returns a negative number if [a] is older than [b], 0 if equal, and a
  /// positive number if newer. Segments compare left to right: at the same
  /// major.minor the '+' naming outranks the legacy '-', and once the
  /// version segments are equal a stable release outranks its pre-releases.
  static int compareVersions(List<int> a, List<int> b) {
    for (var i = 0; i < 8; i++) {
      if (a[i] != b[i]) {
        return a[i] - b[i];
      }
    }
    return 0;
  }

  /// The current SDK version as eight segments: from the OHOS release tag
  /// pointing at HEAD, or parsed from the cached framework version.
  Future<List<int>?> currentVersion({
    required String? workingDirectory,
    required FlutterVersion localVersion,
  }) async {
    try {
      final RunResult result = await globals.git.run(
        ['tag', '--points-at', 'HEAD'],
        throwOnError: true,
        workingDirectory: workingDirectory,
      );
      for (final String line in result.stdout.split('\n')) {
        final List<int>? version = parseLooseVersion(line);
        if (version != null) {
          return version;
        }
      }
    } on ProcessException {
      // Fall through to the cached framework version.
    }
    return parseLooseVersion(localVersion.frameworkVersion);
  }

  /// Returns the newest OHOS stable release tag on the 'origin' remote.
  ///
  /// Exits the tool if the remote cannot be queried or has no stable tag.
  Future<({String tag, String revision})> fetchLatestStableTag({
    required String? workingDirectory,
  }) async {
    final RunResult result;
    try {
      result = await globals.git.run(
        ['ls-remote', '--tags', 'origin'],
        throwOnError: true,
        workingDirectory: workingDirectory,
      );
    } on ProcessException catch (e) {
      throwToolExit(
        'Unable to upgrade Flutter: failed to query the remote repository.\n'
        'Error: $e.',
      );
    }
    final tags = <String, String>{};
    final peeledTags = <String, String>{};
    for (final String line in result.stdout.split('\n')) {
      final List<String> parts = line.trim().split('\t');
      if (parts.length != 2 || !parts[1].startsWith('refs/tags/')) {
        continue;
      }
      final String tagName = parts[1].substring('refs/tags/'.length);
      if (tagName.endsWith('^{}')) {
        // An annotated tag points to the commit via a tag object; prefer the
        // peeled revision when resetting.
        peeledTags[tagName.substring(0, tagName.length - 3)] = parts[0];
      } else {
        tags[tagName] = parts[0];
      }
    }
    String? latestTag;
    List<int>? latestVersion;
    for (final String tagName in tags.keys) {
      final List<int>? version = parseStableTag(tagName);
      if (version == null) {
        continue;
      }
      if (latestVersion == null || compareVersions(version, latestVersion) > 0) {
        latestTag = tagName;
        latestVersion = version;
      }
    }
    if (latestTag == null) {
      throwToolExit(
        'Unable to upgrade Flutter: no OHOS stable release tag found on the '
        'remote.',
      );
    }
    return (tag: latestTag, revision: peeledTags[latestTag] ?? tags[latestTag]!);
  }
}
