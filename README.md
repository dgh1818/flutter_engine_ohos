# Flutter Engine

原始仓来源：https://github.com/flutter/engine

## 仓库说明：
本仓库是基于flutter官方engine仓库拓展，可构建支持在OpenHarmony设备上运行的flutter engine程序。

## 构建说明：

* 构建环境：
1. 目前支持在Linux与MacOS中构建，Windows环境中支持构建gen_snapshot;
2. 请确保当前构建环境可以访问 `DEPS` 配置文件中 `allowed_hosts` 字段的URL列表。

* 构建步骤：
1. 构建基础环境：可参照[官网](https://github.com/flutter/flutter/wiki/Setting-up-the-Engine-development-environment)；

   a) 需要安装的工具： `git`, `curl` and `unzip`

   b) 克隆 `gclient` 与 `gn` 构建工具的代码仓库

   ```
   git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
   ```

   添加 `depot_tools` 到 `PATH` 环境变量中

   ```
   export PATH=/home/<user>/depot_tools:$PATH
   ```

   c) 需要安装的基础库：

   ```
    sudo apt install python3
    sudo apt install pkg-config
    sudo apt install ninja-build
   ```

   Windows构建环境：
   可参考[官网](https://github.com/flutter/flutter/wiki/Compiling-the-engine#compiling-for-windows) 
   "Compiling for Windows" 章节搭建Windows构建环境


2. 配置文件：

   a) 创建名为 `engine` 的空目录

   b) 在 `engine` 目录内新建 `.gclient` 文件
   
   c) 编辑 `.gclient` 文件：
   ```
    solutions = [
      {
        "managed": False,
        "name": "src/flutter",
        "url": "git@gitee.com:harmonycommando_flutter/flutter_engine.git@oh-3.22.0",
        "custom_deps": {},
        "deps_file": "DEPS",
        "safesync_url": "",
      },
    ]
   ```

3. 同步代码：在 `engine` 目录中执行 `gclient sync` 命令；这里会同步engine源码、官方packages仓，还有执行ohos_setup任务；

4. 下载sdk： 从[鸿蒙SDK](https://developer.huawei.com/consumer/cn/develop)下载配套开发工具，暂不支持非该渠道下载的套件

   ```sh
    # 需要设置的环境变量: HarmonyOS SDK, ohpm, hvigor, node
    export TOOL_HOME=/Applications/DevEco-Studio.app/Contents # mac环境
    export DEVECO_SDK_HOME=$TOOL_HOME/sdk # command-line-tools/sdk
    export PATH=$TOOL_HOME/tools/ohpm/bin:$PATH # command-line-tools/ohpm/bin
    export PATH=$TOOL_HOME/tools/hvigor/bin:$PATH # command-line-tools/ hvigor/bin
    export PATH=$TOOL_HOME/tools/node/bin:$PATH # command-line-tools/tool/node/bin
   ```

5. 开始构建：在 `engine` 目录，执行`./ohos`，即可开始构建支持ohos设备的flutter engine。
   从3.22.0版本开始，engine编译会默认编译local-engine以及local-host-engine, sdk在指定本地编译产物时需要同时指定这两个编译产物，例如：
   ```shell
   flutter build hap --target-platform ohos-arm64 --release --local-engine=<DIR>/engine/src/out/ohos_release_arm64/ --local-engine-host=<DIR>//engine/src/out/host_release
   ```
   
6. 更新代码：在 `engine` 目录，执行`./ohos -b master`

- 手动更新代码方法(至少经过一次完整gclient sync之后)：
   * 直接进入engine/src/flutter目录，在确保当前更新代码未涉及patch后，可以自由编辑或切换代码分支，然后执行编译命令，如若涉及patch修改，可手动apply相关patch或参考FAQ.7 批量reverse后重新apply patch

## FAQ
1. 运行项目工程报 `Member notfound:'isOhos'` 的错误：请确保src/third_party/dart目录下应用了所有的dart patch（补丁位于src/flutter/attachment/repos目录，可使用git apply应用patch）应用patch后重新编译engine

2. 提示`Permission denied:` 执行 `chmod +x <脚本文件>` 添加执行权限

3. 单独编译 `debug/release/profile` 模式的engine：`./ohos -t debug|release|profile`

4. 查看帮助：`./ohos -h`

5. 由于Windows和MacOS、Linux对换行符处理方式不同，在应用dart补丁时会造成dart vm snapshot hash结果不同，可通过以下方法获取当前snapshot hash值

   ```shell
   python engine/src/third_party/dart/tools/make_version.py --format='{{SNAPSHOT_HASH}}'
   ```

   如果获取到的值不是“8af474944053df1f0a3be6e6165fa7cf”那么就需要检查 `engine/src/third_party/dart/runtime/vm/dart.cc` 文件和 `engine/src/third_party/dart/runtime/vm/image_snapshot.cc` 文件中全部行的结尾是不是以LF结尾的，Windows可以使用notepad++查看，其它系统具体方法请自行查询

6. 修改了embedding层代码，运行./ohos没有生效，需要将任意cpp层文件修改一下时间戳，编译系统才可以识别(例如任意cpp文件添加空格保存后再删除空格后保存)，重新执行则可以触发embedding层重新打包

7. 手动重新应用patch的方法：

   ```shell
   python engine/src/flutter/attachment/scripts/ohos_reverse_patch.py
   python engine/src/flutter/attachment/scripts/ohos_setup.py
   ```

8. 配置代码自动跳转

   推荐使用vscode+clangd(与C/C++ IntelliSense有冲突)插件
   可参考配置：
   ```json
    "clangd.path": "./src/flutter/buildtools/linux-x64/clang/bin/clangd",
    "clangd.arguments": [
    "--compile-commands-dir=./src/out/ohos_release_arm64/",
    "--query-driver=./src/flutter/buildtools/linux-x64/clang/bin/clang++",
    ],
    "C_Cpp.intelliSenseEngine": "disabled",
   ```

9. Windows下gclient sync报错

   关键报错信息'A required privilege is not held by the client'

   解决方案：设置->开发者选项->开启"开发人员模式"

## embedding层代码构建指导

1. 编辑 `shell/platform/ohos/flutter_embedding/local.properties`：

   ```
    sdk.dir=<OpenHarmony的sdk目录>
    nodejs.dir=<nodejs的sdk目录>
   ```

2. 你需要复制文件到 `shell/platform/ohos/flutter_embedding/flutter/libs/arm64-v8a/`
   1. `debug/release`，复制 `libflutter.so`
   2. `profile`，复制 `libflutter.so` 和 `libvmservice_snapshot.so`

3. 在 `shell/platform/ohos/flutter_embedding` 目录下，执行 

    ```
     # buildMode可选值为: debug release profile
     hvigorw --mode module -p module=flutter@default -p product=default -p buildMode=debug assembleHar --no-daemon
    ```

4. `har` 文件输出路径为：`shell/platform/ohos/flutter_embedding/flutter/build/default/outputs/default/flutter.har`

ps:如果你使用的是DevEco Studio的Beta版本，编译工程时遇到“must have required property 'compatibleSdkVersion', location: build-profile.json5:17:11"错误，请参考《DevEco Studio环境配置指导.docx》中的‘6 创建工程和运行Hello World’【配置插件】章节修改 `shell/platform/ohos/flutter_embedding/hvigor/hvigor-config.json5` 文件。
