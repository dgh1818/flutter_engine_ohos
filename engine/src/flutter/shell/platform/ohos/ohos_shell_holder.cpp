/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#include "flutter/shell/platform/ohos/ohos_shell_holder.h"
#include "flutter/fml/native_library.h"
#include "flutter/shell/common/rasterizer.h"
#include "flutter/shell/common/run_configuration.h"
#include "flutter/shell/common/thread_host.h"
#include "flutter/shell/platform/ohos/ohos_display.h"

#include "flutter/shell/platform/ohos/ohos_logging.h"
#include "fml/trace_event.h"
#include "shell/platform/common/client_wrapper/include/flutter/standard_message_codec.h"
#include "third_party/skia/src/ports/skia_ohos/SkFontMgr_ohos.h"
#include "txt/platform.h"

#include <qos/qos.h>
#include <sys/resource.h>
#include <sys/time.h>

namespace flutter {

std::string OHOSLastFontPath = "";

static void OHOSPlatformThreadConfigSetter(
    const fml::Thread::ThreadConfig& config) {
  fml::Thread::SetCurrentThreadName(config);
  // set thread priority
  switch (config.priority) {
    case fml::Thread::ThreadPriority::kBackground: {
      int ret = OH_QoS_SetThreadQoS(QoS_Level::QOS_BACKGROUND);
      FML_DLOG(INFO) << "qos set background result:" << ret
                     << ",tid:" << gettid();
      break;
    }
    case fml::Thread::ThreadPriority::kDisplay: {
      int ret = OH_QoS_SetThreadQoS(QoS_Level::QOS_USER_INTERACTIVE);
      FML_DLOG(INFO) << "qos set display result:" << ret << ",tid:" << gettid();
      break;
    }
    case fml::Thread::ThreadPriority::kRaster: {
      int ret = OH_QoS_SetThreadQoS(QoS_Level::QOS_USER_INTERACTIVE);
      FML_DLOG(INFO) << "qos set raster result:" << ret << ",tid:" << gettid();
      break;
    }
    default:
      int ret = OH_QoS_SetThreadQoS(QoS_Level::QOS_DEFAULT);
      FML_DLOG(INFO) << "qos set default result:" << ret << ",tid:" << gettid();
  }
}

static PlatformData GetDefaultPlatformData() {
  FML_DLOG(INFO) << "GetDefaultPlatformData";
  PlatformData platform_data;
  platform_data.lifecycle_state = "AppLifecycleState.detached";
  return platform_data;
}

static bool EndsWith(std::string str, std::string suffix) {
  if (str.length() < suffix.length()) {
    return false;
  }
  return str.substr(str.length() - suffix.length()) == suffix;
}

static std::string GetFontFileName(std::string path) {
  std::string fontFamilyName = "";
  DIR* dir;
  struct dirent* ent;
  if ((dir = opendir(path.c_str())) == nullptr) {
    return fontFamilyName;
  }
  while ((ent = readdir(dir)) != nullptr) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
      continue;
    }
    if (EndsWith(ent->d_name, ".ttf")) {
      fontFamilyName = ent->d_name;
      break;
    }
  }
  closedir(dir);
  return fontFamilyName;
}

static bool IsFontDirValid(std::string path) {
  DIR* dir;
  struct dirent* ent;
  bool isFlagFileExist = false;
  bool isFontDirExist = false;
  if ((dir = opendir(path.c_str())) == nullptr) {
    if (errno == ENOENT) {
      FML_DLOG(ERROR) << "ERROR ENOENT";
    } else if (errno == EACCES) {
      FML_DLOG(ERROR) << "ERROR EACCES";
    } else {
      FML_DLOG(ERROR) << "ERROR Other";
    }
    return false;
  }
  while ((ent = readdir(dir)) != nullptr) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
      continue;
    }
    if (strcmp(ent->d_name, "flag") == 0) {
      isFlagFileExist = true;
    } else if (strcmp(ent->d_name, "fonts") == 0) {
      isFontDirExist = true;
    }
  }
  closedir(dir);
  if (isFlagFileExist && isFontDirExist) {
    FML_DLOG(INFO) << "font path exist" << path;
    return true;
  }
  return false;
}

static std::string CheckFontSource() {
  std::string path = "/data/themes/a/app";
  if (!IsFontDirValid(path)) {
    path = "/data/themes/b/app";
    if (!IsFontDirValid(path)) {
      return "";
    }
  }
  path = path.append("/fonts/");
  std::string fileName = GetFontFileName(path);
  if (fileName.empty()) {
    return "";
  }
  path = path.append(fileName);
  return path;
}

static bool IsFontChanged(std::string currentPath) {
  if (!currentPath.empty() && currentPath != OHOSLastFontPath) {
    OHOSLastFontPath = currentPath;
    return true;
  } else {
    return false;
  }
}

OHOSShellHolder::OHOSShellHolder(
    const flutter::Settings& settings,
    std::shared_ptr<PlatformViewOHOSNapi> napi_facade,
    void* platform_loop)
    : settings_(settings), napi_facade_(napi_facade) {
  FML_DLOG(INFO) << " ohos shell holder constructor";
  static size_t thread_host_count = 1;
  auto thread_label = std::to_string(thread_host_count++);
  TRACE_EVENT0("OHOSShellHolder", "Create");
  auto mask =
      ThreadHost::Type::kUi | ThreadHost::Type::kRaster | ThreadHost::Type::kIo;

  flutter::ThreadHost::ThreadHostConfig host_config(
      thread_label, mask, OHOSPlatformThreadConfigSetter);
  host_config.ui_config = fml::Thread::ThreadConfig(
      flutter::ThreadHost::ThreadHostConfig::MakeThreadName(
          flutter::ThreadHost::Type::kUi, thread_label),
      fml::Thread::ThreadPriority::kDisplay);
  host_config.raster_config = fml::Thread::ThreadConfig(
      flutter::ThreadHost::ThreadHostConfig::MakeThreadName(
          flutter::ThreadHost::Type::kRaster, thread_label),
      fml::Thread::ThreadPriority::kRaster);
  host_config.io_config = fml::Thread::ThreadConfig(
      flutter::ThreadHost::ThreadHostConfig::MakeThreadName(
          flutter::ThreadHost::Type::kIo, thread_label),
      fml::Thread::ThreadPriority::kNormal);

  thread_host_ = std::make_shared<ThreadHost>(host_config);
  FML_DLOG(INFO) << "thred host config finish";
  fml::WeakPtr<PlatformViewOHOS> weak_platform_view;

  Shell::CreateCallback<PlatformView> on_create_platform_view =
      [&napi_facade, &weak_platform_view](Shell& shell) {
        FML_DLOG(INFO) << "on_create_platform_view";
        std::unique_ptr<PlatformViewOHOS> platform_view_OHOS;
        platform_view_OHOS = std::make_unique<PlatformViewOHOS>(
            shell,                   // delegate
            shell.GetTaskRunners(),  // task runners
            napi_facade,             // napi interop
            shell.GetSettings()
                .enable_software_rendering,   // use software rendering
            shell.GetSettings().msaa_samples  // msaa sample count
        );
        LOGI("on_create_platform_view LOGI");
        FML_LOG(INFO) << "on_create_platform_view end";
        weak_platform_view = platform_view_OHOS->GetWeakPtr();
        LOGI("on_create_platform_view LOGI2");
        FML_LOG(INFO) << "on_create_platform_view end1";
        // std::vector<std::unique_ptr<Display>> displays;
        // displays.push_back(std::make_unique<OHOSDisplay>(napi_facade));
        // FML_DLOG(INFO) << "on_create_platform_view LOGI3";
        // FML_LOG(INFO) << "on_create_platform_view end3---here";
        // shell.OnDisplayUpdates(std::move(displays));
        LOGI("on_create_platform_view LOGI4");
        FML_LOG(INFO) << "on_create_platform_view end3";
        return platform_view_OHOS;
      };

  Shell::CreateCallback<Rasterizer> on_create_rasterizer = [](Shell& shell) {
    FML_DLOG(INFO) << "Create Rasterizer Callback";
    return std::make_unique<Rasterizer>(shell);
  };

  // The current thread will be used as the platform thread. Ensure that the
  // message loop is initialized.
  fml::MessageLoop::EnsureInitializedForCurrentThread(platform_loop);
  fml::RefPtr<fml::TaskRunner> raster_runner;
  fml::RefPtr<fml::TaskRunner> ui_runner;
  fml::RefPtr<fml::TaskRunner> io_runner;
  fml::RefPtr<fml::TaskRunner> platform_runner =
      fml::MessageLoop::GetCurrent().GetTaskRunner();
  raster_runner = thread_host_->raster_thread->GetTaskRunner();
  ui_runner = thread_host_->ui_thread->GetTaskRunner();
  io_runner = thread_host_->io_thread->GetTaskRunner();

  flutter::TaskRunners task_runners(thread_label,     // label
                                    platform_runner,  // platform
                                    raster_runner,    // raster
                                    ui_runner,        // ui
                                    io_runner         // io
  );

  napi_facade_->SetPlatformTaskRunner(platform_runner);
  FML_DLOG(INFO) << "before shell create";
  shell_ =
      Shell::Create(GetDefaultPlatformData(),  // window data
                    task_runners,              // task runners
                    settings_,                 // settings
                    on_create_platform_view,   // platform view create callback
                    on_create_rasterizer       // rasterizer create callback
      );
  FML_DLOG(INFO) << "shell create end";
  if (shell_) {
    shell_->GetDartVM()->GetConcurrentMessageLoop()->PostTaskToAllWorkers([]() {
      if (::setpriority(PRIO_PROCESS, gettid(), 1) != 0) {
        FML_DLOG(ERROR) << "Failed to set Workers task runner priority";
      }
    });

    LOGI("shell_ end");
    shell_->RegisterImageDecoder(
        [](sk_sp<SkData> buffer) {
          return OHOSImageGenerator::MakeFromData(std::move(buffer));
        },
        // OHOS's PixelMap decoding supports hardware decoding, offering higher
        // performance compared to Skia decoding.
        1);

    FML_DLOG(INFO) << "Registered ohos image decoder";
  }

  platform_view_ = weak_platform_view;
  bridge_ = std::make_shared<SemanticsBridge>();
  bridge_mutex_ = std::make_shared<std::mutex>();
  platform_view_->SetSemanticsBridge(bridge_, bridge_mutex_);
  local_font_path_ = OHOSLastFontPath;
  FML_DCHECK(platform_view_);
}

OHOSShellHolder::OHOSShellHolder(
    const Settings& settings,
    const std::shared_ptr<PlatformViewOHOSNapi>& napi_facade,
    const std::shared_ptr<ThreadHost>& thread_host,
    std::unique_ptr<Shell> shell,
    std::unique_ptr<OHOSAssetProvider> hap_asset_provider,
    const fml::WeakPtr<PlatformViewOHOS>& platform_view)
    : settings_(settings),
      platform_view_(platform_view),
      thread_host_(thread_host),
      shell_(std::move(shell)),
      asset_provider_(std::move(hap_asset_provider)),
      napi_facade_(napi_facade) {
  FML_DCHECK(napi_facade);
  FML_DCHECK(shell_);
  FML_DCHECK(shell_->IsSetup());
  FML_DCHECK(platform_view_);
  FML_DCHECK(thread_host_);
  bridge_ = std::make_shared<SemanticsBridge>();
  bridge_mutex_ = std::make_shared<std::mutex>();
  platform_view_->SetSemanticsBridge(bridge_, bridge_mutex_);
  local_font_path_ = OHOSLastFontPath;
}

OHOSShellHolder::~OHOSShellHolder() {
  FML_LOG(INFO) << "MHN enter ~OHOSShellHolder()";
  shell_.reset();
  thread_host_.reset();
  napi_facade_.reset();
  shell_ = nullptr;
}

bool OHOSShellHolder::IsValid() const {
  return shell_ != nullptr;
}

const flutter::Settings& OHOSShellHolder::GetSettings() const {
  return settings_;
}

std::unique_ptr<OHOSShellHolder> OHOSShellHolder::Spawn(
    std::shared_ptr<PlatformViewOHOSNapi> napi_facade,
    const std::string& entrypoint,
    const std::string& libraryUrl,
    const std::string& initial_route,
    const std::vector<std::string>& entrypoint_args) const {
  if (!IsValid()) {
    FML_LOG(ERROR) << "A new Shell can only be spawned "
                      "if the current Shell is properly constructed";
    return nullptr;
  }

  fml::WeakPtr<PlatformViewOHOS> weak_platform_view;
  PlatformViewOHOS* ohos_platform_view = platform_view_.get();
  FML_DCHECK(ohos_platform_view);
  std::shared_ptr<flutter::OHOSContext> ohos_context =
      ohos_platform_view->GetOHOSContext();
  FML_DCHECK(ohos_context);

  Shell::CreateCallback<PlatformView> on_create_platform_view =
      [&napi_facade, ohos_context, &weak_platform_view](Shell& shell) {
        std::unique_ptr<PlatformViewOHOS> platform_view_ohos;
        platform_view_ohos = std::make_unique<PlatformViewOHOS>(
            shell,                   // delegate
            shell.GetTaskRunners(),  // task runners
            napi_facade,             // NAPI interop
            ohos_context             // Ohos context
        );
        weak_platform_view = platform_view_ohos->GetWeakPtr();
        return platform_view_ohos;
      };

  Shell::CreateCallback<Rasterizer> on_create_rasterizer = [](Shell& shell) {
    return std::make_unique<Rasterizer>(shell);
  };

  auto config = BuildRunConfiguration(entrypoint, libraryUrl, entrypoint_args);
  if (!config) {
    // If the RunConfiguration was null, the kernel blob wasn't readable.
    // Fail the whole thing.
    return nullptr;
  }

  std::unique_ptr<flutter::Shell> shell =
      shell_->Spawn(std::move(config.value()), initial_route,
                    on_create_platform_view, on_create_rasterizer);

  return std::unique_ptr<OHOSShellHolder>(new OHOSShellHolder(
      GetSettings(), napi_facade, thread_host_, std::move(shell),
      asset_provider_->Clone(), weak_platform_view));
}

fml::WeakPtr<PlatformViewOHOS> OHOSShellHolder::GetPlatformView() {
  FML_DCHECK(platform_view_);
  return platform_view_;
}

void OHOSShellHolder::NotifyLowMemoryWarning() {
  if (!IsValid()) {
    FML_LOG(ERROR) << "NotifyLowMemoryWarning, Is Not Valid";
    return;
  }
  shell_->NotifyLowMemoryWarning();
}

void OHOSShellHolder::Launch(
    std::unique_ptr<OHOSAssetProvider> hap_asset_provider,
    const std::string& entrypoint,
    const std::string& libraryUrl,
    const std::vector<std::string>& entrypoint_args) {
  FML_DLOG(INFO) << "Launch ...entrypoint<<" << entrypoint
                 << ",libraryUrl:" << libraryUrl;
  if (!IsValid()) {
    FML_LOG(ERROR) << "Launch, Is Not Valid";
    return;
  }

  asset_provider_ = std::move(hap_asset_provider);
  auto config = BuildRunConfiguration(entrypoint, libraryUrl, entrypoint_args);
  if (!config) {
    return;
  }
  std::vector<std::unique_ptr<Display>> displays;
  displays.push_back(std::make_unique<OHOSDisplay>(napi_facade_));
  FML_DLOG(INFO) << "on_create_platform_view LOGI3";
  FML_LOG(INFO) << "on_create_platform_view end3---here";
  shell_->OnDisplayUpdates(std::move(displays));
  FML_LOG(INFO) << "start RunEngine";
  shell_->RunEngine(std::move(config.value()));
  FML_LOG(INFO) << "end Launch";
}

void OHOSShellHolder::InitializeSystemFont() {
  SkFontMgr_OHOS* mgr = (SkFontMgr_OHOS*)(txt::GetDefaultFontManager().get());

  std::string path = CheckFontSource();
  if (path.empty()) {
    LOGE("system font file not found");
    return;
  }
  if (OHOSLastFontPath.empty()) {
    OHOSLastFontPath = path;
  }
  mgr->InitializeSystemFont(path);
}

void OHOSShellHolder::ReloadSystemFonts() {
  std::string currentPath = CheckFontSource();
  if (IsFontChanged(currentPath)) {
    SkFontMgr_OHOS* mgr = (SkFontMgr_OHOS*)(txt::GetDefaultFontManager().get());
    mgr->AddSystemFont(currentPath);
  }
  if (local_font_path_ != OHOSLastFontPath) {
    local_font_path_ = OHOSLastFontPath;
    shell_->ReloadSystemFonts();
  }
}

std::optional<RunConfiguration> OHOSShellHolder::BuildRunConfiguration(
    const std::string& entrypoint,
    const std::string& libraryUrl,
    const std::vector<std::string>& entrypoint_args) const {
  std::unique_ptr<IsolateConfiguration> isolate_configuration;
  FML_DLOG(INFO) << "entrypoint." << entrypoint.c_str();
  FML_DLOG(INFO) << "libraryUrl." << libraryUrl.c_str();
  if (flutter::DartVM::IsRunningPrecompiledCode()) {
    FML_LOG(INFO) << "isolate_configuration.";
    isolate_configuration = IsolateConfiguration::CreateForAppSnapshot();
  } else {
    std::unique_ptr<fml::Mapping> kernel_blob =
        fml::FileMapping::CreateReadOnly(
            GetSettings().application_kernel_asset);
    if (!kernel_blob) {
      FML_DLOG(ERROR) << "Unable to load the kernel blob asset.";
      return std::nullopt;
    }
    FML_LOG(INFO) << "CreateForKernel.";
    isolate_configuration =
        IsolateConfiguration::CreateForKernel(std::move(kernel_blob));
  }

  RunConfiguration config(std::move(isolate_configuration));
  config.AddAssetResolver(asset_provider_->Clone());

  {
    if (!entrypoint.empty() && !libraryUrl.empty()) {
      FML_LOG(INFO) << "SetEntrypointAndLibrary.";
      config.SetEntrypointAndLibrary(entrypoint, libraryUrl);
    } else if (!entrypoint.empty()) {
      FML_LOG(INFO) << "SetEntrypoint.";
      config.SetEntrypoint(entrypoint);
    }
    if (!entrypoint_args.empty()) {
      FML_LOG(INFO) << "SetEntrypointArgs.";
      config.SetEntrypointArgs(entrypoint_args);
    }
  }
  return config;
}

static ACTIONS_ ArkuiActionsToFlutterActions(
    ArkUI_Accessibility_ActionType arkui_action,
    SemanticsNodeExtend* node) {
  switch (arkui_action) {
    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CLICK:
      return ACTIONS_::kTap;

    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_LONG_CLICK:
      return ACTIONS_::kLongPress;

    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_FORWARD:
      if (node->HasAction(SemanticsAction::kScrollUp)) {
        return ACTIONS_::kScrollUp;
      } else if (node->HasAction(SemanticsAction::kScrollLeft)) {
        return ACTIONS_::kScrollLeft;
      } else if (node->HasAction(SemanticsAction::kIncrease)) {
        return ACTIONS_::kIncrease;
      } else {
        return ACTIONS_::kCustomAction;
      }

    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_BACKWARD:
      if (node->HasAction(SemanticsAction::kScrollDown)) {
        return ACTIONS_::kScrollDown;
      } else if (node->HasAction(SemanticsAction::kScrollRight)) {
        return ACTIONS_::kScrollRight;
      } else if (node->HasAction(SemanticsAction::kDecrease)) {
        return ACTIONS_::kDecrease;
      } else {
        return ACTIONS_::kCustomAction;
      }

    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_COPY:
      return ACTIONS_::kCopy;

    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CUT:
      return ACTIONS_::kCut;

    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_PASTE:
      return ACTIONS_::kPaste;

    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_GAIN_ACCESSIBILITY_FOCUS:
      return ACTIONS_::kDidGainAccessibilityFocus;

    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CLEAR_ACCESSIBILITY_FOCUS:
      return ACTIONS_::kDidLoseAccessibilityFocus;

    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SELECT_TEXT:
      return ACTIONS_::kSetSelection;

    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SET_TEXT:
      return ACTIONS_::kPaste;

    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SET_CURSOR_POSITION:
      return ACTIONS_::kSetSelection;
    default:
      // might not match to the valid action in arkui
      return ACTIONS_::kCustomAction;
  }
}

static const char* ARKUI_ACTION_ARG_SELECT_TEXT_START = "selectTextBegin";
static const char* ARKUI_ACTION_ARG_SELECT_TEXT_END = "selectTextEnd";
static const char* ARKUI_ACTION_ARG_SET_OFFSET = "offset";

static EncodableValue GetAccessibilitySelectText(
    ArkUI_AccessibilityActionArguments* args,
    SemanticsNodeExtend* node) {
  char* textSelectBase = nullptr;
  OH_ArkUI_FindAccessibilityActionArgumentByKey(
      args, ARKUI_ACTION_ARG_SELECT_TEXT_START, &textSelectBase);
  if (textSelectBase == nullptr) {
    LOGE("PerformSelectText -> textSelectBase get null value");
    return EncodableValue();
  }

  char* textSelectExtent = nullptr;
  OH_ArkUI_FindAccessibilityActionArgumentByKey(
      args, ARKUI_ACTION_ARG_SELECT_TEXT_END, &textSelectExtent);
  if (textSelectExtent == nullptr) {
    LOGE("PerformSelectText -> textSelectExtent get null value");
    return EncodableValue();
  }

  bool hasSelected = args != nullptr && textSelectBase != nullptr &&
                     textSelectExtent != nullptr;
  if (hasSelected) {
    node->textSelectionBase = std::stoi(textSelectBase);
    node->textSelectionExtent = std::stoi(textSelectExtent);
  }
  return EncodableValue(EncodableMap{
      {EncodableValue("base"), EncodableValue(node->textSelectionBase)},
      {EncodableValue("extent"), EncodableValue(node->textSelectionExtent)}});
}

static EncodableValue GetAccessibilitySetCursorPosition(
    ArkUI_AccessibilityActionArguments* args,
    SemanticsNodeExtend* node) {
  char* setCursorOffset = nullptr;
  OH_ArkUI_FindAccessibilityActionArgumentByKey(
      args, ARKUI_ACTION_ARG_SET_OFFSET, &setCursorOffset);
  if (setCursorOffset == nullptr) {
    LOGE(
        "PerformSetCursorPosition -> "
        "OH_ArkUI_FindAccessibilityActionArgumentByKey get "
        "null value");
    return EncodableValue();
  }
  node->textSelectionBase = std::stoi(setCursorOffset);
  node->textSelectionExtent = node->textSelectionBase;
  return EncodableValue(EncodableMap{
      {EncodableValue("base"), EncodableValue(node->textSelectionBase)},
      {EncodableValue("extent"), EncodableValue(node->textSelectionExtent)}});
}

void OHOSShellHolder::SetAccessibilityProvider(
    ArkUI_AccessibilityProvider* provider) {
  LOGD("SetAccessibilityProvider %{public}p", provider);
  std::lock_guard<std::mutex> lock(*bridge_mutex_);
  if (bridge_->provider_ohos_) {
    bridge_->old_provider_ohos_ = bridge_->provider_ohos_;
  }
  bridge_->provider_ohos_ = provider;
}

int32_t OHOSShellHolder::FindFocusNode(int32_t id,
                                       ArkUI_AccessibilityFocusType focusType,
                                       ArkUI_AccessibilityElementInfo* info) {
  std::lock_guard<std::mutex> lock(*bridge_mutex_);
  return bridge_->FindFocusNode(id, focusType, info);
}

int32_t OHOSShellHolder::FindNextFocusNode(
    int32_t id,
    ArkUI_AccessibilityFocusMoveDirection direction,
    ArkUI_AccessibilityElementInfo* info) {
  std::lock_guard<std::mutex> lock(*bridge_mutex_);
  return bridge_->FindNextFocusNode(id, direction, info);
}

int32_t OHOSShellHolder::FillNodesWithSearchText(
    int32_t id,
    const char* text,
    ArkUI_AccessibilityElementInfoList* list) {
  std::lock_guard<std::mutex> lock(*bridge_mutex_);
  return bridge_->FillNodesWithSearchText(id, text, list);
  ;
}

int32_t OHOSShellHolder::FillNodesWithSearch(
    int32_t id,
    ArkUI_AccessibilitySearchMode mode,
    ArkUI_AccessibilityElementInfoList* list) {
  std::lock_guard<std::mutex> lock(*bridge_mutex_);
  return bridge_->FillNodesWithSearch(id, mode, list);
}

int32_t OHOSShellHolder::ExecuteAction(
    int64_t elementId,
    ArkUI_Accessibility_ActionType action,
    ArkUI_AccessibilityActionArguments* actionArguments) {
  std::lock_guard<std::mutex> lock(*bridge_mutex_);
  if (bridge_->provider_ohos_ == nullptr) {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }

  auto node = bridge_->GetNodeById((int32_t)elementId);
  if (!node) {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }

  FML_DLOG(INFO) << "ExecuteAction: " << elementId << " action:" << action;

  std::string trace_str =
      "id-" + std::to_string(elementId) + "-action-" + std::to_string(action);
  TRACE_EVENT1("flutter", "ExecuteAction", "action", trace_str.c_str());

  EncodableValue args;
  if (action == ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SELECT_TEXT) {
    args = GetAccessibilitySelectText(actionArguments, node);
  } else if (action ==
             ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SET_CURSOR_POSITION) {
    args = GetAccessibilitySetCursorPosition(actionArguments, node);
  }

  auto flutter_action = ArkuiActionsToFlutterActions(action, node);

  fml::TaskRunner::RunNowOrPostTask(
      shell_->GetTaskRunners().GetPlatformTaskRunner(),
      [this, id = elementId, flutter_action, action, args]() {
        // this ptr is valid because the shell is running.

        std::lock_guard<std::mutex> lock(*bridge_mutex_);
        auto node = bridge_->GetNodeById(id);
        if (!node) {
          return;
        }

        bool needShowOnScreen = false;
        if (flutter_action == ACTIONS_::kCustomAction) {
          bridge_->SendSemanticsEvent(
              node, ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_INVALID, nullptr);
          return;
        }
        if (action == ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SELECT_TEXT) {
          node->performSelectAction = true;
        }

        switch (action) {
          case ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CLICK:
            if (!node->HasAction(SemanticsAction::kTap)) {
              platform_view_->SimulateTouchEvent(node);
              break;
            }
          case ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_LONG_CLICK:
          case ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_FORWARD:
          case ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_BACKWARD:
          case ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_COPY:
          case ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_PASTE:
          case ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CUT:
          case ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SET_TEXT:
          case ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SELECT_TEXT:
          case ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SET_CURSOR_POSITION:
            if (!args.IsNull()) {
              auto encoded_message =
                  StandardMessageCodec::GetInstance().EncodeMessage(args);

              platform_view_->DispatchSemanticsAction(
                  id, flutter_action,
                  fml::MallocMapping::Copy(encoded_message->data(),
                                           encoded_message->size()));
            } else {
              platform_view_->DispatchSemanticsAction(id, flutter_action, {});
            }
            break;
          case ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_GAIN_ACCESSIBILITY_FOCUS:
            bridge_->GainAccessibilityFocus(id, &needShowOnScreen);
            if (needShowOnScreen) {
              platform_view_->DispatchSemanticsAction(
                  id, ACTIONS_::kShowOnScreen, {});
            }
            break;
          case ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CLEAR_ACCESSIBILITY_FOCUS:
            bridge_->ClearAccessibilityFocus(id);
            break;
          default:
            break;
        }
      });

  return ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL;
}

int32_t OHOSShellHolder::ClearAccessibilityFocus(int64_t elementId) {
  std::lock_guard<std::mutex> lock(*bridge_mutex_);
  return bridge_->ClearAccessibilityFocus(elementId);
}

int32_t OHOSShellHolder::GetAccessibilityNodeCursorPosition(int64_t elementId,
                                                            int32_t* index) {
  std::lock_guard<std::mutex> lock(*bridge_mutex_);
  return bridge_->GetAccessibilityNodeCursorPosition(elementId, index);
}

std::shared_ptr<PlatformViewOHOSNapi> OHOSShellHolder::GetNapiFacade() {
  return napi_facade_;
}

void OHOSShellHolder::WaitRasterTasksFinished() {
  FML_CHECK(shell_);
  fml::AutoResetWaitableEvent gpu_latch;
  fml::TaskRunner::RunNowOrPostTask(
      shell_->GetTaskRunners().GetRasterTaskRunner(),
      [&gpu_latch] { gpu_latch.Signal(); });
  gpu_latch.Wait();
}

}  // namespace flutter
