/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_WINDOWING_OHOS_WINDOW_CONTROLLER_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_WINDOWING_OHOS_WINDOW_CONTROLLER_H_

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "flutter/common/constants.h"  // kFlutterImplicitViewId
#include "flutter/fml/macros.h"
#include "flutter/fml/memory/weak_ptr.h"
#include "flutter/lib/ui/window/viewport_metrics.h"
#include "flutter/shell/platform/ohos/platform_view_ohos.h"
#include "flutter/shell/platform/ohos/windowing/ohos_window.h"
#include "flutter/shell/platform/ohos/windowing/ohos_window_types.h"

namespace flutter {

class OHOSShellHolder;
class PlatformViewOHOSNapi;

/// Engine-side bookkeeping for single-engine, multi-view windowing on OHOS:
/// view-id allocation, the handle -> OHOSWindow map, and the napi/FFI
/// plumbing; per-window behavior lives in the OHOSWindow hierarchy.
///
/// Window creation registers the view with the engine synchronously on the
/// FFI/UI thread, so registration completes before the FFI returns; the OS
/// host (UIAbility or sub-window) attaches its surface later via
/// `PlatformViewOHOS::NotifyCreateForView`.
class OHOSWindowController {
 public:
  // Host-primitive mapping: Regular -> UIAbility; Dialog without parent
  // (modeless, task center) -> UIAbility; Dialog with parent (modal) and
  // Tooltip/Popup (auxiliary, parent-bound) -> SubWindow.
  static WindowHostKind ResolveHostKind(WindowType type,
                                        bool has_parent,
                                        int64_t parent_view_id);

  struct WindowHost {
    int64_t view_id;
    WindowHostKind kind;
    int64_t parent_view_id;
    // Opaque handle the embedding fills in to identify the host window in
    // later calls.
    void* host_handle;
  };

  OHOSWindowController(OHOSShellHolder* holder);

  ~OHOSWindowController();

  // The engine id this controller answers for in ForEngineId — stamped by
  // the holder where it builds its RunConfiguration (same id Dart echoes
  // back via PlatformDispatcher.engineId). 0 = engine not run yet.
  void SetEngineId(int64_t engine_id);

  // Create-window FFI failure sentinel. Deliberately NOT 0 (the implicit
  // view id); the Dart bindings throw on this value.
  static constexpr int64_t kCreateWindowFailedViewId = -1;

  // The napi layer's single active controller (no shell_holder handle
  // needed). Null before the first window, after teardown, or with more than
  // one engine (ambiguous — use ForView).
  static OHOSWindowController* ActiveInstance();

  // The controller owning `view_id` across all engines, so a view id is
  // never answered by another engine's controller. Null when untracked.
  static OHOSWindowController* ForView(int64_t view_id);

  // The controller of the engine the Dart caller lives in — resolved by the
  // engine id Dart echoes back through the FFI (holder-assigned, unique per
  // engine). First-choice routing when several main engines coexist; null
  // when no controller's engine id matches (engine not run yet / spawned
  // engine without a controller).
  static OHOSWindowController* ForEngineId(int64_t engine_id);

  // The controller currently tracking `window`'s handle, across all engines
  // (same multi-engine contract as ForView). Null when untracked — callers
  // fall back to ActiveInstance to preserve the single-engine no-op paths.
  static OHOSWindowController* ForWindowHandle(void* window);

  // ---- Engine-side (synchronous) view creation ---------------------------

  // Allocates a view id and registers it with the engine via AddView (returns
  // kCreateWindowFailedViewId if refused — no FlutterView would exist), then
  // asks the ArkTS side to create the OS window; its surface attaches
  // asynchronously via NotifyCreateForView.
  int64_t CreateRegularWindow(const FlutterWindowCreationRequest& request);
  int64_t CreateDialogWindow(const FlutterWindowCreationRequest& request);
  int64_t CreateTooltipWindow(const FlutterWindowCreationRequest& request);
  int64_t CreatePopupWindow(const FlutterWindowCreationRequest& request);

  // Tears down engine bookkeeping (`RemoveView`) for the view backing `window`
  // and asks the embedding to destroy the OS window.
  void DestroyWindow(void* window);

  void* GetHandleForView(int64_t view_id);

  // ---- Host-side (ArkTS) accessors --------------------------------------
  FlutterWindowSize GetContentSize(void* window) const;
  void SetContentSize(void* window, const FlutterWindowSize& size);
  void SetConstraints(void* window,
                      const FlutterWindowConstraints& constraints);
  void SetTitle(void* window, const char* title);
  // Copies the cached title (see OHOSWindow::SetTitleCache) into `out`,
  // truncating to `capacity - 1` bytes; always NUL-terminates. The window's
  // only title writer is SetTitle, so the cache IS the platform truth here.
  void GetTitle(void* window, char* out, int64_t capacity) const;
  void Activate(void* window);
  // Window-state mutations, dispatched to the ETS host via the napi facade
  // (`window` is reinterpret_cast<void*>(view_id)).
  void SetMaximized(void* window, bool maximized);
  void SetMinimized(void* window, bool minimized);
  void SetFullscreen(void* window, bool fullscreen);

  // Caches a view's actual content geometry pushed from the XComponent
  // surface callbacks (OHOS can't query a UIAbility window synchronously
  // from C++). Input is physical px; stores logical = physical / density.
  // Platform thread.
  void SetViewActualSize(int64_t view_id,
                         double physical_width,
                         double physical_height,
                         double density);

  // Fires the window's notify_listeners Dart callback (window-state changes
  // observed on the ArkTS side; entered from the napi
  // nativeNotifyWindowActivated bridge when focus changes).
  void NotifyListeners(void* window);

  // Focus tracking for the Dart `isActivated` query: the ArkTS side pushes
  // windowEvent/stageEvent ACTIVE↔INACTIVE here; a CHANGED value also fires
  // the window's notify_listeners chain so Dart dependents rebuild and
  // re-query.
  void SetViewActivated(int64_t view_id, bool activated);

  // Latest pushed activation; views never pushed (older stages without the
  // event) report ACTIVE, preserving the historical contract.
  bool GetViewActivated(int64_t view_id) const;

  // OS-initiated inverse of [DestroyWindow] (title-bar close, system
  // sub-window teardown): fires the Dart close callbacks and cleans up
  // bookkeeping but does NOT tear down the OS host (already being destroyed).
  // No-op if already erased by a Dart-initiated DestroyWindow.
  void HandleOsWindowClosed(int64_t view_id);

  // Hot restart: the isolate owning the windows' Dart callbacks is being
  // replaced. Drops all entries WITHOUT firing callbacks (the old isolate
  // must not run app logic mid-restart), removes the non-implicit views from
  // the engine and tears down their OS hosts (RuntimeController::Clone would
  // otherwise resurrect them in the new isolate as controller-less ghost
  // views), and resets implicit-view adoption so the restarted isolate
  // re-adopts view 0. Platform thread, isolate quiesced.
  void OnPreEngineRestart();

  // Desired position for a positioner-anchored sub-window, via the
  // on_get_window_position Dart callback. The ETS host calls this after
  // createSubWindow resolves the parent geometry; all units are LOGICAL px.
  // False = keep default placement (unknown view / no positioner callback).
  bool ComputeWindowPosition(int64_t view_id,
                             const FlutterWindowSize& child_size,
                             const FlutterWindowRect& parent_rect,
                             const FlutterWindowRect& work_area,
                             FlutterWindowRect* out);

  // ---- Accessors used by the OHOSWindow hierarchy -------------------------

  // The napi facade of the owning shell holder (null during teardown).
  std::shared_ptr<PlatformViewOHOSNapi> GetNapiFacade();

 private:
  // Shared body of the Create* variants: resolve the host kind, allocate and
  // register a view, request the host.
  int64_t CreateWindow(const FlutterWindowCreationRequest& request,
                       WindowType type);

  std::unique_ptr<OHOSWindow> CreateWindowObject(
      const FlutterWindowCreationRequest& request,
      const OHOSWindow::InitParams& params);

  // Allocates the next non-implicit view id (never collides with implicit 0).
  int64_t AllocateViewId();

 public:
  // The Dart-facing handle encoding: view id biased by +1 so view 0's handle
  // is a real pointer value, never the nullptr "not found" sentinel.
  static void* HandleForViewId(int64_t view_id);
  static int64_t ViewIdForHandle(void* handle);

 private:

  // Registers the view with the engine synchronously inline on the FFI/UI
  // thread: the Dart add_view_ callback runs inline, so the FlutterView is in
  // PlatformDispatcher.views before this returns. False = engine refused;
  // treat the view id as never-registered.
  bool AddViewSync(int64_t view_id);

  // Posts `Shell::OnPlatformViewRemoveView` to the platform task runner.
  void RemoveViewOnPlatformThread(int64_t view_id);

  OHOSWindow* LookupWindow(void* window);
  const OHOSWindow* LookupWindow(void* window) const;

  OHOSShellHolder* const holder_;
  // Engine id routed by ForEngineId; 0 until the holder's Launch runs.
  int64_t engine_id_ = 0;
  // True once the first kUiAbility window adopted implicit view 0 (the
  // EntryAbility's pre-created main window); later ones spawn their own
  // UIAbility.
  bool entry_ability_bound_ = false;
  // Live windows keyed by host handle (HandleForViewId of the view id).
  // Cross-thread when platform/UI threads are not merged: UI/FFI thread
  // (create/destroy) and the ArkTS/napi thread (HandleOsWindowClosed) — every
  // access takes `windows_mutex_`. Dart-re-entering callbacks (FireWillClose)
  // run AFTER the entry has been moved out to a sole-owner local
  // unique_ptr, so a concurrent or re-entrant destroy can never free the
  // object whose callbacks are still on another thread's stack.
  mutable std::mutex windows_mutex_;
  std::map<void*, std::unique_ptr<OHOSWindow>> windows_;

  // Process-wide monotonic view-id source: several main engines can hold
  // windows concurrently (multi-engine FFI routing), and view ids — hence
  // Dart-facing handles (id+1) — must NEVER collide across controllers, or
  // ForWindowHandle/ForView could answer with another engine's window.
  static std::atomic<int64_t> g_next_view_id_;

  // Actual per-view geometry in LOGICAL px, pushed from the surface callbacks
  // (platform thread), read cross-thread; mutex-guarded. NEVER erased (view
  // ids are never reused).
  mutable std::mutex actual_sizes_mutex_;
  std::unordered_map<int64_t, FlutterWindowSize> actual_sizes_;

  // Latest ArkTS-pushed focus flag per view (SetViewActivated); absent =
  // never pushed → GetViewActivated answers true (historical contract).
  mutable std::mutex view_activated_mutex_;
  std::unordered_map<int64_t, bool> view_activated_;

  FML_DISALLOW_COPY_AND_ASSIGN(OHOSWindowController);
};

}  // namespace flutter

// ---- Dart FFI entry points -----------------------------------------------
// Resolved by `lookupFunction` in `_window_ohos.dart`. `engine_id` is the
// holder-assigned id Dart's PlatformDispatcher.engineId reports back; the
// create/destroy ops route to the calling engine's controller via
// ForEngineId, window-property ops via ForWindowHandle — several main
// engines can hold windows concurrently. Window handles remain unique
// process-wide (view-id-biased per controller; ids allocated per controller
// can collide across engines, so NEVER compare handles across engines).
extern "C" {

// Default visibility for the FFI symbols (the engine is built with
// -fvisibility=hidden); inherited by the definitions in the .cpp.
#define FLUTTER_OHOS_WINDOWING_API __attribute__((visibility("default")))

// Whether multi-window is supported: HarmonyOS PC (`2in1`) only. Other form
// factors report false so Dart keeps the unsupported-platform contract
// (UnsupportedError), matching Android.
FLUTTER_OHOS_WINDOWING_API
bool OHOS_WindowingSupported();

FLUTTER_OHOS_WINDOWING_API
int64_t InternalFlutter_WindowController_CreateRegularWindow(
    int64_t engine_id,
    const flutter::FlutterWindowCreationRequest* request);

FLUTTER_OHOS_WINDOWING_API
int64_t InternalFlutter_WindowController_CreateDialogWindow(
    int64_t engine_id,
    const flutter::FlutterWindowCreationRequest* request);

FLUTTER_OHOS_WINDOWING_API
int64_t InternalFlutter_WindowController_CreateTooltipWindow(
    int64_t engine_id,
    const flutter::FlutterWindowCreationRequest* request);

FLUTTER_OHOS_WINDOWING_API
int64_t InternalFlutter_WindowController_CreatePopupWindow(
    int64_t engine_id,
    const flutter::FlutterWindowCreationRequest* request);

FLUTTER_OHOS_WINDOWING_API
void InternalFlutter_Window_Destroy(int64_t engine_id, void* window);

FLUTTER_OHOS_WINDOWING_API
void* InternalFlutter_Window_GetHandle(int64_t engine_id, int64_t view_id);

FLUTTER_OHOS_WINDOWING_API
flutter::FlutterWindowSize InternalFlutter_Window_GetContentSize(void* window);

FLUTTER_OHOS_WINDOWING_API
void InternalFlutter_Window_SetContentSize(
    void* window,
    const flutter::FlutterWindowSize* size);

FLUTTER_OHOS_WINDOWING_API
void InternalFlutter_Window_SetConstraints(
    void* window,
    const flutter::FlutterWindowConstraints* constraints);

FLUTTER_OHOS_WINDOWING_API
void InternalFlutter_Window_SetTitle(void* window, const char* title);

FLUTTER_OHOS_WINDOWING_API
void InternalFlutter_Window_Activate(void* window);

FLUTTER_OHOS_WINDOWING_API
void InternalFlutter_Window_SetMaximized(void* window, bool maximized);

FLUTTER_OHOS_WINDOWING_API
void InternalFlutter_Window_SetMinimized(void* window, bool minimized);

FLUTTER_OHOS_WINDOWING_API
void InternalFlutter_Window_SetFullscreen(void* window, bool fullscreen);

// Latest ArkTS-pushed focus flag for the view; unknown views answer false.
FLUTTER_OHOS_WINDOWING_API
bool InternalFlutter_Window_GetActivated(int64_t engine_id, int64_t view_id);

// Anchored (tooltip/popup) content offset inside its host window, PHYSICAL
// px; writes {0,0} when the window has no offset (not anchored / none yet).
FLUTTER_OHOS_WINDOWING_API
void InternalFlutter_Window_GetOffsetFromParent(
    void* window,
    flutter::FlutterWindowSize* out_offset_physical);

// Cached window title (the SetTitle echo; see OHOSWindow::SetTitleCache).
// Writes at most `capacity - 1` bytes + NUL into `out`; empty string when
// the window has no title or no title was ever set.
FLUTTER_OHOS_WINDOWING_API
void InternalFlutter_Window_GetTitle(void* window, char* out, int64_t capacity);

}  // extern "C"

#endif  // FLUTTER_SHELL_PLATFORM_OHOS_WINDOWING_OHOS_WINDOW_CONTROLLER_H_
