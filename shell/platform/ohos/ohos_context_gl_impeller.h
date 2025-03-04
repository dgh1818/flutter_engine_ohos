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