/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#define FML_USED_ON_EMBEDDER

#include "flutter/shell/platform/ohos/platform_message_handler_ohos.h"
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "flutter/fml/mapping.h"
#include "flutter/fml/memory/ref_counted.h"
#include "flutter/fml/memory/ref_ptr.h"
#include "flutter/fml/message_loop_impl.h"
#include "flutter/fml/task_runner.h"
#include "flutter/lib/ui/window/platform_message.h"
#include "flutter/lib/ui/window/platform_message_response.h"
#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"

namespace flutter {
namespace testing {

// Mock PlatformMessageResponse that tracks Complete/CompleteEmpty calls
class MockPlatformMessageResponse : public PlatformMessageResponse {
 public:
  static fml::RefPtr<MockPlatformMessageResponse> Create() {
    return fml::AdoptRef(new MockPlatformMessageResponse());
  }

  void Complete(std::unique_ptr<fml::Mapping> data) override {
    is_complete_called_ = true;
    if (data) {
      complete_data_ = std::string(data->GetMapping(), data->GetMapping() + data->GetSize());
    }
  }

  void CompleteEmpty() override { is_complete_empty_called_ = true; }

  bool is_complete_called() const { return is_complete_called_; }
  bool is_complete_empty_called() const { return is_complete_empty_called_; }
  const std::string& complete_data() const { return complete_data_; }

 private:
  MockPlatformMessageResponse() = default;
  ~MockPlatformMessageResponse() override = default;

  bool is_complete_called_ = false;
  bool is_complete_empty_called_ = false;
  std::string complete_data_;
};

class PlatformMessageHandlerOHOSTest : public ::testing::Test {
 protected:
  void SetUp() override {
    loop_ = fml::MessageLoopImpl::Create(nullptr);
    ASSERT_TRUE(loop_);
    task_runner_ = fml::MakeRefCounted<fml::TaskRunner>(loop_);
    napi_facade_ = std::make_shared<PlatformViewOHOSNapi>(nullptr);
    handler_ = std::make_unique<PlatformMessageHandlerOHOS>(napi_facade_,
                                                            task_runner_);
  }

  void TearDown() override {
    handler_.reset();
    napi_facade_.reset();
    task_runner_ = nullptr;
    loop_->Terminate();
    loop_ = nullptr;
  }

  std::shared_ptr<PlatformViewOHOSNapi> napi_facade_;
  fml::RefPtr<fml::MessageLoopImpl> loop_;
  fml::RefPtr<fml::TaskRunner> task_runner_;
  std::unique_ptr<PlatformMessageHandlerOHOS> handler_;
};

// DoesHandlePlatformMessageOnPlatformThread should return true
TEST_F(PlatformMessageHandlerOHOSTest,
       DoesHandlePlatformMessageOnPlatformThreadReturnsTrue) {
  EXPECT_TRUE(handler_->DoesHandlePlatformMessageOnPlatformThread());
}

// InvokePlatformMessageResponseCallback with non-existent response_id should return early
TEST_F(PlatformMessageHandlerOHOSTest,
       InvokeResponseCallbackWithUnknownIdReturnsEarly) {
  auto mapping = fml::MallocMapping::Copy(
      reinterpret_cast<const uint8_t*>("hello"), 5);
  handler_->InvokePlatformMessageResponseCallback(
      999, std::make_unique<fml::MallocMapping>(std::move(mapping)));
  SUCCEED();
}

// InvokePlatformMessageEmptyResponseCallback with non-existent response_id should return early
TEST_F(PlatformMessageHandlerOHOSTest,
       InvokeEmptyResponseCallbackWithUnknownIdReturnsEarly) {
  handler_->InvokePlatformMessageEmptyResponseCallback(999);
  SUCCEED();
}

// HandlePlatformMessage with a response should store the response into
// pending_responses_ (covers if (auto response = message->response()) true branch)
TEST_F(PlatformMessageHandlerOHOSTest,
       HandlePlatformMessageWithResponseRegistersPending) {
  auto mock_response = MockPlatformMessageResponse::Create();
  auto message = std::make_unique<PlatformMessage>("test_channel",
                                                    mock_response);
  // PostTask only enqueues without executing; napi_env=nullptr won't crash
  handler_->HandlePlatformMessage(std::move(message));
  // response_id starts at 1; verify Invoke can find the pending response
  auto mapping = fml::MallocMapping::Copy(
      reinterpret_cast<const uint8_t*>("data"), 4);
  handler_->InvokePlatformMessageResponseCallback(
      1, std::make_unique<fml::MallocMapping>(std::move(mapping)));
  EXPECT_TRUE(mock_response->is_complete_called());
}

// HandlePlatformMessage without a response
// (covers if (auto response = message->response()) false branch)
// Verifies: messages without a response are not registered, so subsequent
// Invoke cannot find that id; but messages with a response (incrementing id)
// can still be invoked normally.
TEST_F(PlatformMessageHandlerOHOSTest,
       HandlePlatformMessageWithoutResponseSkipsRegistration) {
  // id=1: message without response — should NOT be registered
  auto message_no_response = std::make_unique<PlatformMessage>(
      "test_channel", fml::RefPtr<PlatformMessageResponse>());
  handler_->HandlePlatformMessage(std::move(message_no_response));

  // id=2: message with response — should be registered
  auto mock_response = MockPlatformMessageResponse::Create();
  auto message_with_response =
      std::make_unique<PlatformMessage>("test_channel", mock_response);
  handler_->HandlePlatformMessage(std::move(message_with_response));

  // Invoke id=1 (unregistered) — should not complete anything
  handler_->InvokePlatformMessageEmptyResponseCallback(1);
  EXPECT_FALSE(mock_response->is_complete_empty_called());

  // Invoke id=2 (registered) — should complete the response
  handler_->InvokePlatformMessageEmptyResponseCallback(2);
  EXPECT_TRUE(mock_response->is_complete_empty_called());
}

// InvokePlatformMessageResponseCallback finds the pending response and calls Complete
// (covers happy path: response_id matches -> message_response->Complete)
TEST_F(PlatformMessageHandlerOHOSTest,
       InvokeResponseCallbackCompletesPendingResponse) {
  auto mock_response = MockPlatformMessageResponse::Create();
  auto message = std::make_unique<PlatformMessage>("test_channel",
                                                    mock_response);
  handler_->HandlePlatformMessage(std::move(message));

  auto mapping = fml::MallocMapping::Copy(
      reinterpret_cast<const uint8_t*>("hello"), 5);
  handler_->InvokePlatformMessageResponseCallback(
      1, std::make_unique<fml::MallocMapping>(std::move(mapping)));
  EXPECT_TRUE(mock_response->is_complete_called());
  EXPECT_EQ(mock_response->complete_data(), "hello");
}

// InvokePlatformMessageEmptyResponseCallback finds the pending response and calls
// CompleteEmpty (covers happy path: response_id matches ->
// message_response->CompleteEmpty)
TEST_F(PlatformMessageHandlerOHOSTest,
       InvokeEmptyResponseCallbackCompletesPendingResponse) {
  auto mock_response = MockPlatformMessageResponse::Create();
  auto message = std::make_unique<PlatformMessage>("test_channel",
                                                    mock_response);
  handler_->HandlePlatformMessage(std::move(message));

  handler_->InvokePlatformMessageEmptyResponseCallback(1);
  EXPECT_TRUE(mock_response->is_complete_empty_called());
}

// Zero-ID guard: even with a registered response, calling with response_id=0
// should not trigger the callback. First register a response (id=1), then
// call with 0 to verify no trigger, and finally with the correct id=1 to
// verify it does trigger.
TEST_F(PlatformMessageHandlerOHOSTest,
       InvokeResponseCallbackWithZeroIdDoesNotCompleteRegisteredResponse) {
  auto mock_response = MockPlatformMessageResponse::Create();
  auto message =
      std::make_unique<PlatformMessage>("test_channel", mock_response);
  handler_->HandlePlatformMessage(std::move(message));

  // response_id=0 should be ignored even though id=1 is registered
  handler_->InvokePlatformMessageResponseCallback(0, nullptr);
  EXPECT_FALSE(mock_response->is_complete_called());

  // Correct id=1 should trigger the callback
  auto mapping = fml::MallocMapping::Copy(
      reinterpret_cast<const uint8_t*>("data"), 4);
  handler_->InvokePlatformMessageResponseCallback(
      1, std::make_unique<fml::MallocMapping>(std::move(mapping)));
  EXPECT_TRUE(mock_response->is_complete_called());
}

// Zero-ID guard (Empty variant): even with a registered response, calling
// InvokePlatformMessageEmptyResponseCallback with response_id=0 should not
// trigger CompleteEmpty.
TEST_F(PlatformMessageHandlerOHOSTest,
       InvokeEmptyResponseCallbackWithZeroIdDoesNotCompleteRegisteredResponse) {
  auto mock_response = MockPlatformMessageResponse::Create();
  auto message =
      std::make_unique<PlatformMessage>("test_channel", mock_response);
  handler_->HandlePlatformMessage(std::move(message));

  // response_id=0 should be ignored even though id=1 is registered
  handler_->InvokePlatformMessageEmptyResponseCallback(0);
  EXPECT_FALSE(mock_response->is_complete_empty_called());

  // Correct id=1 should trigger the callback
  handler_->InvokePlatformMessageEmptyResponseCallback(1);
  EXPECT_TRUE(mock_response->is_complete_empty_called());
}

}  // namespace testing
}  // namespace flutter
