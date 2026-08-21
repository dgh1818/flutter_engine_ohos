/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { AbilityStage, Want } from '@kit.AbilityKit';
import { FlutterAbilityLaunchConfigs } from '@ohos/flutter_ohos';

/**
 * Module-level AbilityStage, required for RegularWindowAbility's
 * `launchType: "specified"`: the key returned by onAcceptWant decides instance
 * reuse — a never-seen key creates a NEW UIAbility instance (and task-center
 * card), a seen key reuses the existing one. Returning a unique key per
 * startAbility makes each Regular window its own task-center entry; '' keeps
 * the default singleton behavior for everything else.
 */
export default class MyAbilityStage extends AbilityStage {
  onAcceptWant(want: Want): string {
    const abilityName = want.abilityName ?? '';
    if (abilityName === 'RegularWindowAbility') {
      const params = (want.parameters ?? {}) as Record<string, Object>;
      const instanceKey =
        params['instanceKey'] ??
        params[FlutterAbilityLaunchConfigs.EXTRA_FLUTTER_WINDOW_REQUEST_ID];
      const keyStr = instanceKey != null ? String(instanceKey) : `${Date.now()}`;
      return `RegularWindowAbility_${keyStr}`;
    }
    return '';
  }
}
