# ArkTS Linter - 门禁检查工具

基于 ArkTS 门禁规则集的代码检查工具，支持 64 条门禁规则的自动化检查。

## 项目结构

```
arkts_linter/
├── __init__.py                 # 包初始化
├── arkts_linter.py            # 主入口（命令行接口）
├── config/
│   ├── __init__.py
│   └── rules.yaml             # 规则配置文件（YAML 格式）
├── core/
│   ├── __init__.py
│   ├── models.py              # 数据模型（LintIssue, LintReport 等）
│   ├── rule_loader.py         # 规则配置加载器
│   ├── scanner.py             # 文件扫描器
│   ├── runner.py              # codelinter 运行器
│   └── checker.py             # 规则检查器
├── utils/
│   ├── __init__.py
│   ├── helpers.py             # 工具函数
│   └── report.py              # 报告生成器
└── README.md                  # 本文件
```

## 安装依赖

```bash
pip install pyyaml
```

## 使用方法

### 基本用法

```bash
# 使用内置规则检查（推荐，默认方式）
python arkts_linter_cli.py --target-path ./src --output report.json

# 使用内置规则 + codelinter 检查
python arkts_linter_cli.py --codelinter-path "D:/DevEco Studio/plugins/codelinter/run/index.js" \
                           --target-path ./src --output report.json

# 仅使用 codelinter 检查（不使用内置规则）
python arkts_linter_cli.py --codelinter-path "D:/command-line-tools/bin/codelinter.bat" \
                           --target-path ./src --output report.json --no-builtin-rules

# 增量检查
python arkts_linter_cli.py --target-path ./src --incremental --output report.json

# 使用自定义规则配置文件
python arkts_linter_cli.py --target-path ./src --rules-config ./my_rules.yaml --output report.json
```

### 作为 Python 包使用

```python
from arkts_linter import ArkTSLinter

# 创建 linter 实例
linter = ArkTSLinter(
    codelinter_path="D:/command-line-tools/bin/codelinter.bat",
    use_builtin_rules=True
)

# 执行检查
report = linter.check(
    target_path="./src",
    output_path="report.json"
)

# 查看结果
print(f"门禁结果: {report.gate_result}")
print(f"总问题数: {report.total_issues}")
```

## 规则配置

规则配置文件使用 YAML 格式，位于 `config/rules.yaml`。你可以：

1. 修改现有规则的配置
2. 添加新的规则
3. 使用 `--rules-config` 参数指定自定义配置文件

### 规则配置示例

```yaml
rules:
  "规则名称":
    level: NORMAL  # 严重级别: CRITICAL, NORMAL, HINT
    cwe: "123"     # CWE 编号
    pattern: '正则表达式'
    suggestion: "修改建议"
    # 其他配置选项...
```

## 命令行参数

- `--codelinter-path`: codelinter 路径（.bat 文件或 index.js）
- `--node-path`: Node.js 路径（当 codelinter-path 是 index.js 时需要）
- `--target-path`: 要检查的目标路径（文件或目录，必需）
- `--output, -o`: 输出报告路径（默认: arkts_lint_report.json）
- `--config, -c`: codelinter 配置文件路径（code-linter.json5）
- `--rules-config`: 自定义规则配置文件路径（YAML 格式）
- `--incremental, -i`: 增量检查（仅检查变更文件）
- `--no-builtin-rules`: 不使用内置规则，只使用 codelinter
- `--error-threshold`: 严重错误阈值，超过则门禁失败（默认: 0）
- `--warning-threshold`: 一般警告阈值，超过则门禁失败（默认: 10）

## 输出报告

报告以 JSON 格式保存，包含以下信息：

- 项目路径
- 扫描时间
- 检查文件数
- 问题统计（严重/一般/提示）
- 问题详情列表
- 规则统计
- 门禁结果

## 支持的平台

- Windows
- Linux
- macOS

## 许可证

MIT License
