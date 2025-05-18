// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/renderer/context.h"

#include "impeller/core/capture.h"

namespace impeller {

Context::~Context() = default;

Context::Context() : capture(CaptureContext::MakeInactive()) {}

bool Context::UpdateOffscreenLayerPixelFormat(PixelFormat format) {
  return false;
}

int Context::hdr_ = 0;
bool Context::is_image_ = true;
bool Context::enable_hdr_ = false;

}  // namespace impeller
