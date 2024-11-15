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

   For Windows:
   Refer to the section "Compiling for Windows"
   in the [official document](https://github.com/flutter/flutter/wiki/Compiling-the-engine#compiling-for-windows).


2. Configure the file. Specifically, create an empty folder **engine**, create the **.gclient** file in this folder, and edit the file.

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
Starting from version 3.22.0, the engine compilation will by default compile both the `local-engine` and `local-host-engine`. When the SDK specifies local compiled artifacts, it must specify both of these compiled artifacts. For example:

   ```shell
   flutter build hap --target-platform ohos-arm64 --release --local-engine=<DIR>/engine/src/out/ohos_release_arm64/ --local-engine-host=<DIR>/engine/src/out/host_release
   ```

6. Update the code. In the **engine** directory, execute `./ohos -b master`.

- Manual code update method (after completing at least one full `gclient sync`):
   * Navigate directly to the `engine/src/flutter` directory. After ensuring that the current code update does not involve any patches, you may freely edit or switch code branches, and then execute the build command. If patches are involved, you can manually apply the relevant patches or refer to FAQ.7 to batch reverse and reapply patches.


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

6. After modifying the embedding layer code, running `./ohos` does not take effect. You need to modify the timestamp of any file in the C++ layer for the build system to recognize it (e.g., add a space in any C++ file, save it, then remove the space and save again). Re-run the build to trigger the embedding layer to be repackaged.

7. Method to manually reapply patches:

   ```shell
   python engine/src/flutter/attachment/scripts/ohos_reverse_patch.py
   python engine/src/flutter/attachment/scripts/ohos_setup.py
   ```

8. Configure automatic code navigation

   It is recommended to use the `vscode+clangd` plugin (note: it conflicts with C/C++ IntelliSense).
   Example configuration:
   ```json
    "clangd.path": "./src/flutter/buildtools/linux-x64/clang/bin/clangd",
    "clangd.arguments": [
      "--compile-commands-dir=./src/out/ohos_release_arm64/",
      "--query-driver=./src/flutter/buildtools/linux-x64/clang/bin/clang++"
    ],
    "C_Cpp.intelliSenseEngine": "disabled",
   ```

9. `gclient sync` error on Windows

   Key error message: `'A required privilege is not held by the client'`

   Solution: Go to Settings -> Developer Options -> Enable "Developer Mode"


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

If you are using DevEco Studio of a Beta version and encounter the error message `must have required property 'compatibleSdkVersion', location: build-profile.json5:17:11` when building the project, refer to the section "Configuring Plugins" in chapter 6 "Creating a Project and Runing Hello World" of *DevEco Studio Environment Setup.docx* to modify the **shell/platform/ohos/flutter_embedding/hvigor/hvigor-config.json5** file.
