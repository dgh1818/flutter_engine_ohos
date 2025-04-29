# Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_KHZG file.

#!/usr/bin/python
import sys
import json
import sub_process_with_timeout
from operator import itemgetter
"""
在gclient中pre_deps_hooks中配置执行,用于在sync前回滚patch
职责如下：
1.解析config.json,并按顺序倒序排序
2.回滚目录下的patch
"""
ROOT = './src/flutter/attachment'


def apply_reverse_patch(task, log=False):
  file_path = task['file_path']
  target_path = task['target']
  retcode, stdout, stderr = sub_process_with_timeout.excuteArr([
      'git', 'apply', '-R', '--ignore-whitespace', '--whitespace=nowarn', file_path
  ],
                                                               target_path,
                                                               log,
                                                               timeout=20)
  if retcode == 0 and log:
    print("Apply reverse succeded. file path:" + file_path)
  if log:
    print(str(stdout))
    print(str(stderr))
  if retcode != 0 and log:
    print("Apply reverse failed. file path:" + file_path + " Error:" + str(stderr))
  pass


def apply_reverse_check(task, log=False):
  file_path = task['file_path']
  target_path = task['target']
  retcode, stdout, stderr = sub_process_with_timeout.excuteArr([
      'git', 'apply', '-R', '--check', '--ignore-whitespace', file_path
  ],
                                                               target_path,
                                                               log,
                                                               timeout=20)
  if log:
    print("retcode:" + str(retcode))
    print(str(stdout))
    print(str(stderr))
  if retcode != 0 and log:
    print("Apply reverse check failed. file path:" + file_path + " Error:" + str(stderr))
  return retcode != '-1' and 'error' not in str(stderr)


def doTask(task, log=False):
  if (task['type'] == 'patch'):
    if (apply_reverse_check(task, False)):
      apply_reverse_patch(task, log)


def parse_config(config_file="{}/scripts/config.json".format(ROOT)):
  log = False
  if (len(sys.argv) > 1):
    if (sys.argv[1] == '-v'):
      log = True
  with open(config_file) as json_file:
    data = json.load(json_file)
    data = sorted(data, key=itemgetter('name'), reverse=True)
    for task in data:
      doTask(task, log)


if __name__ == "__main__":
  parse_config()
