import logging
import argparse
from pathlib import Path
from utils import getEnginePath, runGitCommand
from utils import upload

engine_path = getEnginePath()

logging.basicConfig()
log = logging.getLogger()
log.setLevel(logging.INFO)


def main():
  parser = argparse.ArgumentParser(
      prog='upload sky_engine', description='upload sky_engine', epilog='upload sky_engine'
  )
  parser.add_argument('-t', '--tag')
  parser.add_argument('-f', '--file')
  args = parser.parse_args()
  file_name = 'sky_engine.zip'
  if args.file:
    file_path = args.file
  else:
    file_path = Path(engine_path).joinpath(file_name).__str__()
  if args.tag:
    version = args.tag
  else:
    flutter_path = Path(engine_path).joinpath('src/flutter').__str__()
    version = runGitCommand(f'git -C {flutter_path} rev-parse HEAD')
  log.info(version)
  upload(version, file_path)


if __name__ == "__main__":
  exit(main())
