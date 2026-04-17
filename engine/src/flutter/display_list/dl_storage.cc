// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/display_list/dl_storage.h"
#ifdef FML_OS_OHOS
#include "flutter/fml/platform/ohos/restrace.h"
#endif


namespace flutter {

static constexpr inline bool is_power_of_two(int value) {
  return (value & (value - 1)) == 0;
}

// static
size_t DisplayListStorage::NextPowerOfTwoSize(size_t x) {
  if (x == 0) {
    return 1;
  }

  --x;

  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  if constexpr (sizeof(size_t) > 4) {
    x |= x >> 32;
  }

  return x + 1;
}

DisplayListStorage::~DisplayListStorage() {
  #ifdef FML_OS_OHOS
  OH_RESTRACE_FREE_REGION(ptr_.get(), allocated_);
  #endif
}

void DisplayListStorage::realloc(size_t count) {
  uint8_t* old_ptr = ptr_.release();
  uint8_t* new_ptr = static_cast<uint8_t*>(std::realloc(old_ptr, count));
  if (!new_ptr) {
    ptr_.reset(old_ptr);
    FML_CHECK(false) << "relloc failed for size" << count;
    return;
  }
  #ifdef FML_OS_OHOS
  OH_RESTRACE_FREE_REGION(old_ptr, allocated_);
  OH_RESTRACE(new_ptr, count);
  #endif
  ptr_.reset(new_ptr);
  FML_CHECK(ptr_);
  allocated_ = count;
}

uint8_t* DisplayListStorage::allocate(size_t needed) {
  if (used_ + needed > allocated_) {
    static_assert(is_power_of_two(kDLPageSize),
                  "This math needs updating for non-pow2.");

    // NPOT, with minimum size of kDLPageSize.
    size_t new_size = std::max(NextPowerOfTwoSize(used_ + needed), kDLPageSize);
    size_t old_size = allocated_;
    realloc(new_size);
    FML_CHECK(ptr_.get());
    FML_CHECK(allocated_ == new_size);
    FML_CHECK(allocated_ >= old_size);
    FML_CHECK(used_ + needed <= allocated_);
    memset(ptr_.get() + used_, 0, allocated_ - old_size);
  }
  uint8_t* ret = ptr_.get() + used_;
  used_ += needed;
  FML_CHECK(used_ <= allocated_);
  return ret;
}

DisplayListStorage::DisplayListStorage(DisplayListStorage&& source) {
  ptr_ = std::move(source.ptr_);
  used_ = source.used_;
  allocated_ = source.allocated_;
  source.used_ = 0u;
  source.allocated_ = 0u;
}

void DisplayListStorage::reset() {
  ptr_.reset();
  used_ = 0u;
  allocated_ = 0u;
}

DisplayListStorage& DisplayListStorage::operator=(DisplayListStorage&& source) {
  ptr_ = std::move(source.ptr_);
  used_ = source.used_;
  allocated_ = source.allocated_;
  source.used_ = 0u;
  source.allocated_ = 0u;
  return *this;
}

}  // namespace flutter
