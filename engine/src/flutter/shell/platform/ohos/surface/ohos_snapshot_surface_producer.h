/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef OHOS_SNAPSHOT_SURFACE_PRODUCER_H
#define OHOS_SNAPSHOT_SURFACE_PRODUCER_H
#include "flutter/flow/surface.h"
#include "flutter/shell/common/snapshot_surface_producer.h"
#include "flutter/shell/platform/ohos/surface/ohos_surface.h"

namespace flutter {
class OHOSSnapshotSurfaceProducer : public SnapshotSurfaceProducer {
 public:
  explicit OHOSSnapshotSurfaceProducer(OHOSSurface& ohos_surface);

  // |SnapshotSurfaceProducer|
  std::unique_ptr<Surface> CreateSnapshotSurface() override;

 private:
  OHOSSurface& ohos_surface_;
};
}  // namespace flutter

#endif