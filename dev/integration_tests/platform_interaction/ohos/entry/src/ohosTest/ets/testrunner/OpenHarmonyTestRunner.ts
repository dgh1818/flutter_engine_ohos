/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import hilog from '@ohos.hilog';
import TestRunner from '@ohos.application.testRunner';
import AbilityDelegatorRegistry from '@ohos.app.ability.abilityDelegatorRegistry';

var abilityDelegator = undefined
var abilityDelegatorArguments = undefined

async function onAbilityCreateCallback() {
    hilog.info(0x0000, 'testTag', '%{public}s', 'onAbilityCreateCallback');
}

async function addAbilityMonitorCallback(err: any) {
    hilog.info(0x0000, 'testTag', 'addAbilityMonitorCallback : %{public}s', JSON.stringify(err) ?? '');
}

export default class OpenHarmonyTestRunner implements TestRunner {
    constructor() {
    }

    onPrepare() {
        hilog.info(0x0000, 'testTag', '%{public}s', 'OpenHarmonyTestRunner OnPrepare ');
    }

    async onRun() {
        hilog.info(0x0000, 'testTag', '%{public}s', 'OpenHarmonyTestRunner onRun run');
        abilityDelegatorArguments = AbilityDelegatorRegistry.getArguments()
        abilityDelegator = AbilityDelegatorRegistry.getAbilityDelegator()
        var testAbilityName = abilityDelegatorArguments.bundleName + '.TestAbility'
        let lMonitor = {
            abilityName: testAbilityName,
            onAbilityCreate: onAbilityCreateCallback,
        };
        abilityDelegator.addAbilityMonitor(lMonitor, addAbilityMonitorCallback)
        var cmd = 'aa start -d 0 -a TestAbility' + ' -b ' + abilityDelegatorArguments.bundleName
        var debug = abilityDelegatorArguments.parameters['-D']
        if (debug == 'true')
        {
            cmd += ' -D'
        }
        hilog.info(0x0000, 'testTag', 'cmd : %{public}s', cmd);
        abilityDelegator.executeShellCommand(cmd,
            (err: any, data: any) => {
                hilog.info(0x0000, 'testTag', 'executeShellCommand : err : %{public}s', JSON.stringify(err) ?? '');
                hilog.info(0x0000, 'testTag', 'executeShellCommand : data : %{public}s', JSON.stringify(data) ?? '');
                var testInfo = data?.d?.result
                if (testInfo) {
                    hilog.info(0x0000, 'testTag', 'executeShellCommand : testInfo : %{public}s', JSON.stringify(testInfo) ?? '');
                }
            }
        )
        hilog.info(0x0000, 'testTag', '%{public}s', 'OpenHarmonyTestRunner onRun end');
    }
}
