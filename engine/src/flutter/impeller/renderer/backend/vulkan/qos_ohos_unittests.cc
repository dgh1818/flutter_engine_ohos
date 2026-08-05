/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

// OHOS-only smoke assertions for the FML_OS_OHOS low-memory QoS branches in
// FenceWaiterVK (fence_waiter_vk.cc) and ResourceManagerVK
// (resource_manager_vk.cc): after setQosOnLowMemory() the wait/reclaim loops
// still perform their core function. The actual QoS effect (thread priority)
// has no getter and is device behavior, so it is not observable from here.
// Registered only in flutter_ohos_unittests; the guard keeps this file an
// empty translation unit on non-OHOS builds.
#include "flutter/fml/build_config.h"

#include "fml/closure.h"
#include "fml/synchronization/waitable_event.h"
#include "gtest/gtest.h"
#include "impeller/renderer/backend/vulkan/fence_waiter_vk.h"
#include "impeller/renderer/backend/vulkan/resource_manager_vk.h"
#include "impeller/renderer/backend/vulkan/test/mock_vulkan.h"

#if defined(FML_OS_OHOS)

namespace impeller {
namespace testing {

TEST(QosOhosTest, FenceWaiterWorksAfterSetQosOnLowMemory) {
  auto const context = MockVulkanContextBuilder().Build();
  ASSERT_NE(context, nullptr);
  auto const device = context->GetDevice();
  auto const waiter = context->GetFenceWaiter();

  // Queues a QoS event processed by the OHOS branch of FenceWaiterVK::Main.
  // A failing native QoS call only logs, it does not abort the wait loop.
  waiter->setQosOnLowMemory(OHOS_MEMORY_LEVEL_CRITICAL);

  auto signal = fml::ManualResetWaitableEvent();
  auto fence = device.createFenceUnique({}).value;
  waiter->AddFence(std::move(fence), [&signal]() { signal.Signal(); });

  signal.Wait();
}

TEST(QosOhosTest, ResourceManagerReclaimsAfterSetQosOnLowMemory) {
  auto const manager = ResourceManagerVK::Create();

  // Queues a QoS event processed by the OHOS branch of ResourceManagerVK::
  // Start. A failing native QoS call only logs, it does not abort the
  // reclaim loop.
  manager->setQosOnLowMemory(OHOS_MEMORY_LEVEL_CRITICAL);

  auto waiter = fml::AutoResetWaitableEvent();
  auto rattle = fml::ScopedCleanupClosure([&waiter]() { waiter.Signal(); });

  // Not killed immediately.
  EXPECT_FALSE(waiter.IsSignaledForTest());

  {
    auto resource = UniqueResourceVKT<fml::ScopedCleanupClosure>(
        manager, std::move(rattle));
  }

  waiter.Wait();
}

}  // namespace testing
}  // namespace impeller

#endif  // defined(FML_OS_OHOS)
