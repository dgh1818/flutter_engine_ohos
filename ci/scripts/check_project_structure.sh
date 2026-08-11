#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

check_project_structure() {
    WORK_DIR=$(pwd)
    PROJECT_DIR="$WORK_DIR/third_party"
    ARCHIVE_DIR="$WORK_DIR/Archive/out"

    run_cmd "pwd"

    if [[ ! -d "$PROJECT_DIR" ]]; then
        log_error "Project directory does not exist: $PROJECT_DIR"
        exit 1
    fi

    run_cmd "ls -la $PROJECT_DIR"

    log_info "Creating necessary directories"
    run_cmd "mkdir -p $ARCHIVE_DIR/out"
}

check_project_structure
