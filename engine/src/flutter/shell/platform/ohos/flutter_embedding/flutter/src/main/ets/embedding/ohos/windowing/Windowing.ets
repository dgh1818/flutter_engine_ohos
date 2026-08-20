/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

/**
 * Barrel module for the windowing package: every host class, the registry,
 * the archetype enum and its wire decoder, re-exported from one place so
 * consumers import once instead of piecemeal per class. Internal to the
 * embedder — deliberately NOT re-exported from the package root index.ets.
 */
export * from './WindowHostContext';
export * from './FlutterWindowHost';
export * from './WindowHostRegistry';
export * from './WindowArchetype';
export * from './SubWindowHost';
export * from './DialogWindowHost';
export * from './TooltipWindowHost';
export * from './PopupWindowHost';
export * from './AnchoredWindowHost';
export * from './AbilityWindowHost';
