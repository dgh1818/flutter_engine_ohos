#!/usr/bin/env python3
# coding=utf-8
#
# Copyright (c) 2021-2024 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

import os
import logging
import argparse
import platform
from pathlib import Path
from utils import getArch, getEnginePath, runGitCommand, upload

engine_path = getEnginePath()

logging.basicConfig()
log = logging.getLogger()
log.setLevel(logging.INFO)


def main():
  parser = argparse.ArgumentParser(
      prog='upload_dart_sdk', description='upload dart sdk', epilog='upload dart sdk'
  )
  parser.add_argument('-t', '--tag')
  parser.add_argument('-f', '--file')
  parser.add_argument('--arch', type=str, choices=['x64', 'arm64'], default="x64")
  args = parser.parse_args()
  osArch = args.arch
  osName = platform.system().lower()
  file_name = f'dart-sdk-{osName}-{osArch}.zip'
  if args.file:
    file_path = args.file
  else:
    file_path = Path(engine_path).joinpath(file_name).__str__()
  flutter_path = Path(engine_path).joinpath('src/flutter').__str__()
  if args.tag:
    version = args.tag
  else:
    version = runGitCommand(f'git -C {flutter_path} rev-parse HEAD')
  log.info(version)
  upload(version, file_path)


if __name__ == "__main__":
  exit(main())
