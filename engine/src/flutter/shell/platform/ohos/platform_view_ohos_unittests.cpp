/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#define private public
#include "flutter/shell/platform/ohos/platform_view_ohos.h"
#undef private

#include <gtest/gtest.h>
#include <atomic>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include "flutter/common/constants.h"
#include "flutter/common/settings.h"
#include "flutter/common/task_runners.h"
#include "flutter/display_list/geometry/dl_geometry_types.h"
#include "flutter/fml/mapping.h"
#include "flutter/fml/synchronization/waitable_event.h"
#include "flutter/fml/thread.h"
#include "flutter/lib/ui/semantics/semantics_node.h"
#include "flutter/lib/ui/window/platform_message_response.h"
#include "flutter/lib/ui/window/pointer_data.h"
#include "flutter/lib/ui/window/pointer_data_packet.h"
#include "flutter/shell/common/pointer_data_dispatcher.h"
#include "flutter/shell/common/snapshot_surface_producer.h"
#include "flutter/shell/platform/ohos/accessibility/ohos_semantics_bridge.h"
#include "flutter/shell/platform/ohos/accessibility/ohos_semantics_node.h"
#include "flutter/shell/platform/ohos/context/ohos_context.h"
#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"
#include "flutter/shell/platform/ohos/ohos_external_texture.h"
#include "flutter/shell/platform/ohos/ohos_shell_holder.h"
#include "flutter/shell/platform/ohos/platform_message_handler_ohos.h"
#include "flutter/shell/platform/ohos/surface/ohos_native_window.h"
#include "flutter/shell/platform/ohos/test_stubs/ace_graphic_ndk_stub.h"
#if !defined(OHOS_X64_UNITTEST)
#include <native_image/native_image.h>
#endif

namespace flutter {

std::unique_ptr<OHOSContext> CreateOHOSContext(const TaskRunners& task_runners,
                                               OHOSRenderingAPI rendering_api,
                                               bool enable_vulkan_validation,
                                               bool enable_opengl_gpu_tracing,
                                               bool enable_vulkan_gpu_tracing);

extern std::map<uint64_t, PlatformViewOHOS*> g_texture_platformview_map;
extern std::recursive_mutex g_map_mutex;

namespace testing {

namespace {

Settings MakeTestSettings() {
  Settings settings;
  settings.ohos_rendering_api = OHOSRenderingAPI::kSoftware;
  return settings;
}

// Minimal delegate: no-op overrides except settings, which the
// PlatformViewOHOS constructor queries for the rendering API and HCPP flag.
class NullDelegate : public PlatformView::Delegate {
 public:
  void OnPlatformViewCreated(std::unique_ptr<Surface> surface) override {}
  void OnPlatformViewDestroyed() override {}
  void OnPlatformViewScheduleFrame() override {}
  void OnPlatformViewAddView(int64_t view_id,
                             const ViewportMetrics& viewport_metrics,
                             AddViewCallback callback) override {}
  void OnPlatformViewRemoveView(int64_t view_id,
                                RemoveViewCallback callback) override {}
  void OnPlatformViewSendViewFocusEvent(const ViewFocusEvent& event) override {}
  void OnPlatformViewSetNextFrameCallback(const fml::closure& closure) override {}
  void OnPlatformViewSetViewportMetrics(int64_t view_id,
                                        const ViewportMetrics& metrics) override {}
  void OnPlatformViewDispatchPlatformMessage(
      std::unique_ptr<PlatformMessage> message) override {}
  void OnPlatformViewDispatchPointerDataPacket(
      std::unique_ptr<PointerDataPacket> packet) override {}
  void OnPlatformViewDispatchSemanticsAction(int64_t view_id,
                                             int32_t node_id,
                                             SemanticsAction action,
                                             fml::MallocMapping args) override {}
  void OnPlatformViewSetSemanticsEnabled(bool enabled) override {}
  void OnPlatformViewSetAccessibilityFeatures(int32_t flags) override {}
  void OnPlatformViewRegisterTexture(std::shared_ptr<Texture> texture) override {}
  void OnPlatformViewUnregisterTexture(int64_t texture_id) override {}
  void OnPlatformViewMarkTextureFrameAvailable(int64_t texture_id) override {}
  void LoadDartDeferredLibrary(intptr_t loading_unit_id,
                               std::unique_ptr<const fml::Mapping> snapshot_data,
                               std::unique_ptr<const fml::Mapping> snapshot_instructions) override {}
  void LoadDartDeferredLibraryError(intptr_t loading_unit_id,
                                    const std::string error_message,
                                    bool transient) override {}
  void UpdateAssetResolverByType(std::unique_ptr<AssetResolver> updated_asset_resolver,
                                 AssetResolver::AssetResolverType type) override {}
  const Settings& OnPlatformViewGetSettings() const override { return settings_; }

  Settings settings_;
};

#if defined(OHOS_X64_UNITTEST)

// x64 模拟器：hdc shell 起不了 JIT VM（mmap 无权限），绕开 OHOSShellHolder
// 直接构造 PlatformViewOHOS。
class TestViewHandle {
 public:
  explicit TestViewHandle(const Settings& settings)
      : runners_("test",
                 platform_thread_.GetTaskRunner(),
                 raster_thread_.GetTaskRunner(),
                 ui_thread_.GetTaskRunner(),
                 io_thread_.GetTaskRunner()) {
    delegate_.settings_ = settings;
    napi_facade_ = std::make_shared<PlatformViewOHOSNapi>(nullptr);
    view_ = std::make_unique<PlatformViewOHOS>(
        delegate_, runners_, napi_facade_, /*use_software_rendering=*/true);
  }

  bool IsValid() const { return view_ != nullptr; }
  PlatformViewOHOS* view() { return view_.get(); }

 private:
  NullDelegate delegate_;
  fml::Thread platform_thread_;
  fml::Thread raster_thread_;
  fml::Thread ui_thread_;
  fml::Thread io_thread_;
  TaskRunners runners_;
  std::shared_ptr<PlatformViewOHOSNapi> napi_facade_;
  std::unique_ptr<PlatformViewOHOS> view_;
};

#else

// 真机：原路径，完整 Shell + Dart VM。
class TestViewHandle {
 public:
  explicit TestViewHandle(const Settings& settings)
      : napi_facade_(std::make_shared<PlatformViewOHOSNapi>(nullptr)),
        holder_(
            std::make_unique<OHOSShellHolder>(settings, napi_facade_, nullptr)) {}

  bool IsValid() const { return holder_->IsValid(); }
  PlatformViewOHOS* view() { return holder_->GetPlatformView().get(); }

 private:
  std::shared_ptr<PlatformViewOHOSNapi> napi_facade_;
  std::unique_ptr<OHOSShellHolder> holder_;
};

#endif  // defined(OHOS_X64_UNITTEST)

OHNativeWindow* const kPvUtHandleA = reinterpret_cast<OHNativeWindow*>(0x5000);
OHNativeWindow* const kPvUtHandleB = reinterpret_cast<OHNativeWindow*>(0x5100);

#if defined(OHOS_X64_UNITTEST)

fml::RefPtr<OHOSNativeWindow> MakePvUtWindow(OHNativeWindow* handle) {
  return fml::MakeRefCounted<OHOSNativeWindow>(handle, false);
}

bool PvUtInjectOffscreenAcquireFailure() {
  g_stub_graphic_fail_mask =
      kStubFailNativeImageCreate | kStubFailAcquireNativeWindow;
  return true;
}
void PvUtClearOffscreenAcquireFailure() { g_stub_graphic_fail_mask = 0; }

void PvUtSetWindowGeometry(int32_t width, int32_t height) {
  g_stub_geometry_width = width;
  g_stub_geometry_height = height;
}

class PvUtKnobGuard {
 private:
  GraphicStubKnobGuard stub_guard_;
};

#else

std::vector<OH_NativeImage*>& PvUtRegistryImages() {
  static std::vector<OH_NativeImage*> images;
  return images;
}
int32_t g_pv_ut_window_width = 0;
int32_t g_pv_ut_window_height = 0;

fml::RefPtr<OHOSNativeWindow> MakePvUtWindow(OHNativeWindow* request) {
  if (request == nullptr) {
    return fml::MakeRefCounted<OHOSNativeWindow>(nullptr, false);
  }
  OH_NativeImage* image = OH_NativeImage_Create(0, 0);
  OHNativeWindow* window =
      image != nullptr ? OH_NativeImage_AcquireNativeWindow(image) : nullptr;
  if (image != nullptr) {
    PvUtRegistryImages().push_back(image);
  }
  if (window != nullptr && g_pv_ut_window_width > 0 &&
      g_pv_ut_window_height > 0) {
    OH_NativeWindow_NativeWindowHandleOpt(window, SET_BUFFER_GEOMETRY,
                                          g_pv_ut_window_width,
                                          g_pv_ut_window_height);
  }
  return fml::MakeRefCounted<OHOSNativeWindow>(window, false);
}

bool PvUtInjectOffscreenAcquireFailure() {
  g_stub_graphic_engaged = 1;
  g_stub_graphic_fail_mask =
      kStubFailNativeImageCreate | kStubFailAcquireNativeWindow;
  return true;
}
void PvUtClearOffscreenAcquireFailure() {
  g_stub_graphic_fail_mask = 0;
  g_stub_graphic_engaged = 0;
}

void PvUtSetWindowGeometry(int32_t width, int32_t height) {
  g_pv_ut_window_width = width;
  g_pv_ut_window_height = height;
}

class PvUtKnobGuard {
 public:
  ~PvUtKnobGuard() {
    g_pv_ut_window_width = 0;
    g_pv_ut_window_height = 0;
  }
};

#endif  // defined(OHOS_X64_UNITTEST)

class PvOhosRecordingDelegate : public NullDelegate {
 public:
  void OnPlatformViewCreated(std::unique_ptr<Surface> surface) override {
    std::lock_guard<std::mutex> lock(mutex_);
    created_count_++;
  }
  void OnPlatformViewDestroyed() override {
    std::lock_guard<std::mutex> lock(mutex_);
    destroyed_count_++;
  }
  void OnPlatformViewScheduleFrame() override {
    std::lock_guard<std::mutex> lock(mutex_);
    schedule_frame_count_++;
  }
  void OnPlatformViewSetViewportMetrics(
      int64_t view_id, const ViewportMetrics& metrics) override {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_events_.push_back({view_id, metrics});
  }
  void OnPlatformViewDispatchPlatformMessage(
      std::unique_ptr<PlatformMessage> message) override {
    std::lock_guard<std::mutex> lock(mutex_);
    messages_.push_back(std::move(message));
  }
  void OnPlatformViewDispatchPointerDataPacket(
      std::unique_ptr<PointerDataPacket> packet) override {
    std::lock_guard<std::mutex> lock(mutex_);
    if (packet && packet->GetLength() > 0) {
      pointer_data_.push_back(packet->GetPointerData(0));
    }
  }
  void OnPlatformViewSetSemanticsEnabled(bool enabled) override {
    std::lock_guard<std::mutex> lock(mutex_);
    semantics_enabled_.push_back(enabled);
  }
  void OnPlatformViewSetAccessibilityFeatures(int32_t flags) override {
    std::lock_guard<std::mutex> lock(mutex_);
    feature_flags_.push_back(flags);
  }
  void OnPlatformViewRegisterTexture(std::shared_ptr<Texture>) override {
    std::lock_guard<std::mutex> lock(mutex_);
    register_texture_count_++;
  }
  void OnPlatformViewUnregisterTexture(int64_t texture_id) override {
    std::lock_guard<std::mutex> lock(mutex_);
    unregistered_textures_.push_back(texture_id);
  }
  void OnPlatformViewMarkTextureFrameAvailable(int64_t texture_id) override {
    std::lock_guard<std::mutex> lock(mutex_);
    marked_textures_.push_back(texture_id);
  }
  void OnPlatformViewSetNextFrameCallback(const fml::closure&) override {
    std::lock_guard<std::mutex> lock(mutex_);
    next_frame_callback_count_++;
  }
  void OnPlatformViewAddView(int64_t view_id,
                             const ViewportMetrics&,
                             AddViewCallback callback) override {
    std::lock_guard<std::mutex> lock(mutex_);
    add_view_ids_.push_back(view_id);
    if (callback) {
      callback(false);
    }
  }
  void OnPlatformViewRemoveView(int64_t view_id,
                                RemoveViewCallback callback) override {
    std::lock_guard<std::mutex> lock(mutex_);
    remove_view_ids_.push_back(view_id);
    if (callback) {
      callback(false);
    }
  }
  void LoadDartDeferredLibrary(intptr_t loading_unit_id,
                               std::unique_ptr<const fml::Mapping>,
                               std::unique_ptr<const fml::Mapping>) override {
    std::lock_guard<std::mutex> lock(mutex_);
    deferred_library_ids_.push_back(loading_unit_id);
  }
  void LoadDartDeferredLibraryError(intptr_t loading_unit_id,
                                    const std::string,
                                    bool) override {
    std::lock_guard<std::mutex> lock(mutex_);
    deferred_error_ids_.push_back(loading_unit_id);
  }
  void UpdateAssetResolverByType(std::unique_ptr<AssetResolver>,
                                 AssetResolver::AssetResolverType) override {
    std::lock_guard<std::mutex> lock(mutex_);
    asset_resolver_updates_++;
  }

  int created_count() const { return Get(created_count_); }
  int destroyed_count() const { return Get(destroyed_count_); }
  int schedule_frame_count() const { return Get(schedule_frame_count_); }
  int next_frame_callback_count() const {
    return Get(next_frame_callback_count_);
  }
  int register_texture_count() const { return Get(register_texture_count_); }
  size_t metrics_count() const { return Get(metrics_events_).size(); }
  std::vector<ViewportMetrics> metrics_for(int64_t view_id) const {
    std::vector<ViewportMetrics> out;
    for (const auto& event : Get(metrics_events_)) {
      if (event.first == view_id) {
        out.push_back(event.second);
      }
    }
    return out;
  }
  int message_count(const std::string& channel) const {
    std::lock_guard<std::mutex> lock(mutex_);
    int count = 0;
    for (const auto& message : messages_) {
      if (message->channel() == channel) {
        count++;
      }
    }
    return count;
  }
  size_t pointer_packet_count() const { return Get(pointer_data_).size(); }
  std::optional<PointerData> pointer_data(size_t index) const {
    auto packets = Get(pointer_data_);
    if (index >= packets.size()) {
      return std::nullopt;
    }
    return packets[index];
  }
  std::vector<bool> semantics_enabled() const {
    return Get(semantics_enabled_);
  }
  std::vector<int32_t> feature_flags() const { return Get(feature_flags_); }
  std::vector<int64_t> unregistered_textures() const {
    return Get(unregistered_textures_);
  }
  std::vector<int64_t> add_view_ids() const { return Get(add_view_ids_); }
  std::vector<int64_t> remove_view_ids() const { return Get(remove_view_ids_); }
  std::vector<intptr_t> deferred_library_ids() const {
    return Get(deferred_library_ids_);
  }
  std::vector<intptr_t> deferred_error_ids() const {
    return Get(deferred_error_ids_);
  }
  int asset_resolver_updates() const { return Get(asset_resolver_updates_); }

 private:
  template <typename T>
  T Get(const T& value) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return value;
  }

  mutable std::mutex mutex_;
  int created_count_ = 0;
  int destroyed_count_ = 0;
  int schedule_frame_count_ = 0;
  int next_frame_callback_count_ = 0;
  int register_texture_count_ = 0;
  std::vector<std::pair<int64_t, ViewportMetrics>> metrics_events_;
  std::vector<std::unique_ptr<PlatformMessage>> messages_;
  std::vector<PointerData> pointer_data_;
  std::vector<bool> semantics_enabled_;
  std::vector<int32_t> feature_flags_;
  std::vector<int64_t> unregistered_textures_;
  std::vector<int64_t> marked_textures_;
  std::vector<int64_t> add_view_ids_;
  std::vector<int64_t> remove_view_ids_;
  std::vector<intptr_t> deferred_library_ids_;
  std::vector<intptr_t> deferred_error_ids_;
  int asset_resolver_updates_ = 0;
};

class PvOhosDispatcherDelegate : public PointerDataDispatcher::Delegate {
 public:
  void DoDispatchPacket(std::unique_ptr<PointerDataPacket> packet,
                        uint64_t trace_flow_id) override {
    if (packet) {
      dispatched_++;
    }
  }
  void ScheduleSecondaryVsyncCallback(uintptr_t id,
                                      const fml::closure& callback) override {
    secondary_++;
  }

  int dispatch_count() const { return dispatched_; }
  int secondary_callback_count() const { return secondary_; }

 private:
  std::atomic<int> dispatched_{0};
  std::atomic<int> secondary_{0};
};

class PvOhosMockResponse : public PlatformMessageResponse {
 public:
  static fml::RefPtr<PvOhosMockResponse> Create() {
    return fml::AdoptRef(new PvOhosMockResponse());
  }

  void Complete(std::unique_ptr<fml::Mapping> data) override {
    complete_called_ = true;
  }
  void CompleteEmpty() override { complete_empty_called_ = true; }

  bool is_complete_called() const { return complete_called_; }
  bool is_complete_empty_called() const { return complete_empty_called_; }

 private:
  PvOhosMockResponse() = default;
  ~PvOhosMockResponse() override = default;

  bool complete_called_ = false;
  bool complete_empty_called_ = false;
};

Settings MakeWbSettings() {
  Settings settings;
  settings.ohos_rendering_api = OHOSRenderingAPI::kSoftware;
  return settings;
}

class WbNullDelegate : public PlatformView::Delegate {
 public:
  void OnPlatformViewCreated(std::unique_ptr<Surface> surface) override {}
  void OnPlatformViewDestroyed() override {}
  void OnPlatformViewScheduleFrame() override {}
  void OnPlatformViewAddView(int64_t view_id,
                             const ViewportMetrics& viewport_metrics,
                             AddViewCallback callback) override {}
  void OnPlatformViewRemoveView(int64_t view_id,
                                RemoveViewCallback callback) override {}
  void OnPlatformViewSendViewFocusEvent(const ViewFocusEvent& event) override {}
  void OnPlatformViewSetNextFrameCallback(const fml::closure& closure) override {}
  void OnPlatformViewSetViewportMetrics(int64_t view_id,
                                        const ViewportMetrics& metrics) override {}
  void OnPlatformViewDispatchPlatformMessage(
      std::unique_ptr<PlatformMessage> message) override {}
  void OnPlatformViewDispatchPointerDataPacket(
      std::unique_ptr<PointerDataPacket> packet) override {}
  void OnPlatformViewDispatchSemanticsAction(int64_t view_id,
                                             int32_t node_id,
                                             SemanticsAction action,
                                             fml::MallocMapping args) override {}
  void OnPlatformViewSetSemanticsEnabled(bool enabled) override {}
  void OnPlatformViewSetAccessibilityFeatures(int32_t flags) override {}
  void OnPlatformViewRegisterTexture(std::shared_ptr<Texture> texture) override {}
  void OnPlatformViewUnregisterTexture(int64_t texture_id) override {}
  void OnPlatformViewMarkTextureFrameAvailable(int64_t texture_id) override {}
  void LoadDartDeferredLibrary(intptr_t loading_unit_id,
                               std::unique_ptr<const fml::Mapping> snapshot_data,
                               std::unique_ptr<const fml::Mapping> snapshot_instructions) override {}
  void LoadDartDeferredLibraryError(intptr_t loading_unit_id,
                                    const std::string error_message,
                                    bool transient) override {}
  void UpdateAssetResolverByType(std::unique_ptr<AssetResolver> updated_asset_resolver,
                                 AssetResolver::AssetResolverType type) override {}
  const Settings& OnPlatformViewGetSettings() const override { return settings_; }

  Settings settings_;
};

class WbRecordingDelegate : public WbNullDelegate {
 public:
  void OnPlatformViewCreated(std::unique_ptr<Surface>) override { Bump(created_); }
  void OnPlatformViewDestroyed() override { Bump(destroyed_); }
  void OnPlatformViewScheduleFrame() override { Bump(schedule_frame_); }
  void OnPlatformViewRegisterTexture(std::shared_ptr<Texture>) override {
    Bump(register_texture_);
  }
  void OnPlatformViewUnregisterTexture(int64_t id) override {
    std::lock_guard<std::mutex> lock(mutex_);
    unregistered_.push_back(id);
  }
  void OnPlatformViewMarkTextureFrameAvailable(int64_t id) override {
    std::lock_guard<std::mutex> lock(mutex_);
    marked_.push_back(id);
  }
  void OnPlatformViewDispatchPlatformMessage(
      std::unique_ptr<PlatformMessage> message) override {
    std::lock_guard<std::mutex> lock(mutex_);
    if (message) {
      messages_.push_back(message->channel());
    }
  }

  int created_count() const { return Get(created_); }
  int destroyed_count() const { return Get(destroyed_); }
  int schedule_frame_count() const { return Get(schedule_frame_); }
  int register_texture_count() const { return Get(register_texture_); }
  std::vector<int64_t> unregistered_ids() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return unregistered_;
  }
  std::vector<int64_t> marked_texture_ids() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return marked_;
  }
  int message_count(const std::string& channel) const {
    std::lock_guard<std::mutex> lock(mutex_);
    int count = 0;
    for (const auto& c : messages_) {
      if (c == channel) {
        count++;
      }
    }
    return count;
  }

 private:
  void Bump(int& counter) {
    std::lock_guard<std::mutex> lock(mutex_);
    counter++;
  }
  int Get(const int& counter) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return counter;
  }

  mutable std::mutex mutex_;
  int created_ = 0;
  int destroyed_ = 0;
  int schedule_frame_ = 0;
  int register_texture_ = 0;
  std::vector<int64_t> unregistered_;
  std::vector<int64_t> marked_;
  std::vector<std::string> messages_;
};

class WbFakeTexture : public OHOSExternalTexture {
 public:
  explicit WbFakeTexture(int64_t id)
      : OHOSExternalTexture(id, OH_OnFrameAvailableListener{nullptr, nullptr}) {}

  void MarkNewFrameAvailable() override { ++mark_new_frame_calls_; }
  void Paint(PaintContext& context,
             const DlRect& bounds,
             bool freeze,
             DlImageSampling sampling) override {}
  void OnGrContextCreated() override {}
  void OnGrContextDestroyed() override {}
  void OnTextureUnregistered() override {}
  void SetGPUFence(OHNativeWindowBuffer* window_buffer, int* fence_fd) override {}
  void GPUResourceDestroy() override {}
  sk_sp<flutter::DlImage> CreateDlImage(PaintContext& context,
                                        const SkRect& bounds,
                                        NativeBufferKey key,
                                        OH_NativeBuffer_Config& config,
                                        OHNativeWindowBuffer* nw_buffer) override {
    return nullptr;
  }
  void DeleteBufferGPUResource(NativeBufferKey key) override {}

  int mark_new_frame_calls() const { return mark_new_frame_calls_.load(); }

 private:
  std::atomic<int> mark_new_frame_calls_{0};
};

OHNativeWindow* const kWbHandleA = reinterpret_cast<OHNativeWindow*>(0x6000);

#if defined(OHOS_X64_UNITTEST)

fml::RefPtr<OHOSNativeWindow> WbMakeWindow(OHNativeWindow* handle) {
  return fml::MakeRefCounted<OHOSNativeWindow>(handle, false);
}

#else

std::vector<OH_NativeImage*>& WbRegistryImages() {
  static std::vector<OH_NativeImage*> images;
  return images;
}

fml::RefPtr<OHOSNativeWindow> WbMakeWindow(OHNativeWindow* request) {
  if (request == nullptr) {
    return fml::MakeRefCounted<OHOSNativeWindow>(nullptr, false);
  }
  OH_NativeImage* image = OH_NativeImage_Create(0, 0);
  OHNativeWindow* window =
      image != nullptr ? OH_NativeImage_AcquireNativeWindow(image) : nullptr;
  if (image != nullptr) {
    WbRegistryImages().push_back(image);
  }
  return fml::MakeRefCounted<OHOSNativeWindow>(window, false);
}

#endif  // defined(OHOS_X64_UNITTEST)

class WbMapEntry {
 public:
  WbMapEntry(uint64_t key, PlatformViewOHOS* view) : key_(key) {
    std::lock_guard<std::recursive_mutex> lock(g_map_mutex);
    g_texture_platformview_map[key_] = view;
  }
  ~WbMapEntry() { Erase(); }
  void Erase() {
    std::lock_guard<std::recursive_mutex> lock(g_map_mutex);
    g_texture_platformview_map.erase(key_);
  }
  bool Present(PlatformViewOHOS* view) const {
    std::lock_guard<std::recursive_mutex> lock(g_map_mutex);
    auto it = g_texture_platformview_map.find(key_);
    return it != g_texture_platformview_map.end() && it->second == view;
  }

 private:
  uint64_t key_;
};

class PlatformViewOHOSWbTest : public ::testing::Test {
 protected:
  explicit PlatformViewOHOSWbTest(bool with_context = true)
      : with_context_(with_context) {}
  ~PlatformViewOHOSWbTest() override {
    FlushTasks();
    if (view_) {
      std::lock_guard<std::recursive_mutex> lock(g_map_mutex);
      for (auto it = g_texture_platformview_map.begin();
           it != g_texture_platformview_map.end();) {
        if (it->second == view_.get()) {
          it = g_texture_platformview_map.erase(it);
        } else {
          ++it;
        }
      }
    }
  }

  void SetUp() override {
    delegate_.settings_ = MakeWbSettings();
    napi_facade_ = std::make_shared<PlatformViewOHOSNapi>(nullptr);
    runners_ = std::make_unique<TaskRunners>(
        "wb_ut", platform_thread_.GetTaskRunner(), raster_thread_.GetTaskRunner(),
        ui_thread_.GetTaskRunner(), io_thread_.GetTaskRunner());
    std::shared_ptr<OHOSContext> context =
        with_context_
            ? std::make_shared<OHOSContext>(OHOSRenderingAPI::kSoftware)
            : std::shared_ptr<OHOSContext>();
    view_ = std::make_unique<PlatformViewOHOS>(delegate_, *runners_, napi_facade_,
                                               context);
    bridge_ = std::make_shared<SemanticsBridge>();
    bridge_mutex_ = std::make_shared<std::mutex>();
    view_->SetSemanticsBridge(bridge_, bridge_mutex_);
  }

  PlatformViewOHOS* view() { return view_.get(); }
  WbRecordingDelegate& delegate() { return delegate_; }
  TaskRunners& runners() { return *runners_; }

  void FlushTasks() {
    if (!runners_) {
      return;
    }
    fml::AutoResetWaitableEvent raster_done, platform_once, platform_twice;
    runners_->GetRasterTaskRunner()->PostTask([&] { raster_done.Signal(); });
    runners_->GetPlatformTaskRunner()->PostTask([&] { platform_once.Signal(); });
    raster_done.Wait();
    platform_once.Wait();
    runners_->GetPlatformTaskRunner()->PostTask([&] { platform_twice.Signal(); });
    platform_twice.Wait();
  }

  bool WaitForPlatformIdleAfter(fml::TimeDelta delay) {
    fml::AutoResetWaitableEvent done;
    runners_->GetPlatformTaskRunner()->PostDelayedTask([&] { done.Signal(); },
                                                       delay);
    return !done.WaitWithTimeout(delay + fml::TimeDelta::FromSeconds(5));
  }

  void SendLifecycle(const std::string& state) {
    std::string channel = "flutter/lifecycle";
    view()->DispatchPlatformMessage(channel, const_cast<char*>(state.c_str()),
                                    static_cast<int>(state.size()), 0);
  }

  void CallFrameAvailable(uint64_t key) {
    PlatformViewOHOS::OnNativeImageFrameAvailable(reinterpret_cast<void*>(key));
  }

  bool with_context_;
  WbRecordingDelegate delegate_;
  std::shared_ptr<SemanticsBridge> bridge_;
  std::shared_ptr<std::mutex> bridge_mutex_;
  std::shared_ptr<PlatformViewOHOSNapi> napi_facade_;
  fml::Thread platform_thread_{"wb_ut_platform"};
  fml::Thread raster_thread_{"wb_ut_raster"};
  fml::Thread ui_thread_{"wb_ut_ui"};
  fml::Thread io_thread_{"wb_ut_io"};
  std::unique_ptr<TaskRunners> runners_;
  std::unique_ptr<PlatformViewOHOS> view_;
};

}

class PlatformViewOHOSUt : public ::testing::Test {
 protected:
  explicit PlatformViewOHOSUt(bool with_context = true)
      : with_context_(with_context) {}
  ~PlatformViewOHOSUt() override { FlushTasks(); }

  void SetUp() override {
    delegate_.settings_ = MakeTestSettings();
    napi_facade_ = std::make_shared<PlatformViewOHOSNapi>(nullptr);
    runners_ = std::make_unique<TaskRunners>(
        "pv_ohos_ut", platform_thread_.GetTaskRunner(),
        raster_thread_.GetTaskRunner(), ui_thread_.GetTaskRunner(),
        io_thread_.GetTaskRunner());
    std::shared_ptr<OHOSContext> context =
        with_context_
            ? std::make_shared<OHOSContext>(OHOSRenderingAPI::kSoftware)
            : std::shared_ptr<OHOSContext>();
    view_ = std::make_unique<PlatformViewOHOS>(delegate_, *runners_,
                                               napi_facade_, context);
    bridge_ = std::make_shared<SemanticsBridge>();
    bridge_mutex_ = std::make_shared<std::mutex>();
    view_->SetSemanticsBridge(bridge_, bridge_mutex_);
  }

  PlatformViewOHOS* view() { return view_.get(); }
  PlatformView* base() { return view_.get(); }
  PvOhosRecordingDelegate& delegate() { return delegate_; }
  TaskRunners& runners() { return *runners_; }
  std::shared_ptr<SemanticsBridge>& bridge() { return bridge_; }

  void FlushTasks() {
    if (!runners_) {
      return;
    }
    fml::AutoResetWaitableEvent raster_done, platform_once, platform_twice;
    runners_->GetRasterTaskRunner()->PostTask([&] { raster_done.Signal(); });
    runners_->GetPlatformTaskRunner()->PostTask([&] { platform_once.Signal(); });
    raster_done.Wait();
    platform_once.Wait();
    runners_->GetPlatformTaskRunner()->PostTask([&] { platform_twice.Signal(); });
    platform_twice.Wait();
  }

  bool WaitForPlatformIdleAfter(fml::TimeDelta delay) {
    fml::AutoResetWaitableEvent done;
    runners_->GetPlatformTaskRunner()->PostDelayedTask([&] { done.Signal(); },
                                                       delay);
    return !done.WaitWithTimeout(delay + fml::TimeDelta::FromSeconds(5));
  }

  void SendLifecycle(const std::string& state) {
    std::string channel = "flutter/lifecycle";
    view()->DispatchPlatformMessage(channel, const_cast<char*>(state.c_str()),
                                    static_cast<int>(state.size()), 0);
  }

  bool with_context_;

  PvOhosRecordingDelegate delegate_;
  std::shared_ptr<SemanticsBridge> bridge_;
  std::shared_ptr<std::mutex> bridge_mutex_;
  std::shared_ptr<PlatformViewOHOSNapi> napi_facade_;
  fml::Thread platform_thread_{"pv_ut_platform"};
  fml::Thread raster_thread_{"pv_ut_raster"};
  fml::Thread ui_thread_{"pv_ut_ui"};
  fml::Thread io_thread_{"pv_ut_io"};
  std::unique_ptr<TaskRunners> runners_;
  std::unique_ptr<PlatformViewOHOS> view_;
};

// HCPP 默认关闭：开关未打开时 IsHybridCompositionEnabled() 为 false，
// CreateExternalViewEmbedder() 返回非 null 的默认 embedder
// （HCPP embedder 仅在 hybrid_composition_enabled_ 时创建）。
TEST_F(PlatformViewOHOSUt, DisabledByDefault) {
  auto settings = MakeTestSettings();
  EXPECT_FALSE(settings.enable_ohos_hybrid_composition);

  TestViewHandle handle(settings);
  ASSERT_TRUE(handle.IsValid());
  auto platform_view = handle.view();
  ASSERT_NE(platform_view, nullptr);

  EXPECT_FALSE(platform_view->IsHybridCompositionEnabled());
  // CreateExternalViewEmbedder 在 PlatformViewOHOS 中是 private override，
  // 经基类 public 虚接口调用触发虚表分发。
  PlatformView* base_view = platform_view;
  auto embedder = base_view->CreateExternalViewEmbedder();
  ASSERT_NE(embedder, nullptr);
  EXPECT_EQ(embedder->CompositeEmbeddedView(1), nullptr);
}

// HCPP 以 Settings.enable_ohos_hybrid_composition 方式打开，但软件渲染后端
// 被排除：开关不无条件生效（HCPP 依赖 ArkUI 系统合成层）。
TEST_F(PlatformViewOHOSUt, DisabledWithSoftwareRenderingDespiteSettings) {
  auto settings = MakeTestSettings();
  settings.enable_ohos_hybrid_composition = true;

  TestViewHandle handle(settings);
  ASSERT_TRUE(handle.IsValid());
  auto platform_view = handle.view();
  ASSERT_NE(platform_view, nullptr);

  EXPECT_FALSE(platform_view->IsHybridCompositionEnabled());
  PlatformView* base_view = platform_view;
  auto embedder = base_view->CreateExternalViewEmbedder();
  ASSERT_NE(embedder, nullptr);
  EXPECT_EQ(embedder->CompositeEmbeddedView(1), nullptr);
}

// HCPP 关闭时 overlay 窗口的生命周期方法必须安全：
// - SetHybridCompositionOverlayWindow 在 embedder 未创建时走 stash 分支；
// - ClearHybridCompositionOverlayWindowSync 在无 embedder 时直接返回，
//   不会阻塞等待 raster 线程。
TEST_F(PlatformViewOHOSUt, OverlayWindowCallsSafeWhenDisabled) {
  auto settings = MakeTestSettings();

  TestViewHandle handle(settings);
  ASSERT_TRUE(handle.IsValid());
  auto platform_view = handle.view();
  ASSERT_NE(platform_view, nullptr);

  int dummy_window = 0;
  platform_view->SetHybridCompositionOverlayWindow(&dummy_window);
  platform_view->ClearHybridCompositionOverlayWindowSync();
  // 清空路径（nullptr）同样安全。
  platform_view->SetHybridCompositionOverlayWindow(nullptr);
  platform_view->ClearHybridCompositionOverlayWindowSync();
}

class PlatformViewOHOSUtNoCtx : public PlatformViewOHOSUt {
 protected:
  PlatformViewOHOSUtNoCtx() : PlatformViewOHOSUt(false) {}
};

TEST_F(PlatformViewOHOSUt, SurfaceFactoryCreatesSoftwareSurface) {
  auto context = std::make_shared<OHOSContext>(OHOSRenderingAPI::kSoftware);
  OhosSurfaceFactoryImpl factory(context);
  auto surface = factory.CreateSurface();
  ASSERT_NE(surface, nullptr);
  EXPECT_TRUE(surface->IsValid());
  EXPECT_FALSE(surface->ResourceContextMakeCurrent());
  auto gpu_surface = surface->CreateGPUSurface(nullptr);
  ASSERT_NE(gpu_surface, nullptr);
  EXPECT_TRUE(gpu_surface->IsValid());
}

TEST_F(PlatformViewOHOSUt, CreateOHOSContextSoftwareIsValid) {
  auto context = CreateOHOSContext(runners(), OHOSRenderingAPI::kSoftware, false,
                                   false, false);
  ASSERT_NE(context, nullptr);
  EXPECT_EQ(context->RenderingApi(), OHOSRenderingAPI::kSoftware);
  EXPECT_TRUE(context->IsValid());
}

TEST_F(PlatformViewOHOSUt, CreateOHOSContextGlesReturnsContext) {
  auto context = CreateOHOSContext(runners(), OHOSRenderingAPI::kOpenGLES,
                                   false, false, false);
  ASSERT_NE(context, nullptr);
  EXPECT_EQ(context->RenderingApi(), OHOSRenderingAPI::kOpenGLES);
}

TEST_F(PlatformViewOHOSUt, CreateOHOSContextVulkanReturnsContext) {
  auto context = CreateOHOSContext(runners(), OHOSRenderingAPI::kImpellerVulkan,
                                   false, false, false);
  ASSERT_NE(context, nullptr);
  EXPECT_EQ(context->RenderingApi(), OHOSRenderingAPI::kImpellerVulkan);
}

TEST_F(PlatformViewOHOSUtNoCtx, SurfacelessViewEarlyReturns) {
  EXPECT_FALSE(view()->NotifyCreateForView(2, MakePvUtWindow(kPvUtHandleA), 10,
                                           10));
  view()->NotifyCreated();
  FlushTasks();
  EXPECT_EQ(delegate().created_count(), 0);
  EXPECT_EQ(base()->CreateResourceContext(), nullptr);
  EXPECT_EQ(base()->GetImpellerContext(), nullptr);
}

TEST_F(PlatformViewOHOSUtNoCtx, SurfacelessNotifyCreateAndDestroy) {
  view()->NotifyCreate(MakePvUtWindow(kPvUtHandleA));
  view()->NotifySurfaceWindowChanged(fml::RefPtr<OHOSNativeWindow>());
  view()->NotifyChanged(DlISize{10, 10});
  view()->Preload(10, 10);
  view()->NotifyDestroyed();
  FlushTasks();
  EXPECT_EQ(delegate().created_count(), 0);
  EXPECT_EQ(delegate().next_frame_callback_count(), 0);
  EXPECT_EQ(delegate().destroyed_count(), 1);
  ASSERT_FALSE(delegate().semantics_enabled().empty());
  EXPECT_FALSE(delegate().semantics_enabled().back());
}

TEST_F(PlatformViewOHOSUt, SetViewportMetricsUsesDisplaySizeWhenSet) {
  ViewportMetrics metrics;
  metrics.physical_width = 100;
  metrics.physical_height = 200;
  view()->SetViewportMetrics(kFlutterImplicitViewId, metrics);
  ASSERT_EQ(delegate().metrics_count(), 1u);
  auto first = delegate().metrics_for(kFlutterImplicitViewId);
  EXPECT_EQ(first[0].physical_width, 100);
  EXPECT_EQ(first[0].physical_height, 200);

  view()->UpdateDisplaySize(640, 480);
  ASSERT_EQ(delegate().metrics_count(), 2u);
  auto second = delegate().metrics_for(kFlutterImplicitViewId);
  EXPECT_EQ(second[1].physical_width, 640);
  EXPECT_EQ(second[1].physical_height, 480);
  EXPECT_EQ(second[1].physical_min_width_constraint, 640);
  EXPECT_EQ(second[1].physical_max_height_constraint, 480);

  view()->SetViewportMetrics(kFlutterImplicitViewId, metrics);
  ASSERT_EQ(delegate().metrics_count(), 3u);
  auto third = delegate().metrics_for(kFlutterImplicitViewId);
  EXPECT_EQ(third[2].physical_width, 640);

  view()->UpdateDisplaySize(640, 480);
  EXPECT_EQ(delegate().metrics_count(), 3u);
}

TEST_F(PlatformViewOHOSUt, UpdateSemanticsQueuedUntilNotifyCreateDrains) {
  SemanticsNodeUpdates nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1};
  nodes[0] = root;
  SemanticsNode child;
  child.id = 1;
  child.label = "queued";
  nodes[1] = child;
  base()->UpdateSemantics(kFlutterImplicitViewId, nodes, {});
  base()->UpdateSemantics(kFlutterImplicitViewId, nodes, {});
  EXPECT_EQ(bridge()->tree_.FindNodeById(1), nullptr);

  view()->NotifyCreate(MakePvUtWindow(kPvUtHandleA));
  EXPECT_NE(bridge()->tree_.FindNodeById(1), nullptr);
  FlushTasks();
  EXPECT_EQ(delegate().created_count(), 0);
  EXPECT_EQ(delegate().next_frame_callback_count(), 1);
}

TEST_F(PlatformViewOHOSUt, NotifyCreateNotifiesCreatedWithEmbedder) {
  ASSERT_NE(base()->CreateExternalViewEmbedder(), nullptr);
  view()->NotifyCreate(MakePvUtWindow(kPvUtHandleB));
  FlushTasks();
  EXPECT_EQ(delegate().created_count(), 1);
  EXPECT_EQ(delegate().next_frame_callback_count(), 1);
}

TEST_F(PlatformViewOHOSUt, NotifyCreateSetDisplayWindowFailure) {
  view()->NotifyCreate(MakePvUtWindow(nullptr));
  FlushTasks();
  EXPECT_EQ(delegate().created_count(), 0);
  EXPECT_EQ(delegate().next_frame_callback_count(), 1);
}

TEST_F(PlatformViewOHOSUt, PreloadOffscreenThenSecondPreloadSkips) {
  ASSERT_NE(base()->CreateExternalViewEmbedder(), nullptr);
  view()->Preload(640, 480);
  FlushTasks();
  EXPECT_EQ(delegate().created_count(), 1);
  EXPECT_EQ(delegate().next_frame_callback_count(), 1);
  view()->Preload(640, 480);
  FlushTasks();
  EXPECT_EQ(delegate().created_count(), 1);
  EXPECT_EQ(delegate().next_frame_callback_count(), 1);
}

TEST_F(PlatformViewOHOSUt, PreloadRetriesWhenOffscreenPrepareFails) {
  ASSERT_NE(base()->CreateExternalViewEmbedder(), nullptr);
  PvUtKnobGuard guard;
  if (!PvUtInjectOffscreenAcquireFailure()) {
    GTEST_SKIP() << "offscreen acquire-failure injection needs x64 stubs";
  }
  view()->Preload(100, 100);
  FlushTasks();
  EXPECT_EQ(delegate().created_count(), 0);
  EXPECT_EQ(delegate().next_frame_callback_count(), 1);

  PvUtClearOffscreenAcquireFailure();
  view()->Preload(100, 100);
  FlushTasks();
  EXPECT_EQ(delegate().created_count(), 1);
  EXPECT_EQ(delegate().next_frame_callback_count(), 2);
}

TEST_F(PlatformViewOHOSUt, NotifyCreateForViewRegistersSurfaceAndMetrics) {
  ASSERT_NE(base()->CreateExternalViewEmbedder(), nullptr);
  EXPECT_TRUE(
      view()->NotifyCreateForView(7, MakePvUtWindow(kPvUtHandleA), 300, 400));
  auto metrics = delegate().metrics_for(7);
  ASSERT_EQ(metrics.size(), 1u);
  EXPECT_EQ(metrics[0].physical_width, 300);
  EXPECT_EQ(metrics[0].physical_height, 400);

  view()->NotifySurfaceChangedForView(7, MakePvUtWindow(kPvUtHandleB), 500,
                                      600);
  metrics = delegate().metrics_for(7);
  ASSERT_EQ(metrics.size(), 2u);
  EXPECT_EQ(metrics[1].physical_width, 500);
  EXPECT_EQ(metrics[1].physical_height, 600);

  view()->NotifyDestroyForView(7);
  FlushTasks();
  view()->NotifySurfaceChangedForView(7, MakePvUtWindow(kPvUtHandleB), 800,
                                      900);
  EXPECT_EQ(delegate().metrics_for(7).size(), 2u);
}

TEST_F(PlatformViewOHOSUt, NotifyCreateForViewFailsWithoutEmbedder) {
  EXPECT_FALSE(
      view()->NotifyCreateForView(5, MakePvUtWindow(kPvUtHandleA), 100, 100));
  EXPECT_TRUE(delegate().metrics_for(5).empty());
}

TEST_F(PlatformViewOHOSUt, NotifyCreateForViewFailsWithNullWindow) {
  ASSERT_NE(base()->CreateExternalViewEmbedder(), nullptr);
  EXPECT_FALSE(view()->NotifyCreateForView(
      5, fml::RefPtr<OHOSNativeWindow>(), 100, 100));
  EXPECT_TRUE(delegate().metrics_for(5).empty());
}

TEST_F(PlatformViewOHOSUt, NotifyCreateForViewZeroSizeSkipsMetrics) {
  ASSERT_NE(base()->CreateExternalViewEmbedder(), nullptr);
  EXPECT_TRUE(view()->NotifyCreateForView(9, MakePvUtWindow(kPvUtHandleA), 0,
                                          0));
  EXPECT_TRUE(delegate().metrics_for(9).empty());
}

TEST_F(PlatformViewOHOSUt, NotifyDestroyForViewUnknownIsNoop) {
  view()->NotifyDestroyForView(12345);
  FlushTasks();
  EXPECT_EQ(delegate().metrics_count(), 0u);
  EXPECT_EQ(delegate().schedule_frame_count(), 0);
}

TEST_F(PlatformViewOHOSUt, AddRemoveViewForWindowRoundTrip) {
  view()->AddViewForWindow(3);
  FlushTasks();
  auto added = delegate().add_view_ids();
  ASSERT_EQ(added.size(), 1u);
  EXPECT_EQ(added[0], 3);
  view()->RemoveViewForWindow(3);
  FlushTasks();
  auto removed = delegate().remove_view_ids();
  ASSERT_EQ(removed.size(), 1u);
  EXPECT_EQ(removed[0], 3);
}

TEST_F(PlatformViewOHOSUt, DispatchPlatformMessageForwardsNonLifecycle) {
  std::string data = "hello";
  view()->DispatchPlatformMessage("some/channel", &data[0],
                                  static_cast<int>(data.size()), 11);
  FlushTasks();
  EXPECT_EQ(delegate().message_count("some/channel"), 1);
  EXPECT_EQ(delegate().message_count("flutter/system"), 0);
  EXPECT_EQ(delegate().schedule_frame_count(), 0);

  view()->DispatchEmptyPlatformMessage("empty/channel", 12);
  FlushTasks();
  EXPECT_EQ(delegate().message_count("empty/channel"), 1);
}

TEST_F(PlatformViewOHOSUt, LifecycleGuardsSkipReclaim) {
  SendLifecycle("");
  SendLifecycle("AppLifecycleState.bogus");
  FlushTasks();
  EXPECT_EQ(delegate().message_count("flutter/lifecycle"), 2);
  EXPECT_EQ(delegate().message_count("flutter/system"), 0);
  EXPECT_FALSE(view()->IsFrameGateEnabled());
  EXPECT_EQ(delegate().schedule_frame_count(), 0);
}

TEST_F(PlatformViewOHOSUt, LifecyclePausedAggressiveResumedRestore) {
  SendLifecycle("AppLifecycleState.paused");
  FlushTasks();
  EXPECT_TRUE(view()->IsFrameGateEnabled());
  EXPECT_EQ(delegate().message_count("flutter/system"), 1);
  EXPECT_GE(delegate().schedule_frame_count(), 1);

  SendLifecycle("AppLifecycleState.resumed");
  FlushTasks();
  EXPECT_FALSE(view()->IsFrameGateEnabled());
}

TEST_F(PlatformViewOHOSUt, LifecycleHiddenDetachedAggressiveInactiveRestore) {
  SendLifecycle("AppLifecycleState.hidden");
  FlushTasks();
  EXPECT_TRUE(view()->IsFrameGateEnabled());
  SendLifecycle("AppLifecycleState.resumed");
  FlushTasks();
  EXPECT_FALSE(view()->IsFrameGateEnabled());
  SendLifecycle("AppLifecycleState.detached");
  FlushTasks();
  EXPECT_TRUE(view()->IsFrameGateEnabled());
  SendLifecycle("AppLifecycleState.inactive");
  FlushTasks();
  EXPECT_FALSE(view()->IsFrameGateEnabled());
}

TEST_F(PlatformViewOHOSUt, SetPipVisibleKeepsRestoreAndDeduplicates) {
  view()->SetPipVisible(false);
  FlushTasks();
  EXPECT_EQ(delegate().schedule_frame_count(), 0);
  EXPECT_FALSE(view()->IsFrameGateEnabled());

  view()->SetPipVisible(true);
  FlushTasks();
  EXPECT_FALSE(view()->IsFrameGateEnabled());
  SendLifecycle("AppLifecycleState.paused");
  FlushTasks();
  EXPECT_FALSE(view()->IsFrameGateEnabled());

  view()->SetPipVisible(false);
  FlushTasks();
  EXPECT_TRUE(view()->IsFrameGateEnabled());
  view()->SetPipVisible(true);
  FlushTasks();
  EXPECT_FALSE(view()->IsFrameGateEnabled());
}

TEST_F(PlatformViewOHOSUt, OnSurfaceCreatedForcesRestoreFromDestroyedLevel) {
  SendLifecycle("AppLifecycleState.paused");
  FlushTasks();
  EXPECT_TRUE(view()->IsFrameGateEnabled());
  int system_msgs = delegate().message_count("flutter/system");

  view()->OnSurfaceDestroyed();
  SendLifecycle("AppLifecycleState.paused");
  FlushTasks();
  EXPECT_EQ(delegate().message_count("flutter/system"), system_msgs);
  EXPECT_TRUE(view()->IsFrameGateEnabled());

  view()->OnSurfaceCreated();
  FlushTasks();
  EXPECT_FALSE(view()->IsFrameGateEnabled());

  SendLifecycle("AppLifecycleState.paused");
  FlushTasks();
  EXPECT_TRUE(view()->IsFrameGateEnabled());
}

TEST_F(PlatformViewOHOSUt, AggressiveReclaimCoreTeardownThenRestoreRebuild) {
  ASSERT_NE(base()->CreateExternalViewEmbedder(), nullptr);
  view()->NotifyCreate(MakePvUtWindow(kPvUtHandleA));
  FlushTasks();
  EXPECT_EQ(delegate().created_count(), 1);

  SendLifecycle("AppLifecycleState.paused");
  FlushTasks();
  EXPECT_TRUE(view()->IsFrameGateEnabled());
  ASSERT_TRUE(WaitForPlatformIdleAfter(fml::TimeDelta::FromMilliseconds(1400)));

  int frames_before = delegate().schedule_frame_count();
  SendLifecycle("AppLifecycleState.resumed");
  FlushTasks();
  EXPECT_FALSE(view()->IsFrameGateEnabled());
  EXPECT_GT(delegate().schedule_frame_count(), frames_before);
}

TEST_F(PlatformViewOHOSUt, AggressiveReclaimSkippedWhenContextInvalid) {
  ASSERT_NE(base()->CreateExternalViewEmbedder(), nullptr);
  view()->NotifyCreate(MakePvUtWindow(kPvUtHandleA));
  FlushTasks();
  view()->NotifyDestroyed();
  FlushTasks();
  EXPECT_EQ(delegate().destroyed_count(), 1);

  SendLifecycle("AppLifecycleState.paused");
  FlushTasks();
  EXPECT_FALSE(view()->IsFrameGateEnabled());
  int frames = delegate().schedule_frame_count();
  SendLifecycle("AppLifecycleState.resumed");
  FlushTasks();
  EXPECT_EQ(delegate().schedule_frame_count(), frames);
}

TEST_F(PlatformViewOHOSUt, AggressiveReclaimRebuildFailsWithInvalidCachedWindow) {
  view()->NotifyCreate(MakePvUtWindow(nullptr));
  FlushTasks();
  EXPECT_EQ(delegate().created_count(), 0);
  SendLifecycle("AppLifecycleState.paused");
  FlushTasks();
  ASSERT_TRUE(WaitForPlatformIdleAfter(fml::TimeDelta::FromMilliseconds(1400)));
  int frames = delegate().schedule_frame_count();
  SendLifecycle("AppLifecycleState.resumed");
  FlushTasks();
  EXPECT_EQ(delegate().schedule_frame_count(), frames);
  EXPECT_FALSE(view()->IsFrameGateEnabled());
}

TEST_F(PlatformViewOHOSUt, ExternalTextureApiWithoutRegisteredTextures) {
  EXPECT_EQ(view()->RegisterExternalTexture(3), 0u);
  EXPECT_EQ(view()->CreateExternalTexture(3), nullptr);
  EXPECT_EQ(view()->GetExternalTextureWindowId(3), 0u);
  EXPECT_FALSE(view()->SetExternalNativeImage(3, nullptr));
  EXPECT_EQ(view()->ResetExternalTexture(3, true), 0u);
  view()->RegisterExternalTextureByPixelMap(3, nullptr, nullptr);
  view()->SetExternalTextureBackGroundPixelMap(3, nullptr, nullptr);
  view()->SetExternalTextureBackGroundColor(3, 0xff0000ffu);
  view()->SetTextureBufferSize(3, 64, 64);
  view()->NotifyTextureResizing(3, 32, 32);
  view()->UnRegisterExternalTexture(3);
  FlushTasks();
  auto unregistered = delegate().unregistered_textures();
  ASSERT_EQ(unregistered.size(), 1u);
  EXPECT_EQ(unregistered[0], 3);
  EXPECT_EQ(delegate().schedule_frame_count(), 0);
  EXPECT_EQ(delegate().register_texture_count(), 0);
}

TEST_F(PlatformViewOHOSUt, CreateRenderingSurfaceRequiresWindowingEmbedder) {
  view()->NotifyCreated();
  FlushTasks();
  EXPECT_EQ(delegate().created_count(), 0);
  ASSERT_NE(base()->CreateExternalViewEmbedder(), nullptr);
  view()->NotifyCreated();
  FlushTasks();
  EXPECT_EQ(delegate().created_count(), 1);
}

TEST_F(PlatformViewOHOSUt, SnapshotProducerAndSoftwareContexts) {
  auto producer = base()->CreateSnapshotSurfaceProducer();
  ASSERT_NE(producer, nullptr);
  EXPECT_EQ(producer->CreateSnapshotSurface(), nullptr);
  EXPECT_EQ(base()->CreateResourceContext(), nullptr);
  EXPECT_EQ(base()->GetImpellerContext(), nullptr);
  EXPECT_NO_FATAL_FAILURE(base()->ReleaseResourceContext());
}

TEST_F(PlatformViewOHOSUt, DispatcherMakerProducesSmoothDispatcher) {
  auto vsync_waiter = base()->CreateVSyncWaiter();
  EXPECT_NE(vsync_waiter, nullptr);
  PvOhosDispatcherDelegate dispatcher_delegate;
  auto maker = view()->GetDispatcherMaker();
  ASSERT_TRUE(maker);
  auto dispatcher = maker(dispatcher_delegate);
  ASSERT_NE(dispatcher, nullptr);
  auto packet = std::make_unique<PointerDataPacket>(1);
  dispatcher->DispatchPacket(std::move(packet), 42);
  EXPECT_EQ(dispatcher_delegate.dispatch_count(), 1);
  EXPECT_GE(dispatcher_delegate.secondary_callback_count(), 1);
}

TEST_F(PlatformViewOHOSUt, ComputeResolvedLocalesDefaultAndPassthrough) {
  auto empty = base()->ComputePlatformResolvedLocales({});
  ASSERT_NE(empty, nullptr);
  ASSERT_EQ(empty->size(), 3u);
  EXPECT_EQ((*empty)[0], "zh");
  auto en = base()->ComputePlatformResolvedLocales({"en", "US", "Latn"});
  ASSERT_NE(en, nullptr);
  ASSERT_EQ(en->size(), 3u);
  EXPECT_EQ((*en)[0], "en");
  EXPECT_NO_FATAL_FAILURE(base()->SetApplicationLocale("en-US"));
}

TEST_F(PlatformViewOHOSUt, DeferredLibraryAndAssetResolverForwarded) {
  view()->LoadDartDeferredLibrary(4, nullptr, nullptr);
  auto library_ids = delegate().deferred_library_ids();
  ASSERT_EQ(library_ids.size(), 1u);
  EXPECT_EQ(library_ids[0], 4);
  view()->LoadDartDeferredLibraryError(5, "boom", true);
  auto error_ids = delegate().deferred_error_ids();
  ASSERT_EQ(error_ids.size(), 1u);
  EXPECT_EQ(error_ids[0], 5);
  view()->UpdateAssetResolverByType(nullptr,
                                    AssetResolver::kDirectoryAssetBundle);
  EXPECT_EQ(delegate().asset_resolver_updates(), 1);
  EXPECT_NO_FATAL_FAILURE(base()->RequestDartDeferredLibrary(6));
}

TEST_F(PlatformViewOHOSUt, HandlePlatformMessageRegistersPendingResponse) {
  auto response = PvOhosMockResponse::Create();
  auto message = std::make_unique<PlatformMessage>("test_channel", response);
  base()->HandlePlatformMessage(std::move(message));
  FlushTasks();
  auto handler = std::static_pointer_cast<PlatformMessageHandlerOHOS>(
      view()->GetPlatformMessageHandler());
  handler->InvokePlatformMessageEmptyResponseCallback(1);
  EXPECT_TRUE(response->is_complete_empty_called());
}

TEST_F(PlatformViewOHOSUt, AccessibilityLifecycleAndSemanticsTree) {
  bridge()->is_accessibility_enabled_ = false;
#if defined(OHOS_X64_UNITTEST)
  static char provider_storage;
  bridge()->provider_ohos_ =
      reinterpret_cast<ArkUI_AccessibilityProvider*>(&provider_storage);
#endif

  SemanticsNodeUpdates nodes;
  SemanticsNode child;
  child.id = 1;
  child.label = "node1";
  nodes[1] = child;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1};
  nodes[0] = root;
  base()->UpdateSemantics(kFlutterImplicitViewId, nodes, {});
#if defined(OHOS_X64_UNITTEST)
  auto* node1 = bridge()->tree_.FindNodeById(1);
  ASSERT_NE(node1, nullptr);
  auto* root_node = bridge()->tree_.FindNodeById(0);
  ASSERT_NE(root_node, nullptr);
  EXPECT_TRUE(node1->hasUpdate);
#else
  EXPECT_EQ(bridge()->tree_.FindNodeById(1), nullptr);
  bridge()->tree_.UpdateWithNodes(nodes);
  auto* node1 = bridge()->tree_.FindNodeById(1);
  ASSERT_NE(node1, nullptr);
  auto* root_node = bridge()->tree_.FindNodeById(0);
  ASSERT_NE(root_node, nullptr);
  EXPECT_TRUE(node1->hasUpdate);
#endif

  auto message = std::make_unique<char[]>(16);
  std::strncpy(message.get(), "welcome", 15);
  view()->AccessibilityOnTap(1);
  view()->AccessibilityAnnounce(message);
  view()->AccessibilityOnLongPress(1);
  view()->AccessibilityOnTooltip(message);
  EXPECT_TRUE(node1->hasUpdate);

  view()->OnAccessibilityStateChange(true);
  EXPECT_TRUE(bridge()->is_accessibility_enabled_);
  ASSERT_FALSE(delegate().feature_flags().empty());
  EXPECT_EQ(delegate().feature_flags().back(), 1);
  ASSERT_FALSE(delegate().semantics_enabled().empty());
  EXPECT_TRUE(delegate().semantics_enabled().back());

  view()->AccessibilityOnTap(1);
  EXPECT_FALSE(node1->hasUpdate);
  view()->AccessibilityOnLongPress(1);
  view()->AccessibilityOnTooltip(message);
  EXPECT_FALSE(root_node->hasUpdate);
  view()->AccessibilityAnnounce(message);

  view()->SetBoldText(1.5);
  EXPECT_EQ(delegate().feature_flags().back(), 1 | 8);
  view()->SetBoldText(1.0);
  EXPECT_EQ(delegate().feature_flags().back(), 1);

  view()->SetNavigation(true);
  EXPECT_TRUE(bridge()->has_navigationed_);

  view()->OnAccessibilityStateChange(false);
  EXPECT_TRUE(bridge()->is_accessibility_enabled_);
  bridge()->OnAccessibilityStateChange(false);
  EXPECT_FALSE(bridge()->is_accessibility_enabled_);
  EXPECT_EQ(delegate().feature_flags().back(), 0);
  EXPECT_FALSE(delegate().semantics_enabled().back());
  size_t flag_events = delegate().feature_flags().size();
  view()->OnAccessibilityStateChange(false);
  EXPECT_EQ(delegate().feature_flags().size(), flag_events);

  base()->SetSemanticsTreeEnabled(false);
  EXPECT_EQ(bridge()->tree_.FindNodeById(1), nullptr);
}

TEST_F(PlatformViewOHOSUt, SimulateTouchEventDispatchesDownThenUp) {
  SemanticsNodeExtend node;
  node.id = 1;
  node.absoluteRect = SkRect::MakeLTRB(100, 200, 300, 400);
  view()->SimulateTouchEvent(&node);
  FlushTasks();
  EXPECT_EQ(delegate().pointer_packet_count(), 2u);
  auto down = delegate().pointer_data(0);
  auto up = delegate().pointer_data(1);
  ASSERT_TRUE(down.has_value());
  ASSERT_TRUE(up.has_value());
  EXPECT_EQ(down->change, PointerData::Change::kDown);
  EXPECT_EQ(up->change, PointerData::Change::kUp);
  EXPECT_EQ(down->view_id, kFlutterImplicitViewId);
  EXPECT_DOUBLE_EQ(down->physical_x, 200);
  EXPECT_DOUBLE_EQ(down->physical_y, 300);
  EXPECT_FLOAT_EQ(static_cast<float>(down->pressure), 0.05f);
}

TEST_F(PlatformViewOHOSUt, RunTaskDispatchesByThreadTypeAndDelay) {
  std::atomic<int> ran{0};
  std::atomic<bool> on_platform{false};
  fml::AutoResetWaitableEvent done;
  view()->RunTask(OhosThreadType::kPlatform, [&] {
    on_platform = runners().GetPlatformTaskRunner()->RunsTasksOnCurrentThread();
    ran++;
    done.Signal();
  });
  done.Wait();
  EXPECT_EQ(ran.load(), 1);
  EXPECT_TRUE(on_platform.load());

  fml::AutoResetWaitableEvent ui_done, raster_done, io_done;
  view()->RunTask(OhosThreadType::kUI, [&] { ui_done.Signal(); });
  view()->RunTask(OhosThreadType::kRaster, [&] { raster_done.Signal(); });
  view()->RunTask(OhosThreadType::kIO, [&] { io_done.Signal(); });
  EXPECT_FALSE(ui_done.WaitWithTimeout(fml::TimeDelta::FromSeconds(2)));
  EXPECT_FALSE(raster_done.WaitWithTimeout(fml::TimeDelta::FromSeconds(2)));
  EXPECT_FALSE(io_done.WaitWithTimeout(fml::TimeDelta::FromSeconds(2)));

  view()->RunTask(static_cast<OhosThreadType>(99), [&] { ran++; });
  FlushTasks();
  EXPECT_EQ(ran.load(), 1);

  fml::AutoResetWaitableEvent delayed_done;
  view()->RunTask(OhosThreadType::kPlatform, [&] { delayed_done.Signal(); },
                  30);
  EXPECT_FALSE(delayed_done.WaitWithTimeout(fml::TimeDelta::FromSeconds(3)));
  EXPECT_EQ(ran.load(), 1);
}

TEST_F(PlatformViewOHOSUt, WindowChangeAndResizePathsScheduleNothing) {
  view()->NotifySurfaceWindowChanged(fml::RefPtr<OHOSNativeWindow>());
  view()->NotifySurfaceWindowChanged(MakePvUtWindow(kPvUtHandleA));
  view()->NotifyChanged(DlISize{100, 50});
  FlushTasks();
  EXPECT_EQ(delegate().schedule_frame_count(), 0);
  EXPECT_EQ(delegate().metrics_count(), 0u);
  EXPECT_EQ(delegate().created_count(), 0);
  EXPECT_NO_FATAL_FAILURE(
      view()->NotifySurfaceWindowChanged(MakePvUtWindow(kPvUtHandleA)));
}

TEST_F(PlatformViewOHOSUt, NapiForwardingAndPreEngineRestartSafe) {
  auto packets =
      std::shared_ptr<std::string[]>(new std::string[1]{std::string("p")});
  view()->OnTouchEvent(nullptr, 0);
  view()->OnTouchEvent(packets, 1);
  view()->OnMouseEvent(packets, 1);
  view()->OnAxisEvent(packets, 1);
  EXPECT_NO_FATAL_FAILURE(base()->OnPreEngineRestart());
  FlushTasks();
}

TEST_F(PlatformViewOHOSUt, PreloadThenNotifyCreateFiresFirstFrameCallback) {
  ASSERT_NE(base()->CreateExternalViewEmbedder(), nullptr);
  view()->Preload(640, 480);
  FlushTasks();
  EXPECT_EQ(delegate().created_count(), 1);

  view()->NotifyCreate(MakePvUtWindow(kPvUtHandleA));
  FlushTasks();
  EXPECT_EQ(delegate().created_count(), 1);
  EXPECT_EQ(delegate().schedule_frame_count(), 0);
  EXPECT_EQ(delegate().next_frame_callback_count(), 2);
}

TEST_F(PlatformViewOHOSUt, PreloadThenNotifyCreateSchedulesWhenSizeMatches) {
  PvUtKnobGuard guard;
  PvUtSetWindowGeometry(100, 100);
  ASSERT_NE(base()->CreateExternalViewEmbedder(), nullptr);
  view()->Preload(100, 100);
  FlushTasks();
  EXPECT_EQ(delegate().created_count(), 1);

  view()->NotifyCreate(MakePvUtWindow(kPvUtHandleA));
  FlushTasks();
  EXPECT_EQ(delegate().schedule_frame_count(), 1);
  EXPECT_EQ(delegate().created_count(), 1);
}

TEST_F(PlatformViewOHOSUt, NotifySurfaceChangedForViewZeroSizeKeepsMetrics) {
  ASSERT_NE(base()->CreateExternalViewEmbedder(), nullptr);
  ASSERT_TRUE(
      view()->NotifyCreateForView(11, MakePvUtWindow(kPvUtHandleA), 300, 400));
  ASSERT_EQ(delegate().metrics_for(11).size(), 1u);

  view()->NotifySurfaceChangedForView(11, MakePvUtWindow(kPvUtHandleB), 0, 0);
  FlushTasks();
  EXPECT_EQ(delegate().metrics_for(11).size(), 1u);
}

TEST_F(PlatformViewOHOSUt, OnSurfaceCreatedInertAtRestoreLevel) {
  EXPECT_FALSE(view()->IsFrameGateEnabled());
  view()->OnSurfaceCreated();
  EXPECT_FALSE(view()->IsFrameGateEnabled());
  EXPECT_EQ(delegate().schedule_frame_count(), 0);
  EXPECT_EQ(delegate().message_count("flutter/system"), 0);
}

TEST_F(PlatformViewOHOSUt, DeferredAggressiveSupersededSkipsTeardown) {
  ASSERT_NE(base()->CreateExternalViewEmbedder(), nullptr);
  view()->NotifyCreate(MakePvUtWindow(kPvUtHandleA));
  FlushTasks();
  SendLifecycle("AppLifecycleState.paused");
  FlushTasks();
  EXPECT_TRUE(view()->IsFrameGateEnabled());
  SendLifecycle("AppLifecycleState.resumed");
  FlushTasks();
  EXPECT_FALSE(view()->IsFrameGateEnabled());
  ASSERT_TRUE(WaitForPlatformIdleAfter(fml::TimeDelta::FromMilliseconds(1400)));
  EXPECT_FALSE(view()->IsFrameGateEnabled());
  SendLifecycle("AppLifecycleState.paused");
  FlushTasks();
  EXPECT_TRUE(view()->IsFrameGateEnabled());
}

TEST_F(PlatformViewOHOSUt, DeferredAggressiveCoreSkipsAfterNotifyDestroyed) {
  SendLifecycle("AppLifecycleState.paused");
  FlushTasks();
  EXPECT_TRUE(view()->IsFrameGateEnabled());
  view()->NotifyDestroyed();
  FlushTasks();
  EXPECT_EQ(delegate().destroyed_count(), 1);
  ASSERT_TRUE(WaitForPlatformIdleAfter(fml::TimeDelta::FromMilliseconds(1400)));
  EXPECT_TRUE(view()->IsFrameGateEnabled());
  SendLifecycle("AppLifecycleState.resumed");
  FlushTasks();
  EXPECT_FALSE(view()->IsFrameGateEnabled());
}

TEST_F(PlatformViewOHOSUtNoCtx, DeferredAggressiveCoreStopsAtNullSurface) {
  SendLifecycle("AppLifecycleState.paused");
  FlushTasks();
  EXPECT_TRUE(view()->IsFrameGateEnabled());
  EXPECT_EQ(delegate().schedule_frame_count(), 1);
  ASSERT_TRUE(WaitForPlatformIdleAfter(fml::TimeDelta::FromMilliseconds(1400)));
  int frames = delegate().schedule_frame_count();
  SendLifecycle("AppLifecycleState.resumed");
  FlushTasks();
  EXPECT_FALSE(view()->IsFrameGateEnabled());
  EXPECT_EQ(delegate().schedule_frame_count(), frames);
}

TEST_F(PlatformViewOHOSUt, UpdateDisplaySizeSingleAxisChangeResetsViewport) {
  ViewportMetrics metrics;
  metrics.physical_width = 100;
  metrics.physical_height = 200;
  view()->SetViewportMetrics(kFlutterImplicitViewId, metrics);
  ASSERT_EQ(delegate().metrics_count(), 1u);

  view()->UpdateDisplaySize(100, 200);
  ASSERT_EQ(delegate().metrics_count(), 2u);
  view()->UpdateDisplaySize(100, 300);
  ASSERT_EQ(delegate().metrics_count(), 3u);
  auto events = delegate().metrics_for(kFlutterImplicitViewId);
  EXPECT_EQ(events[2].physical_width, 100);
  EXPECT_EQ(events[2].physical_height, 300);
}

TEST_F(PlatformViewOHOSUt, SetSemanticsTreeEnabledTrueKeepsTree) {
  bridge()->is_accessibility_enabled_ = false;
#if defined(OHOS_X64_UNITTEST)
  static char provider_storage;
  bridge()->provider_ohos_ =
      reinterpret_cast<ArkUI_AccessibilityProvider*>(&provider_storage);
#endif
  SemanticsNodeUpdates nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1};
  nodes[0] = root;
  SemanticsNode node;
  node.id = 1;
  node.label = "keep";
  nodes[1] = node;
  base()->UpdateSemantics(kFlutterImplicitViewId, nodes, {});
#if defined(OHOS_X64_UNITTEST)
  ASSERT_NE(bridge()->tree_.FindNodeById(1), nullptr);
#else
  EXPECT_EQ(bridge()->tree_.FindNodeById(1), nullptr);
#endif

  base()->SetSemanticsTreeEnabled(true);
#if defined(OHOS_X64_UNITTEST)
  EXPECT_NE(bridge()->tree_.FindNodeById(1), nullptr);
#endif
}

#if !defined(OHOS_X64_UNITTEST)
TEST_F(PlatformViewOHOSUt, HybridCompositionEnabledWithGlContextWhenEglWorks) {
  std::shared_ptr<OHOSContext> gl_context =
      CreateOHOSContext(runners(), OHOSRenderingAPI::kOpenGLES, false, false,
                        false);
  ASSERT_NE(gl_context, nullptr);
  if (!gl_context->IsValid()) {
    GTEST_SKIP() << "EGL display unavailable on emulator";
  }
  OhosSurfaceFactoryImpl probe(gl_context);
  auto probe_surface = probe.CreateSurface();
  if (!probe_surface || !probe_surface->IsValid()) {
    GTEST_SKIP() << "EGL offscreen surface unavailable on emulator";
  }

  PvOhosRecordingDelegate hcpp_delegate;
  hcpp_delegate.settings_ = MakeTestSettings();
  hcpp_delegate.settings_.enable_ohos_hybrid_composition = true;
  auto napi = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  PlatformViewOHOS hcpp_view(hcpp_delegate, *runners_, napi, gl_context);
  EXPECT_TRUE(hcpp_view.IsHybridCompositionEnabled());

  int dummy_window = 0;
  hcpp_view.SetHybridCompositionOverlayWindow(&dummy_window);
  PlatformView* hcpp_base = &hcpp_view;
  auto embedder1 = hcpp_base->CreateExternalViewEmbedder();
  ASSERT_NE(embedder1, nullptr);
  auto embedder2 = hcpp_base->CreateExternalViewEmbedder();
  EXPECT_EQ(embedder2.get(), embedder1.get());

  hcpp_view.ClearHybridCompositionOverlayWindowSync();
  hcpp_view.SetHybridCompositionOverlayWindow(&dummy_window);
  hcpp_view.SetHybridCompositionOverlayWindow(nullptr);
}
#endif  // !defined(OHOS_X64_UNITTEST)

TEST_F(PlatformViewOHOSWbTest, UnknownTextureIdReturnsEarly) {
  CallFrameAvailable(0x42);
  FlushTasks();
  EXPECT_TRUE(delegate().marked_texture_ids().empty());
  EXPECT_EQ(delegate().schedule_frame_count(), 0);
}

TEST_F(PlatformViewOHOSWbTest, NullPlatformEntryReturnsEarly) {
  WbMapEntry entry(0x42, nullptr);
  CallFrameAvailable(0x42);
  FlushTasks();
  EXPECT_TRUE(delegate().marked_texture_ids().empty());
}

class PlatformViewOHOSWbTestNoCtx : public PlatformViewOHOSWbTest {
 protected:
  PlatformViewOHOSWbTestNoCtx() : PlatformViewOHOSWbTest(false) {}
};

TEST_F(PlatformViewOHOSWbTestNoCtx, SurfacelessViewReturnsEarly) {
  WbMapEntry entry(0x42, view());
  CallFrameAvailable(0x42);
  FlushTasks();
  EXPECT_TRUE(delegate().marked_texture_ids().empty());
  EXPECT_EQ(delegate().schedule_frame_count(), 0);
}

TEST_F(PlatformViewOHOSWbTest, MarksFrameThroughPlatformTask) {
  WbMapEntry entry(0x42, view());
  CallFrameAvailable(0x42);
  FlushTasks();
  auto marked = delegate().marked_texture_ids();
  ASSERT_EQ(marked.size(), 1u);
  EXPECT_EQ(marked[0], static_cast<int64_t>(0x42));
}

TEST_F(PlatformViewOHOSWbTest, InnerLookupMissSkipsMark) {
  WbMapEntry entry(0x42, view());
  fml::AutoResetWaitableEvent block;
  runners().GetPlatformTaskRunner()->PostTask([&] { block.Wait(); });
  CallFrameAvailable(0x42);
  entry.Erase();
  block.Signal();
  FlushTasks();
  EXPECT_TRUE(delegate().marked_texture_ids().empty());
}

TEST_F(PlatformViewOHOSWbTest, GateEnabledDrainsThroughTextureOnly) {
  constexpr int64_t kTexId = 0x42;
  auto texture = std::make_shared<WbFakeTexture>(kTexId);
  view()->all_external_texture_[kTexId] = texture;
  WbMapEntry entry(static_cast<uint64_t>(kTexId), view());
  SendLifecycle("AppLifecycleState.paused");
  FlushTasks();
  ASSERT_TRUE(view()->IsFrameGateEnabled());

  CallFrameAvailable(static_cast<uint64_t>(kTexId));
  FlushTasks();
  FlushTasks();
  EXPECT_EQ(texture->mark_new_frame_calls(), 1);
  EXPECT_TRUE(delegate().marked_texture_ids().empty());

  SendLifecycle("AppLifecycleState.resumed");
  FlushTasks();
  EXPECT_FALSE(view()->IsFrameGateEnabled());
}

TEST_F(PlatformViewOHOSWbTest, GateEnabledWithoutTextureSkips) {
  WbMapEntry entry(0x42, view());
  SendLifecycle("AppLifecycleState.paused");
  FlushTasks();
  ASSERT_TRUE(view()->IsFrameGateEnabled());

  CallFrameAvailable(0x42);
  FlushTasks();
  EXPECT_TRUE(delegate().marked_texture_ids().empty());

  SendLifecycle("AppLifecycleState.resumed");
  FlushTasks();
  EXPECT_FALSE(view()->IsFrameGateEnabled());
}

TEST_F(PlatformViewOHOSWbTest, LookupAndBackgroundColor) {
  constexpr int64_t kId = 60;
  auto texture = std::make_shared<WbFakeTexture>(kId);
  view()->all_external_texture_[kId] = texture;

  EXPECT_EQ(view()->GetExternalTextureWindowId(kId),
            texture->GetProducerWindowId());
  EXPECT_EQ(view()->GetExternalTextureWindowId(999), 0u);

  int frames = delegate().schedule_frame_count();
  view()->SetExternalTextureBackGroundColor(kId, 0xff00ff00u);
  EXPECT_EQ(delegate().schedule_frame_count(), frames + 1);
  view()->SetExternalTextureBackGroundColor(999, 0xff00ff00u);
  EXPECT_EQ(delegate().schedule_frame_count(), frames + 1);
}

TEST_F(PlatformViewOHOSWbTest, BufferSizeAndResizingState) {
  constexpr int64_t kId = 61;
  auto texture = std::make_shared<WbFakeTexture>(kId);
  view()->all_external_texture_[kId] = texture;

  view()->SetTextureBufferSize(kId, 64, 48);
  EXPECT_EQ(texture->producer_nativewindow_width_, 64);
  EXPECT_EQ(texture->producer_nativewindow_height_, 48);

  view()->NotifyTextureResizing(kId, 32, 24);
  EXPECT_TRUE(texture->size_is_changing_.load());
}

TEST_F(PlatformViewOHOSWbTest, BackgroundPixelMapPaths) {
  GraphicStubKnobGuard knob_guard;
  constexpr int64_t kId = 62;
  auto texture = std::make_shared<WbFakeTexture>(kId);
  view()->all_external_texture_[kId] = texture;

  int frames = delegate().schedule_frame_count();
  view()->SetExternalTextureBackGroundPixelMap(kId, nullptr, nullptr);
  EXPECT_EQ(delegate().schedule_frame_count(), frames);
  view()->SetExternalTextureBackGroundPixelMap(
      kId, reinterpret_cast<NativePixelMap*>(0x999),
      reinterpret_cast<OH_NativeBuffer*>(0x998));
  EXPECT_EQ(delegate().schedule_frame_count(), frames + 1);
  FlushTasks();
  view()->all_external_texture_.erase(kId);
}

TEST_F(PlatformViewOHOSWbTest, SetExternalNativeImageResult) {
  GraphicStubKnobGuard knob_guard;
  constexpr int64_t kId = 63;
  auto texture = std::make_shared<WbFakeTexture>(kId);
  view()->all_external_texture_[kId] = texture;

  EXPECT_FALSE(view()->SetExternalNativeImage(kId, nullptr));
  EXPECT_TRUE(view()->SetExternalNativeImage(
      kId, reinterpret_cast<OH_NativeImage*>(0x777)));
  FlushTasks();
  view()->all_external_texture_.erase(kId);
}

TEST_F(PlatformViewOHOSWbTest, ResetReturnsFreshSurfaceId) {
  constexpr int64_t kId = 64;
  auto texture = std::make_shared<WbFakeTexture>(kId);
  view()->all_external_texture_[kId] = texture;

  EXPECT_EQ(view()->ResetExternalTexture(kId, false), 0u);
  EXPECT_EQ(view()->ResetExternalTexture(kId, true),
            texture->GetProducerSurfaceId());
}

TEST_F(PlatformViewOHOSWbTest, UnregisterCleansGlobalMap) {
  constexpr int64_t kId = 71;
  auto texture = std::make_shared<WbFakeTexture>(kId);
  view()->all_external_texture_[kId] = texture;
  WbMapEntry entry(static_cast<uint64_t>(kId), view());

  view()->UnRegisterExternalTexture(kId);
  FlushTasks();
  EXPECT_EQ(view()->all_external_texture_.count(kId), 0u);
  auto unregistered = delegate().unregistered_ids();
  ASSERT_EQ(unregistered.size(), 1u);
  EXPECT_EQ(unregistered[0], kId);
  EXPECT_FALSE(entry.Present(view()));
}

TEST_F(PlatformViewOHOSWbTest, CreateAndDestroyWalkTextureRegistry) {
  constexpr int64_t kId = 70;
  auto texture = std::make_shared<WbFakeTexture>(kId);
  view()->all_external_texture_[kId] = texture;

  view()->NotifyCreate(WbMakeWindow(kWbHandleA));
  FlushTasks();
  EXPECT_EQ(delegate().register_texture_count(), 1);
  {
    std::lock_guard<std::recursive_mutex> lock(g_map_mutex);
    auto it = g_texture_platformview_map.find(static_cast<uint64_t>(kId));
    ASSERT_NE(it, g_texture_platformview_map.end());
    EXPECT_EQ(it->second, view());
  }

  view()->NotifyDestroyed();
  FlushTasks();
  EXPECT_EQ(delegate().destroyed_count(), 1);
  auto unregistered = delegate().unregistered_ids();
  ASSERT_EQ(unregistered.size(), 1u);
  EXPECT_EQ(unregistered[0], kId);
  {
    std::lock_guard<std::recursive_mutex> lock(g_map_mutex);
    EXPECT_EQ(g_texture_platformview_map.count(static_cast<uint64_t>(kId)), 0u);
  }
}

TEST_F(PlatformViewOHOSWbTest, CreateExternalTextureGlesArm) {
  std::shared_ptr<OHOSContext> gl_context =
      CreateOHOSContext(runners(), OHOSRenderingAPI::kOpenGLES, false, false,
                        false);
  ASSERT_NE(gl_context, nullptr);
  auto saved = view()->ohos_context_;
  view()->ohos_context_ = gl_context;
  auto texture = view()->CreateExternalTexture(80);
  ASSERT_NE(texture, nullptr);
  EXPECT_EQ(delegate().register_texture_count(), 1);
  EXPECT_EQ(view()->all_external_texture_.count(80), 1u);
  {
    std::lock_guard<std::recursive_mutex> lock(g_map_mutex);
    EXPECT_EQ(g_texture_platformview_map.count(80), 1u);
  }
  EXPECT_NE(view()->RegisterExternalTexture(80), 0u);
  EXPECT_EQ(delegate().register_texture_count(), 2);
  view()->ohos_context_ = saved;
  fml::AutoResetWaitableEvent context_released;
  runners().GetPlatformTaskRunner()->PostTask(
      [&context_released, dying_context = std::move(gl_context)] {
        context_released.Signal();
      });
  context_released.Wait();
}

TEST_F(PlatformViewOHOSWbTest, CreateExternalTextureVulkanArm) {
  std::shared_ptr<OHOSContext> vk_context = CreateOHOSContext(
      runners(), OHOSRenderingAPI::kImpellerVulkan, false, false, false);
  ASSERT_NE(vk_context, nullptr);
  auto saved = view()->ohos_context_;
  view()->ohos_context_ = vk_context;
  auto texture = view()->CreateExternalTexture(81);
  view()->ohos_context_ = saved;
  ASSERT_NE(texture, nullptr);
  EXPECT_EQ(delegate().register_texture_count(), 1);
  EXPECT_EQ(view()->all_external_texture_.count(81), 1u);
  EXPECT_EQ(view()->GetExternalTextureWindowId(81),
            texture->GetProducerWindowId());
}

TEST_F(PlatformViewOHOSWbTest, RegisterByPixelMapSchedulesOnSuccess) {
  GraphicStubKnobGuard knob_guard;
  std::shared_ptr<OHOSContext> gl_context =
      CreateOHOSContext(runners(), OHOSRenderingAPI::kOpenGLES, false, false,
                        false);
  ASSERT_NE(gl_context, nullptr);
  int frames = delegate().schedule_frame_count();
  auto saved = view()->ohos_context_;
  view()->ohos_context_ = gl_context;
  view()->RegisterExternalTextureByPixelMap(
      82, reinterpret_cast<NativePixelMap*>(0x999),
      reinterpret_cast<OH_NativeBuffer*>(0x998));
  view()->ohos_context_ = saved;
  FlushTasks();
  EXPECT_EQ(delegate().schedule_frame_count(), frames + 1);
  EXPECT_EQ(view()->all_external_texture_.count(82), 1u);
  view()->all_external_texture_.erase(82);
}

TEST_F(PlatformViewOHOSWbTest, NullContextSkipsSkiaFreeStillTearsDown) {
  auto saved = view()->ohos_context_;
  view()->ohos_context_ = nullptr;
  view()->onscreen_context_valid_.store(true);
  view()->ExecuteReclaimAggressiveCore();
  EXPECT_FALSE(view()->onscreen_context_valid_.load());
  EXPECT_TRUE(view()->ohos_surface_->IsValid());
  view()->ohos_context_ = saved;
}

TEST_F(PlatformViewOHOSWbTest, TryFreeSkiaGpuResourcesGuardArms) {
  auto software_context =
      std::make_shared<OHOSContext>(OHOSRenderingAPI::kSoftware);
  EXPECT_NO_FATAL_FAILURE(
      PlatformViewOHOS::TryFreeSkiaGpuResources(nullptr, nullptr));
  EXPECT_NO_FATAL_FAILURE(PlatformViewOHOS::TryFreeSkiaGpuResources(
      view()->ohos_surface_, nullptr));
  EXPECT_NO_FATAL_FAILURE(
      PlatformViewOHOS::TryFreeSkiaGpuResources(nullptr, software_context));
  EXPECT_NO_FATAL_FAILURE(PlatformViewOHOS::TryFreeSkiaGpuResources(
      view()->ohos_surface_, software_context));
  EXPECT_TRUE(view()->ohos_surface_->IsValid());
}

TEST_F(PlatformViewOHOSWbTest, DeferredAggressiveCancelledByPipVisible) {
  SendLifecycle("AppLifecycleState.paused");
  FlushTasks();
  ASSERT_TRUE(view()->IsFrameGateEnabled());
  view()->pip_visible_.store(true, std::memory_order_release);
  ASSERT_TRUE(
      WaitForPlatformIdleAfter(fml::TimeDelta::FromMilliseconds(1400)));
  EXPECT_FALSE(view()->IsFrameGateEnabled());
  EXPECT_TRUE(view()->onscreen_context_valid_.load());
  EXPECT_EQ(view()->current_reclaim_level_, GpuReclaimLevel::kRestore);
}

#if !defined(OHOS_X64_UNITTEST)
TEST_F(PlatformViewOHOSWbTest, SkiaArmOnGlContextWhenEglWorks) {
  std::shared_ptr<OHOSContext> gl_context =
      CreateOHOSContext(runners(), OHOSRenderingAPI::kOpenGLES, false, false,
                        false);
  ASSERT_NE(gl_context, nullptr);
  if (!gl_context->IsValid()) {
    GTEST_SKIP() << "EGL display unavailable on emulator";
  }
  OhosSurfaceFactoryImpl probe(gl_context);
  auto probe_surface = probe.CreateSurface();
  if (!probe_surface || !probe_surface->IsValid()) {
    GTEST_SKIP() << "EGL offscreen surface unavailable on emulator";
  }

  WbRecordingDelegate gl_delegate;
  gl_delegate.settings_ = MakeWbSettings();
  PlatformViewOHOS gl_view(gl_delegate, runners(), napi_facade_, gl_context);
  gl_view.onscreen_context_valid_.store(true);
  EXPECT_NO_FATAL_FAILURE(gl_view.ExecuteReclaimAggressiveCore());
  EXPECT_FALSE(gl_view.onscreen_context_valid_.load());
  FlushTasks();
}
#endif  // !defined(OHOS_X64_UNITTEST)

#if !defined(OHOS_X64_UNITTEST)
TEST_F(PlatformViewOHOSWbTest, TryFreeSkiaGpuResourcesOnGlContext) {
  std::shared_ptr<OHOSContext> gl_context =
      CreateOHOSContext(runners(), OHOSRenderingAPI::kOpenGLES, false, false,
                        false);
  ASSERT_NE(gl_context, nullptr);
  if (!gl_context->IsValid()) {
    GTEST_SKIP() << "EGL display unavailable on emulator";
  }
  OhosSurfaceFactoryImpl probe(gl_context);
  std::shared_ptr<OHOSSurface> gl_surface = probe.CreateSurface();
  if (!gl_surface || !gl_surface->IsValid()) {
    GTEST_SKIP() << "EGL offscreen surface unavailable on emulator";
  }

  EXPECT_NO_FATAL_FAILURE(PlatformViewOHOS::TryFreeSkiaGpuResources(
      view()->ohos_surface_, gl_context));
  EXPECT_TRUE(view()->ohos_surface_->IsValid());
  bool was_valid = gl_surface->IsValid();
  EXPECT_NO_FATAL_FAILURE(
      PlatformViewOHOS::TryFreeSkiaGpuResources(gl_surface, gl_context));
  EXPECT_EQ(gl_surface->IsValid(), was_valid);
}
#endif  // !defined(OHOS_X64_UNITTEST)

TEST_F(PlatformViewOHOSWbTest, NotifyCreateForViewPostsImplicitNotify) {
  auto saved_surface = view()->ohos_surface_;
  view()->ohos_surface_ = nullptr;
  EXPECT_FALSE(
      view()->NotifyCreateForView(4, WbMakeWindow(kWbHandleA), 100, 100));
  EXPECT_TRUE(view()->implicit_view_notify_posted_.load());
  EXPECT_TRUE(view()->secondary_surfaces_.empty());
  view()->ohos_surface_ = saved_surface;
  FlushTasks();

  EXPECT_FALSE(
      view()->NotifyCreateForView(4, WbMakeWindow(kWbHandleA), 100, 100));
  EXPECT_TRUE(view()->implicit_view_notify_posted_.load());
  EXPECT_TRUE(view()->secondary_surfaces_.empty());
  FlushTasks();
}

TEST_F(PlatformViewOHOSWbTest, NullMessageGuardKeepsLifecycleState) {
  EXPECT_EQ(view()->lifecycle_state_, AppLifecycleState::kDetached);
  view()->HandleLifecyclePlatformMessage("flutter/lifecycle", nullptr, 5);
  EXPECT_EQ(view()->lifecycle_state_, AppLifecycleState::kDetached);
  EXPECT_EQ(delegate().message_count("flutter/system"), 0);

  SendLifecycle("AppLifecycleState.paused");
  FlushTasks();
  EXPECT_EQ(view()->lifecycle_state_, AppLifecycleState::kPaused);
  SendLifecycle("AppLifecycleState.resumed");
  FlushTasks();
}

}  // namespace testing
}  // namespace flutter
