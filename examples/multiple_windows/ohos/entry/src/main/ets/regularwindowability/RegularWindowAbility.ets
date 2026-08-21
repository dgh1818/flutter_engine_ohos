/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import AbilityConstant from '@ohos.app.ability.AbilityConstant';
import Want from '@ohos.app.ability.Want';
import window from '@ohos.window';
import display from '@ohos.display';
import {
  FlutterAbility,
  FlutterAbilityLaunchConfigs,
  FlutterEngineCache,
  FlutterManager,
  FlutterView,
  FlutterNapi,
  Log,
} from '@ohos/flutter_ohos';

const TAG = 'RegularWindowAbility';

/**
 * Secondary Regular window hosted as a separate UIAbility instance so it gets
 * its own task-center card. Reuses EntryAbility's cached engine (single
 * engine, multi-view; all UIAbility instances of a HAP share one process).
 */
export default class RegularWindowAbility extends FlutterAbility {
  private regularFlutterView: FlutterView | null = null;
  private flutterViewNumericId: number = 0;
  private requestId: number = 0;
  private preferredWidth: number = 0;
  private preferredHeight: number = 0;
  private windowTitle: string = '';
  /** Whether onForeground has fired (window-stage creation may race it). */
  private isForeground = false;

  // ── Engine lifecycle overrides: reuse, do NOT own the engine ──────────────

  getCachedEngineId(): string {
    return FlutterAbilityLaunchConfigs.MULTI_WINDOW_ENGINE_CACHE_ID;
  }

  shouldDestroyEngineWithHost(): boolean {
    return false;
  }

  shouldDispatchAppLifecycleState(): boolean {
    return false;
  }

  shouldAttachEngineToAbility(): boolean {
    return false;
  }

  pagePath(): string {
    return 'pages/FlutterWindowPage';
  }

  isDefaultFullScreen(): boolean {
    return false;
  }

  // ── Lifecycle ─────────────────────────────────────────────────────────────

  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    // View-id + geometry passed by FlutterManager.startAbility.
    const params = want.parameters as Record<string, Object>;
    const num = (k: string): number => {
      const v = params?.[k];
      return v != null ? Number(v) : 0;
    };
    this.flutterViewNumericId =
      num(FlutterAbilityLaunchConfigs.EXTRA_FLUTTER_VIEW_ID);
    this.requestId =
      num(FlutterAbilityLaunchConfigs.EXTRA_FLUTTER_WINDOW_REQUEST_ID);
    this.preferredWidth =
      num(FlutterAbilityLaunchConfigs.EXTRA_FLUTTER_PREFERRED_WIDTH);
    this.preferredHeight =
      num(FlutterAbilityLaunchConfigs.EXTRA_FLUTTER_PREFERRED_HEIGHT);
    const titleVal = params?.[FlutterAbilityLaunchConfigs.EXTRA_FLUTTER_WINDOW_TITLE];
    this.windowTitle = titleVal != null ? String(titleVal) : '';

    super.onCreate(want, launchParam);
  }

  /**
   * The base onWindowStageCreate creates the implicit main view — wrong for a
   * secondary window. Instead: create a FlutterView keyed by the decimal viewId
   * (same scheme as createSubWindow so C++ IsSubViewId routes it), attach it to
   * the shared engine, and load the view-host page with viewId in LocalStorage.
   */
  onWindowStageCreate(windowStage: window.WindowStage): void {
    FlutterManager.getInstance().pushWindowStage(this, windowStage);
    this.getFlutterAbilityAndEntryDelegate()?.initWindow();
    try {
      windowStage.on('windowStageEvent', this.windowStageEventCallback);

      const viewIdStr = String(this.flutterViewNumericId);

      let fv = FlutterManager.getInstance().getFlutterView(viewIdStr);
      if (!fv) {
        fv = new FlutterView(viewIdStr, this.context);
        FlutterManager.getInstance().registerFlutterView(viewIdStr, fv);
      }
      this.regularFlutterView = fv;
      // The base class registers only the implicit main view; secondary
      // windows must register themselves. FlutterManager resolves both
      // sub-window parents (viewToAbility) and cascadeCloseChildren's
      // terminateSelf through it — unregistered children linger as blank
      // orphans.
      FlutterManager.getInstance().registerViewAbility(viewIdStr, this);

      // Attach to the same engine as the main view — safe: state is per-view,
      // napi commands are view-id-keyed.
      const engine = this.getFlutterEngine();
      if (engine) {
        try {
          fv.attachToFlutterEngine(engine);
          // If onForeground raced ahead of stage creation, catch up the
          // visibility report; idempotent if onForeground runs later.
          if (this.isForeground) {
            fv.onPageVisibilityChanged(true);
          }
        } catch (e) {
          Log.w(TAG, `attachToFlutterEngine failed: ${JSON.stringify(e)}`);
        }
      } else {
        Log.e(TAG, `no cached engine for view ${viewIdStr}; rendering will not happen`);
      }

      const main = windowStage.getMainWindowSync();
      // resize() takes PHYSICAL px while preferredSize is logical — scale by
      // the display DPR. The resize animates visibly and cannot be disabled.
      let dpr = 1.0;
      try { dpr = display.getDefaultDisplaySync().densityPixels || 1.0; } catch (_) { /* keep 1.0 */ }
      if (this.preferredWidth > 0 && this.preferredHeight > 0) {
        try {
          // Promise-returning: the catch only sees synchronous throws —
          // rejections need .catch or they escape as unhandled.
          main.resize(Math.round(this.preferredWidth * dpr), Math.round(this.preferredHeight * dpr))
            .catch((e: Object) => {
              Log.w(TAG, `resize preferred size rejected: ${JSON.stringify(e)}`);
            });
        } catch (e) {
          Log.w(TAG, `resize preferred size failed: ${JSON.stringify(e)}`);
        }
      }
      if (this.windowTitle.length > 0) {
        try {
          main.setWindowTitle(this.windowTitle)
            .catch((e: Object) => {
              Log.w(TAG, `setWindowTitle rejected: ${JSON.stringify(e)}`);
            });
        } catch (e) {
          Log.w(TAG, `setWindowTitle failed: ${JSON.stringify(e)}`);
        }
      }
      // viewId in LocalStorage lets FlutterPage resolve this view.
      const storage: LocalStorage = new LocalStorage();
      storage.setOrCreate('viewId', viewIdStr);
      windowStage.loadContent(this.pagePath(), storage, (err, _data) => {
        if (err && err.code) {
          Log.e(TAG, `loadContent failed: ${JSON.stringify(err)}`);
          this.failLoadContent();
          return;
        }
        this.regularFlutterView?.onWindowCreated();

        // resize() sizes the OUTER window, so the floating-window decoration
        // eats into the drawable content. Compensate by adding the measured
        // inset (windowRect − drawableRect) back so the content area equals
        // preferredSize×DPR, like win32 GetWindowSizeForClientSize; no-ops
        // without inset.
        try {
          const props = main.getWindowProperties();
          const outer = props.windowRect;
          const drawable = props.drawableRect;
          const decorW = Math.max(0, Math.round(outer.width - drawable.width));
          const decorH = Math.max(0, Math.round(outer.height - drawable.height));
          const targetW = Math.round(this.preferredWidth * dpr) + decorW;
          const targetH = Math.round(this.preferredHeight * dpr) + decorH;
          if ((decorW > 0 || decorH > 0) && this.preferredWidth > 0 && this.preferredHeight > 0) {
            main.resize(targetW, targetH).catch((e: Object) => {
              Log.w(TAG, `DECOR compensate resize rejected: ${JSON.stringify(e)}`);
            });
          }
        } catch (e) {
          Log.w(TAG, `DECOR compensate failed: ${JSON.stringify(e)}`);
        }
      });
    } catch (exception) {
      Log.e(TAG, `onWindowStageCreate failed: ${JSON.stringify(exception)}`);
    }
  }

  /**
   * A Regular window must count toward the engine's visibleHostCount, or
   * hiding EntryAbility alone would pause the engine and freeze this
   * still-visible window (delegate onShow/onHide is gated off here).
   */
  onForeground() {
    this.isForeground = true;
    super.onForeground();
    this.regularFlutterView?.onPageVisibilityChanged(true);
  }

  onBackground() {
    this.isForeground = false;
    this.regularFlutterView?.onPageVisibilityChanged(false);
    super.onBackground();
  }

  // loadContent failed — this window can never render; tear everything down
  // so no blank window lingers.
  private failLoadContent(): void {
    const viewIdStr = String(this.flutterViewNumericId);
    try {
      FlutterNapi.nativeHandleOsWindowClosed(this.flutterViewNumericId);
    } catch (e) {
      Log.w(TAG, `failLoadContent: nativeHandleOsWindowClosed failed: ${JSON.stringify(e)}`);
    }
    FlutterManager.getInstance().deleteViewAbility(viewIdStr);
    const fv = this.regularFlutterView;
    if (fv) {
      FlutterManager.getInstance().deleteFlutterView(viewIdStr, fv);
    }
    this.context.terminateSelf().catch((e: Error) => {
      Log.w(TAG, `failLoadContent: terminateSelf failed: ${JSON.stringify(e)}`);
    });
  }
}
