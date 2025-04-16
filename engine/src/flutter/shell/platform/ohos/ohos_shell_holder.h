/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_OHOS_SHELL_HOLDER_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_OHOS_SHELL_HOLDER_H_
#define FML_USED_ON_EMBEDDER
#include "flutter/assets/asset_manager.h"
#include "flutter/fml/macros.h"
#include "flutter/fml/unique_fd.h"
#include "flutter/lib/ui/window/viewport_metrics.h"
#include "flutter/runtime/platform_data.h"
#include "flutter/shell/common/run_configuration.h"
#include "flutter/shell/common/shell.h"
#include "flutter/shell/common/thread_host.h"

#include "accessibility/ohos_semantics_bridge.h"
#include "flutter/assets/asset_resolver.h"
#include "flutter/common/settings.h"
#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"
#include "flutter/shell/platform/ohos/platform_view_ohos.h"

#include "napi_common.h"
#include "ohos_asset_provider.h"
#include "ohos_image_generator.h"

namespace flutter {

class OHOSShellHolder {
 public:
  OHOSShellHolder(const flutter::Settings& settings,
                  std::shared_ptr<PlatformViewOHOSNapi> napi_facade,
                  void* plateform_loop);

  ~OHOSShellHolder();
  bool IsValid() const;

  const flutter::Settings& GetSettings() const;

  fml::WeakPtr<PlatformViewOHOS> GetPlatformView();

  void NotifyLowMemoryWarning();

  void Launch(std::unique_ptr<OHOSAssetProvider> hap_asset_provider,
              const std::string& entrypoint,
              const std::string& libraryUrl,
              const std::vector<std::string>& entrypoint_args);

  std::unique_ptr<OHOSShellHolder> Spawn(
      std::shared_ptr<PlatformViewOHOSNapi> napi_facade,
      const std::string& entrypoint,
      const std::string& libraryUrl,
      const std::string& initial_route,
      const std::vector<std::string>& entrypoint_args) const;

  const std::shared_ptr<PlatformMessageHandler>& GetPlatformMessageHandler()
      const {
    return shell_->GetPlatformMessageHandler();
  }

  const std::weak_ptr<VsyncWaiter> GetVsyncWaiter() const {
    return shell_->GetVsyncWaiter();
  }

  static void InitializeSystemFont();

  void ReloadSystemFonts();

  void SetAccessibilityProvider(ArkUI_AccessibilityProvider* provider);

  int32_t FindFocusNode(int32_t id,
                        ArkUI_AccessibilityFocusType focusType,
                        ArkUI_AccessibilityElementInfo* info);
  int32_t FindNextFocusNode(int32_t id,
                            ArkUI_AccessibilityFocusMoveDirection direction,
                            ArkUI_AccessibilityElementInfo* info);

  int32_t FillNodesWithSearchText(int32_t id,
                                  const char* text,
                                  ArkUI_AccessibilityElementInfoList* list);

  int32_t FillNodesWithSearch(int32_t id,
                              ArkUI_AccessibilitySearchMode mode,
                              ArkUI_AccessibilityElementInfoList* list);

  int32_t ExecuteAction(int64_t elementId,
                        ArkUI_Accessibility_ActionType action,
                        ArkUI_AccessibilityActionArguments* actionArguments);

  int32_t ClearAccessibilityFocus(int64_t elementId);

  int32_t GetAccessibilityNodeCursorPosition(int64_t elementId, int32_t* index);

 private:
  std::optional<RunConfiguration> BuildRunConfiguration(
      const std::string& entrypoint,
      const std::string& libraryUrl,
      const std::vector<std::string>& entrypoint_args) const;

  const flutter::Settings settings_;
  std::shared_ptr<SemanticsBridge> bridge_;
  std::shared_ptr<std::mutex> bridge_mutex_;
  fml::WeakPtr<PlatformViewOHOS> platform_view_;
  std::shared_ptr<ThreadHost> thread_host_;
  std::unique_ptr<Shell> shell_;
  uint64_t next_pointer_flow_id_ = 0;
  std::string local_font_path_;

  std::unique_ptr<OHOSAssetProvider> asset_provider_;

  std::shared_ptr<PlatformViewOHOSNapi> napi_facade_;

  OHOSShellHolder(const flutter::Settings& settings,
                  const std::shared_ptr<PlatformViewOHOSNapi>& napi_facade,
                  const std::shared_ptr<ThreadHost>& thread_host,
                  std::unique_ptr<Shell> shell,
                  std::unique_ptr<OHOSAssetProvider> hap_asset_provider,
                  const fml::WeakPtr<PlatformViewOHOS>& platform_view);

  static void ThreadDestructCallback(void* value);

  FML_DISALLOW_COPY_AND_ASSIGN(OHOSShellHolder);
};
}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_OHOS_OHOS_SHELL_HOLDER_H_
