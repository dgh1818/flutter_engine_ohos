#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

# Constants
readonly WORK_DIR="$(pwd)"
readonly PROJECT_DIR="${WORK_DIR}/third_party"
readonly ARCHIVE_DIR="${WORK_DIR}/Archive/out"
readonly BUILD_INSTRUCTION="${ARCHIVE_DIR}/build_mode.txt"

compile_web() {
    local engine_dir="${1:-}"

    if [[ ! -f "$BUILD_INSTRUCTION" ]]; then
        log_info "Incremental build mode, skip compiling web sdk"
        exit 0
    fi

    cd "$PROJECT_DIR/$engine_dir" || { log_error "Failed to cd to $PROJECT_DIR/$engine_dir"; exit 1; }
    run_cmd "./src/flutter/tools/gn --web --runtime-mode=release --no-goma"
    run_cmd "ninja -C src/out/wasm_release flutter/web_sdk:flutter_web_sdk_archive"
}

compile_web "$@"
