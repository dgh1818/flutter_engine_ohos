#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

# Common utility library - for unit test scripts

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

# Get shared PUB_CACHE path
get_shared_pub_cache() {
    local pub_cache_file="$WORK_DIR/Archive/out/unit_tests_logs/pub_cache_path.txt"
    if [ -f "$pub_cache_file" ]; then
        cat "$pub_cache_file"
    else
        echo "$WORK_DIR/pub_cache_shared"
    fi
}

# Parse test parameters (path:command format)
parse_test_spec() {
    local test_spec="$1"
    local default_cmd="$2"
    
    if [[ "$test_spec" == *:* ]]; then
        local path="${test_spec%%:*}"
        local cmd="${test_spec#*:}"
        echo "$path|$cmd"
    else
        echo "$test_spec|$default_cmd"
    fi
}

# Write log
write_log() {
    local log_file="$1"
    local level="$2"
    local message="$3"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo "[$timestamp] [$level] $message" >> "$log_file"
}

# Clean Flutter lock files
clean_flutter_locks() {
    local flutter_root="$1"
    local locks=(
        "$flutter_root/bin/cache/.dart_tool_state"
        "$flutter_root/.flutter_tool_state"
    )
    
    for lock_file in "${locks[@]}"; do
        if [ -f "$lock_file" ]; then
            rm -f "$lock_file"
            log_info "Removed: $lock_file"
        fi
    done
}

# Initialize a single package (with timeout)
init_package() {
    local pkg_path="$1"
    local timeout_sec="${2:-120}"
    local log_file="$3"
    
    if [ -d "$pkg_path" ]; then
        log_info "Initializing: $(basename "$pkg_path")"
        timeout $timeout_sec bash -c "cd '$pkg_path' && flutter pub get" >> "$log_file" 2>&1
        return $?
    else
        log_warn "Package not found: $pkg_path"
        return 1
    fi
}
