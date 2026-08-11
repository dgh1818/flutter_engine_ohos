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
import { TraceSection } from '../../../../../main/ets/util/TraceSection';

export default function TraceSectionTest() {
  describe('TraceSectionTest', () => {
    it('begin', 0, () => {
      const sectionName = "DartExecutor";
      expect(TraceSection.begin(sectionName)).assertLarger(0);
    });
    it('end', 0, () => {
      const sectionName = "DartExecutor";
      TraceSection.end(sectionName);
    });
    it('endWithId', 0, () => {
      const sectionName = "DartExecutor";
      TraceSection.endWithId(sectionName, 1);
    });
    it('begin_and_end_pair', 0, () => {
      const sectionName = "DartExecutor";
      const taskId = TraceSection.begin(sectionName);
      expect(taskId).assertLarger(0);
      // end应该使用与begin相同的taskId
      TraceSection.end(sectionName); // 验证不抛异常
    });
  })
}