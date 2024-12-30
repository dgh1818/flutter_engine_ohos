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


def runGitCommand(command):
  result = subprocess.run(command, capture_output=True, text=True, shell=True)
  if result.returncode != 0:
    raise Exception(f"Git command failed: {result.stderr}")
  return result.stdout.strip()


# 自动更新flutter.har的版本号,把日期加到末尾。如: 1.0.0-20240731
def updateVersion(buildDir):
  filePath = os.path.join(buildDir, "flutter", "oh-package.json5")
  currentDir = os.path.dirname(__file__)
  latestCommit = runGitCommand(f'git -C {currentDir} rev-parse --short HEAD')

  with open(filePath, "r") as sources:
    lines = sources.readlines()

    pattern = r"\d+\.(?:\d+\.)*\d+"
    with open(filePath, "w") as sources:
      for line in lines:
        if "version" in line:
          matches = re.findall(pattern, line)
          print(f'matches = {matches}')
          if matches and len(matches) > 0:
            result = ''.join(matches[0])
            versionArr = result.split("-")
            list = [versionArr[0], latestCommit]
            versionStr = "-".join(list)
            print(f'versionStr = {versionStr}')
            sources.write(re.sub(pattern, versionStr, line))
          else:
            sources.write(line)
        else:
          sources.write(line)


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
  updateVersion(buildDir)
  hvigorwCommand = "hvigorw" if apiInt != 11 else (".%shvigorw" % os.sep)
  runCommand(
      "cd %s && %s clean --mode module " % (buildDir, hvigorwCommand) +
      "-p module=flutter@default -p product=default -p buildMode=%s " % buildType +
      "assembleHar --no-daemon"
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
  parser.add_argument("--ohos_api_int", type=int, default=13, help="Ohos api int. Deprecated.")
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
      os.path.join(options.build_dir, "flutter/build/default/outputs/default/flutter.har"),
      options.output,
  )


if __name__ == "__main__":
  sys.exit(main())
