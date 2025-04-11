/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
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

