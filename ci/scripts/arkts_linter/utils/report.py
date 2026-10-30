# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
"""报告生成器"""
import json
from dataclasses import asdict
from datetime import datetime
from typing import Dict, List

from ..core.models import LintIssue, LintReport, RuleLevel


class ReportGenerator:
    """报告生成器"""

    @staticmethod
    def generate_report(project_path: str,
                        issues: List[LintIssue],
                        gate_rules: Dict[str, Dict]) -> LintReport:
        """生成报告"""
        # 统计
        error_count = sum(1 for i in issues if i.severity == RuleLevel.CRITICAL.value)
        warning_count = sum(1 for i in issues if i.severity == RuleLevel.NORMAL.value)
        suggestion_count = sum(1 for i in issues if i.severity == RuleLevel.HINT.value)

        # 规则统计
        rule_stats = {}
        for issue in issues:
            rule_stats[issue.rule_name] = rule_stats.get(issue.rule_name, 0) + 1

        # 门禁判断
        gate_result = "PASS"
        gate_message = "所有检查通过"

        if error_count > 0:
            gate_result = "FAIL"
            gate_message = f"发现 {error_count} 个严重问题，门禁不通过"
        elif warning_count > 10:  # 阈值可配置
            gate_result = "FAIL"
            gate_message = f"发现 {warning_count} 个一般问题，超过阈值(10)，门禁不通过"

        return LintReport(
            project_path=project_path,
            scan_time=datetime.now().isoformat(),
            total_files=len(set(i.file_path for i in issues)),
            total_issues=len(issues),
            error_count=error_count,
            warning_count=warning_count,
            suggestion_count=suggestion_count,
            issues=issues,
            rule_statistics=rule_stats,
            gate_result=gate_result,
            gate_message=gate_message
        )

    @staticmethod
    def save_json_report(report: LintReport, output_path: str):
        """保存 JSON 报告"""
        report_dict = asdict(report)

        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(report_dict, f, ensure_ascii=False, indent=2)

        print(f"报告已保存: {output_path}")

    @staticmethod
    def print_summary(report: LintReport):
        """打印摘要"""
        print("\n" + "=" * 60)
        print("ArkTS 门禁检查报告")
        print("=" * 60)
        print(f"项目路径: {report.project_path}")
        print(f"扫描时间: {report.scan_time}")
        print(f"检查文件数: {report.total_files}")
        print("-" * 60)
        print(f"严重问题: {report.error_count}")
        print(f"一般问题: {report.warning_count}")
        print(f"提示问题: {report.suggestion_count}")
        print(f"总问题数: {report.total_issues}")
        print("-" * 60)
        print(f"门禁结果: {report.gate_result}")
        print(f"门禁信息: {report.gate_message}")
        print("=" * 60)

        if report.issues:
            print("\n问题详情:")
            print("-" * 60)
            for i, issue in enumerate(report.issues[:20], 1):  # 只显示前20条
                print(f"{i}. [{issue.severity}] {issue.file_path}:{issue.line}")
                print(f"   规则: {issue.rule_name}")
                print(f"   问题: {issue.message}")
                if issue.suggestion:
                    print(f"   建议: {issue.suggestion}")
                print()

            if len(report.issues) > 20:
                print(f"... 还有 {len(report.issues) - 20} 个问题，详见 JSON 报告")
