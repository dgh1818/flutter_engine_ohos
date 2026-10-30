# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
"""文件扫描器"""
import os
from pathlib import Path
from typing import List


class ArkTSFileScanner:
    """ArkTS 文件扫描器"""

    # ArkTS 文件扩展名
    ARKTS_EXTENSIONS = {'.ets', '.ts'}

    def __init__(self, target_path: str):
        """
        初始化

        Args:
            target_path: 目标路径（文件或目录）
        """
        self.target_path = Path(target_path)

    def scan(self) -> List[Path]:
        """
        扫描 ArkTS 文件

        Returns:
            ArkTS 文件路径列表
        """
        files = []

        if self.target_path.is_file():
            if self._is_arkts_file(self.target_path):
                files.append(self.target_path)
        elif self.target_path.is_dir():
            for root, _, filenames in os.walk(self.target_path):
                for filename in filenames:
                    file_path = Path(root) / filename
                    if self._is_arkts_file(file_path):
                        files.append(file_path)

        return files

    def _is_arkts_file(self, file_path: Path) -> bool:
        """判断是否是 ArkTS 文件"""
        return file_path.suffix.lower() in self.ARKTS_EXTENSIONS
