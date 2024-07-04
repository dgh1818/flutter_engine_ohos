#!/usr/bin/env python3
#
# Copyright (c) 2024 Hunan OpenValley Digital Industry Development Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os
import platform
import subprocess
import sys
import traceback
from obs import ObsClient, PutObjectHeader

# 使用华为obs
# pip install esdk-obs-python --trusted-host pypi.org

OS_NAME = platform.system().lower()
IS_WINDOWS = OS_NAME.startswith("win")
IS_MAC = OS_NAME.startswith("darwin")
FLUTTER_ENGINE_PATH = os.path.join(os.getcwd(), 'src', 'flutter')
OHOS_ENGINE_TYPE_OUT = {'ohos-arm64' : 'ohos_debug_unopt_arm64',
                        'ohos-arm64-profile' : 'ohos_profile_arm64',
                        'ohos-arm64-release' : 'ohos_release_arm64',
                        'ohos-x64' : 'ohos_debug_unopt_x64',
                        'ohos-x64-profile' : 'ohos_profile_x64',
                        'ohos-x64-release' : 'ohos_release_x64',
                        }

# OBS 环境变量配置key信息
ACCESS_KEY = os.getenv("AccessKeyID")
SECRET_KEY = os.getenv("SecretAccessKey")

# 服务器地址 华南-广州
SERVER = "https://obs.cn-south-1.myhuaweicloud.com"

# OBS桶 
FLUTTER_OHOS = 'flutter-ohos'

def log(msg):
    print(f'================{msg}============')

def runGitCommand(command):
    result = subprocess.run(command, capture_output=True, text=True, shell=True)
    if result.returncode != 0:
        raise Exception(f"Git command failed: {result.stderr}")
    return result.stdout.strip()

def runPyCommand(command):

    if IS_WINDOWS:
        proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    else:
        proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=False)

    # 获取实时输出
    for line in iter(proc.stdout.readline, b''):
        print(line.decode('utf-8').strip())

    # 等待命令执行完成
    proc.wait()

def getRemoteTags(repoPath, remoteName='origin'):

    # 获取远程仓库的所有标签
    tagsOutput = runGitCommand(f'git -C {repoPath} ls-remote --tags {remoteName}')

    # 分割输出，每一行都是一个tag
    tags = tagsOutput.split('\n')

    # 去除空行和解析tag信息
    parsedTags = [line.split()[0] for line in tags if line]

    return parsedTags

# 在指定目录及其子目录中查找指定的文件
def findFile(directory, filename):
    for root, dirs, files in os.walk(directory):
        if filename in files:
            return (os.path.join(root, filename))

# 获取上传对象的进度
def uploadCallback(transferredAmount, totalAmount, totalSeconds):
    # 获取上传平均速率(KB/S)
    speed = int(transferredAmount / totalSeconds / 1024)
    # 获取上传进度百分比
    progress = int(transferredAmount * 100.0 / totalAmount)
    print("\r", end="")
    print("Speed: {} KB/S progress: {}%: ".format(speed, progress), "▓" * (progress // 2), end="")
    sys.stdout.flush()
    if progress == 100:
        print("")

# 检查TAG版本
def checkRemoteTagsUpdates():
    # 获取远程TAG的最新提交哈希
    remoteTags = getRemoteTags(FLUTTER_ENGINE_PATH)
    remoteLatestTag = remoteTags[-1]

    # 获取本地分支的最新提交哈希
    localLatestCommit = runGitCommand(f'git -C {FLUTTER_ENGINE_PATH} rev-parse HEAD')

    if remoteLatestTag != localLatestCommit:
        log("Remote repository has updates.")
        return True
    else:
        log("Local repository is up to date.")
        return False

# 检查分支更新
def checkRemoteBranchUpdates(repoPath, remoteName='origin', branch='dev'):
    # 切换到你的Git仓库目录
    os.chdir(repoPath)

    # 获取远程分支的最新提交哈希
    remoteLatestCommit = runGitCommand(f'git -C {repoPath} rev-parse {remoteName}/{branch}')

    # 获取本地分支的最新提交哈希
    localLatestCommit = runGitCommand(f'git -C {repoPath} rev-parse HEAD')

    if remoteLatestCommit != localLatestCommit:
        log("Remote repository has updates.")
        return True
    else:
        log("Local repository is up to date.")
        return False


# 获取编译产物
def getCompileFiles(buildType):
    zipfiles = ['artifacts.zip','linux-x64.zip','windows-x64.zip','darwin-x64.zip','symbols.zip']

    files = []
    for fileName in zipfiles:
        if IS_WINDOWS and fileName != 'windows-x64.zip':
            continue
        if IS_MAC and fileName != 'darwin-x64.zip':
            continue

        files.append(findFile(os.path.join(os.getcwd(), 'src', 'out', buildType), fileName))

    return files

# 上传服务器
def uploadServer(version, buildType, filePath):
    try:
        log(f'upload: {filePath}')

        bucketName = FLUTTER_OHOS
        obsClient = ObsClient(access_key_id=ACCESS_KEY, secret_access_key=SECRET_KEY, server=SERVER)
        
        # 上传对象的附加头域
        headers = PutObjectHeader()
        # 【可选】待上传对象的MIME类型
        headers.contentType = 'text/plain'

        # https://storage.flutter-io.cn/flutter_infra_release/flutter/cececddab019a56da828c41d55cb54484278e880/ohos-arm64-profile/linux-x64.zip
        fileName = os.path.basename(filePath)
        objectKey = f'flutter_infra_release/flutter/{version}/{buildType}/{fileName}'

        # 待上传文件/文件夹的完整路径，如aa/bb.txt，或aa/
        file_path = filePath

        # 文件上传
        resp = obsClient.putFile(bucketName, objectKey, file_path, headers=headers, progressCallback=uploadCallback)
        # 返回码为2xx时，接口调用成功，否则接口调用失败
        if resp.status < 300:
            print('Put Content Succeeded')
            print('requestId:', resp.requestId)
            print('etag:', resp.body.etag)
        else:
            print('Put Content Failed')
            print('requestId:', resp.requestId)
            print('errorCode:', resp.errorCode)
            print('errorMessage:', resp.errorMessage)

    except:
        log('Put Content Failed')          
        log(traceback.format_exc())


def main():
    if checkRemoteTagsUpdates() :
        localVersion = runGitCommand(f'git -C {FLUTTER_ENGINE_PATH} rev-parse HEAD')
        log(localVersion)

        # 获取编译产物
        for buildType in OHOS_ENGINE_TYPE_OUT:
            zipfiles = getCompileFiles(OHOS_ENGINE_TYPE_OUT[buildType])
            for filePath in zipfiles:
                if not filePath or not os.path.exists(filePath):
                    continue
                print(filePath)
                uploadServer(localVersion, buildType, filePath)

        log('上传完成')

    else :
        log("本地代码已经是最新")

if __name__ == "__main__":
    exit(main())
