#!/usr/bin/env python
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

"""Apply OHOS-specific patches to BoringSSL after gclient sync.

This script is called by DEPS hooks to apply patches to third-party
dependencies that are not maintained by the OHOS project.

Supports Linux, macOS, and Windows.
"""

import platform
import subprocess
import sys
from pathlib import Path

# Configuration
# SCRIPT_DIR is engine/src/flutter/tools/, so we need to go up one level
# to reach engine/src/flutter/, then into third_party/boringssl/
SCRIPT_DIR = Path(__file__).parent
FLUTTER_DIR = SCRIPT_DIR.parent  # engine/src/flutter/
PATCHES_DIR = FLUTTER_DIR / "third_party" / "boringssl" / "ohos_patches"
BORINGSSL_SRC = FLUTTER_DIR / "third_party" / "boringssl" / "src"


def _git_apply_cmd(args: list[str], patch_file: Path) -> subprocess.CompletedProcess:
  """Run git apply with graceful fallback for unsupported flags.

  Tries the full command (including platform-specific flags like
  --ignore-cr-at-eol).  If the git binary is too old to recognise that
  flag it exits with code 129 and prints "unknown option"; in that
  case we retry without the platform-specific extras.
  """
  cmd = ["git", "apply"] + args + [str(patch_file)]
  result = subprocess.run(cmd, capture_output=True, text=True, cwd=BORINGSSL_SRC)
  if result.returncode == 129 and "unknown option" in result.stderr:
    # Retry without platform-specific extras (e.g. --ignore-cr-at-eol)
    cmd = ["git", "apply"] + [a for a in args if a != "--ignore-cr-at-eol"] + [str(patch_file)]
    result = subprocess.run(cmd, capture_output=True, text=True, cwd=BORINGSSL_SRC)
  return result


def _skip_worktree_files() -> None:
  """Mark patched files with --skip-worktree so gclient sync won't complain.

  After applying the patch, the BoringSSL source has local modifications.
  gclient sync checks for uncommitted changes and aborts if found.  By
  marking the patched files with --skip-worktree, git ignores the
  modifications and gclient sync proceeds normally.
  """
  # Files modified by the patch
  patched_files = [
      "crypto/x509/x509_vfy.cc",
      "pki/verify_certificate_chain.cc",
  ]
  for f in patched_files:
    file_path = BORINGSSL_SRC / f
    if file_path.exists():
      subprocess.run(
          ["git", "update-index", "--skip-worktree", f],
          capture_output=True,
          cwd=BORINGSSL_SRC,
      )


def is_patch_applied(patch_file: Path) -> bool:
  """Check if a patch is already applied using git apply --reverse --check.

  This is the most reliable cross-platform method:
  - Exit code 0: patch can be reverse-applied = patch IS applied
  - Exit code non-zero: patch cannot be reverse-applied = patch NOT applied

  Returns:
    True if the patch is already applied, False otherwise.
  """
  is_windows = platform.system() == "Windows"
  # --ignore-cr-at-eol handles CRLF vs LF differences on Windows
  extra_args = ["--ignore-cr-at-eol"] if is_windows else []
  cmd = ["--reverse", "--check", "-p1"] + extra_args
  try:
    result = _git_apply_cmd(cmd, patch_file)
    return result.returncode == 0
  except (FileNotFoundError, subprocess.SubprocessError):
    # If git is not available or fails, assume patch is not applied
    return False


def apply_patch_command(patch_file: Path) -> subprocess.CompletedProcess:
  """Apply a patch using the best available command.

  Tries 'git apply' first (cross-platform, more predictable for
  already-applied patches), falls back to 'patch' command if git is
  not available. On Windows, '--ignore-cr-at-eol' is added to handle
  potential CRLF/LF mismatches between the patch file and BoringSSL
  source.
  """
  is_windows = platform.system() == "Windows"
  # --ignore-cr-at-eol handles CRLF vs LF differences on Windows
  extra_args = ["--ignore-cr-at-eol"] if is_windows else []

  # Prefer git apply as it's more predictable and cross-platform
  cmd = ["-p1"] + extra_args
  try:
    return _git_apply_cmd(cmd, patch_file)
  except FileNotFoundError:
    # Fall back to patch command if git is not available
    cmd = ["patch", "-p1", "-i", str(patch_file)]
    try:
      return subprocess.run(cmd, capture_output=True, text=True, cwd=BORINGSSL_SRC)
    except FileNotFoundError as e:
      return subprocess.CompletedProcess(
          args=cmd, returncode=1, stdout="", stderr=f"Neither 'git' nor 'patch' command found: {e}"
      )


def apply_patch(patch_file: Path) -> bool:
  """Apply a single patch file to BoringSSL source.

  This function handles two cases:
  1. Patch is already applied -> skip it
  2. Patch can be applied -> apply it

  Note: We intentionally do NOT use --fuzz for security patches.
  Security patches must either apply cleanly or fail explicitly.
  Using --fuzz could silently apply the patch to the wrong location,
  causing security protections to fail without any error.
  """
  if not patch_file.exists():
    print(f"Patch file not found: {patch_file}")
    return False

  print(f"Applying patch: {patch_file.name}")

  # Check if patch is already applied (using reliable reverse check)
  if is_patch_applied(patch_file):
    print(f"  Patch already applied: {patch_file.name}")
    return True

  # Apply the patch
  result = apply_patch_command(patch_file)
  if result.returncode == 0:
    print(f"  Successfully applied: {patch_file.name}")
    return True

  # Patch failed to apply - this is a security patch, so fail explicitly
  # rather than trying to apply it with fuzz (which could silently
  # apply to wrong location)
  print(f"  ERROR: Patch cannot be applied: {patch_file.name}")
  print(f"  This is a security patch and must be updated manually.")
  print(f"  Details: {result.stderr.strip()}")
  print(f"  ")
  print(f"  Please update the patch file to match the current BoringSSL source.")
  return False


def main() -> int:
  """Main entry point."""
  # Check if BoringSSL source exists
  if not BORINGSSL_SRC.exists():
    print(f"No BoringSSL source found, skipping patches: {BORINGSSL_SRC}")
    return 0

  # Check if patches directory exists
  if not PATCHES_DIR.exists():
    print(f"ERROR: BoringSSL source exists but patches directory not found: {PATCHES_DIR}")
    return 1

  # Find all patch files
  patch_files = sorted(PATCHES_DIR.glob("*.patch"))
  if not patch_files:
    print("No patches to apply")
    return 0

  print(f"Found {len(patch_files)} patch(es) to apply")

  # Apply patches and collect failures
  failed = [f.name for f in patch_files if not apply_patch(f)]

  if failed:
    print(f"\nFailed to apply {len(failed)} patch(es):")
    for name in failed:
      print(f"  - {name}")
    return 1

  # Mark patched files with --skip-worktree so gclient sync won't complain
  # about uncommitted changes on subsequent runs.
  _skip_worktree_files()

  print(f"\nSuccessfully applied all {len(patch_files)} patch(es)")
  return 0


if __name__ == "__main__":
  sys.exit(main())
