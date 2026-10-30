# unittest_ohos — Flutter 引擎 UT 执行器

在非 root 鸿蒙设备上,通过 `dlopen` 加载 Flutter 引擎编译的测试 so,执行 C++ GoogleTest 单元测试并收集 LLVM 覆盖率数据。同一进程内可反复执行。

## 功能特性

- **动态加载执行**:`dlopen` 加载 `libflutter_ohos_app_test.so`,调用导出的 `FlutterOhosRunAllTests` 执行用例
- **gtest_filter 过滤**:输入框填写表达式可单独执行指定用例(如 `OhosExternalTextureTest.*`)
- **异步执行**:测试在 napi worker 线程跑,主线程零阻塞,「执行测试」按钮可无限次点击
- **覆盖率收集**:测试结束手动调用 `FlutterOhosWriteCoverageProfile` 导出 `coverage.profraw`
- **输出捕获**:stdout/stderr 重定向到内存文件,执行完毕后在 UI 显示并分批写入 hilog
- **结果徽章**:UI 直接显示 ✓ 全部通过 / ✗ 失败(退出码);「覆盖率」按钮弹窗显示 profraw 路径与大小

## 快速开始

```bash
# 1. 编译引擎(带覆盖率插桩)
cd <engine-source>/engine
./ohos -t debug -g '\--coverage' -n config compile
# 产物: engine/src/out/ohos_debug_arm64/libflutter_ohos_app_test.so

# 2. 部署到 App(注意是 entry/libs,不是 entry/src/main/libs)
cp src/out/ohos_debug_arm64/libflutter_ohos_app_test.so \
   src/flutter/testing/ohos/unittest_ohos/entry/libs/arm64-v8a/

# 3. 打包并安装
cd src/flutter/testing/ohos/unittest_ohos
hvigorw assembleHap --mode module -p module=entry@default -p product=default -p buildMode=debug --no-daemon
hdc install -r entry/build/default/outputs/default/entry-default-signed.hap
```

> 真机安装需要已签名的 HAP:在 DevEco Studio 中为本工程配置自动签名后,`assembleHap` 才会产出 `entry-default-signed.hap`。

普通构建(不带 `-g '\--coverage'`)的 so 也能正常执行测试,`FlutterOhosWriteCoverageProfile` 会返回 -1(弱符号降级),UI 显示"无覆盖率"。

## 使用方法

### 在 App 里手动执行

打开 App,启动即自动执行全量测试,也可点击「执行测试」按钮。执行指定用例:在 gtest_filter 输入框中填写过滤表达式,例如:

- `OhosExternalTextureTest.*` — 执行整个 TestSuite
- `OhosExternalTextureTest.FdIsValidIdentifiesCharDevice` — 执行单个用例
- `OhosContextTest.*:OhosExternalTextureTest.*` — 执行多个 Suite

> 清空输入框后再次执行,自动恢复全量(每次调用前重置 `GTEST_FLAG(filter)` 为 `"*"`)。

### 一键执行(run_ohos_unittest.py)

一条命令自动完成:冷启动 App 执行全量测试、判定退出码、拉取 profraw 生成 HTML 报告(默认即带覆盖率):

```bash
cd <engine-source>/engine/src/flutter/testing
python3 run_ohos_unittest.py
```

测试失败时脚本以非 0 退出,可接 CI。参数见[「生成覆盖率报告」](#生成覆盖率报告)。

### 查看日志

```bash
hdc shell hilog | grep unittest_ohos
```

关键日志包括:
- `Running tests: argc=` — 测试执行参数
- `Tests exited with code` — 测试退出码
- `[coverage] profraw target` / `profraw size` — 覆盖率写入路径与大小

## 生成覆盖率报告

测试执行与报告生成使用 run_ohos_unittest.py 脚本,自动执行测试并生成 HTML 报告:

```bash
cd <engine-source>/engine/src/flutter/testing
python3 run_ohos_unittest.py
```

脚本会自动完成:
1. 冷启动 App 自动执行全量测试(`aboutToAppear` 触发),等待完成,失败以非 0 退出
2. 通过 hdc file recv 从设备沙箱拉取 coverage.profraw
3. 用 llvm-profdata merge -sparse 合并为 all.profile
4. 用 llvm-cov show 生成 HTML 覆盖率报告

报告输出路径:

```
engine/src/out/ohos_debug_arm64/coverage/unittest_app/index.html
```

可选参数:

| 参数 | 默认值 | 说明 |
|------|--------|------|
| --type | ohos | 测试类型(与 run_tests.py 同名,当前仅支持 ohos) |
| --ohos-variant | ohos_debug_arm64 | 引擎构建变体(与 run_tests.py 同名) |
| --coverage / --no-coverage | 开 | 生成覆盖率报告(与 run_tests.py 同名;默认开) |
| --coverage-source-regex | ohos | 源文件过滤正则 |
| --hdc-path | PATH 查找 | hdc 二进制路径 |
| --serial | 无 | 目标设备(`hdc list targets` 可查;单设备直连不需要) |
| --timeout | 300 | 等待测试完成的秒数 |
| --output-dir | \<build_dir\>/coverage/unittest_app | 报告输出目录 |
| --skip-run | 关 | 不重启 App,直接用设备上现有 profraw 出报告 |
| --local-profraw | 无 | 使用本地 profraw,不从设备拉取 |

## 技术原理

### dlopen 执行测试

鸿蒙应用沙箱禁止直接执行二进制,但允许 `dlopen` 共享库并调用其中函数。引擎的 `ohos_unittests_entry.cc` 导出两个 C 接口:

```cpp
// 执行所有 GoogleTest 用例
extern "C" int FlutterOhosRunAllTests(int argc, char** argv);
// 手动触发 LLVM 覆盖率数据写入
extern "C" int FlutterOhosWriteCoverageProfile(const char* filename);
```

### 线程模型与 so 生命周期(两个关键约束)

- **测试必须跑在 worker 线程**:全量测试耗时十几秒,同步调用会阻塞 ArkTS 主线程,触发系统 `THREAD_BLOCK_3S` watchdog(多次发生会被冻结/杀进程)。因此 `runTestsSo` 用 napi async work 在 worker 线程执行,主线程立即返回 Promise。
- **加载后绝不 dlclose**:测试执行中引擎代码会在 worker 线程注册 TLS 析构函数(如 `fml::MessageLoop` 的 thread_local),这些析构代码位于测试 so 内;worker 线程被线程池回收时会执行它们,若 so 已被卸载,析构函数即成野指针,表现为"测试跑完几秒后 App 自行退出"。so 驻留进程,由系统在 App 退出时统一回收。

### 覆盖率收集

`enable_coverage = true` 编译的 so 内含 LLVM profile runtime,正常在进程 `exit()` 时经 atexit 写盘;App 不能 `exit()`,因此通过导出的 `FlutterOhosWriteCoverageProfile` 手动写入沙箱:

```
沙箱路径: /data/storage/el2/base/haps/entry/files/coverage.profraw
真实路径: /data/app/el2/100/base/com.example.unittest_ohos/haps/entry/files/coverage.profraw
```

### gtest_filter 处理

so 内的 `InitGoogleTest` 不解析命令行参数,由入口函数手动解析 argv 设置 `testing::GTEST_FLAG(filter)`;so 驻留内存导致 flag 残留,每次调用前先重置为 `"*"`。

### stdout/stderr 捕获

`memfd_create` 创建内存文件,`dup2` 重定向 stdout/stderr,测试完成后读回显示(UI 一次性显示 + 分批写 hilog)。

## NAPI 接口(libentry.so 导出)

| 接口 | 参数 | 返回值 |
|------|------|--------|
| `checkBinary(path)` | so 路径 | `{exists}` |
| `getSandboxPath()` | 无 | `{sandboxPath, soPath, cwd}` |
| `runTestsSo(soPath, filesDir, filter?)` | so 路径、可写目录、可选 filter | `Promise<{exitCode, output, profrawPath, profrawSize}>` |

## 设备路径

| 用途 | 路径 |
|------|------|
| 测试 so(沙箱) | `/data/storage/el1/bundle/libs/arm64/libflutter_ohos_app_test.so` |
| profraw(沙箱) | `/data/storage/el2/base/haps/entry/files/coverage.profraw` |
| profraw(真实,供 hdc pull) | `/data/app/el2/100/base/com.example.unittest_ohos/haps/entry/files/coverage.profraw` |
