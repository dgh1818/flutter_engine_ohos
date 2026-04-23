#! /bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
#
# USE IN CI
# compileCMD：sh ./third_party/flutter_flutter/ci/compile.sh

set -e  # Exit on error
set -u  # Exit on undefined variable

# Logging functions
log_info() {
    echo "[INFO] $*"
}

log_warn() {
    echo "[WARN] $*"
}

log_error() {
    echo "[ERROR] $*" >&2
}

log_step() {
    echo "\n========================================"
    echo "$*"
    echo "========================================\n"
}

# Command execution wrapper
run_cmd() {
    local cmd="$1"
    echo "$ $cmd"
    eval "$cmd"
}

# Cleanup on error
function cleanup_on_error() {
    local exit_code=$?
    if [[ $exit_code -ne 0 ]]; then
        log_error "Compilation failed with exit code: $exit_code"
    fi
}

trap cleanup_on_error EXIT

ROOT_DIR=$(pwd)
# Project directory
PROJECT_DIR="$ROOT_DIR/third_party"
# Engine directory
ENGINE_DIR="$PROJECT_DIR/flutter_flutter/engine"
# Archive directory
ARCHIVE_DIR="$ROOT_DIR/Archive/out"
# Build mode, randomly select one from debug, profile and release
readonly MODES=("debug" "profile" "release")
readonly BUILD_MODE=${MODES[$RANDOM % ${#MODES[@]}]}

# Target branch
readonly TARGET_FLUTTER_BRANCH="oh-3.35.7-dev"

# Set environment variables
function setup_env_vars() {
    log_step "Setting up environment variables"
    # command-line-tools
    export TOOL_HOME=/home/tools/command-line-tools
    export DEVECO_SDK_HOME=$TOOL_HOME/sdk
    # Flutter
    export PUB_CACHE=/home/tools/Flutter/PUB
    export PUB_HOSTED_URL=https://pub.flutter-io.cn
    export FLUTTER_STORAGE_BASE_URL=https://storage.flutter-io.cn
    # Flutter gclient
    export DEPOT_TOOLS_UPDATE=0
    export GCLIENT_SUPPRESS_GIT_VERSION_WARNING=1
    # Flutter cipd
    export CIPD_CACHE_DIR=/home/tools/cipd_cache
    export CIPD_HTTP_USER_AGENT_PREFIX="offline"
    export CIPD_NO_SELF_UPDATE=true

    # Build PATH in correct order
    local path_components=(
        "$PROJECT_DIR/flutter_flutter/bin"
        "$DEVECO_SDK_HOME/default/openharmony/toolchains"
        "$TOOL_HOME/ohpm/bin"
        "$TOOL_HOME/hvigor/bin"
        "$TOOL_HOME/tool/node/bin"
        "/home/tools/depot_tools"
        "$DEVECO_SDK_HOME/default/openharmony/native/llvm/bin"
        "$PROJECT_DIR/cipd/bin"
        "$PATH"
    )
    export PATH=$(IFS=:; echo "${path_components[*]}")
}

# Check environment
function check_env() {
    log_info "Checking Environment"
    setup_env_vars
    run_cmd "set"
}

# Replace download URL in sysroot scripts
function replace_download_url() {
    log_info "Replacing download URL in sysroot scripts"
    local sysroot_file="$ENGINE_DIR/src/build/linux/sysroot_scripts/install-sysroot.py"

    if [[ -f "$sysroot_file" ]]; then
        run_cmd "sed -i 's|https://commondatastorage.googleapis.com|file:///home/tools/Flutter/repo/binary|g' $sysroot_file"
    fi
}

# Skip unit test module
function skip_unittests() {
    local testing_gni="$ENGINE_DIR/src/flutter/testing/testing.gni"
    if [[ -f "$testing_gni" ]]; then
        log_info "Skipping unit test module"
        run_cmd "sed -i 's|enable_unittests = current_toolchain == host_toolchain \|\| is_fuchsia \|\| is_mac|enable_unittests = false|g' $testing_gni"
    fi
}

# Sync project dependencies
function gclient_sync() {
    log_step "Syncing Project Dependencies"

    cd "$PROJECT_DIR/flutter_flutter"

    if [[ -d ./ci/resources ]]; then
        run_cmd "cp -a ./ci/resources/. ."
    fi
    run_cmd "ls -la"

    log_info "Running gclient sync"
    if ! run_cmd "gclient sync --ignore-dep-type=cipd -n"; then
        log_error "gclient sync failed"
        return 1
    fi

    log_info "Ensuring CIPD packages"
    if ! run_cmd "cipd ensure -root . -ensure-file cipd_manifest.txt"; then
        log_error "CIPD ensure failed"
        return 1
    fi

    replace_download_url

    log_info "Running gclient hooks"
    if ! run_cmd "gclient runhooks"; then
        log_error "gclient runhooks failed"
        return 1
    fi

    skip_unittests

    log_info "Project dependencies synced successfully"
}

# Archive engine output
function archive_output() {
    log_info "Archiving engine output"
    if [[ -d "$ENGINE_DIR/src/out" ]]; then
        (cp -a "$ENGINE_DIR/src/out/." "$ARCHIVE_DIR/out" &)
    else
        log_warn "Engine output directory does not exist: $ENGINE_DIR/src/out"
    fi
}

# Compile engine, randomly select one from debug, profile and release
function compile_engine_random() {
    log_step "Compiling Engine (Random Mode: $BUILD_MODE)"

    cd "$ENGINE_DIR"

    log_info "Compiling engine in $BUILD_MODE mode"
    if ! run_cmd "./ohos -t $BUILD_MODE"; then
        log_error "Engine compilation failed for mode: $BUILD_MODE"
        return 1
    fi

    archive_output
    log_info "Engine compilation completed successfully"
}

# Compile engine, full build
function compile_engine_all() {
    log_step "Compiling Engine (Full Build)"

    cd "$ENGINE_DIR"

    log_info "Starting full engine compilation"
    if ! run_cmd "./ohos && ./ohos --ohos-cpu x64"; then
        log_error "Full engine compilation failed"
        return 1
    fi

    # Archive with mtime preservation
    log_info "Saving file modification times"
    cd src || {
        log_error "Failed to cd to src directory"
        return 1
    }

    if [[ -d "out" ]]; then
        run_cmd "save_mtime out $ARCHIVE_DIR/restore_mtimes.sh"
        (cp -a out/. "$ARCHIVE_DIR/out" &)
    fi

    log_info "Full engine compilation completed successfully"
}

# Check if it's Friday
function is_friday() {
    [[ "$(date +%u)" -eq 5 ]]
}

# Sync out artifacts and restore engine mtimes
function restore_engine_mtimes() {
    log_step "Restoring Engine Modification Times"

    # If it's Friday, skip mtime restoration
    if is_friday; then
        log_info "Today is Friday, skipping mtime restoration"
        return 0
    fi

    cd "$ENGINE_DIR/src"

    log_info "Initializing archive"
    run_cmd "archive init"

    log_info "Downloading engine version file"
    run_cmd "archive cp cloud://$TARGET_FLUTTER_BRANCH/engine.ohos.version engine.ohos.version"

    if [[ ! -f "engine.ohos.version" ]]; then
        log_warn "Engine version file not found, skipping mtime restoration"
        return 0
    fi

    local commit_id=$(cat engine.ohos.version)
    log_info "Last engine commit ID: $commit_id"

    log_info "Setting old file timestamps"
    run_cmd "find . -type f -exec touch -d '10 days ago' {} +"

    log_info "Touching changed files since $commit_id"
    cd "$PROJECT_DIR/flutter_flutter"
    if ! run_cmd "git diff --name-only --diff-filter=d $commit_id | xargs -r touch"; then
        log_error "Failed to touch changed files"
        return 1
    fi
    cd "$ENGINE_DIR/src"

    log_info "Syncing artifacts from cloud"
    run_cmd "archive sync cloud://$TARGET_FLUTTER_BRANCH/out out"

    log_info "Downloading mtime restoration script"
    run_cmd "archive cp cloud://$TARGET_FLUTTER_BRANCH/restore_mtimes.sh restore_mtimes.sh"

    if [[ -f "restore_mtimes.sh" ]]; then
        log_info "Restoring modification times"
        chmod +x ./restore_mtimes.sh && run_cmd "./restore_mtimes.sh"
    fi

    log_info "Engine modification times restored successfully"
}

# Pack Flutter SDK
function pack_flutter() {
    log_step "Packing Flutter SDK"

    cd "$PROJECT_DIR/flutter_flutter"

    local output_file="$ARCHIVE_DIR/sdk-$TARGET_FLUTTER_BRANCH.zip"

    log_info "Creating SDK archive: $output_file"
    if ! run_cmd "zip -r $output_file *"; then
        log_error "Failed to create SDK archive"
        return 1
    fi

    log_info "SDK packed successfully"
}

# Compile Tester
function compile_tester() {
    log_step "Compiling Tester"

    log_info "Checking Flutter environment"
    run_cmd "flutter doctor -v"

    cd "$PROJECT_DIR/flutter_tester"

    log_info "Building tester in $BUILD_MODE mode"
    local build_cmd="flutter build hap --$BUILD_MODE --local-engine-src-path=$ENGINE_DIR/src --local-engine=ohos_${BUILD_MODE}_arm64 --local-engine-host=host_$BUILD_MODE"
    run_cmd "$build_cmd"

    # Archive HAP file
    local hap_source="$PROJECT_DIR/flutter_tester/ohos/entry/build/default/outputs/default/entry-default-unsigned.hap"
    local hap_dest="$ARCHIVE_DIR/entry-default-unsigned.hap"

    if [[ -f "$hap_source" ]]; then
        log_info "Copying HAP file to archive"
        run_cmd "cp $hap_source $hap_dest"
    else
        log_error "HAP file not found: $hap_source"
        return 1
    fi

    log_info "Tester compiled successfully"
}

# Upload artifacts to cloud
function upload_to_cloud() {
    log_step "Uploading Artifacts to Cloud"

    log_info "Uploading mtimes script"
    run_cmd "archive cp $ARCHIVE_DIR/restore_mtimes.sh cloud://$TARGET_FLUTTER_BRANCH/restore_mtimes.sh"

    log_info "Uploading engine version file"
    run_cmd "archive cp $ARCHIVE_DIR/engine.ohos.version cloud://$TARGET_FLUTTER_BRANCH/engine.ohos.version"

    log_info "Syncing output artifacts"
    run_cmd "archive sync $ARCHIVE_DIR/out cloud://$TARGET_FLUTTER_BRANCH/out"

    log_info "Cloud upload completed successfully"
}

# Main compilation function
function compile() {
    local start_time=$(date +%s)

    log_step "Starting Compilation Pipeline"
    log_info "Build mode: $BUILD_MODE"
    log_info "Target branch: $TARGET_FLUTTER_BRANCH"
    log_info "PR URL: ${PR_URL:-empty (daily build)}"
    log_info "Date: $(date)"

    check_env || {
        log_error "Environment check failed"
        return 1
    }

    pack_flutter || {
        log_error "Flutter packaging failed"
        return 1
    }

    gclient_sync || {
        log_error "Dependencies sync failed"
        return 1
    }

    # Determine build type based on PR_URL
    if [[ -z "${PR_URL:-}" ]]; then
        # PR_URL is empty, indicates daily build, requires full compilation
        log_info "Daily build detected - performing full compilation"
        restore_engine_mtimes || {
            log_error "Engine mtime restoration failed"
            return 1
        }
        compile_engine_all || {
            log_error "Full engine compilation failed"
            return 1
        }
    else
        # Gatekeeper
        log_info "Gatekeeper build detected - performing random compilation"
        compile_engine_random || {
            log_error "Random engine compilation failed"
            return 1
        }
    fi

    compile_tester || {
        log_error "Tester compilation failed"
        return 1
    }

    # Upload to cloud only for daily builds
    if [[ -z "${PR_URL:-}" ]]; then
        upload_to_cloud
    fi

    # Cleanup archive
    log_info "Uninstalling archive"
    run_cmd "archive uninstall"

    local end_time=$(date +%s)
    local duration=$((end_time - start_time))

    log_step "Compilation Completed Successfully"
    log_info "Total duration: ${duration}s"

    return 0
}

# Main entry point
main() {
    compile "$@"
    local exit_code=$?

    if [[ $exit_code -ne 0 ]]; then
        exit $exit_code
    fi

    exit 0
}

main "$@"
