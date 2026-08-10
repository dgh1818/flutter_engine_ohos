# Flutter 3.44.9-ohos-0.0.1 Release Notes

> **版本状态**：canary1<br/>
> **Flutter 上游社区基线版本**：[![Flutter Version](https://img-transfer.gitcode.com?p=https%3A%2F%2Fimg.shields.io%2Fbadge%2FFlutter-3.44.9-blue.svg%3Flogo%3Dflutter&projectId=CPF-Flutter&pageUrl=https%3A%2F%2Fgitcode.com%2FCPF-Flutter)](https://github.com/flutter/flutter/commit/6b182d2c7585eba26d4edce0f97630effd256c33)

---

## 版本概述

本版本为 Flutter OpenHarmony 平台 0.0.1（canary1）版本，基于 Flutter 3.44.9 版本适配。本版本支持和完善 OpenHarmony 平台侧能力，提供平台化 Channel、外接纹理、云端 SDK 等特性，并优化性能。

## 版本配套

| 配套 | 版本 / 要求 |
| --- | --- |
| **Flutter SDK** | [**3.44.9-ohos-0.0.1**](https://gitcode.com/CPF-Flutter/flutter_flutter/releases/tag/3.44.10-ohos-0.0.1-canary1)<br/>*（由于 Flutter 版本解析规则，为避免版本比较解析失败，实际显示为 `3.44.10-ohos-0.0.1-canary1`）* |
| **DevEco Studio** | **DevEco Studio 26.0.0 Beta2**<br/>`Build Version：26.0.0.621` |
| **Command Line Tools** | **Command Line Tools 26.0.0 Beta2**<br/>`Build Version：26.0.0.621` |
| **引擎构建最低要求 API** | **OpenHarmony API 26.0.0** |
| **应用目标 API** | **OpenHarmony API 26.0.0** |
| **应用最低运行 API** | **OpenHarmony API 26.0.0** |

## 主要变更

### 新增

- 支持 OpenHarmony 平台 Flutter Channel
- 支持 OpenHarmony 平台 Flutter Engine
- 支持 OpenHarmony 平台 Flutter 命令行工具
- 支持外接纹理
- 支持云端 SDK

### 修复

- 解决 Mac 编译 engine 时 metal 侧报错问题
- 修复 OHOS 侧 `flutter --build-dir=build3` 无法生成 build3 目录问题
- 解决 flutter 编译执行其他平台产物 crash 的问题

## Changelog

- [CHANGELOG_OHOS.md](../CHANGELOG_OHOS.md)

## 资料文档

- [文档](https://gitcode.com/CPF-Flutter/flutter_samples/tree/master/ohos/docs)
