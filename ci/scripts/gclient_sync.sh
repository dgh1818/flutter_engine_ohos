#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

PARENT_DIR="$(dirname "$SCRIPT_DIR")"

gclient_sync() {
    WORK_DIR=$(pwd)
    PROJECT_DIR="$WORK_DIR/third_party"

    log_info "Restoring .gitconfig from cipd repo (url insteadOf redirects)"
    local cipd_gitconfig="$WORK_DIR/third_party/cipd/resource/.gitconfig"
    if [[ -f "$cipd_gitconfig" ]]; then
        run_cmd "cp $cipd_gitconfig ~/.gitconfig"
    else
        log_warn "cipd/resource/.gitconfig not found at $cipd_gitconfig, skipping"
    fi
    run_cmd "git config -l"

    local root_dir="$1"

    cd "$PROJECT_DIR/$root_dir"

    if [[ -d "$PARENT_DIR/resources" ]]; then
        run_cmd "cp -a \"$PARENT_DIR/resources\"/. ."
    fi
    run_cmd "ls -la"

    log_info "Running gclient sync"
    if ! run_cmd "gclient sync --ignore-dep-type=cipd -n"; then
        log_error "gclient sync failed"
        exit 1
    fi

    log_info "Ensuring CIPD packages"
    if ! run_cmd "cipd ensure -root . -ensure-file cipd_manifest.txt"; then
        log_error "CIPD ensure failed"
        exit 1
    fi
}

gclient_sync "$@"
