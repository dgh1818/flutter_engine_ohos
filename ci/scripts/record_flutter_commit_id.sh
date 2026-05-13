#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

write_commit_id() {
    WORK_DIR=$(pwd)
    PROJECT_DIR="$WORK_DIR/third_party"
    ARCHIVE_DIR="$WORK_DIR/Archive/out"

    local root_dir="$1"

    cd "$PROJECT_DIR/$root_dir"

    local commit_id=$(git rev-parse HEAD)

    if [[ -z "$commit_id" ]]; then
        log_error "Failed to get commit ID"
        exit 1
    fi

    log_info "Writing commit ID: $commit_id"
    run_cmd "echo $commit_id > $ARCHIVE_DIR/engine.ohos.har.version"
    run_cmd "echo $commit_id > $ARCHIVE_DIR/engine.ohos.version"
}

write_commit_id "$@"
