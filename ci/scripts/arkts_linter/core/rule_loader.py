# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
"""规则配置加载器"""
import yaml
from pathlib import Path
from typing import Dict, Optional

from .models import RuleLevel


class RuleConfigLoader:
    """规则配置加载器"""

    def __init__(self, config_path: Optional[str] = None):
        """
        初始化

        Args:
            config_path: 规则配置文件路径，默认为包内 rules.yaml
        """
        if config_path is None:
            # 默认使用包内的 rules.yaml
            config_path = Path(__file__).parent.parent / 'config' / 'rules.yaml'
        self.config_path = Path(config_path)
        self._rules: Dict[str, Dict] = {}
        self._load_rules()

    def _load_rules(self):
        """加载规则配置"""
        if not self.config_path.exists():
            raise FileNotFoundError(f"规则配置文件不存在: {self.config_path}")

        with open(self.config_path, 'r', encoding='utf-8') as f:
            config = yaml.safe_load(f)

        self._rules = config.get('rules', {})

        # 转换 level 字符串为 RuleLevel 枚举
        for rule_name, rule_config in self._rules.items():
            if 'level' in rule_config:
                level_str = rule_config['level']
                rule_config['level'] = RuleLevel[level_str]

    def get_rule_config(self, rule_name: str) -> Optional[Dict]:
        """获取规则配置"""
        return self._rules.get(rule_name)

    def get_all_rules(self) -> Dict[str, Dict]:
        """获取所有规则"""
        return self._rules

    def reload(self):
        """重新加载规则"""
        self._load_rules()
