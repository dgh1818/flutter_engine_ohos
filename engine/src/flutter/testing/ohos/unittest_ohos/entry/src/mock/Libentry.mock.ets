/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

const NativeMock: Record<string, Object> = {
  'runTestsSo': (soPath: string, filesDir: string, gtestFilter?: string) => {
    return Promise.resolve({ exitCode: 0, output: 'mock', profrawPath: '', profrawSize: 0 });
  },
  'getSandboxPath': () => {
    return { sandboxPath: '/mock/sandbox', soPath: '/mock/sandbox/libs/arm64/libentry.so', cwd: '/' };
  },
  'checkBinary': (binaryPath: string) => {
    return { exists: true };
  },
  'getLiveOutput': () => {
    return '';
  },
};

export default NativeMock;