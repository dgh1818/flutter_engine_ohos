/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

export interface BinaryResult {
  exitCode: number;
  output: string;
  profrawPath: string;
  profrawSize: number;
}

export interface BinaryCheckResult {
  exists: boolean;
}

export interface SandboxInfo {
  sandboxPath: string;
  soPath: string;
  cwd: string;
}

export const checkBinary: (binaryPath: string) => BinaryCheckResult;
export const getSandboxPath: () => SandboxInfo;
export const runTestsSo: (soPath: string, filesDir: string, gtestFilter?: string) => Promise<BinaryResult>;
export const getLiveOutput: () => string;