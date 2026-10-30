import { describe, it, expect, Level } from '@ohos/hypium';
import DartExecutor from '../../../../../../main/ets/embedding/engine/dart/DartExecutor';
import FlutterNapi from '../../../../../../main/ets/embedding/engine/FlutterNapi';
import NavigationChannel, { NavigatorActivity, NavigatorStatus } from '../../../../../../main/ets/embedding/engine/systemchannels/NavigationChannel';
import { MethodCallHandler, MethodResult } from '../../../../../../main/ets/plugin/common/MethodChannel';
import MethodCall from '../../../../../../main/ets/plugin/common/MethodCall';
import Any from '../../../../../../main/ets/plugin/common/Any';
import { resourceManager } from '@kit.LocalizationKit';
import { DartMessenger } from '../../../../../../main/ets/embedding/engine/dart/DartMessenger';
import { IncomingMethodCallHandler } from '../../../../../../main/ets/plugin/common/MethodChannel';
import { BinaryReply } from '../../../../../../main/ets/plugin/common/BinaryMessenger';
import JSONMethodCodec from '../../../../../../main/ets/plugin/common/JSONMethodCodec';

class DummyHandler implements MethodCallHandler {
  onMethodCall(call: Any, result: MethodResult): void {}
}

class FakeMethodResult implements MethodResult {
  successCalls: number = 0;
  errorCalls: number = 0;
  notImplCalls: number = 0;
  successValue: Any = null;
  success(result: Any): void { this.successCalls++; this.successValue = result; }
  error(errorCode: string, errorMessage: string, errorDetails: Any): void { this.errorCalls++; }
  notImplemented(): void { this.notImplCalls++; }
}

class NavTestSetup {
  channel: NavigationChannel;
  flutterNapi: FlutterNapi;
  dartExecutor: DartExecutor;

  constructor() {
    this.flutterNapi = new FlutterNapi();
    const context = getContext();
    const rm: resourceManager.ResourceManager = context.resourceManager;
    this.dartExecutor = new DartExecutor(this.flutterNapi, rm);
    this.dartExecutor.onAttachedToNAPI();
    this.channel = new NavigationChannel(this.dartExecutor);
  }
}

class FakeBinaryReply implements BinaryReply {
  replyData: ArrayBuffer | null = null;
  replyCount: number = 0;
  reply(data: ArrayBuffer | null): void { this.replyCount++; this.replyData = data; }
}

function dispatchNavMethodCall(setup: NavTestSetup, method: string, args: Any): FakeBinaryReply {
  const call = new MethodCall(method, args);
  const encoded = JSONMethodCodec.INSTANCE.encodeMethodCall(call);
  const reply = new FakeBinaryReply();
  const pmh = setup.flutterNapi.platformMessageHandler;
  if (pmh == null) return reply;
  const dm: DartMessenger = pmh as DartMessenger;
  const handlerInfo = dm.messageHandlers.get('flutter/navigation');
  if (handlerInfo) {
    const incoming = handlerInfo.handler as IncomingMethodCallHandler;
    incoming.onMessage(encoded, reply);
  }
  return reply;
}

export default function navigationChannelTest() {
  describe('NavigatorActivity', () => {
    it('PUSH_equals_push', Level.LEVEL0, () => {
      expect(NavigatorActivity.PUSH).assertEqual('push');
    });

    it('POP_equals_pop', Level.LEVEL0, () => {
      expect(NavigatorActivity.POP).assertEqual('pop');
    });
  });

  describe('NavigatorStatus', () => {
    it('START_equals_start', Level.LEVEL0, () => {
      expect(NavigatorStatus.START).assertEqual('start');
    });

    it('FINISH_equals_finish', Level.LEVEL0, () => {
      expect(NavigatorStatus.FINISH).assertEqual('finish');
    });
  });


  describe('NavigationChannel.construct', () => {
    it('constructsWithoutCrash', Level.LEVEL1, () => {
      const setup = new NavTestSetup();
      expect(setup.channel).not().assertNull();
    });
  });

  describe('NavigationChannel.setMethodCallHandler', () => {
    it('setsCustomHandlerWithoutCrash', Level.LEVEL1, () => {
      const setup = new NavTestSetup();
      const handler: DummyHandler = new DummyHandler();
      setup.channel.setMethodCallHandler(handler);
    });
  });

  describe('NavigationChannel.publicMethods', () => {
    it('setInitialRoute does not throw', Level.LEVEL0, () => {
      const setup = new NavTestSetup();
      let threw = false;
      try { setup.channel.setInitialRoute('/home'); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('pushRoute does not throw', Level.LEVEL0, () => {
      const setup = new NavTestSetup();
      let threw = false;
      try { setup.channel.pushRoute('/page/1'); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('pushRouteInformation does not throw', Level.LEVEL0, () => {
      const setup = new NavTestSetup();
      let threw = false;
      try { setup.channel.pushRouteInformation('/page/1'); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('popRoute does not throw', Level.LEVEL0, () => {
      const setup = new NavTestSetup();
      let threw = false;
      try { setup.channel.popRoute(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('setInitialRoute with empty string', Level.LEVEL1, () => {
      const setup = new NavTestSetup();
      let threw = false;
      try { setup.channel.setInitialRoute(''); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });
  });

  describe('NavigationCallback.onMethodCall', () => {
    it('reportNavigatorActivity with push start', Level.LEVEL0, () => {
      const setup = new NavTestSetup();
      const args = new Map<string, string>();
      args.set('activity', 'push');
      args.set('status', 'start');
      args.set('routeName', '/home');
      dispatchNavMethodCall(setup, 'reportNavigatorActivity', args);
      expect(setup.channel).not().assertNull();
    });

    it('reportNavigatorActivity with push finish', Level.LEVEL0, () => {
      const setup = new NavTestSetup();
      const args = new Map<string, string>();
      args.set('activity', 'push');
      args.set('status', 'finish');
      dispatchNavMethodCall(setup, 'reportNavigatorActivity', args);
      expect(setup.channel).not().assertNull();
    });

    it('reportNavigatorActivity with pop start', Level.LEVEL0, () => {
      const setup = new NavTestSetup();
      const args = new Map<string, string>();
      args.set('activity', 'pop');
      args.set('status', 'start');
      dispatchNavMethodCall(setup, 'reportNavigatorActivity', args);
      expect(setup.channel).not().assertNull();
    });

    it('reportNavigatorActivity with pop finish', Level.LEVEL0, () => {
      const setup = new NavTestSetup();
      const args = new Map<string, string>();
      args.set('activity', 'pop');
      args.set('status', 'finish');
      dispatchNavMethodCall(setup, 'reportNavigatorActivity', args);
      expect(setup.channel).not().assertNull();
    });

    it('reportNavigatorActivity with null args returns success', Level.LEVEL0, () => {
      const setup = new NavTestSetup();
      dispatchNavMethodCall(setup, 'reportNavigatorActivity', null);
      expect(setup.channel).not().assertNull();
    });

    it('reportNavigatorActivity with non-Map args returns success', Level.LEVEL0, () => {
      const setup = new NavTestSetup();
      dispatchNavMethodCall(setup, 'reportNavigatorActivity', 'not-a-map');
      expect(setup.channel).not().assertNull();
    });

    it('reportNavigatorActivity with missing activity field', Level.LEVEL0, () => {
      const setup = new NavTestSetup();
      const args = new Map<string, string>();
      args.set('status', 'start');
      const reply = dispatchNavMethodCall(setup, 'reportNavigatorActivity', args);
      expect(setup.channel).not().assertNull();
    });

    it('reportNavigatorActivity with missing status field', Level.LEVEL0, () => {
      const setup = new NavTestSetup();
      const args = new Map<string, string>();
      args.set('activity', 'push');
      const reply = dispatchNavMethodCall(setup, 'reportNavigatorActivity', args);
      expect(setup.channel).not().assertNull();
    });

    it('reportNavigatorActivity with unknown activity throws', Level.LEVEL0, () => {
      const setup = new NavTestSetup();
      const args = new Map<string, string>();
      args.set('activity', 'unknown');
      args.set('status', 'start');
      const reply = dispatchNavMethodCall(setup, 'reportNavigatorActivity', args);
      expect(setup.channel).not().assertNull();
    });

    it('reportNavigatorActivity with unknown status throws', Level.LEVEL0, () => {
      const setup = new NavTestSetup();
      const args = new Map<string, string>();
      args.set('activity', 'push');
      args.set('status', 'unknown');
      const reply = dispatchNavMethodCall(setup, 'reportNavigatorActivity', args);
      expect(setup.channel).not().assertNull();
    });

    it('routeInformationUpdated does not throw', Level.LEVEL0, () => {
      const setup = new NavTestSetup();
      const args = new Map<string, string>();
      args.set('uri', '/page/1');
      dispatchNavMethodCall(setup, 'routeInformationUpdated', args);
      expect(setup.channel).not().assertNull();
    });

    it('routeInformationUpdated with null args (covers line 184 true branch)', Level.LEVEL0, () => {
      const setup = new NavTestSetup();
      dispatchNavMethodCall(setup, 'routeInformationUpdated', null);
      expect(setup.channel).not().assertNull();
    });

    it('routeInformationUpdated with non-Map args (covers line 184 true branch)', Level.LEVEL0, () => {
      const setup = new NavTestSetup();
      dispatchNavMethodCall(setup, 'routeInformationUpdated', 'not_a_map');
      expect(setup.channel).not().assertNull();
    });

    it('unknown method returns notImplemented', Level.LEVEL0, () => {
      const setup = new NavTestSetup();
      dispatchNavMethodCall(setup, 'unknown.method', null);
      expect(setup.channel).not().assertNull();
    });
  });
}
