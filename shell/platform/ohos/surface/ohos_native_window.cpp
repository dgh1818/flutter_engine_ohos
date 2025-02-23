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

#include "flutter/shell/platform/ohos/surface/ohos_native_window.h"
#include "flutter/fml/logging.h"

namespace flutter {

OHOSNativeWindow::OHOSNativeWindow(Handle window)
    : window_(window), is_fake_window_(false) {
  FML_LOG(INFO) << " native_window:" << (int64_t)window_;
}

OHOSNativeWindow::OHOSNativeWindow(Handle window, bool is_fake_window)
    : window_(window), is_fake_window_(is_fake_window) {}

OHOSNativeWindow::~OHOSNativeWindow() {
  if (window_ != nullptr) {
    window_ = nullptr;
  }
}

bool OHOSNativeWindow::IsValid() const {
  return window_ != nullptr;
}

SkISize OHOSNativeWindow::GetSize() const {
  if (window_ != nullptr) {
    int32_t width, height;
    int ret = OH_NativeWindow_NativeWindowHandleOpt(
        window_, GET_BUFFER_GEOMETRY, &height, &width);
    if (ret != 0) {
      FML_LOG(ERROR) << "OH_NativeWindow_NativeWindowHandleOpt GetSize err:"
                     << ret;
      return SkISize::Make(0, 0);
    }
    return SkISize::Make(width, height);
  }
  return SkISize::Make(0, 0);
}

void OHOSNativeWindow::SetSize(int width, int height) {
  if (window_ != nullptr) {
    int ret = OH_NativeWindow_NativeWindowHandleOpt(
        window_, SET_BUFFER_GEOMETRY, width, height);
    if (ret != 0) {
      FML_LOG(ERROR) << "OHOSNativeWindow setSize failed:" << ret;
    }
  }
  return;
}

int OHOSNativeWindow::GetHdr() const {
  if (window_ != nullptr) {
    int32_t color_gamut;
    int ret = OH_NativeWindow_NativeWindowHandleOpt(
        window_, GET_COLOR_GAMUT, &color_gamut);
    if (ret != 0) {
      FML_LOG(ERROR) << "OH_NativeWindow_NativeWindowHandleOpt GetSize err:"
                     << ret;
      return -1;
    }

    if(color_gamut == 9){
      return 1;
    }else{
      return 0;
    }
  }
  return -1;
}

void OHOSNativeWindow::SetHdr(bool hdr) {
  if (window_ != nullptr) {
    int ret = -1;

    if(hdr==1){
      // ret = OH_NativeWindow_NativeWindowHandleOpt(
      //   window_, SET_COLOR_GAMUT, NATIVEBUFFER_COLOR_GAMUT_BT2100_HLG);
      ret = OH_NativeWindow_NativeWindowHandleOpt(
        window_, SET_COLOR_GAMUT, 9);
      ret = OH_NativeWindow_NativeWindowHandleOpt(
        window_, SET_FORMAT, 34);
    }else if(hdr==0){
      ret = OH_NativeWindow_NativeWindowHandleOpt(
        window_, SET_COLOR_GAMUT, 0);
      ret = OH_NativeWindow_NativeWindowHandleOpt(
        window_, SET_FORMAT, 12);
    }

    FML_DLOG(INFO) << "OHOSNativeWindow setHdr" << hdr;
    
    if (ret != 0) {
      FML_LOG(ERROR) << "OHOSNativeWindow setHdr failed:" << ret;
    }
  }
  return;
}

OHOSNativeWindow::Handle OHOSNativeWindow::Gethandle() const {
  return window_;
}

OHOSNativeWindow::Handle OHOSNativeWindow::handle() const {
  return window_;
}

}  // namespace flutter
