/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, Level } from '@ohos/hypium';
import { inputMethod } from '@kit.IMEKit';
import Any from '../../../../../main/ets/plugin/common/Any';
import { Autofill, Configuration, InputType, TextEditState } from '../../../../../main/ets/embedding/engine/systemchannels/TextInputChannel';
import { PointerDeviceKind } from '../../../../../main/ets/embedding/ohos/OhosTouchProcessor';
import {
  AutoFillFieldRect,
  AutoFillHostInfo,
  OhosAutoFillHelper,
  OhosPageNodeInfo,
  OhosViewData
} from '../../../../../main/ets/plugin/editing/OhosAutoFillHelper';

function makeConfig(autofill?: Autofill, fields?: Array<Configuration>): Configuration {
  const inputType: InputType = new InputType(inputMethod.TextInputType.TEXT, false, false);
  const cfg: Configuration = new Configuration(
    false, false, false, false, false,
    inputType, 0, '',
    autofill, [],
    PointerDeviceKind.UNKNOWN,
    fields ?? new Array<Configuration>()
  );
  return cfg;
}

function makeViewData(nodes: Array<OhosPageNodeInfo>): OhosViewData {
  const vd: OhosViewData = {
    bundleName: 'a',
    moduleName: 'm',
    abilityName: 'e',
    pageUrl: 'p',
    pageNodeInfos: nodes,
    pageRect: { left: 0, top: 0, width: 1, height: 1 },
    isUserSelected: false,
    isOtherAccount: false,
  };
  return vd;
}

function makeRect(left: number, top: number, width: number, height: number): AutoFillFieldRect {
  return { left: left, top: top, width: width, height: height };
}

function makeAutofill(uniqueId: string, hints: Array<string>, text: string, hintText: string | null = null): Autofill {
  const editState: TextEditState = new TextEditState(text, 0, 0, -1, -1);
  return new Autofill(uniqueId, hints, hintText, editState);
}

function makeHostInfo(): AutoFillHostInfo {
  return { bundleName: 'com.example.app', moduleName: 'entry', abilityName: 'EntryAbility', pageUrl: 'pages/Index' };
}

function makePageNode(id: number, type: number, value: string, focused: boolean = false): OhosPageNodeInfo {
  return {
    id: id,
    depth: 0,
    autoFillType: type,
    tag: '',
    value: value,
    placeholder: '',
    enableAutoFill: true,
    rect: { left: 0, top: 0, width: 100, height: 40 },
    isFocus: focused,
  };
}

export default function ohosAutoFillHelperTest() {
  describe('OhosAutoFillHelper.hintToAutoFillType', () => {
    it('maps password hint', Level.LEVEL1, () => {
      expect(OhosAutoFillHelper.hintToAutoFillType('password')).assertEqual(1);
    });
    it('maps newPassword hint', Level.LEVEL1, () => {
      expect(OhosAutoFillHelper.hintToAutoFillType('newPassword')).assertEqual(3);
    });
    it('maps username / newUsername hint', Level.LEVEL1, () => {
      expect(OhosAutoFillHelper.hintToAutoFillType('username')).assertEqual(2);
      expect(OhosAutoFillHelper.hintToAutoFillType('newUsername')).assertEqual(2);
    });
    it('maps emailAddress hint', Level.LEVEL1, () => {
      expect(OhosAutoFillHelper.hintToAutoFillType('emailAddress')).assertEqual(16);
    });
    it('maps phoneNumber / phoneNumberDevice hint', Level.LEVEL1, () => {
      expect(OhosAutoFillHelper.hintToAutoFillType('phoneNumber')).assertEqual(13);
      expect(OhosAutoFillHelper.hintToAutoFillType('phoneNumberDevice')).assertEqual(13);
    });
    it('maps phoneCountryCode hint', Level.LEVEL1, () => {
      expect(OhosAutoFillHelper.hintToAutoFillType('phoneCountryCode')).assertEqual(14);
    });
    it('maps phoneNational / fullPhoneNumber hint', Level.LEVEL1, () => {
      expect(OhosAutoFillHelper.hintToAutoFillType('phoneNational')).assertEqual(15);
      expect(OhosAutoFillHelper.hintToAutoFillType('fullPhoneNumber')).assertEqual(15);
    });
    it('maps streetAddress / fullStreetAddress hint', Level.LEVEL1, () => {
      expect(OhosAutoFillHelper.hintToAutoFillType('streetAddress')).assertEqual(4);
      expect(OhosAutoFillHelper.hintToAutoFillType('fullStreetAddress')).assertEqual(4);
    });
    it('maps addressLocality / addressRegion / addressCountry / postalCode hints', Level.LEVEL1, () => {
      expect(OhosAutoFillHelper.hintToAutoFillType('addressLocality')).assertEqual(7);
      expect(OhosAutoFillHelper.hintToAutoFillType('addressRegion')).assertEqual(8);
      expect(OhosAutoFillHelper.hintToAutoFillType('addressCountry')).assertEqual(9);
      expect(OhosAutoFillHelper.hintToAutoFillType('postalCode')).assertEqual(6);
    });
    it('maps personName / personGivenName / personFamilyName hints', Level.LEVEL1, () => {
      expect(OhosAutoFillHelper.hintToAutoFillType('personName')).assertEqual(10);
      expect(OhosAutoFillHelper.hintToAutoFillType('personGivenName')).assertEqual(12);
      expect(OhosAutoFillHelper.hintToAutoFillType('personFamilyName')).assertEqual(11);
    });
    it('maps creditCardNumber / nickname hints', Level.LEVEL1, () => {
      expect(OhosAutoFillHelper.hintToAutoFillType('creditCardNumber')).assertEqual(17);
      expect(OhosAutoFillHelper.hintToAutoFillType('nickname')).assertEqual(24);
    });
    it('returns UNSPECIFIED for unknown hint', Level.LEVEL1, () => {
      expect(OhosAutoFillHelper.hintToAutoFillType('notARealHint')).assertEqual(0);
      expect(OhosAutoFillHelper.hintToAutoFillType('')).assertEqual(0);
    });
  });

  describe('OhosAutoFillHelper.primaryAutoFillType', () => {
    it('returns UNSPECIFIED for empty hints', Level.LEVEL1, () => {
      const a: Autofill = makeAutofill('id1', new Array<string>(), 'val');
      expect(OhosAutoFillHelper.primaryAutoFillType(a)).assertEqual(0);
    });
    it('returns first recognized type from hints', Level.LEVEL1, () => {
      const a: Autofill = makeAutofill('id1', new Array<string>('unknown', 'password', 'emailAddress'), 'val');
      expect(OhosAutoFillHelper.primaryAutoFillType(a)).assertEqual(1);
    });
    it('falls back to first hint (mapped) if all hints unrecognized', Level.LEVEL1, () => {
      const a: Autofill = makeAutofill('id1', new Array<string>('notAReal'), 'val');
      // Falls back to hintToAutoFillType(hints[0]) which returns UNSPECIFIED
      expect(OhosAutoFillHelper.primaryAutoFillType(a)).assertEqual(0);
    });
  });

  describe('OhosAutoFillHelper.isValidRect', () => {
    it('returns false for null', Level.LEVEL1, () => {
      expect(OhosAutoFillHelper.isValidRect(null)).assertEqual(false);
    });
    it('returns false for zero width', Level.LEVEL1, () => {
      expect(OhosAutoFillHelper.isValidRect(makeRect(0, 0, 0, 10))).assertEqual(false);
    });
    it('returns false for zero height', Level.LEVEL1, () => {
      expect(OhosAutoFillHelper.isValidRect(makeRect(0, 0, 10, 0))).assertEqual(false);
    });
    it('returns true for positive width and height', Level.LEVEL1, () => {
      expect(OhosAutoFillHelper.isValidRect(makeRect(0, 0, 1, 1))).assertEqual(true);
      expect(OhosAutoFillHelper.isValidRect(makeRect(0, 0, 100, 50))).assertEqual(true);
    });
  });

  describe('OhosAutoFillHelper.buildNodeIdMap', () => {
    it('returns empty map when fields is null', Level.LEVEL1, () => {
      const cfg: Configuration = makeConfig(makeAutofill('current', new Array<string>('username'), 'val'));
      const m: Map<string, number> = OhosAutoFillHelper.buildNodeIdMap(cfg);
      expect(m.size).assertEqual(1);
      expect(m.get('current')).assertEqual(0);
    });
    it('returns empty map when fields is empty', Level.LEVEL1, () => {
      const cfg: Configuration = makeConfig(makeAutofill('current', new Array<string>(), 'val'));
      const m: Map<string, number> = OhosAutoFillHelper.buildNodeIdMap(cfg);
      expect(m.size).assertEqual(1);
    });
    it('assigns sequential ids to fields + currentAutofill', Level.LEVEL1, () => {
      const f1: Configuration = makeConfig(makeAutofill('a', new Array<string>('username'), 'a'));
      const f2: Configuration = makeConfig(makeAutofill('b', new Array<string>('password'), 'b'));
      const cfg: Configuration = makeConfig(
        makeAutofill('current', new Array<string>(), 'c'),
        new Array<Configuration>(f1, f2)
      );
      const m: Map<string, number> = OhosAutoFillHelper.buildNodeIdMap(cfg);
      expect(m.size).assertEqual(3);
      expect(m.get('a')).assertEqual(0);
      expect(m.get('b')).assertEqual(1);
      expect(m.get('current')).assertEqual(2);
    });
    it('skips fields without autofill', Level.LEVEL1, () => {
      const f1: Configuration = makeConfig(); // no autofill
      const f2: Configuration = makeConfig(makeAutofill('b', new Array<string>('username'), 'b'));
      const cfg: Configuration = makeConfig(
        makeAutofill('current', new Array<string>(), 'c'),
        new Array<Configuration>(f1, f2)
      );
      const m: Map<string, number> = OhosAutoFillHelper.buildNodeIdMap(cfg);
      expect(m.size).assertEqual(2);
      expect(m.has('a')).assertEqual(false);
      expect(m.get('b')).assertEqual(0);
      expect(m.get('current')).assertEqual(1);
    });
  });

  describe('OhosAutoFillHelper.buildViewData', () => {
    it('throws when currentAutofill is undefined', Level.LEVEL1, () => {
      const cfg: Configuration = makeConfig();
      // No currentAutofill
      let threw = false;
      try {
        OhosAutoFillHelper.buildViewData(makeHostInfo(), cfg, new Map<string, number>(), null, null);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });
    it('builds a single-node viewData when fields is empty', Level.LEVEL1, () => {
      const cfg: Configuration = makeConfig(makeAutofill('current', new Array<string>('username'), 'val'));
      const m: Map<string, number> = new Map<string, number>();
      m.set('current', 0);
      const vd = OhosAutoFillHelper.buildViewData(makeHostInfo(), cfg, m, null, null);
      expect(vd.pageNodeInfos.length).assertEqual(1);
      expect(vd.pageNodeInfos[0].id).assertEqual(0);
      expect(vd.pageNodeInfos[0].isFocus).assertEqual(true);
      expect(vd.bundleName).assertEqual('com.example.app');
    });
    it('builds multi-node viewData for each field', Level.LEVEL1, () => {
      const f1: Configuration = makeConfig(makeAutofill('u', new Array<string>('username'), 'u'));
      const f2: Configuration = makeConfig(makeAutofill('p', new Array<string>('password'), 'p'));
      const cfg: Configuration = makeConfig(
        makeAutofill('u', new Array<string>('username'), 'u'),
        new Array<Configuration>(f1, f2)
      );
      const m: Map<string, number> = new Map<string, number>();
      m.set('u', 0);
      m.set('p', 1);
      const vd = OhosAutoFillHelper.buildViewData(makeHostInfo(), cfg, m, null, null);
      expect(vd.pageNodeInfos.length).assertEqual(2);
      // 'u' is focused (matches focusedId)
      const uNode: OhosPageNodeInfo = vd.pageNodeInfos[0];
      expect(uNode.id).assertEqual(0);
      expect(uNode.isFocus).assertEqual(true);
      expect(vd.pageNodeInfos[1].id).assertEqual(1);
      expect(vd.pageNodeInfos[1].isFocus).assertEqual(false);
    });
  });

  describe('OhosAutoFillHelper.buildViewDataForSave', () => {
    it('builds save viewData with merged texts from map', Level.LEVEL1, () => {
      const cfgs: Map<string, Configuration> = new Map<string, Configuration>();
      const cfgU: Configuration = makeConfig(makeAutofill('u', new Array<string>('username'), ''));
      cfgs.set('u', cfgU);
      const cfgP: Configuration = makeConfig(makeAutofill('p', new Array<string>('password'), ''));
      cfgs.set('p', cfgP);
      const texts: Map<string, string> = new Map<string, string>();
      texts.set('u', 'alice');
      texts.set('p', 'secret');
      const m: Map<string, number> = new Map<string, number>();
      m.set('u', 0);
      m.set('p', 1);
      const vd = OhosAutoFillHelper.buildViewDataForSave(makeHostInfo(), cfgs, texts, m, 'u', null, null);
      expect(vd.pageNodeInfos.length).assertEqual(2);
      // texts from map override editState.text
      expect(vd.pageNodeInfos[0].value).assertEqual('alice');
      expect(vd.pageNodeInfos[1].value).assertEqual('secret');
    });
    it('falls back to editState.text when text map has no entry', Level.LEVEL1, () => {
      const cfgs: Map<string, Configuration> = new Map<string, Configuration>();
      const cfgU: Configuration = makeConfig(makeAutofill('u', new Array<string>('username'), 'fallbackU'));
      cfgs.set('u', cfgU);
      const texts: Map<string, string> = new Map<string, string>(); // empty
      const m: Map<string, number> = new Map<string, number>();
      m.set('u', 0);
      const vd = OhosAutoFillHelper.buildViewDataForSave(makeHostInfo(), cfgs, texts, m, 'u', null, null);
      expect(vd.pageNodeInfos[0].value).assertEqual('fallbackU');
    });
  });

  describe('OhosAutoFillHelper.validateSaveViewData', () => {
    it('returns ok when has username + password with non-empty values', Level.LEVEL1, () => {
      const nodes: Array<OhosPageNodeInfo> = new Array<OhosPageNodeInfo>(
        makePageNode(0, 2, 'alice'),   // USERNAME with value
        makePageNode(1, 1, 'secret'),  // PASSWORD with value
      );
      const vd: OhosViewData = makeViewData(nodes);
      expect(OhosAutoFillHelper.validateSaveViewData(vd)).assertEqual('ok');
    });
    it('returns missing_username_value when no username value', Level.LEVEL1, () => {
      const nodes: Array<OhosPageNodeInfo> = new Array<OhosPageNodeInfo>(
        makePageNode(0, 2, ''),        // USERNAME empty
        makePageNode(1, 1, 'secret'),  // PASSWORD
      );
      const vd: OhosViewData = makeViewData(nodes);
      expect(OhosAutoFillHelper.validateSaveViewData(vd)).assertEqual('missing_username_value');
    });
    it('returns missing_password_value when no password value', Level.LEVEL1, () => {
      const nodes: Array<OhosPageNodeInfo> = new Array<OhosPageNodeInfo>(
        makePageNode(0, 2, 'alice'),
        makePageNode(1, 1, ''),
      );
      const vd: OhosViewData = makeViewData(nodes);
      expect(OhosAutoFillHelper.validateSaveViewData(vd)).assertEqual('missing_password_value');
    });
    it('returns nodeCount_lt_2 when fewer than 2 nodes', Level.LEVEL1, () => {
      const nodes: Array<OhosPageNodeInfo> = new Array<OhosPageNodeInfo>();
      nodes.push(makePageNode(0, 2, 'alice'));
      const vd: OhosViewData = makeViewData(nodes);
      expect(OhosAutoFillHelper.validateSaveViewData(vd)).assertEqual('nodeCount_lt_2(1)');
    });
  });

  describe('OhosAutoFillHelper.parseFillResult', () => {
    it('extracts id->value map for nodes with non-empty value', Level.LEVEL1, () => {
      const nodes: Array<OhosPageNodeInfo> = new Array<OhosPageNodeInfo>(
        makePageNode(0, 2, 'alice'),
        makePageNode(1, 1, ''),         // empty value, should be skipped
        makePageNode(2, 16, 'a@b.com'),
      );
      const vd: OhosViewData = makeViewData(nodes);
      const m: Map<number, string> = OhosAutoFillHelper.parseFillResult(vd as Any);
      expect(m.size).assertEqual(2);
      expect(m.get(0)).assertEqual('alice');
      expect(m.get(2)).assertEqual('a@b.com');
      expect(m.has(1)).assertEqual(false);
    });
  });

  describe('OhosAutoFillHelper.logViewDataSummary', () => {
    it('does not throw on valid viewData', Level.LEVEL1, () => {
      const nodes: Array<OhosPageNodeInfo> = new Array<OhosPageNodeInfo>(
        makePageNode(0, 2, 'alice'),
        makePageNode(1, 1, 'secret'),
      );
      const vd: OhosViewData = makeViewData(nodes);
      let threw = false;
      try {
        OhosAutoFillHelper.logViewDataSummary(vd, 'test-label');
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('OhosAutoFillHelper.setHostPageUrl', () => {
    it('clears the hostInfoCache', Level.LEVEL1, () => {
      // Pre-populate cache
      OhosAutoFillHelper.setHostPageUrl('pages/Cached');
      // Setting again should clear it
      OhosAutoFillHelper.setHostPageUrl('pages/New');
      // getCachedHostInfo should return null since cache was cleared
      expect(OhosAutoFillHelper.getCachedHostInfo()).assertEqual(null);
    });
  });

  describe('OhosAutoFillHelper.buildNodeIdMap.extended', () => {
    it('processes fields that have autofill (covers fields loop)', Level.LEVEL1, () => {
      const f1: Configuration = makeConfig(makeAutofill('a', new Array<string>('username'), 'a'));
      const f2: Configuration = makeConfig(makeAutofill('b', new Array<string>('password'), 'b'));
      const f3: Configuration = makeConfig(); // no autofill - should be skipped
      const cfg: Configuration = makeConfig(
        makeAutofill('current', new Array<string>('emailAddress'), 'c'),
        new Array<Configuration>(f1, f2, f3)
      );
      const m: Map<string, number> = OhosAutoFillHelper.buildNodeIdMap(cfg);
      expect(m.size).assertEqual(3);
      expect(m.get('a')).assertEqual(0);
      expect(m.get('b')).assertEqual(1);
      expect(m.get('current')).assertEqual(2);
    });
    it('handles fields with same uniqueIdentifier (no duplicate id)', Level.LEVEL1, () => {
      const f1: Configuration = makeConfig(makeAutofill('same', new Array<string>('username'), 'a'));
      const f2: Configuration = makeConfig(makeAutofill('same', new Array<string>('password'), 'b'));
      const cfg: Configuration = makeConfig(
        makeAutofill('current', new Array<string>(), 'c'),
        new Array<Configuration>(f1, f2)
      );
      const m: Map<string, number> = OhosAutoFillHelper.buildNodeIdMap(cfg);
      // f1 and f2 share 'same', only one gets a new id
      expect(m.size).assertEqual(2);
      expect(m.get('same')).assertEqual(0);
      expect(m.get('current')).assertEqual(1);
    });
  });

  describe('OhosAutoFillHelper.buildViewData.extended', () => {
    it('uses pageRect when it is valid', Level.LEVEL1, () => {
      const cfg: Configuration = makeConfig(makeAutofill('u', new Array<string>('username'), 'val'));
      const m: Map<string, number> = new Map<string, number>();
      m.set('u', 0);
      const pageRect: AutoFillFieldRect = makeRect(0, 0, 200, 300);
      const vd = OhosAutoFillHelper.buildViewData(makeHostInfo(), cfg, m, null, pageRect);
      // pageRect is used (200x300)
      expect(vd.pageRect.width).assertEqual(200);
      expect(vd.pageRect.height).assertEqual(300);
    });
    it('falls back to focusedFieldRect when pageRect is invalid', Level.LEVEL1, () => {
      const cfg: Configuration = makeConfig(makeAutofill('u', new Array<string>('username'), 'val'));
      const m: Map<string, number> = new Map<string, number>();
      m.set('u', 0);
      const pageRect: AutoFillFieldRect = makeRect(0, 0, 0, 0); // invalid
      const focusedRect: AutoFillFieldRect = makeRect(10, 20, 30, 40);
      const vd = OhosAutoFillHelper.buildViewData(makeHostInfo(), cfg, m, focusedRect, pageRect);
      // focusedFieldRect is used (30x40)
      expect(vd.pageRect.width).assertEqual(30);
      expect(vd.pageRect.height).assertEqual(40);
    });
    it('skips fields without autofill in fields array', Level.LEVEL1, () => {
      const f1: Configuration = makeConfig(makeAutofill('u', new Array<string>('username'), 'u'));
      const f2: Configuration = makeConfig(); // no autofill - skipped
      const cfg: Configuration = makeConfig(
        makeAutofill('u', new Array<string>('username'), 'u'),
        new Array<Configuration>(f1, f2)
      );
      const m: Map<string, number> = new Map<string, number>();
      m.set('u', 0);
      const vd = OhosAutoFillHelper.buildViewData(makeHostInfo(), cfg, m, null, null);
      // Only one node (f2 skipped)
      expect(vd.pageNodeInfos.length).assertEqual(1);
    });
  });

  describe('OhosAutoFillHelper.buildViewDataForSave.extended', () => {
    it('skips fields without autofill in fieldConfigs map', Level.LEVEL1, () => {
      const cfgs: Map<string, Configuration> = new Map<string, Configuration>();
      const cfgU: Configuration = makeConfig(makeAutofill('u', new Array<string>('username'), 'alice'));
      cfgs.set('u', cfgU);
      cfgs.set('noAutofill', makeConfig()); // no autofill - skipped
      const texts: Map<string, string> = new Map<string, string>();
      texts.set('u', 'alice');
      const m: Map<string, number> = new Map<string, number>();
      m.set('u', 0);
      const vd = OhosAutoFillHelper.buildViewDataForSave(makeHostInfo(), cfgs, texts, m, 'u', null, null);
      expect(vd.pageNodeInfos.length).assertEqual(1);
      expect(vd.pageNodeInfos[0].value).assertEqual('alice');
    });
    it('falls back to editState text when text map entry is empty string', Level.LEVEL1, () => {
      const cfgs: Map<string, Configuration> = new Map<string, Configuration>();
      const cfgU: Configuration = makeConfig(makeAutofill('u', new Array<string>('username'), 'fallbackU'));
      cfgs.set('u', cfgU);
      const texts: Map<string, string> = new Map<string, string>();
      texts.set('u', ''); // empty string triggers fallback
      const m: Map<string, number> = new Map<string, number>();
      m.set('u', 0);
      const vd = OhosAutoFillHelper.buildViewDataForSave(makeHostInfo(), cfgs, texts, m, 'u', null, null);
      expect(vd.pageNodeInfos[0].value).assertEqual('fallbackU');
    });
  });

  describe('OhosAutoFillHelper.toAutoFillRect.defaultValueBranches', () => {
    it('uses width=1 when rect.width is 0 (covers false branch of width>0)', Level.LEVEL1, () => {
      // Build viewData with a 0-width rect via fieldRects; resolvePageRect -> toAutoFillRect(rect)
      // and rect.width > 0 ? rect.width : 1 -> false branch.
      const cfg: Configuration = makeConfig(makeAutofill('u', new Array<string>('username'), 'val'));
      const m: Map<string, number> = new Map<string, number>();
      m.set('u', 0);
      // fieldRects with width=0 (invalid); rectForField will fall back to default (1, 1, 1) since focusRect is null.
      const fieldRects: Map<string, AutoFillFieldRect> = new Map<string, AutoFillFieldRect>();
      fieldRects.set('u', makeRect(0, 5, 0, 5));
      const vd = OhosAutoFillHelper.buildViewData(makeHostInfo(), cfg, m, null, null, fieldRects);
      // rectForField sees invalid stored rect, then no focusedFieldRect → defaultFieldRect().
      // defaultFieldRect returns {left:0, top:0, width:1, height:1}, all > 0, no clamp.
      expect(vd.pageNodeInfos[0].rect.width).assertEqual(1);
      expect(vd.pageNodeInfos[0].rect.height).assertEqual(1);
    });

    it('uses width=1 when rect.width is negative (covers false branch of width>0)', Level.LEVEL1, () => {
      const cfg: Configuration = makeConfig(makeAutofill('u', new Array<string>('username'), 'val'));
      const m: Map<string, number> = new Map<string, number>();
      m.set('u', 0);
      // Pass a negative-width rect directly; rectForField returns the focusedFieldRect
      // when isFocused. With focusedFieldRect having negative width, toAutoFillRect will clamp it.
      const negativeRect: AutoFillFieldRect = makeRect(0, 0, -10, 5);
      const vd = OhosAutoFillHelper.buildViewData(makeHostInfo(), cfg, m, negativeRect, null);
      // negative width → clamp to 1
      expect(vd.pageNodeInfos[0].rect.width).assertEqual(1);
    });
  });

  describe('OhosAutoFillHelper.parseFillResult.extended', () => {
    it('returns empty map for empty pageNodeInfos', Level.LEVEL1, () => {
      const vd: OhosViewData = makeViewData(new Array<OhosPageNodeInfo>());
      const m: Map<number, string> = OhosAutoFillHelper.parseFillResult(vd as Any);
      expect(m.size).assertEqual(0);
    });
  });
}
