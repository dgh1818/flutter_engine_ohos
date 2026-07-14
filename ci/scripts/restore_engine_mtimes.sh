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

restore_engine_mtimes() {
    local root_dir="$1"
    local src_dir="$2"

    if [[ -n "${PR_URL:-}" ]]; then
        log_info "Gatekeeper build detected, skipping mtime restoration"
        exit 0
    fi

    if [[ -f "$BUILD_INSTRUCTION" ]]; then
        log_info "Full build mode detected, skipping mtime restoration"
        exit 0
    fi

    cd "$PROJECT_DIR/$src_dir" || { log_error "Failed to cd to $PROJECT_DIR/$src_dir"; exit 1; }

    if [[ ! -f "engine.ohos.har.version" ]]; then
        log_warn "engine.ohos.har.version not found, skipping mtime restoration"
        exit 0
    fi

    run_cmd "find . -type f -exec touch -d '10 days ago' {} +"

    local commit_id
    commit_id=$(cat engine.ohos.har.version)
    log_info "Touching changed files since $commit_id"
    cd "$PROJECT_DIR/$root_dir" || { log_error "Failed to cd to $PROJECT_DIR/$root_dir"; exit 1; }
    if ! run_cmd "git diff --name-only --diff-filter=d $commit_id | xargs -r touch"; then
        log_warn "Failed to touch changed files"
        exit 0
    fi

    cd "$PROJECT_DIR/$src_dir" || { log_error "Failed to cd to $PROJECT_DIR/$src_dir"; exit 1; }
    if [[ ! -f "artifacts.txt" ]]; then
        log_warn "artifacts.txt not found, skipping mtime restoration"
        exit 0
    fi
    local artifacts_url
    artifacts_url=$(cat artifacts.txt)
    if ! curl -f -L -- "$artifacts_url" > out.tar.gz; then
        log_warn "Download failed"
        exit 0
    fi

    log_info "Extracting tarball"
    if ! tar -xzf out.tar.gz; then
        log_warn "Failed to extract tarball"
        rm -f out.tar.gz
        rm -rf out
        exit 0
    fi

    if [[ -f "restore_mtimes.sh" ]]; then
        log_info "Restoring modification times"
        chmod +x ./restore_mtimes.sh && run_cmd "./restore_mtimes.sh"
    fi
}

restore_engine_mtimes "$@"
