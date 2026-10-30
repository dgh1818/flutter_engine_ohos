#!/usr/bin/env python
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
"""ArkTS Linter 入口脚本（向后兼容）"""
import sys
from pathlib import Path

# 添加包路径到 sys.path
package_dir = Path(__file__).parent
if str(package_dir) not in sys.path:
    sys.path.insert(0, str(package_dir))

from arkts_linter import main

if __name__ == '__main__':
    main()
