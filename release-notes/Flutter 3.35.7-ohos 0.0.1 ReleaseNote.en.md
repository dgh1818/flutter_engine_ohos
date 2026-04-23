## Version Overview
This version is the Flutter OpenHarmony platform version 0.0.1 (canary1), based on the Flutter 3.35.7. This version supports and improves the capabilities of the OpenHarmony platform, provides platform-based channels, external texture, cloud SDKs and other features, and optimizes performance.

## Basic Features
- Support OpenHarmony platform Flutter Channel
- Support OpenHarmony platform Flutter Engine
- Support OpenHarmony platform Flutter Command line tool
- Support OpenHarmony platform External Texture
- Support OpenHarmony platform SDK

## Bugfix
- Fix Rect.fromLTRB display issue
- Fixed the issue where 'flutter clean' on Windows could not correctly delete the build and oh_modules directories
- Fix the path issue for gen_snapshot in Flutter build on Mac ARM64
- Fix the Dart breakpoint mode app freeze issue
- Fix the issue in version 3.35 where the Testfield keyboard fails to retract
- Fix the issue where pasting is not possible after cutting
- Fix the issue with the ohos platform's "build" command, which fails to create the specified folder.
- Fixed an issue where certain breakpoints could not be hit in Android Studio.

## Version Compatibility
- Compilation engine version requirements: OpenHarmony API 23 or higher
- Flutter SDK: 3.35.7-ohos-0.0.1 (Due to Flutter version parsing rules, to avoid version comparison failures, it will display as 3.35.8-ohos-0.0.1-canary1)

## Changelog
- [3.35.7-ohos-0.0.1](../CHANGELOG_OHOS.md)

## Enabling Documentation
- [Documentation Link](https://gitcode.com/openharmony-tpc/flutter_samples/tree/master/ohos/docs)
