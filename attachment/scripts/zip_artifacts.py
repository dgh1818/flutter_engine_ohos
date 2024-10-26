import json
import os
import zipfile
import logging
from utils import getArch
from pathlib import PurePath
from pathlib import Path

log = logging.getLogger(__name__)

artifact_json = "artifacts_files.json"


def getFileMap():
  script_path = os.path.realpath(__file__)
  script_dir = PurePath(script_path).parent
  zip_artifact_json = script_dir.joinpath(artifact_json).__str__()
  with open(zip_artifact_json, 'r') as jsonFile:
    jsonData = json.load(jsonFile)
  return jsonData


# BUILD.gn file does not exist in out/host_release/shader_lib.
# However, Google's flutter artifacts.zip contains it, so I add it manually here
# See. https://storage.flutter-io.cn/flutter_infra_release/flutter/edd8546116457bdf1c5bdfb13ecb9463d2bb5ed4/darwin-arm64/artifacts.zip

shader_flutter_build = '''# Copyright 2013 The Flutter Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

copy("flutter") {
  sources = [ "runtime_effect.glsl" ]
  outputs = [ "$root_out_dir/shader_lib/flutter/{{source_file_part}}" ]
}
'''

shader_impeller_build = '''# Copyright 2013 The Flutter Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

copy("impeller") {
  sources = [
    "blending.glsl",
    "branching.glsl",
    "color.glsl",
    "constants.glsl",
    "conversions.glsl",
    "dithering.glsl",
    "external_texture_oes.glsl",
    "gaussian.glsl",
    "gradient.glsl",
    "path.glsl",
    "texture.glsl",
    "tile_mode.glsl",
    "transform.glsl",
    "types.glsl",
  ]
  outputs = [ "$root_out_dir/shader_lib/impeller/{{source_file_part}}" ]
}
'''

shader_gn_build = '''# Copyright 2013 The Flutter Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

group("shader_lib") {
  deps = [
    "./flutter",
    "./impeller",
  ]
}
'''


def genZipFile():
  engine_project_root_dir = Path(os.path.realpath(__file__)).parents[4]
  files_map = getFileMap()[getArch()]
  with zipfile.ZipFile(engine_project_root_dir.joinpath('artifacts.zip'), 'w',
                       zipfile.ZIP_DEFLATED) as zipf:
    for file_to_zip in files_map:
      path = engine_project_root_dir.joinpath(file_to_zip)
      if not path.exists():
        log.error(f"Error: file {path} does not exist")
        exit(2)
      else:
        if path.is_file():
          log.info('Compressing:' + path.__str__())
          zipf.write(path, files_map[file_to_zip])
        elif Path(path).is_dir():
          dir_name_in_zip = files_map[file_to_zip]
          for entry in path.rglob("*"):
            log.info(
                'Compressing:' + Path(dir_name_in_zip).joinpath(entry.relative_to(path)).__str__()
            )
            zipf.write(entry, Path(dir_name_in_zip).joinpath(entry.relative_to(path)))
      zipf.writestr('shader_lib/flutter/BUILD.gn', shader_flutter_build)
      zipf.writestr('shader_lib/impeller/BUILD.gn', shader_impeller_build)
      zipf.writestr('shader_lib/BUILD.gn', shader_gn_build)


def main():
  genZipFile()


if __name__ == "__main__":
  exit(main())
