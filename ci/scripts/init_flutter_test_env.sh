#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_DIR="$WORK_DIR/Archive/out/unit_tests_logs"
LOG_FILE="$LOG_DIR/init_env_$TIMESTAMP.log"

# 加载公共函数库
source "$SCRIPT_DIR/test_utils.sh"

# 设置共享 PUB_CACHE
SHARED_PUB_CACHE="$WORK_DIR/pub_cache_shared"
export PUB_CACHE="$SHARED_PUB_CACHE"

mkdir -p "$LOG_DIR"
mkdir -p "$PUB_CACHE"

# 保存 PUB_CACHE 路径
PUB_CACHE_FILE="$LOG_DIR/pub_cache_path.txt"
echo "$SHARED_PUB_CACHE" > "$PUB_CACHE_FILE"

print_log_on_exit() {
    echo "========== LOG FILE CONTENT ($LOG_FILE) =========="
    cat "$LOG_FILE"
    echo "========== END OF LOG FILE =========="
}

trap print_log_on_exit EXIT

log_step "Initializing Flutter Test Environment"
log_info "PUB_CACHE: $SHARED_PUB_CACHE | Log: $LOG_FILE"

log_step "Step 1/3: Pre-warm Flutter tool"
flutter --version >> "$LOG_FILE" 2>&1
exit_code=$?
if [ $exit_code -eq 0 ]; then
    log_info "Flutter tool ready"
else
    log_error "Flutter tool not available (exit code: $exit_code)"
    exit 1
fi

INIT_PACKAGES=(
    "packages/flutter"
    "packages/flutter_test"
    "packages/flutter_tools"
    "packages/flutter_driver"
    "packages/integration_test"
    "packages/flutter_localizations"
    "packages/flutter_goldens"
    "packages/flutter_web_plugins"
    "packages/fuchsia_remote_debug_protocol"
    "dev/a11y_assessments"
    "dev/bots"
    "dev/tools"
    "dev/devicelab"
    "dev/snippets"
    "dev/customer_testing"
    "dev/packages_autoroller"
    "dev/tracing_tests"
    "dev/manual_tests"
    "dev/integration_tests/ui"
    "dev/integration_tests/link_hook"
    "dev/integration_tests/flutter_gallery"
    "dev/integration_tests/hook_user_defines"
)

log_step "Step 2/3: Apply OHOS pubspec and initialize test packages"

OHOS_PUBSPEC="$SCRIPT_DIR/../resources/pubspec_ohos.yaml"
ROOT_PUBSPEC="$SCRIPT_DIR/../../pubspec.yaml"

if [ -f "$OHOS_PUBSPEC" ]; then
    cp "$OHOS_PUBSPEC" "$ROOT_PUBSPEC"
    log_info "Replaced root pubspec.yaml with OHOS version (skipped Android/iOS/macOS/Windows workspace entries)"
else
    log_warn "OHOS pubspec not found at $OHOS_PUBSPEC, using original pubspec.yaml"
fi

for pkg in "${INIT_PACKAGES[@]}"; do
    pkg_path="$SCRIPT_DIR/../../$pkg"
    pkg_name=$(basename "$pkg")

    if ! init_package "$pkg_path" 300 "$LOG_FILE"; then
        log_error "Failed: $pkg_name"
        exit 1
    fi
done
write_log "$LOG_FILE" "INFO" "Initialization completed!"

log_step "Step 3/3: Clean Flutter tool locks"
clean_flutter_locks "$SCRIPT_DIR/../.."

log_step "Complete: all packages initialized"

trap - EXIT
