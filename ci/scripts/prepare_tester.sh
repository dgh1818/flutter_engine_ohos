#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

prepare_tester() {
    WORK_DIR=$(pwd)
    PROJECT_DIR="$WORK_DIR/third_party"

    local target_branch="$1"

    if [[ ! -d "$PROJECT_DIR/flutter_tester" ]]; then
        log_error "Directory $PROJECT_DIR/flutter_tester does not exist"
        exit 1
    fi

    cd "$PROJECT_DIR/flutter_tester" || {
        log_error "Failed to cd to flutter_tester directory"
        exit 1
    }

    run_cmd "git branch -a"
    run_cmd "git checkout $target_branch"
    run_cmd "git reset --hard origin/$target_branch"
    run_cmd "git pull --rebase"
}

prepare_tester "$@"
