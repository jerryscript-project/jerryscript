/* Copyright 2015 Samsung Electronics Co., Ltd.
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
# define EXTERN_C "C"
#else /* !__cplusplus */
# define EXTERN_C
#endif /* !__cplusplus */

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
  JERRY_API_DATA_TYPE_VOID, /**< no return value */
  JERRY_API_DATA_TYPE_UNDEFINED, /**< undefined */
  JERRY_API_DATA_TYPE_NULL, /**< null */
  JERRY_API_DATA_TYPE_BOOLEAN, /**< bool */
  JERRY_API_DATA_TYPE_FLOAT32, /**< 32-bit float */
  JERRY_API_DATA_TYPE_FLOAT64, /**< 64-bit float */
  JERRY_API_DATA_TYPE_UINT32, /**< number converted to 32-bit unsigned integer */
  JERRY_API_DATA_TYPE_STRING, /**< string */
  JERRY_API_DATA_TYPE_OBJECT  /**< object */
} jerry_api_data_type_t;

/**
 * Jerry API Error object types
 */
typedef enum
{
  JERRY_API_ERROR_COMMON, /**< Error */
  JERRY_API_ERROR_EVAL, /**< EvalError */
  JERRY_API_ERROR_RANGE, /**< RangeError */
  JERRY_API_ERROR_REFERENCE, /**< ReferenceError */
  JERRY_API_ERROR_SYNTAX, /**< SyntaxError */
  JERRY_API_ERROR_TYPE, /**< TypeError */
  JERRY_API_ERROR_URI /**< URIError */
} jerry_api_error_t;

/**
 * Jerry's char value
*/
typedef uint8_t jerry_api_char_t;

/**
 * Pointer to an array of character values
 */
typedef jerry_api_char_t *jerry_api_char_ptr_t;

/**
 * Jerry's size
*/
typedef uint32_t jerry_api_size_t;

/**
 * Jerry's length
*/
typedef uint32_t jerry_api_length_t;

/**
 * Jerry's string value
 */
typedef struct ecma_string_t jerry_api_string_t;

/**
 * Jerry's object value
 */
typedef struct ecma_object_t jerry_api_object_t;

/**
 * Description of an extension function's argument
 */
typedef struct jerry_api_value_t
{
  jerry_api_data_type_t type; /**< argument data type */

  union
  {
    bool v_bool; /**< boolean */

    float v_float32; /**< 32-bit float */
    double v_float64; /**< 64-bit float */

    uint32_t v_uint32; /**< number converted 32-bit unsigned integer */

    union
    {
      jerry_api_string_t *v_string; /**< pointer to a JS string */
      jerry_api_object_t *v_object; /**< pointer to a JS object */
    };
  };
} jerry_api_value_t;

/**
 * Jerry external function handler type
 */
typedef bool (*jerry_external_handler_t) (const jerry_api_object_t *function_obj_p,
                                          const jerry_api_value_t *this_p,
                                          jerry_api_value_t *ret_val_p,
                                          const jerry_api_value_t args_p[],
                                          const jerry_api_length_t args_count);

/**
 * An object's native free callback
 */
typedef void (*jerry_object_free_callback_t) (const uintptr_t native_p);

/**
 * function type applied for each fields in objects
 */
typedef bool (*jerry_object_field_foreach_t) (const jerry_api_string_t *field_name_p,
                                              const jerry_api_value_t *field_value_p,
                                              void *user_data_p);

extern EXTERN_C ssize_t
jerry_api_string_to_char_buffer (const jerry_api_string_t *, jerry_api_char_t *, ssize_t);
extern EXTERN_C
jerry_api_string_t *jerry_api_acquire_string (jerry_api_string_t *);
extern EXTERN_C
void jerry_api_release_string (jerry_api_string_t *);

extern EXTERN_C
jerry_api_object_t *jerry_api_acquire_object (jerry_api_object_t *);
extern EXTERN_C
void jerry_api_release_object (jerry_api_object_t *);

extern EXTERN_C
void jerry_api_release_value (jerry_api_value_t *);

extern EXTERN_C
jerry_api_string_t *jerry_api_create_string (const jerry_api_char_t *);
extern EXTERN_C
jerry_api_string_t *jerry_api_create_string_sz (const jerry_api_char_t *, jerry_api_size_t);
extern EXTERN_C
jerry_api_object_t *jerry_api_create_object (void);

extern EXTERN_C
jerry_api_object_t *jerry_api_create_array_object (jerry_api_size_t);
extern EXTERN_C
bool jerry_api_set_array_index_value (jerry_api_object_t *, jerry_api_length_t, jerry_api_value_t *);
extern EXTERN_C
bool jerry_api_get_array_index_value (jerry_api_object_t *, jerry_api_length_t, jerry_api_value_t *);

extern EXTERN_C
jerry_api_object_t *jerry_api_create_error (jerry_api_error_t, const jerry_api_char_t *);
extern EXTERN_C
jerry_api_object_t *jerry_api_create_error_sz (jerry_api_error_t, const jerry_api_char_t *, jerry_api_size_t);
extern EXTERN_C
jerry_api_object_t *jerry_api_create_external_function (jerry_external_handler_t);

extern EXTERN_C
bool jerry_api_is_function (const jerry_api_object_t *);
extern EXTERN_C
bool jerry_api_is_constructor (const jerry_api_object_t *);

extern EXTERN_C
bool jerry_api_add_object_field (jerry_api_object_t *, const jerry_api_char_t *, jerry_api_size_t,
                                 const jerry_api_value_t *, bool);
extern EXTERN_C
bool jerry_api_delete_object_field (jerry_api_object_t *, const jerry_api_char_t *, jerry_api_size_t);
extern EXTERN_C
bool jerry_api_get_object_field_value (jerry_api_object_t *, const jerry_api_char_t *, jerry_api_value_t *);

extern EXTERN_C
bool jerry_api_get_object_field_value_sz (jerry_api_object_t *, const jerry_api_char_t *, jerry_api_size_t,
                                          jerry_api_value_t *);

extern EXTERN_C
bool jerry_api_set_object_field_value (jerry_api_object_t *, const jerry_api_char_t *, const jerry_api_value_t *);

extern EXTERN_C
bool jerry_api_set_object_field_value_sz (jerry_api_object_t *, const jerry_api_char_t *, jerry_api_size_t,
                                          const jerry_api_value_t *);

extern EXTERN_C
bool jerry_api_foreach_object_field (jerry_api_object_t *, jerry_object_field_foreach_t, void *);

extern EXTERN_C
bool jerry_api_get_object_native_handle (jerry_api_object_t *, uintptr_t *);

extern EXTERN_C
void jerry_api_set_object_native_handle (jerry_api_object_t *, uintptr_t, jerry_object_free_callback_t);

extern EXTERN_C
bool jerry_api_call_function (jerry_api_object_t *, jerry_api_object_t *, jerry_api_value_t *,
                              const jerry_api_value_t[], uint16_t);

extern EXTERN_C
bool jerry_api_construct_object (jerry_api_object_t *, jerry_api_value_t *, const jerry_api_value_t[], uint16_t);

extern EXTERN_C
jerry_completion_code_t jerry_api_eval (const jerry_api_char_t *, size_t, bool, bool, jerry_api_value_t *);

extern EXTERN_C
jerry_api_object_t *jerry_api_get_global (void);

extern EXTERN_C
void jerry_api_gc (void);

extern EXTERN_C
void jerry_register_external_magic_strings (const jerry_api_char_ptr_t *, uint32_t, const jerry_api_length_t *);

extern EXTERN_C
size_t jerry_parse_and_save_snapshot (const jerry_api_char_t *, size_t, bool, uint8_t *, size_t);

extern EXTERN_C
jerry_completion_code_t jerry_exec_snapshot (const void *, size_t, bool, jerry_api_value_t *);

/**
 * @}
 */

#endif /* !JERRY_API_H */
