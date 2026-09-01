// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/lib/ui/painting/set_hdr.h"
#include "flutter/impeller/renderer/context.h"

#include "flutter/lib/ui/painting/matrix.h"
#include "flutter/lib/ui/ui_dart_state.h"
#include "third_party/tonic/converter/dart_converter.h"
#include "third_party/tonic/dart_args.h"
#include "third_party/tonic/dart_binding_macros.h"
#include "third_party/tonic/dart_library_natives.h"

namespace flutter {

IMPLEMENT_WRAPPERTYPEINFO(ui, SetHdr);

void SetHdr::Create(Dart_Handle wrapper) {
  // UIDartState::ThrowIfUIOperationsProhibited();
  auto res = fml::MakeRefCounted<SetHdr>();
  res->AssociateWithDartWrapper(wrapper);
}

void SetHdr::initSetHdr(int hdr, bool is_image) {
#ifdef __OHOS__
  impeller::Context::is_image_ = is_image;
  FML_DLOG(ERROR) << "is_image= " << impeller::Context::is_image_;
  if (hdr >= 0) {
    FML_DLOG(ERROR) << "set hdr:" << hdr;
    impeller::Context::hdr_ = hdr;
  } else if (!is_image) {
    // Video updates HDR mode from the native buffer color space. Reset here so
    // SDR video does not inherit a previous image/video HDR swapchain.
    impeller::Context::hdr_ = 0;
  }
#endif
}

void SetHdr::enableHdr(bool enable_hdr) {
#ifdef __OHOS__
  impeller::Context::enable_hdr_ = enable_hdr;
  FML_LOG(ERROR) << "SetHdrProbe enableHdr=" << enable_hdr
                 << " hdr=" << impeller::Context::hdr_;
  if (!enable_hdr) {
    impeller::Context::hdr_ = 0;
  }
#endif
}

SetHdr::SetHdr() {}
SetHdr::~SetHdr() {}

}  // namespace flutter
