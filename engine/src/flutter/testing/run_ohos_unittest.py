#!/usr/bin/env python3
#
# Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

r"""
Runs the OHOS engine unit-tests in the unittestApp on a connected device and
generates an HTML coverage report (on by default; --no-coverage skips it).

This is the app-based counterpart of run_tests.py's `--type ohos` harness:
run_tests.py pushes and executes the root-only flutter_ohos_unittests binary,
while the tests here live in libflutter_ohos_app_test.so loaded by the
installed unittestApp (see testing/ohos/unittest_ohos). The app is
cold-started — its aboutToAppear auto-runs the full suite — and the exit code
is read from hilog.

Prerequisites:
  1. The unittestApp HAP, carrying libflutter_ohos_app_test.so in
     entry/libs/arm64-v8a/, is already installed on the device.
  2. The --variant build was compiled with coverage enabled
     (e.g. `./ohos -t debug -g '\--coverage' -n config compile`).

Usage:
  python3 run_ohos_unittest.py                        # run + coverage report
  python3 run_ohos_unittest.py --serial <serial>      # pick a device
  python3 run_ohos_unittest.py --no-coverage          # run only, no report
  python3 run_ohos_unittest.py --skip-run             # report only, no re-run
  python3 run_ohos_unittest.py --local-profraw FILE
  # run_tests.py-style flags (same names and semantics):
  python3 run_ohos_unittest.py --type ohos --ohos-variant ohos_debug_arm64 --coverage
"""

import argparse
import logging
import os
import re
import shutil
import subprocess
import sys

# ── Paths ────────────────────────────────────────────────────────────────────
THIS_DIR = os.path.abspath(os.path.dirname(__file__))
# THIS_DIR = .../engine/src/flutter/testing
# BUILDROOT_DIR = .../engine/src
BUILDROOT_DIR = os.path.abspath(os.path.join(THIS_DIR, '..', '..'))
OUT_DIR = os.path.join(BUILDROOT_DIR, 'out')

# The app (see testing/ohos/unittest_ohos) and its test library.
BUNDLE_NAME = 'com.example.unittest_ohos'
ABILITY_NAME = 'EntryAbility'
SO_NAME = 'libflutter_ohos_app_test.so'

# profraw is written to the app's filesDir, which maps to this real path:
REMOTE_PROFRAW_DIR = ('/data/app/el2/100/base/%s/haps/entry/files' % BUNDLE_NAME)
REMOTE_PROFRAW_PATH = os.path.join(REMOTE_PROFRAW_DIR, 'coverage.profraw')

COVERAGE_VARIANT_HINT = (
    'Build the engine with coverage enabled, e.g.:\n'
    '  cd <engine-source>/engine\n'
    "  ./ohos -t debug -g '\\--coverage' -n config compile\n"
    'Then redeploy the so and reinstall the app. '
    'See flutter/testing/ohos/unittest_ohos/README.md.'
)

_logger = logging.getLogger(__name__)


def run_cmd(cmd, cwd=None, check=True):
  """Runs a command and returns its stdout. Raises on failure if check=True."""
  _logger.info('Running: %s', ' '.join(cmd))
  result = subprocess.run(
      cmd,
      cwd=cwd,
      capture_output=True,
      text=True,
      # Native test binaries may emit non-UTF-8 bytes; strict decoding raises
      # UnicodeDecodeError mid-stream and kills the whole run.
      errors='replace',
      check=False,
  )
  if check and result.returncode != 0:
    _logger.error('Command failed (exit %d): %s', result.returncode, ' '.join(cmd))
    _logger.error('stdout: %s', result.stdout)
    _logger.error('stderr: %s', result.stderr)
    raise subprocess.CalledProcessError(result.returncode, cmd, result.stdout, result.stderr)
  return result.stdout.strip()


def find_hdc(hdc_path=None):
  """Locates the hdc binary."""
  if hdc_path:
    return hdc_path
  found = shutil.which('hdc')
  if found:
    return found
  raise FileNotFoundError(
      'hdc not found. Install DevEco Studio or add hdc to PATH, '
      'or pass --hdc-path.'
  )


def hdc_base(hdc_path, serial):
  """Returns the hdc command prefix, optionally targeting a single device."""
  cmd = [hdc_path]
  if serial:
    cmd += ['-s', serial]
  return cmd


def run_tests_on_device(hdc, timeout_seconds):
  """Cold-starts the app (auto-running the suite) and waits for the exit code.

  Returns (exit_code, summary) where summary is the "[  PASSED  ] N tests."
  hilog line ('' when it cannot be found).

  The wait runs a device-side `grep -m1` over the hilog stream, which returns
  as soon as the line is printed; the subprocess timeout is the outer bound.
  """
  # Clear the hilog buffer so a stale "Exit code" line from a previous run
  # cannot immediately satisfy the wait below.
  run_cmd(hdc + ['shell', 'hilog', '-r'], check=False)
  # Cold start: aboutToAppear auto-runs the full suite shortly after launch,
  # so force-stop first to re-trigger the auto-run on a warm foreground app.
  run_cmd(hdc + ['shell', 'aa', 'force-stop', BUNDLE_NAME], check=False)
  try:
    run_cmd(hdc + ['shell', 'aa', 'start', '-a', ABILITY_NAME, '-b', BUNDLE_NAME])
  except subprocess.CalledProcessError:
    raise RuntimeError(
        'Failed to start %s. Is the app (with %s in entry/libs/arm64-v8a/) '
        'installed on the device?' % (BUNDLE_NAME, SO_NAME)
    ) from None

  _logger.info(
      'Waiting up to %d s for the app to report "Exit code" in hilog...',
      timeout_seconds,
  )
  try:
    result = subprocess.run(
        hdc + ['shell', 'hilog | grep -m1 "unittest_ohos.*Exit code"'],
        capture_output=True,
        text=True,
        errors='replace',
        timeout=timeout_seconds,
        check=False,
    )
  except subprocess.TimeoutExpired:
    raise RuntimeError(
        'Timed out after %d s waiting for the app to report "Exit code" in '
        'hilog. Inspect `hdc shell hilog` for a crash during the test run.' % timeout_seconds
    ) from None
  output = (result.stdout or '') + (result.stderr or '')
  match = re.search(r'Exit code:\s*(-?\d+)', output)
  if result.returncode != 0 or match is None:
    raise RuntimeError(
        'The app did not report an exit code (device grep exit status %d). '
        'Inspect `hdc shell hilog` for a crash or an install failure.' % result.returncode
    )
  exit_code = int(match.group(1))

  summary = ''
  try:
    summary_out = subprocess.run(
        hdc + ['shell', 'hilog | grep -m1 -E "unittest_ohos.*\\[ +(PASSED|FAILED)"'],
        capture_output=True,
        text=True,
        errors='replace',
        timeout=30,
        check=False,
    ).stdout or ''
  except subprocess.TimeoutExpired:
    summary_out = ''
  summary_match = re.search(r'\[ +(PASSED|FAILED) +\] \d+ tests?', summary_out)
  if summary_match:
    summary = summary_match.group(0).strip()
  return exit_code, summary


def pull_profraw(hdc, local_profraw):
  """Pulls the profraw file from the device."""
  _logger.info('Pulling profraw from device: %s -> %s', REMOTE_PROFRAW_PATH, local_profraw)
  # Remove stale local file.
  if os.path.exists(local_profraw):
    os.remove(local_profraw)
  run_cmd(hdc + ['file', 'recv', REMOTE_PROFRAW_PATH, local_profraw])
  if not os.path.exists(local_profraw):
    raise RuntimeError(
        'Failed to pull %s from the device. Did the app run with a '
        'coverage-instrumented so?\n%s' % (REMOTE_PROFRAW_PATH, COVERAGE_VARIANT_HINT)
    )
  size = os.path.getsize(local_profraw)
  _logger.info('Pulled profraw: %s (%d bytes)', local_profraw, size)
  if size == 0:
    raise RuntimeError(
        'Pulled profraw is empty. The installed so likely lacks coverage '
        'instrumentation.\n%s' % COVERAGE_VARIANT_HINT
    )
  return local_profraw


def get_llvm_bin_dir(build_dir):
  """
  Returns the LLVM bin directory from the OHOS build's custom_toolchain.
  Falls back to the OHOS NDK's LLVM if custom_toolchain is not set.
  """
  args_gn_path = os.path.join(build_dir, 'args.gn')
  if not os.path.exists(args_gn_path):
    _logger.warning('args.gn not found at %s', args_gn_path)
    return None
  with open(args_gn_path, 'r') as args_file:
    content = args_file.read()
  match = re.search(r'custom_toolchain\s*=\s*"([^"]+)"', content)
  if match:
    llvm_bin = os.path.join(match.group(1), 'bin')
    if os.path.isdir(llvm_bin):
      return llvm_bin
  # Fall back to the OHOS NDK's LLVM (standard install path on macOS).
  ndk_llvm = (
      '/Applications/DevEco-Studio.app/Contents/sdk/default/openharmony'
      '/native/llvm/bin'
  )
  if os.path.isdir(ndk_llvm):
    return ndk_llvm
  # Fall back to buildtools.
  buildtools_llvm = os.path.join(
      BUILDROOT_DIR, 'flutter', 'buildtools', 'linux-x64', 'clang', 'bin'
  )
  if os.path.isdir(buildtools_llvm):
    return buildtools_llvm
  return None


def filter_source_files(cov_binary, unstripped_so, merged_profile, source_regex):
  """Returns source files matching source_regex, excluding
  third_party/unittest/fixture/test_stubs."""
  report_output = subprocess.check_output(
      [cov_binary, 'report', '-object', unstripped_so,
       '-instr-profile=%s' % merged_profile],
      cwd=BUILDROOT_DIR,
      universal_newlines=True,
      errors='replace',
  )
  source_pattern = re.compile(source_regex, re.IGNORECASE)
  exclude_pattern = re.compile(r'flutter/third_party/|unittest|fixture|test_stubs|/out/|^out/')
  filtered = []
  for line in report_output.splitlines():
    parts = line.split()
    if not parts:
      continue
    filename = parts[0]
    if (source_pattern.search(filename) and not exclude_pattern.search(filename) and
        filename.endswith(('.cc', '.cpp', '.h', '.c'))):
      filtered.append(filename)
  return filtered


def generate_coverage_report(local_profraw, build_dir, coverage_dir, source_regex='ohos'):
  """Merges the profraw and generates an HTML coverage report."""
  # Use the unstripped .so for coverage reporting (it has __llvm_covmap).
  # GN places unstripped .so files under so.unstripped/ for shared_library
  # targets, and under exe.unstripped/ for executable targets.
  unstripped_so = os.path.join(build_dir, 'so.unstripped', SO_NAME)
  if not os.path.exists(unstripped_so):
    unstripped_so = os.path.join(build_dir, 'exe.unstripped', SO_NAME)
  if not os.path.exists(unstripped_so):
    unstripped_so = os.path.join(build_dir, SO_NAME)
  if not os.path.exists(unstripped_so):
    raise FileNotFoundError(
        'Could not find the unstripped %s under %s. Make sure the engine is '
        'built with coverage enabled for this variant.\n%s' %
        (SO_NAME, build_dir, COVERAGE_VARIANT_HINT)
    )

  shutil.rmtree(coverage_dir, ignore_errors=True)
  os.makedirs(coverage_dir, exist_ok=True)

  # Locate LLVM tools.
  llvm_bin_dir = get_llvm_bin_dir(build_dir)
  if not llvm_bin_dir:
    raise FileNotFoundError(
        'Could not find LLVM bin directory. Check custom_toolchain in args.gn '
        'or install the OHOS NDK.'
    )
  profdata_binary = os.path.join(llvm_bin_dir, 'llvm-profdata')
  cov_binary = os.path.join(llvm_bin_dir, 'llvm-cov')
  _logger.info('Using LLVM tools from: %s', llvm_bin_dir)

  # Merge the raw profile into a single profile.
  merged_profile = os.path.join(coverage_dir, 'all.profile')
  run_cmd(
      [profdata_binary, 'merge', '-sparse', local_profraw, '-o', merged_profile],
      cwd=BUILDROOT_DIR,
  )
  _logger.info('Merged profile: %s', merged_profile)

  # Generate the HTML report.
  if source_regex:
    filtered_files = filter_source_files(cov_binary, unstripped_so, merged_profile, source_regex)
  else:
    filtered_files = None

  if filtered_files:
    _logger.info('Filtering to %d source files matching "%s"', len(filtered_files), source_regex)
    run_cmd(
        [
            cov_binary, 'show',
            '-instr-profile=%s' % merged_profile, '-format=html',
            '-output-dir=%s' % coverage_dir, '-tab-size=2', unstripped_so
        ] + filtered_files,
        cwd=BUILDROOT_DIR,
    )
  else:
    run_cmd(
        [
            cov_binary, 'show', '-object', unstripped_so,
            '-instr-profile=%s' % merged_profile, '-format=html',
            '-output-dir=%s' % coverage_dir, '-tab-size=2',
            '-ignore-filename-regex=flutter/third_party/|unittest|fixture|test_stubs'
        ],
        cwd=BUILDROOT_DIR,
    )
  _logger.info('Coverage report generated at: %s', coverage_dir)


def main():
  parser = argparse.ArgumentParser(
      description='Run the OHOS engine unit-tests in the unittestApp on a '
      'connected device and generate an HTML coverage report by default '
      '(--no-coverage skips it). App-based counterpart of run_tests.py '
      '--type ohos.'
  )
  parser.add_argument(
      '--hdc-path', default=None, help='Path to the hdc binary. If omitted, searches PATH.'
  )
  parser.add_argument(
      '--serial',
      default=None,
      help='Target a specific device (passes -s SERIAL to hdc). '
      'Use `hdc list targets` to list devices.'
  )
  parser.add_argument(
      '--type',
      default='ohos',
      choices=['ohos'],
      help='Test type to run. Only "ohos" exists in this app-based runner '
      '(kept for parity with run_tests.py).'
  )
  parser.add_argument(
      '--ohos-variant',
      dest='ohos_variant',
      default='ohos_debug_arm64',
      help='The engine build variant that produced the installed so '
      '(default: ohos_debug_arm64; build it with -g "\\--coverage" to get '
      'a coverage report).'
  )
  parser.add_argument(
      '--timeout',
      type=int,
      default=300,
      help='Seconds to wait for the test run to finish (default: 300).'
  )
  parser.add_argument(
      '--coverage',
      action=argparse.BooleanOptionalAction,  # pylint: disable=no-member
      default=True,
      help='Pull the profraw and generate an HTML coverage report '
      '(default: on; --no-coverage runs the tests only).'
  )
  parser.add_argument(
      '--coverage-source-regex',
      dest='coverage_source_regex',
      default='ohos',
      help='Regex to filter source files in the coverage report (default: "ohos"). '
      'Pass ".*" to include all sources.'
  )
  parser.add_argument(
      '--output-dir',
      default=None,
      help='Directory for the coverage report. '
      'Default: <build_dir>/coverage/unittest_app'
  )
  parser.add_argument(
      '--skip-run',
      dest='skip_run',
      action='store_true',
      default=False,
      help='Do not restart the app; build the report from the profraw already '
      'on the device.'
  )
  parser.add_argument(
      '--local-profraw',
      dest='local_profraw',
      default=None,
      help='Generate the report from a local profraw file instead of pulling '
      'from the device.'
  )
  args = parser.parse_args()

  logging.basicConfig(
      level=logging.INFO,
      format='[%(levelname)s] %(message)s',
  )

  build_dir = os.path.join(OUT_DIR, args.ohos_variant)
  if not os.path.isdir(build_dir):
    raise FileNotFoundError(
        'Build directory not found: %s. Make sure the engine is built.' % build_dir
    )

  exit_code = None
  if not args.skip_run and not args.local_profraw:
    hdc = hdc_base(find_hdc(args.hdc_path), args.serial)
    exit_code, summary = run_tests_on_device(hdc, args.timeout)
    if summary:
      _logger.info('Test result: %s', summary)
    if exit_code != 0:
      _logger.error('Tests FAILED with exit code %d.', exit_code)

  # The report is on by default: the regular build entry
  # (./ohos -t debug -g '\--coverage') produces an instrumented so.
  # --skip-run/--local-profraw are report-only modes and imply the report.
  want_report = args.coverage or args.skip_run or args.local_profraw
  if want_report:
    if args.local_profraw:
      local_profraw = os.path.abspath(args.local_profraw)
      if not os.path.exists(local_profraw):
        raise FileNotFoundError('Profraw file not found: %s' % local_profraw)
    else:
      hdc = hdc_base(find_hdc(args.hdc_path), args.serial)
      local_profraw = os.path.join(build_dir, 'unittest_app.profraw')
      pull_profraw(hdc, local_profraw)

    coverage_dir = args.output_dir or os.path.join(build_dir, 'coverage', 'unittest_app')
    source_regex = args.coverage_source_regex if args.coverage_source_regex else None
    generate_coverage_report(local_profraw, build_dir, coverage_dir, source_regex)

    print('\n✓ Coverage report generated at: %s' % coverage_dir)
    print('  Open index.html in a browser to view the report.')

  if exit_code is not None:
    return 0 if exit_code == 0 else 1
  return 0


if __name__ == '__main__':
  sys.exit(main())
