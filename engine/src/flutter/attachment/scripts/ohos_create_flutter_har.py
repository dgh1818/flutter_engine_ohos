#!/usr/bin/env python3
#
# Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Create a HAR incorporating all the components required to build a Flutter application"""

import argparse
import logging
import os
import re
import shutil
import subprocess
import sys


HAR_CONFIG_TEMPLATE = """
{
  "app": {
    "signingConfigs": [],
    "products": [
      {
        "name": "default",
        "signingConfig": "default",
        "compileSdkVersion": "%s",
        "compatibleSdkVersion": "%s",
        "runtimeOS": "HarmonyOS",
      }
    ],
    "buildModeSet": [
      {
        "name": "debug",
      },
      {
        "name": "release"
      },
      {
        "name": "profile"
      },
    ]
  },
  "modules": [
    {
      "name": "flutter",
      "srcPath": "./flutter"
    }
  ]
}
"""


# 更新har的配置文件，指定编译使用的api版本
def updateConfig(buildDir, apiInt):
    apiStr = "4.1.0(11)" if apiInt == 11 else "4.0.0(10)"
    jsonFile = os.path.join(buildDir, "build-profile.json5")
    with open(jsonFile, "w", encoding="utf-8") as file:
        file.write(HAR_CONFIG_TEMPLATE % (apiStr, apiStr))


# 执行命令
def runCommand(command, checkCode=True, timeout=None):
    logging.info("runCommand start, command = %s" % (command))
    code = subprocess.Popen(command, shell=True).wait(timeout)
    if code != 0:
        logging.error("runCommand error, code = %s, command = %s" % (code, command))
        if checkCode:
            exit(code)
    else:
        logging.info("runCommand finish, code = %s, command = %s" % (code, command))


# 编译har文件，通过hvigorw的命令行参数指定编译类型(debug/release/profile)
def buildHar(buildDir, apiInt, buildType):
    updateConfig(buildDir, apiInt)
    runCommand(
        "cd %s && .%shvigorw clean --mode module " % (buildDir, os.sep)
        + "-p module=flutter@default -p product=default -p buildMode=%s " % buildType
        + "assembleHar --no-daemon"
    )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--embedding_src", help="Path of embedding source code.")
    parser.add_argument("--build_dir", help="Path to build.")
    parser.add_argument(
        "--build_type",
        choices=["debug", "release", "profile"],
        help="Type to build flutter.har.",
    )
    parser.add_argument("--output", help="Path to output flutter.har.")
    parser.add_argument("--native_lib", action="append", help="Native code library.")
    parser.add_argument("--ohos_abi", help="Native code ABI.")
    parser.add_argument(
        "--ohos_api_int", type=int, choices=[11], help="Ohos api int."
    )
    options = parser.parse_args()
    # copy source code
    if os.path.exists(options.build_dir):
        shutil.rmtree(options.build_dir)
    shutil.copytree(options.embedding_src, options.build_dir)

    # copy so files
    for file in options.native_lib:
        dir_name, full_file_name = os.path.split(file)
        targetDir = os.path.join(options.build_dir, "flutter/libs", options.ohos_abi)
        if not os.path.exists(targetDir):
            os.makedirs(targetDir)
        shutil.copyfile(
            file,
            os.path.join(targetDir, full_file_name),
        )
    buildHar(options.build_dir, options.ohos_api_int, options.build_type)
    shutil.copyfile(
        os.path.join(
            options.build_dir, "flutter/build/default/outputs/default/flutter.har"
        ),
        options.output,
    )


if __name__ == "__main__":
    sys.exit(main())