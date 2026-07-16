/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import path from 'path'
import { injectNativeModules } from 'flutter-hvigor-plugin';

injectNativeModules(__dirname, path.dirname(__dirname))