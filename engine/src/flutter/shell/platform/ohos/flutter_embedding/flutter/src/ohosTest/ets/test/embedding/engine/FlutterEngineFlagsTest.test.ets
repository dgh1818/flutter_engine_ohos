/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, expect, it } from '@ohos/hypium';
import FlutterEngineFlags from '../../../../../main/ets/embedding/engine/FlutterEngineFlags';

export default function FlutterEngineFlagsTest() {
  describe('FlutterEngineFlagsTest', () => {

    it('computeMetadataArgs_emptyMetadata_returnsEmpty', 0, () => {
      const args = FlutterEngineFlags.computeMetadataArgs(new Map<string, string>(), true);
      expect(args.length).assertEqual(0);
    })

    it('computeMetadataArgs_unknownKeysIgnored', 0, () => {
      const metadata = new Map<string, string>();
      metadata.set('flutter.engine.NotAFlag', 'true');
      metadata.set('flutter.engine.', 'true');
      metadata.set('some.other.key', '--anything');
      const args = FlutterEngineFlags.computeMetadataArgs(metadata, false);
      expect(args.length).assertEqual(0);
    })

    it('computeMetadataArgs_booleanFlag_exactTrueEnables', 0, () => {
      const metadata = new Map<string, string>();
      metadata.set(FlutterEngineFlags.ENABLE_SOFTWARE_RENDERING.metadataKey, 'true');
      const args = FlutterEngineFlags.computeMetadataArgs(metadata, false);
      expect(args.length).assertEqual(1);
      expect(args[0]).assertEqual('--enable-software-rendering');
    })

    it('computeMetadataArgs_booleanFlag_nonExactTrueIgnored', 0, () => {
      const values: string[] = ['True', '1', 'yes', 'false', ''];
      for (const value of values) {
        const metadata = new Map<string, string>();
        metadata.set(FlutterEngineFlags.ENABLE_SOFTWARE_RENDERING.metadataKey, value);
        const args = FlutterEngineFlags.computeMetadataArgs(metadata, false);
        expect(args.length).assertEqual(0);
      }
    })

    it('computeMetadataArgs_valueFlag_appendsValue', 0, () => {
      const metadata = new Map<string, string>();
      metadata.set(FlutterEngineFlags.ENABLE_IMPELLER.metadataKey, 'false');
      const args = FlutterEngineFlags.computeMetadataArgs(metadata, true);
      expect(args.length).assertEqual(1);
      expect(args[0]).assertEqual('--enable-impeller=false');
    })

    it('computeMetadataArgs_valueFlag_emptyValueDropped', 0, () => {
      const metadata = new Map<string, string>();
      metadata.set(FlutterEngineFlags.VM_SERVICE_PORT.metadataKey, '');
      const args = FlutterEngineFlags.computeMetadataArgs(metadata, false);
      expect(args.length).assertEqual(0);
    })

    it('computeMetadataArgs_valueFlag_trueValueYieldsSingleValuedArg', 0, () => {
      const metadata = new Map<string, string>();
      metadata.set(FlutterEngineFlags.VM_SERVICE_PORT.metadataKey, 'true');
      const args = FlutterEngineFlags.computeMetadataArgs(metadata, false);
      // The value branch and the boolean branch are mutually exclusive: the
      // value flag must appear once with "=true", never as a bare flag.
      expect(args.length).assertEqual(1);
      expect(args[0]).assertEqual('--vm-service-port=true');
    })

    it('computeMetadataArgs_testFlagDropped', 0, () => {
      const metadata = new Map<string, string>();
      metadata.set(FlutterEngineFlags.TEST_FLAG.metadataKey, 'true');
      const args = FlutterEngineFlags.computeMetadataArgs(metadata, false);
      expect(args.length).assertEqual(0);
    })

    it('computeMetadataArgs_releaseDropsNonAllowedFlags', 0, () => {
      const metadata = new Map<string, string>();
      metadata.set(FlutterEngineFlags.TRACE_SKIA.metadataKey, 'true');
      metadata.set(FlutterEngineFlags.VM_SERVICE_PORT.metadataKey, '12345');
      const args = FlutterEngineFlags.computeMetadataArgs(metadata, true);
      expect(args.length).assertEqual(0);
    })

    it('computeMetadataArgs_releaseKeepsAllowedFlags', 0, () => {
      const metadata = new Map<string, string>();
      metadata.set(FlutterEngineFlags.TRACE_STARTUP.metadataKey, 'true');
      metadata.set(FlutterEngineFlags.ENABLE_DART_PROFILING.metadataKey, 'true');
      metadata.set(FlutterEngineFlags.PROFILE_STARTUP.metadataKey, 'true');
      metadata.set(FlutterEngineFlags.ENABLE_IMPELLER.metadataKey, 'false');
      const args = FlutterEngineFlags.computeMetadataArgs(metadata, true);
      expect(args.length).assertEqual(4);
      expect(args[0]).assertEqual('--trace-startup');
      expect(args[1]).assertEqual('--enable-dart-profiling');
      expect(args[2]).assertEqual('--profile-startup');
      expect(args[3]).assertEqual('--enable-impeller=false');
    })

    it('computeMetadataArgs_debugAndProfilePassNonAllowedFlags', 0, () => {
      const metadata = new Map<string, string>();
      metadata.set(FlutterEngineFlags.TRACE_SKIA.metadataKey, 'true');
      expect(FlutterEngineFlags.computeMetadataArgs(metadata, false).length).assertEqual(1);
    })

    it('computeMetadataArgs_flagsFollowRegistryOrder', 0, () => {
      const metadata = new Map<string, string>();
      metadata.set(FlutterEngineFlags.VM_SERVICE_PORT.metadataKey, '12345');
      metadata.set(FlutterEngineFlags.TRACE_STARTUP.metadataKey, 'true');
      const args = FlutterEngineFlags.computeMetadataArgs(metadata, false);
      // Map iteration order must not matter; args follow the registry order
      // (TraceStartup is declared before VMServicePort).
      expect(args.length).assertEqual(2);
      expect(args[0]).assertEqual('--trace-startup');
      expect(args[1]).assertEqual('--vm-service-port=12345');
    })

    it('mergeMetadataArgs_metadataFirstShellArgsLast', 0, () => {
      const merged = FlutterEngineFlags.mergeMetadataArgs(
        ['--from-want'], ['--from-metadata']);
      expect(merged.length).assertEqual(2);
      expect(merged[0]).assertEqual('--from-metadata');
      expect(merged[1]).assertEqual('--from-want');
    })

    it('mergeMetadataArgs_emptyInputs', 0, () => {
      const merged = FlutterEngineFlags.mergeMetadataArgs([], []);
      expect(merged.length).assertEqual(0);
    })

    it('getFlagByEngineArgument_exactBooleanMatch', 0, () => {
      expect(FlutterEngineFlags.getFlagByEngineArgument('--trace-skia'))
        .assertEqual(FlutterEngineFlags.TRACE_SKIA);
    })

    it('getFlagByEngineArgument_valueFlagMatchesByPrefix', 0, () => {
      expect(FlutterEngineFlags.getFlagByEngineArgument('--enable-impeller=false'))
        .assertEqual(FlutterEngineFlags.ENABLE_IMPELLER);
      expect(FlutterEngineFlags.getFlagByEngineArgument('--vm-service-port=12345'))
        .assertEqual(FlutterEngineFlags.VM_SERVICE_PORT);
    })

    it('getFlagByEngineArgument_allowlistNotShadowedByTraceSkia', 0, () => {
      // "--trace-skia-allowlist=x" starts with the boolean flag "--trace-skia",
      // but boolean flags match exactly, so it resolves to the allowlist flag.
      expect(FlutterEngineFlags.getFlagByEngineArgument('--trace-skia-allowlist=x'))
        .assertEqual(FlutterEngineFlags.TRACE_SKIA_ALLOWLIST);
    })

    it('getFlagByEngineArgument_bareValueFlagIsUnknown', 0, () => {
      expect(FlutterEngineFlags.getFlagByEngineArgument('--enable-impeller')).assertNull();
    })

    it('getFlagByEngineArgument_unknownArgsReturnNull', 0, () => {
      expect(FlutterEngineFlags.getFlagByEngineArgument('--unknown-flag')).assertNull();
      // Boolean flags must not match by prefix.
      expect(FlutterEngineFlags.getFlagByEngineArgument('--trace-startup-x')).assertNull();
    })

    it('filterArgsForRelease_dropsNonAllowedFlagsInRelease', 0, () => {
      const filtered = FlutterEngineFlags.filterArgsForRelease(
        ['--trace-skia', '--start-paused', '--dart-flags=foo', '--vm-service-port=1'], true);
      expect(filtered.length).assertEqual(0);
    })

    it('filterArgsForRelease_keepsAllowedAndUnknownArgsInRelease', 0, () => {
      const filtered = FlutterEngineFlags.filterArgsForRelease(
        ['--trace-startup', '--enable-impeller=false', '--unknown-want-flag'], true);
      expect(filtered.length).assertEqual(3);
      expect(filtered[0]).assertEqual('--trace-startup');
      expect(filtered[1]).assertEqual('--enable-impeller=false');
      expect(filtered[2]).assertEqual('--unknown-want-flag');
    })

    it('filterArgsForRelease_keepsEverythingInDebug', 0, () => {
      const args: string[] = ['--trace-skia', '--start-paused'];
      expect(FlutterEngineFlags.filterArgsForRelease(args, false)).assertEqual(args);
    })

    it('flag_metadataKeyHasEnginePrefix', 0, () => {
      expect(FlutterEngineFlags.METADATA_PREFIX).assertEqual('flutter.engine.');
      expect(FlutterEngineFlags.ENABLE_IMPELLER.metadataKey)
        .assertEqual('flutter.engine.EnableImpeller');
      expect(FlutterEngineFlags.TRACE_SKIA.metadataKey)
        .assertEqual('flutter.engine.TraceSkia');
    })

    it('flag_hasCommandLineValueFollowsTrailingEquals', 0, () => {
      expect(FlutterEngineFlags.ENABLE_IMPELLER.hasCommandLineValue()).assertTrue();
      expect(FlutterEngineFlags.VM_SERVICE_PORT.hasCommandLineValue()).assertTrue();
      expect(FlutterEngineFlags.TRACE_SKIA.hasCommandLineValue()).assertFalse();
    })

    it('registry_releaseAllowedFlagsAreTheExpectedFour', 0, () => {
      const allowed: string[] = [];
      for (const flag of FlutterEngineFlags.ALL_FLAGS) {
        if (flag.allowedInRelease) {
          allowed.push(flag.metadataKey);
        }
      }
      expect(allowed.join(',')).assertEqual(
        'flutter.engine.TraceStartup,' +
        'flutter.engine.EnableDartProfiling,' +
        'flutter.engine.ProfileStartup,' +
        'flutter.engine.EnableImpeller');
    })
  })
}
