#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
#
# Build hello_world and stage its products under the engine's
# flutter_embedding for ArkTS unit testing.
#
# Steps:
#   1. Invokes bin/flutter to package hello_world with main_ohos.dart entry
#      in profile mode.
#   2. Copies native .so files into
#      ./engine/src/flutter/shell/platform/ohos/flutter_embedding/flutter/libs/arm64-v8a
#   3. Copies flutter_assets into
#      ./engine/src/flutter/shell/platform/ohos/flutter_embedding/flutter/src/ohosTest/resources/rawfile/flutter_assets
#
# Usage: build_hello_world_for_arkts_test.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

build_hello_world_for_arkts_test() {
    # Resolve paths relative to this script so it can be invoked from
    # anywhere (workspace root, ci/scripts/, or an absolute path).
    WORK_DIR="$(cd "$(dirname "$(dirname "$SCRIPT_DIR")")" && pwd)"
    cd "$WORK_DIR"

    local hello_world_dir="./examples/hello_world"
    if [[ ! -d "$hello_world_dir" ]]; then
        log_error "hello_world directory not found: $hello_world_dir"
        exit 1
    fi

    local flutter_bin="$(pwd)/bin/flutter"
    if [[ ! -x "$flutter_bin" ]]; then
        log_error "flutter binary not found or not executable: $flutter_bin"
        exit 1
    fi

    local build_mode="profile"

    log_info "Building hello_world in $build_mode mode"
    local build_cmd="$flutter_bin build hap --$build_mode -t lib/main_ohos.dart"
    run_cmd "(cd $hello_world_dir && $build_cmd)"

    local embedding_dir="./engine/src/flutter/shell/platform/ohos/flutter_embedding/flutter"
    local libs_src="$hello_world_dir/ohos/entry/build/default/intermediates/libs/default/arm64-v8a"
    local libs_dst="$embedding_dir/libs/arm64-v8a"
    local assets_src="$hello_world_dir/ohos/entry/src/main/resources/rawfile/flutter_assets"
    local assets_dst="$embedding_dir/src/ohosTest/resources/rawfile/flutter_assets"

    if [[ -d "$libs_src" ]]; then
        log_info "Staging hello_world native libs into $libs_dst"
        run_cmd "mkdir -p $libs_dst"
        run_cmd "cp $libs_src/*.so $libs_dst/"
    else
        log_error "hello_world native libs not found: $libs_src"
        exit 1
    fi

    if [[ -d "$assets_src" ]]; then
        log_info "Staging hello_world flutter_assets into $assets_dst"
        run_cmd "mkdir -p $assets_dst"
        run_cmd "cp -R $assets_src/. $assets_dst/"
    else
        log_error "hello_world flutter_assets not found: $assets_src"
        exit 1
    fi
}

build_hello_world_for_arkts_test "$@"