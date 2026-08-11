#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

pack_flutter() {
    WORK_DIR=$(pwd)
    PROJECT_DIR="$WORK_DIR/third_party"
    ARCHIVE_DIR="$WORK_DIR/Archive/out"

    local target_branch="$1"

    cd "$PROJECT_DIR/flutter_flutter"

    local output_file="$ARCHIVE_DIR/sdk-$target_branch.zip"

    log_info "Creating SDK archive: $output_file"
    if ! run_cmd "zip -r $output_file *"; then
        log_error "Failed to create SDK archive"
        exit 1
    fi
}

pack_flutter "$@"
