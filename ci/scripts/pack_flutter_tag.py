#!/usr/bin/env python3
# Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
"""
Pack tag-versioned Flutter OHOS SDK archives (pure Python, runs on
linux/macos/windows CI hosts), following the upstream release flow in
dev/bots/prepare_package.dart:

  - full clone of the tag, .git minified with "git gc"; history is kept
    so users can fetch + checkout a newer tag (deliberately a tag
    snapshot with detached HEAD, unlike upstream's channel branch)
  - bin/cache pre-populated for ohos + universal + web + the host's
    desktop set and iOS on mac (Android never ships, like upstream's
    per-host layout otherwise); non-OHOS downloads go through the CN
    mirror storage.flutter-io.cn
  - .dart_tool is dropped before zipping: its package_config.json points
    flutter_template_images at the build machine's pub cache and would
    break "flutter create" templates on the user side
  - archive name: flutter_<os>_<arch?><tag>.<ext>, <arch?> empty for x64
    and "arm64_" for arm hosts; tar.xz on linux, zip elsewhere
    (upstream convention), e.g.
      flutter_windows_3.35.8-ohos-1.0.4.zip
      flutter_linux_3.41.10-ohos-1.0.1.tar.xz

Tags are packed serially. A tag whose archive already exists under
Archive/out/<tag>/ is skipped (delete it to repack). A failed tag does
not stop the remaining ones; re-run the same command to retry only the
failed tags.

Usage:
  python pack_flutter_tag.py <tag> [<tag> ...]
"""

import os
import platform
import re
import shutil
import stat
import subprocess
import sys
import tarfile
import time
import urllib.request
import zipfile
from pathlib import Path

from logger import Logger

DEFAULT_REPO_URL = "https://gitcode.com/CPF-Flutter/flutter_flutter.git"
# CN mirror of flutter_infra_release; the base for precache downloads
# (this script only runs on CN hosts).
DEFAULT_STORAGE_BASE = "https://storage.flutter-io.cn"
DEFAULT_PUB_HOSTED = "https://pub.flutter-io.cn"
# MinGit build pinned by upstream; keep this URL byte-identical to
# mingitForWindowsUrl in dev/bots/prepare_package/common.dart and re-sync
# (new hash) when upstream bumps it. The googleapis host is never
# contacted: _populate_staging rewrites it to DEFAULT_STORAGE_BASE.
MINGIT_URL = ("https://storage.googleapis.com/flutter_infra_release/mingit/"
              "603511c649b00bbef0a6122a827ac419b656bc19/mingit.zip")


def run(cmd, extra_env=None):
    """Run a command, streaming output; raise on failure."""
    Logger.info(f"$ {' '.join(str(c) for c in cmd)}")
    # Fail fast instead of hanging on a git credential prompt in CI.
    env = {**os.environ, "GIT_TERMINAL_PROMPT": "0", **(extra_env or {})}
    subprocess.run([str(c) for c in cmd], check=True, env=env)


def detect_platform():
    """Return (os, arch) of the host in dart naming (linux/macos/windows, x64/arm64)."""
    system = platform.system().lower()
    machine = platform.machine().lower()
    os_map = {"linux": "linux", "darwin": "macos", "windows": "windows"}
    if system not in os_map:
        Logger.error(f"Unsupported host OS: {system}")
        sys.exit(1)
    if machine in ("x86_64", "amd64"):
        arch = "x64"
    elif machine in ("arm64", "aarch64"):
        arch = "arm64"
    else:
        Logger.error(f"Unsupported host arch: {machine}")
        sys.exit(1)
    return os_map[system], arch


def force_rmtree(path):
    """rmtree that clears read-only bits first (read-only git pack files break plain rmtree on Windows)."""
    def _on_error(func, p, _exc):
        os.chmod(p, stat.S_IWRITE)
        func(p)
    # onerror is deprecated in favor of onexc since Python 3.12
    if sys.version_info >= (3, 12):
        shutil.rmtree(path, onexc=_on_error)
    else:
        shutil.rmtree(path, onerror=_on_error)


def _zip_tree(zf, source_dir):
    """Add source_dir under a top-level "flutter/" entry; permissions,
    symlinks and pre-1980 mtimes are handled here (see create_archive)."""
    for path in sorted(source_dir.rglob("*")):
        arcname = (Path("flutter") / path.relative_to(source_dir)).as_posix()
        if path.is_symlink():
            zi = zipfile.ZipInfo(arcname)
            # Explicit Unix semantics: Info-ZIP unzip only restores
            # symlinks and permissions for create_system == 3.
            zi.create_system = 3
            zi.date_time = time.localtime(path.lstat().st_mtime)[:6]
            if zi.date_time[0] < 1980:
                zi.date_time = (1980, 1, 1, 0, 0, 0)
            zi.external_attr = 0o120777 << 16  # S_IFLNK | 0777
            zf.writestr(zi, os.readlink(path))
        else:
            if path.stat().st_mtime < 315619200:  # 1980-01-02 UTC
                os.utime(path, (315619200, 315619200))
            zf.write(path, arcname)


def create_archive(output_file, source_dir):
    """Archive source_dir into output_file (tar.xz on linux, zip
    elsewhere: Finder/Explorer cannot handle tar.xz).

    - ".part" name first, then atomic rename: no truncated archive on
      failure.
    - tar keeps permissions, symlinks and mtimes natively; for zip they
      are set via external_attr (bin/flutter must stay executable).
    - mtimes before 1980 (zip's DOS epoch; e.g. CanvasKit files from
      CIPD carry mtime 0) are clamped, or zf.write raises ValueError.
    """
    part_file = output_file.with_suffix(output_file.suffix + ".part")
    try:
        if output_file.name.endswith(".tar.xz"):
            # Default xz level, like upstream's "tar cJf".
            with tarfile.open(part_file, "w:xz") as tf:
                tf.add(source_dir, arcname="flutter")
        else:
            with zipfile.ZipFile(part_file, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
                _zip_tree(zf, source_dir)
        os.replace(part_file, output_file)
    except BaseException:
        part_file.unlink(missing_ok=True)
        raise


def pack_tag(tag, host_os, arch, staging):
    """Pack one tag; returns (ok, info) -- the archive path on success, the failure reason otherwise."""
    if (tag in (".", "..") or tag.startswith("-")
            or not re.fullmatch(r"[0-9A-Za-z._+-]+", tag)):
        Logger.error(f"Invalid tag name: {tag}")
        return False, "invalid tag name"
    archive_dir = Path.cwd() / "Archive" / "out" / tag
    arch_tag = "arm64_" if arch == "arm64" else ""
    ext = ".tar.xz" if host_os == "linux" else ".zip"
    output_file = archive_dir / f"flutter_{host_os}_{arch_tag}{tag}{ext}"
    if output_file.exists():
        Logger.info(f"Already packed, skipping: {output_file}")
        return True, str(output_file)
    # Remove a .part left by a hard-killed previous run.
    output_file.with_suffix(output_file.suffix + ".part").unlink(missing_ok=True)
    archive_dir.mkdir(parents=True, exist_ok=True)

    Logger.step(f"Packing tag SDK: {tag} on host {host_os}-{arch}")
    Logger.info(f"Archive: {output_file}")

    Logger.step(f"Cloning {tag}")
    try:
        if staging.exists():
            force_rmtree(staging)
        _populate_staging(tag, host_os, staging)
        Logger.step("Creating archive")
        create_archive(output_file, staging / "flutter")
    except Exception as e:
        try:
            failed_staging = staging.parent / f"staging.failed.{tag}"
            if failed_staging.exists():
                force_rmtree(failed_staging)
            staging.rename(failed_staging)
            kept = f"; staging kept at: {failed_staging}"
        except OSError:
            kept = f"; staging left at: {staging}"
        Logger.error(f"Tag {tag} failed: {e}{kept}")
        return False, str(e)

    # The archive is complete; a locked staging tree (AV/indexer on
    # Windows) must not crash the run or the remaining tags.
    try:
        force_rmtree(staging)
    except Exception as cleanup_err:
        Logger.warn(f"Staging cleanup failed (archive ok): {cleanup_err}")
    Logger.info(f"Done: {output_file}")
    return True, str(output_file)


def _populate_staging(tag, host_os, staging):
    """Clone the tag into staging and pre-fill bin/cache; raises on
    failure."""
    run(["git", "clone", "--quiet", "--branch", tag,
         DEFAULT_REPO_URL, staging / "flutter"])
    # Minify .git like upstream (~300M in the official archives).
    run(["git", "-C", staging / "flutter", "gc", "--prune=now", "--aggressive"])
    revision = subprocess.run(
        ["git", "-C", str(staging / "flutter"), "rev-parse", "HEAD"],
        check=True, capture_output=True, text=True).stdout.strip()
    Logger.info(f"Revision: {revision}")

    Logger.step("Pre-populating flutter cache")
    # No bare precache (Android/iOS everywhere); web, host desktop and
    # iOS-on-mac ship because "flutter run" fetches them otherwise.
    # PUB_CACHE stays at the staging root, outside the zip.
    flutter_bin = ("flutter.bat" if host_os == "windows" else "flutter")
    host_flags = {"linux": ["--linux"], "windows": ["--windows"],
                  "macos": ["--macos", "--ios"]}[host_os]
    run([staging / "flutter" / "bin" / flutter_bin, "precache",
         "--ohos", "--universal", "--web", *host_flags],
        extra_env={
            "PUB_CACHE": str(staging / ".pub-cache"),
            "FLUTTER_STORAGE_BASE_URL": DEFAULT_STORAGE_BASE,
            "PUB_HOSTED_URL": DEFAULT_PUB_HOSTED,
        })

    # Drop .dart_tool (build-machine pub-cache paths inside); the
    # user side rebuilds it on the first run.
    for dart_tool in (staging / "flutter").rglob(".dart_tool"):
        force_rmtree(dart_tool)

    if host_os == "windows":
        # Bundle MinGit like upstream: flutter.bat hard-requires git,
        # and Windows end users may not have one installed.
        Logger.step("Bundling MinGit")
        url = MINGIT_URL.replace(
            "https://storage.googleapis.com", DEFAULT_STORAGE_BASE)
        mingit_zip = staging.parent / "mingit.zip"
        if not mingit_zip.exists():
            _download_mingit(url, mingit_zip)
        _extract_mingit(mingit_zip, staging / "flutter" / "bin" / "mingit")


def _download_mingit(url, mingit_zip):
    """Fetch mingit.zip through a .part file so an interrupted download cannot corrupt the cache."""
    part = mingit_zip.with_suffix(mingit_zip.suffix + ".part")
    try:
        with urllib.request.urlopen(url, timeout=300) as resp, open(part, "wb") as f:
            shutil.copyfileobj(resp, f)
        os.replace(part, mingit_zip)
    except BaseException:
        part.unlink(missing_ok=True)
        raise


def _extract_mingit(mingit_zip, target):
    """Extract mingit.zip into target; a bad zip is dropped so the next run re-downloads it."""
    try:
        with zipfile.ZipFile(mingit_zip) as zf:
            _check_zip_members(zf.namelist())
            zf.extractall(target)
    except (zipfile.BadZipFile, ValueError):
        mingit_zip.unlink(missing_ok=True)
        raise


def _check_zip_members(names):
    """ZipSlip check: archive members must stay inside the extraction target."""
    for m in names:
        if Path(m).is_absolute() or ".." in Path(m).parts:
            raise ValueError(f"unsafe zip member: {m}")


def main():
    tags = sys.argv[1:]
    if not tags:
        Logger.error("Usage: python pack_flutter_tag.py <tag> [<tag> ...]")
        return 1

    work_dir = Path.cwd()
    staging = work_dir / f"Archive/staging.{os.getpid()}"

    # Sweep staging trees left by previous runs (failed ones are kept).
    for old in (work_dir / "Archive").glob("staging.*"):
        if old.name.startswith("staging.failed."):
            continue
        try:
            force_rmtree(old)
        except Exception as sweep_err:
            Logger.warn(f"Staging sweep skipped {old}: {sweep_err}")

    host_os, arch = detect_platform()

    results = [(tag, *pack_tag(tag, host_os, arch, staging)) for tag in tags]

    Logger.step("Result summary")
    for i, (tag, ok, info) in enumerate(results, 1):
        if ok:
            Logger.info(f"{i}. {tag} ok, archive: {info}")
        else:
            Logger.error(f"{i}. {tag} failed, reason: {info}")

    failed = [tag for tag, ok, _ in results if not ok]
    if failed:
        Logger.error(f"Failed tags (re-run to retry): {', '.join(failed)}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
