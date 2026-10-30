# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
"""Code Linter 执行器"""
import glob
import json
import os
import subprocess
import sys
from typing import Any, Dict, Optional


class CodeLinterRunner:
    """Code Linter 执行器"""

    def __init__(self, codelinter_path: str, node_path: Optional[str] = None):
        """
        初始化

        Args:
            codelinter_path: codelinter 路径（可以是 .bat 文件或 index.js）
            node_path: Node.js 路径（当 codelinter_path 是 index.js 时需要）
        """
        self.codelinter_path = codelinter_path
        self.node_path = node_path or self._find_node()

    def _find_node(self) -> str:
        """查找 Node.js 路径"""
        # 尝试从环境变量查找
        node_path = os.environ.get('NODE_PATH')
        if node_path:
            return node_path

        # 根据平台设置常见路径
        if sys.platform == 'win32':
            common_paths = [
                r"D:\DevEco Studio\tools\node\node.exe",
                r"C:\Program Files\nodejs\node.exe",
                r"C:\Program Files (x86)\nodejs\node.exe",
            ]
        else:
            # Linux/macOS 常见路径
            common_paths = [
                "/usr/local/bin/node",
                "/usr/bin/node",
                "/opt/node/bin/node",
                os.path.expanduser("~/.nvm/versions/node/*/bin/node"),  # nvm 安装
            ]

        for path in common_paths:
            # 处理通配符路径（如 nvm）
            if '*' in path:
                matched = glob.glob(path)
                if matched:
                    # 使用最新版本
                    path = sorted(matched)[-1]
                else:
                    continue

            if os.path.exists(path):
                return path

        # 尝试 which/where 命令
        try:
            if sys.platform == 'win32':
                result = subprocess.run(['where', 'node'], capture_output=True, text=True, shell=False)
            else:
                result = subprocess.run(['which', 'node'], capture_output=True, text=True, shell=False)

            if result.returncode == 0:
                return result.stdout.strip().split('\n')[0]
        except Exception:
            pass

        return 'node'  # 默认使用系统 PATH 中的 node

    def run_linter(self,
                   target_path: str,
                   config_path: Optional[str] = None,
                   output_format: str = 'json',
                   output_file: Optional[str] = None,
                   incremental: bool = False,
                   error_level: str = 'error') -> Dict[str, Any]:
        """
        运行 codelinter

        Args:
            target_path: 目标路径
            config_path: 配置文件路径
            output_format: 输出格式 (json/xml/html/default)
            output_file: 输出文件路径
            incremental: 是否增量检查
            error_level: 错误级别

        Returns:
            解析后的 JSON 结果
        """
        # 构建命令
        if self.codelinter_path.endswith('.js'):
            # 路径 B：IDE 内置插件
            cmd = [self.node_path, self.codelinter_path]
        elif self.codelinter_path.endswith('.bat') and sys.platform != 'win32':
            # Linux/macOS 下运行 .bat 文件，尝试找到对应的 shell 脚本
            sh_path = self.codelinter_path.replace('.bat', '.sh')
            if os.path.exists(sh_path):
                cmd = ['/bin/bash', sh_path]
            else:
                # 尝试直接执行（如果有 shebang）
                cmd = [self.codelinter_path]
        else:
            # 路径 A：独立 CLI
            cmd = [self.codelinter_path]

        # 添加参数
        if config_path and os.path.exists(config_path):
            cmd.extend(['-c', config_path])

        if output_format:
            cmd.extend(['-f', output_format])

        if output_file:
            cmd.extend(['-o', output_file])

        if incremental:
            cmd.append('-i')

        if error_level:
            cmd.extend(['-e', error_level])

        # 目标路径作为位置参数放在最后
        cmd.append(target_path)

        print(f"执行命令: {' '.join(cmd)}")

        try:
            # 执行命令
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                encoding='utf-8',
                errors='replace'
            )

            # 解析输出
            if output_file and os.path.exists(output_file):
                with open(output_file, 'r', encoding='utf-8') as f:
                    return json.load(f)
            elif result.stdout:
                try:
                    return json.loads(result.stdout)
                except json.JSONDecodeError:
                    return {
                        'stdout': result.stdout,
                        'stderr': result.stderr,
                        'returncode': result.returncode
                    }
            else:
                return {
                    'stdout': result.stdout,
                    'stderr': result.stderr,
                    'returncode': result.returncode
                }

        except Exception as e:
            return {
                'error': str(e),
                'returncode': -1
            }
