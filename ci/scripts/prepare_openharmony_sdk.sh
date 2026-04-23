#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

prepare_openharmony_sdk() {
    local sdk_dir="/home/tools/command-line-tools/sdk/default"
    local sdk_url="$1"
    local correct_sha256="$2"

    cd "$sdk_dir" || {
        log_error "Failed to cd to $sdk_dir"
        exit 1
    }

    log_info "Cleaning old SDK"
    rm -rf openharmony download 2>/dev/null
    mkdir -p download

    log_info "Downloading daily build SDK"
    log_info "URL: $sdk_url"

    curl -f -L -- "$sdk_url" > download/sdk_openharmony.tar.gz 
    if [ $? -ne 0 ]; then
        log_error "Download failed"
        exit 1
    fi
    log_info "Download completed successfully"

    log_info "Verifying SDK integrity"
    local sdk_sum=$(sha256sum "download/sdk_openharmony.tar.gz" | cut -d' ' -f1)
    if [[ "$sdk_sum" != "$correct_sha256" ]]; then
        log_error "SHA256 mismatch!"
        log_error "Expected: $correct_sha256"
        log_error "Got:      $sdk_sum"
        exit 1
    fi
    log_info "SDK verification passed"

    log_info "Extracting SDK"
    cd download || exit 1
    if ! tar -xzf "sdk_openharmony.tar.gz"; then
        log_error "Failed to extract SDK tarball"
        exit 1
    fi

    cd linux || {
        log_error "linux directory not found in extracted files"
        exit 1
    }

    log_info "Extracting zip files"
    local success_count=0
    local fail_count=0
    for zipfile in *.zip; do
        if [[ ! -f "$zipfile" ]]; then
            continue
        fi

        if unzip -qo "$zipfile"; then
            log_info "✓ Extracted: $zipfile"
            rm -f "$zipfile"
            ((success_count++))
        else
            log_error "✗ Failed to extract: $zipfile"
            ((fail_count++))
        fi
    done

    if [[ $fail_count -gt 0 ]]; then
        log_error "Failed to extract $fail_count file(s)"
        exit 1
    fi

    log_info "Successfully extracted $success_count file(s)"

    cd "$sdk_dir" || exit 1
    mv download/linux openharmony || {
        log_error "Failed to rename download/linux to openharmony"
        exit 1
    }

    log_info "SDK patch completed successfully"
}

prepare_openharmony_sdk "$@"
