# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
"""ArkTS Linter 主类"""
import argparse
import os
import sys
from pathlib import Path
from typing import Dict, List, Optional

from .core.models import LintIssue, LintReport, RuleLevel
from .core.rule_loader import RuleConfigLoader
from .core.scanner import ArkTSFileScanner
from .core.runner import CodeLinterRunner
from .core.checker import CustomRuleChecker
from .utils.report import ReportGenerator


class ArkTSLinter:
    """ArkTS Linter 主类"""

    def __init__(self,
                 codelinter_path: Optional[str] = None,
                 node_path: Optional[str] = None,
                 use_builtin_rules: bool = True,
                 rules_config_path: Optional[str] = None):
        """
        初始化

        Args:
            codelinter_path: codelinter 路径（可选，如果不提供则只使用内置规则）
            node_path: Node.js 路径
            use_builtin_rules: 是否使用内置规则检查
            rules_config_path: 规则配置文件路径（可选）
        """
        self.codelinter_path = codelinter_path
        self.node_path = node_path
        self.use_builtin_rules = use_builtin_rules

        if codelinter_path:
            self.runner = CodeLinterRunner(codelinter_path, node_path)
        else:
            self.runner = None

        if use_builtin_rules:
            # 加载规则配置
            rule_loader = RuleConfigLoader(rules_config_path)
            self.rules = rule_loader.get_all_rules()
            self.rule_checker = CustomRuleChecker(self.rules)
        else:
            self.rules = {}
            self.rule_checker = None

    def check(self,
              target_path: str,
              output_path: Optional[str] = None,
              config_path: Optional[str] = None,
              incremental: bool = False) -> LintReport:
        """
        执行检查

        Args:
            target_path: 目标路径
            output_path: 输出报告路径
            config_path: codelinter 配置文件路径
            incremental: 是否增量检查

        Returns:
            LintReport 对象
        """
        all_issues = []

        # 1. 扫描文件
        scanner = ArkTSFileScanner(target_path)
        files = scanner.scan()
        print(f"发现 {len(files)} 个 ArkTS 文件")

        # 2. 使用内置规则检查
        if self.rule_checker:
            print("使用内置门禁规则检查...")
            for file_path in files:
                issues = self.rule_checker.check_file(file_path)
                all_issues.extend(issues)

        # 3. 调用 codelinter（如果提供）
        if self.runner:
            print("调用 codelinter 检查...")
            codelinter_result = self.runner.run_linter(
                target_path=target_path,
                config_path=config_path,
                output_format='json',
                incremental=incremental
            )

            # 解析 codelinter 结果并合并
            codelinter_issues = self._parse_codelinter_result(codelinter_result)
            all_issues.extend(codelinter_issues)

        # 4. 生成报告
        report = ReportGenerator.generate_report(
            project_path=target_path,
            issues=all_issues,
            gate_rules=self.rules
        )

        # 5. 保存报告
        if output_path:
            ReportGenerator.save_json_report(report, output_path)

        # 6. 打印摘要
        ReportGenerator.print_summary(report)

        return report

    def _parse_codelinter_result(self, result: Dict) -> List[LintIssue]:
        """解析 codelinter 结果"""
        issues = []

        # 这里根据 codelinter 的实际输出格式进行解析
        # 由于 codelinter 输出格式可能因版本而异，这里提供通用解析逻辑

        if isinstance(result, dict):
            # 如果是字典格式
            if 'files' in result:
                for file_info in result.get('files', []):
                    file_path = file_info.get('filePath', '')
                    for msg in file_info.get('messages', []):
                        issue = LintIssue(
                            file_path=file_path,
                            line=msg.get('line', 0),
                            column=msg.get('column', 0),
                            rule_id=msg.get('ruleId', ''),
                            rule_name=msg.get('ruleId', ''),
                            severity=self._map_severity(msg.get('severity', 1)),
                            message=msg.get('message', ''),
                            suggestion=msg.get('suggestion', ''),
                            cwe=''
                        )
                        issues.append(issue)

        return issues

    def _map_severity(self, severity: int) -> str:
        """映射严重级别"""
        # codelinter: 1=warning, 2=error
        mapping = {
            2: RuleLevel.CRITICAL.value,
            1: RuleLevel.NORMAL.value,
            0: RuleLevel.HINT.value
        }
        return mapping.get(severity, RuleLevel.NORMAL.value)


def main():
    """主函数"""
    parser = argparse.ArgumentParser(
        description='ArkTS Code Linter 门禁检查工具',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
示例:
  # 使用内置规则检查（推荐，默认方式）
  python arkts_linter.py --target-path ./src --output report.json

  # 使用内置规则 + codelinter 检查（codelinter 需要在 HarmonyOS 项目目录下运行）
  python arkts_linter.py --codelinter-path "D:/DevEco Studio/plugins/codelinter/run/index.js" \\
                         --target-path ./src --output report.json

  # 仅使用 codelinter 检查（不使用内置规则）
  python arkts_linter.py --codelinter-path "D:/command-line-tools/bin/codelinter.bat" \\
                         --target-path ./src --output report.json --no-builtin-rules

  # 增量检查
  python arkts_linter.py --target-path ./src --incremental --output report.json

  # 使用自定义规则配置文件
  python arkts_linter.py --target-path ./src --rules-config ./my_rules.yaml --output report.json

注意:
  - 默认使用内置规则进行检查，覆盖全部 64 条门禁规则
  - codelinter 是可选的附加检查，需要在 HarmonyOS 项目根目录下运行
  - 使用 --no-builtin-rules 可以禁用内置规则，仅使用 codelinter
  - 使用 --rules-config 可以指定自定义的规则配置文件
        '''
    )

    parser.add_argument(
        '--codelinter-path',
        help='codelinter 路径（.bat 文件或 index.js）'
    )

    parser.add_argument(
        '--node-path',
        help='Node.js 路径（当 codelinter-path 是 index.js 时需要）'
    )

    parser.add_argument(
        '--target-path',
        required=True,
        help='要检查的目标路径（文件或目录）'
    )

    parser.add_argument(
        '--output', '-o',
        default='arkts_lint_report.json',
        help='输出报告路径（默认: arkts_lint_report.json）'
    )

    parser.add_argument(
        '--config', '-c',
        help='codelinter 配置文件路径（code-linter.json5）'
    )

    parser.add_argument(
        '--rules-config',
        help='自定义规则配置文件路径（YAML 格式）'
    )

    parser.add_argument(
        '--incremental', '-i',
        action='store_true',
        help='增量检查（仅检查变更文件）'
    )

    parser.add_argument(
        '--no-builtin-rules',
        action='store_true',
        help='不使用内置规则，只使用 codelinter'
    )

    parser.add_argument(
        '--error-threshold',
        type=int,
        default=0,
        help='严重错误阈值，超过则门禁失败（默认: 0）'
    )

    parser.add_argument(
        '--warning-threshold',
        type=int,
        default=10,
        help='一般警告阈值，超过则门禁失败（默认: 10）'
    )

    args = parser.parse_args()

    # 检查目标路径
    if not os.path.exists(args.target_path):
        print(f"错误: 目标路径不存在: {args.target_path}")
        sys.exit(1)

    # 默认使用内置规则
    # 只有当用户显式指定 --no-builtin-rules 时才禁用内置规则
    use_builtin = not args.no_builtin_rules

    # 创建 linter 实例
    linter = ArkTSLinter(
        codelinter_path=args.codelinter_path,
        node_path=args.node_path,
        use_builtin_rules=use_builtin,
        rules_config_path=args.rules_config
    )

    # 执行检查
    try:
        report = linter.check(
            target_path=args.target_path,
            output_path=args.output,
            config_path=args.config,
            incremental=args.incremental
        )

        # 根据门禁结果退出
        if report.gate_result == "FAIL":
            sys.exit(1)
        else:
            sys.exit(0)

    except Exception as e:
        print(f"检查过程中发生错误: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(2)


if __name__ == '__main__':
    main()
