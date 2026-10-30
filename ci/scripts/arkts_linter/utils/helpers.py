# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
"""工具函数"""
import re
from typing import List


def remove_line_comment(line: str) -> str:
    """移除单行注释"""
    # 简单处理：移除 // 之后的内容（不处理字符串内的 //）
    if '//' in line:
        # 找到不在字符串内的 //
        in_string = False
        string_char = None
        for i, ch in enumerate(line):
            if ch in ('"', "'", '`') and (i == 0 or line[i-1] != '\\'):
                if not in_string:
                    in_string = True
                    string_char = ch
                elif ch == string_char:
                    in_string = False
                    string_char = None
            elif ch == '/' and i + 1 < len(line) and line[i+1] == '/' and not in_string:
                return line[:i]
    return line


def remove_comments(lines: List[str]) -> str:
    """移除多行注释和单行注释"""
    result = []
    in_multiline = False
    for line in lines:
        # 处理多行注释
        if '/*' in line:
            in_multiline = True
        if '*/' in line:
            in_multiline = False
            continue
        if in_multiline:
            continue
        # 处理单行注释
        if '//' in line:
            line = line[:line.index('//')]
        result.append(line)
    return '\n'.join(result)


def is_inside_try_block(lines: List[str], line_num: int) -> bool:
    """
    判断指定行是否在 try 块内
    使用 brace 配对从当前行向上搜索最近的 try {
    注意：向上扫描时，} 表示进入块（深度+1），{ 表示离开块（深度-1）
    """
    # 将 line_num 转换为 0-based 索引
    idx = line_num - 1
    brace_depth = 0
    try_line = -1
    try_depth = 0  # 记录 try 块的深度

    # 从当前行向上扫描
    for i in range(idx, -1, -1):
        line = lines[i]
        # 移除注释后再计算 brace
        clean_line = remove_line_comment(line)

        # 向上扫描时，} 增加深度（进入块），{ 减少深度（离开块）
        brace_depth += clean_line.count('}') - clean_line.count('{')

        # 检查是否找到 try {
        if re.search(r'\btry\s*\{', clean_line):
            try_line = i
            try_depth = brace_depth  # 记录 try 块的深度
            # 找到 try 后，继续向上扫描，直到离开 try 块
            continue

        # 如果已经找到 try，检查是否离开了 try 块
        if try_line >= 0:
            # 如果 brace_depth < try_depth，说明离开了 try 块
            if brace_depth < try_depth:
                # 离开了 try 块，返回 False
                return False

        # 如果 brace_depth > 0，说明当前行在某个块内
        if brace_depth > 0:
            # 如果遇到函数定义，停止搜索
            if re.search(r'\b(function|def|class)\b', clean_line):
                break

    # 如果没有找到 try，返回 False
    if try_line < 0:
        return False

    # 如果找到了 try，且当前行在 try 之后，说明在 try 块内
    return idx > try_line


def extract_rule_id(rule_name: str) -> str:
    """提取规则 ID"""
    # 提取类似 G.NAM.01 或 SecH_ATs_HW_CBG_CS_xxx 的规则ID
    match = re.match(r'([GS]\.[A-Z]+\.\d+|SecH_[A-Za-z_]+)', rule_name)
    if match:
        return match.group(1)
    return rule_name[:20]  # 使用前20个字符作为ID
