# Flutter 3.47.9-ohos-0.0.1 Release Notes

> **版本状态**：canary1<br/>
> **Flutter 上游社区基线版本**：[![Flutter Version](https://img-transfer.gitcode.com?p=https%3A%2F%2Fimg.shields.io%2Fbadge%2FFlutter-3.47.9-blue.svg%3Flogo%3Dflutter&projectId=CPF-Flutter&pageUrl=https%3A%2F%2Fgitcode.com%2FCPF-Flutter)](https://github.com/flutter/flutter/commit/c197ca5199e9ee5661e865596c9c07294dfab51c)

---

## 版本概述

本版本为 Flutter OpenHarmony 平台 0.0.1（canary1）版本，基于 Flutter 3.47.9 版本适配。本版本支持和完善 OpenHarmony 平台侧能力，提供平台化 Channel、外接纹理、云端 SDK 等特性，并优化性能。

## 版本配套

| 配套 | 版本 / 要求 |
| --- | --- |
| **Flutter SDK** | [**3.47.9-ohos-0.0.1**](https://gitcode.com/CPF-Flutter/flutter_flutter/releases/tag/3.47.9+ohos-0.0.1-canary1) |
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

- 修复标准消息编解码器对负数 BigInt 的解码错误
- 修复 Windowing embedder 在非 Impeller Vulkan 后端下的崩溃问题（限定其仅运行于 Impeller Vulkan）
- 修复 `flutter build hap` 报错问题
- 修复 Linux 侧 engine 编译报错问题

## Changelog

- [CHANGELOG_OHOS.md](../CHANGELOG_OHOS.md)

## 资料文档

- [文档](https://gitcode.com/CPF-Flutter/flutter_samples/tree/master/ohos/docs)
