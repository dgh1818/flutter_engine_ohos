# Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_KHZG file.

"""Create a HAR incorporating all the components required to build a Flutter application"""

import argparse
import logging
import os
import re
import shutil
import subprocess
import sys
import json

def runGitCommand(command):
  result = subprocess.run(command, capture_output=True, text=True, shell=True)
  if result.returncode != 0:
    raise Exception(f"Git command failed: {result.stderr}")
  return result.stdout.strip()


# 更新har中的oh-package.json5,把commitid追加到版本号末尾，如：1.0.0-1a3a3617f2
def updateVersion(options):
  buildDir = options.build_dir
  buildType = options.build_type
  harName = options.har_name
  filePath = os.path.join(buildDir, harName, "oh-package.json5")
  currentDir = os.path.dirname(__file__)
  latestCommit = runGitCommand(f'git -C {currentDir} rev-parse --short HEAD')

  with open(filePath, "r") as sources:
    config = json.load(sources)

  # update version
  pattern = r"\d+\.(?:\d+\.)*\d+"
  matches = re.findall(pattern, config['version'])
  print(f'matches = {matches}')
  if matches and len(matches) > 0:
    result = ''.join(matches[0])
    versionArr = result.split("-")
    list = [versionArr[0], latestCommit]
    versionStr = "-".join(list)
    config['version'] = versionStr

  # update name
  if options.append_abi is True:
    name = config['name']
    appendStr = options.ohos_abi.replace("-", "_")
    config['name'] = f'{name}_{appendStr}'

  with open(filePath, "w") as sources:
    json.dump(config, sources)


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
def buildHar(options):
  buildDir = options.build_dir
  apiInt = options.ohos_api_int
  buildType = options.build_type
  harName = options.har_name
  updateVersion(options)
  hvigorwCommand = "hvigorw" if apiInt != 11 else (".%shvigorw" % os.sep)
  runCommand(
      "cd %s && %s clean --mode module " % (buildDir, hvigorwCommand)
      + "-p module=%s@default -p product=default -p buildMode=%s " % (harName, buildType)
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
      "--ohos_api_int", type=int, default=13, help="Ohos api int. Deprecated."
  )
  parser.add_argument("--har_name", help="Har file name.", default="flutter")
  parser.add_argument("--append_abi", type=bool, default=False, help="Append arch after name in oh-package.json5.")
  options = parser.parse_args()
  # copy source code
  if os.path.exists(options.build_dir):
    shutil.rmtree(options.build_dir)
  shutil.copytree(options.embedding_src, options.build_dir)

  # copy so files
  harName = options.har_name
  if options.native_lib is not None:
    for file in options.native_lib:
      dir_name, full_file_name = os.path.split(file)
      targetDir = os.path.join(options.build_dir, f"{harName}/libs", options.ohos_abi)
      if not os.path.exists(targetDir):
        os.makedirs(targetDir)
      shutil.copyfile(
          file,
          os.path.join(targetDir, full_file_name),
      )
  buildHar(options)
  outputFile = f'{harName}/build/default/outputs/default/{harName}.har'
  shutil.copyfile(os.path.join(options.build_dir, outputFile), options.output)


if __name__ == "__main__":
  sys.exit(main())
