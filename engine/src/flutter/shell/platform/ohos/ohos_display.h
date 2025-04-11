/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_OHOS_DISPLAY_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_OHOS_DISPLAY_H_

#include <cstdint>

#include "flutter/fml/macros.h"
#include "flutter/shell/common/display.h"
#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"
namespace flutter {

class OHOSDisplay : public Display {
 public:
  explicit OHOSDisplay(std::shared_ptr<PlatformViewOHOSNapi> napi_facade_);
  ~OHOSDisplay() = default;

  double GetRefreshRate() const override;

  // |Display|
  virtual double GetWidth() const override;

  // |Display|
  virtual double GetHeight() const override;

  // |Display|
  virtual double GetDevicePixelRatio() const override;

 private:
  std::shared_ptr<PlatformViewOHOSNapi> napi_facade_;
  FML_DISALLOW_COPY_AND_ASSIGN(OHOSDisplay);
};
}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_OHOS_DISPLAY_H_