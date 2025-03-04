/*
* Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd.
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "txt/platform.h"
#include "third_party/skia/include/ports/SkFontMgr_ohos_api.h"

namespace txt {

std::vector<std::string> GetDefaultFontFamilies() {
  return {"sans-serif"};
}

sk_sp<SkFontMgr> GetDefaultFontManager(uint32_t font_initialization_data) {
  static sk_sp<SkFontMgr> mgr = SkFontMgr_New_OHOS();
  return mgr;
}

}  // namespace txt

