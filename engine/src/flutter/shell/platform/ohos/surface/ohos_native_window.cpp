/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#include "flutter/shell/platform/ohos/surface/ohos_native_window.h"

#include "flutter/display_list/geometry/dl_geometry_types.h"
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

DlISize OHOSNativeWindow::GetSize() const {
  if (window_ != nullptr) {
    int32_t width, height;
    int ret = OH_NativeWindow_NativeWindowHandleOpt(
        window_, GET_BUFFER_GEOMETRY, &height, &width);
    if (ret != 0) {
      FML_LOG(ERROR) << "OH_NativeWindow_NativeWindowHandleOpt GetSize err:"
                     << ret;
      return DlISize(0, 0);
    }
    return DlISize(width, height);
  }
  return DlISize(0, 0);
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

OHOSNativeWindow::Handle OHOSNativeWindow::Gethandle() const {
  return window_;
}

OHOSNativeWindow::Handle OHOSNativeWindow::handle() const {
  return window_;
}

}  // namespace flutter
