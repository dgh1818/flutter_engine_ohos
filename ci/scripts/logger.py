# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

import sys


class Logger:
    """Simple logging functions"""

    @staticmethod
    def info(message: str):
        print(f"[INFO] {message}")

    @staticmethod
    def warn(message: str):
        print(f"[WARN] {message}")

    @staticmethod
    def error(message: str):
        print(f"[ERROR] {message}", file=sys.stderr)

    @staticmethod
    def step(message: str):
        print(f"\n{'=' * 40}")
        print(message)
        print(f"{'=' * 40}\n")
