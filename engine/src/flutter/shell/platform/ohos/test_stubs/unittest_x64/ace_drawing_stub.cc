/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */
#include "native_drawing/drawing_text_font_descriptor.h"
#include "native_drawing/drawing_text_typography.h"

extern "C" {

void OH_Drawing_DestroyFontDescriptor(OH_Drawing_FontDescriptor* /*descriptor*/) {}

void OH_Drawing_DestroySystemFontConfigInfo(
    OH_Drawing_FontConfigInfo* /*drawFontCfgInfo*/) {}

void OH_Drawing_DestroySystemFontFullNames(OH_Drawing_Array* /*fullNameArray*/) {}

size_t OH_Drawing_GetDrawingArraySize(OH_Drawing_Array* /*drawingArray*/) {
  return 0;
}

OH_Drawing_FontDescriptor* OH_Drawing_GetFontDescriptorByFullName(
    const OH_Drawing_String* /*fullName*/,
    OH_Drawing_SystemFontType /*fontType*/) {
  return nullptr;
}

OH_Drawing_FontConfigInfo* OH_Drawing_GetSystemFontConfigInfo(
    OH_Drawing_FontConfigInfoErrorCode* errorCode) {
  if (errorCode != nullptr) {
    *errorCode = static_cast<OH_Drawing_FontConfigInfoErrorCode>(0);
  }
  return nullptr;
}

const OH_Drawing_String* OH_Drawing_GetSystemFontFullNameByIndex(
    OH_Drawing_Array* /*fullNameArray*/,
    size_t /*index*/) {
  return nullptr;
}

OH_Drawing_Array* OH_Drawing_GetSystemFontFullNamesByType(
    OH_Drawing_SystemFontType /*fontType*/) {
  return nullptr;
}

}  // extern "C"
