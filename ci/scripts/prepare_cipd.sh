#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

prepare_cipd() {
    WORK_DIR=$(pwd)
    PROJECT_DIR="$WORK_DIR/third_party"
    CIPD_CACHE_DIR=${CIPD_CACHE_DIR:-/home/tools/cipd_cache}

    if [[ ! -d "$CIPD_CACHE_DIR" ]]; then
        echo "[ERROR] CIPD cache directory does not exist: $CIPD_CACHE_DIR" >&2
        exit 1
    fi

    if [[ ! -d "$CIPD_CACHE_DIR/instances" ]]; then
        echo "[ERROR] instances directory does not exist in $CIPD_CACHE_DIR" >&2
        exit 1
    fi

    log_info "Refreshing CIPD cache timestamps"
    run_cmd "find $CIPD_CACHE_DIR -type f -exec touch {} +" "Touch all cache files"

    log_info "Clearing CIPD state database"
    local state_db="$CIPD_CACHE_DIR/instances/state.db"
    if [[ -f "$state_db" ]]; then
        run_cmd "rm $state_db"
    fi

    if [[ ! -d "$PROJECT_DIR/cipd" ]]; then
        log_warn "CIPD directory does not exist, skipping"
        exit 0
    fi

    cd "$PROJECT_DIR/cipd" || {
        log_error "Failed to cd to cipd directory"
        exit 1
    }

    run_cmd "git branch -a"
    run_cmd "git checkout main"
    run_cmd "git reset --hard origin/main"
    run_cmd "git pull --rebase"
    run_cmd "patchcipd"

    log_info "CIPD instances statistics"
    run_cmd "ls -al $CIPD_CACHE_DIR/instances"
    local cipd_count=$(ls -1A "$CIPD_CACHE_DIR/instances" | wc -l)
    log_info "Total CIPD instances: $cipd_count"
}

prepare_cipd
