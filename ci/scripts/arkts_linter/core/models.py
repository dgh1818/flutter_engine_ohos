# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
"""数据模型定义"""
from dataclasses import dataclass, field
from enum import Enum
from typing import List, Optional


class SeverityLevel(Enum):
    """问题严重程度"""
    CRITICAL = "严重"
    NORMAL = "一般"
    HINT = "提示"


class RuleLevel(Enum):
    """规则级别"""
    CRITICAL = "严重"
    NORMAL = "一般"
    HINT = "提示"


@dataclass
class LintIssue:
    """Lint 问题"""
    file_path: str
    line: int
    column: int
    rule_id: str
    rule_name: str
    severity: str
    message: str
    suggestion: str
    cwe: str = ""


@dataclass
class LintReport:
    """Lint 报告"""
    project_path: str
    scan_time: str
    total_files: int
    total_issues: int = 0
    error_count: int = 0
    warning_count: int = 0
    suggestion_count: int = 0
    issues: List[LintIssue] = field(default_factory=list)
    rule_statistics: dict = field(default_factory=dict)
    gate_result: str = "PASS"
    gate_message: str = "所有检查通过"

    @property
    def critical_count(self) -> int:
        return self.error_count

    @property
    def normal_count(self) -> int:
        return self.warning_count

    @property
    def hint_count(self) -> int:
        return self.suggestion_count
