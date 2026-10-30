/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
import { beforeEach, describe, expect, it } from '@ohos/hypium';
import TextInputChannel, {
  Configuration,
  InputType,
  TextEditState,
  Autofill,
} from '../../../../../main/ets/embedding/engine/systemchannels/TextInputChannel';
import TextInputPlugin from '../../../../../main/ets/plugin/editing/TextInputPlugin';
import common from '@ohos.app.ability.common';
import DartExecutor from '../../../../../main/ets/embedding/engine/dart/DartExecutor';
import FlutterInjector from '../../../../../main/ets/FlutterInjector';
import { PointerDeviceKind } from '../../../../../main/ets/embedding/ohos/OhosTouchProcessor';
import inputMethod from '@ohos.inputMethod';
import Any from '../../../../../main/ets/plugin/common/Any';
import { BusinessError } from '@kit.BasicServicesKit';

/**
 * State-machine / pure-logic coverage for TextInputMethodHandlerImpl.
 * Every test gets a fresh plugin/channel via beforeEach; the real impl is the
 * handler installed on the channel (`textInputChannel.textInputMethodHandler`),
 * reached through ESObject so private fields can be injected and private
 * methods invoked. Tails that hit the real IMC (attach/showTextInput) are
 * avoided; everything here stays in pure state wiring plus safe channel sends.
 */

function makeAutofill(id: string, hints: string[], text: string): Autofill {
  return new Autofill(id, hints, null,
    new TextEditState(text, text.length, text.length, -1, -1));
}

function makeConfigKind(
  autofill: Autofill | undefined,
  fields: Configuration[],
  textInputType: inputMethod.TextInputType,
  deviceKind: PointerDeviceKind): Configuration {
  return new Configuration(false, false, false, false, false,
    new InputType(textInputType, false, false),
    0, '', autofill, [], deviceKind, fields);
}

function makeConfig(
  autofill: Autofill | undefined,
  fields: Configuration[],
  textInputType: inputMethod.TextInputType): Configuration {
  return makeConfigKind(autofill, fields, textInputType, PointerDeviceKind.UNKNOWN);
}

function makeFieldConfig(id: string, hints: string[], text: string): Configuration {
  return makeConfig(makeAutofill(id, hints, text), [], inputMethod.TextInputType.TEXT);
}

/** setClient with a single-field autofill group so needsAutofill() is true. */
function activateAutofill(h: ESObject, clientId: number): void {
  const pass = makeFieldConfig('pass', ['password'], 'secret');
  h.setClient(clientId, makeConfig(pass.autofill, [pass], inputMethod.TextInputType.TEXT));
}

/**
 * Stand-in for inputMethod.InputMethodController so changeSelection records the call
 * instead of touching a real IMC.
 */
class FakeInputMethodController {
  lastSelectionText: string = '';
  lastSelectionStart: number = -1;
  lastSelectionEnd: number = -1;
  changeSelection(text: string, selectionStart: number, selectionEnd: number): void;
  changeSelection(text: string, selectionStart: number, selectionEnd: number,
    callback: (err: BusinessError) => void): void;
  changeSelection(text: string, selectionStart: number, selectionEnd: number,
    callback?: (err: BusinessError) => void): void {
    this.lastSelectionText = text;
    this.lastSelectionStart = selectionStart;
    this.lastSelectionEnd = selectionEnd;
    if (callback) {
      callback(null as Any as BusinessError);
    }
  }
  hideTextInput(): void {
  }
}

/** Stage preview metadata on the shared handler mEditable via public setters (no real IME merge). */
function stagePreview(h: ESObject, cache: string, preview: string, leftIdx: number, rightIdx: number,
  selStart: number, selEnd: number): void {
  const es: ESObject = h.mEditable as ESObject;
  es.setStringCache(cache);
  es.setSelectionStart(selStart);
  es.setSelectionEnd(selEnd);
  es.setPreviewText(preview);
  es.updatePreviewTextRange(leftIdx, rightIdx);
}

export default function TextInputPluginStateTest() {
  describe('TextInputPluginStateTest', () => {
    let textInputPlugin: TextInputPlugin;
    let textInputChannelMock: TextInputChannel;
    beforeEach(() => {
      let context: common.Context = getContext();
      const assetManager = context.resourceManager;
      const flutterNapi = FlutterInjector.getInstance().getFlutterNapi();
      const dartExecutor = new DartExecutor(flutterNapi, assetManager);
      textInputChannelMock = new TextInputChannel(dartExecutor);
      textInputPlugin = new TextInputPlugin(textInputChannelMock, "oh_flutter_state_1", undefined);
    })

    it('getRequestReason_maps_device_kinds', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      h.configuration = makeConfigKind(undefined, [], inputMethod.TextInputType.TEXT, PointerDeviceKind.TOUCH);
      expect(h.getRequestReason()).assertEqual(inputMethod.RequestKeyboardReason.TOUCH);
      h.configuration = makeConfigKind(undefined, [], inputMethod.TextInputType.TEXT, PointerDeviceKind.MOUSE);
      expect(h.getRequestReason()).assertEqual(inputMethod.RequestKeyboardReason.MOUSE);
      h.configuration = makeConfigKind(undefined, [], inputMethod.TextInputType.TEXT, PointerDeviceKind.STYLUS);
      expect(h.getRequestReason()).assertEqual(inputMethod.RequestKeyboardReason.OTHER);
      h.configuration = makeConfigKind(undefined, [], inputMethod.TextInputType.TEXT, PointerDeviceKind.INVERTED_STYLUS);
      expect(h.getRequestReason()).assertEqual(inputMethod.RequestKeyboardReason.OTHER);
      h.configuration = makeConfigKind(undefined, [], inputMethod.TextInputType.TEXT, PointerDeviceKind.TRACKPAD);
      expect(h.getRequestReason()).assertEqual(inputMethod.RequestKeyboardReason.OTHER);
      h.configuration = makeConfigKind(undefined, [], inputMethod.TextInputType.TEXT, PointerDeviceKind.UNKNOWN);
      expect(h.getRequestReason()).assertEqual(inputMethod.RequestKeyboardReason.NONE);
      h.configuration = null;
      expect(h.getRequestReason()).assertEqual(inputMethod.RequestKeyboardReason.NONE);
    })

    it('evaluateAutofillRequest_reject_without_autofill', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      h.setClient(100, makeConfig(undefined, [], inputMethod.TextInputType.TEXT));
      expect(h.evaluateAutofillRequest()).assertEqual('reject');
    })

    it('evaluateAutofillRequest_reject_suppressed_client', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      activateAutofill(h, 101);
      h.autofillSuppressedClientId = 101;
      expect(h.evaluateAutofillRequest()).assertEqual('reject');
    })

    it('evaluateAutofillRequest_reject_in_flight', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      activateAutofill(h, 102);
      h.autoFillRequestInFlight = true;
      expect(h.evaluateAutofillRequest()).assertEqual('reject');
    })

    it('evaluateAutofillRequest_reject_already_requested', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      activateAutofill(h, 103);
      h.autoFillRequestedClientId = 103;
      h.autoFillSessionActive = true;
      expect(h.evaluateAutofillRequest()).assertEqual('reject');
    })

    it('requestAutofill_proceed_clears_pending', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      activateAutofill(h, 111);
      h.lastClientRect = { left: 0, top: 0, width: 100, height: 100 };
      h.isInputMethodAttached = true;
      h.requestAutofill();
      expect(h.pendingAutofillRequest).assertEqual(false);
      expect(h.pendingAutofillAfterImeAttach).assertEqual(false);
    })

    it('isSameAutofillGroup_matrix', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      const pass = makeFieldConfig('pass', ['password'], 'secret');
      const user = makeFieldConfig('user', ['username'], 'alice');
      const passOnly = makeConfig(pass.autofill, [pass], inputMethod.TextInputType.TEXT);
      const nextWithoutAutofill = makeConfig(undefined, [], inputMethod.TextInputType.TEXT);

      // L1731 T: needsAutofill() false (nothing configured yet)
      expect(h.isSameAutofillGroup(passOnly)).assertEqual(false);

      activateAutofill(h, 120); // needsAutofill() now true, prevFields=[pass]
      // L1731 F branch: next config autofill undefined -> true -> false
      expect(h.isSameAutofillGroup(nextWithoutAutofill)).assertEqual(false);
      // L1736 T: next fields empty
      expect(h.isSameAutofillGroup(makeConfig(pass.autofill, [], inputMethod.TextInputType.TEXT))).assertEqual(false);
      // L1740 T: length differs (2 vs 1)
      expect(h.isSameAutofillGroup(makeConfig(pass.autofill, [user, pass], inputMethod.TextInputType.TEXT)))
        .assertEqual(false);
      // L1745 T: next field without autofill
      expect(h.isSameAutofillGroup(makeConfig(pass.autofill, [nextWithoutAutofill], inputMethod.TextInputType.TEXT)))
        .assertEqual(false);
      // L1750 T: next field id unknown
      expect(h.isSameAutofillGroup(makeConfig(pass.autofill, [makeFieldConfig('other', ['other'], 'x')],
        inputMethod.TextInputType.TEXT))).assertEqual(false);
      // L1750 F: same group -> true
      expect(h.isSameAutofillGroup(passOnly)).assertEqual(true);
    })

    it('updateConfig_null_is_noop', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      const before: ESObject = h.configuration;
      h.updateConfig(null);
      expect(h.configuration).assertEqual(before);
    })

    it('updateConfig_applies_without_autofill', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      h.updateConfig(makeConfig(undefined, [], inputMethod.TextInputType.TEXT));
      expect(h.configuration).not().assertNull();
      expect(h.textConfig.inputAttribute.textInputType).assertEqual(inputMethod.TextInputType.TEXT);
    })

    it('updateConfig_with_autofill', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      const pass = makeFieldConfig('pass', ['password'], 'secret');
      h.updateConfig(makeConfig(pass.autofill, [pass], inputMethod.TextInputType.TEXT));
      expect(h.autofillConfiguration).not().assertNull();
    })

    it('canShowTextInput_both_arms', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      expect(h.canShowTextInput()).assertEqual(true); // config null -> true
      h.setClient(130, makeConfig(undefined, [], inputMethod.TextInputType.NONE));
      expect(h.canShowTextInput()).assertEqual(false); // type NONE -> false
    })

    it('clearTextInputClient_platform_view_returns', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      h.setPlatformViewClient(500, true); // VIRTUAL_DISPLAY_PLATFORM_VIEW
      h.clearTextInputClient();
      expect(h.inputTarget.id).assertEqual(500); // untouched
    })

    it('clearTextInputClient_framework_resets', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      h.setClient(501, makeConfig(undefined, [], inputMethod.TextInputType.TEXT));
      h.clearTextInputClient();
      expect(h.inputTarget.id).assertEqual(0); // NO_TARGET
      expect(h.inputTarget.type).assertEqual(0); // Type.NO_TARGET
    })

    it('clearPlatformViewClient_non_matching_returns', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      h.setPlatformViewClient(600, false); // PHYSICAL_DISPLAY_PLATFORM_VIEW
      h.clearPlatformViewClient(601);
      expect(h.inputTarget.id).assertEqual(600); // untouched
    })

    it('clearPlatformViewClient_matching_releases', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      h.setPlatformViewClient(601, false);
      h.clearPlatformViewClient(601);
      expect(h.inputTarget.id).assertEqual(0);
      expect(h.keyboardStatus).assertEqual(inputMethod.KeyboardStatus.HIDE);
    })

    it('clearPlatformViewClientForTeardown_non_matching_returns', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      h.setPlatformViewClient(700, false);
      h.clearPlatformViewClientForTeardown(701);
      expect(h.inputTarget.id).assertEqual(700); // untouched
    })

    it('clearPlatformViewClientForTeardown_matching_releases', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      h.setPlatformViewClient(701, false);
      h.clearPlatformViewClientForTeardown(701);
      expect(h.inputTarget.id).assertEqual(0);
      expect(h.keyboardStatus).assertEqual(inputMethod.KeyboardStatus.HIDE);
    })

    it('applyKeyboardStatus_hide_autofill_active_returns', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      h.autoFillSessionActive = true;
      h.pendingTeardownPlatformViewClientId = 5;
      h.applyKeyboardStatus(inputMethod.KeyboardStatus.HIDE);
      expect(h.pendingTeardownPlatformViewClientId).assertEqual(5); // returned early
    })

    it('applyKeyboardStatus_hide_ime_changed_returns', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      h.setClient(800, makeConfig(undefined, [], inputMethod.TextInputType.TEXT));
      h.previousInputType = inputMethod.TextInputType.NUMBER; // differs from config TEXT
      h.pendingTeardownPlatformViewClientId = 7;
      h.applyKeyboardStatus(inputMethod.KeyboardStatus.HIDE);
      expect(h.pendingTeardownPlatformViewClientId).assertEqual(7); // L1509 T returned
    })

    it('applyKeyboardStatus_hide_sends_connection_closed', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      h.setClient(801, makeConfig(undefined, [], inputMethod.TextInputType.TEXT));
      h.previousInputType = inputMethod.TextInputType.TEXT; // matches config
      h.pendingTeardownPlatformViewClientId = 7;
      h.applyKeyboardStatus(inputMethod.KeyboardStatus.HIDE);
      expect(h.pendingTeardownPlatformViewClientId).assertEqual(-1); // reached L1518
    })

    it('applyKeyboardStatus_show_sets_state', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      h.applyKeyboardStatus(inputMethod.KeyboardStatus.SHOW);
      expect(h.keyboardStatus).assertEqual(inputMethod.KeyboardStatus.SHOW);
    })

    it('notifyViewExited_active_resets', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      h.autoFillSessionActive = true;
      h.autoFillRequestInFlight = true;
      h.autoFillRequestedClientId = 3;
      h.notifyViewExited();
      expect(h.autoFillSessionActive).assertEqual(false);
      expect(h.autoFillRequestInFlight).assertEqual(false);
      expect(h.autoFillRequestedClientId).assertEqual(-1);
    })

    it('updateAutofillConfigurationIfNeeded_matrix', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      const pass = makeFieldConfig('pass', ['password'], 'secret');
      const user = makeFieldConfig('user', ['username'], 'alice');
      const plainField = makeConfig(undefined, [], inputMethod.TextInputType.TEXT);

      // L1927 T: fields empty + autofillConfiguration null -> fresh map
      h.updateAutofillConfigurationIfNeeded(makeConfig(pass.autofill, [], inputMethod.TextInputType.TEXT));
      expect(h.autofillConfiguration).not().assertNull();
      expect(h.autofillConfiguration.get('pass')).not().assertNull();

      // L1926 F: fields non-empty; L1934 T skips field without autofill
      h.updateAutofillConfigurationIfNeeded(makeConfig(pass.autofill, [plainField, pass],
        inputMethod.TextInputType.TEXT));
      expect(h.autofillConfiguration.size).assertEqual(1);
      expect(h.autofillConfiguration.get('pass')).not().assertNull();

      // L1919 T: null config clears the map
      h.updateAutofillConfigurationIfNeeded(null);
      expect(h.autofillConfiguration).assertEqual(null);

      // full group rebuild (L1934 F for both fields)
      h.updateAutofillConfigurationIfNeeded(makeConfig(pass.autofill, [user, pass],
        inputMethod.TextInputType.TEXT));
      expect(h.autofillConfiguration.size).assertEqual(2);
    })

    it('autofill_applies_current_field', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      const pass = makeFieldConfig('pass', ['password'], 'secret');
      const user = makeFieldConfig('user', ['username'], 'alice');
      h.setClient(900, makeConfig(pass.autofill, [user, pass], inputMethod.TextInputType.TEXT));
      const values = new Map<number, string>();
      values.set(h.nodeIdFor('pass'), 'secret2');
      values.set(h.nodeIdFor('user'), 'alice2');
      let threw = false;
      try {
        textInputPlugin.autofill(values);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertFalse();
      expect(h.autofillFieldTexts.get('pass')).assertEqual('secret2');
      expect(h.autofillFieldTexts.get('user')).assertEqual('alice2');
    })

    it('maybeRunPendingAutofillRequest_matrix', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      // L1758 T: nothing pending
      h.maybeRunPendingAutofillRequest();

      activateAutofill(h, 950);
      // L1761 T: no valid rect
      h.pendingAutofillRequest = true;
      h.maybeRunPendingAutofillRequest();
      expect(h.pendingAutofillRequest).assertEqual(true);

      // L1764 T: suppressed -> clears both flags
      h.lastClientRect = { left: 0, top: 0, width: 100, height: 100 };
      h.autofillSuppressedClientId = 950;
      h.maybeRunPendingAutofillRequest();
      expect(h.pendingAutofillRequest).assertEqual(false);
      expect(h.pendingAutofillAfterImeAttach).assertEqual(false);

      // L1769 T: in flight -> return, pending kept
      h.autofillSuppressedClientId = -1;
      h.pendingAutofillRequest = true;
      h.autoFillRequestInFlight = true;
      h.maybeRunPendingAutofillRequest();
      expect(h.pendingAutofillRequest).assertEqual(true);

      // L1769 F: notifyViewEntered (flutterViewContext undefined -> safe early return)
      h.autoFillRequestInFlight = false;
      h.maybeRunPendingAutofillRequest();
      expect(h.pendingAutofillRequest).assertEqual(false);
    })

    it('rememberCurrentAutofillFieldText_records', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      // L1817 T: no autofill -> no record
      h.rememberCurrentAutofillFieldText();
      const pass = makeFieldConfig('pass', ['password'], 'secret');
      h.setClient(960, makeConfig(pass.autofill, [pass], inputMethod.TextInputType.TEXT));
      // L1817 F: records the (empty) current cache under the autofill id
      h.rememberCurrentAutofillFieldText();
      expect(h.autofillFieldTexts.get('pass')).assertEqual('');
    })

    it('setAutoFillFieldRect_matrix', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      const pass = makeFieldConfig('pass', ['password'], 'secret');
      h.setClient(970, makeConfig(pass.autofill, [pass], inputMethod.TextInputType.TEXT));
      // L541 T + L544 F: records rect, no pending
      h.setAutoFillFieldRect(0, 0, 100, 100);
      expect(h.lastClientRect).not().assertNull();
      expect(h.fieldRects.get('pass')).not().assertNull();

      // L546 T: pending + not attached -> pendingAutofillAfterImeAttach
      h.pendingAutofillRequest = true;
      h.setAutoFillFieldRect(0, 0, 100, 100);
      expect(h.pendingAutofillRequest).assertEqual(false);
      expect(h.pendingAutofillAfterImeAttach).assertEqual(true);

      // L546 F: pending + attached -> notifyViewEntered early-return path
      h.pendingAutofillRequest = true;
      h.isInputMethodAttached = true;
      h.setAutoFillFieldRect(0, 0, 100, 100);
      expect(h.pendingAutofillRequest).assertEqual(false);

      // L541 F: no autofill config
      h.setClient(971, makeConfig(undefined, [], inputMethod.TextInputType.TEXT));
      h.setAutoFillFieldRect(0, 0, 100, 100);
      expect(h.lastClientRect).not().assertNull();
    })
  })

  describe('TextInputPluginPreviewTest', () => {
    let textInputChannelMock: TextInputChannel;
    let textInputPlugin: TextInputPlugin;
    beforeEach(() => {
      let context: common.Context = getContext();
      const assetManager = context.resourceManager;
      const flutterNapi = FlutterInjector.getInstance().getFlutterNapi();
      const dartExecutor = new DartExecutor(flutterNapi, assetManager);
      textInputChannelMock = new TextInputChannel(dartExecutor);
      textInputPlugin = new TextInputPlugin(textInputChannelMock, "oh_flutter_preview_1", undefined);
    })

    it('finishPreview_clears_staging_and_pushes_cache', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      const fake = new FakeInputMethodController();
      h.inputMethodController = fake as Any as inputMethod.InputMethodController;
      stagePreview(h, "ni'hao", 'ni', 0, 2, 2, 2);
      let threw = false;
      try {
        h.finishPreviewTextCallback();
      } catch (e) {
        threw = true;
      }
      expect(threw).assertFalse();
      // Finish commits locally: preview cleared, staging off, cache intact.
      expect(h.mEditable.getPreviewText()).assertEqual('');
      expect(h.mEditable.isPreviewStagingActive()).assertEqual(false);
      expect(h.mEditable.getStringCache()).assertEqual("ni'hao");
      // The IMC push is deferred to the IME echo: finishPreviewTextCallback arms
      // mConsumeNextInsertTextIfEquals, and the follow-up insertText('ni') is consumed and
      // synced via syncImcSelectionAfterEditableChange -> changeSelection(cache, 2, 2).
      (h as ESObject).insertTextCallback('ni');
      expect(fake.lastSelectionText).assertEqual("ni'hao");
      expect(fake.lastSelectionStart).assertEqual(2);
      expect(fake.lastSelectionEnd).assertEqual(2);
    })

    it('notifySelectionToFramework_forwards_selection', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      const fake = new FakeInputMethodController();
      h.inputMethodController = fake as Any as inputMethod.InputMethodController;
      h.notifySelectionToFramework('abc', 2, 3);
      expect(fake.lastSelectionText).assertEqual('abc');
      expect(fake.lastSelectionStart).assertEqual(2);
      expect(fake.lastSelectionEnd).assertEqual(3);
    })

    it('getEditingState_returns_handler_editable', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      h.mEditable.setStringCache('proxy-cache');
      expect(textInputPlugin.getEditingState().getStringCache()).assertEqual('proxy-cache');
    })

    it('getKeyboardFocusState_reflects_flag', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      h.keyboardFocusState = true;
      expect(h.getKeyboardFocusState()).assertEqual(true);
      h.keyboardFocusState = false;
      expect(h.getKeyboardFocusState()).assertEqual(false);
    })

    it('setLastInputType_records_type', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      h.setLastInputType(inputMethod.TextInputType.NUMBER);
      expect(h.lastInputType).assertEqual(inputMethod.TextInputType.NUMBER);
      h.setLastInputType(undefined);
      expect(h.lastInputType).assertEqual(undefined);
    })

    it('cacheMethodCall_is_noop', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      let threw = false;
      try {
        h.cacheMethodCall();
      } catch (e) {
        threw = true;
      }
      expect(threw).assertFalse();
    })

    it('setAutoFillPageRect_records_rect', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      h.setAutoFillPageRect(1, 2, 300, 400);
      expect(h.lastPageRect.left).assertEqual(1);
      expect(h.lastPageRect.top).assertEqual(2);
      expect(h.lastPageRect.width).assertEqual(300);
      expect(h.lastPageRect.height).assertEqual(400);
    })

    it('hasSecureKeyboardInSwitch_compares_last_input_type', 0, () => {
      const h: ESObject = textInputChannelMock.textInputMethodHandler!;
      const fake = new FakeInputMethodController();
      h.inputMethodController = fake as Any as inputMethod.InputMethodController;
      h.setLastInputType(inputMethod.TextInputType.TEXT);
      h.configuration = makeConfig(undefined, [], inputMethod.TextInputType.TEXT);
      expect(h.hasSecureKeyboardInSwitch()).assertEqual(false);
      h.setLastInputType(inputMethod.TextInputType.VISIBLE_PASSWORD);
      expect(h.hasSecureKeyboardInSwitch()).assertEqual(true);
    })
  })
}
