# Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_KHZG file.

#!/usr/bin/python
import shutil
import os


def copytree(src, dst, symlinks=False, ignore=None):
    if not os.path.exists(dst):
        os.makedirs(dst)
    for item in os.listdir(src):
        s = os.path.join(src, item)
        d = os.path.join(dst, item)

        if os.path.islink(s):
            if symlinks:
                linkto = os.readlink(s)
                os.symlink(linkto, d)
            else:
                pass
        elif os.path.isdir(s):
            copytree(s, d, symlinks, ignore)
        else:
            copy_file(s, d)


def copy_file(sourceFile, targetFile, log=False):
    if log:
      print("拷贝文件，从{}到{}".format(sourceFile, targetFile))
    if os.path.exists(targetFile):
        os.remove(targetFile)
    shutil.copy2(sourceFile, targetFile)


# 拷贝文件夹
def copy_dir(sourceFiles, targetFiles, log=False):
    if log:
      print("拷贝文件夹，从{}到{}".format(sourceFiles, targetFiles))
    if os.path.exists(targetFiles):
        print("目标文件夹已存在，退出")
        pass
        # shutil.rmtree(targetFiles)
    else:
        copytree(sourceFiles, targetFiles)


# 拷贝文件夹中所有文件到目标目录（必须存在），不拷贝文件夹本身
def copy_files(source_folder, destination_folder, log=False):
    if log:
      print("拷贝文件夹内所有文件，从{}到{}".format(source_folder, destination_folder))
    # 获取源文件夹中的所有文件
    files = os.listdir(source_folder)

    # 遍历文件并拷贝到目标文件夹
    for file_name in files:
        source_file = os.path.join(source_folder, file_name)
        destination_file = os.path.join(destination_folder, file_name)
        if os.path.isfile(source_file) and os.path.exists(source_file) and not os.path.islink(source_file):
            copy_file(source_file, destination_file)
        elif os.path.isdir(source_file) and os.path.exists(source_file):
            copy_dir(source_file, destination_file)
