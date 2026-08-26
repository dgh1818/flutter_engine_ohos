/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include <array>
#include <cstring>
#include <new>
#include "napi/native_api.h"

namespace {

napi_extended_error_info g_error_info = {
    "ace_napi_stub: no JS runtime",
    nullptr,
    0,
    napi_ok,
};

struct NapiStubState {
  napi_valuetype valuetype = napi_undefined;
  bool force_valuetype = false;
  const char* string_value = "";
  uint32_t array_length = 0;
  bool is_arraybuffer = false;
  bool is_array = false;
  bool is_typedarray = false;
  void* arraybuffer_data = nullptr;
  size_t arraybuffer_len = 0;
  napi_value create_arraybuffer_value = nullptr;
  napi_status fail_typeof = napi_ok;
  size_t cb_argc = 0;
  int fail_int64_on_call = 0;
  int int64_calls = 0;
  int32_t int32_value = 0;
  int fail_int32_on_call = 0;
  int int32_calls = 0;
  int fail_uint32_on_call = 0;
  int uint32_calls = 0;
  int64_t int64_value = 0;
  double double_value = 0.0;
  int fail_double_on_call = 0;
  int double_calls = 0;
  int fail_get_boolean_on_call = 0;
  int get_boolean_calls = 0;
  int fail_bool_on_call = 0;
  int bool_calls = 0;
  bool bigint_lossless = true;
  napi_status fail_string_utf8 = napi_ok;
  int fail_string_utf8_skip = 0;
  napi_status fail_array_length = napi_ok;
  napi_status fail_is_arraybuffer = napi_ok;
  napi_status fail_is_array = napi_ok;
  napi_status fail_arraybuffer_info = napi_ok;
  napi_status fail_create_arraybuffer = napi_ok;
  napi_status fail_reference = napi_ok;
  napi_status fail_named_property = napi_ok;
  napi_status fail_call_function = napi_ok;
  napi_module* registered_module = nullptr;
  std::array<void*, 128> live_arraybuffer_data = {};
  size_t live_arraybuffer_count = 0;
};

NapiStubState g_napi_stub;

}

extern "C" {

void napi_module_register(napi_module* mod) {
  g_napi_stub.registered_module = mod;
}

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
  if (g_napi_stub.fail_typeof != napi_ok) {
    napi_status s = g_napi_stub.fail_typeof;
    g_napi_stub.fail_typeof = napi_ok;
    return s;
  }
  *result = g_napi_stub.force_valuetype ? g_napi_stub.valuetype
                                        : napi_undefined;
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
  ++g_napi_stub.get_boolean_calls;
  if (g_napi_stub.fail_get_boolean_on_call != 0 &&
      g_napi_stub.get_boolean_calls >= g_napi_stub.fail_get_boolean_on_call) {
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
    *argc = g_napi_stub.cb_argc;
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
  if (g_napi_stub.fail_create_arraybuffer != napi_ok) {
    napi_status s = g_napi_stub.fail_create_arraybuffer;
    g_napi_stub.fail_create_arraybuffer = napi_ok;
    return s;
  }
  if (data != nullptr) {
    void* buffer = byte_length == 0 ? nullptr : ::operator new(byte_length);
    *data = buffer;
    if (buffer != nullptr &&
        g_napi_stub.live_arraybuffer_count < 128) {
      g_napi_stub.live_arraybuffer_data[g_napi_stub.live_arraybuffer_count++] =
          buffer;
    }
  }
  if (result != nullptr) {
    *result = g_napi_stub.create_arraybuffer_value;
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
  if (g_napi_stub.fail_is_array != napi_ok) {
    napi_status s = g_napi_stub.fail_is_array;
    g_napi_stub.fail_is_array = napi_ok;
    return s;
  }
  *result = g_napi_stub.is_array;
  return napi_ok;
}

napi_status napi_is_arraybuffer(napi_env /*env*/, napi_value /*value*/, bool* result) {
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  if (g_napi_stub.fail_is_arraybuffer != napi_ok) {
    napi_status s = g_napi_stub.fail_is_arraybuffer;
    g_napi_stub.fail_is_arraybuffer = napi_ok;
    return s;
  }
  *result = g_napi_stub.is_arraybuffer;
  return napi_ok;
}

napi_status napi_is_typedarray(napi_env /*env*/, napi_value /*value*/, bool* result) {
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  *result = g_napi_stub.is_typedarray;
  return napi_ok;
}

napi_status napi_get_array_length(napi_env /*env*/, napi_value /*value*/, uint32_t* result) {
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  if (g_napi_stub.fail_array_length != napi_ok) {
    napi_status s = g_napi_stub.fail_array_length;
    g_napi_stub.fail_array_length = napi_ok;
    return s;
  }
  *result = g_napi_stub.array_length;
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
  if (g_napi_stub.fail_arraybuffer_info != napi_ok) {
    napi_status s = g_napi_stub.fail_arraybuffer_info;
    g_napi_stub.fail_arraybuffer_info = napi_ok;
    return s;
  }
  if (data != nullptr) {
    *data = g_napi_stub.arraybuffer_data;
  }
  if (byte_length != nullptr) {
    *byte_length = g_napi_stub.arraybuffer_len;
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
  ++g_napi_stub.bool_calls;
  if (g_napi_stub.fail_bool_on_call != 0 &&
      g_napi_stub.bool_calls >= g_napi_stub.fail_bool_on_call) {
    return napi_invalid_arg;
  }
  *result = false;
  return napi_ok;
}

napi_status napi_get_value_int32(napi_env /*env*/, napi_value /*value*/, int32_t* result) {
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  ++g_napi_stub.int32_calls;
  if (g_napi_stub.fail_int32_on_call != 0 &&
      g_napi_stub.int32_calls >= g_napi_stub.fail_int32_on_call) {
    return napi_invalid_arg;
  }
  *result = g_napi_stub.int32_value;
  return napi_ok;
}

napi_status napi_get_value_uint32(napi_env /*env*/, napi_value /*value*/, uint32_t* result) {
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  ++g_napi_stub.uint32_calls;
  if (g_napi_stub.fail_uint32_on_call != 0 &&
      g_napi_stub.uint32_calls >= g_napi_stub.fail_uint32_on_call) {
    return napi_invalid_arg;
  }
  *result = 0;
  return napi_ok;
}

napi_status napi_get_value_int64(napi_env /*env*/, napi_value /*value*/, int64_t* result) {
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  ++g_napi_stub.int64_calls;
  if (g_napi_stub.fail_int64_on_call != 0 &&
      g_napi_stub.int64_calls >= g_napi_stub.fail_int64_on_call) {
    return napi_invalid_arg;
  }
  *result = g_napi_stub.int64_value;
  return napi_ok;
}

napi_status napi_get_value_double(napi_env /*env*/, napi_value /*value*/, double* result) {
  if (result == nullptr) {
    return napi_invalid_arg;
  }
  ++g_napi_stub.double_calls;
  if (g_napi_stub.fail_double_on_call != 0 &&
      g_napi_stub.double_calls >= g_napi_stub.fail_double_on_call) {
    return napi_invalid_arg;
  }
  *result = g_napi_stub.double_value;
  return napi_ok;
}

napi_status napi_get_value_string_utf8(napi_env /*env*/,
                                       napi_value /*value*/,
                                       char* buf,
                                       size_t bufsize,
                                       size_t* result) {
  if (g_napi_stub.fail_string_utf8 != napi_ok) {
    if (g_napi_stub.fail_string_utf8_skip > 0) {
      g_napi_stub.fail_string_utf8_skip--;
    } else {
      napi_status s = g_napi_stub.fail_string_utf8;
      g_napi_stub.fail_string_utf8 = napi_ok;
      return s;
    }
  }
  size_t len = std::strlen(g_napi_stub.string_value);
  if (buf == nullptr || bufsize == 0) {
    if (result != nullptr) {
      *result = len;
    }
    return napi_ok;
  }
  size_t copy = len < bufsize - 1 ? len : bufsize - 1;
  if (copy > 0) {
    std::memcpy(buf, g_napi_stub.string_value, copy);
  }
  buf[copy] = '\0';
  if (result != nullptr) {
    *result = copy;
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
    *lossless = g_napi_stub.bigint_lossless;
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

}

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
  if (g_napi_stub.fail_reference != napi_ok) {
    napi_status s = g_napi_stub.fail_reference;
    g_napi_stub.fail_reference = napi_ok;
    return s;
  }
  if (result) {
    *result = reinterpret_cast<napi_value>(0x4);
  }
  return napi_ok;
}

extern "C" napi_status napi_get_named_property(napi_env env, napi_value object, const char* name, napi_value* result) {
  if (g_napi_stub.fail_named_property != napi_ok) {
    napi_status s = g_napi_stub.fail_named_property;
    g_napi_stub.fail_named_property = napi_ok;
    return s;
  }
  if (result) {
    *result = reinterpret_cast<napi_value>(0x5);
  }
  return napi_ok;
}

extern "C" napi_status napi_call_function(napi_env env, napi_value recv, napi_value fn, size_t argc, const napi_value* argv, napi_value* result) {
  if (g_napi_stub.fail_call_function != napi_ok) {
    napi_status s = g_napi_stub.fail_call_function;
    g_napi_stub.fail_call_function = napi_ok;
    return s;
  }
  return napi_ok;
}

extern "C" void StubNapiReset(void) {
  for (size_t i = 0; i < g_napi_stub.live_arraybuffer_count; i++) {
    ::operator delete(g_napi_stub.live_arraybuffer_data[i]);
  }
  napi_module* mod = g_napi_stub.registered_module;
  g_napi_stub = NapiStubState{};
  g_napi_stub.registered_module = mod;
}

extern "C" void StubNapiSetValuetype(napi_valuetype t) {
  g_napi_stub.force_valuetype = true;
  g_napi_stub.valuetype = t;
}

extern "C" void StubNapiSetString(const char* s) {
  g_napi_stub.string_value = s != nullptr ? s : "";
}

extern "C" void StubNapiSetArrayLength(uint32_t n) {
  g_napi_stub.array_length = n;
}

extern "C" void StubNapiSetArrayLike(bool is_arraybuffer,
                                     bool is_array,
                                     bool is_typedarray) {
  g_napi_stub.is_arraybuffer = is_arraybuffer;
  g_napi_stub.is_array = is_array;
  g_napi_stub.is_typedarray = is_typedarray;
}

extern "C" void StubNapiSetArraybufferData(void* data, size_t len) {
  g_napi_stub.arraybuffer_data = data;
  g_napi_stub.arraybuffer_len = len;
}

extern "C" void StubNapiSetCbArgc(size_t argc) {
  g_napi_stub.cb_argc = argc;
}

extern "C" void StubNapiFailInt64OnCall(int nth) {
  g_napi_stub.fail_int64_on_call = nth;
  g_napi_stub.int64_calls = 0;
}

extern "C" void StubNapiSetInt32Value(int32_t value) {
  g_napi_stub.int32_value = value;
}

extern "C" void StubNapiFailInt32OnCall(int nth) {
  g_napi_stub.fail_int32_on_call = nth;
  g_napi_stub.int32_calls = 0;
}

extern "C" void StubNapiFailUint32OnCall(int nth) {
  g_napi_stub.fail_uint32_on_call = nth;
  g_napi_stub.uint32_calls = 0;
}

extern "C" void StubNapiSetInt64Value(int64_t value) {
  g_napi_stub.int64_value = value;
}

extern "C" void StubNapiSetDoubleValue(double value) {
  g_napi_stub.double_value = value;
}

extern "C" void StubNapiFailDoubleOnCall(int nth) {
  g_napi_stub.fail_double_on_call = nth;
  g_napi_stub.double_calls = 0;
}

extern "C" void StubNapiFailGetBooleanOnCall(int nth) {
  g_napi_stub.fail_get_boolean_on_call = nth;
  g_napi_stub.get_boolean_calls = 0;
}

extern "C" void StubNapiFailBoolOnCall(int nth) {
  g_napi_stub.fail_bool_on_call = nth;
  g_napi_stub.bool_calls = 0;
}

extern "C" void StubNapiSetBigintLossless(bool lossless) {
  g_napi_stub.bigint_lossless = lossless;
}

extern "C" void StubNapiSetCreateArraybufferValue(napi_value v) {
  g_napi_stub.create_arraybuffer_value = v;
}

extern "C" void StubNapiFailTypeof(napi_status s) {
  g_napi_stub.fail_typeof = s;
}

extern "C" void StubNapiFailStringUtf8(napi_status s, int skip) {
  g_napi_stub.fail_string_utf8 = s;
  g_napi_stub.fail_string_utf8_skip = skip;
}

extern "C" void StubNapiFailArrayLength(napi_status s) {
  g_napi_stub.fail_array_length = s;
}

extern "C" void StubNapiFailIsArraybuffer(napi_status s) {
  g_napi_stub.fail_is_arraybuffer = s;
}

extern "C" void StubNapiFailIsArray(napi_status s) {
  g_napi_stub.fail_is_array = s;
}

extern "C" void StubNapiFailArraybufferInfo(napi_status s) {
  g_napi_stub.fail_arraybuffer_info = s;
}

extern "C" void StubNapiFailCreateArraybuffer(napi_status s) {
  g_napi_stub.fail_create_arraybuffer = s;
}

extern "C" void StubNapiFailReference(napi_status s) {
  g_napi_stub.fail_reference = s;
}

extern "C" void StubNapiFailNamedProperty(napi_status s) {
  g_napi_stub.fail_named_property = s;
}

extern "C" void StubNapiFailCallFunction(napi_status s) {
  g_napi_stub.fail_call_function = s;
}

extern "C" napi_module* StubNapiGetRegisteredModule(void) {
  return g_napi_stub.registered_module;
}
