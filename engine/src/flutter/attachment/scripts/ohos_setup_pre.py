# Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_KHZG file.

#!/usr/bin/python
import ohos_setup

# 在预编译脚本中使用的配置
ohos_setup.parse_config("{}/scripts/config_pre.json".format(ohos_setup.ROOT), useStash=False)
