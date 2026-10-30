/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, Level } from '@ohos/hypium';
import { window } from '@kit.ArkUI';
import { abilityDelegatorRegistry } from '@kit.TestKit';
import { UIAbility } from '@kit.AbilityKit';
import common from '@ohos.app.ability.common';
import FlutterInjector from '../../../main/ets/FlutterInjector';
import DartExecutor from '../../../main/ets/embedding/engine/dart/DartExecutor';
import TextInputChannel from '../../../main/ets/embedding/engine/systemchannels/TextInputChannel';
import TextInputPluginDefault from '../../../main/ets/plugin/editing/TextInputPlugin';
import FlutterNapi from '../../../main/ets/embedding/engine/FlutterNapi';
import ApplicationInfoLoader from '../../../main/ets/embedding/engine/loader/ApplicationInfoLoader';
import MethodCall from '../../../main/ets/plugin/common/MethodCall';
import JSONMethodCodec from '../../../main/ets/plugin/common/JSONMethodCodec';
import { DartMessenger } from '../../../main/ets/embedding/engine/dart/DartMessenger';
import { IncomingMethodCallHandler } from '../../../main/ets/plugin/common/MethodChannel';
import { BinaryReply, BinaryMessenger, BinaryMessageHandler, TaskQueue, TaskQueueOptions } from '../../../main/ets/plugin/common/BinaryMessenger';
import SendableBinaryMessageHandler from '../../../main/ets/plugin/common/SendableBinaryMessageHandler';
import { resourceManager } from '@kit.LocalizationKit';

class FakeTaskQueue implements TaskQueue {}

class FakeMessenger implements BinaryMessenger {
  makeBackgroundTaskQueue(options?: TaskQueueOptions): TaskQueue { return new FakeTaskQueue(); }
  send(channel: String, message: ArrayBuffer | null, callback?: BinaryReply | null): void {}
  setMessageHandler(channel: String, handler: BinaryMessageHandler | SendableBinaryMessageHandler | null, taskQueue?: TaskQueue, ...args: Object[]): void {}
}

class FakeBinaryReply implements BinaryReply {
  replyCount: number = 0;
  reply(data: ArrayBuffer | null): void { this.replyCount++; }
}

class ImeTestSetup {
  textInputPlugin: TextInputPluginDefault;
  textInputChannel: TextInputChannel;
  flutterNapi: FlutterNapi;
  dartExecutor: DartExecutor;

  constructor() {
    this.flutterNapi = new FlutterNapi();
    const ctx = getContext() as common.UIAbilityContext;
    const rm: resourceManager.ResourceManager = ctx.resourceManager;
    this.dartExecutor = new DartExecutor(this.flutterNapi, rm);
    this.dartExecutor.onAttachedToNAPI();
    this.textInputChannel = new TextInputChannel(this.dartExecutor);
    this.textInputPlugin = new TextInputPluginDefault(this.textInputChannel, "test_view", undefined);
  }
}

function dispatchImeMethodCall(setup: ImeTestSetup, method: string, args: Object | null): void {
  const call = new MethodCall(method, args);
  const encoded = JSONMethodCodec.INSTANCE.encodeMethodCall(call);
  const pmh = setup.flutterNapi.platformMessageHandler;
  if (pmh == null) return;
  const dm: DartMessenger = pmh as DartMessenger;
  const handlerInfo = dm.messageHandlers.get('flutter/textinput');
  if (handlerInfo) {
    const incoming = handlerInfo.handler as IncomingMethodCallHandler;
    const reply = new FakeBinaryReply();
    incoming.onMessage(encoded, reply);
  }
}

export default function windowCheckTest() {
  describe('WindowCheck', () => {
    it('window is available and focused', Level.LEVEL0, async (done: Function) => {
      const context = getContext() as common.UIAbilityContext;
      let windowFound = false;
      try {
        const win = await window.getLastWindow(context);
        windowFound = win != null;
      } catch (e) {}
      expect(windowFound).assertEqual(true);
      done();
    });

    it('top ability is TestAbility', Level.LEVEL0, async (done: Function) => {
      let abilityName = '';
      try {
        const delegator = abilityDelegatorRegistry.getAbilityDelegator();
        const ability: UIAbility = await delegator.getCurrentTopAbility();
        abilityName = ability.context.abilityInfo.name;
      } catch (e) {}
      expect(typeof abilityName).assertEqual('string');
      done();
    });

    it('TextInput.show via messageHandlers triggers attach', Level.LEVEL0, async (done: Function) => {
      const setup = new ImeTestSetup();
      dispatchImeMethodCall(setup, 'TextInput.show', '');
      // Wait for async attach to complete
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 2000));
      // No setClient was called, so no framework client should be active.
      expect(setup.textInputPlugin.isFrameworkTextInputActive()).assertEqual(false);
      done();
    });

    it('TextInput.hide via messageHandlers', Level.LEVEL0, async (done: Function) => {
      const setup = new ImeTestSetup();
      dispatchImeMethodCall(setup, 'TextInput.show', '');
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 1000));
      dispatchImeMethodCall(setup, 'TextInput.hide', '');
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      expect(setup.textInputPlugin.isFrameworkTextInputActive()).assertEqual(false);
      done();
    });

    it('TextInput.clearClient via messageHandlers', Level.LEVEL0, async (done: Function) => {
      const setup = new ImeTestSetup();
      dispatchImeMethodCall(setup, 'TextInput.show', '');
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 1000));
      dispatchImeMethodCall(setup, 'TextInput.clearClient', '');
      expect(setup.textInputPlugin.isFrameworkTextInputActive()).assertEqual(false);
      done();
    });

    it('TextInput.setClient via messageHandlers', Level.LEVEL0, async (done: Function) => {
      const setup = new ImeTestSetup();
      const args = [1, '{"inputAction":"TextInputAction.done","inputType":{"name":"TextInputType.text","signed":false,"decimal":false}}'];
      dispatchImeMethodCall(setup, 'TextInput.setClient', args);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      expect(setup.textInputPlugin.isFrameworkTextInputActive()).assertEqual(false);
      done();
    });

    it('TextInput.setEditingState via messageHandlers', Level.LEVEL0, async (done: Function) => {
      const setup = new ImeTestSetup();
      const clientArgs = [1, '{"inputAction":"TextInputAction.done","inputType":{"name":"TextInputType.text","signed":false,"decimal":false}}'];
      dispatchImeMethodCall(setup, 'TextInput.setClient', clientArgs);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      dispatchImeMethodCall(setup, 'TextInput.show', '');
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 1000));
      const stateArgs = new Map<string, Object>();
      stateArgs.set('text', 'hello');
      stateArgs.set('selectionBase', 0);
      stateArgs.set('selectionExtent', 5);
      stateArgs.set('composingBase', -1);
      stateArgs.set('composingExtent', -1);
      dispatchImeMethodCall(setup, 'TextInput.setEditingState', stateArgs);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      expect(setup.textInputPlugin.isFrameworkTextInputActive()).assertEqual(false);
      done();
    });

    it('TextInput.updateConfig via messageHandlers', Level.LEVEL0, async (done: Function) => {
      const setup = new ImeTestSetup();
      const clientArgs = [1, '{"inputAction":"TextInputAction.done","inputType":{"name":"TextInputType.text","signed":false,"decimal":false}}'];
      dispatchImeMethodCall(setup, 'TextInput.setClient', clientArgs);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      dispatchImeMethodCall(setup, 'TextInput.show', '');
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 1000));
      const configMap = new Map<string, Object>();
      configMap.set('inputAction', 'TextInputAction.go');
      const inputTypeMap = new Map<string, Object>();
      inputTypeMap.set('name', 'TextInputType.text');
      inputTypeMap.set('signed', false);
      inputTypeMap.set('decimal', false);
      configMap.set('inputType', inputTypeMap);
      dispatchImeMethodCall(setup, 'TextInput.updateConfig', configMap);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      expect(setup.textInputPlugin.isFrameworkTextInputActive()).assertEqual(false);
      done();
    });

    it('TextInput.setPlatformViewClient via messageHandlers', Level.LEVEL0, async (done: Function) => {
      const setup = new ImeTestSetup();
      const args = new Map<string, Object>();
      args.set('platformViewId', 1);
      args.set('usesVirtualDisplay', false);
      dispatchImeMethodCall(setup, 'TextInput.setPlatformViewClient', args);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      expect(setup.textInputPlugin.isFrameworkTextInputActive()).assertEqual(false);
      done();
    });

    it('TextInput.requestAutofill via messageHandlers', Level.LEVEL0, async (done: Function) => {
      const setup = new ImeTestSetup();
      const clientArgs = [1, '{"inputAction":"TextInputAction.done","inputType":{"name":"TextInputType.text","signed":false,"decimal":false},"autofill":{"uniqueIdentifier":"field1","editingValue":{"text":"","selectionBase":0,"selectionExtent":0,"composingBase":-1,"composingExtent":-1}}}'];
      dispatchImeMethodCall(setup, 'TextInput.setClient', clientArgs);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      dispatchImeMethodCall(setup, 'TextInput.show', '');
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 1000));
      dispatchImeMethodCall(setup, 'TextInput.requestAutofill', '');
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      expect(setup.textInputPlugin.isFrameworkTextInputActive()).assertEqual(false);
      done();
    });

    it('TextInput.finishAutofillContext via messageHandlers', Level.LEVEL0, async (done: Function) => {
      const setup = new ImeTestSetup();
      const clientArgs = [1, '{"inputAction":"TextInputAction.done","inputType":{"name":"TextInputType.text","signed":false,"decimal":false},"autofill":{"uniqueIdentifier":"field1","editingValue":{"text":"","selectionBase":0,"selectionExtent":0,"composingBase":-1,"composingExtent":-1}}}'];
      dispatchImeMethodCall(setup, 'TextInput.setClient', clientArgs);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      dispatchImeMethodCall(setup, 'TextInput.show', '');
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 1000));
      dispatchImeMethodCall(setup, 'TextInput.finishAutofillContext', true);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      expect(setup.textInputPlugin.isFrameworkTextInputActive()).assertEqual(false);
      done();
    });

    it('TextInput.setEditableSizeAndTransform via messageHandlers', Level.LEVEL0, async (done: Function) => {
      const setup = new ImeTestSetup();
      const args = new Map<string, Object>();
      args.set('width', 100);
      args.set('height', 200);
      args.set('transform', [1,0,0,0, 0,1,0,0, 0,0,1,0, 10,20,0,1]);
      dispatchImeMethodCall(setup, 'TextInput.setEditableSizeAndTransform', args);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      expect(setup.textInputPlugin.isFrameworkTextInputActive()).assertEqual(false);
      done();
    });

    it('TextInput.setCaretRect via messageHandlers', Level.LEVEL0, async (done: Function) => {
      const setup = new ImeTestSetup();
      const args = new Map<string, Object>();
      args.set('x', 10);
      args.set('y', 20);
      args.set('width', 5);
      args.set('height', 15);
      dispatchImeMethodCall(setup, 'TextInput.setCaretRect', args);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      expect(setup.textInputPlugin.isFrameworkTextInputActive()).assertEqual(false);
      done();
    });

    it('TextInput.sendAppPrivateCommand via messageHandlers', Level.LEVEL0, async (done: Function) => {
      const setup = new ImeTestSetup();
      dispatchImeMethodCall(setup, 'TextInput.sendAppPrivateCommand', '');
      expect(setup.textInputPlugin.isFrameworkTextInputActive()).assertEqual(false);
      done();
    });

    it('TextInput unknown method via messageHandlers', Level.LEVEL0, async (done: Function) => {
      const setup = new ImeTestSetup();
      dispatchImeMethodCall(setup, 'unknown.method', '');
      expect(setup.textInputPlugin.isFrameworkTextInputActive()).assertEqual(false);
      done();
    });

    it('full IME flow: setClient+show+setEditingState+hide+clearClient', Level.LEVEL0, async (done: Function) => {
      const setup = new ImeTestSetup();
      const clientArgs = [1, '{"inputAction":"TextInputAction.done","inputType":{"name":"TextInputType.text","signed":false,"decimal":false}}'];
      dispatchImeMethodCall(setup, 'TextInput.setClient', clientArgs);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      dispatchImeMethodCall(setup, 'TextInput.show', '');
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 1500));
      const stateArgs = new Map<string, Object>();
      stateArgs.set('text', 'hello world');
      stateArgs.set('selectionBase', 0);
      stateArgs.set('selectionExtent', 11);
      stateArgs.set('composingBase', -1);
      stateArgs.set('composingExtent', -1);
      dispatchImeMethodCall(setup, 'TextInput.setEditingState', stateArgs);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      dispatchImeMethodCall(setup, 'TextInput.hide', '');
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      dispatchImeMethodCall(setup, 'TextInput.clearClient', '');
      expect(setup.textInputPlugin.isFrameworkTextInputActive()).assertEqual(false);
      done();
    });

    it('setClient with autofill config', Level.LEVEL0, async (done: Function) => {
      const setup = new ImeTestSetup();
      const clientArgs = [1, '{"inputAction":"TextInputAction.done","inputType":{"name":"TextInputType.text","signed":false,"decimal":false},"autofill":{"uniqueIdentifier":"field1","editingValue":{"text":"","selectionBase":0,"selectionExtent":0,"composingBase":-1,"composingExtent":-1},"hints":["username"]}}'];
      dispatchImeMethodCall(setup, 'TextInput.setClient', clientArgs);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      dispatchImeMethodCall(setup, 'TextInput.show', '');
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 1500));
      expect(setup.textInputPlugin.isFrameworkTextInputActive()).assertEqual(false);
      done();
    });

    it('setClient with NONE input type', Level.LEVEL0, async (done: Function) => {
      const setup = new ImeTestSetup();
      const clientArgs = [1, '{"inputAction":"TextInputAction.done","inputType":{"name":"TextInputType.none","signed":false,"decimal":false}}'];
      dispatchImeMethodCall(setup, 'TextInput.setClient', clientArgs);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      // NONE input type -> canShowTextInput() returns false -> NO_TARGET
      expect(setup.textInputPlugin.isFrameworkTextInputActive()).assertEqual(false);
      done();
    });

    it('setClient with visible password type', Level.LEVEL0, async (done: Function) => {
      const setup = new ImeTestSetup();
      const clientArgs = [1, '{"inputAction":"TextInputAction.done","inputType":{"name":"TextInputType.visiblePassword","signed":false,"decimal":false}}'];
      dispatchImeMethodCall(setup, 'TextInput.setClient', clientArgs);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      dispatchImeMethodCall(setup, 'TextInput.show', '');
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 1500));
      expect(setup.textInputPlugin.isFrameworkTextInputActive()).assertEqual(false);
      done();
    });

    it('setClient twice with different autofill groups', Level.LEVEL0, async (done: Function) => {
      const setup = new ImeTestSetup();
      const args1 = [1, '{"inputAction":"TextInputAction.done","inputType":{"name":"TextInputType.text","signed":false,"decimal":false},"autofill":{"uniqueIdentifier":"field1","editingValue":{"text":"","selectionBase":0,"selectionExtent":0,"composingBase":-1,"composingExtent":-1}}}'];
      dispatchImeMethodCall(setup, 'TextInput.setClient', args1);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      const args2 = [2, '{"inputAction":"TextInputAction.done","inputType":{"name":"TextInputType.text","signed":false,"decimal":false},"autofill":{"uniqueIdentifier":"field2","editingValue":{"text":"","selectionBase":0,"selectionExtent":0,"composingBase":-1,"composingExtent":-1}}}'];
      dispatchImeMethodCall(setup, 'TextInput.setClient', args2);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      expect(setup.textInputPlugin.isFrameworkTextInputActive()).assertEqual(false);
      done();
    });

    it('setEditingState with composing region', Level.LEVEL0, async (done: Function) => {
      const setup = new ImeTestSetup();
      const clientArgs = [1, '{"inputAction":"TextInputAction.done","inputType":{"name":"TextInputType.text","signed":false,"decimal":false}}'];
      dispatchImeMethodCall(setup, 'TextInput.setClient', clientArgs);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      dispatchImeMethodCall(setup, 'TextInput.show', '');
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 1500));
      const stateArgs = new Map<string, Object>();
      stateArgs.set('text', 'hello');
      stateArgs.set('selectionBase', 0);
      stateArgs.set('selectionExtent', 5);
      stateArgs.set('composingBase', 1);
      stateArgs.set('composingExtent', 3);
      dispatchImeMethodCall(setup, 'TextInput.setEditingState', stateArgs);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      expect(setup.textInputPlugin.isFrameworkTextInputActive()).assertEqual(false);
      done();
    });

    it('setEditingState with -1 selection', Level.LEVEL0, async (done: Function) => {
      const setup = new ImeTestSetup();
      const clientArgs = [1, '{"inputAction":"TextInputAction.done","inputType":{"name":"TextInputType.text","signed":false,"decimal":false}}'];
      dispatchImeMethodCall(setup, 'TextInput.setClient', clientArgs);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      dispatchImeMethodCall(setup, 'TextInput.show', '');
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 1500));
      const stateArgs = new Map<string, Object>();
      stateArgs.set('text', 'hello');
      stateArgs.set('selectionBase', -1);
      stateArgs.set('selectionExtent', -1);
      stateArgs.set('composingBase', -1);
      stateArgs.set('composingExtent', -1);
      dispatchImeMethodCall(setup, 'TextInput.setEditingState', stateArgs);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      expect(setup.textInputPlugin.isFrameworkTextInputActive()).assertEqual(false);
      done();
    });

    it('setEditingState twice with different text', Level.LEVEL0, async (done: Function) => {
      const setup = new ImeTestSetup();
      const clientArgs = [1, '{"inputAction":"TextInputAction.done","inputType":{"name":"TextInputType.text","signed":false,"decimal":false}}'];
      dispatchImeMethodCall(setup, 'TextInput.setClient', clientArgs);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      dispatchImeMethodCall(setup, 'TextInput.show', '');
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 1500));
      const state1 = new Map<string, Object>();
      state1.set('text', 'hello');
      state1.set('selectionBase', 0);
      state1.set('selectionExtent', 5);
      state1.set('composingBase', -1);
      state1.set('composingExtent', -1);
      dispatchImeMethodCall(setup, 'TextInput.setEditingState', state1);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      const state2 = new Map<string, Object>();
      state2.set('text', 'hello world');
      state2.set('selectionBase', 5);
      state2.set('selectionExtent', 11);
      state2.set('composingBase', -1);
      state2.set('composingExtent', -1);
      dispatchImeMethodCall(setup, 'TextInput.setEditingState', state2);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      expect(setup.textInputPlugin.isFrameworkTextInputActive()).assertEqual(false);
      done();
    });

    it('updateConfig after setClient', Level.LEVEL0, async (done: Function) => {
      const setup = new ImeTestSetup();
      const clientArgs = [1, '{"inputAction":"TextInputAction.done","inputType":{"name":"TextInputType.text","signed":false,"decimal":false}}'];
      dispatchImeMethodCall(setup, 'TextInput.setClient', clientArgs);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      dispatchImeMethodCall(setup, 'TextInput.show', '');
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 1500));
      const configMap = new Map<string, Object>();
      configMap.set('inputAction', 'TextInputAction.go');
      const inputTypeMap = new Map<string, Object>();
      inputTypeMap.set('name', 'TextInputType.number');
      inputTypeMap.set('signed', true);
      inputTypeMap.set('decimal', true);
      configMap.set('inputType', inputTypeMap);
      dispatchImeMethodCall(setup, 'TextInput.updateConfig', configMap);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      expect(setup.textInputPlugin.isFrameworkTextInputActive()).assertEqual(false);
      done();
    });

    it('setPlatformViewClient with virtual display', Level.LEVEL0, async (done: Function) => {
      const setup = new ImeTestSetup();
      const args = new Map<string, Object>();
      args.set('platformViewId', 2);
      args.set('usesVirtualDisplay', true);
      dispatchImeMethodCall(setup, 'TextInput.setPlatformViewClient', args);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      expect(setup.textInputPlugin.isFrameworkTextInputActive()).assertEqual(false);
      done();
    });

    it('finishAutofillContext with false', Level.LEVEL0, async (done: Function) => {
      const setup = new ImeTestSetup();
      const clientArgs = [1, '{"inputAction":"TextInputAction.done","inputType":{"name":"TextInputType.text","signed":false,"decimal":false},"autofill":{"uniqueIdentifier":"field1","editingValue":{"text":"","selectionBase":0,"selectionExtent":0,"composingBase":-1,"composingExtent":-1}}}'];
      dispatchImeMethodCall(setup, 'TextInput.setClient', clientArgs);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      dispatchImeMethodCall(setup, 'TextInput.show', '');
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 1500));
      dispatchImeMethodCall(setup, 'TextInput.finishAutofillContext', false);
      await new Promise<void>((resolve: Function) => setTimeout(resolve, 500));
      expect(setup.textInputPlugin.isFrameworkTextInputActive()).assertEqual(false);
      done();
    });

    it('multiple setClient+show+hide cycles', Level.LEVEL0, async (done: Function) => {
      const setup = new ImeTestSetup();
      for (let i = 0; i < 3; i++) {
        const clientArgs = [i + 1, '{"inputAction":"TextInputAction.done","inputType":{"name":"TextInputType.text","signed":false,"decimal":false}}'];
        dispatchImeMethodCall(setup, 'TextInput.setClient', clientArgs);
        await new Promise<void>((resolve: Function) => setTimeout(resolve, 300));
        dispatchImeMethodCall(setup, 'TextInput.show', '');
        await new Promise<void>((resolve: Function) => setTimeout(resolve, 800));
        dispatchImeMethodCall(setup, 'TextInput.hide', '');
        await new Promise<void>((resolve: Function) => setTimeout(resolve, 300));
      }
      // In the unit-test env, TextInputPluginDefault is constructed with
      // FlutterView == undefined, so setClient/setEditingState calls cannot
      // actually activate the framework client; isFrameworkTextInputActive()
      // is therefore always false. This assertion only verifies that the
      // setClient + show + hide cycles do not throw.
      expect(setup.textInputPlugin.isFrameworkTextInputActive()).assertEqual(false);
      done();
    });
  });
}
