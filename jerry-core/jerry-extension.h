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

#ifndef JERRY_EXTENSION_H
#define JERRY_EXTENSION_H

#include <stddef.h>
#include <stdint.h>

/** \addtogroup jerry Jerry engine extension interface
 * @{
 */

/**
 * Jerry's extension-related data types
 */
typedef enum
{
  JERRY_EXTENSION_FIELD_TYPE_BOOLEAN, /**< bool */
  JERRY_EXTENSION_FIELD_TYPE_FLOAT32, /**< 32-bit float */
  JERRY_EXTENSION_FIELD_TYPE_FLOAT64, /**< 64-bit float */
  JERRY_EXTENSION_FIELD_TYPE_UINT32, /**< number converted to 32-bit unsigned integer*/
  JERRY_EXTENSION_FIELD_TYPE_STRING /**< chars buffer */
} jerry_extension_data_type_t;

/**
 * Description of an extension object's fields
 */
typedef struct
{
  const char *field_name_p; /**< field name */

  jerry_extension_data_type_t type; /**< field data type */

  /**
   * Value description
   */
  union
  {
    const char* v_string; /**< string */
    bool v_bool; /**< boolean */
    float v_float32; /**< 32-bit float */
    double v_float64; /**< 64-bit float */
    uint32_t v_uint32; /**< 32-bit unsigned integer */
  };
} jerry_extension_field_t;

/**
 * Description of an extension function's argument
 */
typedef struct
{
  jerry_extension_data_type_t type; /**< argument data type */

  union
  {
    bool v_bool; /**< boolean */

    float v_float32; /**< 32-bit float */
    double v_float64; /**< 64-bit float */

    uint32_t v_uint32; /**< number converted 32-bit unsigned integer */

    /** String copied to external characters buffer (not zero-terminated) */
    struct
    {
      char* chars_p; /**< pointer to the string's chars in characters buffer */
      size_t length; /**< number of characters */
    } v_string;
  };
} jerry_extension_function_arg_t;

/**
 * Pointer to extension function implementation
 */
typedef void (*jerry_extension_function_pointer_t) (const struct jerry_extension_function_t *function_block_p);

/**
 * Description of an extension object's function
 */
typedef struct jerry_extension_function_t
{
  const char* function_name_p; /**< name of function */

  jerry_extension_function_pointer_t function_wrapper_p; /**< pointer to function implementation */

  jerry_extension_function_arg_t *args_p; /**< arrays of the function's arguments */
  uint32_t args_number; /**< number of arguments */
} jerry_extension_function_t;

/**
 * Description of an extention object
 */
typedef struct jerry_extension_descriptor_t
{
  const uint32_t fields_count; /**< number of fields */
  const uint32_t functions_count; /**< number of functions */

  const jerry_extension_field_t* const fields_p; /**< array of field descriptor */
  const jerry_extension_function_t* const functions_p; /**< array of function descriptors */

  const char* const name_p; /**< name of the extension */
  struct jerry_extension_descriptor_t *next_p; /**< next descriptor in list of registered extensions */
  uint32_t index; /**< global index of the extension among registered exceptions */
} jerry_extension_descriptor_t;

extern bool
jerry_extend_with (jerry_extension_descriptor_t *desc_p);

/**
 * @}
 */

#endif /* !JERRY_EXTENSION_H */
