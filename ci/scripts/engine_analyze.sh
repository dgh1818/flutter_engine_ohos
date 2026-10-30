#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
#
# OHOS-adapted engine Dart analysis.
#
# Unlike the upstream analyze.sh, this wrapper skips Web SDK related
# steps (pub get for web_ui/web_sdk, and Web SDK integrity tests) since
# OHOS does not use the Web backend. It runs:
#   1. pub_get_offline.py  - resolve dependencies for non-web packages
#   2. dart analyze        - static analysis on the engine source tree
#
# Web directories (lib/web_ui, web_sdk) are excluded from analysis via
# analysis_options.yaml.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

engine_analyze() {
    WORK_DIR=$(pwd)
    PROJECT_DIR="$WORK_DIR/third_party"
    ENGINE_SRC="$PROJECT_DIR/flutter_flutter/engine/src"
    FLUTTER_DIR="$ENGINE_SRC/flutter"

    if [[ ! -d "$FLUTTER_DIR" ]]; then
        log_error "Engine flutter directory not found: $FLUTTER_DIR"
        exit 1
    fi

    # Locate a usable Dart SDK.
    # Preference order:
    #   1. out/host_release/dart-sdk/bin  (built by OHOS CI)
    #   2. out/host_debug_unopt/dart-sdk/bin  (upstream default)
    #   3. flutter/third_party/dart/tools/sdks/dart-sdk/bin  (bundled)
    local dart_bin=""
    for candidate in \
        "$ENGINE_SRC/out/host_release/dart-sdk/bin" \
        "$ENGINE_SRC/out/host_debug_unopt/dart-sdk/bin" \
        "$ENGINE_SRC/out/ci/host_debug_unopt/dart-sdk/bin" \
        "$FLUTTER_DIR/third_party/dart/tools/sdks/dart-sdk/bin"; do
        if [[ -f "$candidate/dart" ]]; then
            dart_bin="$candidate"
            break
        fi
    done

    if [[ -z "$dart_bin" ]]; then
        log_error "No Dart SDK found. Build the engine first or ensure a dart-sdk is available."
        exit 1
    fi

    local dart="$dart_bin/dart"
    log_info "Using Dart SDK from: $dart_bin"
    "$dart" --version

    # Step 1: Resolve dependencies for non-web packages via pub_get_offline.py.
    # This covers all packages listed in pub_get_offline.py's ALL_PACKAGES.
    # Web packages (web_ui, web_sdk) are intentionally not included.
    log_info "Resolving dependencies (pub_get_offline.py)"
    local pub_get_script="$FLUTTER_DIR/tools/pub_get_offline.py"
    if [[ -f "$pub_get_script" ]]; then
        if ! (cd "$FLUTTER_DIR" && python3 "$pub_get_script"); then
            log_error "pub_get_offline.py failed"
            exit 1
        fi
    else
        log_warn "pub_get_offline.py not found, skipping dependency resolution"
    fi

    # Step 2: Run dart analyze on the engine source tree.
    # Web directories are excluded via analysis_options.yaml.
    log_info "Analyzing the Flutter engine (excluding Web SDK)"
    if ! "$dart" analyze --suppress-analytics --fatal-infos --fatal-warnings "$FLUTTER_DIR"; then
        log_error "Engine dart analyze failed"
        exit 1
    fi

    log_info "Engine analyze completed successfully"
}

engine_analyze
