/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_SURFACE_OHOS_NATIVE_WINDOW_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_SURFACE_OHOS_NATIVE_WINDOW_H_

#include "flutter/fml/macros.h"
#include "flutter/fml/memory/ref_counted.h"
#include "third_party/skia/include/core/SkSize.h"

#include <native_window/external_window.h>

namespace flutter {

/*
  class adapater for ThreadSafe
*/
class OHOSNativeWindow : public fml::RefCountedThreadSafe<OHOSNativeWindow> {
 public:
  using Handle = OHNativeWindow*;

  Handle Gethandle() const;

  bool IsValid() const;

  SkISize GetSize() const;

  void SetSize(int width, int height);

  Handle handle() const;

  /// Returns true when this HarmonyOS NativeWindow is not backed by a real
  /// window (used for testing).
  bool IsFakeWindow() const { return is_fake_window_; }

 private:
  Handle window_;
  const bool is_fake_window_;

  explicit OHOSNativeWindow(Handle window);
  explicit OHOSNativeWindow(Handle window, bool is_fake_window);

  ~OHOSNativeWindow();

  FML_FRIEND_MAKE_REF_COUNTED(OHOSNativeWindow);
  FML_FRIEND_REF_COUNTED_THREAD_SAFE(OHOSNativeWindow);
  FML_DISALLOW_COPY_AND_ASSIGN(OHOSNativeWindow);
};
}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_OHOS_SURFACE_OHOS_NATIVE_WINDOW_H_
