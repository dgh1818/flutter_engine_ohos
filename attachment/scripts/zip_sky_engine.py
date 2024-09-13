import os
import zipfile
import logging
from pathlib import Path
from utils import getArch

log = logging.getLogger(__name__)


def genZipFile():
  engine_project_root_dir = Path(os.path.realpath(__file__)).parents[4]
  sky_engine_path = engine_project_root_dir.joinpath("src/out/host_release/gen/dart-pkg/sky_engine")
  host_release_path = engine_project_root_dir.joinpath("src/out/host_release/gen/dart-pkg")
  arch = getArch()
  with zipfile.ZipFile(engine_project_root_dir.joinpath('sky_engine.zip'), 'w',
                       zipfile.ZIP_DEFLATED) as zipf:
    for entry in sky_engine_path.rglob("*"):
      if (entry.is_dir()):
        continue
      elif (entry.is_symlink()):
        link = str(entry)
        real = str(entry.readlink())
        print(f'found sym_link: {link} -> {real}')
        zipf.write(real, entry.relative_to(host_release_path))
      else:
        print(entry.relative_to(host_release_path))
        zipf.write(entry, entry.relative_to(host_release_path))


def main():
  genZipFile()


if __name__ == "__main__":
  exit(main())
