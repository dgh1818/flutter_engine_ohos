#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

prepare_project() {
    WORK_DIR=$(pwd)
    PROJECT_DIR="$WORK_DIR/third_party"

    local project_name="$1"
    local target_branch="$2"

    if [[ ! -d "$PROJECT_DIR/$project_name" ]]; then
        log_error "Directory $PROJECT_DIR/$project_name does not exist"
        exit 1
    fi

    cd "$PROJECT_DIR/$project_name" || {
        log_error "Failed to cd to $project_name directory"
        exit 1
    }

    log_info "Fetching remote branches"
    run_cmd "git fetch --all"
    run_cmd "git branch -a"

    log_info "Rebasing to $target_branch"
    if ! run_cmd "git rebase remotes/gitcode/$target_branch"; then
        log_error "Rebase failed!"
        exit 1
    fi

    log_info "Git status after rebase"
    run_cmd "git log -10 --pretty=format:'%h - %s'"
    run_cmd "git status"
    run_cmd "git diff"
}

prepare_project "$@"
