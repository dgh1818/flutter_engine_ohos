#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
#
# USE IN CI
# testCMD：sh ./third_party/flutter_flutter/ci/test.sh

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RUNNER="$SCRIPT_DIR/scripts/runner.py"

# Check if Python 3 is available
if ! command -v python3 &> /dev/null; then
    echo "[ERROR] Python 3 is required but not installed."
    exit 1
fi

# Check if PyYAML is installed
python3 -c "import yaml" 2>/dev/null || {
    echo "[ERROR] PyYAML is not installed. Run: pip install pyyaml"
    exit 1
}

# Run the Python runner
python3 "$RUNNER" "test"
