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

# Set build mode to full and exit
set_full_build() {
    local reason="$1"
    log_warn "${reason}, full build required"
    echo full > "$BUILD_INSTRUCTION"
    exit 0
}

# Check if DEPS or DEPS_ohos has been modified since the given commit
check_deps_changed() {
    local commit_id="$1"
    local git_dir="$2"
    git -C "$git_dir" diff --name-only "$commit_id" | grep -qE "(^|/)DEPS(_ohos)?$"
}

check_build_mode() {
    local root_dir="$1"
    local src_dir="$2"
    local target_branch="$3"
    local flutter_dir="$4"

    if [[ $# -lt 4 ]]; then
        log_error "Usage: $0 <root_dir> <src_dir> <target_branch> <flutter_dir>"
        exit 1
    fi

    if [[ -n "${PR_URL:-}" ]]; then
        log_info "Gatekeeper build detected, skipping"
        exit 0
    fi

    log_info "Initializing archive"
    run_cmd "archive init"

    # Thursday early morning -> full build
    local day_of_week
    local hour
    day_of_week=$(date +%u)
    hour=$(date +%H)
    if [[ "$day_of_week" -eq 4 && "10#$hour" -lt 9 ]]; then
        set_full_build "Today is Thursday and before 9 AM, triggering full build"
    fi

    local src_path="$PROJECT_DIR/$src_dir"
    local root_path="$PROJECT_DIR/$root_dir"

    # Download and check engine version file and artifacts.txt
    log_info "Downloading engine version file and artifacts.txt"
    cd "$src_path" || { log_error "Failed to cd to $src_path"; exit 1; }
    run_cmd "archive cp cloud://$target_branch/engine.ohos.har.version engine.ohos.har.version"
    run_cmd "archive cp cloud://$target_branch/artifacts.txt artifacts.txt"

    # First build -> full build
    if [[ ! -f "engine.ohos.har.version" || ! -f "artifacts.txt" ]]; then
        set_full_build "Engine version file or artifacts.txt not found"
    fi

    local commit_id
    commit_id=$(cat engine.ohos.har.version)
    if [[ -z "$commit_id" ]]; then
        set_full_build "Engine version file is empty"
    fi
    log_info "Last engine commit ID: $commit_id"

    # DEPS changed -> full build
    if check_deps_changed "$commit_id" "$root_path"; then
        set_full_build "DEPS or DEPS_ohos file has been modified"
    fi

    # Release version: check DEPS changes against local version
    if [[ "${version_type:-}" == "Master_Version" ]]; then
        local last_version_file="$PROJECT_DIR/${flutter_dir}/bin/internal/engine.ohos.har.version"
        if [[ ! -f "$last_version_file" ]]; then
            set_full_build "Local version file not found: $last_version_file"
        fi
        local last_version
        last_version=$(cat "$last_version_file")
        if [[ -z "$last_version" ]]; then
            set_full_build "Local version file is empty"
        fi
        log_info "Last version: $last_version"
        if check_deps_changed "$last_version" "$root_path"; then
            set_full_build "DEPS or DEPS_ohos file has been modified (release version)"
        fi
    fi
}

check_build_mode "$@"
