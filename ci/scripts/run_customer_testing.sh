#!/bin/bash
# Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

# Run upstream customer_testing suite with OHOS-specific patches applied.
#
# Workflow:
#   1. Clone flutter/tests @ tests.version (from github.com, upstream canonical)
#   2. Apply OHOS patches via patch_tests_for_ohos.sh
#   3. Sync missing commits into mirror repos (e.g. packages/)
#   4. Resolve customer_testing tool deps
#   5. Run upstream run_tests.dart directly with registry/*.test glob
#
# Exit codes:
#   0 - all (non-disabled) tests passed
#   1 - one or more tests failed
#   2 - environment/setup error (e.g., git clone failed)

set -e

export TZ=UTC

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
TESTS_DIR="$ROOT_DIR/bin/cache/pkg/tests"
REGISTRY="$TESTS_DIR/registry"
SHA_FILE="$ROOT_DIR/dev/customer_testing/tests.version"
RUNNER="$ROOT_DIR/dev/customer_testing/run_tests.dart"

log() { echo "[customer_testing] $*"; }
err() { echo "[customer_testing] ERROR: $*" >&2; }

# --- Step 1: Clone flutter/tests @ pinned SHA ---
if [ ! -f "$SHA_FILE" ]; then
  err "tests.version not found at $SHA_FILE"
  exit 2
fi
SHA=$(cat "$SHA_FILE" | tr -d '[:space:]')

log "Cloning flutter/tests @ $SHA (from github.com)"
rm -rf "$TESTS_DIR"
cd "$ROOT_DIR"
git clone --depth 1 "https://github.com/flutter/tests.git" "$TESTS_DIR"
git -C "$TESTS_DIR" fetch origin "$SHA"
git -C "$TESTS_DIR" checkout "$SHA"

# --- Step 2: Apply OHOS patches ---
log "Applying OHOS patches"
"$SCRIPT_DIR/patch_tests_for_ohos.sh" "$TESTS_DIR"

# --- Step 3: Sync missing commits into mirror repos ---
# Remove this step once packages/ mirror is updated in the Docker image
#       and flutter_packages/ is removed.
REPO_CACHE_DIR="${REPO_CACHE_DIR:-/home/tools/Flutter/repo}"
PACKAGES_MIRROR="$REPO_CACHE_DIR/packages"
FLUTTER_PACKAGES_MIRROR="$REPO_CACHE_DIR/flutter_packages"
if [ -d "$FLUTTER_PACKAGES_MIRROR" ] && [ -d "$PACKAGES_MIRROR" ]; then
  log "Syncing commits from flutter_packages to packages mirror"
  commits=$(git -C "$FLUTTER_PACKAGES_MIRROR" cat-file --batch-all-objects --batch-check 2>/dev/null \
    | grep ' commit ' | awk '{print $1}')
  if [ -n "$commits" ]; then
    git -C "$PACKAGES_MIRROR" fetch --update-shallow \
      "$FLUTTER_PACKAGES_MIRROR" $commits 2>/dev/null \
      || log "WARN: sync from flutter_packages may have partially failed"
  fi
fi

# --- Step 4: Resolve customer_testing tool deps ---
cd "$ROOT_DIR/dev/customer_testing"
if ! flutter pub get; then
  err "flutter pub get failed; check network/git access"
  exit 2
fi

# --- Step 5: Run upstream run_tests.dart with registry/*.test glob ---
# run_tests.dart resolves args as Globs; pass registry/*.test so we pick up
# every .test file (post-patch, the disabled ones have .test.disabled suffix
# and are naturally skipped by the glob).
cd "$REGISTRY"
log "Running upstream run_tests.dart with registry/*.test"
exec dart --enable-asserts "$RUNNER" --skip-template '*.test'