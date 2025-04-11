/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_OHOS_CONTEXT_GL_IMPELLER_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_OHOS_CONTEXT_GL_IMPELLER_H_
#include "context/ohos_context.h"
#include "flutter/fml/macros.h"

namespace flutter {
class OHOSContextGLImpeller : public OHOSContext {
 public:
  OHOSContextGLImpeller();

  ~OHOSContextGLImpeller() override;

  bool IsValid() const override;

 private:
  FML_DISALLOW_COPY_AND_ASSIGN(OHOSContextGLImpeller);
};
}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_OHOS_CONTEXT_GL_IMPELLER_H_