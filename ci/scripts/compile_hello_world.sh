#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

compile_hello_world() {
    WORK_DIR=$(pwd)
    PROJECT_DIR="$WORK_DIR/third_party"
    ARCHIVE_DIR="$WORK_DIR/Archive/out"

    local engine_dir="$1"

    log_info "Checking Flutter environment"
    run_cmd "flutter doctor -v"

    local hello_world_dir="$PROJECT_DIR/flutter_flutter/examples/hello_world"
    if [[ ! -d "$hello_world_dir" ]]; then
        log_error "Directory $hello_world_dir does not exist"
        exit 1
    fi

    log_info "Detaching hello_world from root workspace"
    local root_pubspec="$PROJECT_DIR/flutter_flutter/pubspec.yaml"
    if [[ -f "$root_pubspec" ]]; then
        sed -i '/^  - examples\/hello_world$/d' "$root_pubspec"
    fi
    sed -i '/^resolution: workspace$/d' "$hello_world_dir/pubspec.yaml"
    rm -rf "$hello_world_dir/.dart_tool" "$hello_world_dir/pubspec.lock"

    cd "$hello_world_dir"

    # Determine BUILD_MODE by checking available out directories
    local out_dir="$PROJECT_DIR/$engine_dir/src/out"

    # If all three exist, randomly choose one
    if [[ -d "$out_dir/host_debug" && -d "$out_dir/host_profile" && -d "$out_dir/host_release" ]]; then
        local modes=("debug" "profile" "release")
        BUILD_MODE="${modes[$((RANDOM % 3))]}"
        log_info "Multiple build modes found, randomly selected: $BUILD_MODE"
    elif [[ -d "$out_dir/host_release" ]]; then
        BUILD_MODE="release"
    elif [[ -d "$out_dir/host_profile" ]]; then
        BUILD_MODE="profile"
    elif [[ -d "$out_dir/host_debug" ]]; then
        BUILD_MODE="debug"
    else
        log_error "No valid build mode found in $out_dir"
        exit 1
    fi

    log_info "Building hello_world in $BUILD_MODE mode"
    local build_cmd="flutter build hap --$BUILD_MODE -t lib/main_ohos.dart --local-engine-src-path=$PROJECT_DIR/$engine_dir/src --local-engine=ohos_${BUILD_MODE}_arm64 --local-engine-host=host_$BUILD_MODE"
    run_cmd "$build_cmd"

    # Archive HAP file
    local hap_source="$hello_world_dir/ohos/entry/build/default/outputs/default/entry-default-unsigned.hap"
    local hap_dest="$ARCHIVE_DIR/entry-default-unsigned.hap"

    if [[ -f "$hap_source" ]]; then
        log_info "Copying HAP file to archive"
        run_cmd "cp $hap_source $hap_dest"
    else
        log_error "HAP file not found: $hap_source"
        exit 1
    fi
}

compile_hello_world "$@"