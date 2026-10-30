/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { hilog } from '@kit.PerformanceAnalysisKit';
import { common } from '@kit.AbilityKit';
import { promptAction } from '@kit.ArkUI';
import testNapi from 'libentry.so';

const DOMAIN = 0x0000;
const TAG = '[unittest_ohos]';

@Entry
@Component
struct Index {
  @State message: string = 'Flutter 引擎 UT 执行器';
  @State outputText: string = '启动后自动执行测试,也可点击下方按钮手动执行';
  @State exitCode: number = -1;
  @State isRunning: boolean = false;
  @State loadingDots: string = '';
  @State profrawPath: string = '';
  @State profrawSize: number = 0;
  @State gtestFilter: string = '';
  private loadingTimer: number = -1;
  private liveTimer: number = -1;
  private scroller: Scroller = new Scroller();

  build() {
    Column() {
      // 标题
      Text(this.message)
        .fontSize(22)
        .fontWeight(FontWeight.Bold)
        .margin({ top: 24, bottom: 16 })

      // gtest_filter 输入框
      TextInput({ text: this.gtestFilter, placeholder: 'gtest_filter' })
        .width('90%')
        .height(42)
        .fontSize(13)
        .fontFamily('monospace')
        .borderRadius(8)
        .margin({ bottom: 10 })
        .onChange((value: string) => {
          this.gtestFilter = value;
        })

      // 三个按钮一行
      Row() {
        // 执行测试
        Button(this.isRunning ? '测试中...' : '执行测试')
          .layoutWeight(1)
          .height(42)
          .fontSize(15)
          .enabled(!this.isRunning)
          .backgroundColor(this.isRunning ? '#999999' : '#007DFF')
          .borderRadius(8)
          .onClick(() => {
            this.runTestsSo();
          })

        // 测试结果状态徽章(结果展示,非按钮;失败详情看下方输出日志)
        Text(this.badgeText())
          .layoutWeight(1)
          .height(42)
          .fontSize(15)
          .textAlign(TextAlign.Center)
          .fontColor('#FFFFFF')
          .backgroundColor(this.badgeColor())
          .borderRadius(8)
          .margin({ left: 8 })

        // 覆盖率原始数据(profraw)
        Button(this.profrawSize > 0 ? '📊 覆盖率' : '无覆盖率')
          .layoutWeight(1)
          .height(42)
          .fontSize(15)
          .enabled(this.profrawSize > 0)
          .backgroundColor(this.profrawSize > 0 ? '#17a2b8' : '#CCCCCC')
          .borderRadius(8)
          .margin({ left: 8 })
          .onClick(() => {
            promptAction.showDialog({
              title: '覆盖率原始数据',
              message: `profraw 路径:\n${this.profrawPath}\n\n文件大小: ${this.profrawSize} bytes`,
              buttons: [{ text: '确定', color: '#007DFF' }],
            });
          })
      }
      .width('90%')
      .margin({ bottom: 8 })

      // Loading 动画
      if (this.isRunning) {
        Row() {
          LoadingProgress()
            .width(24)
            .height(24)
            .color('#007DFF')
          Text(`正在执行测试${this.loadingDots}`)
            .fontSize(13)
            .fontColor('#666666')
            .margin({ left: 8 })
        }
        .width('90%')
        .height(32)
        .justifyContent(FlexAlign.Center)
        .margin({ bottom: 8 })
      }

      // 输出区域（占满剩余空间，支持文本选择复制）
      Scroll(this.scroller) {
        Text(this.outputText)
          .fontSize(12)
          .fontFamily('monospace')
          .fontColor('#333333')
          .padding(10)
          .width('100%')
          .textAlign(TextAlign.Start)
          .copyOption(CopyOptions.LocalDevice)
      }
      .width('90%')
      .layoutWeight(1)
      .backgroundColor('#F8F9FA')
      .borderRadius(8)
      .margin({ bottom: 16 })
      .scrollBar(BarState.On)
      .scrollable(ScrollDirection.Vertical)

    }
    .width('100%')
    .height('100%')
    .justifyContent(FlexAlign.Start)
    .alignItems(HorizontalAlign.Center)
  }

  private startLoadingAnimation() {
    this.loadingDots = '';
    let count = 0;
    this.loadingTimer = setInterval(() => {
      count = (count + 1) % 4;
      this.loadingDots = '.'.repeat(count);
    }, 500);
  }

  private stopLoadingAnimation() {
    if (this.loadingTimer !== -1) {
      clearInterval(this.loadingTimer);
      this.loadingTimer = -1;
    }
    this.loadingDots = '';
  }

  aboutToAppear() {
    // 远程无人值守场景:启动后自动执行一次全量测试。
    // 测试已 async 化(worker 线程执行),需要再跑直接点「执行测试」即可,
    // 同进程可反复执行,无需重启应用。
    setTimeout(() => {
      this.runTestsSo();
    }, 500);
  }

  aboutToDisappear() {
    this.stopLiveOutput();
  }

  private dumpOutputToHilog(text: string) {
    const lines: string[] = text.split('\n');
    let batch: string[] = [];
    let len = 0;
    for (const line of lines) {
      const pieces: string[] = [];
      if (line.length > 900) {
        for (let i = 0; i < line.length; i += 900) {
          pieces.push(line.substring(i, i + 900));
        }
      } else {
        pieces.push(line);
      }
      for (const piece of pieces) {
        batch.push(piece);
        len += piece.length + 1;
        if (len > 900) {
          hilog.info(DOMAIN, TAG, 'OUT: %{public}s', batch.join('\n'));
          batch = [];
          len = 0;
        }
      }
    }
    if (batch.length > 0) {
      hilog.info(DOMAIN, TAG, 'OUT: %{public}s', batch.join('\n'));
    }
  }

  private runTestsSo() {
    if (this.isRunning) {
      return; // 上一轮还在跑(含 50ms 启动窗口),忽略重复触发
    }
    this.isRunning = true;
    this.outputText = '正在通过 dlopen 加载并执行单元测试...';
    this.exitCode = -1;
    this.profrawPath = '';
    this.profrawSize = 0;
    this.startLoadingAnimation();
    this.startLiveOutput();

    // 延迟执行，让 UI 先渲染 isRunning=true 的状态（按钮文字变为"测试中..."）
    setTimeout(() => {
      this.doRunTests();
    }, 50);
  }

  private startLiveOutput() {
    this.stopLiveOutput();
    this.liveTimer = setInterval(() => {
      const live: string = testNapi.getLiveOutput();
      if (live.length > 0) {
        this.outputText = live;
        this.scrollToBottom();
      }
    }, 250);
  }

  private stopLiveOutput() {
    if (this.liveTimer !== -1) {
      clearInterval(this.liveTimer);
      this.liveTimer = -1;
    }
  }

  private async doRunTests() {
    try {
      const sandboxInfo: testNapi.SandboxInfo = testNapi.getSandboxPath();
      hilog.info(DOMAIN, TAG, 'Sandbox path: %{public}s', sandboxInfo.sandboxPath);
      hilog.info(DOMAIN, TAG, 'SO path: %{public}s', sandboxInfo.soPath);

      let libsDir: string = '';
      if (sandboxInfo.soPath.length > 0) {
        const lastSlash: number = sandboxInfo.soPath.lastIndexOf('/');
        if (lastSlash > 0) {
          libsDir = sandboxInfo.soPath.substring(0, lastSlash);
        }
      }
      hilog.info(DOMAIN, TAG, 'libsDir: %{public}s', libsDir);

      const soPath: string = `${libsDir}/libflutter_ohos_app_test.so`;
      hilog.info(DOMAIN, TAG, 'soPath: %{public}s', soPath);

      const status: testNapi.BinaryCheckResult = testNapi.checkBinary(soPath);
      hilog.info(DOMAIN, TAG, 'Check %{public}s: exists=%{public}b', soPath, status.exists);

      if (!status.exists) {
        this.outputText = `未找到 .so 文件！\n\nsoPath: ${soPath}\nlibsDir: ${libsDir}\nsoPath(sandbox): ${sandboxInfo.soPath}\ncwd: ${sandboxInfo.cwd}`;
        this.scrollToBottom();
        return;
      }

      const context = getContext(this) as common.UIAbilityContext;
      const filesDir: string = context.filesDir;
      hilog.info(DOMAIN, TAG, 'filesDir: %{public}s', filesDir);

      const result: testNapi.BinaryResult = await testNapi.runTestsSo(soPath, filesDir, this.gtestFilter);
      this.exitCode = result.exitCode;
      this.profrawPath = result.profrawPath;
      this.profrawSize = result.profrawSize;
      this.outputText = `soPath: ${soPath}\n\n${result.output}`;
      this.dumpOutputToHilog(result.output);
      hilog.info(DOMAIN, TAG, 'Exit code: %{public}d', result.exitCode);
      hilog.info(DOMAIN, TAG, 'profrawPath: %{public}s', result.profrawPath);
      hilog.info(DOMAIN, TAG, 'profrawSize: %{public}d', result.profrawSize);
      this.scrollToBottom();
    } catch (err) {
      this.outputText = `执行失败: ${JSON.stringify(err)}`;
      hilog.error(DOMAIN, TAG, 'Run failed: %{public}s', JSON.stringify(err));
      this.scrollToBottom();
    } finally {
      this.stopLiveOutput();
      this.stopLoadingAnimation();
      this.isRunning = false;
    }
  }

  // gtest 对"filter 匹配 0 个用例"返回 0,若不解析会误显示"全部通过"
  private ranTestCount(): number {
    const m = this.outputText.match(/Running (\d+) tests? from/);
    return m ? Number(m[1]) : -1;
  }

  private badgeText(): string {
    // -1 = 尚未执行;小于 -1 = 原生侧基建失败(dlopen/捕获器等),与"未测试"区分
    if (this.exitCode === -1) {
      return '未测试';
    }
    if (this.exitCode < -1) {
      return `✗ 执行失败(${this.exitCode})`;
    }
    if (this.ranTestCount() === 0) {
      return '⚠ 未匹配到用例';
    }
    return this.exitCode === 0 ? '✓ 全部通过' : `✗ 失败(${this.exitCode})`;
  }

  private badgeColor(): string {
    if (this.exitCode === -1) {
      return '#CCCCCC';
    }
    if (this.exitCode < -1) {
      return '#dc3545';
    }
    if (this.ranTestCount() === 0) {
      return '#e8a23d';
    }
    return this.exitCode === 0 ? '#28a745' : '#dc3545';
  }

  private scrollToBottom() {
    // 延迟执行，确保 UI 已完成布局更新后再滚动
    setTimeout(() => {
      this.scroller.scrollEdge(Edge.Bottom);
    }, 100);
  }
}
