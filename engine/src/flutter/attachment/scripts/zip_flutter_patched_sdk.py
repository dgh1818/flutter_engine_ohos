import os
import zipfile
import logging
from pathlib import Path

log = logging.getLogger(__name__)


def genZipFile(is_product):
  if is_product:
    product_suffix = '_product'
    sdk_rel_path = 'src/out/ohos_release_arm64/flutter_patched_sdk/'
    zip_file_name = 'flutter_patched_sdk_product.zip'
  else:
    product_suffix = ''
    sdk_rel_path = 'src/out/ohos_debug_unopt_arm64/flutter_patched_sdk/'
    zip_file_name = 'flutter_patched_sdk.zip'
  print(f'zipping {zip_file_name}...')
  engine_project_root_path = Path(os.path.realpath(__file__)).parents[4]
  patched_sdk_path = engine_project_root_path.joinpath(sdk_rel_path)
  zip_file_prefix = Path('flutter_patched_sdk' + product_suffix)
  with zipfile.ZipFile(engine_project_root_path.joinpath(zip_file_name), 'w',
                       zipfile.ZIP_DEFLATED) as zipf:
    for entry in patched_sdk_path.rglob("*.dill"):
      if (entry.is_dir()):
        continue
      else:
        zip_path = zip_file_prefix.joinpath(entry.relative_to(patched_sdk_path))
        print(zip_path)
        zipf.write(entry, zip_path)


def main():
  genZipFile(True)
  genZipFile(False)


if __name__ == "__main__":
  exit(main())
