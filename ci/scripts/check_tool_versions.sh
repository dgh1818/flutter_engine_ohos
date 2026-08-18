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

    log_info "Configuring git"
    run_cmd "git config --global user.name \"Flutter CI\""
    run_cmd "git config --global user.email \"flutter_ci@flutter.com\""
    run_cmd "git config -l"
}

check_tool_versions
