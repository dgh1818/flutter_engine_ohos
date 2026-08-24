/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */


#include <cstdlib>
#include <cstring>

#include "napi/native_api.h"

namespace {

napi_extended_error_info g_error_info = {
    "ace_napi_stub: no JS runtime",
    nullptr,
    0,
    napi_ok,
};

}  // namespace

extern "C" {

void napi_module_register(napi_module* /*mod*/) {}

napi_status napi_get_last_error_info(napi_env /*env*/,
                                     const napi_extended_error_info** result) {
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  *result = &g_error_info;
  return napi_ok;
}

napi_status napi_throw_error(napi_env /*env*/,
                             const char* /*code*/,
                             const char* /*msg*/) {
  return napi_ok;
}

napi_status napi_throw_type_error(napi_env /*env*/,
                                  const char* /*code*/,
                                  const char* /*msg*/) {
  return napi_ok;
}

napi_status napi_is_exception_pending(napi_env /*env*/, bool* result) {
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  *result = false;
  return napi_ok;
}

napi_status napi_typeof(napi_env /*env*/,
                        napi_value /*value*/,
                        napi_valuetype* result) {
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  *result = napi_undefined;
  return napi_ok;
}

napi_status napi_get_undefined(napi_env /*env*/, napi_value* result) {
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  *result = nullptr;
  return napi_ok;
}

napi_status napi_get_boolean(napi_env env, bool /*value*/, napi_value* result) {
  if (env == nullptr) {
    return napi_invalid_arg;
  }
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  *result = nullptr;
  return napi_ok;
}

napi_status napi_get_cb_info(napi_env env,
                             napi_callback_info /*cbinfo*/,
                             size_t* argc,
                             napi_value* /*argv*/,
                             napi_value* this_arg,
                             void** data) {
  if (env == nullptr) {
    return napi_invalid_arg;
  }
  if (argc != nullptr) {
    *argc = 0;
  }
  if (this_arg != nullptr) {
    *this_arg = nullptr;
  }
  if (data != nullptr) {
    *data = nullptr;
  }
  return napi_ok;
}

napi_status napi_create_int32(napi_env env, int32_t /*value*/, napi_value* result) {
  if (env == nullptr) {
    return napi_invalid_arg;
  }
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  *result = nullptr;
  return napi_ok;
}

napi_status napi_create_uint32(napi_env /*env*/, uint32_t /*value*/, napi_value* result) {
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  *result = nullptr;
  return napi_ok;
}

napi_status napi_create_int64(napi_env env, int64_t /*value*/, napi_value* result) {
  if (env == nullptr) {
    return napi_invalid_arg;
  }
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  *result = nullptr;
  return napi_ok;
}

napi_status napi_create_double(napi_env env, double /*value*/, napi_value* result) {
  if (env == nullptr) {
    return napi_invalid_arg;
  }
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  *result = nullptr;
  return napi_ok;
}

napi_status napi_create_bigint_uint64(napi_env /*env*/,
                                      uint64_t /*value*/,
                                      napi_value* result) {
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  *result = nullptr;
  return napi_ok;
}

napi_status napi_create_arraybuffer(napi_env /*env*/,
                                    size_t byte_length,
                                    void** data,
                                    napi_value* result) {
  if (data != nullptr) {
    *data = byte_length == 0 ? nullptr : std::malloc(byte_length);
  }
  if (result != nullptr) {
    *result = nullptr;
  }
  return napi_ok;
}

napi_status napi_create_typedarray(napi_env /*env*/,
                                   napi_typedarray_type /*type*/,
                                   size_t /*length*/,
                                   napi_value /*arraybuffer*/,
                                   size_t /*byte_offset*/,
                                   napi_value* result) {
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  *result = nullptr;
  return napi_ok;
}

napi_status napi_create_promise(napi_env /*env*/,
                                napi_deferred* deferred,
                                napi_value* promise) {
  if (deferred != nullptr) {
    *deferred = nullptr;
  }
  if (promise != nullptr) {
    *promise = nullptr;
  }
  return napi_ok;
}

napi_status napi_resolve_deferred(napi_env /*env*/,
                                  napi_deferred /*deferred*/,
                                  napi_value /*resolution*/) {
  return napi_ok;
}

napi_status napi_reject_deferred(napi_env /*env*/,
                                 napi_deferred /*deferred*/,
                                 napi_value /*rejection*/) {
  return napi_ok;
}

napi_status napi_create_reference(napi_env /*env*/,
                                  napi_value /*value*/,
                                  uint32_t /*initial_refcount*/,
                                  napi_ref* result) {
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  *result = nullptr;
  return napi_ok;
}

napi_status napi_delete_reference(napi_env /*env*/, napi_ref /*ref*/) {
  return napi_ok;
}

napi_status napi_reference_unref(napi_env /*env*/, napi_ref /*ref*/, uint32_t* result) {
  if (result != nullptr) {
    *result = 0;
  }
  return napi_ok;
}

napi_status napi_define_properties(napi_env /*env*/,
                                   napi_value /*object*/,
                                   size_t /*property_count*/,
                                   const napi_property_descriptor* /*properties*/) {
  return napi_ok;
}

napi_status napi_unwrap(napi_env /*env*/, napi_value /*js_object*/, void** result) {
  if (result != nullptr) {
    *result = nullptr;
  }
  return napi_ok;
}

napi_status napi_is_array(napi_env /*env*/, napi_value /*value*/, bool* result) {
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  *result = false;
  return napi_ok;
}

napi_status napi_is_arraybuffer(napi_env /*env*/, napi_value /*value*/, bool* result) {
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  *result = false;
  return napi_ok;
}

napi_status napi_is_typedarray(napi_env /*env*/, napi_value /*value*/, bool* result) {
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  *result = false;
  return napi_ok;
}

napi_status napi_get_array_length(napi_env /*env*/, napi_value /*value*/, uint32_t* result) {
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  *result = 0;
  return napi_ok;
}

napi_status napi_get_element(napi_env /*env*/,
                             napi_value /*object*/,
                             uint32_t /*index*/,
                             napi_value* result) {
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  *result = nullptr;
  return napi_ok;
}

napi_status napi_get_arraybuffer_info(napi_env /*env*/,
                                      napi_value /*arraybuffer*/,
                                      void** data,
                                      size_t* byte_length) {
  if (data != nullptr) {
    *data = nullptr;
  }
  if (byte_length != nullptr) {
    *byte_length = 0;
  }
  return napi_ok;
}

napi_status napi_get_typedarray_info(napi_env /*env*/,
                                     napi_value /*typedarray*/,
                                     napi_typedarray_type* type,
                                     size_t* length,
                                     void** data,
                                     napi_value* arraybuffer,
                                     size_t* byte_offset) {
  if (type != nullptr) {
    *type = napi_uint8_array;
  }
  if (length != nullptr) {
    *length = 0;
  }
  if (data != nullptr) {
    *data = nullptr;
  }
  if (arraybuffer != nullptr) {
    *arraybuffer = nullptr;
  }
  if (byte_offset != nullptr) {
    *byte_offset = 0;
  }
  return napi_ok;
}

napi_status napi_get_value_bool(napi_env /*env*/, napi_value /*value*/, bool* result) {
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  *result = false;
  return napi_ok;
}

napi_status napi_get_value_int32(napi_env /*env*/, napi_value /*value*/, int32_t* result) {
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  *result = 0;
  return napi_ok;
}

napi_status napi_get_value_uint32(napi_env /*env*/, napi_value /*value*/, uint32_t* result) {
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  *result = 0;
  return napi_ok;
}

napi_status napi_get_value_int64(napi_env /*env*/, napi_value /*value*/, int64_t* result) {
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  *result = 0;
  return napi_ok;
}

napi_status napi_get_value_double(napi_env /*env*/, napi_value /*value*/, double* result) {
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  *result = 0.0;
  return napi_ok;
}

napi_status napi_get_value_string_utf8(napi_env /*env*/,
                                       napi_value /*value*/,
                                       char* buf,
                                       size_t bufsize,
                                       size_t* result) {
  if (buf != nullptr && bufsize > 0) {
    buf[0] = '\0';
  }
  if (result != nullptr) {
    *result = 0;
  }
  return napi_ok;
}

napi_status napi_get_value_bigint_int64(napi_env /*env*/,
                                        napi_value /*value*/,
                                        int64_t* result,
                                        bool* lossless) {
  if (result != nullptr) {
    *result = 0;
  }
  if (lossless != nullptr) {
    *lossless = true;
  }
  return napi_ok;
}

napi_status napi_get_value_bigint_uint64(napi_env /*env*/,
                                         napi_value /*value*/,
                                         uint64_t* result,
                                         bool* lossless) {
  if (result != nullptr) {
    *result = 0;
  }
  if (lossless != nullptr) {
    *lossless = true;
  }
  return napi_ok;
}

napi_status napi_get_uv_event_loop(napi_env /*env*/, struct uv_loop_s** loop) {
  if (loop == nullptr) {
    return napi_invalid_arg;
  }
  *loop = nullptr;
  return napi_ok;
}

napi_status napi_create_async_work(napi_env /*env*/,
                                   napi_value /*async_resource*/,
                                   napi_value /*async_resource_name*/,
                                   napi_async_execute_callback /*execute*/,
                                   napi_async_complete_callback /*complete*/,
                                   void* /*data*/,
                                   napi_async_work* result) {
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  *result = nullptr;
  return napi_ok;
}

napi_status napi_delete_async_work(napi_env /*env*/, napi_async_work /*work*/) {
  return napi_ok;
}

napi_status napi_queue_async_work_with_qos(napi_env /*env*/,
                                           napi_async_work /*work*/,
                                           napi_qos_t /*qos*/) {
  return napi_ok;
}

}  // extern "C"

extern "C" napi_status napi_open_handle_scope(napi_env env, napi_handle_scope* result) {
  if (result) {
    *result = reinterpret_cast<napi_handle_scope>(0x1);
  }
  return napi_ok;
}

extern "C" napi_status napi_close_handle_scope(napi_env env, napi_handle_scope scope) {
  return napi_ok;
}

extern "C" napi_status napi_create_array(napi_env env, napi_value* result) {
  if (result) {
    *result = reinterpret_cast<napi_value>(0x2);
  }
  return napi_ok;
}

extern "C" napi_status napi_create_string_utf8(napi_env env, const char* str, size_t length, napi_value* result) {
  if (result) {
    *result = reinterpret_cast<napi_value>(0x3);
  }
  return napi_ok;
}

extern "C" napi_status napi_set_element(napi_env env, napi_value object, uint32_t index, napi_value value) {
  return napi_ok;
}

extern "C" napi_status napi_get_reference_value(napi_env env, napi_ref ref, napi_value* result) {
  if (result) {
    *result = reinterpret_cast<napi_value>(0x4);
  }
  return napi_ok;
}

extern "C" napi_status napi_get_named_property(napi_env env, napi_value object, const char* name, napi_value* result) {
  if (result) {
    *result = reinterpret_cast<napi_value>(0x5);
  }
  return napi_ok;
}

extern "C" napi_status napi_call_function(napi_env env, napi_value recv, napi_value fn, size_t argc, const napi_value* argv, napi_value* result) {
  return napi_ok;
}
