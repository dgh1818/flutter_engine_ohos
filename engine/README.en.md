# Flutter Engine

Source of the original repository: https://github.com/flutter/engine

## Repository Description
This repository is an extension of the Flutter engine repository. It enables Flutter engine to run on OpenHarmony devices.

## How to Build

* Build environment:
1. Linux or macOS that support Flutter engine; Windows that supports **gen_snapshot**.
2. Access to the **allowed_hosts** field in the DEPS_ohos file.

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


2. Configure the engine development environment by copying one of the `engine/scripts/*.gclient` files to the [flutter_flutter](../) root directory as `.gclient`

   a) Googlers: copy `rbe.gclient` to enable faster builds with [RBE](https://github.com/flutter/flutter/blob/master/engine/src/flutter/docs/rbe/rbe.md)

   b) Everyone else: copy `standard.gclient`

   c) For ohos: copy `ohos.gclient`

3. Synchronize the code. In the **flutter_flutter** directory, execute `gclient sync`. The engine source code and packages repository will be synchronized, and the **ohos_setup** task will be executed.

4. After the synchronization is complete, execute the following python commands in the **flutter_flutter** directory:

   ```shell
   Linux：
   sed -i 's/vpython3/python3/g' ./engine/src/flutter/tools/gn
   sed -i 's/vpython3/python3/g' ./engine/src/.gn

   MacOS：
   sed -i '' 's/vpython3/python3/g' ./engine/src/flutter/tools/gn
   sed -i '' 's/vpython3/python3/g' ./engine/src/.gn
   ```

   ```shell
   cd engine/src
   python3 ./flutter/tools/pub_get_offline.py
   ```

5. Start building. In the **engine** directory, execute `./ohos` to start building the Flutter engine that supports ohos devices.
Starting from version 3.22.0, the engine compilation will by default compile both the `local-engine` and `local-host-engine`. When the SDK specifies local compiled artifacts, it must specify both of these compiled artifacts. For example:

   ```shell
   flutter build hap --target-platform ohos-arm64 --release --local-engine=<DIR>/engine/src/out/ohos_release_arm64/ --local-engine-host=<DIR>/engine/src/out/host_release
   ```
