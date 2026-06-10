#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"
PUBLISH_SCRIPT="$SCRIPT_DIR/publish.py"

# Constants
readonly WORK_DIR="$(pwd)"
readonly PROJECT_DIR="${WORK_DIR}/third_party"
readonly ARCHIVE_DIR="${WORK_DIR}/Archive/out"
readonly VERSION_FILE="${ARCHIVE_DIR}/engine.ohos.har.version"
readonly BUILD_INSTRUCTION="${ARCHIVE_DIR}/build_mode.txt"

publish() {
    local root_dir="$1"
    local engine_dir="${2:-}"

    # Skip publish for PR builds or daily builds
    if [[ -n "${PR_URL:-}" || "${version_type:-}" != "Master_Version" ]]; then
        log_info "PR build or daily build, skip publish"
        exit 0
    fi

    # Parameter validation
    if [[ -z "$root_dir" ]]; then
        log_error "root_dir is required"
        exit 1
    fi

    # Check version file
    if [[ ! -f "$VERSION_FILE" ]]; then
        log_error "Version file not found: $VERSION_FILE"
        exit 1
    fi

    # Check Python script
    if [[ ! -f "$PUBLISH_SCRIPT" ]]; then
        log_error "Publish script not found: $PUBLISH_SCRIPT"
        exit 1
    fi

    local version="$(cat "$VERSION_FILE")"
    local mode="full"
    if [[ ! -f "$BUILD_INSTRUCTION" ]]; then
        mode="incremental"
    fi

    # zip artifacts for full mode
    if [[ "$mode" == "full" ]]; then
        local zip_scripts_dir="$PROJECT_DIR/$engine_dir/src/flutter/attachment/scripts"
        local scripts=(
            "zip_artifacts.py"
            "zip_dart_sdk.py"
            "zip_sky_engine.py"
            "zip_flutter_patched_sdk.py"
        )

        log_step "Zipping artifacts"
        for script in "${scripts[@]}"; do
            local script_path="$zip_scripts_dir/$script"
            if [[ -f "$script_path" ]]; then
                log_info "Running $script"
                if ! python3 "$script_path"; then
                    log_error "Failed to run $script"
                    exit 1
                fi
            else
                log_warn "Script not found: $script_path"
            fi
        done
    fi

    # Run the Python script
    log_info "Publishing in $mode mode"
    if ! python3 "$PUBLISH_SCRIPT" \
        --mode "$mode" \
        --engine-dir "$PROJECT_DIR/$engine_dir" \
        --version "$version"; then
        log_error "Publish failed"
        exit 1
    fi

    run_cmd "archive uninstall"
}

publish "$@"
