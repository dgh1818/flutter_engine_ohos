## 版本概述
本版本为Flutter OpenHarmony平台0.0.1版本（canary1），基于Flutter 3.44.8版本适配。本版本支持和完善OpenHarmony平台侧能力，提供平台化Channel、外接纹理、云端SDK等特性，并优化性能。

## 基础特性
- 支持OpenHarmony平台Flutter Channel
- 支持OpenHarmony平台Flutter Engine
- 支持OpenHarmony平台Flutter命令行工具
- 支持外接纹理
- 支持云端SDK

## Bug修复
- 解决 Mac 编译 engine 时 metal 侧报错问题
- 修复 OHOS 侧 `flutter --build-dir=build3` 无法生成 build3 目录问题

## 版本配套
- 引擎构建最低要求 API：**OpenHarmony API 26**
- 应用构建推荐适配 API：**OpenHarmony API 23**
- 应用构建最低适配 API：**OpenHarmony API 20**
- 应用最低运行 API：**OpenHarmony API 12**
- Flutter SDK：**3.44.8-ohos-0.0.1**（由于flutter版本解析规则，为了避免版本比较解析失败，将显示为3.44.9-ohos-0.0.1-canary1）

## Changelog
- [3.44.8-ohos-0.0.1](../CHANGELOG_OHOS.md)

## 赋能文档
- [文档链接](https://gitcode.com/openharmony-tpc/flutter_samples/tree/master/ohos/docs)
