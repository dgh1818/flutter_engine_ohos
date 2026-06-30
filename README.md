Flutter SDK & Engine 仓库
========================

<span style="color:red">**仓库迁移公告**：Flutter 鸿蒙化相关仓库已整体迁移至 [CPF-Flutter](https://gitcode.com/CPF-Flutter) 组织。本仓库（flutter_flutter）新地址为 [CPF-Flutter/flutter_flutter](https://gitcode.com/CPF-Flutter/flutter_flutter)，旧仓库将不再维护，请及时更新远程地址和依赖引用。详情参见：[迁移公告](https://gitcode.com/openharmony-tpc/flutter_flutter/wiki/Flutter%20%E9%B8%BF%E8%92%99%E5%8C%96%E4%BB%93%E5%BA%93%E8%BF%81%E7%A7%BB%E5%85%AC%E5%91%8A%EF%BC%9A%E5%85%A8%E6%96%B0%20CPF-Flutter%20%E7%BB%84%E7%BB%87%E4%B8%8A%E7%BA%BF)</span>

## 仓库说明

本仓库是 **[Flutter SDK](https://github.com/flutter/flutter)** 和 **[Flutter Engine](https://github.com/flutter/flutter/tree/master/engine)** 的 **OpenHarmony** 适配版本，当前版本分支基于 Flutter 官方社区 [![Flutter Version](https://img.shields.io/badge/Flutter-3.41.9-blue?logo=flutter)](https://github.com/flutter/flutter/commit/00b0c91f06209d9e4a41f71b7a512d6eb3b9c694) 构建，由 OpenHarmony-Flutter 团队维护。开发者可使用熟悉的 Flutter 技术栈开发 OpenHarmony 应用，也可基于本仓库源码构建支持 OpenHarmony 的 Flutter Engine。

> 版本规划与分支策略请参见：[Flutter OH 版本规划与分支策略](https://gitcode.com/CPF-Flutter/flutter_flutter/wiki/Flutter-OH%E7%89%88%E6%9C%AC%E6%BC%94%E8%BF%9B%E8%A7%84%E5%88%92%E5%92%8C%E5%88%86%E6%94%AF%E7%AD%96%E7%95%A5.md)

## 仓库结构

Flutter 3.41.9 版本，Engine 源码已合并到仓库的 `engine/` 目录下，无需再单独克隆 Engine 仓库。

```json
flutter_flutter/                 # 仓库根目录
├── packages/                    # Flutter SDK 框架代码
│   ├── flutter/                 # Flutter 框架核心
│   ├── flutter_tools/           # Flutter CLI 工具
│   └── ...
├── engine/                      # Flutter Engine 引擎代码
└── ...
```

## 开发指南

### 应用开发

- [Flutter OH 开发文档](https://gitcode.com/openharmony-tpc/flutter_samples/blob/master/README.md)

- [Flutter OH 环境搭建指导](https://gitcode.com/openharmony-tpc/flutter_samples/blob/master/ohos/docs/03_environment/OpenHarmony-flutter%E7%8E%AF%E5%A2%83%E6%90%AD%E5%BB%BA%E6%8C%87%E5%AF%BC.md)
- [Flutter OH 应用构建指导](https://gitcode.com/openharmony-tpc/flutter_samples/blob/master/ohos/docs/04_development/OpenHarmony-flutter%E5%BA%94%E7%94%A8%E6%9E%84%E5%BB%BA%E6%8C%87%E5%AF%BC.md)
- [Flutter OH 三方库适配列表](https://gitcode.com/OpenHarmony-Flutter/docs/blob/main/ThirdpartyLibrarites.md)
- [Flutter 官方开发指南与 API 文档](https://docs.flutter.dev/)

### 构建Engine

- [Flutter OH Engine构建指导](https://gitcode.com/openharmony-tpc/flutter_samples/blob/master/ohos/docs/03_environment/Flutter-OH-engine%E6%9E%84%E5%BB%BA%E6%8C%87%E5%AF%BC.md)

## 升级指导

请参见：[Flutter OH 版本升级指导](https://gitcode.com/openharmony-tpc/flutter_samples/blob/master/ohos/docs/10_appendix/Flutter-OH%E7%89%88%E6%9C%AC%E5%8D%87%E7%BA%A7%E6%8C%87%E5%AF%BC.md)

## 支持指令

已适配 OpenHarmony 开发的指令列表：

| 指令名称   | 指令描述           | 使用说明                                                     |
| ---------- | ------------------ | ------------------------------------------------------------ |
| doctor     | 环境检测           | `flutter doctor`                                             |
| config     | 环境配置           | `flutter config --<key> <value>`                             |
| create     | 创建新项目         | `flutter create --platforms [ohos,android,ios] --org <org> <appName>` |
| create     | 创建module模板     | `flutter create -t module <module_name>`                     |
| create     | 创建plugin模板     | `flutter create -t plugin --platforms [ohos,android,ios] <plugin_name>` |
| create     | 创建plugin_ffi模板 | `flutter create -t plugin_ffi --platforms [ohos,android,ios] <plugin_name>` |
| devices    | 已连接设备查找     | `flutter devices`                                            |
| install    | 应用安装           | `flutter install -t <deviceId> <hap文件路径>`                |
| assemble   | 资源打包           | `flutter assemble`                                           |
| build      | 测试应用构建       | `flutter build hap --debug [--target-platform ohos-arm64] [--local-engine=<兼容ohos的debug engine产物路径>]` |
| build      | 正式应用构建       | `flutter build hap --release [--target-platform ohos-arm64] [--local-engine=<兼容ohos的release engine产物路径>]` |
| run        | 应用运行           | `flutter run [--local-engine=<兼容ohos的engine产物路径>]`    |
| attach     | 调试模式           | `flutter attach`                                             |
| screenshot | 截屏               | `flutter screenshot`                                         |
| pub        | 获取依赖           | `flutter pub get`                                            |
| clean      | 清除项目依赖       | `flutter clean`                                              |
| cache      | 清除全局缓存数据   | `flutter pub cache clean`                                    |

## 常见问题

1. 切换FLUTTER_STORAGE_BASE_URL后需删除\<flutter\>/bin/cache 目录，并在项目中执行flutter clean后再运行

2. 若出现报错：`The SDK license agreement is not accepted`，参考执行以下命令后再次编译：

   ```shell
   ./ohsdkmgr install ets:9 js:9 native:9 previewer:9 toolchains:9 --sdk-directory='/home/xc/code/sdk/ohos-sdk/' --accept-license
   ```

3. 如果你使用的是DevEco Studio的Beta版本，编译工程时遇到"must have required property 'compatibleSdkVersion', location: demo/ohos/build-profile.json5:17:11"错误。请参考[《DevEco Studio配置文件》](https://developer.huawei.com/consumer/cn/doc/harmonyos-guides/ide-hvigor-configuration-file)中的 [`工程级build-profile.json5文件 → products`](https://developer.huawei.com/consumer/cn/doc/harmonyos-guides/ide-hvigor-build-profile-app#section45865492619) 配置 `compatibleSdkVersion`。

4. 若提示安装报错：`fail to verify pkcs7 file` 请执行指令

   ```shell
   hdc shell param set persist.bms.ohCert.verify true
   ```

5. linux虚拟机通过hdc无法直接发现OpenHarmony设备

   解决方案：在windows宿主机中，开启hdc server，具体指令如下：

   ```shell
   hdc kill
   hdc -s serverIP:8710 -m
   ```

    在linux中配置环境变量：

   ```shell
   HDC_SERVER=<serverIP>
   HDC_SERVER_PORT=8710
   ```

   配置完成后flutter sdk可以通过hdc server完成设备连接，也可参考[官方指导](https://docs.openharmony.cn/pages/v5.0/zh-cn/device-dev/subsystems/subsys-toolchain-hdc-guide.md/#hdc-client%E5%A6%82%E4%BD%95%E8%BF%9C%E7%A8%8B%E8%AE%BF%E9%97%AEhdc-server)。

6. 构建Hap任务时报错：Error: The hvigor depends on the npmrc file. Configure the npmrc file first.

   请在用户目录`~`下创建文件`.npmrc`，该配置也可参考[DevEco Studio官方文档](https://developer.harmonyos.com/cn/docs/documentation/doc-guides-V3/environment_config-0000001052902427-V3)，编辑内容如下：

   ```json
   registry=https://repo.huaweicloud.com/repository/npm/
   @ohos:registry=https://repo.harmonyos.com/npm/
   ```

7. 查日志时，存在日志丢失现象。
   解决方案：关闭全局日志，只打开自己领域的日志

   ```shell
   # 步骤一：关闭所有领域的日志打印（部分特殊日志无法关闭）
   hdc shell hilog -b X
   # 步骤二：只打开自己领域的日志
   hdc shell hilog <level> -D <domain>
   # 其中<level>为日志打印的级别：D/I/W/E/F,<domain>为Tag前面的数字
   # 举例：
   # 打印A00000/XComFlutterOHOS_Native的日志，需要设置hdc shell hilog -b D -D A00000
   # 注：上面的设置在机器重启后会失效，如果要继续使用，需要重新设置。
   ```

8. 若Api11 Beta1版本的机器上无法启动debug签名的应用，可以通过将签名换成正式签名，或在手机端打开开发者模式解决（步骤：设置->通用->开发者模式）

9. 如果报`Invalid CEN header (invalid zip64 extra data field size)`异常，请更换Jdk版本，参见[JDK-8313765](https://bugs.openjdk.org/browse/JDK-8313765)

10. 运行debug版本的flutter应用用到鸿蒙设备后报错（release和profile版本正常）

    1. 报错信息: `Error while initializing the Dart VM: Wrong full snapshot version, expected '8af474944053df1f0a3be6e6165fa7cf' found 'adb4292f3ec25074ca70abcd2d5c7251'`
    2. 解决方案: 依次执行以下操作
       1. 设置环境变量 `export FLUTTER_STORAGE_BASE_URL=https://flutter-ohos.obs.cn-south-1.myhuaweicloud.com`
       2. 删除 <flutter>/bin/cache 目录下的缓存
       3. 执行 `flutter clean`，清除项目编译缓存
       4. 运行 `flutter run -d $DEVICE --debug`
    3. 补充信息: 运行android或ios出现类似错误，也可以尝试还原环境变量 FLUTTER_STORAGE_BASE_URL ，清除缓存后重新运行。

11. Beta2版本的ROM更新后，不再支持申请有执行权限的匿名内存，导致debug运行闪退。

    1. 解决方案：更新 flutter_flutter 到 a44b8a6d (2024-07-25) 之后的版本。

    2. 关键日志：

       ```json
           #20 at attachToNative (oh_modules/.ohpm/@ohos+flutter_ohos@g8zhdaqwu8gotysbmqcstpfpcpy=/oh_modules/@ohos/flutter_ohos/src/main/ets/embedding/engine/FlutterNapi.ets:78:32)
           #21 at attachToNapi (oh_modules/.ohpm/@ohos+flutter_ohos@g8zhdaqwu8gotysbmqcstpfpcpy=/oh_modules/@ohos/flutter_ohos/src/main/ets/embedding/engine/FlutterEngine.ets:144:5)
           #22 at init (oh_modules/.ohpm/@ohos+flutter_ohos@g8zhdaqwu8gotysbmqcstpfpcpy=/oh_modules/@ohos/flutter_ohos/src/main/ets/embedding/engine/FlutterEngine.ets:133:7)
       ```

12. 构建Hap命令直接执行`flutter build hap`即可，不再需要`--local-engine`参数，直接从云端获取编译产物。

13. 配置环境完成后执行 flutter 命令 出现闪退。

    1. 解决方案：windows环境中添加git环境变量配置。

       ```shell
       export PATH=<git path>/cmd:$PATH
       ```

14. 执行`flutter pub cache clean` 正常 执行`flutter clean` 报错，按照报错信息执行 update 命令也没有效果。

    1. 解决方案：通过注释掉 build.json5 文件中的配置规避。

    2. 报错信息:

       ```json
        #Parse ohos module. json5 error: Exception: Can not found module.json5 at
        #D:\pub_cache\git\flutter_packages-b00939bb44d018f0710d1b080d91dcf4c34ed06\packages\video_player\video_player_ohos\ohossrc\main\module.json5.
        #You need to update the Flutter plugin project structure.
        #See
        #https://gitcode.com/openharmony-tpc/flutter_samples/blob/master/ohos/docs/09_specifications/update-flutter-plugin-structure.md
       ```

15. 执行`flutter build hap` 时遇到路径校验报错。

    1. 解决方案：

       - 打开 deveco 安装路径 D:\DevEco Studio\tools\hvigor\hvigor-ohos-plugin\res\schemas 下的 ohos-project-build-profile-schema.json文件。
       - 在该文件中找到包含："pattern": "^(\\./|\\.\\./)[\\s\\S]+$"的行,并删除此行。

    2. 报错信息:

       ```json
        #hvigor  ERROR: Schema validate failed.
        #        Detail: Please check the following fields.
        #instancePath: 'modules[1].scrPath',
        #keyword: 'pattern'
        #params: { pattern:'^(\\./|\\.\\./)[\\s\\S]+$' },
        #message: 'must match pattern "^(\\./|\\.\\./)[\\s\\S]+$"',
        #location: 'D:/work/videoplayerdemo/video_cannot_stop_at_background/ohos/build-profile.json:42:146'
       ```

16. 执行`flutter build hap` 报错。

    1. 解决方案：打开 deveco 安装路径 D:\DevEco Studio\tools\hvigor\hvigor-ohos-plugin\src\model\module 下的 core-module-model-impl.js,
       修改 findBelongProjectPath 方法（需要管理员权限，可另存为后替换）

       ```
        findBelongProjectPath(e) {
          if (e === path_1.default.dirname(e)) {
             return this.parentProject.getProjectDir()
          }
        }
       ```

    2. 报错信息:

       ```json
       # hvigor  ERROR: Cannot find belonging project path for module at D:\.
       # hvigor  ERROR:  BUILD FAILED in 2s 556ms.
       #Running Hvigor task assembleHap...
       #Oops; flutter has exited unexpectedly: "ProcessException: The command failed
       #  <Command: hvigorw --mode module -p module=video_player_ohos@default -p product=default assmbleHar --no-daemon"
       #A crash report has been written to D:\work\videoplayerdemo\video_cannot_stop_at_background\flutter_03.log.
       ```

17. DevEco-Studio(5.0.3.600 Beta3)，windows版本编译flutter应用报错

    1. 解决方案：更新 flutter_flutter 到 c6fbac2b (2024-08-09) 之后的版本。

    2. 关键日志：

       ```json
       hvigor ERROR: Schema validate failed.
       	Detail: Please check the following fields.
       	{
       		instancePath: 'modules[2].srcPath',
       		keyword: 'pattern',
       		params: { pattern: '^(\\./|\\.\\./)[\\s\\S]+$' },
       		message: 'must match pattern "^(\\./|\\.\\./)[\\s\\S]+$"',
       	}
       ```

18. 在.ohos的项目执行`flutter clean` 报错，然后再执行`flutter pub get`也报错。

    1. 解决方案：删除.ohos文件夹，重新flutter pub get 即可

    2. 报错信息：

       ```json
          Oops; flutter has exited unexpectedly: "PathNotFoundException: Cannot open file, path = 'D:\code\.ohos\build-profile.json5' (OS Error: 系统找不到指定的文件。，error = 2)".
          A crash report has been written to D:\code\flutter_01.log.
       ```

19. 模拟器执行时发生白屏、崩溃等现象。

    1. 模拟器调试支持Mac(arm64)和Windows(x64)，还不支持Mac(x86)
    2. 模拟器暂不支持vulkan，请尝试构建步骤2.1，关闭impeller后重试

20. flutter profile模式下编译或运行失败

    1. 请在ohos项目build_profile.json5中添加buildModeSet字段，可参考[complex_layout](./dev/benchmarks/complex_layout/ohos/build-profile.json5)

    2. 报错信息:

       ```json
       hvigor ERROR: Build mode 'profile' used in command line is not declared in buildModeSet in /xxx/example/ohos/build-profile.json5.
       ```

> [更多FAQ](https://gitcode.com/openharmony-tpc/flutter_samples/blob/master/ohos/docs/08_FAQ/README.md)

## 贡献指南

如果您想为 Flutter-OH 贡献代码，请参考 [Flutter-OH 代码合入流程](https://gitcode.com/openharmony-tpc/flutter_flutter/wiki/Flutter_OH%E4%BB%93%E5%BA%93%E4%BB%A3%E7%A0%81%E5%90%88%E5%85%A5%E6%B5%81%E7%A8%8B.md) 了解详细的贡献步骤和规范。

## 问题交流

- 问题反馈：欢迎在 [Flutter框架仓库](https://gitcode.com/openharmony-tpc/flutter_flutter/issues) 以及各个Flutter三方库提交 issue。

