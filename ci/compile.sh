#! /bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
#
# USE IN CI
# compileCMD：sh ./third_party/flutter_flutter/ci/compile.sh

ROOT_DIR=$(pwd)
# Project directory
PROJECT_DIR="$ROOT_DIR/third_party"
# Engine directory
ENGINE_DIR="$PROJECT_DIR/flutter_flutter/engine"
# Archive directory
ARCHIVE_DIR="$ROOT_DIR/Archive/out"
# Build mode, randomly select one from debug, profile and release
MODES=("debug" "profile" "release")
BUILD_MODE=${MODES[$RANDOM % ${#MODES[@]}]}

# Target branch
TARGET_FLUTTER_BRANCH="oh-3.35.7-dev"

# Check environment
function check_env() {
    echo "Check environment"
    # Set environment variables
    # command-line-tools
    export TOOL_HOME=/home/tools/command-line-tools
    export DEVECO_SDK_HOME=$TOOL_HOME/sdk
    export PATH=$DEVECO_SDK_HOME/default/openharmony/toolchains:$TOOL_HOME/ohpm/bin:$TOOL_HOME/hvigor/bin:$TOOL_HOME/tool/node/bin:$PATH
    # Flutter
    export PUB_CACHE=/home/tools/Flutter/PUB
    export PUB_HOSTED_URL=https://pub.flutter-io.cn
    export FLUTTER_STORAGE_BASE_URL=https://storage.flutter-io.cn
    # Flutter gclient
    export PATH=/home/tools/depot_tools:$PATH
    export DEPOT_TOOLS_UPDATE=0
    export GCLIENT_SUPPRESS_GIT_VERSION_WARNING=1
    # Flutter cipd
    export CIPD_CACHE_DIR=/home/tools/cipd_cache
    export CIPD_HTTP_USER_AGENT_PREFIX="offline"
    export CIPD_NO_SELF_UPDATE=true
    # llvm
    export PATH=$DEVECO_SDK_HOME/default/openharmony/native/llvm/bin:$PATH
    echo "$ set"
    set
}

# Sync project dependencies
function gclient_sync() {
    echo "Sync project dependencies"
    echo "$ cd $PROJECT_DIR/flutter_flutter"
    cd $PROJECT_DIR/flutter_flutter
    echo "Sync .gclient"
    echo "$ cp -a ./ci/resources/. ."
    cp -a ./ci/resources/. .
    echo "$ ls -al"
    ls -al

    echo "$ gclient sync --ignore-dep-type=cipd -n"
    gclient sync --ignore-dep-type=cipd -n
    if [ $? -ne 0 ]; then
        echo "Failed to execute: gclient sync --ignore-dep-type=cipd -n"
        return 1
    fi

    echo "$ cipd ensure -root . -ensure-file cipd_manifest.txt"
    cipd ensure -root . -ensure-file cipd_manifest.txt
    if [ $? -ne 0 ]; then
        echo "Failed to execute: cipd ensure -root . -ensure-file cipd_manifest.txt"
        return 1
    fi

    # Temporary, replace download URL
    sed -i 's|https://commondatastorage.googleapis.com|file:///home/tools/Flutter/repo/binary|g' $ENGINE_DIR/src/build/linux/sysroot_scripts/install-sysroot.py

    echo "$ gclient runhooks"
    gclient runhooks
    if [ $? -ne 0 ]; then
        echo "Failed to execute: gclient runhooks"
        return 1
    fi

    # Skip unit test module for versions above 3.7
    if [ "$TARGET_FLUTTER_BRANCH" = "dev" ]; then
        echo "No need to skip unit test module"
        return 0
    fi
    sed -i 's|enable_unittests = current_toolchain == host_toolchain \|\| is_fuchsia \|\| is_mac|enable_unittests = false|g' $ENGINE_DIR/src/flutter/testing/testing.gni
}

# Compile engine, randomly select one from debug, profile and release
function compile_engine_random() {
    echo "Build mode: $BUILD_MODE"
    echo "$ cd $ENGINE_DIR"
    cd $ENGINE_DIR

    echo "Start compiling engine"
    echo "$ ./ohos -t $BUILD_MODE"
    ./ohos -t $BUILD_MODE
    if [ $? -ne 0 ]; then
        echo "Failed to execute: ./ohos -t $BUILD_MODE"
        return 1
    fi

    # 3.7 needs to compile host additionally
    if [ "$TARGET_FLUTTER_BRANCH" = "dev" ]; then
        echo "$ ./ohos -t $BUILD_MODE -n host"
        ./ohos -t $BUILD_MODE -n host
        if [ $? -ne 0 ]; then
            echo "Failed to execute: ./ohos -t $BUILD_MODE -n host"
            return 1
        fi
    fi

    # Archive
    (cp -a $ENGINE_DIR/src/out/. $ARCHIVE_DIR &)
}

# Compile engine, full build
function compile_engine_all() {
    echo "Compile engine, full build"
    echo "$ cd $ENGINE_DIR"
    cd $ENGINE_DIR

    echo "Start compiling engine"
    if [ "$TARGET_FLUTTER_BRANCH" = "dev" ]; then
        # 3.7 needs to compile host additionally
        echo "$ ./ohos && ./ohos -n host && ./ohos --ohos-cpu x64"
        ./ohos && ./ohos -n host && ./ohos --ohos-cpu x64
    else
        echo "$ ./ohos && ./ohos --ohos-cpu x64"
        ./ohos && ./ohos --ohos-cpu x64
    fi
    if [ $? -ne 0 ]; then
        echo "Engine compilation failed"
        return 1
    fi

    # Archive
    (cp -a $ENGINE_DIR/src/out/. $ARCHIVE_DIR &)
}

# Pack SDK
function pack_flutter() {
    echo "Pack SDK"
    echo "$ cd $PROJECT_DIR/flutter_flutter"
    cd $PROJECT_DIR/flutter_flutter
    echo "$ zip -r $ARCHIVE_DIR/flutter.ohos.zip *"
    zip -r $ARCHIVE_DIR/sdk-$TARGET_FLUTTER_BRANCH.zip *
}

# Compile Tester
function compile_tester() {
    echo "Compile Tester"
    # Check flutter environment
    export PATH=$PROJECT_DIR/flutter_flutter/bin:$PATH
    echo "$ echo \$PATH"
    echo $PATH
    echo "$ flutter doctor -v"
    flutter doctor -v

    echo "$ cd $PROJECT_DIR/flutter_tester"
    cd $PROJECT_DIR/flutter_tester
    if [ "$TARGET_FLUTTER_BRANCH" = "dev" ]; then
        # 3.7 does not need --local-engine-host
        echo "$ flutter build hap --$BUILD_MODE --local-engine-src-path=$ENGINE_DIR/src --local-engine=ohos_${BUILD_MODE}_arm64"
        flutter build hap --$BUILD_MODE --local-engine-src-path=$ENGINE_DIR/src --local-engine=ohos_${BUILD_MODE}_arm64
    else
        echo "$ flutter build hap --$BUILD_MODE --local-engine-src-path=$ENGINE_DIR/src --local-engine=ohos_${BUILD_MODE}_arm64 --local-engine-host=host_$BUILD_MODE"
        flutter build hap --$BUILD_MODE --local-engine-src-path=$ENGINE_DIR/src --local-engine=ohos_${BUILD_MODE}_arm64 --local-engine-host=host_$BUILD_MODE
    fi
    # Archive
    cp $PROJECT_DIR/flutter_tester/ohos/entry/build/default/outputs/default/entry-default-unsigned.hap $ARCHIVE_DIR/entry-default-unsigned.hap
    if [ $? -ne 0 ]; then
        echo "Failed to execute: flutter build hap --$BUILD_MODE"
        return 1
    fi
}

# Upload to obs
function upload_to_obs() {
    echo "Upload to obs"
    # To be done
}

function compile() {
    echo "Start compilation"
    check_env

    pack_flutter
    if [ $? -ne 0 ]; then
        echo "Failed to execute: pack_flutter"
        return 1
    fi

    gclient_sync
    if [ $? -ne 0 ]; then
        echo "Failed to execute: gclient_sync"
        return 1
    fi

    if [ -z "${PR_URL}" ]; then
        # PR_URL is empty, indicates daily build, needs full compilation
        echo "PR_URL is empty, indicates daily build, needs full compilation"
        compile_engine_all
    else
        # Gatekeeper
        echo "PR_URL is not empty, indicates gatekeeper build, needs random compilation"
        compile_engine_random
    fi
    if [ $? -ne 0 ]; then
        echo "Engine compilation failed"
        return 1
    fi
    
    compile_tester
    if [ $? -ne 0 ]; then
        echo "Failed to execute: compile_tester"
        return 1
    fi
    echo "Compilation stage completed"
}

compile $@
if [ $? -ne 0 ]; then
    # Delete in background, src folder has been polluted
    (rm -rf $ENGINE_DIR/src &)
    echo "Compilation stage failed"
    exit 1
fi
exit 0
