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
      prog='upload flutter patched sdk',
      description='upload flutter_patched_sdk',
      epilog='upload flutter_patched_sdk'
  )
  parser.add_argument('-t', '--tag')
  parser.add_argument('-d', '--patched_sdk')
  parser.add_argument('-p', '--patched_sdk_product')
  args = parser.parse_args()
  patched_sdk = 'flutter_patched_sdk.zip'
  patched_sdk_product = 'flutter_patched_sdk_product.zip'
  if args.patched_sdk and args.patched_sdk_product:
    patched_sdk_path = args.patched_sdk
    patched_sdk_product_path = args.patched_sdk_product
  else:
    patched_sdk_path = Path(engine_path).joinpath(patched_sdk).__str__()
    patched_sdk_product_path = Path(engine_path).joinpath(patched_sdk_product).__str__()
  if args.tag:
    version = args.tag
  else:
    flutter_path = Path(engine_path).joinpath('src/flutter').__str__()
    version = runGitCommand(f'git -C {flutter_path} rev-parse HEAD')
  log.info(version)
  upload(version, patched_sdk_path)
  upload(version, patched_sdk_product_path)


if __name__ == "__main__":
  exit(main())
