/*
 * Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd.
 * All rights reserved. Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE_KHZG file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_NAPI_PLATFORM_VIEW_OHOS_NAPI_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_NAPI_PLATFORM_VIEW_OHOS_NAPI_H_
#include <memory>
#include <mutex>
#include <vector>
#include "flutter/fml/file.h"
#include "flutter/fml/mapping.h"
#include "flutter/fml/platform/ohos/dynamic_library_loader.h"
#include "flutter/fml/task_runner.h"
#include "flutter/lib/ui/window/platform_message.h"
#include "napi/native_api.h"

// class for all c++ to call js function
namespace flutter {

struct locale {
  std::string language;
  std::string script;
  std::string region;
};

struct mouseWheelEvent {
  std::string eventType;
  int64_t shellHolder;
  int64_t fingerId;
  double globalX;
  double globalY;
  double offsetY;
  int64_t timestamp;
};

class PlatformViewOHOSNapi {
 public:
  static napi_value nativeDispatchEmptyPlatformMessage(
      napi_env env,
      napi_callback_info info);  // 发送一个空的PlatformMessage
  static napi_value nativeDispatchPlatformMessage(
      napi_env env,
      napi_callback_info info);  // 发送一个PlatformMessage
  static napi_value nativeInvokePlatformMessageEmptyResponseCallback(
      napi_env env,
      napi_callback_info info);  // 空的PlatformMessage响应回调
  static napi_value nativeInvokePlatformMessageResponseCallback(
      napi_env env,
      napi_callback_info info);

  explicit PlatformViewOHOSNapi(napi_env env);
  ~PlatformViewOHOSNapi();
  void SetPlatformTaskRunner(fml::RefPtr<fml::TaskRunner> platform_task_runner);
  void FlutterViewHandlePlatformMessageResponse(
      int reponse_id,
      std::unique_ptr<fml::Mapping> data);
  void FlutterViewHandlePlatformMessage(
      int reponse_id,
      std::unique_ptr<flutter::PlatformMessage> message);

  void FlutterViewOnFirstFrame(bool is_preload = false);
  void FlutterViewOnPreEngineRestart();

  void OnDisplayPlatformViewHybrid(int64_t view_id,
                              double x,
                              double y,
                              double width,
                              double height,
                              double view_width,
                              double view_height);

  void OnDisplayOverlayHybrid(int64_t view_id,
                         double x,
                         double y,
                         double width,
                         double height);

  void OnDisplayMutatorsHybrid(int64_t view_id, const std::vector<double>& data);

  void HidePlatformViewHybrid(int64_t view_id);

  void ShowOverlaySurfaceHybrid();

  void HideOverlaySurfaceHybrid();

  void OnBeginFrameHybrid();

  void OnEndFrameHybrid();

  // Multi-window: ask the ETS host to create/destroy a sub-window for a
  // non-implicit view. width/height are LOGICAL px.
  void RequestWindowHost(int64_t view_id,
                         int64_t parent_view_id,
                         double width,
                         double height,
                         int32_t archetype);
  // Regular window → real UIAbility host (startAbility-launched, cached
  // engine); `title` is currently unused on this path.
  void CreateRegularAbility(int64_t view_id,
                            int64_t request_id,
                            double width,
                            double height,
                            const std::string& title);
  // First Regular window → rebind the EntryAbility's main window surface to
  // `view_id` instead of spawning a sibling RegularWindowAbility.
  void BindEntryAbilityToView(int64_t view_id,
                              double width,
                              double height,
                              const std::string& title);
  void DestroyWindowHost(int64_t view_id);
  // View 0 → exit via ArkTS `exitApplication` (terminateSelf, full Ability
  // teardown — NOT a raw exit()); HandleWillClose fires first.
  void ExitApplication();

  // Runtime window-property mutations; width/height are LOGICAL px.
  void SetWindowSize(int64_t view_id, double width, double height);
  void SetWindowTitle(int64_t view_id, const std::string& title);
  void SetWindowMaximized(int64_t view_id, bool maximized);
  void SetWindowMinimized(int64_t view_id, bool minimized);
  void SetWindowFullscreen(int64_t view_id, bool fullscreen);
  // LOGICAL-px min/max (max 0 == unbounded); forwards to the ETS host so the
  // WM re-clamps live resizes, and mirrors the request copy used at birth.
  void SetWindowConstraints(int64_t view_id,
                            double min_width,
                            double max_width,
                            double min_height,
                            double max_height);
  // Brings the window to front (ETS focusWindow/activate path).
  void ActivateWindow(int64_t view_id);

  flutter::locale resolveNativeLocale(
      std::vector<flutter::locale> supportedLocales);
  std::unique_ptr<std::vector<std::string>>
  FlutterViewComputePlatformResolvedLocales(
      const std::vector<std::string>& support_locale_data);

  // Notify the ETS layer of application locale change.
  void FlutterViewSetApplicationLocale(std::string locale);

  void FlutterViewOnTouchEvent(std::shared_ptr<std::string[]> touchPacketString,
                               int size);

  void FlutterViewOnMouseEvent(
      const std::shared_ptr<std::string[]>& mousePacketString,
      const int& size);
  void FlutterViewOnAxisEvent(
      const std::shared_ptr<std::string[]>& axisPacketString,
      const int& size);
  /**
   * accessibility-relevant interfaces
   */
  void SetSemanticsEnabled(int64_t shell_hoder, bool enabled);
  void SetAccessibilityFeatures(int64_t shell_hoder, int32_t flags);

  static napi_value nativeUpdateRefreshRate(
      napi_env env,
      napi_callback_info info);  // 设置刷新率
  static napi_value nativeUpdateSize(napi_env env,
                                     napi_callback_info info);  // 设置屏幕尺寸
  static napi_value nativeUpdateDensity(
      napi_env env,
      napi_callback_info info);  // 设置屏幕像素密度（也就是缩放系数）
  static napi_value nativeRunBundleAndSnapshotFromLibrary(
      napi_env env,
      napi_callback_info info);  // 加载dart工程构建产物
  static napi_value nativePrefetchDefaultFontManager(
      napi_env env,
      napi_callback_info
          info);  // 初始化SkFontMgr::RefDefault(),skia引擎文字管理初始化
  static napi_value nativeCheckAndReloadFont(
      napi_env env,
      napi_callback_info info);  // hot reload font
  static napi_value nativeGetIsSoftwareRenderingEnabled(
      napi_env env,
      napi_callback_info info);  // 返回是否支持软件绘制

  static napi_value nativeAttach(
      napi_env env,
      napi_callback_info
          info);  // attach flutterNapi实例给到 native
                  // engine，这个支持rkts到flutter平台的无关引擎之间的通信
                  // attach只需要执行一次
  static napi_value nativeSpawn(
      napi_env env,
      napi_callback_info info);  // 从当前的flutterNapi复制一个新的实例
  static napi_value nativeDestroy(
      napi_env env,
      napi_callback_info info);  // Detaches flutterNapi和engine之间的关联

  static napi_value nativeSpawnAsync(napi_env env, napi_callback_info info);
  static napi_value nativeDestroyAsync(napi_env env, napi_callback_info info);

  static napi_value nativeSetViewportMetrics(
      napi_env env,
      napi_callback_info info);  // 把物理屏幕参数通知到native
  static napi_value nativeSetAccessibilityFeatures(
      napi_env env,
      napi_callback_info info);  // 设置能力参数

  static napi_value nativeCleanupMessageData(
      napi_env env,
      napi_callback_info info);  // 清除某个messageData
  static napi_value nativeLoadDartDeferredLibrary(
      napi_env env,
      napi_callback_info info);  // load一个合法的.so文件到dart vm
  static napi_value nativeUpdateOhosAssetManager(
      napi_env env,
      napi_callback_info info);  // 设置ResourceManager和assetBundlePath到engine
  static napi_value nativeDeferredComponentInstallFailure(
      napi_env env,
      napi_callback_info info);  // 加载动态库，或者dart库失败时的通知
  static napi_value nativeGetPixelMap(
      napi_env env,
      napi_callback_info info);  // 从engine获取当前绘制pixelMap
  static napi_value nativeNotifyLowMemoryWarning(
      napi_env env,
      napi_callback_info info);  // 应用低内存警告

  // 下面的方法，从键盘输入中判断当前字符是否是emoji，实现优先级低
  static napi_value nativeFlutterTextUtilsIsEmoji(napi_env env,
                                                  napi_callback_info info);
  static napi_value nativeFlutterTextUtilsIsEmojiModifier(
      napi_env env,
      napi_callback_info info);
  static napi_value nativeFlutterTextUtilsIsEmojiModifierBase(
      napi_env env,
      napi_callback_info info);
  static napi_value nativeFlutterTextUtilsIsVariationSelector(
      napi_env env,
      napi_callback_info info);
  static napi_value nativeFlutterTextUtilsIsRegionalIndicator(
      napi_env env,
      napi_callback_info info);
  static napi_value nativeGetSystemLanguages(
      napi_env env,
      napi_callback_info info);  // 应用下发系统语言设置

  static napi_value nativeUnregisterTexture(napi_env env,
                                            napi_callback_info info);

  static napi_value nativeMarkTextureFrameAvailable(napi_env env,
                                                    napi_callback_info info);

  static napi_value nativeRegisterPixelMap(napi_env env,
                                           napi_callback_info info);

  static napi_value nativeRegisterTexture(napi_env env,
                                          napi_callback_info info);

  static napi_value nativeGetTextureWindowId(napi_env env,
                                             napi_callback_info info);
  static napi_value nativeGetTextureWindowPtr(napi_env env,
                                              napi_callback_info info);

  static napi_value nativeSetTextureBackGroundPixelMap(napi_env env,
                                                       napi_callback_info info);
  static napi_value nativeSetTextureBackGroundColor(napi_env env,
                                                    napi_callback_info info);

  static napi_value nativeSetTextureBufferSize(napi_env env,
                                               napi_callback_info info);

  static napi_value nativeNotifyTextureResizing(napi_env env,
                                                napi_callback_info info);

  static napi_value nativeSetExternalNativeImage(napi_env env,
                                                 napi_callback_info info);

  static napi_value nativeSetExternalNativeImagePtr(napi_env env,
                                                    napi_callback_info info);

  static napi_value nativeResetExternalTexture(napi_env env,
                                               napi_callback_info info);

  static napi_value nativeEnableFrameCache(napi_env env,
                                           napi_callback_info info);

  static napi_value nativeSetPipVisible(napi_env env, napi_callback_info info);

  // HCPP: returns whether Hybrid Composition is enabled for the engine.
  static napi_value nativeIsHybridCompositionEnabled(napi_env env,
                                                     napi_callback_info info);

  // HCPP route D: injects a DISPLAY system layer's touch event into the
  // engine as a PointerDataPacket, bypassing the (broken) main XComponent
  // hit path so the Dart gesture arena can arbitrate. touchData is an array
  // of objects whose fields mirror HandleTouchEvent's PointerData layout.
  static napi_value nativeDispatchTouchToEngine(napi_env env,
                                                napi_callback_info info);

  // Surface相关，XComponent调用
  static void SurfaceCreated(int64_t shell_holder,
                             void* window,
                             int width,
                             int height);

  static void SurfacePreload(int64_t shell_holder, int width, int height);

  static void SurfaceChanged(int64_t shell_holder,
                             void* window,
                             int width,
                             int height);

  static void SurfaceDestroyed(int64_t shell_holder);

  // Multi-view counterparts of the three Surface* entries above, for a
  // non-implicit view (per-view swapchain/GPUSurface).
  static void NotifyCreateForView(int64_t shell_holder,
                                  int64_t view_id,
                                  void* window,
                                  int width,
                                  int height);

  static void NotifyDestroyForView(int64_t shell_holder, int64_t view_id);

  static void NotifySurfaceChangedForView(int64_t shell_holder,
                                          int64_t view_id,
                                          void* window,
                                          int width,
                                          int height);

  static napi_value nativeXComponentAttachFlutterEngine(
      napi_env env,
      napi_callback_info info);
  static napi_value nativeXComponentDetachFlutterEngine(
      napi_env env,
      napi_callback_info info);
  static napi_value nativeXComponentPreDraw(napi_env env,
                                            napi_callback_info info);

  static int64_t display_width;
  static int64_t display_height;
  static int32_t display_refresh_rate;
  static std::shared_ptr<std::set<int>> all_refresh_rates;
  static double display_density_pixels;
  static napi_value nativeXComponentDispatchMouseWheel(napi_env env,
                                                       napi_callback_info info);
  static napi_value nativeEncodeUtf8(napi_env env, napi_callback_info info);
  static napi_value nativeDecodeUtf8(napi_env env, napi_callback_info info);
  static napi_value nativeLookupCallbackInformation(napi_env env,
                                                    napi_callback_info info);
  static napi_value nativeLookupCallbackInformationBigInt(
      napi_env env,
      napi_callback_info info);
  // ETS -> C++: the OS closed a host window; fires the Dart
  // onWindowDestroyed teardown. Arg: view_id (int64).
  static napi_value nativeHandleOsWindowClosed(napi_env env,
                                               napi_callback_info info);
  // ETS -> C++: computes a positioner-anchored sub-window position via the
  // Dart on_get_window_position callback; returns 0 on success, nonzero =
  // default.
  static napi_value nativeComputeWindowPosition(napi_env env,
                                                napi_callback_info info);

  // ETS -> C++: window focus changed (windowEvent/stageEvent ACTIVE↔
  // INACTIVE). Args: view_id (int64), activated (bool). Caches the flag for
  // the Dart `isActivated` query and pings notify_listeners on change.
  static napi_value nativeNotifyWindowActivated(napi_env env,
                                                napi_callback_info info);

  static napi_value nativeUnicodeIsEmoji(napi_env env, napi_callback_info info);

  static napi_value nativeUnicodeIsEmojiModifier(napi_env env,
                                                 napi_callback_info info);

  static napi_value nativeUnicodeIsEmojiModifierBase(napi_env env,
                                                     napi_callback_info info);

  static napi_value nativeUnicodeIsVariationSelector(napi_env env,
                                                     napi_callback_info info);

  static napi_value nativeUnicodeIsRegionalIndicatorSymbol(
      napi_env env,
      napi_callback_info info);

  /**
   * ets call c++
   */
  static napi_value nativeAccessibilityStateChange(napi_env env,
                                                   napi_callback_info info);
  static napi_value nativeAccessibilityAnnounce(napi_env env,
                                                napi_callback_info info);
  static napi_value nativeAccessibilityOnTap(napi_env env,
                                             napi_callback_info info);
  static napi_value nativeAccessibilityOnLongPress(napi_env env,
                                                   napi_callback_info info);
  static napi_value nativeAccessibilityOnTooltip(napi_env env,
                                                 napi_callback_info info);
  static napi_value nativeSetSemanticsEnabled(napi_env env,
                                              napi_callback_info info);
  static napi_value nativeSetFlutterNavigationAction(napi_env env,
                                                     napi_callback_info info);

  static napi_value nativeSetFontWeightScale(napi_env env,
                                             napi_callback_info info);

  static napi_value nativeUpdateCurrentXComponentId(napi_env env,
                                                    napi_callback_info info);
  static napi_value nativeSetDVsyncSwitch(napi_env env,
                                          napi_callback_info info);

  static napi_value nativeAnimationVoting(napi_env env,
                                          napi_callback_info info);

  static napi_value nativeVideoVoting(napi_env env, napi_callback_info info);

  static napi_value nativePrefetchFramesCfg(napi_env env,
                                            napi_callback_info info);

  static napi_value nativeCheckLTPOSwitchState(napi_env env,
                                               napi_callback_info info);
  static napi_value nativeSetQosOnLowMemory(napi_env env,
                                            napi_callback_info info);
  static napi_value nativeSetAnimationStatus(napi_env env,
                                             napi_callback_info info);
  static napi_value nativeNotifyPageChanged(napi_env env,
                                            napi_callback_info info);

  static napi_value nativeLTPODispatchHighFrameRate(napi_env env,
                                                    napi_callback_info info);

 private:
  static napi_env env_;
  napi_ref ref_napi_obj_ = nullptr;
  static std::vector<std::string> system_languages;
  fml::RefPtr<fml::TaskRunner> platform_task_runner_;
  static int64_t napi_shell_holder_id_;
  // Dynamic library loader for
  // OH_AbilityRuntime_ApplicationContextNotifyPageChanged
  static std::once_flag notify_page_changed_init_flag_;
  static std::unique_ptr<DynamicLibraryLoader> ability_runtime_loader_;
  using NotifyPageChangedFunc = int32_t (*)(const char*, int32_t, int32_t);
  static NotifyPageChangedFunc notify_page_changed_func_;
  static void InitNotifyPageChangedLoader();
};

}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_NAPI_PLATFORM_VIEW_OHOS_NAPI_H_
