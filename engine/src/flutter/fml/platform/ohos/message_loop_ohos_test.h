/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef FLUTTER_FML_PLATFORM_OHOS_LOOP_TEST_H_
#define FLUTTER_FML_PLATFORM_OHOS_LOOP_TEST_H_

namespace fml {
namespace message_loop_test {

void MessageLoopTestPostTask(void);
void MessageLoopTestObserverFire(void);

}  // namespace message_loop_test
}  // namespace fml

#endif  // FLUTTER_FML_PLATFORM_OHOS_LOOP_TEST_H_
