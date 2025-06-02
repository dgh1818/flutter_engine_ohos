## Version Overview
This version is an OpenHarmony version based on Flutter 3.22.0. This version supports and improves the capabilities of the OpenHarmony platform and improves stability.

## Release Scope
OpenHarmony API16

## BugFix
- Fix memory leak in NativeWindow when DetachFlutterEngine occurs before OnSurfaceDestroy
- Fix the deadlock on the production side of external textures in some scenarios
- Fix the parameter format issue of interface setTextureBackGroundColor, change from ARGB to ABGR
- Fix an issue where some scene components were not rendered due to the simple occlusion culling function of Impeller
- Fix the issue that when an external keyboard is connected, the text is deleted by pressing shift and arrow keys at the same time
- Fix the RangeError issue in the Google community for version 3.22

## Version Release Time
May 21, 2025

## Version Support
- ROM: 5.0.1.120
- IDE: DevEco Studio 5.0.13.100
- Flutter SDK: 3.22.0-ohos-1.0.1

## Changelog
- [5.1.0.403SP1](../CHANGELOG.md)

## Enablement Documents
- [Document Link](https://gitcode.com/openharmony-sig/flutter_samples/tree/master/ohos/docs)
