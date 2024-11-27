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

#include "flutter/shell/platform/ohos/ohos_shell_holder.h"
#include "flutter/fml/native_library.h"
#include "flutter/shell/common/rasterizer.h"
#include "flutter/shell/common/run_configuration.h"
#include "flutter/shell/common/thread_host.h"
#include "flutter/shell/platform/ohos/ohos_display.h"

#include "flutter/shell/platform/ohos/ohos_logging.h"
#include "fml/trace_event.h"

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
  if (OHOSLastFontPath.empty()) {
    OHOSLastFontPath = path;
  }
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
        -1);

    FML_DLOG(INFO) << "Registered ohos image decoder";
  }

  platform_view_ = weak_platform_view;
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
}

OHOSShellHolder::~OHOSShellHolder() {
  FML_LOG(INFO) << "MHN enter ~OHOSShellHolder()";
  shell_.reset();
  thread_host_.reset();
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
  mgr->InitializeSystemFont(path);
}

void OHOSShellHolder::ReloadSystemFonts() {
  std::string currentPath = CheckFontSource();
  if (IsFontChanged(currentPath)) {
    SkFontMgr_OHOS* mgr = (SkFontMgr_OHOS*)(txt::GetDefaultFontManager().get());
    mgr->AddSystemFont(currentPath);
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

}  // namespace flutter
