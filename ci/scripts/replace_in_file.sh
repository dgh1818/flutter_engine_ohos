#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

# 通用文件替换函数
# 参数: $1=文件相对路径, $2=sed替换表达式
replace_in_file() {
    local file_rel_path="$1"
    local sed_expr="$2"

    WORK_DIR=$(pwd)
    PROJECT_DIR="$WORK_DIR/third_party"
    local target_file="$PROJECT_DIR/$file_rel_path"

    if [[ -f "$target_file" ]]; then
        run_cmd "sed -i '$sed_expr' $target_file"
    fi
}

replace_in_file "$@"
