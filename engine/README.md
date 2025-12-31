# Flutter Engine

原始仓来源：https://github.com/flutter/engine

## 仓库说明：
本仓库是基于flutter官方engine仓库拓展，可构建支持在OpenHarmony设备上运行的flutter engine程序。

## 构建说明：

* 构建环境：
1. 目前支持在Linux与MacOS中构建，Windows环境中支持构建gen_snapshot;
2. 请确保当前构建环境可以访问 `DEPS_ohos` 配置文件中 `allowed_hosts` 字段的URL列表。

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


2. 配置engine开发环境：
   a) 在 [flutter_flutter](../) 目录下新建 .gclient 文件
   b) 复制 engine/scripts/*.gclient 到 .gclient

      googlers: 复制 `rbe.gclient` 到 [RBE](https://github.com/flutter/flutter/blob/master/engine/src/flutter/docs/rbe/rbe.md)

      其他: 复制 `standard.gclient`
      
      ohos: 复制 `ohos.gclient`

3. 同步代码：在 [flutter_flutter](../) 目录中执行 `gclient sync` 命令；这里会同步engine源码、官方packages仓，还有执行ohos_setup任务；


4. 同步代码完成后，在[flutter_flutter](../) 目录下执行以下python命令:

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

5. 开始构建：在 `engine` 目录，执行`./ohos`，即可开始构建支持ohos设备的flutter engine。
   从3.22.0版本开始，engine编译会默认编译local-engine以及local-host-engine, sdk在指定本地编译产物时需要同时指定这两个编译产物，例如：
   ```shell
   flutter build hap --target-platform ohos-arm64 --release --local-engine=<DIR>/engine/src/out/ohos_release_arm64/ --local-engine-host=<DIR>//engine/src/out/host_release
   ```
