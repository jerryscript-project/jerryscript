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
  JERRY_EXTENSION_FIELD_TYPE_FLOAT, /**< float */
  JERRY_EXTENSION_FIELD_TYPE_STRING /**< chars buffer */
} jerry_extension_data_type_t;

/**
 * Description of an extension object's fields
 */
typedef struct
{
  jerry_extension_data_type_t type; /**< field data type */

  /**
   * Value description
   */
  union
  {
    bool v_boolean; /**< boolean */
    float v_float; /**< number */
    const char* v_string; /**< string */
  };
} jerry_extension_field_t;

typedef struct
{
  jerry_extension_data_type_t type; /**< argument data type */

  union
  {
    bool v_bool; /**< boolean */

    float v_float; /**< number converted to float */

    /** String copied to external characters buffer (not zero-terminated) */
    struct
    {
      char* chars_p; /**< pointer to the string's chars in characters buffer */
      size_t length; /**< number of characters */
    } v_string;
  };
} jerry_extension_function_arg_t;

/**
 * Description of an extension object's function
 */
typedef struct
{
  const char* function_name_p; /**< name of function */

  jerry_extension_function_arg_t *args_p; /**< arrays of the function's arguments */
  uint32_t args_number; /**< number of arguments */
} jerry_extension_function_t;

/**
 * Description of an extention object
 */
typedef struct
{
  uint32_t fields_count; /**< number of fields */
  uint32_t functions_count; /**< number of functions */

  const jerry_extension_field_t *fields_p; /**< array of field descriptor */
  const jerry_extension_function_t *functions_p; /**< array of function descriptors */
} jerry_extension_descriptor_t;

extern void
jerry_extend_with (const char *builtin_object_name,
                   const jerry_extension_descriptor_t *desc_p);

/**
 * @}
 */

#endif /* !JERRY_EXTENSION_H */
