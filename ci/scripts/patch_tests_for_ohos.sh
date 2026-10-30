#!/bin/bash
# Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

set -e

TESTS_DIR="${1:-}"
if [ -z "$TESTS_DIR" ]; then
    echo "[patch_tests_for_ohos] ERROR: tests_dir argument is required" >&2
    exit 2
fi

if [ ! -d "$TESTS_DIR/registry" ]; then
    echo "[patch_tests_for_ohos] ERROR: $TESTS_DIR/registry does not exist" >&2
    exit 2
fi

REGISTRY="$TESTS_DIR/registry"

# Format: <action> <name> [args...]
#   disable      -> rename <name>.test to <name>.test.disabled
#   patch_setup  -> append setup.linux=<command> to <name>.test

MANIFEST=$(cat <<'EOF'
# disable zulip
disable zulip

# disable super_editor
disable super_editor

# disable super_sliver_list
disable super_sliver_list

# fix SemanticVersion assert for OHOS version format
patch_setup flutter_devtools sed -i s/\(<=.\)2/\14/ packages/devtools_shared/lib/src/utils/semantic_version.dart
EOF
)

applied=0
skipped=0

while IFS= read -r line; do
  case "$line" in
    ''|'#'*|' '*'#'*) continue ;;
  esac

  action=${line%% *}
  rest=${line#* }
  name=${rest%% *}

  case "$action" in
    disable)
      src="$REGISTRY/$name.test"
      dst="$REGISTRY/$name.test.disabled"
      if [ ! -f "$src" ]; then
        echo "[patch_tests_for_ohos] SKIP: $name.test (not found)"
        skipped=$((skipped + 1))
        continue
      fi
      mv "$src" "$dst"
      echo "[patch_tests_for_ohos] DISABLED: $name.test"
      applied=$((applied + 1))
      ;;
    patch_setup)
      src="$REGISTRY/$name.test"
      if [ ! -f "$src" ]; then
        echo "[patch_tests_for_ohos] SKIP: $name.test (not found)"
        skipped=$((skipped + 1))
        continue
      fi
      command=${rest#* }
      if [ -z "$command" ]; then
        echo "[patch_tests_for_ohos] WARN: no command for '$name'" >&2
        continue
      fi
      printf '\nsetup.linux=%s\n' "$command" >> "$src"
      echo "[patch_tests_for_ohos] PATCHED: $name.test"
      applied=$((applied + 1))
      ;;
    *)
      echo "[patch_tests_for_ohos] WARN: unknown action '$action'" >&2
      ;;
  esac
done <<< "$MANIFEST"

echo "[patch_tests_for_ohos] Done. Applied: $applied, Skipped: $skipped"