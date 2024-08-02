Flutter Engine
==============

Original warehouse source: https://github.com/flutter/engine

## Warehouse description:
This warehouse is based on the extension of Flutter's official engine warehouse and can build a Flutter engine program that runs on OpenHarmony devices.

## Build instructions:

* Build environment:
1. Supports building in Linux and MacOS, mainly building gen_snapshot for Windows environment;

2. Please ensure that the current build environment can access the URLs in `allowed_hosts` field configured in the DEPS file.


* Building steps:
1. Build a basic environment: please refer to the [official](https://github.com/flutter/flutter/wiki/Setting-up-the-Engine-development-environment) website;
   
   a) Install tools `git`, `curl` and `unzip`

   b) Clone repository that contains gclient and gn build tools

   ```
   git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
   ```

   Set `depot_tools` in `PATH` environment variable.

   ```
   export PATH=/home/<user>/depot_tools:$PATH
   ```

   c) Basic libraries that need to be installed:

   ```
   sudo apt install python3
   sudo apt install pkg-config
   sudo apt install ninja-build
   ```


   For Windows environment: please refer to the [official](https://github.com/flutter/flutter/wiki/Compiling-the-engine#compiling-for-windows) website;
   Chapter "Compiling for Windows"

2. Configuration file: 

   a) Create an empty folder `engine`

   b) Create a new `.gclient` file in the engine 

   c) Edit the `.gclient` file:

   ```
   solutions = [
      {
        "managed": False,
        "name": "src/flutter",
        "url": "git@gitee.com:openharmony-sig/flutter_engine.git",
        "custom_deps": {},
        "deps_file": "DEPS",
        "safesync_url": "",
      },
   ]
   ```

3. Synchronize code: In the engine directory, execute `gclient sync`; Here the engine source code, official packages repository will be synchronized and the ohos_setup task will be executed;

4. Download sdk: From [HarmonyOS Kit List](https://developer.huawei.com/consumer/cn/download/) Download development tools for your platform accordingly.

```sh
# Unzip file commandline/command-line-tools-xxxx.zip
export HOS_SDK_HOME=/home/<user>/ohos/command-line-tools/sdk
export NODE_HOME=/home/<user>/ohos/command-line-tools/tool/node/
export PATH=/home/<user>/ohos/command-line-tools/bin:$PATH
export PATH=$NODE_HOME/bin:$PATH
```

5. Start building: In the engine directory, execute `./ohos` to start building the flutter engine that supports ohos devices.
   
6. Update project: In the engine directory, execute `./ohos -b master`

## FAQ:
1. When running the project, an error of `Member notfound:'isOhos' is reported: Please ensure that all dart patches are applied in the src/third_party/dart directory` (the patches are located in the `src/flutter/attachment/repos` directory, and you can use git apply to apply the patch). Recompile the engine after patching

2. Prompt `Permission denied`: Execute `chmod +x <script file>` to add execution permissions

3. Compile the engine in debug/release/profile mode separately: `./ohos -t debug|release|profile`

4. See help: `./ohos -h`

5. Due to the different ways Windows, macOS, and Linux handle line endings, applying Dart patches can result in different Dart VM snapshot hash values. You can obtain the current snapshot hash value using the following method:

   ```shell
   python engine/src/third_party/dart/tools/make_version.py --format='{{SNAPSHOT_HASH}}'
   ```

   If the obtained value is not "8af474944053df1f0a3be6e6165fa7cf", then you need to check whether all lines at the end of the `engine/src/third_party/dart/runtime/vm/dart.cc` file and the `engine/src/third_party/dart/runtime/vm/image_snapshot.cc` file end with LF. On Windows, you can use Notepad++; for other systems, please consult specific methods on your own.



## Embedding layer code construction guide

1. Edit `shell/platform/ohos/flutter_embedding/local.properties`:

     ```
     sdk.dir=<OpenHarmony sdk directory>
     nodejs.dir=<nodejs sdk directory>
     ```

2. You need to copy file to `shell/platform/ohos/flutter_embedding/flutter/libs/arm64-v8a/` 
   1. debug/release，copy `libflutter.so`
   2. profile，copy `libflutter.so` and `libvmservice_snapshot.so`

3. In the `shell/platform/ohos/flutter_embedding` directory, execute

     ```
     # The optional values for buildMode are: debug release profile
     hvigorw --mode module -p module=flutter@default -p product=default -p buildMode=debug assembleHar --no-daemon
     ```



4. The har file output path is: `shell/platform/ohos/flutter_embedding/flutter/build/default/outputs/default/flutter.har`

ps: If you are using the Beta version of DevEco Studio and encounter the error "must have required property 'compatibleSdkVersion', location: build-profile.json5:17:11" when compiling the project, please refer to the "DevEco Studio Environment Configuration Guide." docx》Chapter '6 Creating Projects and Running Hello World' [Configuration Plugin] Modify the `shell/platform/ohos/flutter_embedding/hvigor/hvigor-config.json5` file.
