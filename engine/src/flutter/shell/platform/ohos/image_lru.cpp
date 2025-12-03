// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/ohos/image_lru.h"
#include <stdint.h>
#include <sstream>
#include <string>
#include "fml/logging.h"
#include "fml/time/time_point.h"
#include "fml/trace_event.h"
namespace flutter {

sk_sp<flutter::DlImage> ImageLRU::FindImage(
    NativeBufferKey key,
    OH_NativeBuffer_Config config,
    NativeBufferKey* delete_key = nullptr) {
  if (key == 0) {
    return nullptr;
  }

  if (image_maps_.find(key) != image_maps_.end()) {
    auto& data = image_maps_[key];
    // Need to check the buffer properties to prevent changes in format or size
    // for the same buffer.
    if (data->config.width == config.width &&
        data->config.height == config.height &&
        data->config.format == config.format) {
      int64_t now = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();

      data->timestamp = now;
      image_lists_.splice(image_lists_.begin(), image_lists_, data);

      if (delete_key) {
        *delete_key = TryDeleteOldest(now - kMaxSurvivalSeconds * 1000);
      }

      return data->value;
    }
  }
  return nullptr;
}

NativeBufferKey ImageLRU::AddImage(const sk_sp<flutter::DlImage>& image,
                                   OH_NativeBuffer_Config config,
                                   NativeBufferKey key) {
  int64_t now = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();

  if (image_maps_.find(key) != image_maps_.end()) {
    auto& data = image_maps_[key];
    data->timestamp = now;
    data->config = config;
    data->value = image;
    image_lists_.splice(image_lists_.begin(), image_lists_, data);
    return 0;
  }

  auto delete_key = TryDeleteOldest(now - kMaxSurvivalSeconds * 1000);

  Data data = {key, now, config, image};
  image_lists_.emplace_front(data);
  image_maps_[key] = image_lists_.begin();

  return delete_key;
}

NativeBufferKey ImageLRU::TryDeleteOldest(int64_t min_time_limit) {
  NativeBufferKey delete_key = 0;
  if (image_lists_.size() >= kMaxQueueSize ||
      (image_lists_.size() > 0 && min_time_limit > 0 &&
       image_lists_.back().timestamp < min_time_limit)) {
    delete_key = image_lists_.back().key;
    std::ostringstream trace_str;
    trace_str << "key:" << delete_key
              << "-timestamp:" << image_lists_.back().timestamp
              << "-min_time_limit:" << min_time_limit
              << "-cache_size:" << image_lists_.size();
    TRACE_EVENT1("flutter", "lru-release-", "lru", trace_str.str().c_str());
    FML_LOG(INFO) << "lru-release-" << trace_str.str();

    image_maps_.erase(delete_key);
    image_lists_.pop_back();
  }
  return delete_key;
}

void ImageLRU::Clear() {
  image_maps_.clear();
  image_lists_.clear();
}

}  // namespace flutter
