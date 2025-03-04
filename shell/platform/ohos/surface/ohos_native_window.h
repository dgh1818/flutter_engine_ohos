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

  // void SetHdr(bool hdr);

  // int GetHdr() const;

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
