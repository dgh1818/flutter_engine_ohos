#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

compile_engine() {
    WORK_DIR=$(pwd)
    PROJECT_DIR="$WORK_DIR/third_party"
    ARCHIVE_DIR="$WORK_DIR/Archive/out"

    cd "$PROJECT_DIR/$1"

    if [[ -z "${PR_URL:-}" ]]; then
        log_info "Daily build detected - performing full compilation"
        if ! run_cmd "./ohos && ./ohos --ohos-cpu x64"; then
            log_error "Full engine compilation failed"
            exit 1
        fi
    else
        readonly MODES=("debug" "profile" "release")
        readonly BUILD_MODE=${MODES[$RANDOM % ${#MODES[@]}]}
        log_info "Compiling engine in $BUILD_MODE mode"
        if ! run_cmd "./ohos -t $BUILD_MODE"; then
            log_error "Engine compilation failed for mode: $BUILD_MODE"
            exit 1
        fi
    fi

    cd src || {
        log_error "Failed to cd to src directory"
        exit 1
    }

    if [[ -z "${PR_URL:-}" ]]; then
        run_cmd "save_mtime out $ARCHIVE_DIR/restore_mtimes.sh"
    fi

    if [[ -d "out" ]]; then
        (cp -a out/. "$ARCHIVE_DIR/out" &)
    fi
}

compile_engine "$@"
