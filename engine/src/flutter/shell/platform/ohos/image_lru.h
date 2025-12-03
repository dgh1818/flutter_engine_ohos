// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_IMAGE_LRU_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_IMAGE_LRU_H_

#include <native_buffer/native_buffer.h>
#include <stdint.h>
#include <array>
#include <cstddef>
#include <list>
#include <unordered_map>

#include "display_list/image/dl_image.h"

namespace flutter {

// This value needs to be larger than the number of swapchain images
// that a typical image reader will produce to ensure that we effectively
// cache. If the value is too small, we will unnecessarily churn through
// images, while if it is too large, it does not matter because unused buffers
// will be released after at most one minute.
// OHOS' camera queue size is 8
// OHOS' video queue size is 9~30
static constexpr size_t kMaxQueueSize = 50u;
static constexpr int64_t kMaxSurvivalSeconds = 60;

using NativeBufferKey = uint64_t;

class ImageLRU {
 public:
  ImageLRU() = default;

  ~ImageLRU() = default;

  /// @brief Retrieve the image associated with the given [key], or nullptr.
  sk_sp<flutter::DlImage> FindImage(NativeBufferKey key,
                                    OH_NativeBuffer_Config config,
                                    NativeBufferKey* delete_key);

  /// @brief Add a new image to the cache with a key, returning the key of the
  ///        LRU entry that was removed.
  ///
  /// The value may be `0`, in which case nothing was removed.
  NativeBufferKey AddImage(const sk_sp<flutter::DlImage>& image,
                           OH_NativeBuffer_Config config,
                           NativeBufferKey key);

  /// @brief Remove all entires from the image cache.
  void Clear();

 private:
  NativeBufferKey TryDeleteOldest(int64_t min_time_limit);

  struct Data {
    NativeBufferKey key = 0u;
    int64_t timestamp = 0;
    OH_NativeBuffer_Config config;
    sk_sp<flutter::DlImage> value;
  };

  std::list<Data> image_lists_;
  std::unordered_map<NativeBufferKey, std::list<Data>::iterator> image_maps_;
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_OHOS_IMAGE_LRU_H_
