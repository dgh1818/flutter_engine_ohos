/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#include "flutter/shell/platform/ohos/ohos_display.h"

namespace flutter {

OHOSDisplay::OHOSDisplay(std::shared_ptr<PlatformViewOHOSNapi> napi_facade)
    : Display(0,
              napi_facade_->display_refresh_rate,
              napi_facade_->display_width,
              napi_facade_->display_height,
              napi_facade_->display_density_pixels),
      napi_facade_(std::move(napi_facade)) {}

double OHOSDisplay::GetRefreshRate() const {
  return (double)napi_facade_->display_refresh_rate;
}

double OHOSDisplay::GetWidth() const {
  return (double)napi_facade_->display_width;
}

double OHOSDisplay::GetHeight() const {
  return (double)napi_facade_->display_height;
}

double OHOSDisplay::GetDevicePixelRatio() const {
  return napi_facade_->display_density_pixels;
}

}  // namespace flutter