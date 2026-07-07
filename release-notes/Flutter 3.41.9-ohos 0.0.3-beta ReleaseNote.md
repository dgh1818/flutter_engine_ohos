## 版本概述
本版本为Flutter OpenHarmony平台0.0.3-beta版本，基于Flutter 3.41.9版本适配。本版本支持和完善OpenHarmony平台侧能力，提供平台化Channel、外接纹理、云端SDK等特性，并优化性能。

### 主要更新

**新增特性**

- 手机端支持密码保险箱功能（依赖待发布API）
- 切换分栏实现方案，支持router路由方式下的分栏
- 添加分栏功能
- 分栏起始页显示图标
- 添加对配置项supportLandscapeFullscreen的处理
- 添加对配置项enableReducedContainerSize的处理
- flutter项目Web页面，支持鼠标拖拽调整尺寸
- [OHOS] Add DMA zero-copy image decode path
- [OHOS] Add DMA zero-copy image decode path with P3 support
- add lookupCallbackInformationBigInt
- add frist colorspace
- feat: Add Dart heap memory monitoring and reporting
- Enable static snapshot linking for OHOS debug mode
- ohos开启指针压缩
- 开启指针压缩
- 发送低内存警告，触发图像缓存清理
- Support llvm18
- Add --profile-startup switch for ohos
- Frame Buffer PTS Optimization for Delayed Frame Presentation

**问题修复**

- 分栏功能中，当栈顶是弹窗时，不要拦截pop函数
- 修改FlutterView.ets中鸿蒙原生事件调用逻辑，增加isActive状态判断
- Fix OHOS platform view active touch cancellation
- Fix the issue where Shift + left arrow can only select one character
- fix NavigationChannel crash
- fix White screen issue when restoring after minimizing the window
- [OHOS] Fix PixelMap ReadPixels temp buffer cleanup
- [OHOS] Restore PixelMap ReadPixels tight-row semantics
- 修复性能雷达滑动丢帧上报字段值问题
- Fix plugin_ffi dynamic library loading on Windows
- Fixed the issue of small mouse scroll step value
- Fix the issue of DPI repeatedly redirecting to the same page
- 修复一定深度的主页调用popUntil问题+模态弹窗无法关闭问题
- Fix OHOS native asset hook OS compatibility
- fix: Fix safe area avoidance in tri-fold freeform multi-window mode
- 修复弹窗问题+优化读取配置文件
- fix TextField accessibility read content
- 解决重复builder问题+解决键盘无法重新聚焦+强制pop不能返回+observe多个监听问题
- 修复分栏模式下开启无障碍阅读左分栏无法响应的问题
- Fix the issue of DPI conflicting with Dart's adaptive behavior
- fix Double click the control for the first time and jump to the green box position
- 当配置文件配置项值为空字符串时，要识别为无效值
- fix status bar icons turn gray when statusBarIconBrightness not set
- Modify the srgb rendering to p3 issue, cpu computing issue, shadow rendering granularity issue
- 防止 SplitViewContainer 在 build 期间调用 setState，避免异常警告
- 解决预加载场景渲染异常问题
- fix green border not update
- 修复鼠标左右键按键异常
- Fixed the error message for wide color gamut merging
- 修复OHOS侧SetSemanticsTreeEnabled未生效问题
- 修复setApplicationLocale原生侧接收失败
- [CP-stable] Check for overflow when computing pixel buffer size for animated PNG frame
- [OHOS] Avoid throwing from detached PlatformView render
- 取消UI线程检测卡死
- Fix OHOS plugin registration error
- [CP-stable] Only use LLDB breakpoint in debug mode
- 解决windowstage可能已经销毁的崩溃
- [OHOS] Fix PlatformView detach lifecycle
- Fixed EventChannel.endOfStream() not triggering onDone event in Dart
- Fix OHOS plugin registration error

## 版本配套
- 引擎构建最低要求 API：**待发布最新API**
- 应用构建目标 API：**待发布最新API**
- 应用最低运行 API：**待发布最新API**
- Flutter SDK：**3.41.9-ohos-0.0.3-beta**（由于flutter版本解析规则，为了避免版本比较解析失败，将显示为3.41.10-ohos-0.0.3-beta）

## Changelog
- [3.41.9-ohos-0.0.3-beta](../CHANGELOG_OHOS.md)

## 赋能文档
- [文档链接](https://gitcode.com/openharmony-tpc/flutter_samples/tree/master/ohos/docs)
