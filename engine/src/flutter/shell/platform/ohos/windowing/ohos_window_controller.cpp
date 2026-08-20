/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/shell/platform/ohos/windowing/ohos_window_controller.h"

#include <deviceinfo.h>
#include <algorithm>
#include <cstring>
#include <mutex>
#include <set>
#include <string_view>
#include <utility>
#include <vector>

#include "flutter/fml/logging.h"
#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"
#include "flutter/shell/platform/ohos/ohos_shell_holder.h"
#include "flutter/shell/platform/ohos/windowing/ohos_window_anchored.h"
#include "flutter/shell/platform/ohos/windowing/ohos_window_dialog.h"
#include "flutter/shell/platform/ohos/windowing/ohos_window_popup.h"
#include "flutter/shell/platform/ohos/windowing/ohos_window_regular.h"
#include "flutter/shell/platform/ohos/windowing/ohos_window_tooltip.h"

static std::mutex g_controllers_mutex;
static std::set<flutter::OHOSWindowController*> g_controllers;

namespace flutter {

// Process-wide view-id source (see the header member comment): starts at 1 —
// 0 is the implicit view — and never collides across controllers.
std::atomic<int64_t> OHOSWindowController::g_next_view_id_{
    static_cast<int64_t>(kFlutterImplicitViewId) + 1};

OHOSWindowController::OHOSWindowController(OHOSShellHolder* holder)
    : holder_(holder) {
  FML_DCHECK(holder_ != nullptr);
  std::lock_guard<std::mutex> lock(g_controllers_mutex);
  g_controllers.insert(this);
}

OHOSWindowController::~OHOSWindowController() {
  std::lock_guard<std::mutex> lock(g_controllers_mutex);
  g_controllers.erase(this);
}

void OHOSWindowController::SetEngineId(int64_t engine_id) {
  engine_id_ = engine_id;
}

// static
OHOSWindowController* OHOSWindowController::ActiveInstance() {
  std::lock_guard<std::mutex> lock(g_controllers_mutex);
  if (g_controllers.size() == 1) {
    return *g_controllers.begin();
  }
  return nullptr;
}

// static
OHOSWindowController* OHOSWindowController::ForView(int64_t view_id) {
  std::vector<OHOSWindowController*> snapshot;
  {
    std::lock_guard<std::mutex> lock(g_controllers_mutex);
    snapshot.assign(g_controllers.begin(), g_controllers.end());
  }
  // windows_ lookups take the per-controller mutex (LookupWindow), so no lock
  // ordering issue with g_controllers_mutex (released above).
  for (OHOSWindowController* c : snapshot) {
    if (c->GetHandleForView(view_id) != nullptr) {
      return c;
    }
  }
  return nullptr;
}

// static
OHOSWindowController* OHOSWindowController::ForEngineId(int64_t engine_id) {
  std::vector<OHOSWindowController*> snapshot;
  {
    std::lock_guard<std::mutex> lock(g_controllers_mutex);
    snapshot.assign(g_controllers.begin(), g_controllers.end());
  }
  for (OHOSWindowController* c : snapshot) {
    if (c->engine_id_ == engine_id) {
      return c;
    }
  }
  return nullptr;
}

// static
OHOSWindowController* OHOSWindowController::ForWindowHandle(void* window) {
  std::vector<OHOSWindowController*> snapshot;
  {
    std::lock_guard<std::mutex> lock(g_controllers_mutex);
    snapshot.assign(g_controllers.begin(), g_controllers.end());
  }
  for (OHOSWindowController* c : snapshot) {
    if (c->LookupWindow(window) != nullptr) {
      return c;
    }
  }
  return nullptr;
}

std::shared_ptr<PlatformViewOHOSNapi> OHOSWindowController::GetNapiFacade() {
  return holder_->GetNapiFacade();
}

OHOSWindow* OHOSWindowController::LookupWindow(void* window) {
  std::lock_guard<std::mutex> lock(windows_mutex_);
  auto it = windows_.find(window);
  return it == windows_.end() ? nullptr : it->second.get();
}

const OHOSWindow* OHOSWindowController::LookupWindow(void* window) const {
  std::lock_guard<std::mutex> lock(windows_mutex_);
  auto it = windows_.find(window);
  return it == windows_.end() ? nullptr : it->second.get();
}

int64_t OHOSWindowController::AllocateViewId() {
  // Monotonic PROCESS-WIDE counter (g_next_view_id_): ids start at 1, never
  // colliding with the implicit 0, any live view, or another engine's views
  // (ids must stay unique across controllers for the multi-engine FFI
  // routing — see the member comment).
  return g_next_view_id_.fetch_add(1);
}

// The handle handed to Dart is the view id biased by +1: view 0's handle must
// NOT be nullptr (the shared "not found" sentinel), or any null-guard added
// on either side of the FFI boundary would silently break every main-window
// handle operation.
// static
void* OHOSWindowController::HandleForViewId(int64_t view_id) {
  return reinterpret_cast<void*>(view_id + 1);
}

// static
int64_t OHOSWindowController::ViewIdForHandle(void* handle) {
  return reinterpret_cast<int64_t>(handle) - 1;
}

WindowHostKind OHOSWindowController::ResolveHostKind(WindowType type,
                                                     bool has_parent,
                                                     int64_t parent_view_id) {
  switch (type) {
    case WindowType::kRegular:
      return WindowHostKind::kUiAbility;
    case WindowType::kDialog:
      // Must use the explicit `has_parent` flag: the main window IS view 0,
      // so parent_view_id == 0 is a legitimate parent, not "no parent".
      return has_parent ? WindowHostKind::kSubWindow
                        : WindowHostKind::kUiAbility;
    case WindowType::kTooltip:
    case WindowType::kPopup:
      return WindowHostKind::kSubWindow;
  }
}

std::unique_ptr<OHOSWindow> OHOSWindowController::CreateWindowObject(
    const FlutterWindowCreationRequest& request,
    const OHOSWindow::InitParams& params) {
  switch (params.type) {
    case WindowType::kRegular:
      return std::make_unique<OHOSWindowRegular>(this, params, request);
    case WindowType::kDialog:
      return std::make_unique<OHOSWindowDialog>(this, params, request);
    case WindowType::kTooltip:
      return std::make_unique<OHOSWindowTooltip>(this, params, request);
    case WindowType::kPopup:
      return std::make_unique<OHOSWindowPopup>(this, params, request);
  }
}

int64_t OHOSWindowController::CreateWindow(
    const FlutterWindowCreationRequest& request,
    WindowType type) {
  const WindowHostKind kind =
      ResolveHostKind(type, request.has_parent, request.parent_view_id);

  // The FIRST Regular UIAbility window ADOPTS implicit view 0 — the
  // EntryAbility's pre-created window, already added at engine startup, so it
  // must NOT be re-AddView'd (AddView DCHECKs view_id != implicit). All other
  // windows (including a dialog created first) allocate a fresh id: only a
  // Regular window may adopt view 0.
  const bool is_first_ui_ability =
      (type == WindowType::kRegular && kind == WindowHostKind::kUiAbility &&
       !entry_ability_bound_);
  int64_t view_id;
  if (is_first_ui_ability) {
    view_id = static_cast<int64_t>(kFlutterImplicitViewId);  // 0
    entry_ability_bound_ = true;
  } else {
    view_id = AllocateViewId();
    if (!AddViewSync(view_id)) {
      return kCreateWindowFailedViewId;
    }
  }

  void* const handle = HandleForViewId(view_id);

  OHOSWindow::InitParams params;
  params.type = type;
  params.host_kind = kind;
  params.view_id = view_id;
  params.parent_view_id = request.parent_view_id;
  params.host_handle = handle;
  params.adopt_entry_ability = is_first_ui_ability;
  auto window = CreateWindowObject(request, params);
  OHOSWindow* const window_ptr = window.get();
  {
    std::lock_guard<std::mutex> lock(windows_mutex_);
    windows_[handle] = std::move(window);
  }

  window_ptr->RequestWindowHost();

  return view_id;
}

int64_t OHOSWindowController::CreateRegularWindow(
    const FlutterWindowCreationRequest& request) {
  return CreateWindow(request, WindowType::kRegular);
}

int64_t OHOSWindowController::CreateDialogWindow(
    const FlutterWindowCreationRequest& request) {
  return CreateWindow(request, WindowType::kDialog);
}

int64_t OHOSWindowController::CreateTooltipWindow(
    const FlutterWindowCreationRequest& request) {
  return CreateWindow(request, WindowType::kTooltip);
}

int64_t OHOSWindowController::CreatePopupWindow(
    const FlutterWindowCreationRequest& request) {
  return CreateWindow(request, WindowType::kPopup);
}

void OHOSWindowController::DestroyWindow(void* window) {
  // Take SOLE ownership under the lock before firing any Dart callback:
  // callbacks re-enter Dart, which may call destroy()/FFI back on this or
  // another thread; a concurrent erase must never free the object a
  // lock-released callback is about to touch.
  std::unique_ptr<OHOSWindow> window_ptr;
  {
    std::lock_guard<std::mutex> lock(windows_mutex_);
    auto it = windows_.find(window);
    if (it == windows_.end()) {
      FML_LOG(WARNING) << "DestroyWindow: unknown window handle " << window;
      return;
    }
    window_ptr = std::move(it->second);
    windows_.erase(it);
  }
  const int64_t view_id = window_ptr->view_id();

  // Fire will-close while the window still exists (sole local ownership), so
  // Dart runs its teardown.
  window_ptr->FireWillClose();

  // The implicit view 0 lives for the whole engine lifetime — never
  // RemoveView'd (Shell::OnPlatformViewRemoveView DCHECKs view_id !=
  // implicit). Closing the main window = quitting the app: ExitApplication
  // runs the full Ability shutdown cascade.
  if (view_id != static_cast<int64_t>(kFlutterImplicitViewId)) {
    RemoveViewOnPlatformThread(view_id);
    if (auto facade = holder_->GetNapiFacade()) {
      facade->DestroyWindowHost(view_id);
    }
  } else {
    if (auto facade = holder_->GetNapiFacade()) {
      facade->ExitApplication();
    }
  }
  // `window_ptr` (sole owner since the move) destructs here, after every
  // callback and facade hop that could touch it has returned.
}

void* OHOSWindowController::GetHandleForView(int64_t view_id) {
  void* const handle = HandleForViewId(view_id);
  std::lock_guard<std::mutex> lock(windows_mutex_);
  return windows_.count(handle) ? handle : nullptr;
}

bool OHOSWindowController::AddViewSync(int64_t view_id) {
  if (!holder_->AddViewSync(view_id)) {
    FML_LOG(ERROR) << "AddViewSync failed for view " << view_id;
    return false;
  }
  return true;
}

void OHOSWindowController::RemoveViewOnPlatformThread(int64_t view_id) {
  auto platform_view = holder_->GetPlatformView();
  if (!platform_view) {
    FML_LOG(ERROR) << "RemoveView: platform view gone for view " << view_id;
    return;
  }
  platform_view->RemoveViewForWindow(view_id);
}

FlutterWindowSize OHOSWindowController::GetContentSize(void* window) const {
  const int64_t view_id = ViewIdForHandle(window);
  {
    std::lock_guard<std::mutex> lock(actual_sizes_mutex_);
    auto it = actual_sizes_.find(view_id);
    if (it != actual_sizes_.end()) {
      return it->second;
    }
  }
  const OHOSWindow* const win = LookupWindow(window);
  if (win != nullptr && win->request().has_size) {
    return {win->request().size.width, win->request().size.height};
  }
  return {0.0, 0.0};
}

void OHOSWindowController::SetViewActualSize(int64_t view_id,
                                             double physical_width,
                                             double physical_height,
                                             double density) {
  if (density <= 0.0) {
    density = 1.0;
  }
  const double logical_w = physical_width / density;
  const double logical_h = physical_height / density;
  {
    std::lock_guard<std::mutex> lock(actual_sizes_mutex_);
    actual_sizes_[view_id] = {logical_w, logical_h};
  }
}

void OHOSWindowController::SetContentSize(void* window,
                                          const FlutterWindowSize& size) {
  const int64_t view_id = ViewIdForHandle(window);
  if (auto facade = holder_->GetNapiFacade()) {
    facade->SetWindowSize(view_id, size.width, size.height);
  }
}
void OHOSWindowController::SetConstraints(
    void* window,
    const FlutterWindowConstraints& constraints) {
  const int64_t view_id = ViewIdForHandle(window);
  // Runtime constraint updates forward to the OS host (WMS clamps user
  // resizes to them) and refresh the stored request so later reads
  // reflect the live value instead of the creation-time copy.
  if (OHOSWindow* const win = LookupWindow(window)) {
    win->SetRuntimeConstraints(constraints);
  }
  if (auto facade = holder_->GetNapiFacade()) {
    facade->SetWindowConstraints(view_id, constraints.min_width,
                                 constraints.max_width, constraints.min_height,
                                 constraints.max_height);
  }
}
void OHOSWindowController::SetTitle(void* window, const char* title) {
  if (title == nullptr) {
    // The symbol is exported with default visibility: an external caller may
    // pass null, which std::string(const char*) would treat as UB.
    FML_LOG(WARNING) << "SetTitle: null title ignored for handle " << window;
    return;
  }
  // Cache for the Dart `title` query (GetTitle): SetTitle is the only writer
  // in this chain, so the cache IS the platform truth. LookupWindow takes
  // windows_mutex_ — no extra locking needed.
  if (auto* win = LookupWindow(window)) {
    win->SetTitleCache(std::string(title));
  }
  const int64_t view_id = ViewIdForHandle(window);
  if (auto facade = holder_->GetNapiFacade()) {
    facade->SetWindowTitle(view_id, std::string(title));
  }
}

void OHOSWindowController::GetTitle(void* window,
                                    char* out,
                                    int64_t capacity) const {
  if (out == nullptr || capacity <= 0) {
    return;
  }
  out[0] = '\0';
  if (const OHOSWindow* win = LookupWindow(window)) {
    const std::string& title = win->GetTitle();
    size_t copy =
        std::min<size_t>(title.size(), static_cast<size_t>(capacity - 1));
    while (copy > 0 &&
           (static_cast<unsigned char>(title[copy - 1]) & 0xC0) == 0x80) {
      copy--;
    }
    std::memcpy(out, title.data(), copy);
    out[copy] = '\0';
  }
}
void OHOSWindowController::Activate(void* window) {
  const int64_t view_id = ViewIdForHandle(window);
  if (auto facade = holder_->GetNapiFacade()) {
    facade->ActivateWindow(view_id);
  }
}

void OHOSWindowController::SetMaximized(void* window, bool maximized) {
  const int64_t view_id = ViewIdForHandle(window);
  if (auto facade = holder_->GetNapiFacade()) {
    facade->SetWindowMaximized(view_id, maximized);
  }
}

void OHOSWindowController::SetMinimized(void* window, bool minimized) {
  const int64_t view_id = ViewIdForHandle(window);
  if (auto facade = holder_->GetNapiFacade()) {
    facade->SetWindowMinimized(view_id, minimized);
  }
}

void OHOSWindowController::SetFullscreen(void* window, bool fullscreen) {
  const int64_t view_id = ViewIdForHandle(window);
  if (auto facade = holder_->GetNapiFacade()) {
    facade->SetWindowFullscreen(view_id, fullscreen);
  }
}

bool OHOSWindowController::ComputeWindowPosition(
    int64_t view_id,
    const FlutterWindowSize& child_size,
    const FlutterWindowRect& parent_rect,
    const FlutterWindowRect& work_area,
    FlutterWindowRect* out) {
  if (out == nullptr) {
    return false;
  }
  // The host handle is the biased view id (see HandleForViewId), so look up
  // directly.
  OHOSWindow* const win = LookupWindow(HandleForViewId(view_id));
  if (win == nullptr) {
    FML_DLOG(WARNING)
        << "ComputeWindowPosition: unknown view " << view_id
        << " (host not created yet, or already destroyed); keeping default "
           "placement.";
    return false;
  }
  return win->ComputeWindowPosition(child_size, parent_rect, work_area, out);
}

void OHOSWindowController::HandleOsWindowClosed(int64_t view_id) {
  void* const window = HandleForViewId(view_id);
  // Take SOLE ownership under the lock (same crash-safety contract as
  // DestroyWindow): the ArkTS thread must never dereference an object a
  // concurrent Dart-side DestroyWindow could already have erased+freed, and
  // a Dart destroy re-entering during FireShouldClose simply finds the map
  // empty and no-ops.
  std::unique_ptr<OHOSWindow> window_ptr;
  {
    std::lock_guard<std::mutex> lock(windows_mutex_);
    auto it = windows_.find(window);
    if (it == windows_.end()) {
      return;
    }
    window_ptr = std::move(it->second);
    windows_.erase(it);
  }

  window_ptr->FireShouldClose();
  window_ptr->FireWillClose();

  // Implicit view 0 is never RemoveView'd (see DestroyWindow).
  if (view_id != static_cast<int64_t>(kFlutterImplicitViewId)) {
    RemoveViewOnPlatformThread(view_id);
  }
  // `window_ptr` destructs here — sole owner since the move.
}
void OHOSWindowController::OnPreEngineRestart() {
  // The restarting isolate owns the windows' Dart callbacks: they must never
  // fire again. Beyond clearing the map, tear the views and their OS hosts
  // down too — RuntimeController::Clone carries viewport_metrics_for_views
  // into the NEW isolate, so leaving the views registered would resurrect
  // them as PlatformDispatcher.views with no controller/handle (ghost views:
  // ForView misses, sub-view surface events unrouted). Destroying the hosts
  // closes the OS windows and unregisters the ETS-side bookkeeping; the
  // restarted isolate starts from the adopted implicit view 0 alone.
  std::vector<std::unique_ptr<OHOSWindow>> doomed;
  {
    std::lock_guard<std::mutex> lock(windows_mutex_);
    doomed.reserve(windows_.size());
    for (auto& [handle, window] : windows_) {
      doomed.push_back(std::move(window));
    }
    windows_.clear();
  }
  auto facade = holder_->GetNapiFacade();
  for (const auto& window : doomed) {
    const int64_t view_id = window->view_id();
    if (view_id == static_cast<int64_t>(kFlutterImplicitViewId)) {
      continue;
    }
    RemoveViewOnPlatformThread(view_id);
    if (facade) {
      facade->DestroyWindowHost(view_id);
    }
  }
  entry_ability_bound_ = false;
}

void OHOSWindowController::NotifyListeners(void* window) {
  if (OHOSWindow* const win = LookupWindow(window)) {
    win->FireNotifyListeners();
  }
}

void OHOSWindowController::SetViewActivated(int64_t view_id, bool activated) {
  bool changed = false;
  {
    std::lock_guard<std::mutex> lock(view_activated_mutex_);
    auto it = view_activated_.find(view_id);
    if (it == view_activated_.end() || it->second != activated) {
      view_activated_[view_id] = activated;
      changed = true;
    }
  }
  if (changed) {
    // Ping dependents: Dart notifyListeners → isActivated dependents rebuild.
    // (view_activated_mutex_ is RELEASED here — NotifyListeners takes
    // windows_mutex_; keep the lock order one-way.)
    NotifyListeners(HandleForViewId(view_id));
  }
}

bool OHOSWindowController::GetViewActivated(int64_t view_id) const {
  std::lock_guard<std::mutex> lock(view_activated_mutex_);
  auto it = view_activated_.find(view_id);
  return it == view_activated_.end() ? true : it->second;
}

}  // namespace flutter

// ---------------------------------------------------------------------------
// Dart FFI entry points (`engine_id` unused: single-engine, multi-view).
// ---------------------------------------------------------------------------
extern "C" {

bool OHOS_WindowingSupported() {
  // Multi-window targets HarmonyOS PC (`2in1`) only; other form factors keep
  // the Android-style unsupported-platform contract (UnsupportedError).
  return std::string_view(OH_GetDeviceType()) == "2in1";
}

static flutter::OHOSWindowController* Controller(int64_t engine_id) {
  // Exact engine match first: when several main engines coexist, each
  // controller must answer only for the engine whose Dart isolate is calling
  // (the id Dart echoes back from PlatformDispatcher.engineId).
  if (auto* c = flutter::OHOSWindowController::ForEngineId(engine_id)) {
    return c;
  }
  // Engine not run yet, or a spawned engine without its own controller:
  // fall through to the historical single-engine contract below.
  flutter::OHOSWindowController* controller =
      flutter::OHOSWindowController::ActiveInstance();
  if (controller == nullptr) {
    FML_LOG(ERROR)
        << "Windowing FFI: no active window controller for engine_id "
        << engine_id
        << " (no engine attached, or more than one engine in this process "
           "and the id matches none of them). The operation is rejected.";
  }
  return controller;
}

// Window-property ops carry no engine_id — they carry the window HANDLE,
// which is only ever tracked by its owning controller.
static flutter::OHOSWindowController* ControllerForWindow(void* window) {
  if (auto* c = flutter::OHOSWindowController::ForWindowHandle(window)) {
    return c;
  }
  // Unknown handle (already closed / null): keep the single-engine no-op
  // semantics — the controller will LookupWindow-miss and ignore it.
  return flutter::OHOSWindowController::ActiveInstance();
}

// Validates the FFI-visible request pointer: the symbols carry default
// visibility, so an external (non-Dart) caller may pass null.
static bool ValidRequest(const flutter::FlutterWindowCreationRequest* request) {
  if (request == nullptr) {
    FML_LOG(WARNING)
        << "Windowing FFI: null creation request; the operation is rejected.";
    return false;
  }
  return true;
}

int64_t InternalFlutter_WindowController_CreateRegularWindow(
    int64_t engine_id,
    const flutter::FlutterWindowCreationRequest* request) {
  auto* c = Controller(engine_id);
  return c && ValidRequest(request)
             ? c->CreateRegularWindow(*request)
             : flutter::OHOSWindowController::kCreateWindowFailedViewId;
}

int64_t InternalFlutter_WindowController_CreateDialogWindow(
    int64_t engine_id,
    const flutter::FlutterWindowCreationRequest* request) {
  auto* c = Controller(engine_id);
  return c && ValidRequest(request)
             ? c->CreateDialogWindow(*request)
             : flutter::OHOSWindowController::kCreateWindowFailedViewId;
}

int64_t InternalFlutter_WindowController_CreateTooltipWindow(
    int64_t engine_id,
    const flutter::FlutterWindowCreationRequest* request) {
  auto* c = Controller(engine_id);
  return c && ValidRequest(request)
             ? c->CreateTooltipWindow(*request)
             : flutter::OHOSWindowController::kCreateWindowFailedViewId;
}

int64_t InternalFlutter_WindowController_CreatePopupWindow(
    int64_t engine_id,
    const flutter::FlutterWindowCreationRequest* request) {
  auto* c = Controller(engine_id);
  return c && ValidRequest(request)
             ? c->CreatePopupWindow(*request)
             : flutter::OHOSWindowController::kCreateWindowFailedViewId;
}

void InternalFlutter_Window_Destroy(int64_t engine_id, void* window) {
  if (auto* c = Controller(engine_id)) {
    c->DestroyWindow(window);
  }
}

void* InternalFlutter_Window_GetHandle(int64_t engine_id, int64_t view_id) {
  // A view id is only ever tracked by its owning controller — route by
  // view, not engine, so the answer is never another engine's handle.
  if (auto* c = flutter::OHOSWindowController::ForView(view_id)) {
    return c->GetHandleForView(view_id);
  }
  return nullptr;
}

flutter::FlutterWindowSize InternalFlutter_Window_GetContentSize(void* window) {
  if (auto* c = ControllerForWindow(window)) {
    return c->GetContentSize(window);
  }
  return flutter::FlutterWindowSize{0, 0};
}

void InternalFlutter_Window_SetContentSize(
    void* window,
    const flutter::FlutterWindowSize* size) {
  if (auto* c = ControllerForWindow(window); c && size != nullptr) {
    c->SetContentSize(window, *size);
  }
}

void InternalFlutter_Window_SetConstraints(
    void* window,
    const flutter::FlutterWindowConstraints* constraints) {
  if (auto* c = ControllerForWindow(window); c && constraints != nullptr) {
    c->SetConstraints(window, *constraints);
  }
}

void InternalFlutter_Window_SetTitle(void* window, const char* title) {
  if (auto* c = ControllerForWindow(window)) {
    c->SetTitle(window, title);
  }
}

void InternalFlutter_Window_Activate(void* window) {
  if (auto* c = ControllerForWindow(window)) {
    c->Activate(window);
  }
}

void InternalFlutter_Window_SetMaximized(void* window, bool maximized) {
  if (auto* c = ControllerForWindow(window)) {
    c->SetMaximized(window, maximized);
  }
}

void InternalFlutter_Window_SetMinimized(void* window, bool minimized) {
  if (auto* c = ControllerForWindow(window)) {
    c->SetMinimized(window, minimized);
  }
}

void InternalFlutter_Window_SetFullscreen(void* window, bool fullscreen) {
  if (auto* c = ControllerForWindow(window)) {
    c->SetFullscreen(window, fullscreen);
  }
}

bool InternalFlutter_Window_GetActivated(int64_t engine_id, int64_t view_id) {
  // A view id is only ever tracked by its owning controller — route by view.
  if (auto* c = flutter::OHOSWindowController::ForView(view_id)) {
    return c->GetViewActivated(view_id);
  }
  return false;
}

void InternalFlutter_Window_GetOffsetFromParent(
    void* window,
    flutter::FlutterWindowSize* out_offset_physical) {
  // Framework `Window.offsetFromParent` API surface kept for parity; without
  // sized-to-content parking the content always sits at the window origin.
  if (out_offset_physical == nullptr) {
    return;
  }
  *out_offset_physical = {0.0, 0.0};
}

void InternalFlutter_Window_GetTitle(void* window, char* out, int64_t capacity) {
  if (out == nullptr || capacity <= 0) {
    return;
  }
  out[0] = '\0';
  if (auto* c = flutter::OHOSWindowController::ForWindowHandle(window)) {
    c->GetTitle(window, out, capacity);
  }
}

}  // extern "C"
