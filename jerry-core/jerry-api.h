/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged.
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

#ifndef JERRY_API_H
#define JERRY_API_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/** \addtogroup jerry Jerry engine interface
 * @{
 */

/**
 * Major version of JerryScript API
 */
#define JERRY_API_MAJOR_VERSION 1

/**
 * Minor version of JerryScript API
 */
#define JERRY_API_MINOR_VERSION 0

/**
 * Jerry init flags
 */
typedef enum
{
  JERRY_INIT_EMPTY               = (0u),      /**< empty flag set */
  JERRY_INIT_SHOW_OPCODES        = (1u << 0), /**< dump byte-code to log after parse */
  JERRY_INIT_SHOW_REGEXP_OPCODES = (1u << 1), /**< dump regexp byte-code to log after compilation */
  JERRY_INIT_MEM_STATS           = (1u << 2), /**< dump memory statistics */
  JERRY_INIT_MEM_STATS_SEPARATE  = (1u << 3), /**< dump memory statistics and reset peak values after parse */
} jerry_init_flag_t;

/**
 * Jerry API Error object types
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
 * Jerry's char value
 */
typedef uint8_t jerry_char_t;

/**
 * Pointer to an array of character values
 */
typedef jerry_char_t *jerry_char_ptr_t;

/**
 * Jerry's size
 */
typedef uint32_t jerry_size_t;

/**
 * Jerry's length
 */
typedef uint32_t jerry_length_t;

/**
 * Description of a JerryScript value
 */
typedef uint32_t jerry_value_t;


/**
 * Description of ECMA property descriptor
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
 * Type of an external function handler
 */
typedef jerry_value_t (*jerry_external_handler_t) (const jerry_value_t function_obj,
                                                   const jerry_value_t this_val,
                                                   const jerry_value_t args_p[],
                                                   const jerry_length_t args_count);

/**
 * Native free callback of an object
 */
typedef void (*jerry_object_free_callback_t) (const uintptr_t native_p);

/**
 * Function type applied for each data property of an object
 */
typedef bool (*jerry_object_property_foreach_t) (const jerry_value_t property_name,
                                                 const jerry_value_t property_value,
                                                 void *user_data_p);

/**
 * General engine functions
 */
void jerry_init (jerry_init_flag_t);
void jerry_cleanup (void);
void jerry_register_magic_strings (const jerry_char_ptr_t *, uint32_t, const jerry_length_t *);
void jerry_get_memory_limits (size_t *, size_t *);
void jerry_gc (void);

/**
 * Parser and executor functions
 */
bool jerry_run_simple (const jerry_char_t *, size_t, jerry_init_flag_t);
jerry_value_t jerry_parse (const jerry_char_t *, size_t, bool);
jerry_value_t jerry_run (const jerry_value_t);
jerry_value_t jerry_eval (const jerry_char_t *, size_t, bool);

/**
 * Get the global context
 */
jerry_value_t jerry_get_global_object (void);

/**
 * Checker functions of 'jerry_value_t'
 */
bool jerry_value_is_array (const jerry_value_t);
bool jerry_value_is_boolean (const jerry_value_t);
bool jerry_value_is_constructor (const jerry_value_t);
bool jerry_value_is_function (const jerry_value_t);
bool jerry_value_is_number (const jerry_value_t);
bool jerry_value_is_null (const jerry_value_t);
bool jerry_value_is_object (const jerry_value_t);
bool jerry_value_is_string (const jerry_value_t);
bool jerry_value_is_undefined (const jerry_value_t);

/**
 * Error flag manipulation functions
 */
bool jerry_value_has_error_flag (const jerry_value_t);
void jerry_value_clear_error_flag (jerry_value_t *);
void jerry_value_set_error_flag (jerry_value_t *);

/**
 * Getter functions of 'jerry_value_t'
 */
bool jerry_get_boolean_value (const jerry_value_t);
double jerry_get_number_value (const jerry_value_t);

/**
 * Functions for string values
 */
jerry_size_t jerry_get_string_size (const jerry_value_t);
jerry_length_t jerry_get_string_length (const jerry_value_t);
jerry_size_t jerry_string_to_char_buffer (const jerry_value_t, jerry_char_t *, jerry_size_t);

/**
 * Functions for array object values
 */
uint32_t jerry_get_array_length (const jerry_value_t);

/**
 * Converters of 'jerry_value_t'
 */
bool jerry_value_to_boolean (const jerry_value_t);
jerry_value_t jerry_value_to_number (const jerry_value_t);
jerry_value_t jerry_value_to_object (const jerry_value_t);
jerry_value_t jerry_value_to_primitive (const jerry_value_t);
jerry_value_t jerry_value_to_string (const jerry_value_t);

/**
 * Acquire types with reference counter (increase the references)
 */
jerry_value_t jerry_acquire_value (jerry_value_t);

/**
 * Release the referenced values
 */
void jerry_release_value (jerry_value_t);

/**
 * Create functions of API values
 */
jerry_value_t jerry_create_array (uint32_t);
jerry_value_t jerry_create_boolean (bool);
jerry_value_t jerry_create_error (jerry_error_t, const jerry_char_t *);
jerry_value_t jerry_create_error_sz (jerry_error_t, const jerry_char_t *, jerry_size_t);
jerry_value_t jerry_create_external_function (jerry_external_handler_t);
jerry_value_t jerry_create_number (double);
jerry_value_t jerry_create_number_infinity (bool);
jerry_value_t jerry_create_number_nan (void);
jerry_value_t jerry_create_null (void);
jerry_value_t jerry_create_object (void);
jerry_value_t jerry_create_string (const jerry_char_t *);
jerry_value_t jerry_create_string_sz (const jerry_char_t *, jerry_size_t);
jerry_value_t jerry_create_undefined (void);

/**
 * General API functions of JS objects
 */
bool jerry_has_property (const jerry_value_t, const jerry_value_t);
bool jerry_has_own_property (const jerry_value_t, const jerry_value_t);
bool jerry_delete_property (const jerry_value_t, const jerry_value_t);

jerry_value_t jerry_get_property (const jerry_value_t, const jerry_value_t);
jerry_value_t jerry_get_property_by_index (const jerry_value_t , uint32_t);
jerry_value_t jerry_set_property (const jerry_value_t, const jerry_value_t, const jerry_value_t);
jerry_value_t jerry_set_property_by_index (const jerry_value_t, uint32_t, const jerry_value_t);

void jerry_init_property_descriptor_fields (jerry_property_descriptor_t *);
jerry_value_t jerry_define_own_property (const jerry_value_t,
                                         const jerry_value_t,
                                         const jerry_property_descriptor_t *);

bool jerry_get_own_property_descriptor (const jerry_value_t,
                                        const jerry_value_t,
                                        jerry_property_descriptor_t *);
void jerry_free_property_descriptor_fields (const jerry_property_descriptor_t *);

jerry_value_t jerry_call_function (const jerry_value_t, const jerry_value_t, const jerry_value_t[], jerry_size_t);
jerry_value_t jerry_construct_object (const jerry_value_t, const jerry_value_t[], jerry_size_t);

jerry_value_t jerry_get_object_keys (const jerry_value_t);
jerry_value_t jerry_get_prototype (const jerry_value_t);
jerry_value_t jerry_set_prototype (const jerry_value_t, const jerry_value_t);

bool jerry_get_object_native_handle (const jerry_value_t, uintptr_t *);
void jerry_set_object_native_handle (const jerry_value_t, uintptr_t, jerry_object_free_callback_t);
bool jerry_foreach_object_property (const jerry_value_t, jerry_object_property_foreach_t, void *);

/**
 * Snapshot functions
 */
size_t jerry_parse_and_save_snapshot (const jerry_char_t *, size_t, bool, bool, uint8_t *, size_t);
jerry_value_t jerry_exec_snapshot (const void *, size_t, bool);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* !JERRY_API_H */
