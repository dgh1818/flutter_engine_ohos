#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

restore_engine_mtimes() {
    WORK_DIR=$(pwd)
    PROJECT_DIR="$WORK_DIR/third_party"

    local root_dir="$1"
    local src_dir="$2"
    local target_branch="$3"

    if [[ -n "${PR_URL:-}" ]]; then
        log_info "Gatekeeper build detected, skipping mtime restoration"
        exit 0
    fi

    log_info "Initializing archive"
    run_cmd "archive init"

    # If it's Thursday and before 9 AM, skip mtime restoration
    local day_of_week=$(date +%u)
    local hour=$(date +%H)
    if [[ "$day_of_week" -eq 4 && "10#$hour" -lt 9 ]]; then
        log_info "Today is Thursday and before 9 AM, skipping mtime restoration"
        exit 0
    fi

    cd "$PROJECT_DIR/$src_dir"

    log_info "Downloading engine version file"
    run_cmd "archive cp cloud://$target_branch/engine.ohos.har.version engine.ohos.har.version"

    if [[ ! -f "engine.ohos.har.version" ]]; then
        log_warn "Engine version file not found, skipping mtime restoration"
        exit 0
    fi

    local commit_id=$(cat engine.ohos.har.version)
    log_info "Last engine commit ID: $commit_id"

    # Check if DEPS or DEPS_ohos file has been modified
    cd "$PROJECT_DIR/$root_dir"
    if git diff --name-only "$commit_id" | grep -q "DEPS"; then
        log_warn "DEPS_ohos file has been modified, skipping mtime restoration"
        exit 0
    fi

    cd "$PROJECT_DIR/$src_dir"
    log_info "Setting old file timestamps"
    run_cmd "find . -type f -exec touch -d '10 days ago' {} +"

    log_info "Touching changed files since $commit_id"
    cd "$PROJECT_DIR/$root_dir"
    if ! run_cmd "git diff --name-only --diff-filter=d $commit_id | xargs -r touch"; then
        log_warn "Failed to touch changed files"
        exit 0
    fi

    cd "$PROJECT_DIR/$src_dir"

    log_info "Downloading artifacts from cloud"
    run_cmd "archive cp cloud://$target_branch/artifacts.txt artifacts.txt"

    if [[ ! -f "artifacts.txt" ]]; then
        log_warn "artifacts.txt not found, skipping mtime restoration"
        exit 0
    fi

    local artifacts_url=$(cat artifacts.txt)

    curl -f -L -- "$artifacts_url" > out.tar.gz
    if [ $? -ne 0 ]; then
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
