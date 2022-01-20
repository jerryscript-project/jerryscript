/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef JERRYSCRIPT_CORE_H
#define JERRYSCRIPT_CORE_H

#include "jerryscript-types.h"

JERRY_C_API_BEGIN

/**
 * @defgroup jerry-api JerryScript public API
 * @{
 */

/**
 * @defgroup jerry-api-general General functions
 * @{
 */

/**
 * @defgroup jerry-api-general-conext Context management
 * @{
 */
void jerry_init (jerry_init_flag_t flags);
void jerry_cleanup (void);

void *jerry_context_data (const jerry_context_data_manager_t *manager_p);

jerry_value_t jerry_current_realm (void);
jerry_value_t jerry_set_realm (jerry_value_t realm);
/**
 * jerry-api-general-conext @}
 */

/**
 * @defgroup jerry-api-general-heap Heap management
 * @{
 */
void *jerry_heap_alloc (jerry_size_t size);
void jerry_heap_free (void *mem_p, jerry_size_t size);

bool jerry_heap_stats (jerry_heap_stats_t *out_stats_p);
void jerry_heap_gc (jerry_gc_mode_t mode);

bool jerry_foreach_live_object (jerry_foreach_live_object_cb_t callback, void *user_data);
bool jerry_foreach_live_object_with_info (const jerry_object_native_info_t *native_info_p,
                                          jerry_foreach_live_object_with_info_cb_t callback,
                                          void *user_data_p);
/**
 * jerry-api-general-heap @}
 */

/**
 * @defgroup jerry-api-general-misc Miscellaneous
 * @{
 */

void JERRY_ATTR_FORMAT (printf, 2, 3) jerry_log (jerry_log_level_t level, const char *format_p, ...);
void jerry_log_set_level (jerry_log_level_t level);
bool jerry_validate_string (const jerry_char_t *buffer_p, jerry_size_t buffer_size, jerry_encoding_t encoding);
bool JERRY_ATTR_CONST jerry_feature_enabled (const jerry_feature_t feature);
void jerry_register_magic_strings (const jerry_char_t *const *ext_strings_p,
                                   uint32_t count,
                                   const jerry_length_t *str_lengths_p);
/**
 * jerry-api-general-misc @}
 */

/**
 * jerry-api-general @}
 */

/**
 * @defgroup jerry-api-code Scripts and Executables
 * @{
 */

/**
 * @defgroup jerry-api-code-parse Parsing
 * @{
 */
jerry_value_t jerry_parse (const jerry_char_t *source_p, size_t source_size, const jerry_parse_options_t *options_p);
jerry_value_t jerry_parse_value (const jerry_value_t source, const jerry_parse_options_t *options_p);
/**
 * jerry-api-code-parse @}
 */

/**
 * @defgroup jerry-api-code-exec Execution
 * @{
 */
jerry_value_t jerry_eval (const jerry_char_t *source_p, size_t source_size, uint32_t flags);
jerry_value_t jerry_run (const jerry_value_t script);
jerry_value_t jerry_run_jobs (void);
/**
 * jerry-api-code-exec @}
 */

/**
 * @defgroup jerry-api-code-sourceinfo Source information
 * @{
 */
jerry_value_t jerry_source_name (const jerry_value_t value);
jerry_value_t jerry_source_user_value (const jerry_value_t value);
jerry_source_info_t *jerry_source_info (const jerry_value_t value);
void jerry_source_info_free (jerry_source_info_t *source_info_p);
/**
 * jerry-api-code-sourceinfo @}
 */

/**
 * @defgroup jerry-api-code-cb Callbacks
 * @{
 */
void jerry_halt_handler (uint32_t interval, jerry_halt_cb_t callback, void *user_p);
/**
 * jerry-api-code-cb @}
 */

/**
 * jerry-api-code @}
 */

/**
 * @defgroup jerry-api-backtrace Backtraces
 * @{
 */

/**
 * @defgroup jerry-api-backtrace-capture Capturing
 * @{
 */
jerry_value_t jerry_backtrace (uint32_t max_depth);
void jerry_backtrace_capture (jerry_backtrace_cb_t callback, void *user_p);
/**
 * jerry-api-backtrace-capture @}
 */

/**
 * @defgroup jerry-api-backtrace-frame Frames
 * @{
 */
jerry_frame_type_t jerry_frame_type (const jerry_frame_t *frame_p);
const jerry_value_t *jerry_frame_callee (jerry_frame_t *frame_p);
const jerry_value_t *jerry_frame_this (jerry_frame_t *frame_p);
const jerry_frame_location_t *jerry_frame_location (jerry_frame_t *frame_p);
bool jerry_frame_is_strict (jerry_frame_t *frame_p);
/**
 * jerry-api-backtrace-frame @}
 */

/**
 * jerry-api-backtrace @}
 */

/**
 * @defgroup jerry-api-value Values
 * @{
 */

/* Reference management */
jerry_value_t JERRY_ATTR_WARN_UNUSED_RESULT jerry_value_copy (const jerry_value_t value);
void jerry_value_free (jerry_value_t value);

/**
 * @defgroup jerry-api-value-checks Type inspection
 * @{
 */
jerry_type_t jerry_value_type (const jerry_value_t value);
bool jerry_value_is_exception (const jerry_value_t value);
bool jerry_value_is_abort (const jerry_value_t value);

bool jerry_value_is_undefined (const jerry_value_t value);
bool jerry_value_is_null (const jerry_value_t value);
bool jerry_value_is_boolean (const jerry_value_t value);
bool jerry_value_is_true (const jerry_value_t value);
bool jerry_value_is_false (const jerry_value_t value);

bool jerry_value_is_number (const jerry_value_t value);
bool jerry_value_is_bigint (const jerry_value_t value);

bool jerry_value_is_string (const jerry_value_t value);
bool jerry_value_is_symbol (const jerry_value_t value);

bool jerry_value_is_object (const jerry_value_t value);
bool jerry_value_is_array (const jerry_value_t value);
bool jerry_value_is_promise (const jerry_value_t value);
bool jerry_value_is_proxy (const jerry_value_t value);
bool jerry_value_is_arraybuffer (const jerry_value_t value);
bool jerry_value_is_shared_arraybuffer (const jerry_value_t value);
bool jerry_value_is_dataview (const jerry_value_t value);
bool jerry_value_is_typedarray (const jerry_value_t value);

bool jerry_value_is_constructor (const jerry_value_t value);
bool jerry_value_is_function (const jerry_value_t value);
bool jerry_value_is_async_function (const jerry_value_t value);

bool jerry_value_is_error (const jerry_value_t value);
/**
 * jerry-api-value-checks @}
 */

/**
 * @defgroup jerry-api-value-coerce Coercion
 * @{
 */
bool jerry_value_to_boolean (const jerry_value_t value);
jerry_value_t jerry_value_to_number (const jerry_value_t value);
jerry_value_t jerry_value_to_object (const jerry_value_t value);
jerry_value_t jerry_value_to_primitive (const jerry_value_t value);
jerry_value_t jerry_value_to_string (const jerry_value_t value);
jerry_value_t jerry_value_to_bigint (const jerry_value_t value);

double jerry_value_as_number (const jerry_value_t value);
double jerry_value_as_integer (const jerry_value_t value);
int32_t jerry_value_as_int32 (const jerry_value_t value);
uint32_t jerry_value_as_uint32 (const jerry_value_t value);
/**
 * jerry-api-value-coerce @}
 */

/**
 * @defgroup jerry-api-value-op Operations
 * @{
 */
jerry_value_t jerry_binary_op (jerry_binary_op_t operation, const jerry_value_t lhs, const jerry_value_t rhs);

/**
 * jerry-api-value-op @}
 */

/**
 * jerry-api-value @}
 */

/**
 * @defgroup jerry-api-exception Exceptions
 * @{
 */

/**
 * @defgroup jerry-api-exception-ctor Constructors
 * @{
 */
jerry_value_t jerry_throw (jerry_error_t type, const jerry_value_t message);
jerry_value_t jerry_throw_sz (jerry_error_t type, const char *message_p);
jerry_value_t jerry_throw_value (jerry_value_t value, bool take_ownership);
jerry_value_t jerry_throw_abort (jerry_value_t value, bool take_ownership);
/**
 * jerry-api-exception-ctor @}
 */

/**
 * @defgroup jerry-api-exception-op Operations
 * @{
 */
void jerry_exception_allow_capture (jerry_value_t value, bool allow_capture);
/**
 * jerry-api-exception-op @}
 */

/**
 * @defgroup jerry-api-exception-get Getters
 * @{
 */
jerry_value_t jerry_exception_value (jerry_value_t value, bool free_exception);
bool jerry_exception_is_captured (const jerry_value_t value);
/**
 * jerry-api-exception-get @}
 */

/**
 * @defgroup jerry-api-exception-cb Callbacks
 * @{
 */
void jerry_on_throw (jerry_throw_cb_t callback, void *user_p);
/**
 * jerry-api-exception-cb @}
 */

/**
 * jerry-api-error @}
 */

/**
 * @defgroup jerry-api-primitives Primitive types
 * @{
 */

/**
 * @defgroup jerry-api-undefined Undefined
 * @{
 */

/**
 * @defgroup jerry-api-undefined-ctor Constructors
 * @{
 */

jerry_value_t JERRY_ATTR_CONST jerry_undefined (void);

/**
 * jerry-api-undefined-ctor @}
 */

/**
 * jerry-api-undefined @}
 */

/**
 * @defgroup jerry-api-null Null
 * @{
 */

/**
 * @defgroup jerry-api-null-ctor Constructors
 * @{
 */

jerry_value_t JERRY_ATTR_CONST jerry_null (void);

/**
 * jerry-api-null-ctor @}
 */

/**
 * jerry-api-null @}
 */

/**
 * @defgroup jerry-api-boolean Boolean
 * @{
 */

/**
 * @defgroup jerry-api-boolean-ctor Constructors
 * @{
 */

jerry_value_t JERRY_ATTR_CONST jerry_boolean (bool value);

/**
 * jerry-api-boolean-ctor @}
 */

/**
 * jerry-api-boolean @}
 */

/**
 * @defgroup jerry-api-number Number
 * @{
 */

/**
 * @defgroup jerry-api-number-ctor Number
 * @{
 */

jerry_value_t jerry_number (double value);
jerry_value_t jerry_infinity (bool sign);
jerry_value_t jerry_nan (void);

/**
 * jerry-api-number-ctor @}
 */

/**
 * jerry-api-number @}
 */

/**
 * @defgroup jerry-api-bigint BigInt
 * @{
 */

/**
 * @defgroup jerry-api-bigint-ctor Constructors
 * @{
 */
jerry_value_t jerry_bigint (const uint64_t *digits_p, uint32_t digit_count, bool sign);
/**
 * jerry-api-bigint-ctor @}
 */

/**
 * @defgroup jerry-api-bigint-get Getters
 * @{
 */
uint32_t jerry_bigint_digit_count (const jerry_value_t value);
/**
 * jerry-api-bigint-get @}
 */

/**
 * @defgroup jerry-api-bigint-op Operations
 * @{
 */
void jerry_bigint_to_digits (const jerry_value_t value, uint64_t *digits_p, uint32_t digit_count, bool *sign_p);
/**
 * jerry-api-bigint-get @}
 */

/**
 * jerry-api-bigint @}
 */

/**
 * @defgroup jerry-api-string String
 * @{
 */

/**
 * @defgroup jerry-api-string-ctor Constructors
 * @{
 */
jerry_value_t jerry_string (const jerry_char_t *buffer_p, jerry_size_t buffer_size, jerry_encoding_t encoding);
jerry_value_t jerry_string_sz (const char *str_p);
jerry_value_t jerry_string_external (const jerry_char_t *buffer_p, jerry_size_t buffer_size, void *user_p);
jerry_value_t jerry_string_external_sz (const char *str_p, void *user_p);
/**
 * jerry-api-string-cotr @}
 */

/**
 * @defgroup jerry-api-string-get Getters
 * @{
 */
jerry_size_t jerry_string_size (const jerry_value_t value, jerry_encoding_t encoding);
jerry_length_t jerry_string_length (const jerry_value_t value);
void *jerry_string_user_ptr (const jerry_value_t value, bool *is_external);
/**
 * jerry-api-string-get @}
 */

/**
 * @defgroup jerry-api-string-op Operations
 * @{
 */
jerry_size_t jerry_string_substr (const jerry_value_t value, jerry_length_t start, jerry_length_t end);
jerry_size_t jerry_string_to_buffer (const jerry_value_t value,
                                     jerry_encoding_t encoding,
                                     jerry_char_t *buffer_p,
                                     jerry_size_t buffer_size);
void jerry_string_iterate (const jerry_value_t value,
                           jerry_encoding_t encoding,
                           jerry_string_iterate_cb_t callback,
                           void *user_p);
/**
 * jerry-api-string-op @}
 */

/**
 * @defgroup jerry-api-string-cb Callbacks
 * @{
 */
void jerry_string_external_on_free (jerry_external_string_free_cb_t callback);
/**
 * jerry-api-string-cb @}
 */

/**
 * jerry-api-string @}
 */

/**
 * @defgroup jerry-api-symbol Symbol
 * @{
 */

/**
 * @defgroup jerry-api-symbol-ctor Constructors
 * @{
 */
jerry_value_t jerry_symbol (jerry_well_known_symbol_t symbol);
jerry_value_t jerry_symbol_with_description (const jerry_value_t value);
/**
 * jerry-api-symbol-ctor @}
 */

/**
 * @defgroup jerry-api-symbol-get Getters
 * @{
 */
jerry_value_t jerry_symbol_description (const jerry_value_t symbol);
jerry_value_t jerry_symbol_descriptive_string (const jerry_value_t symbol);
/**
 * jerry-api-symbol-get @}
 */

/**
 * jerry-api-symbol @}
 */

/**
 * jerry-api-primitives @}
 */

/**
 * @defgroup jerry-api-objects Objects
 * @{
 */

/**
 * @defgroup jerry-api-object-ctor Constructors
 * @{
 */
jerry_value_t jerry_object (void);
/**
 * jerry-api-object-ctor @}
 */

/**
 * @defgroup jerry-api-object-get Getters
 * @{
 */

jerry_object_type_t jerry_object_type (const jerry_value_t object);
jerry_value_t jerry_object_proto (const jerry_value_t object);
jerry_value_t jerry_object_keys (const jerry_value_t object);
jerry_value_t jerry_object_property_names (const jerry_value_t object, jerry_property_filter_t filter);

/**
 * jerry-api-object-get @}
 */

/**
 * @defgroup jerry-api-object-op Operations
 * @{
 */

jerry_value_t jerry_object_set_proto (jerry_value_t object, const jerry_value_t proto);
bool jerry_object_foreach (const jerry_value_t object, jerry_object_property_foreach_cb_t foreach_p, void *user_data_p);

/**
 * @defgroup jerry-api-object-op-set Set
 * @{
 */
jerry_value_t jerry_object_set (jerry_value_t object, const jerry_value_t key, const jerry_value_t value);
jerry_value_t jerry_object_set_sz (jerry_value_t object, const char *key_p, const jerry_value_t value);
jerry_value_t jerry_object_set_index (jerry_value_t object, uint32_t index, const jerry_value_t value);
jerry_value_t jerry_object_define_own_prop (jerry_value_t object,
                                            const jerry_value_t key,
                                            const jerry_property_descriptor_t *prop_desc_p);
bool jerry_object_set_internal (jerry_value_t object, const jerry_value_t key, const jerry_value_t value);
void jerry_object_set_native_ptr (jerry_value_t object,
                                  const jerry_object_native_info_t *native_info_p,
                                  void *native_pointer_p);
/**
 * jerry-api-object-op-set @}
 */

/**
 * @defgroup jerry-api-object-op-has Has
 * @{
 */
jerry_value_t jerry_object_has (const jerry_value_t object, const jerry_value_t key);
jerry_value_t jerry_object_has_sz (const jerry_value_t object, const char *key_p);
jerry_value_t jerry_object_has_own (const jerry_value_t object, const jerry_value_t key);
bool jerry_object_has_internal (const jerry_value_t object, const jerry_value_t key);
bool jerry_object_has_native_ptr (const jerry_value_t object, const jerry_object_native_info_t *native_info_p);
/**
 * jerry-api-object-op-has @}
 */

/**
 * @defgroup jerry-api-object-op-get Get
 * @{
 */
jerry_value_t jerry_object_get (const jerry_value_t object, const jerry_value_t key);
jerry_value_t jerry_object_get_sz (const jerry_value_t object, const char *key_p);
jerry_value_t jerry_object_get_index (const jerry_value_t object, uint32_t index);
jerry_value_t jerry_object_get_own_prop (const jerry_value_t object,
                                         const jerry_value_t key,
                                         jerry_property_descriptor_t *prop_desc_p);
jerry_value_t jerry_object_get_internal (const jerry_value_t object, const jerry_value_t key);
void *jerry_object_get_native_ptr (const jerry_value_t object, const jerry_object_native_info_t *native_info_p);

jerry_value_t jerry_object_find_own (const jerry_value_t object,
                                     const jerry_value_t key,
                                     const jerry_value_t receiver,
                                     bool *found_p);
/**
 * jerry-api-object-op-get @}
 */

/**
 * @defgroup jerry-api-object-op-del Delete
 * @{
 */
jerry_value_t jerry_object_delete (jerry_value_t object, const jerry_value_t key);
jerry_value_t jerry_object_delete_sz (const jerry_value_t object, const char *key_p);
jerry_value_t jerry_object_delete_index (jerry_value_t object, uint32_t index);
bool jerry_object_delete_internal (jerry_value_t object, const jerry_value_t key);
bool jerry_object_delete_native_ptr (jerry_value_t object, const jerry_object_native_info_t *native_info_p);
/**
 * jerry-api-object-op-del @}
 */

/**
 * jerry-api-object-op @}
 */

/**
 * @defgroup jerry-api-object-prop-desc Property descriptors
 * @{
 */

/**
 * @defgroup jerry-api-object-prop-desc-ctor Constructors
 * @{
 */
jerry_property_descriptor_t jerry_property_descriptor (void);
jerry_value_t jerry_property_descriptor_from_object (const jerry_value_t obj_value,
                                                     jerry_property_descriptor_t *out_prop_desc_p);
/**
 * jerry-api-object-prop-desc-ctor @}
 */

/**
 * @defgroup jerry-api-object-prop-desc-op Operations
 * @{
 */
void jerry_property_descriptor_free (jerry_property_descriptor_t *prop_desc_p);
jerry_value_t jerry_property_descriptor_to_object (const jerry_property_descriptor_t *src_prop_desc_p);
/**
 * jerry-api-object-prop-desc-op @}
 */

/**
 * jerry-api-object-prop-desc @}
 */

/**
 * @defgroup jerry-api-object-native-ptr Native pointers
 * @{
 */

/**
 * @defgroup jerry-api-object-native-ptr-op Operations
 * @{
 */
void jerry_native_ptr_init (void *native_pointer_p, const jerry_object_native_info_t *native_info_p);
void jerry_native_ptr_free (void *native_pointer_p, const jerry_object_native_info_t *native_info_p);
void jerry_native_ptr_set (jerry_value_t *reference_p, const jerry_value_t value);
/**
 * jerry-api-object-native-ptr-op @}
 */

/**
 * jerry-api-object-native-ptr @}
 */

/**
 * @defgroup jerry-api-array Array
 * @{
 */

/**
 * @defgroup jerry-api-array-ctor Constructors
 * @{
 */
jerry_value_t jerry_array (jerry_length_t length);
/**
 * jerry-api-array-ctor @}
 */

/**
 * @defgroup jerry-api-array-get Getters
 * @{
 */
jerry_length_t jerry_array_length (const jerry_value_t value);
/**
 * jerry-api-array-get @}
 */

/**
 * jerry-api-array @}
 */

/**
 * @defgroup jerry-api-arraybuffer ArrayBuffer
 * @{
 */

/**
 * @defgroup jerry-api-arraybuffer-ctor Constructors
 * @{
 */
jerry_value_t jerry_arraybuffer (const jerry_length_t size);
jerry_value_t jerry_arraybuffer_external (uint8_t *buffer_p, jerry_length_t size, void *user_p);
/**
 * jerry-api-arraybuffer-ctor @}
 */

/**
 * @defgroup jerry-api-arraybuffer-get Getters
 * @{
 */
jerry_size_t jerry_arraybuffer_size (const jerry_value_t value);
uint8_t *jerry_arraybuffer_data (const jerry_value_t value);
bool jerry_arraybuffer_is_detachable (const jerry_value_t value);
bool jerry_arraybuffer_has_buffer (const jerry_value_t value);
/**
 * jerry-api-arraybuffer-get @}
 */

/**
 * @defgroup jerry-api-arraybuffer-op Operations
 * @{
 */
jerry_size_t
jerry_arraybuffer_read (const jerry_value_t value, jerry_size_t offset, uint8_t *buffer_p, jerry_size_t buffer_size);
jerry_size_t
jerry_arraybuffer_write (jerry_value_t value, jerry_size_t offset, const uint8_t *buffer_p, jerry_size_t buffer_size);
jerry_value_t jerry_arraybuffer_detach (jerry_value_t value);
void jerry_arraybuffer_heap_allocation_limit (jerry_size_t limit);
/**
 * jerry-api-arraybuffer-op @}
 */

/**
 * @defgroup jerry-api-arraybuffer-cb Callbacks
 * @{
 */
void jerry_arraybuffer_allocator (jerry_arraybuffer_allocate_cb_t allocate_callback,
                                  jerry_arraybuffer_free_cb_t free_callback,
                                  void *user_p);
/**
 * jerry-api-arraybuffer-cb @}
 */

/**
 * jerry-api-arraybuffer @}
 */

/**
 * @defgroup jerry-api-sharedarraybuffer SharedArrayBuffer
 * @{
 */

/**
 * @defgroup jerry-api-sharedarraybuffer-ctor Constructors
 * @{
 */
jerry_value_t jerry_shared_arraybuffer (jerry_size_t size);
jerry_value_t jerry_shared_arraybuffer_external (uint8_t *buffer_p, jerry_size_t buffer_size, void *user_p);
/**
 * jerry-api-sharedarraybuffer-ctor @}
 */

/**
 * jerry-api-sharedarraybuffer @}
 */

/**
 * @defgroup jerry-api-dataview DataView
 * @{
 */

/**
 * @defgroup jerry-api-dataview-ctor Constructors
 * @{
 */
jerry_value_t jerry_dataview (const jerry_value_t value, jerry_size_t byte_offset, jerry_size_t byte_length);
/**
 * jerry-api-dataview-ctr @}
 */

/**
 * @defgroup jerry-api-dataview-get Getters
 * @{
 */
jerry_value_t
jerry_dataview_buffer (const jerry_value_t dataview, jerry_size_t *byte_offset, jerry_size_t *byte_length);
/**
 * jerry-api-dataview-get @}
 */

/**
 * jerry-api-dataview @}
 */

/**
 * @defgroup jerry-api-typedarray TypedArray
 * @{
 */

/**
 * @defgroup jerry-api-typedarray-ctor Constructors
 * @{
 */
jerry_value_t jerry_typedarray (jerry_typedarray_type_t type, jerry_length_t length);
jerry_value_t jerry_typedarray_with_buffer (jerry_typedarray_type_t type, const jerry_value_t arraybuffer);
jerry_value_t jerry_typedarray_with_buffer_span (jerry_typedarray_type_t type,
                                                 const jerry_value_t arraybuffer,
                                                 jerry_size_t byte_offset,
                                                 jerry_size_t byte_length);
/**
 * jerry-api-typedarray-ctor @}
 */

/**
 * @defgroup jerry-api-typedarray-get Getters
 * @{
 */
jerry_typedarray_type_t jerry_typedarray_type (const jerry_value_t value);
jerry_length_t jerry_typedarray_length (const jerry_value_t value);
jerry_value_t jerry_typedarray_buffer (const jerry_value_t value, jerry_size_t *byte_offset, jerry_size_t *byte_length);
/**
 * jerry-api-typedarray-get @}
 */

/**
 * jerry-api-typedarray @}
 */

/**
 * @defgroup jerry-api-iterator Iterator
 * @{
 */

/**
 * @defgroup jerry-api-iterator-get Getters
 * @{
 */
jerry_iterator_type_t jerry_iterator_type (const jerry_value_t value);
/**
 * jerry-api-iterator-get @}
 */

/**
 * jerry-api-iterator @}
 */

/**
 * @defgroup jerry-api-function Function
 * @{
 */

/**
 * @defgroup jerry-api-function-ctor Constructors
 * @{
 */
jerry_value_t jerry_function_external (jerry_external_handler_t handler);
/**
 * jerry-api-function-ctor @}
 */

/**
 * @defgroup jerry-api-function-get Getters
 * @{
 */
jerry_function_type_t jerry_function_type (const jerry_value_t value);
bool jerry_function_is_dynamic (const jerry_value_t value);
/**
 * jerry-api-function-get @}
 */

/**
 * @defgroup jerry-api-function-op Operations
 * @{
 */
jerry_value_t jerry_call (const jerry_value_t function,
                          const jerry_value_t this_value,
                          const jerry_value_t *args_p,
                          jerry_size_t args_count);
jerry_value_t jerry_construct (const jerry_value_t function, const jerry_value_t *args_p, jerry_size_t args_count);
/**
 * jerry-api-function-op @}
 */

/**
 * jerry-api-function @}
 */

/**
 * @defgroup jerry-api-proxy Proxy
 * @{
 */

/**
 * @defgroup jerry-api-proxy-ctor Constructors
 * @{
 */
jerry_value_t jerry_proxy (const jerry_value_t target, const jerry_value_t handler);
jerry_value_t jerry_proxy_custom (const jerry_value_t target, const jerry_value_t handler, uint32_t flags);
/**
 * jerry-api-function-proxy-ctor @}
 */

/**
 * @defgroup jerry-api-proxy-get Getters
 * @{
 */
jerry_value_t jerry_proxy_target (const jerry_value_t value);
jerry_value_t jerry_proxy_handler (const jerry_value_t value);
/**
 * jerry-api-function-proxy-get @}
 */

/**
 * jerry-api-proxy @}
 */

/**
 * @defgroup jerry-api-promise Promise
 * @{
 */

/**
 * @defgroup jerry-api-promise-ctor Constructors
 * @{
 */
jerry_value_t jerry_promise (void);
/**
 * jerry-api-promise-ctor @}
 */

/**
 * @defgroup jerry-api-promise-get Getters
 * @{
 */
jerry_value_t jerry_promise_result (const jerry_value_t promise);
jerry_promise_state_t jerry_promise_state (const jerry_value_t promise);
/**
 * jerry-api-promise-get @}
 */

/**
 * @defgroup jerry-api-promise-op Operations
 * @{
 */
jerry_value_t jerry_promise_resolve (jerry_value_t promise, const jerry_value_t argument);
jerry_value_t jerry_promise_reject (jerry_value_t promise, const jerry_value_t argument);
/**
 * jerry-api-promise-op @}
 */

/**
 * @defgroup jerry-api-promise-cb Callbacks
 * @{
 */
void jerry_promise_on_event (jerry_promise_event_filter_t filters, jerry_promise_event_cb_t callback, void *user_p);
/**
 * jerry-api-promise-cb @}
 */

/**
 * jerry-api-promise @}
 */

/**
 * @defgroup jerry-api-container Map, Set, WeakMap, WeakSet
 * @{
 */

/**
 * @defgroup jerry-api-container-ctor Constructors
 * @{
 */
jerry_value_t jerry_container (jerry_container_type_t container_type,
                               const jerry_value_t *arguments_p,
                               jerry_length_t argument_count);
/**
 * jerry-api-promise-ctor @}
 */

/**
 * @defgroup jerry-api-container-get Getters
 * @{
 */
jerry_container_type_t jerry_container_type (const jerry_value_t value);
/**
 * jerry-api-container-get @}
 */

/**
 * @defgroup jerry-api-container-op Operations
 * @{
 */
jerry_value_t jerry_container_to_array (const jerry_value_t value, bool *is_key_value_p);
jerry_value_t jerry_container_op (jerry_container_op_t operation,
                                  jerry_value_t container,
                                  const jerry_value_t *arguments,
                                  uint32_t argument_count);
/**
 * jerry-api-container-op @}
 */

/**
 * jerry-api-container @}
 */

/**
 * @defgroup jerry-api-regexp RegExp
 * @{
 */

/**
 * @defgroup jerry-api-regexp-ctor Constructors
 * @{
 */
jerry_value_t jerry_regexp (const jerry_value_t pattern, uint16_t flags);
jerry_value_t jerry_regexp_sz (const char *pattern_p, uint16_t flags);
/**
 * jerry-api-regexp-ctor @}
 */

/**
 * jerry-api-regexp @}
 */

/**
 * @defgroup jerry-api-error Error
 * @{
 */

/**
 * @defgroup jerry-api-error-ctor Constructors
 * @{
 */
jerry_value_t jerry_error (jerry_error_t type, const jerry_value_t message);
jerry_value_t jerry_error_sz (jerry_error_t type, const char *message_p);
/**
 * jerry-api-error-ctor @}
 */

/**
 * @defgroup jerry-api-error-get Getters
 * @{
 */
jerry_error_t jerry_error_type (jerry_value_t value);
/**
 * jerry-api-error-get @}
 */

/**
 * @defgroup jerry-api-error-cb Callbacks
 * @{
 */
void jerry_error_on_created (jerry_error_object_created_cb_t callback, void *user_p);
/**
 * jerry-api-error-cb @}
 */

/**
 * jerry-api-error @}
 */

/**
 * jerry-api-objects @}
 */

/**
 * @defgroup jerry-api-json JSON
 * @{
 */

/**
 * @defgroup jerry-api-json-op Operations
 * @{
 */
jerry_value_t jerry_json_parse (const jerry_char_t *string_p, jerry_size_t string_size);
jerry_value_t jerry_json_stringify (const jerry_value_t object);
/**
 * jerry-api-json-op @}
 */

/**
 * jerry-api-json @}
 */

/**
 * @defgroup jerry-api-module Modules
 * @{
 */

/**
 * @defgroup jerry-api-module-get Getters
 * @{
 */
jerry_module_state_t jerry_module_state (const jerry_value_t module);
size_t jerry_module_request_count (const jerry_value_t module);
jerry_value_t jerry_module_request (const jerry_value_t module, size_t request_index);
jerry_value_t jerry_module_namespace (const jerry_value_t module);
/**
 * jerry-api-module-get @}
 */

/**
 * @defgroup jerry-api-module-op Operations
 * @{
 */

/**
 * Resolve and parse a module file
 *
 * @param specifier: module request specifier string.
 * @param referrer: parent module.
 * @param user_p: user specified pointer.
 *
 * @return module object if resolving is successful, error otherwise.
 */
jerry_value_t jerry_module_resolve (const jerry_value_t specifier, const jerry_value_t referrer, void *user_p);

jerry_value_t jerry_module_link (const jerry_value_t module, jerry_module_resolve_cb_t callback, void *user_p);
jerry_value_t jerry_module_evaluate (const jerry_value_t module);

/**
 * Release known modules in the current context. If realm parameter is supplied, cleans up modules native to that realm
 * only. This function should be called by the user application when the module database in the current context is no
 * longer needed.
 *
 * @param realm: release only those modules which realm value is equal to this argument.
 */
void jerry_module_cleanup (const jerry_value_t realm);

/**
 * jerry-api-module-op @}
 */

/**
 * @defgroup jerry-api-module-native Native modules
 * @{
 */
jerry_value_t jerry_native_module (jerry_native_module_evaluate_cb_t callback,
                                   const jerry_value_t *const exports_p,
                                   size_t export_count);
jerry_value_t jerry_native_module_get (const jerry_value_t native_module, const jerry_value_t export_name);
jerry_value_t
jerry_native_module_set (jerry_value_t native_module, const jerry_value_t export_name, const jerry_value_t value);
/**
 * jerry-api-module-native @}
 */

/**
 * @defgroup jerry-api-module-cb Callbacks
 * @{
 */
void jerry_module_on_state_changed (jerry_module_state_changed_cb_t callback, void *user_p);
void jerry_module_on_import_meta (jerry_module_import_meta_cb_t callback, void *user_p);
void jerry_module_on_import (jerry_module_import_cb_t callback, void *user_p);
/**
 * jerry-api-module-cb @}
 */

/**
 * jerry-api-module @}
 */

/**
 * @defgroup jerry-api-realm Realms
 * @{
 */

/**
 * @defgroup jerry-api-realm-ctor Constructors
 * @{
 */
jerry_value_t jerry_realm (void);
/**
 * jerry-api-realm-ctor @}
 */

/**
 * @defgroup jerry-api-realm-get Getters
 * @{
 */
jerry_value_t jerry_realm_this (jerry_value_t realm);
/**
 * jerry-api-realm-ctor @}
 */

/**
 * @defgroup jerry-api-realm-op Operation
 * @{
 */
jerry_value_t jerry_realm_set_this (jerry_value_t realm, jerry_value_t this_value);
/**
 * jerry-api-realm-op @}
 */

/**
 * jerry-api-realm @}
 */

/**
 * jerry-api @}
 */

JERRY_C_API_END

#endif /* !JERRYSCRIPT_CORE_H */

/* vim: set fdm=marker fmr=@{,@}: */
