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

import { describe, expect, it } from '@ohos/hypium';
import { createDVModelFromJson } from '../../../../../main/ets/view/DynamicView/dynamicViewJson';

interface TestJson {
  compType: string;
  attributes?: Record<string, Object>;
  events?: Record<string, Object>;
  children?: TestJson[];
  build?: Object;
}

export default function dynamicViewJsonTest() {
  describe('dynamicViewJsonTest', () => {
    it('createDVModelFromJson_withCompType', 0, () => {
      const json: TestJson = {
        compType: 'Column'
      };
      const dvModel = createDVModelFromJson(json);
      expect(dvModel).not().assertNull();
      expect(dvModel.compType).assertEqual('Column');
    })

    it('createDVModelFromJson_withAttributes', 0, () => {
      const json: TestJson = {
        compType: 'Text',
        attributes: {
          'width': 100,
          'height': 50,
          'fontColor': '#FF0000'
        }
      };
      const dvModel = createDVModelFromJson(json);
      expect(dvModel.compType).assertEqual('Text');
      const params = dvModel.params as Record<string, Object>;
      expect(params['width']).assertEqual(100);
      expect(params['height']).assertEqual(50);
      expect(params['fontColor']).assertEqual('#FF0000');
    })

    it('createDVModelFromJson_withEvents', 0, () => {
      const onClick = () => {};
      const json: TestJson = {
        compType: 'Button',
        events: {
          'onClick': onClick
        }
      };
      const dvModel = createDVModelFromJson(json);
      expect(dvModel.compType).assertEqual('Button');
      const events = dvModel.events as Record<string, Object>;
      expect(events['onClick']).assertEqual(onClick);
    })

    it('createDVModelFromJson_withChildren', 0, () => {
      const json: TestJson = {
        compType: 'Column',
        children: [
          { compType: 'Text' } as TestJson,
          { compType: 'Image' } as TestJson
        ]
      };
      const dvModel = createDVModelFromJson(json);
      expect(dvModel.compType).assertEqual('Column');
      expect(dvModel.children.length).assertEqual(2);
      expect(dvModel.children[0].compType).assertEqual('Text');
      expect(dvModel.children[1].compType).assertEqual('Image');
    })

    it('createDVModelFromJson_withNestedChildren', 0, () => {
      const json: TestJson = {
        compType: 'Column',
        children: [
          {
            compType: 'Row',
            children: [
              { compType: 'Text' } as TestJson,
              { compType: 'Image' } as TestJson
            ]
          } as TestJson
        ]
      };
      const dvModel = createDVModelFromJson(json);
      expect(dvModel.compType).assertEqual('Column');
      expect(dvModel.children.length).assertEqual(1);
      const childRow = dvModel.children[0];
      expect(childRow.compType).assertEqual('Row');
      expect(childRow.children.length).assertEqual(2);
      expect(childRow.children[0].compType).assertEqual('Text');
      expect(childRow.children[1].compType).assertEqual('Image');
    })

    it('createDVModelFromJson_withBuilder', 0, () => {
      const builderFunc = () => {};
      const json: TestJson = {
        compType: 'Custom',
        build: builderFunc
      };
      const dvModel = createDVModelFromJson(json);
      expect(dvModel.compType).assertEqual('Custom');
      expect(dvModel.builder).assertEqual(builderFunc);
    })

    it('createDVModelFromJson_invalidInput', 0, () => {
      const dvModel = createDVModelFromJson('invalid');
      expect(dvModel).not().assertNull();
      expect(dvModel.compType).assertEqual('');
    })

    it('createDVModelFromJson_emptyObject', 0, () => {
      const json: TestJson = { compType: '' };
      const dvModel = createDVModelFromJson(json);
      expect(dvModel).not().assertNull();
      expect(dvModel.children.length).assertEqual(0);
    })
  })
}