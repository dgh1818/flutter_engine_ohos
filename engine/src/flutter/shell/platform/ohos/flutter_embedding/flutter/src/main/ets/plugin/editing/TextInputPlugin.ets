/*
* Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd. All rights reserved.
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE_KHZG file.
*
* Based on TextInputPlugin.java originally written by
* Copyright (C) 2013 The Flutter Authors.
*
*/

import TextInputChannel, {
  Configuration,
  TextEditState,
  TextInputMethodHandler
} from '../../embedding/engine/systemchannels/TextInputChannel';
import inputMethod from '@ohos.inputMethod';
import Log from '../../util/Log';
import { EditingStateWatcher, ListenableEditingState, PreviewInsertOptions } from './ListenableEditingState';
import Any from '../common/Any';
import { inputDevice } from '@kit.InputKit';
import { BusinessError } from '@kit.BasicServicesKit';
import { PointerDeviceKind } from '../../embedding/ohos/OhosTouchProcessor';
import deviceInfo from '@ohos.deviceInfo';
import KeyEventChannel from '../../embedding/engine/systemchannels/KeyEventChannel';
import { appManager, autoFillManager, bundleManager } from '@kit.AbilityKit';
import { PreviewTextCoordinator } from './PreviewTextCoordinator';
import { isPreviewTextSupported } from './PreviewTextSupport';
import ArrayList from '@ohos.util.ArrayList';
import { TextEditingDelta } from './TextEditingDelta';
import {
  AutoFillFieldRect,
  AutoFillHostInfo,
  AUTOFILL_SUPPORT_API,
  OhosAutoFillHelper,
  OhosViewData,
} from './OhosAutoFillHelper';

const INPUT_SUPPORT_API = 15;

/** Document half-open [rangeStart, rangeEnd) for Latin finishTextPreview commit (ArkTS: no inline object types). */
interface LatinFinishPreviewRange {
  rangeStart: number;
  rangeEnd: number;
  mode: string;
}

/**
 * Plugin for handling text input in Flutter applications.
 * This class manages the interaction between Flutter's text input system
 * and the OpenHarmony input method framework.
 */
export default class TextInputPlugin implements EditingStateWatcher {
  private static TAG = "TextInputPlugin";
  private textInputChannel: TextInputChannel;
  private keyEventChannel: KeyEventChannel | undefined;
  private mTextInputHandler: TextInputMethodHandlerImpl;

  /**
   * Constructs a new TextInputPlugin instance.
   * @param textInputChannel - The TextInputChannel for communication with Flutter
   * @param viewId - The view ID for focus management
   * @param keyEventChannel - Optional KeyEventChannel for key event handling
   * @param flutterViewContext - Optional page-level UIContext used by autoFillManager APIs
   */
  constructor(textInputChannel: TextInputChannel, viewId: string, keyEventChannel?: KeyEventChannel,
    flutterViewContext?: UIContext) {
    this.textInputChannel = textInputChannel;
    this.keyEventChannel = keyEventChannel;
    // viewId is used for requestFocus
    this.mTextInputHandler = new TextInputMethodHandlerImpl(this, viewId, flutterViewContext);
    this.textInputChannel.setTextInputMethodHandler(this.mTextInputHandler);
  }

  /**
   * Sets the page-level UIContext used by autoFillManager APIs. Supplied by FlutterPage / FlutterView.
   */
  setAutofillUiContext(uiContext: UIContext): void {
    this.mTextInputHandler.setFlutterViewContext(uiContext);
  }

  /**
   * Called when the system autofill service provides values for registered fields.
   * @param values - Map from autofill virtual id to filled text
   */
  autofill(values: Map<number, string>): void {
    this.mTextInputHandler.autofill(values);
  }

  /**
   * Clears the current text input client.
   */
  public clearTextInputClient() {
    this.textInputChannel.textInputMethodHandler?.clearClient();
  }

  /**
   * Gets the current editing state.
   * @returns The current ListenableEditingState instance
   */
  getEditingState() {
    return this.mTextInputHandler.mEditable;
  }

  /**
   * Called when the editing state changes. Delegates sync strategy to PreviewTextCoordinator.
   * @param textChanged - Whether the text has changed
   * @param selectionChanged - Whether the selection has changed
   * @param composingRegionChanged - Whether the composing region has changed
   */
  didChangeEditingState(textChanged: boolean, selectionChanged: boolean, composingRegionChanged: boolean): void {
    if (textChanged) {
      this.mTextInputHandler.notifyValueChanged(this.mTextInputHandler.mEditable.getStringCache());
    }
    const editable = this.mTextInputHandler.mEditable;
    const inputTarget = this.mTextInputHandler.inputTarget;
    const configuration = this.mTextInputHandler.configuration;
    const handler = this.mTextInputHandler;
    PreviewTextCoordinator.syncEditingState(
      editable,
      inputTarget.id,
      configuration,
      (id, text, selStart, selEnd, composingStart, composingEnd) => {
        this.textInputChannel.updateEditingState(id, text, selStart, selEnd, composingStart, composingEnd);
      },
      (id, batchDeltas: ArrayList<TextEditingDelta>) => {
        this.textInputChannel.updateEditingStateWithDeltas(id, batchDeltas);
        editable.clearBatchDeltas();
      },
      (text, start, end) => this.mTextInputHandler.notifySelectionToFramework(text, start, end),
      {
        lastOutgoing: handler.mLastOutgoingFrameworkState,
        onOutgoingCommitted: (s: TextEditState): void => {
          handler.mLastOutgoingFrameworkState = s;
        }
      }
    );
  }

  /**
   * Detaches the text input plugin from the input method framework.
   */
  detach(): void {
    this.mTextInputHandler.inputMethodController.detach((err) => {
      if (err) {
        Log.e(TextInputPlugin.TAG, "Failed to detach: " + JSON.stringify(err));
      }
    })
  }

  /**
   * Destroys the text input plugin, hiding the keyboard and cleaning up resources.
   */
  destroy() {
    // Since the Dart side no longer listens to the lifecycle,
    // the keyboard must be explicitly hidden before the engine is destroyed.
    this.mTextInputHandler.hide();
    this.textInputChannel.setTextInputMethodHandler(null);
  }
}

const INPUT_TYPE_NAME =
  ['NONE', 'TEXT', 'MULTILINE', 'NUMBER', 'PHONE', 'DATETIME', 'EMAIL_ADDRESS', 'URL', 'VISIBLE_PASSWORD']

/**
 * Implementation of TextInputMethodHandler for OpenHarmony input method framework.
 * This class handles all interactions with the system input method.
 */
class TextInputMethodHandlerImpl implements TextInputMethodHandler {
  private static TAG = "TextInputMethodHandlerImpl";

  /**
   * Document [rangeStart, rangeEnd) to replace with committed Latin preview: stacked | dup | lr.
   */
  private static latinFinishPreviewReplaceRange(
    leftIdx: number,
    rightIdx: number,
    previewText: string,
    cache: string,
    imcStart: number,
    imcEnd: number,
  ): LatinFinishPreviewRange {
    const imcW = imcEnd - imcStart;
    const previewLen = previewText.length;
    if (previewLen > 0 &&
        leftIdx === imcW &&
        leftIdx < rightIdx &&
        imcStart >= 0 &&
        imcEnd > imcStart &&
        imcW > 0 &&
        cache.startsWith(previewText)
    ) {
      const rangeEnd = Math.min(cache.length, Math.max(rightIdx, imcW));
      const out: LatinFinishPreviewRange = { rangeStart: 0, rangeEnd: rangeEnd, mode: 'stacked' };
      return out;
    }
    if (
      previewLen > 0 &&
      cache.length >= rightIdx &&
      leftIdx === previewLen &&
      rightIdx === leftIdx + previewLen &&
      cache.startsWith(previewText) &&
      cache.substring(leftIdx, rightIdx) === previewText
    ) {
      const out: LatinFinishPreviewRange = { rangeStart: 0, rangeEnd: rightIdx, mode: 'dup' };
      return out;
    }
    const out: LatinFinishPreviewRange = { rangeStart: leftIdx, rangeEnd: rightIdx, mode: 'lr' };
    return out;
  }

  private static isImeSegmentApostropheLike(ch: string): boolean {
    if (ch === "'" || ch.charCodeAt(0) === 0x2019 || ch.charCodeAt(0) === 0x02bc || ch.charCodeAt(0) === 0xff07) {
      return true;
    }
    return false;
  }

  /**
   * True when preview text contains a segment apostrophe immediately after a Latin letter (e.g. ni'hao).
   * Leading `'` alone (e.g. `'hao` after deleting a syllable prefix) must not defer finishTextPreview:
   * otherwise [leftIdx, rightIdx) is never committed before the CJK insertText candidate.
   */
  private static previewTextHasInternalSegmentApostrophe(previewText: string): boolean {
    for (let idx = 0; idx < previewText.length; idx++) {
      if (!TextInputMethodHandlerImpl.isImeSegmentApostropheLike(previewText.charAt(idx))) {
        continue;
      }
      if (idx > 0) {
        const prev = previewText.charAt(idx - 1);
        if ((prev >= 'a' && prev <= 'z') || (prev >= 'A' && prev <= 'Z')) {
          return true;
        }
      }
    }
    return false;
  }

  /** Leading IME syllable marker before the remaining Latin body (e.g. `'hao`). */
  private static previewStartsWithImeSegmentMarker(previewText: string): boolean {
    if (previewText.length < 1) {
      return false;
    }
    return TextInputMethodHandlerImpl.isImeSegmentApostropheLike(previewText.charAt(0));
  }

  private textConfig: inputMethod.TextConfig;
  /** The input method controller for managing the keyboard. */
  inputMethodController: inputMethod.InputMethodController;
  /** The current input target. */
  inputTarget: InputTarget;
  /** The current text input configuration, or null if not set. */
  public configuration: Configuration | null = null;
  private lastKind?: PointerDeviceKind;
  /** The editable state for text input. */
  mEditable: ListenableEditingState;
  private mRestartInputPending: boolean = false;
  /**
   * Last full editing state pushed from native to Flutter (aligned with Android mLastKnownFrameworkTextEditingState),
   * used to skip duplicate channel updates and to detect Framework echo that mutates the composing region.
   */
  mLastOutgoingFrameworkState: TextEditState | null = null;
  private plugin: EditingStateWatcher | Any;
  private imcFlag: boolean = false;
  private keyboardStatus: inputMethod.KeyboardStatus = inputMethod.KeyboardStatus.HIDE;
  private inputAttribute: inputMethod.InputAttribute =
    { textInputType: inputMethod.TextInputType.TEXT, enterKeyType: inputMethod.EnterKeyType.NONE };
  private keyboardFocusState: boolean = false;
  private focusViewId: string = "";
  // Record the type of soft keyboard that was triggered last time
  private lastInputType?: inputMethod.TextInputType;
  private isInputMethodAttached: boolean = false;
  /**
   * After finishTextPreview commits Latin preview using document [leftIdx, rightIdx), OHOS may still deliver insertText(same).
   * One matching insertText is skipped to avoid duplicate words (e.g. "This This").
   */
  private mConsumeNextInsertTextIfEquals: string | null = null;
  /**
   * finishTextPreview committed with retainPreviewRangeAfterCommit; clear staging on duplicate insertText skip
   * or after the real follow-up insertText.
   */
  private mFinishRetainedPreviewRangePending: boolean = false;
  private autofillConfiguration: Map<string, Configuration> | null = null;
  private flutterViewContext?: UIContext | undefined;
  private autoFillHostInfo: AutoFillHostInfo | null = null;
  private lastClientRect: AutoFillFieldRect | null = null;
  private lastPageRect: AutoFillFieldRect | null = null;
  private fieldRects: Map<string, AutoFillFieldRect> = new Map();
  private pendingAutofillRequest: boolean = false;
  private autoFillSessionActive: boolean = false;
  /** Prevents duplicate requestAutoFill for the same focused client. */
  private autoFillRequestInFlight: boolean = false;
  private autoFillRequestedClientId: number = -1;
  /** After user cancels the vault UI, suppress auto-request until focus changes. */
  private autofillSuppressedClientId: number = -1;
  /** Defer autofill until IME attach completes (avoids sync requestAutoFill throw). */
  private pendingAutofillAfterImeAttach: boolean = false;
  /** Latest text per autofill uniqueIdentifier for save/fill ViewData. */
  private autofillFieldTexts: Map<string, string> = new Map();
  /** Stable nodeId per autofill uniqueIdentifier; rebuilt whenever the autofill group changes. */
  private nodeIdMap: Map<string, number> = new Map();
  /** Blocks clearTextInputClient from tearing down session before requestAutoSave completes. */
  private autoSaveInFlight: boolean = false;
  /** User callback for the in-flight save; cleared when NAPI bridge fires. */
  private pendingSaveUserCallback: autoFillManager.AutoSaveCallback | undefined = undefined;

  private onSaveNapiSuccess = (): void => {
    const userCallback = this.pendingSaveUserCallback;
    this.pendingSaveUserCallback = undefined;
    userCallback?.onSuccess();
  };

  private onSaveNapiFailure = (): void => {
    const userCallback = this.pendingSaveUserCallback;
    this.pendingSaveUserCallback = undefined;
    userCallback?.onFailure();
  };

  private getSaveNapiBridge(): autoFillManager.AutoSaveCallback {
    return {
      onSuccess: this.onSaveNapiSuccess,
      onFailure: this.onSaveNapiFailure,
    };
  }

  /**
   * Constructs a new TextInputMethodHandlerImpl instance.
   * @param plugin - The TextInputPlugin instance
   * @param viewId - The view ID for focus management
   * @param flutterViewContext - Optional page-level UIContext used by autoFillManager APIs
   */
  constructor(plugin: TextInputPlugin | Any, viewId: string, flutterViewContext?: UIContext) {
    this.textConfig = {
      inputAttribute: this.inputAttribute
    };
    this.plugin = plugin;
    this.mEditable = new ListenableEditingState(null, 0);
    this.inputMethodController = inputMethod.getController();
    this.inputTarget = new InputTarget(Type.NO_TARGET, 0);
    this.focusViewId = viewId;
    this.flutterViewContext = flutterViewContext;
  }

  setFlutterViewContext(flutterViewContext: UIContext): void {
    this.flutterViewContext = flutterViewContext;
  }

  /**
   * Requests autofill from the system password manager via autoFillManager.
   */
  requestAutofill(): void {
    const decision = this.evaluateAutofillRequest();
    switch (decision) {
      case 'reject':
        return;
      case 'wait-rect':
        this.pendingAutofillRequest = true;
        return;
      case 'wait-ime':
        this.pendingAutofillAfterImeAttach = true;
        return;
      case 'proceed':
        this.pendingAutofillRequest = false;
        this.pendingAutofillAfterImeAttach = false;
        this.notifyViewEntered();
        return;
    }
  }

  private evaluateAutofillRequest(): 'reject' | 'wait-rect' | 'wait-ime' | 'proceed' {
    if (deviceInfo.sdkApiVersion < AUTOFILL_SUPPORT_API ||
      !this.needsAutofill() ||
      this.configuration?.autofill === undefined) {
      return 'reject';
    }
    if (this.inputTarget.id === this.autofillSuppressedClientId) {
      return 'reject';
    }
    if (this.autoFillRequestInFlight) {
      return 'reject';
    }
    if (this.autoFillRequestedClientId === this.inputTarget.id && this.autoFillSessionActive) {
      return 'reject';
    }
    if (!this.hasValidAutofillRect()) {
      return 'wait-rect';
    }
    if (!this.isInputMethodAttached && this.canShowTextInput()) {
      return 'wait-ime';
    }
    return 'proceed';
  }

  private hasValidAutofillRect(): boolean {
    return OhosAutoFillHelper.isValidRect(this.lastClientRect);
  }

  /**
   * Finishes the autofill context and optionally triggers auto-save.
   * Mirrors Android AutofillManager.commit() / cancel().
   */
  finishAutofillContext(shouldSave: boolean): void {
    if (deviceInfo.sdkApiVersion < AUTOFILL_SUPPORT_API) {
      return;
    }
    if (!shouldSave) {
      this.notifyViewExited();
      return;
    }
    if (this.flutterViewContext === undefined) {
      return;
    }
    if (this.configuration === null || this.configuration.autofill === undefined) {
      return;
    }
    this.autoSaveInFlight = true;
    const uiContext = this.flutterViewContext;
    const configSnapshot = this.configuration;
    const clientRectSnapshot = this.lastClientRect;
    const pageRectSnapshot = this.lastPageRect ?? this.lastClientRect;
    const fieldRectsSnapshot = new Map<string, AutoFillFieldRect>(this.fieldRects);
    this.rememberCurrentAutofillFieldText();
    const fieldTextsSnapshot = new Map<string, string>(this.autofillFieldTexts);
    if (configSnapshot.autofill !== undefined) {
      fieldTextsSnapshot.set(
        configSnapshot.autofill.uniqueIdentifier,
        this.mEditable.getStringCache(),
      );
    }
    const autofillConfigSnapshot = this.autofillConfiguration !== null ?
      new Map<string, Configuration>(this.autofillConfiguration) : null;
    const nodeIdMapSnapshot = new Map<string, number>(this.nodeIdMap);
    const focusedUniqueId = configSnapshot.autofill!.uniqueIdentifier;
    const sessionFieldKeys = new Set<string>(fieldTextsSnapshot.keys());
    const saveCallback: autoFillManager.AutoSaveCallback = {
      onSuccess: (): void => {
        this.autoSaveInFlight = false;
        sessionFieldKeys.forEach((key: string) => {
          this.autofillFieldTexts.delete(key);
        });
        this.resetAutofillRequestTracking();
        this.notifyViewExited();
      },
      onFailure: (): void => {
        this.autoSaveInFlight = false;
        sessionFieldKeys.forEach((key: string) => {
          this.autofillFieldTexts.delete(key);
        });
        this.resetAutofillRequestTracking();
        this.notifyViewExited();
      },
    };
    const invokeSave = (hostInfo: AutoFillHostInfo): void => {
      if (hostInfo.bundleName.length === 0) {
        this.autoSaveInFlight = false;
        this.pendingSaveUserCallback = undefined;
        this.resetAutofillRequestTracking();
        return;
      }
      const mergedFieldConfigs = TextInputMethodHandlerImpl.mergeAutofillFieldConfigs(
        autofillConfigSnapshot, configSnapshot);
      TextInputMethodHandlerImpl.enrichFieldTextsSnapshot(fieldTextsSnapshot, mergedFieldConfigs);
      mergedFieldConfigs.forEach((_config: Configuration, uniqueId: string) => {
        if (!nodeIdMapSnapshot.has(uniqueId)) {
          nodeIdMapSnapshot.set(uniqueId, nodeIdMapSnapshot.size);
        }
      });
      const viewData = OhosAutoFillHelper.buildViewDataForSave(
        hostInfo,
        mergedFieldConfigs,
        fieldTextsSnapshot,
        nodeIdMapSnapshot,
        focusedUniqueId,
        clientRectSnapshot,
        pageRectSnapshot,
        fieldRectsSnapshot,
      );
      OhosAutoFillHelper.logViewDataSummary(viewData, 'requestAutoSave before invoke');
      this.pendingSaveUserCallback = saveCallback;
      OhosAutoFillHelper.requestAutoSave(uiContext, viewData, this.getSaveNapiBridge());
    };
    const cachedHostInfo = OhosAutoFillHelper.getCachedHostInfo();
    if (cachedHostInfo !== null) {
      invokeSave(cachedHostInfo);
      return;
    }
    this.ensureAutoFillHostInfo().then((hostInfo: AutoFillHostInfo) => {
      invokeSave(hostInfo);
    }).catch((error: BusinessError) => {
      this.autoSaveInFlight = false;
      this.pendingSaveUserCallback = undefined;
      this.resetAutofillRequestTracking();
    });
  }

  setAutoFillFieldRect(left: number, top: number, width: number, height: number): void {
    this.lastClientRect = { left, top, width, height };
    if (this.configuration?.autofill !== undefined) {
      this.fieldRects.set(this.configuration.autofill.uniqueIdentifier, this.lastClientRect);
    }
    if (this.pendingAutofillRequest && this.hasValidAutofillRect()) {
      this.pendingAutofillRequest = false;
      if (!this.isInputMethodAttached && this.canShowTextInput()) {
        this.pendingAutofillAfterImeAttach = true;
        return;
      }
      this.notifyViewEntered();
      return;
    }
  }

  setAutoFillPageRect(left: number, top: number, width: number, height: number): void {
    this.lastPageRect = { left, top, width, height };
  }

  /**
   * Shows the text input keyboard if appropriate.
   * The keyboard is only shown if the input type is not NONE.
   */
  show(): void {
    try {
      let isPhysicalKeyboard = false;
      inputDevice.getDeviceList((Error: Error, ids: Array<number>) => {
        for (let i = 0; i < ids.length; i++) {
          const type = inputDevice.getKeyboardTypeSync(ids[i]);
          if (type === inputDevice.KeyboardType.ALPHABETIC_KEYBOARD ||
            type === inputDevice.KeyboardType.DIGITAL_KEYBOARD) {
            isPhysicalKeyboard = true;
            break;
          }
        }
        // API 20+: with physical keyboard attached, avoid repeatedly raising the soft keyboard.
        if (isPhysicalKeyboard) {
          this.keyboardStatus = inputMethod.KeyboardStatus.SHOW;
        }
      })
    } catch (error) {
      Log.e(TextInputMethodHandlerImpl.TAG,
        `Show function failed to query device. Code is ${error.code}, message is ${error.message}`)
    }
    if (this.canShowTextInput()) {
      // Ensure the Xcomponent gains focus before the soft keyboard is displayed.
      focusControl.requestFocus(this.focusViewId);
      this.keyboardFocusState = true;
      this.showTextInput();
    } else {
      this.hide();
    }
  }

  /**
   * Hides the text input keyboard.
   */
  hide(): void {
    // Do not notifyViewExited here: keyboard hide must not tear down autofill session.
    // Ensure the Xcomponent loses focus before the soft keyboard is hided.
    focusControl.requestFocus("unfocus-xcomponent-node");
    this.keyboardFocusState = false;
    this.hideTextInput();
  }

  /**
   * Gets the current keyboard focus state.
   * @returns True if the keyboard has focus, false otherwise
   */
  getKeyboardFocusState() {
    return this.keyboardFocusState;
  }

  /**
   * Notifies the input method framework of current display text and selection.
   * Used by PreviewTextCoordinator so that Plugin does not depend on inputMethodController.
   */
  notifySelectionToFramework(displayText: string, selectionStart: number, selectionEnd: number): void {
    this.inputMethodController.changeSelection(displayText, selectionStart, selectionEnd);
  }

  /** After native edit: `updateEditingState` + `changeSelection`; preview path uses `composeDisplayState` (composing -1). */
  syncImcSelectionAfterEditableChange(): void {
    if (PreviewTextCoordinator.shouldSyncWithPreview(this.mEditable)) {
      const displayState = PreviewTextCoordinator.composeDisplayState(this.mEditable);
      this.plugin.textInputChannel.updateEditingState(
        this.inputTarget.id,
        displayState.text,
        displayState.selectionStart,
        displayState.selectionEnd,
        displayState.composingStart,
        displayState.composingEnd
      );
      this.mLastOutgoingFrameworkState = PreviewTextCoordinator.displayStateToTextEditState(displayState);
      this.inputMethodController.changeSelection(displayState.text, displayState.selectionStart, displayState.selectionEnd);
    } else {
      this.inputMethodController.changeSelection(
        this.mEditable.getStringCache(),
        this.mEditable.getSelectionStart(),
        this.mEditable.getSelectionEnd());
    }
  }

  /**
   * After commitLeadingLatinPreviewWhenCaretOutsideSpan replaces `'+latin` with Han locally, OHOS IME may still
   * emit SetPreviewText for a suffix of the old Latin (e.g. "ha"). Empty-then-full changeSelection clears that state.
   */
  private notifyImcFlushComposingAfterLatinOutsideSpanCommit(): void {
    this.inputMethodController.changeSelection("", -1, -1, (err: BusinessError) => {
      if (err) {
        Log.e(TextInputMethodHandlerImpl.TAG,
          `Failed to changeSelection (flush composing), code: ${err.code}, message: ${err.message}`);
        return;
      }
      const text = this.mEditable.getStringCache();
      const start = this.mEditable.getSelectionStart();
      const end = this.mEditable.getSelectionEnd();
      this.inputMethodController.changeSelection(text, start, end);
    });
  }

  /**
   * Sets the text input client.
   * @param textInputClientId - The client ID
   * @param configuration - The input configuration
   */
  setClient(textInputClientId: number, configuration: Configuration | null): void {
    this.setTextInputClient(textInputClientId, configuration);
  }

  /**
   * Updates the input configuration.
   * @param configuration - The new input configuration
   */
  updateConfig(configuration: Configuration | null) {
    if (configuration) {
      this.lastKind = this.configuration?.deviceKind;
      this.configuration = configuration;
      if (configuration.inputType) {
        this.textConfig.inputAttribute.textInputType = configuration.inputType.type;
        this.textConfig.inputAttribute.enterKeyType = configuration.inputAction as Any;
      }
      if (configuration.autofill !== undefined) {
        this.updateAutofillConfigurationIfNeeded(configuration);
      }
    }
  }

  /**
   * Sets the platform view client for text input.
   * @param id - The platform view ID
   * @param usesVirtualDisplay - Whether the platform view uses a virtual display
   */
  setPlatformViewClient(id: number, usesVirtualDisplay: boolean): void {

  }

  /**
   * Sets the editable size and transform.
   * @param width - The width of the editable area
   * @param height - The height of the editable area
   * @param transform - The transformation matrix
   */
  setEditableSizeAndTransform(width: number, height: number, transform: number[]): void {

  }

  /**
   * Sets the cursor size and position.
   * @param cursorInfo - The cursor information
   */
  setCursorSizeAndPosition(cursorInfo: inputMethod.CursorInfo) {
    try {
      this.inputMethodController.updateCursor(cursorInfo, (err: BusinessError) => {
        if (err) {
          Log.e(TextInputMethodHandlerImpl.TAG, "Failed to updateCursor:" + JSON.stringify(err));
          return;
        }
      })
    } catch (err) {
      Log.e(TextInputMethodHandlerImpl.TAG, "Failed to updateCursor:" + JSON.stringify(err));
    }
  }

  /**
   * Sets the editing state from Flutter.
   * @param editingState - The new editing state
   */
  setEditingState(editingState: TextEditState): void {
    let reconnectImeForComposingDrift = false;
    if (this.mLastOutgoingFrameworkState !== null && this.mLastOutgoingFrameworkState.hasComposing()) {
      if (TextInputMethodHandlerImpl.composingRegionMutatedByFramework(
        this.mLastOutgoingFrameworkState, editingState)) {
        reconnectImeForComposingDrift = true;
      }
    }

    // Preview staging: handle taps inside [leftIdx, rightIdx), strictly before leftIdx, or strictly after rightIdx without falling through to raw Flutter apply.
    if (isPreviewTextSupported() && this.mEditable.isPreviewStagingActive()) {
      const displayNow = PreviewTextCoordinator.composeDisplayState(this.mEditable);
      if (displayNow.text !== editingState.text) {
        this.mEditable.discardPreviewStagingMetadata();
        this.mEditable.updateTextInputState(editingState);
        this.mLastOutgoingFrameworkState = PreviewTextCoordinator.safeTextEditStateForNativeDocument(
          editingState.text,
          editingState.selectionStart,
          editingState.selectionEnd,
          editingState.composingStart,
          editingState.composingEnd
        )
        this.notifySelectionToFramework(editingState.text, editingState.selectionStart, editingState.selectionEnd);
        return;
      }
      const leftIdx = this.mEditable.getLeftIdxOfPreviewTextRange();
      const rightIdx = this.mEditable.getRightIdxOfPreviewTextRange();
      const selStart = editingState.selectionStart;
      const selEnd = editingState.selectionEnd;
      const cursorInsidePreview = (leftIdx !== rightIdx) &&
        selStart >= leftIdx && selStart <= rightIdx && selEnd >= leftIdx && selEnd <= rightIdx;
      if (cursorInsidePreview) {
        const localSelStart = this.mEditable.getSelectionStart();
        const localSelEnd = this.mEditable.getSelectionEnd();
        const displayNow = PreviewTextCoordinator.composeDisplayState(this.mEditable);
        const flutterSelMin = Math.min(selStart, selEnd);
        const flutterSelMax = Math.max(selStart, selEnd);
        const flutterPinsCaretAtPreviewEnd =
          selStart === rightIdx && selEnd === rightIdx;
        const localCaretStrictlyBeforePreviewEnd = localSelStart < rightIdx || localSelEnd < rightIdx;
        const staleEchoSkipsLeft = this.mEditable.getSkipFlutterStaleEndEchoRemaining();
        const skipFlutterSelAsStaleEndEcho =
          staleEchoSkipsLeft > 0 &&
          flutterPinsCaretAtPreviewEnd &&
          localCaretStrictlyBeforePreviewEnd;
        if (skipFlutterSelAsStaleEndEcho) {
          this.mEditable.consumeStaleEndEchoSkipCredit();
        }
        const localPinsCaretAtPreviewEnd =
          localSelStart === rightIdx && localSelEnd === rightIdx;
        const invertedFullPreviewRange =
          selStart === rightIdx && selEnd === leftIdx && rightIdx > leftIdx;
        const skipFlutterSelAsInvertedFullPreviewWhileLocalAtEnd =
          localPinsCaretAtPreviewEnd &&
          invertedFullPreviewRange &&
          editingState.text === displayNow.text;
        const flutterSelectsEntirePreviewRange =
          flutterSelMin === leftIdx &&
          flutterSelMax === rightIdx &&
          selStart !== selEnd;
        const skipFlutterSelAsStaleFullPreviewWhileLocalCaretAtEnd =
          localPinsCaretAtPreviewEnd &&
          flutterSelectsEntirePreviewRange &&
          editingState.text === displayNow.text;
        const skipApplyFlutterSelection =
          skipFlutterSelAsStaleEndEcho ||
          skipFlutterSelAsInvertedFullPreviewWhileLocalAtEnd ||
          skipFlutterSelAsStaleFullPreviewWhileLocalCaretAtEnd;
        if (!skipApplyFlutterSelection) {
          this.mEditable.applyExternalDocumentSelectionWhilePreviewStaging(selStart, selEnd);
        }
        const displayState = PreviewTextCoordinator.composeDisplayState(this.mEditable);
        this.plugin.textInputChannel.updateEditingState(
          this.inputTarget.id,
          displayState.text,
          displayState.selectionStart,
          displayState.selectionEnd,
          displayState.composingStart,
          displayState.composingEnd
        );
        this.mLastOutgoingFrameworkState = PreviewTextCoordinator.displayStateToTextEditState(displayState);
        this.notifySelectionToFramework(displayState.text, displayState.selectionStart, displayState.selectionEnd);
        return;
      }
      const outerSelMin = Math.min(selStart, selEnd);
      const outerSelMax = Math.max(selStart, selEnd);
      const caretStrictlyBeforePreview =
        leftIdx < rightIdx && selStart >= 0 && selEnd >= 0 && outerSelMax < leftIdx;
      if (caretStrictlyBeforePreview) {
        this.mEditable.applyExternalDocumentSelectionWhilePreviewStaging(outerSelMin, outerSelMax);
        const committedLatinPrefix = this.mEditable.commitLeadingLatinPreviewWhenCaretOutsideSpan();
        const displayState = PreviewTextCoordinator.composeDisplayState(this.mEditable);
        this.plugin.textInputChannel.updateEditingState(
          this.inputTarget.id,
          displayState.text,
          displayState.selectionStart,
          displayState.selectionEnd,
          displayState.composingStart,
          displayState.composingEnd,
        );
        this.mLastOutgoingFrameworkState = PreviewTextCoordinator.displayStateToTextEditState(displayState);
        if (committedLatinPrefix) {
          this.notifyImcFlushComposingAfterLatinOutsideSpanCommit();
        } else {
          this.notifySelectionToFramework(displayState.text, displayState.selectionStart, displayState.selectionEnd);
        }
        return;
      }
      const caretStrictlyAfterPreview =
        leftIdx < rightIdx && selStart >= 0 && selEnd >= 0 && outerSelMin > rightIdx;
      if (caretStrictlyAfterPreview) {
        // Same shape as “before”: apply selection, `commitLeadingLatin` then `commitPreviewWhenCursorOutOfRange`, push display, return.
        this.mEditable.applyExternalDocumentSelectionWhilePreviewStaging(outerSelMin, outerSelMax);
        const committedLatinSuffix = this.mEditable.commitLeadingLatinPreviewWhenCaretOutsideSpan();
        let committedCoordinatorOut = false;
        if (!committedLatinSuffix) {
          committedCoordinatorOut = PreviewTextCoordinator.commitPreviewWhenCursorOutOfRange(
            this.mEditable,
            editingState,
            (text, start, end) => this.notifySelectionToFramework(text, start, end)
          );
        }
        if (committedCoordinatorOut) {
          this.mEditable.setIsCursorIdxOutOfPreviewTextRange(false);
          this.inputMethodController.changeSelection("", -1, -1, (err: BusinessError) => {
            if (err) {
              Log.e(TextInputMethodHandlerImpl.TAG, `Failed to changeSelection, code: ${err.code}, message: ${err.message}`);
              return;
            }
            const text = this.mEditable.getStringCache();
            const start = this.mEditable.getSelectionStart();
            const end = this.mEditable.getSelectionEnd();
            this.inputMethodController.changeSelection(text, start, end);
          });
          return;
        }
        const displayState = PreviewTextCoordinator.composeDisplayState(this.mEditable);
        this.plugin.textInputChannel.updateEditingState(
          this.inputTarget.id,
          displayState.text,
          displayState.selectionStart,
          displayState.selectionEnd,
          displayState.composingStart,
          displayState.composingEnd,
        );
        this.mLastOutgoingFrameworkState = PreviewTextCoordinator.displayStateToTextEditState(displayState);
        if (committedLatinSuffix) {
          this.notifyImcFlushComposingAfterLatinOutsideSpanCommit();
        } else {
          this.notifySelectionToFramework(displayState.text, displayState.selectionStart, displayState.selectionEnd);
        }
        return;
      }
    }

    if (isPreviewTextSupported() &&
      PreviewTextCoordinator.commitPreviewWhenCursorOutOfRange(
        this.mEditable,
        editingState,
        (text, start, end) => this.notifySelectionToFramework(text, start, end)
      )) {
      this.mEditable.setIsCursorIdxOutOfPreviewTextRange(false);
      this.inputMethodController.changeSelection("",
        -1, -1, (err: BusinessError) => {
          if (err) {
            Log.e(TextInputMethodHandlerImpl.TAG, `Failed to changeSelection, code: ${err.code}, message: ${err.message}`);
            return;
          }
          const text = this.mEditable.getStringCache();
          const start = this.mEditable.getSelectionStart();
          const end = this.mEditable.getSelectionEnd();
          this.inputMethodController.changeSelection(text, start, end);
        });
      return;
    }

    const flutterEchoesCache = editingState.text === this.mEditable.getStringCache();
    const previewMetaActive = isPreviewTextSupported() &&
      (this.mEditable.getPreviewText() !== '' ||
        this.mEditable.getLeftIdxOfPreviewTextRange() !== this.mEditable.getRightIdxOfPreviewTextRange());
    const previewStagingHeld = isPreviewTextSupported() && this.mEditable.isPreviewStagingActive();
    if ((!flutterEchoesCache && !previewStagingHeld) || !previewMetaActive) {
      this.mEditable.discardPreviewStagingMetadata();
    }
    this.mEditable.setIsCursorIdxOutOfPreviewTextRange(false);
    this.mEditable.updateTextInputState(editingState);
    if (reconnectImeForComposingDrift) {
      this.scheduleImeReconnectAfterFrameworkComposingChange();
    }
    this.inputMethodController.changeSelection("",
      -1, -1, (err: BusinessError) => {
        if (err) {
          Log.e(TextInputMethodHandlerImpl.TAG, `Failed to changeSelection, code: ${err.code}, message: ${err.message}`);
          return;
        }
        const text = this.mEditable.getStringCache();
        const start = this.mEditable.getSelectionStart();
        const end = this.mEditable.getSelectionEnd();
        this.inputMethodController.changeSelection(text, start, end);
      });
  }

  /**
   * Clears the current text input client.
   */
  clearClient(): void {
    this.clearTextInputClient();
  }

  private async showTextInput(): Promise<void> {
    if (!this.lastInputType) {
      this.setLastInputType(this.configuration?.inputType?.type);
    }
    if (this.lastKind === undefined && this.configuration?.deviceKind !== undefined) {
      this.lastKind = this.configuration.deviceKind;
    }
    if (this.keyboardStatus == inputMethod.KeyboardStatus.SHOW) {
      // When keyboard is already SHOW, still call attach; false means attach does not show the keyboard again.
      await this.attach(false);
      if (this.lastKind != this.configuration?.deviceKind ||
        this.hasSecureKeyboardInSwitch()) {
        if (deviceInfo.sdkApiVersion >= INPUT_SUPPORT_API) {
          await this.inputMethodController.showTextInput(this.getRequestReason());
        } else {
          await this.inputMethodController.showTextInput();
        }
        if (this.configuration?.deviceKind !== undefined) {
          this.lastKind = this.configuration.deviceKind;
        }
      }
      this.setLastInputType(this.configuration?.inputType?.type);
      return;
    }
    this.setLastInputType(this.configuration?.inputType?.type);
    await this.attach(true);
    if (this.configuration?.deviceKind !== undefined) {
      this.lastKind = this.configuration.deviceKind;
    }
    if (!this.imcFlag) {
      this.listenKeyBoardEvent();
    }
  }

  private async hideTextInput(): Promise<void> {
    await this.inputMethodController.detach().then(() => {
      this.keyboardStatus = inputMethod.KeyboardStatus.HIDE;
      this.cancelListenKeyBoardEvent();
    }).catch((err: BusinessError) => {
      Log.e(TextInputMethodHandlerImpl.TAG, "Failed to detach: " + JSON.stringify(err));
      this.keyboardStatus = inputMethod.KeyboardStatus.NONE;
    });
  }

  /**
   * Attaches to the input method controller.
   * @param showKeyboard - Whether to show the keyboard immediately
   * @private
   */
  async attach(showKeyboard: boolean): Promise<void> {
    try {
      // must register preview text callbacks before attachment
      if (isPreviewTextSupported()) {
        this.registerPreviewTextCallbacks();
      }
      if (deviceInfo.sdkApiVersion >= INPUT_SUPPORT_API) {
        await this.inputMethodController.attach(showKeyboard, this.textConfig, this.getRequestReason()).then(async () => {
          await this.handleAttach(showKeyboard);
        });
      } else {
        await this.inputMethodController.attach(showKeyboard, this.textConfig).then(async () => {
          await this.handleAttach(showKeyboard);
        });
      }
    } catch (err) {
      Log.e(TextInputMethodHandlerImpl.TAG, "Failed to attach:" + JSON.stringify(err));
      this.keyboardStatus = inputMethod.KeyboardStatus.NONE;
    }
  }

  private async handleAttach(showKeyboard: boolean): Promise<void> {
    let isDetached = false;
    if (showKeyboard) {
      isDetached = await this.detachIfBackground();
    }
    if (isDetached) {
      this.isInputMethodAttached = false;
      this.keyboardStatus = inputMethod.KeyboardStatus.NONE;
    } else {
      this.isInputMethodAttached = true;
      this.cacheMethodCall();
      this.keyboardStatus = inputMethod.KeyboardStatus.SHOW;
      this.maybeRunPendingAutofillRequest();
    }
  }

  private async detachIfBackground(): Promise<boolean> {
    try {
      const bundleFlags = bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_APPLICATION;
      const bundleInfo: bundleManager.BundleInfo  = await bundleManager.getBundleInfoForSelf(bundleFlags);
      const currentInfo = await appManager.getRunningProcessInformation();
      const targetBundleName = bundleInfo?.name;
      const currentProcess = currentInfo.find(info => info.bundleNames?.includes(targetBundleName));
      // STATE_FOREGROUND: process in foreground; when UI is focused and visible the state is often STATE_ACTIVE.
      // See HarmonyOS appManager ProcessState docs.
      if (
        currentProcess &&
        (currentProcess.state === appManager.ProcessState.STATE_FOREGROUND ||
          currentProcess.state === appManager.ProcessState.STATE_BACKGROUND)
      ) {
          await this.inputMethodController.hideTextInput();
          return true;
      }
    } catch (err) {
      Log.e(
        TextInputMethodHandlerImpl.TAG,
        `detachIfBackground error: ${JSON.stringify(err)}`,
      );
    }
    return false;
  }

  cacheMethodCall(): void {
    // Cache method calls when input method is attached; no-op if not used by this version
  }

  /**
   * Gets the request reason for showing the keyboard based on the input device kind.
   * @returns The request reason for keyboard display
   */
  getRequestReason() : inputMethod.RequestKeyboardReason {
    let deviceKind : PointerDeviceKind = this.configuration?.deviceKind ?? PointerDeviceKind.UNKNOWN
    switch (deviceKind) {
      case PointerDeviceKind.TOUCH:
        return inputMethod.RequestKeyboardReason.TOUCH;
      case PointerDeviceKind.MOUSE:
        return inputMethod.RequestKeyboardReason.MOUSE;
      case PointerDeviceKind.STYLUS:
      case PointerDeviceKind.INVERTED_STYLUS:
      case PointerDeviceKind.TRACKPAD:
        return inputMethod.RequestKeyboardReason.OTHER;
      default:
        return inputMethod.RequestKeyboardReason.NONE;
    }
  }

  /**
   * Handles focus state changes.
   * @param focusState - The new focus state
   */
  handleChangeFocus(focusState: boolean) {
    if (focusState && this.keyboardFocusState) {
      // When the app loses focus, the system automatically detaches the input method.
      // Upon regaining focus, if the input method should be displayed, it must be reattached.
      this.show();
    }
    try {
      inputDevice.getDeviceList((Error: Error, ids: Array<number>) => {
        let isPhysicalKeyboard = false;
        for (let i = 0; i < ids.length; i++) {
          const type = inputDevice.getKeyboardTypeSync(ids[i]);
          if (type == inputDevice.KeyboardType.ALPHABETIC_KEYBOARD || type == inputDevice.KeyboardType.DIGITAL_KEYBOARD) {
            isPhysicalKeyboard = true;
            break;
          }
        }

        if(focusState && isPhysicalKeyboard && this.keyboardFocusState) {
          this.cancelListenKeyBoardEvent();
          this.inputMethodController.detach().then(async () =>{
            await this.attach(true);
            this.listenKeyBoardEvent();
          })
        }
      })
    } catch (error) {
      Log.e(TextInputMethodHandlerImpl.TAG, `Failed to query device. Code is ${error.code}, message is ${error.message}`)
    }
  }

  /**
   * Updates the input attribute configuration.
   */
  async updateAttribute(): Promise<void> {
    if (this.keyboardStatus != inputMethod.KeyboardStatus.SHOW) {
      return;
    }
    try {
      await this.inputMethodController.updateAttribute(this.inputAttribute);
    } catch (err) {
      Log.e(TextInputMethodHandlerImpl.TAG, "Failed to updateAttribute:" + JSON.stringify(err));
    }
  }

  /**
   * Sets the text input client with the given configuration.
   * @param client - The client ID
   * @param configuration - The input configuration
   */
  setTextInputClient(client: number, configuration: Configuration | null): void {
    this.rememberCurrentAutofillFieldText();
    const stayingInAutofillGroup = this.isSameAutofillGroup(configuration);
    if (!stayingInAutofillGroup) {
      this.notifyViewExited();
      this.resetAutofillRequestTracking();
    }
    if (this.inputTarget.id !== client ||
      configuration?.autofill?.uniqueIdentifier !== this.configuration?.autofill?.uniqueIdentifier) {
      this.autofillSuppressedClientId = -1;
    }
    this.configuration = configuration;
    if (configuration?.inputType) {
      this.textConfig.inputAttribute.textInputType = configuration.inputType.type;
      this.textConfig.inputAttribute.enterKeyType = configuration.inputAction as Any;
    }
    if (this.canShowTextInput()) {
      this.inputTarget = new InputTarget(Type.FRAMEWORK_CLIENT, client);
    } else {
      this.inputTarget = new InputTarget(Type.NO_TARGET, client);
    }
    this.mEditable.removeEditingStateListener(this.plugin);

    this.mEditable = new ListenableEditingState(
      this.plugin.textInputChannel, this.inputTarget.id, this.plugin.keyEventChannel);

    this.updateAutofillConfigurationIfNeeded(configuration);

    this.mRestartInputPending = true;
    this.mLastOutgoingFrameworkState = null;
    this.mEditable.addEditingStateListener(this.plugin);

    this.inputAttribute = this.textConfig.inputAttribute;

    this.updateAttribute();
  }

  setLastInputType(inputType?: inputMethod.TextInputType): void {
    this.lastInputType = inputType;
  }

  // It is used to determine whether there is a safe keyboard for the keyboard type
  // awakened by the two input boxes when a soft keyboard is already suspended
  // and switching input boxes
  hasSecureKeyboardInSwitch(): boolean {
    // Since this method is called in showTextInput to determine whether a secure 
 	  // keyboard needs to be pulled up, the parameter must be true
    this.handleAttach(true);
    return this.lastInputType === inputMethod.TextInputType.VISIBLE_PASSWORD ||
    this.configuration?.inputType?.type === inputMethod.TextInputType.VISIBLE_PASSWORD;
  }

  /**
   * Checks if text input can be shown.
   * @returns True if text input can be shown, false if input type is NONE
   */
  canShowTextInput(): boolean {
    if (this.configuration == null || this.configuration.inputType == null) {
      return true;
    }
    return this.configuration.inputType.type != inputMethod.TextInputType.NONE;
  }

  /**
   * Registers callbacks for preview text functionality.
   */
  registerPreviewTextCallbacks(): void {
    try {
      this.inputMethodController.on("setPreviewText", this.setPreviewTextCallback)
    } catch (err) {
      Log.e(TextInputMethodHandlerImpl.TAG, "Failed to subscribe setPreviewText:" + JSON.stringify(err));
      this.unregisterPreviewTextCallbacks();
      return;
    }

    try {
      this.inputMethodController.on("finishTextPreview", this.finishPreviewTextCallback)
    } catch (err) {
      Log.e(TextInputMethodHandlerImpl.TAG, "Failed to subscribe finishTextPreview:" + JSON.stringify(err));
      this.unregisterPreviewTextCallbacks();
      return;
    }
  }

  /**
   * Unregisters preview text callbacks.
   */
  unregisterPreviewTextCallbacks(): void {
    this.inputMethodController?.off("setPreviewText", this.setPreviewTextCallback)
    this.inputMethodController?.off("finishTextPreview", this.finishPreviewTextCallback);
  }

  /**
   * Registers listeners for keyboard events from the input method framework.
   */
  listenKeyBoardEvent(): void {
    try {
      this.inputMethodController.on('insertText', this.insertTextCallback);
    } catch (err) {
      Log.e(TextInputMethodHandlerImpl.TAG, "Failed to subscribe insertText:" + JSON.stringify(err));
      this.cancelListenKeyBoardEvent();
      return;
    }

    try {
      this.inputMethodController.on('deleteLeft', this.deleteLeftCallback)
    } catch (err) {
      Log.e(TextInputMethodHandlerImpl.TAG, "Failed to subscribe deleteLeft:" + JSON.stringify(err));
      this.cancelListenKeyBoardEvent();
      return;
    }

    try {
      this.inputMethodController.on('deleteRight', this.deleteRightCallback)
    } catch (err) {
      Log.e(TextInputMethodHandlerImpl.TAG, "Failed to subscribe deleteRight:" + JSON.stringify(err));
      this.cancelListenKeyBoardEvent();
      return;
    }

    try {
      this.inputMethodController.on('sendFunctionKey', this.sendFunctionKeyCallback)
    } catch (err) {
      Log.e(TextInputMethodHandlerImpl.TAG, "Failed to subscribe sendFunctionKey:" + JSON.stringify(err));
      this.cancelListenKeyBoardEvent();
      return;
    }

    try {
      this.inputMethodController.on('sendKeyboardStatus', this.sendKeyboardStatusCallback)
    } catch (err) {
      Log.e(TextInputMethodHandlerImpl.TAG, "Failed to subscribe sendKeyboardStatus:" + JSON.stringify(err));
      this.cancelListenKeyBoardEvent();
      return;
    }

    try {
      this.inputMethodController.on('selectByRange', this.selectByRangeCallback)
    } catch (err) {
      Log.e(TextInputMethodHandlerImpl.TAG, "Failed to subscribe selectByRange:" + JSON.stringify(err));
      this.cancelListenKeyBoardEvent();
      return;
    }

    try {
      this.inputMethodController.on('moveCursor', this.moveCursorCallback)
    } catch (err) {
      Log.e(TextInputMethodHandlerImpl.TAG, "Failed to subscribe moveCursor:" + JSON.stringify(err));
      this.cancelListenKeyBoardEvent();
      return;
    }

    try {
      this.inputMethodController.on('handleExtendAction', this.handleExtendActionCallback)
    } catch (err) {
      Log.e(TextInputMethodHandlerImpl.TAG, "Failed to subscribe handleExtendAction:" + JSON.stringify(err));
      this.cancelListenKeyBoardEvent();
      return;
    }
    try {
      this.inputMethodController.on('selectByMovement', this.selectByMovementCallback)
    } catch (err) {
      Log.e(TextInputMethodHandlerImpl.TAG, "Failed to subscribe selectByMovement:" + JSON.stringify(err));
      this.cancelListenKeyBoardEvent();
      return;
    }
    try {
      this.inputMethodController.on('getLeftTextOfCursor', this.getLeftTextOfCursorCallback)
    } catch (err) {
      Log.e(TextInputMethodHandlerImpl.TAG, "Failed to subscribe getLeftTextOfCursor:" + JSON.stringify(err));
      this.cancelListenKeyBoardEvent();
      return;
    }
    try {
      this.inputMethodController.on('getRightTextOfCursor', this.getRightTextOfCursorCallback)
    } catch (err) {
      Log.e(TextInputMethodHandlerImpl.TAG, "Failed to subscribe getRightTextOfCursor:" + JSON.stringify(err));
      this.cancelListenKeyBoardEvent();
      return;
    }
    try {
      this.inputMethodController.on('getTextIndexAtCursor', this.getTextIndexAtCursorCallback)
    } catch (err) {
      Log.e(TextInputMethodHandlerImpl.TAG, "Failed to subscribe getTextIndexAtCursor:" + JSON.stringify(err));
      this.cancelListenKeyBoardEvent();
      return;
    }

    this.imcFlag = true;
  }

  private setPreviewTextCallback = (text: string, range: inputMethod.Range) => {
    this.mEditable.handleInsertPreviewTextEvent(text, range);
    // Same as insertText/deleteLeft: if merge corrected away from raw IME text, push IMC+Flutter again or IME buffer keeps old tail-delete
    // semantics and suggestions diverge from the field (e.g. field shows abcefg while suggestions still include d).
    if (this.mEditable.consumeImcFullSyncAfterPreviewInsert()) {
      this.syncImcSelectionAfterEditableChange();
    }
  }

  private finishPreviewTextCallback = () => {
    const previewText = this.mEditable.getPreviewText();
    if (typeof previewText !== 'string' || previewText.length === 0) {
      const leftIdx = this.mEditable.getLeftIdxOfPreviewTextRange();
      const rightIdx = this.mEditable.getRightIdxOfPreviewTextRange();
      if (leftIdx < rightIdx) {
        this.mEditable.setPreviewText('');
        return;
      }
      this.mEditable.clearPreviewTextContents();
      return;
    }
    const rawImcStart = this.mEditable.getPreviewTextStart();
    const rawImcEnd = this.mEditable.getPreviewTextEnd();
    let imcStart = rawImcStart;
    let imcEnd = rawImcEnd;
    // When editing preview text, meanwhile switch the app to the background and
    // the preview text will automatically insert into the string cache of TextField.
    // imcMeta start/end are often preview-relative (e.g. 0/4); replacement in the document must use [leftIdx, rightIdx), not imcMeta.
    // OHOS sometimes reports (-1,-1) on finishTextPreview; latinFinishPreviewReplaceRange still needs a half-open
    // preview-relative span — default to [0, previewText.length) so we commit instead of waiting for insertText that never arrives.
    if (imcStart < 0 || imcEnd < 0) {
      imcStart = 0;
      imcEnd = previewText.length;
    }
    // Match `finishTextPreview` defer: internal syllable `'` — wait for `insertText`, no local substitute here.
    if (TextInputMethodHandlerImpl.previewTextHasInternalSegmentApostrophe(previewText)) {
      return;
    }
    const leftIdx = this.mEditable.getLeftIdxOfPreviewTextRange();
    const rightIdx = this.mEditable.getRightIdxOfPreviewTextRange();
    if (!(leftIdx < rightIdx && this.mEditable.isPreviewStagingActive())) {
      return;
    }
    // Caret entirely before span: defer (retainSpan commit with preview cleared breaks in-preview delete UI).
    const selStart = this.mEditable.getSelectionStart();
    const selEnd = this.mEditable.getSelectionEnd();
    if (selStart >= 0 && selEnd >= 0 && Math.max(selStart, selEnd) < leftIdx) {
      return;
    }
    const committedPreview = previewText;
    const cache = this.mEditable.getStringCache();
    const commitRange = TextInputMethodHandlerImpl.latinFinishPreviewReplaceRange(
      leftIdx,
      rightIdx,
      committedPreview,
      cache,
      imcStart,
      imcEnd,
    );
    const retainSpan = TextInputMethodHandlerImpl.previewStartsWithImeSegmentMarker(committedPreview);
    const opts: PreviewInsertOptions = {
      usePreviewRange: true,
      previewStart: commitRange.rangeStart,
      previewEnd: commitRange.rangeEnd,
      retainPreviewRangeAfterCommit: retainSpan,
    };
    this.mEditable.handleInsertTextEvent(committedPreview, opts);
    this.mFinishRetainedPreviewRangePending = retainSpan;
    // Leading-segment tail (e.g. 'hao): IME often sends insertText(same Latin) after finishTextPreview to align
    // the client buffer. Skipping it as "duplicate" can prevent the follow-up insertText(CJK candidate).
    // English / non-retain finish still arms consume to drop the echo word once.
    this.mConsumeNextInsertTextIfEquals = retainSpan ? null : committedPreview;
  }

  private insertTextCallback = (text: string) => {
    if (this.mConsumeNextInsertTextIfEquals !== null) {
      if (text === this.mConsumeNextInsertTextIfEquals) {
        this.mConsumeNextInsertTextIfEquals = null;
        this.syncImcSelectionAfterEditableChange();
        return;
      }
      this.mConsumeNextInsertTextIfEquals = null;
    }
    this.mEditable.handleInsertTextEvent(text);
    this.mFinishRetainedPreviewRangePending = false;
    this.syncImcSelectionAfterEditableChange();
  }

  private deleteLeftCallback = (length: number) => {
    this.mEditable.handleDeleteEvent(false, length, this.configuration?.enableDeltaModel);
    this.syncImcSelectionAfterEditableChange();
  }

  private deleteRightCallback = (length: number) => {
    this.mEditable.handleDeleteEvent(true, length, this.configuration?.enableDeltaModel);
    this.syncImcSelectionAfterEditableChange();
  }

  private sendFunctionKeyCallback = (functionKey: inputMethod.FunctionKey) => {
    if (functionKey.enterKeyType == inputMethod.EnterKeyType.NEWLINE) {
      // insertText callback does not deliver newline; handle it here.
      this.mEditable.handleNewlineEvent();
    }
    this.mEditable.handleFunctionKey(functionKey);
  }

  private sendKeyboardStatusCallback = (state: inputMethod.KeyboardStatus) => {
    // Since API 20, tapping the field with physical keyboard can show soft keyboard; avoid onConnectionClosed then or the field stops accepting input.
    try {
      let isPhysicalKeyboard = false;
      inputDevice.getDeviceList((Error: Error, ids: Array<number>) => {
        for (let i = 0; i < ids.length; i++) {
          const type = inputDevice.getKeyboardTypeSync(ids[i]);
          if (type === inputDevice.KeyboardType.ALPHABETIC_KEYBOARD ||
            type === inputDevice.KeyboardType.DIGITAL_KEYBOARD) {
            isPhysicalKeyboard = true;
            return;
          }
        }
        this.keyboardStatus = state;
        if (state === inputMethod.KeyboardStatus.HIDE) {
          // Autofill vault UI hides the soft keyboard; must not tear down the connection/session.
          const isAutofillActive =
            this.autoFillSessionActive || this.autoFillRequestInFlight || this.autoSaveInFlight;
          if (isAutofillActive) {
            return;
          }
          this.plugin.textInputChannel.onConnectionClosed(this.inputTarget.id);
        }
      })
    } catch (error) {
      Log.e(TextInputMethodHandlerImpl.TAG,
        `SendKeyboardStatusCallback function failed to query device. Code is ${error.code}, message is ${error.message}`)
    }
  }

  // obtain the range to update cursor idx to start/end of text cache
  private selectByRangeCallback = (range: inputMethod.Range) => {
    this.mEditable.handleSelectByRangeEvent(range);
    this.syncImcSelectionAfterEditableChange();
  }

  // move cursor postion with left, right, up and down in editing widget
  private moveCursorCallback = (direction: inputMethod.Direction) => {
    this.mEditable.handleMoveCursorEvent(direction);
    this.syncImcSelectionAfterEditableChange();
  }

  // handle extend clipboard actions, like 'select all', 'cut', 'copy' and 'paste'
  private handleExtendActionCallback = (action: inputMethod.ExtendAction) => {
    this.mEditable.handleExtendActionEvent(action);
    this.syncImcSelectionAfterEditableChange();
  }

  // text selection range changed with cursor direction movement
  private selectByMovementCallback = (movement: inputMethod.Movement) => {
    this.mEditable.handleSelectByMovementEvent(movement);
    this.syncImcSelectionAfterEditableChange();
  }

  // return the left-side text string of current cursor index ("abc|345" -> return "abc")
  private getLeftTextOfCursorCallback = (length: number) : string => {
    return this.mEditable.getLeftTextOfCursor(length);
  }

  // return the right-side text string of current cursor index ("abc|345" -> return "345")
  private getRightTextOfCursorCallback = (length: number) : string => {
    return this.mEditable.getRightTextOfCursor(length);
  }

  // return the current cursor index of editing text
  private getTextIndexAtCursorCallback = () => {
    return this.mEditable.getSelectionStart();
  }

  /**
   * Cancels all keyboard event listeners.
   */
  cancelListenKeyBoardEvent(): void {
    this.inputMethodController?.off('insertText', this.insertTextCallback);
    this.inputMethodController?.off('deleteLeft', this.deleteLeftCallback);
    this.inputMethodController?.off('deleteRight', this.deleteRightCallback);
    this.inputMethodController?.off('sendFunctionKey', this.sendFunctionKeyCallback);
    this.inputMethodController?.off('sendKeyboardStatus', this.sendKeyboardStatusCallback);
    this.inputMethodController?.off('selectByRange', this.selectByRangeCallback);
    this.inputMethodController?.off('moveCursor', this.moveCursorCallback);
    this.inputMethodController?.off('handleExtendAction', this.handleExtendActionCallback);
    this.inputMethodController?.off('selectByMovement', this.selectByMovementCallback);
    this.inputMethodController?.off('getLeftTextOfCursor', this.getLeftTextOfCursorCallback);
    this.inputMethodController?.off('getRightTextOfCursor', this.getRightTextOfCursorCallback);
    this.inputMethodController?.off('getTextIndexAtCursor', this.getTextIndexAtCursorCallback);
    this.imcFlag = false;
  }

  /**
   * Clears the text input client.
   */
  public clearTextInputClient(): void {
    if (this.inputTarget.type == Type.VIRTUAL_DISPLAY_PLATFORM_VIEW) {
      return;
    }
    if (!this.autoSaveInFlight) {
      this.notifyViewExited();
    }
    this.mEditable.removeEditingStateListener(this.plugin);
    this.configuration = null;
    this.autofillConfiguration = null;
    this.nodeIdMap = new Map();
    this.lastClientRect = null;
    this.lastPageRect = null;
    this.fieldRects.clear();
    this.pendingAutofillRequest = false;
    if (!this.autoSaveInFlight) {
      this.autoFillSessionActive = false;
      this.resetAutofillRequestTracking();
      this.autofillFieldTexts.clear();
    }
    this.autofillSuppressedClientId = -1;
    this.inputTarget = new InputTarget(Type.NO_TARGET, 0);
    this.mLastOutgoingFrameworkState = null;
  }

  /**
   * Same idea as Android TextInputPlugin.composingChanged: compare only composing region content and length between two TextEditStates.
   */
  private static composingRegionMutatedByFramework(before: TextEditState, after: TextEditState): boolean {
    const regionLen = before.composingEnd - before.composingStart;
    if (regionLen !== (after.composingEnd - after.composingStart)) {
      return true;
    }
    for (let i = 0; i < regionLen; i++) {
      if (before.text.charAt(before.composingStart + i) !== after.text.charAt(after.composingStart + i)) {
        return true;
      }
    }
    return false;
  }

  /** Same idea as Android InputMethodManager.restartInput: reset IME session when Framework mutates composing unexpectedly. */
  private scheduleImeReconnectAfterFrameworkComposingChange(): void {
    if (this.keyboardStatus !== inputMethod.KeyboardStatus.SHOW) {
      return;
    }
    this.inputMethodController.detach()
      .then(() => this.attach(false))
      .then(() => {
        this.listenKeyBoardEvent();
      })
      .catch((err: BusinessError) => {
        Log.e(TextInputMethodHandlerImpl.TAG, 'IME reconnect failed: ' + JSON.stringify(err));
      });
  }

  // -------- Start: Autofill -------

  private needsAutofill(): boolean {
    return this.autofillConfiguration !== null;
  }

  private nodeIdFor(uniqueIdentifier: string): number {
    return this.nodeIdMap.get(uniqueIdentifier) ?? 0;
  }

  private async ensureAutoFillHostInfo(): Promise<AutoFillHostInfo> {
    if (this.autoFillHostInfo !== null) {
      return this.autoFillHostInfo;
    }
    this.autoFillHostInfo = await OhosAutoFillHelper.loadHostInfo();
    return this.autoFillHostInfo;
  }

  private applyAutofillValues(values: Map<number, string>): void {
    if (values.size === 0) {
      return;
    }
    this.autofill(values);
    this.restoreImeInputSession();
  }

  notifyValueChanged(newValue: string): void {
    if (!this.needsAutofill() || this.configuration?.autofill === undefined) {
      return;
    }
    this.autofillFieldTexts.set(this.configuration.autofill.uniqueIdentifier, newValue);
  }

  private resetAutofillRequestTracking(): void {
    this.autoFillRequestInFlight = false;
    this.autoFillRequestedClientId = -1;
    this.autoFillSessionActive = false;
    this.pendingAutofillAfterImeAttach = false;
  }

  private isSameAutofillGroup(configuration: Configuration | null): boolean {
    if (!this.needsAutofill() || configuration?.autofill === undefined) {
      return false;
    }
    const prevFields = this.configuration?.fields;
    const nextFields = configuration.fields;
    if (prevFields === null || prevFields === undefined || prevFields.length === 0 ||
      nextFields === null || nextFields === undefined || nextFields.length === 0) {
      return false;
    }
    if (prevFields.length !== nextFields.length) {
      return false;
    }
    const prevIds = new Set<string>();
    for (const field of prevFields) {
      if (field.autofill !== undefined) {
        prevIds.add(field.autofill.uniqueIdentifier);
      }
    }
    for (const field of nextFields) {
      if (field.autofill === undefined || !prevIds.has(field.autofill.uniqueIdentifier)) {
        return false;
      }
    }
    return true;
  }

  private maybeRunPendingAutofillRequest(): void {
    if (!this.pendingAutofillAfterImeAttach && !this.pendingAutofillRequest) {
      return;
    }
    if (!this.needsAutofill() || !this.hasValidAutofillRect()) {
      return;
    }
    if (this.inputTarget.id === this.autofillSuppressedClientId) {
      this.pendingAutofillAfterImeAttach = false;
      this.pendingAutofillRequest = false;
      return;
    }
    if (this.autoFillRequestInFlight) {
      return;
    }
    this.pendingAutofillAfterImeAttach = false;
    this.pendingAutofillRequest = false;
    this.notifyViewEntered();
  }

  private restoreImeInputSession(): void {
    if (!this.canShowTextInput()) {
      return;
    }
    this.pendingAutofillAfterImeAttach = false;
    this.pendingAutofillRequest = false;
    focusControl.requestFocus(this.focusViewId);
    this.keyboardFocusState = true;
    if (this.isInputMethodAttached && this.imcFlag &&
      this.keyboardStatus === inputMethod.KeyboardStatus.SHOW) {
      return;
    }
    this.inputMethodController.detach().then(async () => {
      this.isInputMethodAttached = false;
      this.cancelListenKeyBoardEvent();
      await this.attach(true);
      this.listenKeyBoardEvent();
    }).catch((err: BusinessError) => {
      this.showTextInput();
    });
  }

  private applyAutofillEditingState(editingState: TextEditState): void {
    this.mEditable.updateTextInputState(editingState);
    this.mLastOutgoingFrameworkState = editingState;
    this.plugin.textInputChannel.updateEditingState(
      this.inputTarget.id,
      editingState.text,
      editingState.selectionStart,
      editingState.selectionEnd,
      editingState.composingStart,
      editingState.composingEnd,
    );
    if (this.isInputMethodAttached) {
      this.inputMethodController.changeSelection(
        editingState.text, editingState.selectionStart, editingState.selectionEnd);
    }
  }

  private rememberCurrentAutofillFieldText(): void {
    if (this.configuration?.autofill === undefined) {
      return;
    }
    this.autofillFieldTexts.set(this.configuration.autofill.uniqueIdentifier, this.mEditable.getStringCache());
  }

  private applyFieldTextsSnapshotToViewData(
    viewData: OhosViewData,
    fieldTexts: Map<string, string>,
    clearFocus: boolean,
  ): void {
    fieldTexts.forEach((text: string, uniqueIdentifier: string) => {
      const nodeId = this.nodeIdFor(uniqueIdentifier);
      for (let i = 0; i < viewData.pageNodeInfos.length; i++) {
        if (viewData.pageNodeInfos[i].id === nodeId) {
          viewData.pageNodeInfos[i].value = text;
        }
      }
    });
    if (clearFocus) {
      for (let i = 0; i < viewData.pageNodeInfos.length; i++) {
        viewData.pageNodeInfos[i].isFocus = false;
      }
    }
  }

  private syncAllAutofillFieldTextsToViewData(viewData: OhosViewData, configuration: Configuration): void {
    this.applyFieldTextsSnapshotToViewData(viewData, this.autofillFieldTexts, false);
    if (configuration.autofill !== undefined) {
      const focusedId = this.nodeIdFor(configuration.autofill.uniqueIdentifier);
      for (let i = 0; i < viewData.pageNodeInfos.length; i++) {
        if (viewData.pageNodeInfos[i].id === focusedId) {
          viewData.pageNodeInfos[i].value = this.mEditable.getStringCache();
        }
      }
    }
  }

  private notifyViewEntered(): void {
    if (!this.needsAutofill() || this.configuration?.autofill === undefined ||
      this.flutterViewContext === undefined) {
      return;
    }
    if (!this.hasValidAutofillRect()) {
      this.pendingAutofillRequest = true;
      return;
    }
    this.autoFillSessionActive = true;
    this.autoFillRequestInFlight = true;
    this.rememberCurrentAutofillFieldText();
    this.ensureAutoFillHostInfo().then((hostInfo: AutoFillHostInfo) => {
      if (this.configuration === null || this.configuration.autofill === undefined ||
        this.flutterViewContext === undefined) {
        this.autoFillRequestInFlight = false;
        return;
      }
      const viewData = OhosAutoFillHelper.buildViewData(
        hostInfo,
        this.configuration,
        this.nodeIdMap,
        this.lastClientRect,
        this.lastPageRect ?? this.lastClientRect,
        this.fieldRects,
      );
      this.syncAllAutofillFieldTextsToViewData(viewData, this.configuration);
      const autoFillType = OhosAutoFillHelper.primaryAutoFillType(this.configuration.autofill);
      const requestClientId = this.inputTarget.id;
      OhosAutoFillHelper.requestAutoFill(
        this.flutterViewContext,
        viewData,
        autoFillType,
        (values: Map<number, string>): void => {
          this.autoFillRequestInFlight = false;
          this.autoFillSessionActive = false;
          this.autoFillRequestedClientId = requestClientId;
          this.applyAutofillValues(values);
        },
        (errCode: number): void => {
          this.autoFillRequestInFlight = false;
          this.autoFillSessionActive = false;
          if (errCode === 2) {
            this.autofillSuppressedClientId = requestClientId;
            this.restoreImeInputSession();
          }
        },
      );
    }).catch((error: BusinessError) => {
      this.autoFillRequestInFlight = false;
    });
  }

  private notifyViewExited(): void {
    if (!this.autoFillSessionActive && !this.autoFillRequestInFlight &&
      this.autoFillRequestedClientId === -1) {
      return;
    }
    this.autoFillSessionActive = false;
    this.autoFillRequestInFlight = false;
    this.autoFillRequestedClientId = -1;
  }

  private updateAutofillConfigurationIfNeeded(configuration: Configuration | null): void {
    if (configuration === null || configuration.autofill === undefined) {
      this.autofillConfiguration = null;
      this.nodeIdMap = new Map();
      return;
    }

    const fields = configuration.fields;
    if (fields === null || fields.length === 0) {
      if (this.autofillConfiguration === null) {
        this.autofillConfiguration = new Map<string, Configuration>();
      }
      this.autofillConfiguration.set(configuration.autofill.uniqueIdentifier, configuration);
    } else {
      this.autofillConfiguration = new Map<string, Configuration>();
      for (const config of fields) {
        if (config.autofill === undefined) {
          continue;
        }
        this.autofillConfiguration.set(config.autofill.uniqueIdentifier, config);
      }
    }
    this.nodeIdMap = OhosAutoFillHelper.buildNodeIdMap(configuration);
  }

  private static mergeAutofillFieldConfigs(
    autofillConfigSnapshot: Map<string, Configuration> | null,
    configSnapshot: Configuration,
  ): Map<string, Configuration> {
    const merged = new Map<string, Configuration>();
    if (autofillConfigSnapshot !== null) {
      autofillConfigSnapshot.forEach((config: Configuration, uniqueId: string) => {
        merged.set(uniqueId, config);
      });
    }
    const fields = configSnapshot.fields;
    if (fields !== null && fields.length > 0) {
      for (const config of fields) {
        if (config.autofill !== undefined) {
          merged.set(config.autofill.uniqueIdentifier, config);
        }
      }
    }
    if (configSnapshot.autofill !== undefined) {
      merged.set(configSnapshot.autofill.uniqueIdentifier, configSnapshot);
    }
    return merged;
  }

  private static enrichFieldTextsSnapshot(
    fieldTextsSnapshot: Map<string, string>,
    mergedFieldConfigs: Map<string, Configuration>,
  ): void {
    mergedFieldConfigs.forEach((config: Configuration, uniqueId: string) => {
      if (config.autofill === undefined) {
        return;
      }
      const existing = fieldTextsSnapshot.get(uniqueId);
      if (existing !== undefined && existing.length > 0) {
        return;
      }
      const fallbackText = config.autofill.editState.text;
      if (fallbackText.length > 0) {
        fieldTextsSnapshot.set(uniqueId, fallbackText);
      }
    });
  }

  autofill(values: Map<number, string>): void {
    if (this.configuration === null || this.autofillConfiguration === null ||
      this.configuration.autofill === undefined) {
      return;
    }

    const currentAutofill = this.configuration.autofill;
    const editingValues: Map<string, TextEditState> = new Map();
    this.autofillConfiguration.forEach((config: Configuration, uniqueIdentifier: string) => {
      if (config.autofill === undefined) {
        return;
      }
      const nodeId = this.nodeIdFor(uniqueIdentifier);
      const value = values.get(nodeId);
      if (value === undefined || value.length === 0) {
        return;
      }
      if (config.autofill.uniqueIdentifier !== uniqueIdentifier) {
        return;
      }

      const autofill = config.autofill;
      this.autofillFieldTexts.set(autofill.uniqueIdentifier, value);
      const newState = new TextEditState(value, value.length, value.length, -1, -1);

      if (autofill.uniqueIdentifier === currentAutofill.uniqueIdentifier) {
        this.applyAutofillEditingState(newState);
      } else {
        editingValues.set(autofill.uniqueIdentifier, newState);
      }
    });

    if (editingValues.size > 0) {
      this.plugin.textInputChannel.updateEditingStateWithTag(this.inputTarget.id, editingValues);
    }
  }

  // -------- End: Autofill -------
}

/**
 * Enumeration of input target types.
 */
enum Type {
  /** No input target */
  NO_TARGET,
  /** InputConnection is managed by the TextInputPlugin, and events are forwarded to the Flutter framework. */
  FRAMEWORK_CLIENT,
  /** InputConnection is managed by a platform view that is presented on a virtual display. */
  VIRTUAL_DISPLAY_PLATFORM_VIEW,
  /** InputConnection is managed by a platform view that is presented on a physical display. */
  PHYSICAL_DISPLAY_PLATFORM_VIEW,
}

/**
 * Represents a target for text input operations.
 */
export class InputTarget {
  /** The type of input target. */
  type: Type;
  /** The unique identifier for this input target. */
  id: number;

  /**
   * Constructs a new InputTarget instance.
   * @param type - The type of input target
   * @param id - The target ID
   */
  constructor(type: Type, id: number) {
    this.type = type;
    this.id = id;
  }
}
