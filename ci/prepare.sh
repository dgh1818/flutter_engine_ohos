#! /bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
#
# USE IN CI
# preCompile：sh ./third_party/flutter_flutter/ci/prepare.sh

ROOT_DIR=$(pwd)
# Project directory
PROJECT_DIR="$ROOT_DIR/third_party"
# Engine directory
ENGINE_DIR="$PROJECT_DIR/flutter_flutter/engine"
# Archive directory
ARCHIVE_DIR="$ROOT_DIR/Archive/out"
# Backup directory
BACKUP_DIR="$ROOT_DIR/Backup"
# repo cache repository
REPO_CACHE_DIR="/home/tools/Flutter/repo"
# fluttertpc repository list
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

# 目标分支
TARGET_FLUTTER_BRANCH="oh-3.35.7-dev"
TARGET_TESTER_BRANCH="main"

# Check environment
function check_env() {
    echo "Check environment"
    echo "$ uname -a"
    uname -a
    echo "$ cat /etc/os-release"
    cat /etc/os-release
    echo "$ id -un"
    id -un
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
    echo "$ env"
    env
    # set
    echo "Check tools"
    echo "$ node -v"
    node -v
    echo "$ npm -v"
    npm -v
    echo "$ ohpm -v"
    ohpm -v
    echo "$ hvigorw -v"
    hvigorw -v
    echo "$ hdc -v"
    hdc -v
    echo "$ git --version"
    git --version
    git config --global user.name "Flutter CI"
    git config --global user.email "flutter_ci@huawei.com"
    echo "$ git config -l"
    git config -l
    echo "$ java -version"
    java -version
    echo "$ Check network"
    echo "$ curl -s -o /dev/null -w \"%{http_code}\n\" -m 5 $PUB_HOSTED_URL"
    curl -s -o /dev/null -w "%{http_code}\n" -m 5 $PUB_HOSTED_URL
    echo "$ curl -s -o /dev/null -w \"%{http_code}\n\" -m 5 $FLUTTER_STORAGE_BASE_URL"
    curl -s -o /dev/null -w "%{http_code}\n" -m 5 $FLUTTER_STORAGE_BASE_URL
    echo "$ curl -s -o /dev/null -w \"%{http_code}\n\" -m 5 https://chrome-infra-packages.appspot.com/prpc/cipd.Repository/GetInstanceURL"
    curl -s -o /dev/null -w "%{http_code}\n" -m 5 https://chrome-infra-packages.appspot.com/prpc/cipd.Repository/GetInstanceURL
    echo "Check project files"
    echo "$ pwd"
    pwd
    echo "$ cd $PROJECT_DIR"
    cd $PROJECT_DIR
    echo "$ ls -al"
    ls -al
    mkdir -p $ARCHIVE_DIR
    mkdir -p $BACKUP_DIR
}

function patch_cipd() {
    echo "Patch cipd"
    echo "$ cd $PROJECT_DIR"
    cd $PROJECT_DIR
    echo "$ git clone -b main https://gitcode.com/xiedrsz/cipd.git"
    git clone -b main https://gitcode.com/xiedrsz/cipd.git
    echo "$ cd ./cipd && ./patch_cipd.sh"
    cd ./cipd && ./patch_cipd.sh
}

# Sync cache
function sync_cache() {
    echo "Sync cache"
    # Refresh CIPD cache time to avoid cache expiration
    echo "$ find $CIPD_CACHE_DIR -type f -exec touch {} +"
    find $CIPD_CACHE_DIR -type f -exec touch {} +
    echo "$ rm $CIPD_CACHE_DIR/instances/state.db"
    rm $CIPD_CACHE_DIR/instances/state.db
    patch_cipd
    # Refresh FLUTTERTPC repositories
    echo "$ cd $REPO_CACHE_DIR"
    cd $REPO_CACHE_DIR
    for repo in "${FLUTTERTPC_REPOS[@]}"; do
        echo "Fetching in $repo"
        (cd "$repo" && git fetch --all)
    done
    echo "$ Check cache"
    echo "$ ls -al $REPO_CACHE_DIR"
    ls -al $REPO_CACHE_DIR
    echo "$ ls -1A \"$REPO_CACHE_DIR\" | wc -l"
    ls -1A "$REPO_CACHE_DIR" | wc -l
    echo "$ ls -al $CIPD_CACHE_DIR/instances"
    ls -al $CIPD_CACHE_DIR/instances
    echo "$ ls -1A \"$CIPD_CACHE_DIR/instances\" | wc -l"
    ls -1A "$CIPD_CACHE_DIR/instances" | wc -l
}

# restore engine
function maybe_restore_engine() {
    if [ ! -d $ENGINE_DIR/src/flutter ]; then
        cp -a $BACKUP_DIR/src $ENGINE_DIR
    fi
}

# Prepare SDK
function prepare_flutter() {
    echo "Prepare project files"
    echo "Rebase"
    echo "$ cd $PROJECT_DIR/flutter_flutter"
    cd $PROJECT_DIR/flutter_flutter
    echo "$ git fetch --all"
    git fetch --all
    echo "$ git branch -a"
    git branch -a
    echo "$ git rebase remotes/gitcode/$TARGET_FLUTTER_BRANCH"
    git rebase remotes/gitcode/$TARGET_FLUTTER_BRANCH
    if [ $? -ne 0 ]; then
        echo "rebase failed!!!"
        return 1
    fi
    git log -10 --pretty=format:"%h - %s"
    git status
    git diff

    # Get commit id
    COMMIT_ID=$(git rev-parse HEAD)
    echo "COMMIT_ID: $COMMIT_ID"
    # Write commit id file
    echo $COMMIT_ID > $ARCHIVE_DIR/engine.ohos.har.version
    echo $COMMIT_ID > $ARCHIVE_DIR/engine.ohos.version

    # Backup (gclient sync may delete the flutter folder in src or pollute the src folder)
    echo "$ rm -rf $BACKUP_DIR/src"
    rm -rf $BACKUP_DIR/src
    echo "$ cp -a $ENGINE_DIR/src $BACKUP_DIR/src"
    cp -a $ENGINE_DIR/src $BACKUP_DIR/src
}

# Prepare Tester
function prepare_tester() {
    echo "Prepare Tester"
    echo "$ cd $PROJECT_DIR/flutter_tester"
    cd $PROJECT_DIR/flutter_tester
    if [ $? -ne 0 ]; then
        echo "flutter_tester does not exist"
        return 1
    fi
    echo "$ git branch -a"
    git branch -a
    echo "$ git checkout $TARGET_TESTER_BRANCH"
    git checkout $TARGET_TESTER_BRANCH
    echo "$ git reset --hard"
    git reset --hard
    echo "$ git pull --rebase"
    git pull --rebase
}

# Entry
function prepare() {
    check_env
    sync_cache
    maybe_restore_engine

    prepare_flutter
    if [ $? -ne 0 ]; then
        echo "Failed to execute: prepare_flutter"
        return 1
    fi

    prepare_tester
    if [ $? -ne 0 ]; then
        echo "Failed to execute: prepare_tester"
        return 1
    fi

    echo "Preparation stage completed"
}

prepare $@
exit $?
