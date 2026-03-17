#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

run_hooks() {
    WORK_DIR=$(pwd)
    PROJECT_DIR="$WORK_DIR/third_party"

    local root_dir="$1"

    cd "$PROJECT_DIR/$root_dir"

    if ! run_cmd "gclient runhooks"; then
        log_error "gclient runhooks failed"
        exit 1
    fi
}

run_hooks "$@"
