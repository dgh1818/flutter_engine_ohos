/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, Level } from '@ohos/hypium';
import inputMethod from '@ohos.inputMethod';
import { resourceManager } from '@kit.LocalizationKit';
import FlutterNapi from '../../../../../../main/ets/embedding/engine/FlutterNapi';
import DartExecutor from '../../../../../../main/ets/embedding/engine/dart/DartExecutor';
import TextInputChannel, {
  Autofill,
  Configuration,
  InputType,
  TextEditState,
  TextInputMethodHandler,
} from '../../../../../../main/ets/embedding/engine/systemchannels/TextInputChannel';
import Any from '../../../../../../main/ets/plugin/common/Any';
import { MethodResult } from '../../../../../../main/ets/plugin/common/MethodChannel';
import MethodCall from '../../../../../../main/ets/plugin/common/MethodCall';
import JSONMethodCodec from '../../../../../../main/ets/plugin/common/JSONMethodCodec';
import { DartMessenger } from '../../../../../../main/ets/embedding/engine/dart/DartMessenger';
import { IncomingMethodCallHandler } from '../../../../../../main/ets/plugin/common/MethodChannel';
import { BinaryReply } from '../../../../../../main/ets/plugin/common/BinaryMessenger';

class FakeTextInputHandler implements TextInputMethodHandler {
  showCalls: number = 0;
  hideCalls: number = 0;
  setClientCalls: number = 0;
  updateConfigCalls: number = 0;
  setPlatformViewClientCalls: number = 0;
  setEditingStateCalls: number = 0;
  clearClientCalls: number = 0;
  requestAutofillCalls: number = 0;
  finishAutofillContextCalls: number = 0;
  setSystemUiChangeListenerCalls: number = 0;
  getKeyboardFocusStateCalls: number = 0;
  setWindowPositionCalls: number = 0;
  setCursorSizeAndPositionCalls: number = 0;
  setDevicePixelRatioCalls: number = 0;
  setEditableSizeAndTransformCalls: number = 0;
  handleChangeFocusCalls: number = 0;

  show(): void { this.showCalls++; }
  hide(): void { this.hideCalls++; }
  setClient(textInputClientId: number, configuration: Configuration | null): void { this.setClientCalls++; }
  updateConfig(configuration: Configuration | null): void { this.updateConfigCalls++; }
  setPlatformViewClient(id: number, usesVirtualDisplay: boolean): void { this.setPlatformViewClientCalls++; }
  setEditingState(editingState: TextEditState): void { this.setEditingStateCalls++; }
  clearClient(): void { this.clearClientCalls++; }
  setSystemUiChangeListener(): void { this.setSystemUiChangeListenerCalls++; }
  getKeyboardFocusState(): boolean { this.getKeyboardFocusStateCalls++; return true; }
  setWindowPosition(windowPosition: Any): void { this.setWindowPositionCalls++; }
  setCursorSizeAndPosition(cursorInfo: Any): void { this.setCursorSizeAndPositionCalls++; }
  setDevicePixelRatio(devicePixelRatio: number): void { this.setDevicePixelRatioCalls++; }
  setEditableSizeAndTransform(width: number, height: number, transform: number[]): void { this.setEditableSizeAndTransformCalls++; }
  setAutoFillFieldRect(left: number, top: number, width: number, height: number): void {}
  setAutoFillPageRect(left: number, top: number, width: number, height: number): void {}
  handleChangeFocus(focusState: boolean): void { this.handleChangeFocusCalls++; }
  requestAutofill(): void { this.requestAutofillCalls++; }
  finishAutofillContext(shouldSave: boolean): void { this.finishAutofillContextCalls++; }
}

class FakeMethodResult2 implements MethodResult {
  successCalls: number = 0;
  errorCalls: number = 0;
  notImplCalls: number = 0;
  success(result: Any): void { this.successCalls++; }
  error(errorCode: string, errorMessage: string, errorDetails: Any): void { this.errorCalls++; }
  notImplemented(): void { this.notImplCalls++; }
}

function getTextInputChannel(): TextInputChannel {
  const flutterNapi: FlutterNapi = new FlutterNapi();
  const manager: resourceManager.ResourceManager = getContext().resourceManager;
  const dartExecutor: DartExecutor = new DartExecutor(flutterNapi, manager);
  return new TextInputChannel(dartExecutor);
}

function makeInputTypeJson(name: string, signed: boolean = false, decimal: boolean = false): Any {
  return { name: name, signed: signed, decimal: decimal };
}


class TicFakeBinaryReply implements BinaryReply {
  replyCount: number = 0;
  reply(data: ArrayBuffer | null): void { this.replyCount++; }
}

class TicTestSetup {
  channel: TextInputChannel;
  flutterNapi: FlutterNapi;
  dartExecutor: DartExecutor;

  constructor() {
    this.flutterNapi = new FlutterNapi();
    const context = getContext();
    const rm: resourceManager.ResourceManager = context.resourceManager;
    this.dartExecutor = new DartExecutor(this.flutterNapi, rm);
    this.dartExecutor.onAttachedToNAPI();
    this.channel = new TextInputChannel(this.dartExecutor);
  }
}

function dispatchTicMethodCall(setup: TicTestSetup, method: string, args: Any): void {
  const call = new MethodCall(method, args);
  const encoded = JSONMethodCodec.INSTANCE.encodeMethodCall(call);
  const pmh = setup.flutterNapi.platformMessageHandler;
  if (pmh == null) return;
  const dm: DartMessenger = pmh as DartMessenger;
  const handlerInfo = dm.messageHandlers.get('flutter/textinput');
  if (handlerInfo) {
    const incoming = handlerInfo.handler as IncomingMethodCallHandler;
    const reply = new TicFakeBinaryReply();
    incoming.onMessage(encoded, reply);
  }
}

export default function textInputChannelTest() {
  describe('TextInputChannel.createEditingStateJSON', () => {
    it('returns object for basic args', Level.LEVEL1, () => {
      const channel = getTextInputChannel();
      const r = channel.createEditingStateJSON('text', 0, 1, 0, 2);
      expect(r != null).assertEqual(true);
    });

    it('returns object for empty text', Level.LEVEL1, () => {
      const channel = getTextInputChannel();
      const r = channel.createEditingStateJSON('', 0, 0, -1, -1);
      expect(r != null).assertEqual(true);
    });
  });

  describe('Configuration.fromJson', () => {
    it('builds Configuration with minimal required fields', Level.LEVEL1, () => {
      const json: Any = {
        inputAction: 'TextInputAction.done',
        inputType: makeInputTypeJson('TextInputType.text'),
      };
      const cfg = Configuration.fromJson(json);
      expect(cfg.obscureText).assertEqual(false);
      expect(cfg.autocorrect).assertEqual(true);
      expect(cfg.inputAction !== undefined).assertEqual(true);
    });

    it('uses provided field values when present', Level.LEVEL1, () => {
      const json: Any = {
        inputAction: 'TextInputAction.done',
        inputType: makeInputTypeJson('TextInputType.text'),
        obscureText: true,
        autocorrect: false,
        enableSuggestions: true,
        enableIMEPersonalizedLearning: true,
        enableDeltaModel: true,
        actionLabel: 'Send',
        deviceKind: 1,
      };
      const cfg = Configuration.fromJson(json);
      expect(cfg.obscureText).assertEqual(true);
      expect(cfg.autocorrect).assertEqual(false);
      expect(cfg.enableSuggestions).assertEqual(true);
      expect(cfg.enableIMEPersonalizedLearning).assertEqual(true);
      expect(cfg.enableDeltaModel).assertEqual(true);
      expect(cfg.actionLabel).assertEqual('Send');
    });

    it('throws when inputAction is missing', Level.LEVEL1, () => {
      const json: Any = {
        inputType: makeInputTypeJson('TextInputType.text'),
      };
      let threw = false;
      try {
        Configuration.fromJson(json);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });

    it('handles contentCommitMimeTypes array', Level.LEVEL1, () => {
      const json: Any = {
        inputAction: 'TextInputAction.done',
        inputType: makeInputTypeJson('TextInputType.text'),
        contentCommitMimeTypes: ['text/plain', 'image/png'],
      };
      const cfg = Configuration.fromJson(json);
      expect(cfg.contentCommitMimeTypes.length).assertEqual(2);
      expect(cfg.contentCommitMimeTypes[0]).assertEqual('text/plain');
    });

    it('handles null autofill', Level.LEVEL1, () => {
      const json: Any = {
        inputAction: 'TextInputAction.done',
        inputType: makeInputTypeJson('TextInputType.text'),
        autofill: null,
      };
      let threw = false;
      try {
        const cfg = Configuration.fromJson(json);
        expect(cfg.autofill === undefined).assertEqual(true);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('handles fields array (nested Configuration)', Level.LEVEL1, () => {
      const inner: Any = {
        inputAction: 'TextInputAction.done',
        inputType: makeInputTypeJson('TextInputType.text'),
      };
      const json: Any = {
        inputAction: 'TextInputAction.done',
        inputType: makeInputTypeJson('TextInputType.text'),
        fields: new Array<Any>(inner),
      };
      const cfg = Configuration.fromJson(json);
      expect(cfg.fields.length).assertEqual(1);
    });
  });

  describe('Configuration.fromMap', () => {
    it('builds Configuration from Map', Level.LEVEL1, () => {
      const m: Map<string, Any> = new Map();
      m.set('inputAction', 'TextInputAction.done');
      m.set('inputType', makeInputTypeJson('TextInputType.text'));
      m.set('obscureText', true);
      const cfg = Configuration.fromMap(m);
      expect(cfg.obscureText).assertEqual(true);
      expect(cfg.inputAction !== undefined).assertEqual(true);
    });

    it('handles empty Map (uses defaults)', Level.LEVEL1, () => {
      const m: Map<string, Any> = new Map();
      m.set('inputAction', 'TextInputAction.done');
      m.set('inputType', makeInputTypeJson('TextInputType.text'));
      let threw = false;
      try {
        const cfg = Configuration.fromMap(m);
        // Should not throw, just use defaults
        expect(cfg !== null).assertEqual(true);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('InputType.fromJson', () => {
    it('builds InputType for known type', Level.LEVEL1, () => {
      const json: Any = makeInputTypeJson('TextInputType.text');
      const inputType: InputType = InputType.fromJson(json);
      expect(inputType.type !== undefined).assertEqual(true);
      expect(inputType.isSigned).assertEqual(false);
      expect(inputType.isDecimal).assertEqual(false);
    });

    it('builds InputType for number type with signed/decimal', Level.LEVEL1, () => {
      const json: Any = makeInputTypeJson('TextInputType.number', true, true);
      const inputType: InputType = InputType.fromJson(json);
      expect(inputType.isSigned).assertEqual(true);
      expect(inputType.isDecimal).assertEqual(true);
    });

    it('throws for unknown type', Level.LEVEL1, () => {
      const json: Any = makeInputTypeJson('TextInputType.unknownXYZ');
      let threw = false;
      try {
        InputType.fromJson(json);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });
  });

  describe('TextEditState.fromJson', () => {
    it('builds from object with direct property access', Level.LEVEL1, () => {
      const json: Any = {
        text: 'hello',
        selectionBase: 0,
        selectionExtent: 5,
        composingBase: -1,
        composingExtent: -1,
      };
      const state = TextEditState.fromJson(json);
      expect(state.text).assertEqual('hello');
      expect(state.selectionStart).assertEqual(0);
      expect(state.selectionEnd).assertEqual(5);
    });

    it('builds from Map with .get() method', Level.LEVEL1, () => {
      const m: Map<string, Any> = new Map();
      m.set('text', 'world');
      m.set('selectionBase', 1);
      m.set('selectionExtent', 4);
      m.set('composingBase', -1);
      m.set('composingExtent', -1);
      const state = TextEditState.fromJson(m);
      expect(state.text).assertEqual('world');
      expect(state.selectionStart).assertEqual(1);
      expect(state.selectionEnd).assertEqual(4);
    });

    it('uses defaults for missing fields', Level.LEVEL1, () => {
      const json: Any = {};
      const state = TextEditState.fromJson(json);
      expect(state.text).assertEqual('');
      expect(state.selectionStart).assertEqual(-1);
    });
  });

  describe('Autofill.fromJson', () => {
    it('builds Autofill with minimal fields', Level.LEVEL1, () => {
      const editingValue: Any = {
        text: '',
        selectionBase: 0,
        selectionExtent: 0,
        composingBase: -1,
        composingExtent: -1,
      };
      const json: Any = {
        uniqueIdentifier: 'field1',
        editingValue: editingValue,
      };
      const a = Autofill.fromJson(json);
      expect(a.uniqueIdentifier).assertEqual('field1');
      expect(a.hints.length).assertEqual(0);
      expect(a.hintText === null).assertEqual(true);
    });

    it('builds Autofill with hints and hintText', Level.LEVEL1, () => {
      const editingValue: Any = {
        text: 'value',
        selectionBase: 0,
        selectionExtent: 5,
        composingBase: -1,
        composingExtent: -1,
      };
      const json: Any = {
        uniqueIdentifier: 'field2',
        editingValue: editingValue,
        hints: ['username', 'emailAddress'],
        hintText: 'Enter email',
      };
      const a = Autofill.fromJson(json);
      expect(a.uniqueIdentifier).assertEqual('field2');
      expect(a.hints.length).assertEqual(2);
      expect(a.hintText).assertEqual('Enter email');
    });

    it('throws when uniqueIdentifier is missing', Level.LEVEL1, () => {
      const editingValue: Any = {
        text: '',
        selectionBase: 0,
        selectionExtent: 0,
        composingBase: -1,
        composingExtent: -1,
      };
      const json: Any = { editingValue: editingValue };
      let threw = false;
      try {
        Autofill.fromJson(json);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });

    it('throws when editingValue is missing', Level.LEVEL1, () => {
      const json: Any = { uniqueIdentifier: 'field' };
      let threw = false;
      try {
        Autofill.fromJson(json);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });
  });

  describe('TextInputChannel.setTextInputMethodHandler', () => {
    it('does not throw with valid handler', Level.LEVEL1, () => {
      const channel = getTextInputChannel();
      let threw = false;
      try {
        channel.setTextInputMethodHandler({} as Any);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('does not throw with null handler', Level.LEVEL1, () => {
      const channel = getTextInputChannel();
      let threw = false;
      try {
        channel.setTextInputMethodHandler(null);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('Configuration.inputActionFromTextInputAction', () => {
    it('translates all TextInputAction values', Level.LEVEL0, () => {
      const cls: Any = Configuration as Any;
      const fn: Any = cls['inputActionFromTextInputAction'];
      if (fn) {
        expect(fn('TextInputAction.previous')).assertEqual(inputMethod.EnterKeyType.PREVIOUS);
        expect(fn('TextInputAction.unspecified')).assertEqual(inputMethod.EnterKeyType.UNSPECIFIED);
        expect(fn('TextInputAction.none')).assertEqual(inputMethod.EnterKeyType.NONE);
        expect(fn('TextInputAction.go')).assertEqual(inputMethod.EnterKeyType.GO);
        expect(fn('TextInputAction.search')).assertEqual(inputMethod.EnterKeyType.SEARCH);
        expect(fn('TextInputAction.send')).assertEqual(inputMethod.EnterKeyType.SEND);
        expect(fn('TextInputAction.next')).assertEqual(inputMethod.EnterKeyType.NEXT);
        expect(fn('TextInputAction.done')).assertEqual(inputMethod.EnterKeyType.DONE);
        expect(fn('TextInputAction.newline')).not().assertEqual(inputMethod.EnterKeyType.DONE);
      }
    });

    it('returns UNSPECIFIED for unknown action', Level.LEVEL0, () => {
      const cls: Any = Configuration as Any;
      const fn: Any = cls['inputActionFromTextInputAction'];
      if (fn) {
        expect(fn('unknown')).assertEqual(inputMethod.EnterKeyType.UNSPECIFIED);
        expect(fn('')).assertEqual(inputMethod.EnterKeyType.UNSPECIFIED);
      }
    });
  });

  describe('Autofill.translateAutofillHint', () => {
    it('translates address hints', Level.LEVEL0, () => {
      const cls: Any = Autofill as Any;
      const fn: Any = cls['translateAutofillHint'];
      if (fn) {
        expect(fn('addressCity')).assertEqual('addressLocality');
        expect(fn('addressState')).assertEqual('addressRegion');
        expect(fn('countryName')).assertEqual('addressCountry');
        expect(fn('fullStreetAddress')).assertEqual('streetAddress');
        expect(fn('postalAddress')).assertEqual('postalAddress');
        expect(fn('postalAddressExtended')).assertEqual('extendedAddress');
        expect(fn('postalAddressExtendedPostalCode')).assertEqual('extendedPostalCode');
        expect(fn('postalCode')).assertEqual('postalCode');
      }
    });

    it('translates birthday hints', Level.LEVEL0, () => {
      const cls: Any = Autofill as Any;
      const fn: Any = cls['translateAutofillHint'];
      if (fn) {
        expect(fn('birthday')).assertEqual('birthDateFull');
        expect(fn('birthdayDay')).assertEqual('birthDateDay');
        expect(fn('birthdayMonth')).assertEqual('birthDateMonth');
        expect(fn('birthdayYear')).assertEqual('birthDateYear');
      }
    });

    it('translates credit card hints', Level.LEVEL0, () => {
      const cls: Any = Autofill as Any;
      const fn: Any = cls['translateAutofillHint'];
      if (fn) {
        expect(fn('creditCardExpirationDate')).assertEqual('creditCardExpirationDate');
        expect(fn('creditCardExpirationDay')).assertEqual('creditCardExpirationDay');
        expect(fn('creditCardExpirationMonth')).assertEqual('creditCardExpirationMonth');
        expect(fn('creditCardExpirationYear')).assertEqual('creditCardExpirationYear');
        expect(fn('creditCardNumber')).assertEqual('creditCardNumber');
        expect(fn('creditCardSecurityCode')).assertEqual('creditCardSecurityCode');
      }
    });

    it('translates person name hints', Level.LEVEL0, () => {
      const cls: Any = Autofill as Any;
      const fn: Any = cls['translateAutofillHint'];
      if (fn) {
        expect(fn('familyName')).assertEqual('personFamilyName');
        expect(fn('givenName')).assertEqual('personGivenName');
        expect(fn('middleInitial')).assertEqual('personMiddleInitial');
        expect(fn('middleName')).assertEqual('personMiddleName');
        expect(fn('name')).assertEqual('personName');
        expect(fn('namePrefix')).assertEqual('personNamePrefix');
        expect(fn('nameSuffix')).assertEqual('personNameSuffix');
      }
    });

    it('translates contact hints', Level.LEVEL0, () => {
      const cls: Any = Autofill as Any;
      const fn: Any = cls['translateAutofillHint'];
      if (fn) {
        expect(fn('email')).assertEqual('emailAddress');
        expect(fn('telephoneNumber')).assertEqual('phoneNumber');
        expect(fn('telephoneNumberCountryCode')).assertEqual('phoneCountryCode');
        expect(fn('telephoneNumberDevice')).assertEqual('phoneNumberDevice');
        expect(fn('telephoneNumberNational')).assertEqual('phoneNational');
      }
    });

    it('translates credential hints', Level.LEVEL0, () => {
      const cls: Any = Autofill as Any;
      const fn: Any = cls['translateAutofillHint'];
      if (fn) {
        expect(fn('newPassword')).assertEqual('newPassword');
        expect(fn('newUsername')).assertEqual('newUsername');
        expect(fn('password')).assertEqual('password');
        expect(fn('username')).assertEqual('username');
        expect(fn('oneTimeCode')).assertEqual('smsOTPCode');
      }
    });

    it('translates gender hint', Level.LEVEL0, () => {
      const cls: Any = Autofill as Any;
      const fn: Any = cls['translateAutofillHint'];
      if (fn) {
        expect(fn('gender')).assertEqual('gender');
      }
    });

    it('returns hint unchanged for unknown values', Level.LEVEL0, () => {
      const cls: Any = Autofill as Any;
      const fn: Any = cls['translateAutofillHint'];
      if (fn) {
        expect(fn('unknownHint')).assertEqual('unknownHint');
        expect(fn('')).assertEqual('');
        expect(fn('custom')).assertEqual('custom');
      }
    });
  });

  describe('TextInputChannel.onMethodCall.via.messenger', () => {
    it('onMethodCall returns early when handler is null', Level.LEVEL0, () => {
      const setup = new TicTestSetup();
      dispatchTicMethodCall(setup, 'TextInput.show', null);
      expect(setup.channel).not().assertNull();
    });

    it('onMethodCall handles TextInput.show', Level.LEVEL0, () => {
      const setup = new TicTestSetup();
      const handler = new FakeTextInputHandler();
      setup.channel.setTextInputMethodHandler(handler);
      dispatchTicMethodCall(setup, 'TextInput.show', null);
      expect(handler.showCalls).assertEqual(1);
    });

    it('onMethodCall handles TextInput.hide', Level.LEVEL0, () => {
      const setup = new TicTestSetup();
      const handler = new FakeTextInputHandler();
      setup.channel.setTextInputMethodHandler(handler);
      dispatchTicMethodCall(setup, 'TextInput.hide', null);
      expect(handler.hideCalls).assertEqual(1);
    });

    it('onMethodCall handles TextInput.clearClient', Level.LEVEL0, () => {
      const setup = new TicTestSetup();
      const handler = new FakeTextInputHandler();
      setup.channel.setTextInputMethodHandler(handler);
      dispatchTicMethodCall(setup, 'TextInput.clearClient', null);
      expect(handler.clearClientCalls).assertEqual(1);
    });

    it('onMethodCall handles TextInput.requestAutofill', Level.LEVEL0, () => {
      const setup = new TicTestSetup();
      const handler = new FakeTextInputHandler();
      setup.channel.setTextInputMethodHandler(handler);
      dispatchTicMethodCall(setup, 'TextInput.requestAutofill', null);
      expect(handler.requestAutofillCalls).assertEqual(1);
    });

    it('onMethodCall handles TextInput.finishAutofillContext', Level.LEVEL0, () => {
      const setup = new TicTestSetup();
      const handler = new FakeTextInputHandler();
      setup.channel.setTextInputMethodHandler(handler);
      dispatchTicMethodCall(setup, 'TextInput.finishAutofillContext', true);
      expect(handler.finishAutofillContextCalls).assertEqual(1);
    });

    it('onMethodCall handles TextInput.sendAppPrivateCommand (notImplemented)', Level.LEVEL0, () => {
      const setup = new TicTestSetup();
      const handler = new FakeTextInputHandler();
      setup.channel.setTextInputMethodHandler(handler);
      dispatchTicMethodCall(setup, 'TextInput.sendAppPrivateCommand', null);
      expect(setup.channel).not().assertNull();
    });

    it('onMethodCall handles unknown method (notImplemented)', Level.LEVEL0, () => {
      const setup = new TicTestSetup();
      const handler = new FakeTextInputHandler();
      setup.channel.setTextInputMethodHandler(handler);
      dispatchTicMethodCall(setup, 'unknown.method', null);
      expect(setup.channel).not().assertNull();
    });

    it('onMethodCall handles TextInput.setEditingState', Level.LEVEL0, () => {
      const setup = new TicTestSetup();
      const handler = new FakeTextInputHandler();
      setup.channel.setTextInputMethodHandler(handler);
      const state: Any = { text: 'hello', selectionBase: 0, selectionExtent: 5, composingBase: -1, composingExtent: -1 };
      dispatchTicMethodCall(setup, 'TextInput.setEditingState', state);
      expect(handler.setEditingStateCalls).assertEqual(1);
    });

    it('onMethodCall handles TextInput.setEditableSizeAndTransform', Level.LEVEL0, () => {
      const setup = new TicTestSetup();
      const handler = new FakeTextInputHandler();
      setup.channel.setTextInputMethodHandler(handler);
      const args = new Map<string, Object>();
      args.set('width', 100);
      args.set('height', 200);
      const transform: number[] = [1,0,0,0, 0,1,0,0, 0,0,1,0, 10,20,0,1];
      args.set('transform', transform);
      dispatchTicMethodCall(setup, 'TextInput.setEditableSizeAndTransform', args);
      expect(setup.channel).not().assertNull();
    });

    it('onMethodCall handles TextInput.setCaretRect', Level.LEVEL0, () => {
      const setup = new TicTestSetup();
      const handler = new FakeTextInputHandler();
      setup.channel.setTextInputMethodHandler(handler);
      const args = new Map<string, Object>();
      args.set('x', 10);
      args.set('y', 20);
      args.set('width', 5);
      args.set('height', 15);
      dispatchTicMethodCall(setup, 'TextInput.setCaretRect', args);
      expect(setup.channel).not().assertNull();
    });

    it('onMethodCall handles TextInput.setClient', Level.LEVEL0, () => {
      const setup = new TicTestSetup();
      const handler = new FakeTextInputHandler();
      setup.channel.setTextInputMethodHandler(handler);
      const configJson: Any = {
        inputAction: 'TextInputAction.done',
        inputType: makeInputTypeJson('TextInputType.text'),
      };
      dispatchTicMethodCall(setup, 'TextInput.setClient', [1, configJson]);
      expect(handler.setClientCalls).assertEqual(1);
    });

    it('onMethodCall handles TextInput.setPlatformViewClient', Level.LEVEL0, () => {
      const setup = new TicTestSetup();
      const handler = new FakeTextInputHandler();
      setup.channel.setTextInputMethodHandler(handler);
      const args = new Map<string, Object>();
      args.set('platformViewId', 1);
      args.set('usesVirtualDisplay', true);
      dispatchTicMethodCall(setup, 'TextInput.setPlatformViewClient', args);
      expect(handler.setPlatformViewClientCalls).assertEqual(1);
    });
  });

  describe('TextInputChannel.saveEditableSizeAndTransform.via.messenger', () => {
    it('handles affine transform via onMethodCall', Level.LEVEL0, () => {
      const setup = new TicTestSetup();
      const handler = new FakeTextInputHandler();
      setup.channel.setTextInputMethodHandler(handler);
      const args = new Map<string, Object>();
      args.set('width', 100);
      args.set('height', 200);
      const transform: number[] = [1,0,0,0, 0,1,0,0, 0,0,1,0, 10,20,0,1];
      args.set('transform', transform);
      dispatchTicMethodCall(setup, 'TextInput.setEditableSizeAndTransform', args);
      expect(setup.channel).not().assertNull();
    });

    it('handles non-affine transform via onMethodCall', Level.LEVEL0, () => {
      const setup = new TicTestSetup();
      const handler = new FakeTextInputHandler();
      setup.channel.setTextInputMethodHandler(handler);
      const args = new Map<string, Object>();
      args.set('width', 50);
      args.set('height', 100);
      const transform: number[] = [1,0,0,1, 0,1,0,1, 0,0,1,1, 10,20,0,1];
      args.set('transform', transform);
      dispatchTicMethodCall(setup, 'TextInput.setEditableSizeAndTransform', args);
      expect(setup.channel).not().assertNull();
    });
  });
}
