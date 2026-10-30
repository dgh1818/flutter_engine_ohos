# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
"""自定义规则检查器"""
import re
from pathlib import Path
from typing import Dict, List

from .models import LintIssue, RuleLevel
from ..utils.helpers import remove_line_comment, remove_comments, is_inside_try_block, extract_rule_id


class CustomRuleChecker:
    """自定义规则检查器（基于门禁规则集）"""

    def __init__(self, rules: Dict[str, Dict]):
        """
        初始化

        Args:
            rules: 规则配置字典
        """
        self.rules = rules

    def check_file(self, file_path: Path) -> List[LintIssue]:
        """检查单个文件"""
        issues = []

        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
                lines = content.split('\n')
        except Exception as e:
            print(f"读取文件失败 {file_path}: {e}")
            return issues

        # 预处理：生成去除注释后的代码行（保持行号对齐）
        code_lines = self._remove_comments_from_lines(lines)

        # 应用所有规则
        for rule_name, rule_config in self.rules.items():
            rule_issues = self._apply_rule(
                file_path, content, lines, code_lines, rule_name, rule_config
            )
            issues.extend(rule_issues)

        return issues

    def _remove_comments_from_lines(self, lines: List[str]) -> List[str]:
        """
        移除注释，返回与原始行号对齐的代码行列表
        保留字符串字面量中的内容
        """
        result = []
        in_multiline_comment = False

        for line in lines:
            if in_multiline_comment:
                # 检查是否结束多行注释
                if '*/' in line:
                    in_multiline_comment = False
                    # 保留 */ 之后的内容
                    idx = line.index('*/') + 2
                    line = line[idx:]
                else:
                    result.append('')
                    continue

            # 处理单行注释和多行注释开始
            cleaned = []
            i = 0
            in_string = False
            string_char = None

            while i < len(line):
                ch = line[i]

                # 处理字符串
                if ch in ('"', "'", '`') and (i == 0 or line[i-1] != '\\'):
                    if not in_string:
                        in_string = True
                        string_char = ch
                    elif ch == string_char:
                        in_string = False
                        string_char = None
                    cleaned.append(ch)
                    i += 1
                    continue

                # 处理注释
                if not in_string:
                    if ch == '/' and i + 1 < len(line):
                        if line[i+1] == '/':
                            # 单行注释，跳过剩余内容
                            break
                        elif line[i+1] == '*':
                            # 多行注释开始
                            in_multiline_comment = True
                            i += 2
                            # 检查是否在同一行结束
                            if '*/' in line[i:]:
                                end_idx = line.index('*/', i)
                                i = end_idx + 2
                                in_multiline_comment = False
                            continue

                cleaned.append(ch)
                i += 1

            result.append(''.join(cleaned))

        return result

    def _apply_rule(self,
                    file_path: Path,
                    content: str,
                    lines: List[str],
                    code_lines: List[str],
                    rule_name: str,
                    rule_config: Dict) -> List[LintIssue]:
        """应用单个规则"""
        issues = []
        pattern = rule_config.get('pattern')

        if not pattern:
            return issues

        try:
            # 检查是否需要多行匹配
            if rule_config.get('operator_at_line_start'):
                # 特殊处理：检测运算符在行首的情况（需要跨行匹配）
                return self._check_operator_at_line_start(file_path, code_lines, rule_name, rule_config)

            # 检查是否需要全内容匹配（跨行）
            if rule_config.get('multiline_match'):
                return self._check_multiline_pattern(file_path, content, lines, rule_name, rule_config)

            # G.OTH.03 需要检测注释掉的代码，使用原始行
            if rule_config.get('check_commented_code'):
                match_lines = lines
            else:
                # 其他规则使用去除注释后的代码行
                match_lines = code_lines

            regex = re.compile(pattern, re.MULTILINE | re.IGNORECASE)

            # 用于去重的集合（针对同一行同一规则的重复报告）
            reported_lines = set()

            for line_num, line in enumerate(match_lines, 1):
                if not line.strip():
                    continue
                matches = regex.finditer(line)
                for match in matches:
                    # 检查特定条件
                    if self._check_specific_condition(
                            rule_config, content, lines, line_num, line, match
                    ):
                        # 去重：同一行同一规则只报告一次
                        dedup_key = (line_num, rule_name)
                        if dedup_key in reported_lines:
                            continue
                        reported_lines.add(dedup_key)

                        issue = LintIssue(
                            file_path=str(file_path),
                            line=line_num,
                            column=match.start() + 1,
                            rule_id=extract_rule_id(rule_name),
                            rule_name=rule_name,
                            severity=rule_config['level'].value,
                            message=self._generate_message(rule_name, rule_config, match),
                            suggestion=self._generate_suggestion(rule_name, rule_config),
                            cwe=rule_config.get('cwe', '')
                        )
                        issues.append(issue)

        except re.error as e:
            print(f"规则 {rule_name} 的正则表达式错误: {e}")

        return issues

    def _check_multiline_pattern(self,
                                  file_path: Path,
                                  content: str,
                                  lines: List[str],
                                  rule_name: str,
                                  rule_config: Dict) -> List[LintIssue]:
        """跨行匹配模式"""
        issues = []
        pattern = rule_config.get('pattern')

        try:
            regex = re.compile(pattern, re.MULTILINE | re.DOTALL)
            matches = list(regex.finditer(content))

            for match in matches:
                # 计算行号
                line_num = content[:match.start()].count('\n') + 1
                line = lines[line_num - 1] if line_num <= len(lines) else ''

                # 检查特定条件
                if not self._check_specific_condition(rule_config, content, lines, line_num, line, match):
                    continue

                issue = LintIssue(
                    file_path=str(file_path),
                    line=line_num,
                    column=match.start() - content.rfind('\n', 0, match.start()),
                    rule_id=extract_rule_id(rule_name),
                    rule_name=rule_name,
                    severity=rule_config['level'].value,
                    message=self._generate_message(rule_name, rule_config, match),
                    suggestion=self._generate_suggestion(rule_name, rule_config),
                    cwe=rule_config.get('cwe', '')
                )
                issues.append(issue)

        except re.error as e:
            print(f"规则 {rule_name} 的正则表达式错误: {e}")

        return issues

    def _check_operator_at_line_start(self,
                                       file_path: Path,
                                       lines: List[str],
                                       rule_name: str,
                                       rule_config: Dict) -> List[LintIssue]:
        """检查运算符是否在行首（应该在行末）"""
        issues = []
        # 只检查逻辑运算符和比较运算符，这些在表达式换行时最容易出问题
        operators = ['||', '&&', '==', '!=', '===', '!==', '>=', '<=', '>', '<']
        in_multiline_comment = False

        for line_num, line in enumerate(lines, 1):
            stripped = line.strip()

            # 处理多行注释
            if '/*' in line:
                in_multiline_comment = True
            if '*/' in line:
                in_multiline_comment = False
                continue
            if in_multiline_comment:
                continue

            # 跳过空行和单行注释
            if not stripped or stripped.startswith('//'):
                continue

            # 检查行首是否是运算符
            for op in operators:
                if stripped.startswith(op):
                    # 排除字符串开头
                    if stripped.startswith('"') or stripped.startswith("'"):
                        continue

                    # 检查前一行是否以表达式结尾（不是分号、括号等）
                    if line_num > 1:
                        # 向上查找最近的非空行
                        prev_line_num = line_num - 2
                        prev_line = ""
                        while prev_line_num >= 0:
                            prev_line = lines[prev_line_num].strip()
                            if prev_line and not prev_line.startswith('//') and not prev_line.startswith('/*'):
                                break
                            prev_line_num -= 1

                        if not prev_line:
                            continue

                        # 如果前一行以分号、大括号结尾，则不是表达式换行
                        if prev_line.endswith(';') or prev_line.endswith('{') or prev_line.endswith('}'):
                            continue

                        # 如果前一行以 ) 结尾，需要检查是否是函数调用结束
                        # 例如: func()\n  .method() 这种链式调用是合法的
                        if prev_line.endswith(')'):
                            # 检查是否是 if/for/while 等控制语句的条件
                            # 如果前一行包含 if/for/while 且括号未闭合，则是表达式换行
                            if not re.search(r'(if|for|while)\s*\([^)]*$', prev_line):
                                continue

                        issue = LintIssue(
                            file_path=str(file_path),
                            line=line_num,
                            column=len(line) - len(line.lstrip()) + 1,  # 运算符实际位置
                            rule_id=extract_rule_id(rule_name),
                            rule_name=rule_name,
                            severity=rule_config['level'].value,
                            message=f"违反规则: {rule_name} - 运算符 '{op}' 应放在行末",
                            suggestion="将运算符放在上一行行末，保持表达式换行的一致性",
                            cwe=rule_config.get('cwe', '')
                        )
                        issues.append(issue)
                        break  # 每行只报告一次

        return issues

    def _check_specific_condition(self,
                                   rule_config: Dict,
                                   content: str,
                                   lines: List[str],
                                   line_num: int,
                                   line: str,
                                   match: re.Match) -> bool:
        """检查特定条件"""
        # 检查是否需要 catch
        if rule_config.get('check_catch'):
            # 只检查当前函数范围内的 catch 和 try
            # 向前查找函数开始
            func_start = line_num - 1
            brace_count = 0
            for i in range(line_num - 1, max(0, line_num - 50), -1):
                l = lines[i]
                brace_count += l.count('}') - l.count('{')
                if re.match(r'\s*(function|def|class|\w+\s*\()', l) and brace_count <= 0:
                    func_start = i
                    break

            # 向后查找函数结束
            func_end = line_num
            brace_count = 0
            for i in range(line_num - 1, min(len(lines), line_num + 50)):
                l = lines[i]
                brace_count += l.count('{') - l.count('}')
                if brace_count <= 0 and '}' in l:
                    func_end = i
                    break

            # 只检查函数范围内的内容
            func_lines = lines[func_start:func_end + 1]
            func_content = remove_comments(func_lines)

            # 检查是否有 catch 或 try
            if '.catch(' not in func_content and 'try' not in func_content:
                return True
            return False

        # 检查是否需要 finally
        if rule_config.get('check_finally'):
            remaining_content = remove_comments(lines[line_num:])
            if 'finally' not in remaining_content:
                return True
            return False

        # 检查是否需要 close/release
        if rule_config.get('check_close') or rule_config.get('check_release'):
            remaining_content = remove_comments(lines[line_num:])
            close_keywords = ['close(', 'release(', 'reclaim(']
            if not any(kw in remaining_content for kw in close_keywords):
                return True
            return False

        # 检查敏感关键词
        if 'sensitive_keywords' in rule_config:
            line_lower = line.lower()
            for keyword in rule_config['sensitive_keywords']:
                if keyword.lower() in line_lower:
                    # 检查是否有匿名化处理
                    if 'mask' not in line_lower and 'anony' not in line_lower:
                        return True
            return False

        # 检查不安全算法
        if 'unsafe_algorithms' in rule_config:
            for algo in rule_config['unsafe_algorithms']:
                if algo in match.group():
                    return True
            return False

        # 检查硬编码
        if rule_config.get('check_hardcode'):
            # 简单的硬编码检测
            if re.search(r'["\'][^"\']{8,}["\']', line):
                return True
            return False

        # 检查 try-catch（检查当前行是否在 try 块内）
        if rule_config.get('check_try_catch'):
            if is_inside_try_block(lines, line_num):
                return False
            return True

        # 检查是否只在类中检查访问修饰符
        if rule_config.get('in_class_only'):
            # 检查当前行是否在类定义中（不包括接口）
            in_class = False
            in_interface = False
            brace_count = 0
            for i in range(line_num - 1, -1, -1):
                l = lines[i]
                brace_count += l.count('}') - l.count('{')
                if re.match(r'\s*(export\s+)?(default\s+)?interface\s+\w+', l):
                    in_interface = True
                    break
                if re.match(r'\s*(export\s+)?(default\s+)?class\s+\w+', l):
                    in_class = True
                    break
                if brace_count < 0:
                    break
            # 接口中的属性不需要访问修饰符
            if in_interface:
                return False
            if not in_class:
                return False
            # 检查是否已经有访问修饰符
            if re.match(r'^\s+(public|private|protected)\s+', line):
                return False
            return True

        # 检查 if/for/while/do 语句的大括号（处理多行条件）
        if rule_config.get('require_braces') or rule_config.get('require_braces_always'):
            # 获取匹配到的控制语句
            matched_text = match.group()
            # 检查是否是多行条件（括号未闭合或行尾是运算符）
            # 计算括号是否平衡
            open_paren = matched_text.count('(')
            close_paren = matched_text.count(')')
            if open_paren > close_paren:
                # 括号未闭合，是多行条件，不报告
                return False
            # 检查行尾是否是运算符（||、&&、+、- 等）
            stripped_line = line.rstrip()
            if re.search(r'(\|\||&&|\+|\-|\*|\/|%|==|!=|===|!==|>=|<=|>|<|\?|:)\s*$', stripped_line):
                # 行尾是运算符，是多行表达式，不报告
                return False
            # 检查当前行是否以 { 结尾
            if stripped_line.endswith('{'):
                # 当前行以 { 结尾，不报告
                return False
            # 检查下一行是否是 { 或语句
            if line_num < len(lines):
                next_line = lines[line_num].strip()
                if next_line.startswith('{'):
                    # 下一行是 {，不报告
                    return False
            return True

        # 检查 switch 语句的 case/default 缩进
        if rule_config.get('check_indentation'):
            # 获取匹配到的内容
            matched_text = match.group()
            # 从匹配内容中提取 case 行的缩进
            matched_lines = matched_text.split('\n')
            if len(matched_lines) >= 2:
                # 获取 switch 行的实际内容（从 lines 数组中）
                switch_line = lines[line_num - 1]
                # 获取 case 行的缩进（从匹配内容中）
                case_line = matched_lines[1]

                # 计算缩进
                switch_indent = len(switch_line) - len(switch_line.lstrip())
                case_indent = len(case_line) - len(case_line.lstrip())

                # case/default 应该比 switch 多缩进一层（2 个空格）
                if case_indent == switch_indent + 2:
                    # 缩进正确，不报告
                    return False
            return True

        # 默认匹配即报告
        return True

    def _generate_message(self, rule_name: str, rule_config: Dict, match: re.Match) -> str:
        """生成问题消息"""
        base_msg = f"违反规则: {rule_name}"

        if 'unsafe_algorithms' in rule_config:
            return f"{base_msg} - 使用了不安全的算法: {match.group()}"
        elif 'sensitive_keywords' in rule_config:
            return f"{base_msg} - 可能泄露敏感信息"
        elif rule_config.get('check_hardcode'):
            return f"{base_msg} - 存在硬编码"
        else:
            return base_msg

    def _generate_suggestion(self, rule_name: str, rule_config: Dict) -> str:
        """生成修改建议"""
        return rule_config.get('suggestion', '请参考门禁规则集进行修改')
