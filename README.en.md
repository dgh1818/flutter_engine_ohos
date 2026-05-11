Flutter SDK Repository
======================

## Description

This repository is the **OpenHarmony** adaptation of the **[Flutter SDK](https://github.com/flutter/flutter)** and **[Flutter Engine](https://github.com/flutter/flutter/tree/master/engine)**, maintained by the OpenHarmony-Flutter team. It enables developers to use the familiar Flutter technology stack to build OpenHarmony applications, and to build the Flutter Engine with OpenHarmony support from the included source code.

> This branch is based on Flutter version [3.41.9](https://github.com/flutter/flutter/commit/00b0c91f06209d9e4a41f71b7a512d6eb3b9c694).
>
> For information on version planning and branch strategy, see: [Flutter OH Version Planning and Branch Strategy](https://gitcode.com/openharmony-tpc/flutter_flutter/wiki/Flutter-OH%E7%89%88%E6%9C%AC%E6%BC%94%E8%BF%9B%E8%A7%84%E5%88%92%E5%92%8C%E5%88%86%E6%94%AF%E7%AD%96%E7%95%A5.md)

## Repository Structure

Starting from Flutter version 3.41.9, the Engine source code has been merged into the `engine/` directory of this repository. There is no need to clone the Engine repository separately.

```json
flutter_flutter/                 # Root Directory
├── packages/                    # Flutter SDK framework
│   ├── flutter/                 # Flutter framework
│   ├── flutter_tools/           # Flutter CLI tools
│   └── ...
├── engine/                      # Flutter Engine
└── ...
```

## Guides

### Application Development

- [Flutter OH Development Documentation](https://gitcode.com/openharmony-tpc/flutter_samples/blob/master/README.en.md)
- [Flutter OH Environment Setup Guide](https://gitcode.com/openharmony-tpc/flutter_samples/blob/master/ohos/docs/03_environment/OpenHarmony-flutter-environment-setup.md)
- [Flutter OH Application Build Guide](https://gitcode.com/openharmony-tpc/flutter_samples/blob/master/ohos/docs/04_development/OpenHarmony-flutter%E5%BA%94%E7%94%A8%E6%9E%84%E5%BB%BA%E6%8C%87%E5%AF%BC.md)
- [Flutter OH Third-party Library Adaptation List](https://gitcode.com/OpenHarmony-Flutter/docs/blob/main/ThirdpartyLibrarites.en.md)
- [Flutter Official Development Guide and API Documentation](https://docs.flutter.dev/)

### Building the Engine

- [Flutter OH Engine Build Guide](https://gitcode.com/openharmony-tpc/flutter_samples/blob/master/ohos/docs/03_environment/Flutter-OH-engine%E6%9E%84%E5%BB%BA%E6%8C%87%E5%AF%BC.md)

## Upgrade Guide

see: [Flutter OH Version Upgrade Guide](https://gitcode.com/openharmony-tpc/flutter_samples/blob/master/ohos/docs/10_appendix/Flutter-OH%E7%89%88%E6%9C%AC%E5%8D%87%E7%BA%A7%E6%8C%87%E5%AF%BC.md)

## Supported Commands

List of commands adapted for OpenHarmony development:

| Command Name | Command Description              | Usage Instructions                                           |
| ------------ | -------------------------------- | ------------------------------------------------------------ |
| doctor       | environment detection            | `flutter doctor`                                             |
| config       | environment configuration        | `flutter config --<key> <value>`                             |
| create       | Create a new project             | `flutter create --platforms [ohos,android,ios] --org <org> <appName>` |
| create       | Create module template           | `flutter create -t module <module_name>`                     |
| create       | Create plugin template           | `flutter create -t plugin --platforms [ohos,android,ios] <plugin_name>` |
| create       | Create plugin_ffi template       | `flutter create -t plugin_ffi --platforms [ohos,android,ios] <plugin_name>` |
| devices      | Connected device discovery       | `flutter devices`                                            |
| install      | application installation         | `flutter install -t <deviceId> <hap_file_path>`              |
| assemble     | resource packaging               | `flutter assemble`                                           |
| build        | Test application build           | `flutter build hap --debug [--target-platform ohos-arm64] [--local-engine=<ohos-compatible debug engine path>]` |
| build        | Formal application build         | `flutter build hap --release [--target-platform ohos-arm64] [--local-engine=<ohos-compatible release engine path>]` |
| run          | application run                  | `flutter run [--local-engine=<ohos-compatible engine path>]` |
| attach       | debug mode                       | `flutter attach`                                             |
| screenshot   | screenshot                       | `flutter screenshot`                                         |
| pub          | Obtains the dependencies.        | `flutter pub get`                                            |
| clean        | Clears the project dependencies. | `flutter clean`                                              |
| cache        | Clears global cache data.        | `flutter pub cache clean`                                    |

## FAQ

1. After switching to FLUTTER_STORAGE_BASE_URL, you need to delete the \<flutter\>/bin/cache directory and execute Flutter clean in the project before running it again.

2. If you encounter the error: `The SDK license agreement is not accepted`, please execute the following command and compile again:

   ```shell
   ./ohsdkmgr install ets:9 js:9 native:9 previewer:9 toolchains:9 --sdk-directory='/home/xc/code/sdk/ohos-sdk/' --accept-license
   ```

3. If you are using the Beta version of DevEco Studio and encounter the error "must have required property 'compatibleSdkVersion', location: demo/ohos/build-profile.json5:17:11" when building the project, please refer to the [DevEco Studio Configuration File](vscode-file://vscode-app/e:/Microsoft VS Code/resources/app/out/vs/code/electron-browser/workbench/workbench.html) documentation, specifically the section [Project-level build-profile.json5 File → products](vscode-file://vscode-app/e:/Microsoft VS Code/resources/app/out/vs/code/electron-browser/workbench/workbench.html), to configure the `compatibleSdkVersion` property.

4. If you are prompted with an installation error: `fail to verify pkcs7 file`, please execute the command

   ```shell
   hdc shell param set persist.bms.ohCert.verify true
   ```

5. Linux virtual machine cannot directly discover OpenHarmony devices through hdc

   Solution: In the Windows host, open the hdc server.The specific instructions are as follows:

   ```shell
   hdc kill
   hdc -s serverIP:8710 -m
   ```

   Configure environment variables in Linux:

   ```shell
   HDC_SERVER=<serverIP>
   HDC_SERVER_PORT=8710
   ```

   After the configuration is completed, the flutter sdk can complete the device connection through the hdc server. You can also refer to [official guidance](https://docs.openharmony.cn/pages/v5.0/zh-cn/device-dev/subsystems/subsys-toolchain -hdc-guide.md/#hdc-client%E5%A6%82%E4%BD%95%E8%BF%9C%E7%A8%8B%E8%AE%BF%E9%97%AEhdc-server) .

6. An error occurred when building the Hap task: Error: The hvigor depends on the npmrc file. Configure the npmrc file first.

   Please create a file `.npmrc` in the user home directory `~`. This configuration can also refer to [DevEco Studio Official Documentation](https://developer.harmonyos.com/cn/docs/documentation/doc-guides-V3/environment_config-0000001052902427-V3). Edit the content as follows:

   ```json
   registry=https://repo.huaweicloud.com/repository/npm/
   @ohos:registry=https://repo.harmonyos.com/npm/
   ```

7. Symptom Logs are lost during log query.
   Solution：Disable global logs and enable only logs in your domain

   ```shell
   # Step 1: Disable log printing for all domains (some special logs cannot be disabled)
   hdc shell hilog -b X
   # Step 2: Only enable logs for your own domain
   hdc shell hilog <level> -D <domain>
   # Where <level> is the log print level: D/I/W/E/F, <domain> is the number before Tag
   # Example:
   # To print logs for A00000/XComFlutterOHOS_Native, set: hdc shell hilog -b D -D A00000
   # Note: The above settings will be lost after machine restart. If you want to continue using them, you need to set them again.
   ```

8. If the debug signature application cannot be started on API 11BETA1, it can be resolved by changing the signature to an official signature or opening the developer mode on the mobile terminal (steps: Settings -> General -> Developer mode).

9. If `Invalid CEN header (invalid zip64 extra data field size)` is abnormal, please replace the JDK version, see [JDK-8313765](https://bugs.openjdk.org/browse/JDK-8313765).

10. An error occurs when running a debug version of the Flutter application on a HarmonyOS device (release and profile versions are normal)

    1. Error message: `Error while initializing the Dart VM: Wrong full snapshot version, expected '8af474944053df1f0a3be6e6165fa7cf' found 'adb4292f3ec25074ca70abcd2d5c7251'`
    2. Solution: Perform the following actions in sequence
       1. Set environment variables `export FLUTTER_STORAGE_BASE_URL=https://flutter-ohos.obs.cn-south-1.myhuaweicloud.com`
       2. Delete the cache in the<Flutter>/bin/cache directory
       3. Execute `fluent clean` to clear the project compilation cache
       4. Execute `flutter run -d $DEVICE --debug`
    3. Additional information: If a similar error occurs while running Android or iOS, you can also try restoring the environment variable FLUTTER_STORAGE_BASE_URL , clearing the cache, and then running again.

11. After the ROM update of Beta 2 version, it no longer supports requesting anonymous memory with execution permission, resulting in debug crashing.

    1. Solution: Update flutter_flutter to a version after a44b8a6d (2024-07-25).

    2. Key logs:

       ```json
           #20 at attachToNative (oh_modules/.ohpm/@ohos+flutter_ohos@g8zhdaqwu8gotysbmqcstpfpcpy=/oh_modules/@ohos/flutter_ohos/src/main/ets/embedding/engine/FlutterNapi.ets:78:32)
           #21 at attachToNapi (oh_modules/.ohpm/@ohos+flutter_ohos@g8zhdaqwu8gotysbmqcstpfpcpy=/oh_modules/@ohos/flutter_ohos/src/main/ets/embedding/engine/FlutterEngine.ets:144:5)
           #22 at init (oh_modules/.ohpm/@ohos+flutter_ohos@g8zhdaqwu8gotysbmqcstpfpcpy=/oh_modules/@ohos/flutter_ohos/src/main/ets/embedding/engine/FlutterEngine.ets:133:7)
       ```

12. Build Hap command directly execute `flutter build hap`, no longer need `--local-engine` parameter, directly from the cloud to obtain the compilation product

13. After the environment is configured, the system crashes when the flutter command is executed.

    1. Solution：Add git environment variable configuration in windows environment.

       ```shell
       export PATH=<git path>/cmd:$PATH
       ```

14. If `flutter pub cache clean` is executed normally, `flutter clean` will report an error. If update command is executed according to the error message, it has no effect.

    1. Solution：To avoid this problem, comment out the configuration in the build.json5 file.

    2. Error message:

       ```json
        #Parse ohos module. json5 error: Exception: Can not found module.json5 at
        #D:\pub_cache\git\flutter_packages-b00939bb44d018f0710d1b080d91dcf4c34ed06\packages\video_player\video_player_ohos\ohossrc\main\module.json5.
        #You need to update the Flutter plugin project structure.
        #See
        #https://gitcode.com/openharmony-tpc/flutter_samples/blob/master/ohos/docs/09_specifications/update-flutter-plugin-structure.md
       ```

15. An error message indicating path verification occurs when `flutter build hap` is executed.

    1. Solution:

       - Open the ohos-project-build-profile-schema.json file in deveco installation path D:\DevEco Studio\tools\hvigor\hvigor-ohos-plugin\res\schemas.
       - Find the line containing: "pattern": "^(\\./|\\.\\./)[\\s\\S]+$" in the file and delete it.

    2. Error message:

       ```json
        #hvigor  ERROR: Schema validate failed.
        #        Detail: Please check the following fields.
        #instancePath: 'modules[1].scrPath',
        #keyword: 'pattern'
        #params: { pattern:'^(\\./|\\.\\./)[\\s\\S]+$' },
        #message: 'must match pattern "^(\\./|\\.\\./)[\\s\\S]+$"',
        #location: 'D:/work/videoplayerdemo/video_cannot_stop_at_background/ohos/build-profile.json:42:146'
       ```

16. Execute `flutter build hap` report an error.

    1. Solution: Open the core-module-model-impl.js file in the DevEco installation path D:\DevEco Studio\tools\hvigor\hvigor-ohos-plugin\src\model\module, and modify the findBelongProjectPath method (requires administrator privileges, you can save as and replace):

       ```
        findBelongProjectPath(e) {
          if (e === path_1.default.dirname(e)) {
             return this.parentProject.getProjectDir()
          }
        }
       ```

    2. Error message:

       ```json
       # hvigor  ERROR: Cannot find belonging project path for module at D:\.
       # hvigor  ERROR:  BUILD FAILED in 2s 556ms.
       #Running Hvigor task assembleHap...
       #Oops; flutter has exited unexpectedly: "ProcessException: The command failed
       #  <Command: hvigorw --mode module -p module=video_player_ohos@default -p product=default assmbleHar --no-daemon"
       #A crash report has been written to D:\work\videoplayerdemo\video_cannot_stop_at_background\flutter_03.log.
       ```

17. DevEco-Studio(5.0.3.600 Beta3), Windows version compilation error for Flutter app

    1. Solution: Update flutter_flutter to version after c6fbac2b (2024-08-09).

    2. Key logs:

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

18. Executing `flutter clean` in .ohos's project reported an error, and then executing `flutter pub get` also reported an error.

    1. Solution：Delete the .ohos folder and execute `flutter pub get` again

    2. Error message：

       ```json
          Oops; flutter has exited unexpectedly: "PathNotFoundException: Cannot open file, path = 'D:\code\.ohos\build-profile.json5' (OS Error: The system cannot find the file specified., error = 2)".
          A crash report has been written to D:\code\flutter_01.log.
       ```

19. White screen, crashes, or similar issues occur when running the emulator.

    1. The emulator only supports Mac (arm64) and does not yet support Mac (x86) or Windows.
    2. Since the emulator does not currently support Vulkan, please try following the steps in section 2.1. Disable Impeller and try again.

20. Compilation or runtime failure in Flutter profile mode

    1. Please add the buildModeSet field in the ohos project build_profile.json5. You can refer to [complex_layout](./dev/benchmarks/complex_layout/ohos/build-profile.json5).

    2. Error message:

       ```json
       hvigor ERROR: Build mode 'profile' used in command line is not declared in buildModeSet in /xxx/example/ohos/build-profile.json5.
       ```

> [More FAQs](https://gitcode.com/openharmony-tpc/flutter_samples/blob/master/ohos/docs/08_FAQ/README.md)

## Contributing

If you would like to contribute code to Flutter-OH, please refer to the [Flutter-OH Contributor PR Guide](https://gitcode.com/openharmony-tpc/flutter_flutter/wiki/Flutter_OH%E4%BB%93%E5%BA%93%E4%BB%A3%E7%A0%81%E5%90%88%E5%85%A5%E6%B5%81%E7%A8%8B.md) for detailed contribution steps and guidelines.

## Communication

- **Issue Feedback:** Submit issues to the [Flutter Framework Repository](https://gitcode.com/openharmony-tpc/flutter_flutter/issues) or related third-party libraries.

