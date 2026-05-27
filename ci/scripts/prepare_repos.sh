#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

prepare_repos() {
    REPO_CACHE_DIR=${REPO_CACHE_DIR:-/home/tools/Flutter/repo}
    FLUTTERTPC_REPOS=(
        fluttertpc_angle
        fluttertpc_boringssl_gen
        fluttertpc_buildroot
        fluttertpc_dart_native
        fluttertpc_dart_sdk
        fluttertpc_libcxx
        fluttertpc_libcxxabi
        fluttertpc_skia
        fluttertpc_spirv-headers
        fluttertpc_swiftshader
        fluttertpc_vulkan-deps
        fluttertpc_vulkan-headers
        fluttertpc_zlib
    )

    if [[ ! -d "$REPO_CACHE_DIR" ]]; then
        log_error "Repo cache directory does not exist: $REPO_CACHE_DIR"
        exit 1
    fi

    cd "$REPO_CACHE_DIR" || {
        log_error "Failed to cd to repo cache directory"
        exit 1
    }

    log_info "Fetching FLUTTERTPC repositories"

    local success_count=0
    local fail_count=0

    for repo in "${FLUTTERTPC_REPOS[@]}"; do
        if [[ -d "$repo" ]]; then
            log_info "Fetching: $repo"
            if (cd "$repo" && git fetch origin); then
                ((success_count++))
            else
                log_error "Failed to fetch: $repo"
                ((fail_count++))
            fi
        else
            log_warn "Repository does not exist: $repo"
            ((fail_count++))
        fi
    done

    log_info "Fetch summary: $success_count succeeded, $fail_count failed"

    log_info "Repo cache statistics"
    run_cmd "ls -al $REPO_CACHE_DIR"
    local repo_count=$(ls -1A "$REPO_CACHE_DIR" | wc -l)
    log_info "Total repo cache entries: $repo_count"
}

prepare_repos
