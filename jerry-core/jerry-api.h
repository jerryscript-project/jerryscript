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
 * Jerry API data types
 */
typedef enum
{
  JERRY_API_DATA_TYPE_BOOLEAN, /**< bool */
  JERRY_API_DATA_TYPE_FLOAT32, /**< 32-bit float */
  JERRY_API_DATA_TYPE_FLOAT64, /**< 64-bit float */
  JERRY_API_DATA_TYPE_UINT32, /**< number converted to 32-bit unsigned integer*/
  JERRY_API_DATA_TYPE_STRING, /**< string */
  JERRY_API_DATA_TYPE_OBJECT  /**< object */
} jerry_api_data_type_t;

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
typedef struct
{
  const jerry_api_data_type_t type; /**< argument data type */

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

extern EXTERN_C ssize_t
jerry_api_string_to_char_buffer (const jerry_api_string_t *string_p,
                                 char *buffer_p,
                                 ssize_t buffer_size);
extern EXTERN_C
jerry_api_string_t* jerry_api_acquire_string (jerry_api_string_t *string_p);
extern EXTERN_C
void jerry_api_release_string (jerry_api_string_t *string_p);

extern EXTERN_C
jerry_api_object_t* jerry_api_acquire_object (jerry_api_object_t *object_p);
extern EXTERN_C
void jerry_api_release_object (jerry_api_object_t *object_p);

extern EXTERN_C
bool jerry_api_call_function (jerry_api_object_t *function_object_p,
                              jerry_api_value_t *retval_p,
                              const jerry_api_value_t args_p [],
                              uint32_t args_count);

extern EXTERN_C
jerry_api_object_t* jerry_api_create_object (void);

extern EXTERN_C
bool jerry_api_add_object_field (jerry_api_object_t *object_p,
                                 const char *field_name_p,
                                 const jerry_api_value_t *field_value_p,
                                 bool is_writable);
extern EXTERN_C
bool jerry_api_delete_object_field (jerry_api_object_t *object_p,
                                    const char *field_name_p);
extern EXTERN_C
bool jerry_api_get_object_field_value (jerry_api_object_t *object_p,
                                       const char *field_name_p,
                                       jerry_api_value_t *field_value_p);
extern EXTERN_C
bool jerry_api_set_object_field_value (jerry_api_object_t *object_p,
                                       const char *field_name_p,
                                       const jerry_api_value_t *field_value_p);

/**
 * @}
 */

#endif /* !JERRY_API_H */
