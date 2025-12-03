/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_OHOS_CONTEXT_GL_SKIA_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_OHOS_CONTEXT_GL_SKIA_H_

#include "flutter/fml/macros.h"
#include "flutter/fml/memory/ref_counted.h"
#include "flutter/fml/memory/ref_ptr.h"
#include "flutter/shell/common/platform_view.h"
#include "flutter/shell/platform/ohos/context/ohos_context.h"
#include "flutter/shell/platform/ohos/ohos_environment_gl.h"
#include "flutter/shell/platform/ohos/surface/ohos_native_window.h"
#include "third_party/skia/include/core/SkSize.h"

namespace flutter {

class OhosEGLSurface;

//------------------------------------------------------------------------------
/// The Ohos context is used by `OhosSurfaceGL` to create and manage
/// EGL surfaces.
///
/// This context binds `EGLContext` to the current rendering thread and to the
/// draw and read `EGLSurface`s.
///
class OhosContextGLSkia : public OHOSContext {
 public:
  OhosContextGLSkia(OHOSRenderingAPI rendering_api,
                    const TaskRunners& taskRunners);

  ~OhosContextGLSkia();

  //----------------------------------------------------------------------------
  /// @brief      Allocates an new EGL window surface that is used for on-screen
  ///             pixels.
  ///
  /// @return     The window surface.
  ///
  std::unique_ptr<OhosEGLSurface> CreateOnscreenSurface(
      const fml::RefPtr<OHOSNativeWindow>& window) const;

  //----------------------------------------------------------------------------
  /// @brief      Allocates an 1x1 pbuffer surface that is used for making the
  ///             offscreen current for texture uploads.
  ///
  /// @return     The pbuffer surface.
  ///
  std::unique_ptr<OhosEGLSurface> CreateOffscreenSurface() const;

  //----------------------------------------------------------------------------
  /// @brief      Allocates an 1x1 pbuffer surface that is used for making the
  ///             onscreen context current for snapshotting.
  ///
  /// @return     The pbuffer surface.
  ///
  std::unique_ptr<OhosEGLSurface> CreatePbufferSurface(int width = 1,
                                                       int height = 1) const;

  //----------------------------------------------------------------------------
  /// @return     The Ohos environment that contains a reference to the
  /// display.
  ///
  fml::RefPtr<OhosEnvironmentGL> Environment() const;

  //----------------------------------------------------------------------------
  /// @return     Whether the current context is valid. That is, if the EGL
  ///             contexts were successfully created.
  ///
  bool IsValid() const override;

  //----------------------------------------------------------------------------
  /// @return     Whether the current context was successfully clear.
  ///
  bool ClearCurrent() const;

  //----------------------------------------------------------------------------
  /// @brief      Create a new EGLContext using the same EGLConfig.
  ///
  /// @return     The EGLContext.
  ///
  EGLContext CreateNewContext() const;

  //----------------------------------------------------------------------------
  /// @brief      The EGLConfig for this context.
  ///
  EGLConfig Config() const { return config_; }

 private:
  fml::RefPtr<OhosEnvironmentGL> environment_;
  EGLConfig config_;
  EGLContext context_;
  EGLContext resource_context_;
  bool valid_ = false;
  TaskRunners task_runners_;

  FML_DISALLOW_COPY_AND_ASSIGN(OhosContextGLSkia);
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_OHOS_OHOS_CONTEXT_GL_SKIA_H_
