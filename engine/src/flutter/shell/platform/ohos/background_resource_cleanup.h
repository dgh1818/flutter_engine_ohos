/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_BACKGROUND_RESOURCE_CLEANUP_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_BACKGROUND_RESOURCE_CLEANUP_H_

#include <string>

namespace flutter {

/**
 * GPU reclaim level defines the intensity of GPU resource cleanup.
 *
 * kRestore: Foreground restore/normal operation, no cleanup.
 * kAggressive: Aggressive cleanup for background state or surface destroyed.
 *              Actions: freeGpuResources, teardown onscreen/swapchain.
 */
enum class GpuReclaimLevel {
  kRestore,    // Level 0: Foreground restore/normal operation
  kAggressive  // Level 1: Background, release GPU resources
};

/**
 * Decision for how to apply the reclaim policy.
 */
enum class GpuReclaimDecision {
  kNoChange,   // Keep current reclaim level
  kRestore,    // Restore foreground state
  kAggressive  // Aggressive cleanup
};

/**
 * Application lifecycle state.
 */
enum class AppLifecycleState {
  kResumed,   // App is in foreground and visible
  kInactive,  // App is inactive (e.g., receiving a phone call)
  kHidden,    // App is not visible but still running
  kPaused,    // App is in background
  kDetached   // App is detached
};

struct LifecycleStateMapping {
  const char* platform_name;
  const char* display_name;
  AppLifecycleState state;
};

static constexpr LifecycleStateMapping kLifecycleStateMappings[] = {
    {"AppLifecycleState.resumed", "Resumed", AppLifecycleState::kResumed},
    {"AppLifecycleState.inactive", "Inactive", AppLifecycleState::kInactive},
    {"AppLifecycleState.hidden", "Hidden", AppLifecycleState::kHidden},
    {"AppLifecycleState.paused", "Paused", AppLifecycleState::kPaused},
    {"AppLifecycleState.detached", "Detached", AppLifecycleState::kDetached},
};

// Helper utilities for converting lifecycle/reclaim enums to/from strings.
// Inline to allow inclusion in multiple translation units without ODR
inline const char* LifecycleStateToString(AppLifecycleState state) {
  for (const auto& entry : kLifecycleStateMappings) {
    if (entry.state == state) {
      return entry.display_name;
    }
  }
  return "Unknown";
}

inline const char* ReclaimLevelToString(GpuReclaimLevel level) {
  switch (level) {
    case GpuReclaimLevel::kRestore:
      return "Restore";
    case GpuReclaimLevel::kAggressive:
      return "Aggressive";
  }
  return "Unknown";
}

// Parse the lifecycle state string used by the platform layer (e.g.
// "AppLifecycleState.resumed") into the AppLifecycleState enum.
// Returns true on success, false if the string is unrecognized.
inline bool ParseAppLifecycleState(const std::string& state,
                                   AppLifecycleState& out_state) {
  for (const auto& entry : kLifecycleStateMappings) {
    if (state == entry.platform_name) {
      out_state = entry.state;
      return true;
    }
  }
  return false;
}
}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_BACKGROUND_RESOURCE_CLEANUP_H_
