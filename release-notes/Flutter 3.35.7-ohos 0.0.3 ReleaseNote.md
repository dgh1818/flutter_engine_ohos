## 版本概述
本版本为Flutter OpenHarmony平台0.0.3版本，基于Flutter 3.35.7版本适配。本版本支持和完善OpenHarmony平台侧能力，提供平台化Channel、外接纹理、云端SDK等特性，并优化性能。

## 基础特性
- Frame gate enabled: keep draining producer queue, but do not schedule
- Click the status bar to automatically return to the top
- 毕昇编译器替换，开启优化选项
- SensitiveContentChannel适配
- [impeller] Vulkan backend supports skipping rendering when dirty region is 0.
- Add monitor for external textures visible area

## Bug修复
- Addressed the issue where cropping with original dimensions in a transformed coordinate system resulted in a size mismatch.
- 修复软键盘直接弹起到界面上问题
- fix: keyboard home key is not consistent
- Remove 'ohpm clean' during compilation process.
- fix: caplock and return keys are not working with keyboard
- Fixed: onInactive method was not triggered when the WebView became invisible.
- Fix the issue of keyboard popping up and flickering in PlatformView input box
- 修复使用multiply混合模式时，在某些GPU上画面变白/变灰的问题
- 修复monorepo flutter_audioplayers编译失败找不到.dart_tool/package_config.json问题
- Fix the issue where the clipboard cannot paste content in a custom format
- [Impeller] Fixed an issue where gradient effects on HarmonyOS devices exhibited clipping. With mediump enabled by default, `IPOrderedDither8x8 uint(dest.x)` and `uint(dest.y)` might experience precision loss on some GPU chips.
- 修复多PlatformView场景下输入框失焦问题

## 版本配套
- 编译引擎版本要求： OpenHarmony API 23及以上
- Flutter SDK: 3.35.7-ohos-0.0.3（由于flutter版本解析规则，为了避免版本比较解析失败，将显示为3.35.8-ohos-0.0.3）

## Changelog
- [3.35.7-ohos-0.0.3](../CHANGELOG_OHOS.md)

## 赋能文档
- [文档链接](https://gitcode.com/openharmony-tpc/flutter_samples/tree/master/ohos/docs)
