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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#ifdef __GNUC__
#define JERRY_DEPRECATED_API __attribute__((deprecated))
#else /* !__GNUC__ */
/* TODO: for other compilers */
#define JERRY_DEPRECATED_API
#endif /* __GNUC__ */

/** \addtogroup jerry Jerry engine interface
 * @{
 */

/**
 * Major version of JerryScript API.
 */
#define JERRY_API_MAJOR_VERSION 1

/**
 * Minor version of JerryScript API.
 */
#define JERRY_API_MINOR_VERSION 0

/**
 * JerryScript init flags.
 */
typedef enum
{
  JERRY_INIT_EMPTY               = (0u),      /**< empty flag set */
  JERRY_INIT_SHOW_OPCODES        = (1u << 0), /**< dump byte-code to log after parse */
  JERRY_INIT_SHOW_REGEXP_OPCODES = (1u << 1), /**< dump regexp byte-code to log after compilation */
  JERRY_INIT_MEM_STATS           = (1u << 2), /**< dump memory statistics */
  JERRY_INIT_MEM_STATS_SEPARATE  = (1u << 3), /**< deprecated, an unused placeholder now */
  JERRY_INIT_DEBUGGER            = (1u << 4), /**< deprecated, an unused placeholder now */
} jerry_init_flag_t;

/**
 * JerryScript API Error object types.
 */
typedef enum
{
  JERRY_ERROR_COMMON,    /**< Error */
  JERRY_ERROR_EVAL,      /**< EvalError */
  JERRY_ERROR_RANGE,     /**< RangeError */
  JERRY_ERROR_REFERENCE, /**< ReferenceError */
  JERRY_ERROR_SYNTAX,    /**< SyntaxError */
  JERRY_ERROR_TYPE,      /**< TypeError */
  JERRY_ERROR_URI        /**< URIError */
} jerry_error_t;

/**
 * JerryScript feature types.
 */
typedef enum
{
  JERRY_FEATURE_CPOINTER_32_BIT, /**< 32 bit compressed pointers */
  JERRY_FEATURE_ERROR_MESSAGES, /**< error messages */
  JERRY_FEATURE_JS_PARSER, /**< js-parser */
  JERRY_FEATURE_MEM_STATS, /**< memory statistics */
  JERRY_FEATURE_PARSER_DUMP, /**< parser byte-code dumps */
  JERRY_FEATURE_REGEXP_DUMP, /**< regexp byte-code dumps */
  JERRY_FEATURE_SNAPSHOT_SAVE, /**< saving snapshot files */
  JERRY_FEATURE_SNAPSHOT_EXEC, /**< executing snapshot files */
  JERRY_FEATURE_DEBUGGER, /**< debugging */
  JERRY_FEATURE_VM_EXEC_STOP, /**< stopping ECMAScript execution */
  JERRY_FEATURE__COUNT /**< number of features. NOTE: must be at the end of the list */
} jerry_feature_t;

/**
 * Character type of JerryScript.
 */
typedef uint8_t jerry_char_t;

/**
 * Pointer to an array of character values.
 */
typedef jerry_char_t *jerry_char_ptr_t;

/**
 * Size type of JerryScript.
 */
typedef uint32_t jerry_size_t;

/**
 * Length type of JerryScript.
 */
typedef uint32_t jerry_length_t;

/**
 * Description of a JerryScript value.
 */
typedef uint32_t jerry_value_t;

/**
 * Description of ECMA property descriptor.
 */
typedef struct
{
  /** Is [[Value]] defined? */
  bool is_value_defined;

  /** Is [[Get]] defined? */
  bool is_get_defined;

  /** Is [[Set]] defined? */
  bool is_set_defined;

  /** Is [[Writable]] defined? */
  bool is_writable_defined;

  /** [[Writable]] */
  bool is_writable;

  /** Is [[Enumerable]] defined? */
  bool is_enumerable_defined;

  /** [[Enumerable]] */
  bool is_enumerable;

  /** Is [[Configurable]] defined? */
  bool is_configurable_defined;

  /** [[Configurable]] */
  bool is_configurable;

  /** [[Value]] */
  jerry_value_t value;

  /** [[Get]] */
  jerry_value_t getter;

  /** [[Set]] */
  jerry_value_t setter;
} jerry_property_descriptor_t;

/**
 * Description of JerryScript heap memory stats.
 * It is for memory profiling.
 */
typedef struct
{
  size_t version; /**< the version of the stats struct */
  size_t size; /**< heap total size */
  size_t allocated_bytes; /**< currently allocated bytes */
  size_t peak_allocated_bytes; /**< peak allocated bytes */
  size_t reserved[4]; /**< padding for future extensions */
} jerry_heap_stats_t;

/**
 * Type of an external function handler.
 */
typedef jerry_value_t (*jerry_external_handler_t) (const jerry_value_t function_obj,
                                                   const jerry_value_t this_val,
                                                   const jerry_value_t args_p[],
                                                   const jerry_length_t args_count);

/**
 * Native free callback of an object (deprecated).
 */
typedef void (*jerry_object_free_callback_t) (const uintptr_t native_p);

/**
 * Native free callback of an object.
 */
typedef void (*jerry_object_native_free_callback_t) (void *native_p);

/**
 * Callback which tells whether the ECMAScript execution should be stopped.
 *
 * As long as the function returns with undefined the execution continues.
 * When a non-undefined value is returned the execution stops and the value
 * is thrown by the engine (if the error flag is not set for the returned
 * value the engine automatically sets it).
 *
 * Note: if the function returns with a non-undefined value it
 *       must return with the same value for future calls.
 */
typedef jerry_value_t (*jerry_vm_exec_stop_callback_t) (void *user_p);

/**
 * Function type applied for each data property of an object.
 */
typedef bool (*jerry_object_property_foreach_t) (const jerry_value_t property_name,
                                                 const jerry_value_t property_value,
                                                 void *user_data_p);

/**
 * User context item manager
 */
typedef struct
{
  void (*init_cb) (void *); /**< callback responsible for initializing a context item, or NULL to zero out the memory */
  void (*deinit_cb) (void *); /**< callback responsible for deinitializing a context item */
  size_t bytes_needed; /**< number of bytes to allocate for this manager */
} jerry_context_data_manager_t;

/**
 * Function type for allocating buffer for JerryScript instance.
 */
typedef void *(*jerry_instance_alloc_t) (size_t size, void *cb_data_p);

/**
 * Type information of a native pointer.
 */
typedef struct
{
  jerry_object_native_free_callback_t free_cb; /**< the free callback of the native pointer */
} jerry_object_native_info_t;

/**
 * A forward declaration of the JerryScript instance structure.
 */
typedef struct jerry_instance_t jerry_instance_t;

/**
 * General engine functions.
 */
void jerry_init (jerry_init_flag_t flags);
void jerry_cleanup (void);
void jerry_register_magic_strings (const jerry_char_ptr_t *ex_str_items_p, uint32_t count,
                                   const jerry_length_t *str_lengths_p);
void jerry_get_memory_limits (size_t *out_data_bss_brk_limit_p, size_t *out_stack_limit_p);
void jerry_gc (void);
void *jerry_get_context_data (const jerry_context_data_manager_t *manager_p);

bool jerry_get_memory_stats (jerry_heap_stats_t *out_stats_p);

/**
 * Parser and executor functions.
 */
bool jerry_run_simple (const jerry_char_t *script_source_p, size_t script_source_size, jerry_init_flag_t flags);
jerry_value_t jerry_parse (const jerry_char_t *source_p, size_t source_size, bool is_strict);
jerry_value_t jerry_parse_named_resource (const jerry_char_t *resource_name_p, size_t resource_name_length,
                                          const jerry_char_t *source_p, size_t source_size, bool is_strict);
jerry_value_t jerry_parse_function (const jerry_char_t *resource_name_p, size_t resource_name_length,
                                    const jerry_char_t *arg_list_p, size_t arg_list_size,
                                    const jerry_char_t *source_p, size_t source_size, bool is_strict);
jerry_value_t jerry_run (const jerry_value_t func_val);
jerry_value_t jerry_eval (const jerry_char_t *source_p, size_t source_size, bool is_strict);

jerry_value_t jerry_run_all_enqueued_jobs (void);

/**
 * Get the global context.
 */
jerry_value_t jerry_get_global_object (void);

/**
 * Checker functions of 'jerry_value_t'.
 */
bool jerry_value_is_array (const jerry_value_t value);
bool jerry_value_is_boolean (const jerry_value_t value);
bool jerry_value_is_constructor (const jerry_value_t value);
bool jerry_value_is_function (const jerry_value_t value);
bool jerry_value_is_number (const jerry_value_t value);
bool jerry_value_is_null (const jerry_value_t value);
bool jerry_value_is_object (const jerry_value_t value);
bool jerry_value_is_promise (const jerry_value_t value);
bool jerry_value_is_string (const jerry_value_t value);
bool jerry_value_is_undefined (const jerry_value_t value);

/**
 * Checker function of whether the specified compile feature is enabled.
 */
bool jerry_is_feature_enabled (const jerry_feature_t feature);

/**
 * Error flag manipulation functions.
 */
bool jerry_value_has_error_flag (const jerry_value_t value);
void jerry_value_clear_error_flag (jerry_value_t *value_p);
void jerry_value_set_error_flag (jerry_value_t *value_p);
jerry_value_t jerry_get_value_without_error_flag (jerry_value_t value);

/**
 * Getter functions of 'jerry_value_t'.
 */
bool jerry_get_boolean_value (const jerry_value_t value);
double jerry_get_number_value (const jerry_value_t value);

/**
 * Functions for string values.
 */
jerry_size_t jerry_get_string_size (const jerry_value_t value);
jerry_size_t jerry_get_utf8_string_size (const jerry_value_t value);
jerry_length_t jerry_get_string_length (const jerry_value_t value);
jerry_length_t jerry_get_utf8_string_length (const jerry_value_t value);
jerry_size_t jerry_string_to_char_buffer (const jerry_value_t value, jerry_char_t *buffer_p, jerry_size_t buffer_size);
jerry_size_t jerry_string_to_utf8_char_buffer (const jerry_value_t value,
                                               jerry_char_t *buffer_p,
                                               jerry_size_t buffer_size);
jerry_size_t jerry_substring_to_char_buffer (const jerry_value_t value,
                                             jerry_length_t start_pos,
                                             jerry_length_t end_pos,
                                             jerry_char_t *buffer_p,
                                             jerry_size_t buffer_size);
jerry_size_t jerry_substring_to_utf8_char_buffer (const jerry_value_t value,
                                                  jerry_length_t start_pos,
                                                  jerry_length_t end_pos,
                                                  jerry_char_t *buffer_p,
                                                  jerry_size_t buffer_size);

/**
 * Functions for array object values.
 */
uint32_t jerry_get_array_length (const jerry_value_t value);

/**
 * Converters of 'jerry_value_t'.
 */
bool jerry_value_to_boolean (const jerry_value_t value);
jerry_value_t jerry_value_to_number (const jerry_value_t value);
jerry_value_t jerry_value_to_object (const jerry_value_t value);
jerry_value_t jerry_value_to_primitive (const jerry_value_t value);
jerry_value_t jerry_value_to_string (const jerry_value_t value);

/**
 * Acquire types with reference counter (increase the references).
 */
jerry_value_t jerry_acquire_value (jerry_value_t value);

/**
 * Release the referenced values.
 */
void jerry_release_value (jerry_value_t value);

/**
 * Create functions of API values.
 */
jerry_value_t jerry_create_array (uint32_t size);
jerry_value_t jerry_create_boolean (bool value);
jerry_value_t jerry_create_error (jerry_error_t error_type, const jerry_char_t *message_p);
jerry_value_t jerry_create_error_sz (jerry_error_t error_type, const jerry_char_t *message_p,
                                     jerry_size_t message_size);
jerry_value_t jerry_create_external_function (jerry_external_handler_t handler_p);
jerry_value_t jerry_create_number (double value);
jerry_value_t jerry_create_number_infinity (bool sign);
jerry_value_t jerry_create_number_nan (void);
jerry_value_t jerry_create_null (void);
jerry_value_t jerry_create_object (void);
jerry_value_t jerry_create_promise (void);
jerry_value_t jerry_create_string_from_utf8 (const jerry_char_t *str_p);
jerry_value_t jerry_create_string_sz_from_utf8 (const jerry_char_t *str_p, jerry_size_t str_size);
jerry_value_t jerry_create_string (const jerry_char_t *str_p);
jerry_value_t jerry_create_string_sz (const jerry_char_t *str_p, jerry_size_t str_size);
jerry_value_t jerry_create_undefined (void);

/**
 * General API functions of JS objects.
 */
jerry_value_t jerry_has_property (const jerry_value_t obj_val, const jerry_value_t prop_name_val);
jerry_value_t jerry_has_own_property (const jerry_value_t obj_val, const jerry_value_t prop_name_val);
bool jerry_delete_property (const jerry_value_t obj_val, const jerry_value_t prop_name_val);
bool jerry_delete_property_by_index (const jerry_value_t obj_val, uint32_t index);

jerry_value_t jerry_get_property (const jerry_value_t obj_val, const jerry_value_t prop_name_val);
jerry_value_t jerry_get_property_by_index (const jerry_value_t  obj_val, uint32_t index);
jerry_value_t jerry_set_property (const jerry_value_t obj_val, const jerry_value_t prop_name_val,
                                  const jerry_value_t value_to_set);
jerry_value_t jerry_set_property_by_index (const jerry_value_t obj_val, uint32_t index,
                                           const jerry_value_t value_to_set);

void jerry_init_property_descriptor_fields (jerry_property_descriptor_t *prop_desc_p);
jerry_value_t jerry_define_own_property (const jerry_value_t obj_val,
                                         const jerry_value_t prop_name_val,
                                         const jerry_property_descriptor_t *prop_desc_p);

bool jerry_get_own_property_descriptor (const jerry_value_t obj_val,
                                        const jerry_value_t prop_name_val,
                                        jerry_property_descriptor_t *prop_desc_p);
void jerry_free_property_descriptor_fields (const jerry_property_descriptor_t *prop_desc_p);

jerry_value_t jerry_call_function (const jerry_value_t func_obj_val, const jerry_value_t this_val,
                                   const jerry_value_t args_p[], jerry_size_t args_count);
jerry_value_t jerry_construct_object (const jerry_value_t func_obj_val, const jerry_value_t args_p[],
                                      jerry_size_t args_count);

jerry_value_t jerry_get_object_keys (const jerry_value_t obj_val);
jerry_value_t jerry_get_prototype (const jerry_value_t obj_val);
jerry_value_t jerry_set_prototype (const jerry_value_t obj_val, const jerry_value_t proto_obj_val);

JERRY_DEPRECATED_API
bool jerry_get_object_native_handle (const jerry_value_t obj_val, uintptr_t *out_handle_p);
JERRY_DEPRECATED_API
void jerry_set_object_native_handle (const jerry_value_t obj_val, uintptr_t handle_p,
                                     jerry_object_free_callback_t freecb_p);

bool jerry_get_object_native_pointer (const jerry_value_t obj_val,
                                      void **out_native_pointer_p,
                                      const jerry_object_native_info_t **out_pointer_info_p);
void jerry_set_object_native_pointer (const jerry_value_t obj_val,
                                      void *native_pointer_p,
                                      const jerry_object_native_info_t *native_info_p);

bool jerry_foreach_object_property (const jerry_value_t obj_val, jerry_object_property_foreach_t foreach_p,
                                    void *user_data_p);

/**
 * Promise resolve/reject functions.
 */
jerry_value_t jerry_resolve_or_reject_promise (jerry_value_t promise, jerry_value_t argument, bool is_resolve);

/**
 * Input validator functions.
 */
bool jerry_is_valid_utf8_string (const jerry_char_t *utf8_buf_p, jerry_size_t buf_size);
bool jerry_is_valid_cesu8_string (const jerry_char_t *cesu8_buf_p, jerry_size_t buf_size);

/*
 * External context functions.
 */
jerry_instance_t *jerry_create_instance (uint32_t heap_size, jerry_instance_alloc_t alloc, void *cb_data_p);

/**
 * Miscellaneous functions.
 */
void jerry_set_vm_exec_stop_callback (jerry_vm_exec_stop_callback_t stop_cb, void *user_p, uint32_t frequency);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* !JERRYSCRIPT_CORE_H */
