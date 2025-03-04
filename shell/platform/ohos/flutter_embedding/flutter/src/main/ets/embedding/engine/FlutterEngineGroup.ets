/*
* Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd.
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

import FlutterEngine, { EngineLifecycleListener } from "./FlutterEngine"
import common from '@ohos.app.ability.common'
import display from '@ohos.display';
import FlutterLoader from './loader/FlutterLoader'
import FlutterInjector from '../../FlutterInjector'
import { DartEntrypoint } from './dart/DartExecutor'
import PlatformViewsController from '../../plugin/platform/PlatformViewsController'
import ArrayList from '@ohos.util.ArrayList'
import Log from '../../util/Log';
import FlutterManager from '../ohos/FlutterManager';

const TAG = "FlutterEngineGroup"

export default class FlutterEngineGroup {
  private activeEngines: ArrayList<FlutterEngine> = new ArrayList<FlutterEngine>();

  constructor() {

  }


  checkLoader(context: common.Context, args: Array<string>) {
    let loader: FlutterLoader = FlutterInjector.getInstance().getFlutterLoader();
    if (!loader.initialized) {
      loader.startInitialization(context);
      loader.ensureInitializationComplete(args);
    }
  }

  createAndRunEngineByOptions(options: Options) {
    let engine: FlutterEngine | null = null;
    let context: common.Context = options.getContext();
    let dartEntrypoint: DartEntrypoint | null = options.getDartEntrypoint();
    let initialRoute: string = options.getInitialRoute();
    let dartEntrypointArgs: Array<string> = options.getDartEntrypointArgs();
    let platformViewsController: PlatformViewsController | null = options.getPlatformViewsController();
    let waitForRestorationData: boolean = options.getWaitForRestorationData();

    if (dartEntrypoint == null) {
      dartEntrypoint = DartEntrypoint.createDefault();
    }

    if (platformViewsController == null) {
      platformViewsController = new PlatformViewsController();
    }

    Log.i(TAG, "shellHolder, this.activeEngines.length=" + this.activeEngines.length)
    if (this.activeEngines.length == 0) {
      engine = this.createEngine(context, platformViewsController);
      engine.init(context, null, // String[]. The Dart VM has already started, this arguments will have no effect.
        waitForRestorationData)
      if (initialRoute != null) {
        engine.getNavigationChannel()?.setInitialRoute(initialRoute);
      }
      engine.getDartExecutor().executeDartEntrypoint(dartEntrypoint, dartEntrypointArgs)
    } else {
      engine = this.activeEngines[0]
        .spawn(
          context,
          dartEntrypoint,
          initialRoute,
          dartEntrypointArgs,
          platformViewsController,
          waitForRestorationData);
    }
    this.activeEngines.add(engine);

    const engineToCleanUpOnDestroy = engine;
    let listener: EngineLifecycleListener = new EngineLifecycleListenerImpl(
      platformViewsController,
      this.activeEngines,
      engineToCleanUpOnDestroy);
    engine?.addEngineLifecycleListener(listener);
    return engine;
  }

  createEngine(context: common.Context, platformViewsController: PlatformViewsController): FlutterEngine {
    return new FlutterEngine(context, null, null, platformViewsController);
  }

  getDefaultEngine(): FlutterEngine | null {
    let engine: FlutterEngine | null = null;
    if (this.activeEngines.length != 0) {
      engine = this.activeEngines[0];
    }
    return engine;
  }
}

class EngineLifecycleListenerImpl implements EngineLifecycleListener {
  private platformViewsController: PlatformViewsController;
  private activeEngines: ArrayList<FlutterEngine> = new ArrayList();
  private engine: FlutterEngine | null;

  constructor(
    platformViewsController: PlatformViewsController,
    activeEngines: ArrayList<FlutterEngine>,
    engine: FlutterEngine | null) {
    this.platformViewsController = platformViewsController;
    this.activeEngines = activeEngines;
    this.engine = engine;
  }

  onPreEngineRestart(): void {
    this.platformViewsController.onPreEngineRestart();
  }

  onEngineWillDestroy(): void {
    this.activeEngines.remove(this.engine);
  }
}

export class Options {
  private context: common.Context;
  private dartEntrypoint: DartEntrypoint | null = null;
  private initialRoute: string = '';
  private dartEntrypointArgs: Array<string> = [];
  private platformViewsController: PlatformViewsController | null = null;
  private waitForRestorationData: boolean = false;

  constructor(context: common.Context) {
    this.context = context;
  }

  getContext(): common.Context {
    return this.context;
  }

  getDartEntrypoint(): DartEntrypoint | null {
    return this.dartEntrypoint;
  }

  getInitialRoute(): string {
    return this.initialRoute;
  }

  getDartEntrypointArgs(): Array<string> {
    return this.dartEntrypointArgs;
  }

  getWaitForRestorationData(): boolean {
    return this.waitForRestorationData;
  }

  getPlatformViewsController(): PlatformViewsController | null {
    return this.platformViewsController;
  }

  setDartEntrypoint(dartEntrypoint: DartEntrypoint): Options {
    this.dartEntrypoint = dartEntrypoint;
    return this;
  }

  setInitialRoute(initialRoute: string): Options {
    this.initialRoute = initialRoute;
    return this;
  }

  setDartEntrypointArgs(dartEntrypointArgs: Array<string>): Options {
    this.dartEntrypointArgs = dartEntrypointArgs;
    return this;
  }

  setWaitForRestorationData(waitForRestorationData: boolean): Options {
    this.waitForRestorationData = waitForRestorationData;
    return this;
  }

  setPlatformViewsController(platformViewsController: PlatformViewsController): Options {
    this.platformViewsController = platformViewsController;
    return this;
  }
}