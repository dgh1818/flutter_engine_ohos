import os
import logging
import argparse
from pathlib import Path
from utils import getArch, getEnginePath, runGitCommand, upload

engine_path = getEnginePath()

logging.basicConfig()
log = logging.getLogger()
log.setLevel(logging.INFO)


def main():
  parser = argparse.ArgumentParser(
      prog='upload_dart_sdk', description='upload dart sdk', epilog='upload dart sdk'
  )
  parser.add_argument('-t', '--tag')
  parser.add_argument('-f', '--file')
  args = parser.parse_args()
  file_name = 'dart-sdk-' + getArch() + '.zip'
  if args.file:
    file_path = args.file
  else:
    file_path = Path(engine_path).joinpath(file_name).__str__()
  flutter_path = Path(engine_path).joinpath('src/flutter').__str__()
  if args.tag:
    version = args.tag
  else:
    version = runGitCommand(f'git -C {flutter_path} rev-parse HEAD')
  log.info(version)
  upload(version, file_path)


if __name__ == "__main__":
  exit(main())
