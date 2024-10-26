import logging
import platform
import os
import subprocess
from pathlib import Path
import sys
from obs import ObsClient, PutObjectHeader

log = logging.getLogger(__name__)

FLUTTER_OHOS = 'flutter-ohos'

# OBS 环境变量配置key信息
ACCESS_KEY = os.getenv("AccessKeyID")
SECRET_KEY = os.getenv("SecretAccessKey")

# 服务器地址 华南-广州
SERVER = "https://obs.cn-south-1.myhuaweicloud.com"


def upload(version, file_path):
  bucketName = FLUTTER_OHOS
  obsClient = ObsClient(access_key_id=ACCESS_KEY, secret_access_key=SECRET_KEY, server=SERVER)
  headers = PutObjectHeader()
  headers.contentType = 'text/plain'
  file_name = os.path.basename(file_path)
  objectKey = f'flutter_infra_release/flutter/{version}/{file_name}'
  print(f'objectKey: {objectKey}')
  resp = obsClient.putFile(
      bucketName, objectKey, file_path, headers=headers, progressCallback=uploadCallback
  )
  if resp.status < 300:
    print('Put Content Succeeded')
    print('requestId:', resp.requestId)
    print('etag:', resp.body.etag)
  else:
    print('Put Content Failed')
    print('requestId:', resp.requestId)
    print('errorCode:', resp.errorCode)
    print('errorMessage:', resp.errorMessage)


def getEnginePath():
  return Path(os.path.realpath(__file__)).parents[4]


def getArch():
  os = platform.system().lower()
  cpu_arch = platform.machine()
  if cpu_arch == 'arm64':
    return f'{os}-{cpu_arch}'
  elif cpu_arch == 'x86_64':
    return f'{os}-x64'
  elif cpu_arch == 'AMD64':
    return f'{os}-x64'
  else:
    log.error(f"Error: {os}-{cpu_arch} unsupported arch")
    exit(1)


def runGitCommand(command):
  result = subprocess.run(command, capture_output=True, text=True, shell=True)
  if result.returncode != 0:
    raise Exception(f"Git command failed: {result.stderr}")
  return result.stdout.strip()


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
