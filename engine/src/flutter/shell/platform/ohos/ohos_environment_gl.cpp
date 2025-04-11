/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#include "flutter/shell/platform/ohos/ohos_environment_gl.h"
#include "fml/trace_event.h"

namespace flutter {

OhosEnvironmentGL::OhosEnvironmentGL()
    : display_(EGL_NO_DISPLAY), valid_(false) {
  // Get the display.
  TRACE_EVENT0("flutter", "OhosEnvironmentGL");
  display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);

  if (display_ == EGL_NO_DISPLAY) {
    return;
  }

  // Initialize the display connection.
  if (eglInitialize(display_, nullptr, nullptr) != EGL_TRUE) {
    return;
  }

  valid_ = true;
}

OhosEnvironmentGL::~OhosEnvironmentGL() {
  // Disconnect the display if valid.
  if (display_ != EGL_NO_DISPLAY) {
    eglTerminate(display_);
  }
}

bool OhosEnvironmentGL::IsValid() const {
  return valid_;
}

EGLDisplay OhosEnvironmentGL::Display() const {
  return display_;
}

}  // namespace flutter
