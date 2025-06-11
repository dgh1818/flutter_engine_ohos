#!/usr/bin/env python3
# coding=utf-8
#
# Copyright (c) 2021-2024 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

import argparse
import os
import zipfile
import platform
import logging
from pathlib import Path

log = logging.getLogger(__name__)


def genZipFile():
  parser = argparse.ArgumentParser()
  parser.add_argument('--arch', type=str, choices=['x64', 'arm64'], default="x64")
  args = parser.parse_args()
  osArch = args.arch
  osName = platform.system().lower()
  suffix = "_arm64" if osArch == "arm64" else ""
  engine_project_root_dir = Path(os.path.realpath(__file__)).parents[4]
  dart_sdk_path = engine_project_root_dir.joinpath(f"src/out/host_release{suffix}/dart-sdk")
  host_release_path = engine_project_root_dir.joinpath(f"src/out/host_release{suffix}/")

  print(dart_sdk_path)
  with zipfile.ZipFile(engine_project_root_dir.joinpath(f'dart-sdk-{osName}-{osArch}.zip'), 'w',
                       zipfile.ZIP_DEFLATED) as zipf:
    for entry in dart_sdk_path.rglob("*"):
      if (entry.is_dir()):
        continue
      else:
        print(entry.relative_to(host_release_path))
        zipf.write(entry, entry.relative_to(host_release_path))


def main():
  genZipFile()


if __name__ == "__main__":
  exit(main())
