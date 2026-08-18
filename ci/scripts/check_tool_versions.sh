#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

check_tool_versions() {
    tools=(
        "node:-v"
        "npm:-v"
        "ohpm:-v"
        "hvigorw:-v"
        "hdc:-v"
        "git:--version"
        "java:-version"
    )

    for tool in "${tools[@]}"; do
        name="${tool%:*}"
        flag="${tool#*:}"
        run_cmd "$name $flag" || true
    done
}

check_tool_versions
