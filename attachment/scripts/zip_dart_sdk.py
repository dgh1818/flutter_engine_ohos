import os
import zipfile
import platform
import logging
from pathlib import Path
from utils import getArch

log = logging.getLogger(__name__)


def genZipFile():
  engine_project_root_dir = Path(os.path.realpath(__file__)).parents[4]
  dart_sdk_path = engine_project_root_dir.joinpath("src/out/host_release/dart-sdk")
  host_release_path = engine_project_root_dir.joinpath("src/out/host_release/")
  arch = getArch()
  with zipfile.ZipFile(engine_project_root_dir.joinpath(f'dart-sdk-{arch}.zip'), 'w',
                       zipfile.ZIP_DEFLATED) as zipf:
    for entry in dart_sdk_path.rglob("*"):
      if (entry.is_dir()):
        continue
      else:
        print(entry.relative_to(host_release_path))
        zipf.write(entry, entry.relative_to(host_release_path))


def main():
  genZipFile()


if __name__ == "__main__":
  exit(main())
