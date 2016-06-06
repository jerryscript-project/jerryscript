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
#include <sys/types.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/** \addtogroup jerry Jerry engine interface
 * @{
 */

/**
 * Jerry completion codes
 */
typedef enum
{
  JERRY_COMPLETION_CODE_OK                       = 0, /**< successful completion */
  JERRY_COMPLETION_CODE_UNHANDLED_EXCEPTION      = 1, /**< exception occured and it was not handled */
  JERRY_COMPLETION_CODE_INVALID_SNAPSHOT_VERSION = 2, /**< snapshot version mismatch */
  JERRY_COMPLETION_CODE_INVALID_SNAPSHOT_FORMAT  = 3, /**< snapshot format is not valid */
} jerry_completion_code_t;

/**
 * Jerry API data types
 */
typedef enum
{
  JERRY_DATA_TYPE_VOID,      /**< no return value */
  JERRY_DATA_TYPE_UNDEFINED, /**< undefined */
  JERRY_DATA_TYPE_NULL,      /**< null */
  JERRY_DATA_TYPE_BOOLEAN,   /**< bool */
  JERRY_DATA_TYPE_FLOAT32,   /**< 32-bit float */
  JERRY_DATA_TYPE_FLOAT64,   /**< 64-bit float */
  JERRY_DATA_TYPE_UINT32,    /**< number converted to 32-bit unsigned integer */
  JERRY_DATA_TYPE_STRING,    /**< string */
  JERRY_DATA_TYPE_OBJECT     /**< object */
} jerry_data_type_t;

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
 * Jerry's string value
 */
typedef struct ecma_string_t jerry_string_t;

/**
 * Jerry's object value
 */
typedef struct ecma_object_t jerry_object_t;

/**
 * Description of an extension function's argument
 */
typedef struct jerry_value_t
{
  jerry_data_type_t type; /**< argument data type */

  union
  {
    bool v_bool; /**< boolean */

    float v_float32; /**< 32-bit float */
    double v_float64; /**< 64-bit float */

    uint32_t v_uint32; /**< number converted 32-bit unsigned integer */

    jerry_string_t *v_string; /**< pointer to a JS string */
    jerry_object_t *v_object; /**< pointer to a JS object */

  } u;
} jerry_value_t;

/**
 * Type of an external function handler
 */
typedef bool (*jerry_external_handler_t) (const jerry_object_t *function_obj_p,
                                          const jerry_value_t *this_p,
                                          jerry_value_t *ret_val_p,
                                          const jerry_value_t args_p[],
                                          const jerry_length_t args_count);

/**
 * Native free callback of an object
 */
typedef void (*jerry_object_free_callback_t) (const uintptr_t native_p);

/**
 * Function type applied for each fields in objects
 */
typedef bool (*jerry_object_field_foreach_t) (const jerry_string_t *field_name_p,
                                              const jerry_value_t *field_value_p,
                                              void *user_data_p);

/**
 * Get the global context
 */
jerry_object_t *jerry_get_global (void);

/**
 * Checker functions of 'jerry_value_t'
 */
bool jerry_value_is_void (const jerry_value_t *value_p);
bool jerry_value_is_null (const jerry_value_t *value_p);
bool jerry_value_is_undefined (const jerry_value_t *value_p);
bool jerry_value_is_boolean (const jerry_value_t *value_p);
bool jerry_value_is_number (const jerry_value_t *value_p);
bool jerry_value_is_string (const jerry_value_t *value_p);
bool jerry_value_is_object (const jerry_value_t *value_p);
bool jerry_value_is_function (const jerry_value_t *value_p);

/**
 * Getter functions of 'jerry_value_t'
 */
bool jerry_get_boolean_value (const jerry_value_t *value_p);
double jerry_get_number_value (const jerry_value_t *value_p);
jerry_string_t *jerry_get_string_value (const jerry_value_t *value_p);
jerry_object_t *jerry_get_object_value (const jerry_value_t *value_p);

/**
 * Converters of 'jerry_value_t'
 */
jerry_string_t *jerry_value_to_string (const jerry_value_t *);

/**
 * Create functions of 'jerry_value_t'
 */
jerry_value_t jerry_create_void_value (void);
jerry_value_t jerry_create_null_value (void);
jerry_value_t jerry_create_undefined_value (void);
jerry_value_t jerry_create_boolean_value (bool value);
jerry_value_t jerry_create_number_value (double value);
jerry_value_t jerry_create_object_value (jerry_object_t *value);
jerry_value_t jerry_create_string_value (jerry_string_t *value);

/**
 * Acquire types with reference counter (increase the references)
 */
jerry_string_t *jerry_acquire_string (jerry_string_t *);
jerry_object_t *jerry_acquire_object (jerry_object_t *);
jerry_value_t *jerry_acquire_value (jerry_value_t *);

/**
 * Relase the referenced values
 */
void jerry_release_object (jerry_object_t *);
void jerry_release_string (jerry_string_t *);
void jerry_release_value (jerry_value_t *);

/**
 * Create functions of API objects
 */
jerry_object_t *jerry_create_object (void);
jerry_object_t *jerry_create_array_object (jerry_size_t);
jerry_object_t *jerry_create_external_function (jerry_external_handler_t);
jerry_object_t *jerry_create_error (jerry_error_t, const jerry_char_t *);
jerry_object_t *jerry_create_error_sz (jerry_error_t, const jerry_char_t *, jerry_size_t);
jerry_string_t *jerry_create_string (const jerry_char_t *);
jerry_string_t *jerry_create_string_sz (const jerry_char_t *, jerry_size_t);

/**
 * Functions of array objects
 */
bool jerry_set_array_index_value (jerry_object_t *, jerry_length_t, jerry_value_t *);
bool jerry_get_array_index_value (jerry_object_t *, jerry_length_t, jerry_value_t *);

/**
 * Functions of 'jerry_string_t'
 */
jerry_size_t jerry_get_string_size (const jerry_string_t *);
jerry_length_t jerry_get_string_length (const jerry_string_t *);
jerry_size_t jerry_string_to_char_buffer (const jerry_string_t *, jerry_char_t *, jerry_size_t);

/**
 * General API functions of JS objects
 */
bool jerry_is_constructor (const jerry_object_t *);
bool jerry_is_function (const jerry_object_t *);
bool jerry_add_object_field (jerry_object_t *, const jerry_char_t *, jerry_size_t, const jerry_value_t *, bool);
bool jerry_delete_object_field (jerry_object_t *, const jerry_char_t *, jerry_size_t);
bool jerry_get_object_field_value (jerry_object_t *, const jerry_char_t *, jerry_value_t *);
bool jerry_get_object_field_value_sz (jerry_object_t *, const jerry_char_t *, jerry_size_t, jerry_value_t *);
bool jerry_set_object_field_value (jerry_object_t *, const jerry_char_t *, const jerry_value_t *);
bool jerry_set_object_field_value_sz (jerry_object_t *, const jerry_char_t *, jerry_size_t, const jerry_value_t *);
bool jerry_foreach_object_field (jerry_object_t *, jerry_object_field_foreach_t, void *);
bool jerry_get_object_native_handle (jerry_object_t *, uintptr_t *);
void jerry_set_object_native_handle (jerry_object_t *, uintptr_t, jerry_object_free_callback_t);
bool jerry_construct_object (jerry_object_t *, jerry_value_t *, const jerry_value_t[], uint16_t);
bool jerry_call_function (jerry_object_t *, jerry_object_t *, jerry_value_t *, const jerry_value_t[], uint16_t);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* !JERRY_API_H */
