Flutter Engine
==============

Source of the original repository: https://github.com/flutter/engine

## Repository Description
This repository is an extension of the Flutter engine repository. It enables Flutter engine to run on OpenHarmony devices.

## How to Build

* Build environment:
1. Linux or macOS that support Flutter engine; Windows that supports **gen_snapshot**.
2. Access to the **allowed_hosts** field in the DEPS file.

* Build steps:
1. Set up a basic environment. For details, see the [official document](https://github.com/flutter/flutter/wiki/Setting-up-the-Engine-development-environment).

   The following libraries need to be installed:

   ```
   sudo apt install python3
   sudo apt install pkg-config
   sudo apt install ninja-build
   ```

   Configure `node`. Specifically, download `node`, unzip it, and configure it into environment variables.

   ```
   # nodejs
   export NODE_HOME=/home/<user>/env/node-v14.19.1-linux-x64
   export PATH=$NODE_HOME/bin:$PATH
   ```

   For Windows:
   Refer to the section "Compiling for Windows"
   in the [official document](https://github.com/flutter/flutter/wiki/Compiling-the-engine#compiling-for-windows).


2. Configure the file. Specifically, create an empty folder **engine**, create the **.gclient** file in this folder, and edit the file.

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

3. Synchronize the code. In the **engine** directory, execute `gclient sync`. The engine source code and packages repository will be synchronized, and the **ohos_setup** task will be executed.

4. Download the SDK. Download the supporting development kits from [HarmonyOS SDK](https://developer.huawei.com/consumer/en/develop). Suites downloaded from other platforms are not supported.

   ```sh
   # Environment variables to set: HarmonyOS SDK, ohpm, hvigor, and node.
   export TOOL_HOME=/Applications/DevEco-Studio.app/Contents # macOS environment
   export DEVECO_SDK_HOME=$TOOL_HOME/sdk # command-line-tools/sdk
   export PATH=$TOOL_HOME/tools/ohpm/bin:$PATH # command-line-tools/ohpm/bin
   export PATH=$TOOL_HOME/tools/hvigor/bin:$PATH # command-line-tools/ hvigor/bin
   export PATH=$TOOL_HOME/tools/node/bin:$PATH # command-line-tools/tool/node/bin
   ```

5. Start building. In the **engine** directory, execute `./ohos` to start building the Flutter engine that supports ohos devices.
   
6. Update the code. In the **engine** directory, execute `./ohos -b master`.

## Engine Build Products

  See [Build Products](https://docs.qq.com/sheet/DUnljRVBYUWZKZEtF?tab=BB08J2).

## FAQs
1. The message `Member notfound:'isOhos'` is reported during project running.<br>Install all dart patches in the **src/third_party/dart** directory. (The patches are located in the **src/flutter/attachment/repos** directory, and you can use **git apply** to apply the patches). Recompile the engine after installing the patches.

2. The message `Permission denied` is reported.<br>Execute `chmod +x < script file >` to add the execution permission.

3. To compile the engine in debug, release, or profile mode, execute `./ohos -t debug`, `./ohos -t release`, or `./ohos -t profile`, respectively.

4. To find the help, execute `./ohos -h`.

5. Different ways for handling newline characters by Windows, macOS, and Linux will cause different results of **dart vm snapshot hash** when installing the dart patches. You can obtain the value of **snapshot hash** through the following method:

   ```shell
   python xxx/src/third_party/dart/tools/make_version.py --format='{{SNAPSHOT_HASH}}'
   ```

   Here, **xxx** is the engine path you created.

   If the obtained value is not **8af474944053df1f0a3be6e6165fa7cf**, check whether all lines of the **xxx/src/third_party/dart/runtime/vm/dart.cc** file and the **xxx/src/third_party/dart/runtime/vm/image_snapshot.cc** file end with **LF**. For Windows, you can use **Notepad++** to check; for other systems, consult specific methods on your own.


## Code Building at the Embedding Layer

1. Edit **shell/platform/ohos/flutter_embedding/local.properties** as follows:

   ```
   sdk.dir=<OpenHarmony SDK directory>
   nodejs.dir=<nodejs SDK directory>
   ```

2. Copy the file to `shell/platform/ohos/flutter_embedding/flutter/libs/arm64-v8a/` from the built `engine` directory.
    1. For the debug or release version, copy `libflutter.so`.
    2. For the profile version, copy `libflutter.so` and `libvmservice_snapshot.so`.

3. In the **shell/platform/ohos/flutter_embedding** directory, execute the following instruction:

    ```
   # buildMode can be set to debug, release, or profile.
   hvigorw --mode module -p module=flutter@default -p product=default -p buildMode=debug assembleHar --no-daemon
    ```

4. Find the HAR file from the path `shell/platform/ohos/flutter_embedding/flutter/build/default/outputs/default/flutter.har`.

5. Rename the HAR file in the `flutter.har.BUILD_TYPE.API` format, where `BUILD_TYPE` indicates `debug`, `release`, or `profile`, and `API` indicates the current SDK version (for example, 11 indicates API version 11). For example, to build a debug version of API version 11, rename the file `flutter.har.debug.11`.

6. Replace the corresponding file in the `flutter_flutter/packages/flutter_tools/templates/app_shared/ohos.tmpl/har/har_product.tmpl/` directory and execute the project again for the modification to take effect.

If you are using DevEco Studio of a Beta version and encounter the error message `must have required property 'compatibleSdkVersion', location: build-profile.json5:17:11` when building the project, refer to the section "Configuring Plugins" in chapter 6 "Creating a Project and Runing Hello World" of *DevEco Studio Environment Setup.docx* to modify the **shell/platform/ohos/flutter_embedding/hvigor/hvigor-config.json5** file.
