// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/flow/surface_frame.h"

#include <limits>
#include <utility>

#include "flutter/display_list/geometry/dl_geometry_conversions.h"
#include "flutter/fml/logging.h"
#include "flutter/fml/trace_event.h"

#include "third_party/skia/include/core/SkSurface.h"

namespace flutter {

SurfaceFrame::SurfaceFrame(sk_sp<SkSurface> surface,
                           FramebufferInfo framebuffer_info,
                           const EncodeCallback& encode_callback,
                           const SubmitCallback& submit_callback,
                           DlISize frame_size,
                           std::unique_ptr<GLContextResult> context_result,
                           bool display_list_fallback)
    : surface_(std::move(surface)),
      framebuffer_info_(framebuffer_info),
      encode_callback_(encode_callback),
      submit_callback_(submit_callback),
      context_result_(std::move(context_result)) {
  FML_DCHECK(submit_callback_);
  if (surface_) {
#if !SLIMPELLER
    adapter_.set_canvas(surface_->getCanvas());
    canvas_ = &adapter_;
#else   //  !SLIMPELLER
    FML_LOG(FATAL) << "Impeller opt-out unavailable.";
    return;
#endif  //  !SLIMPELLER
  } else if (display_list_fallback) {
    FML_DCHECK(!frame_size.IsEmpty());
    // The root frame of a surface will be filled by the layer_tree which
    // performs branch culling so it will be unlikely to need an rtree for
    // further culling during `DisplayList::Dispatch`. Further, this canvas
    // will live underneath any platform views so we do not need to compute
    // exact coverage to describe "pixel ownership" to the platform.
    dl_builder_ = sk_make_sp<DisplayListBuilder>(DlRect::MakeSize(frame_size),
                                                 /*prepare_rtree=*/false);
    canvas_ = dl_builder_.get();
  }
}

bool SurfaceFrame::Encode() {
  TRACE_EVENT0("flutter", "SurfaceFrame::Encode");
  if (encoded_) {
    return false;
  }

  encoded_ = PerformEncode();

  return encoded_;
}

bool SurfaceFrame::Submit() {
  TRACE_EVENT0("flutter", "SurfaceFrame::Submit");
  if (!encoded_ && !Encode()) {
    return false;
  }

  if (submitted_) {
    FML_LOG(WARNING) << "SurfaceFrame::Submit already submitted";
    return false;
  }

  submitted_ = PerformSubmit();
  if (!submitted_) {
    FML_LOG(ERROR) << "SurfaceFrame::Submit failed";
  }

  return submitted_;
}

bool SurfaceFrame::IsSubmitted() const {
  return submitted_;
}

DlCanvas* SurfaceFrame::Canvas() {
  return canvas_;
}

sk_sp<SkSurface> SurfaceFrame::SkiaSurface() const {
  return surface_;
}

bool SurfaceFrame::PerformEncode() {
  if (encode_callback_ == nullptr) {
    FML_LOG(ERROR) << "SurfaceFrame::PerformEncode submit_callback is null";
    return false;
  }

  if (encode_callback_(*this, Canvas())) {
    return true;
  }
  FML_LOG(ERROR) << "SurfaceFrame::PerformEncode submit_callback failed";
  return false;
}

bool SurfaceFrame::PerformSubmit() {
  if (submit_callback_ == nullptr) {
    FML_LOG(ERROR) << "SurfaceFrame::PerformSubmit submit_callback is null";
    return false;
  }

  if (submit_callback_(*this)) {
    return true;
  }
  FML_LOG(ERROR) << "SurfaceFrame::PerformSubmit submit_callback failed";

  return false;
}

sk_sp<DisplayList> SurfaceFrame::BuildDisplayList() {
  TRACE_EVENT0("impeller", "SurfaceFrame::BuildDisplayList");
  return dl_builder_ ? dl_builder_->Build() : nullptr;
}

}  // namespace flutter
