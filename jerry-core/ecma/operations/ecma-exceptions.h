/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

#ifndef ECMA_EXCEPTIONS_H
#define ECMA_EXCEPTIONS_H

#include "ecma-globals.h"
#include "jrt.h"

/** \addtogroup ecma ECMA
 * @{
 */

/**
 * \addtogroup exceptions Exceptions
 * @{
 */

/**
 * Native errors.
 *
 * See also: 15.11.1, 15.11.6
 */
typedef enum
{
  ECMA_ERROR_COMMON, /**< Error */
  ECMA_ERROR_EVAL, /**< EvalError */
  ECMA_ERROR_RANGE, /**< RangeError */
  ECMA_ERROR_REFERENCE, /**< ReferenceError */
  ECMA_ERROR_SYNTAX, /**< SyntaxError */
  ECMA_ERROR_TYPE, /**< TypeError */
  ECMA_ERROR_URI /**< URIError */
} ecma_standard_error_t;

extern ecma_object_t *ecma_new_standard_error (ecma_standard_error_t error_type);
extern ecma_object_t* ecma_new_standard_error_with_message (ecma_standard_error_t error_type,
                                                            ecma_string_t *message_string_p);

#define ERROR_OBJ(ret_value, error_type, msg) \
  do \
  { \
  ecma_string_t *error_msg_p = ecma_new_ecma_string ((const ecma_char_t *) msg); \
  ecma_object_t *error_obj_p = ecma_new_standard_error_with_message (error_type, error_msg_p); \
  ecma_deref_ecma_string (error_msg_p); \
  ret_value = ecma_make_throw_obj_completion_value (error_obj_p); \
  } while (0)

#define COMMON_ERROR_OBJ(ret_value, msg)  ERROR_OBJ (ret_value, ECMA_ERROR_COMMON, msg)
#define RANGE_ERROR_OBJ(ret_value, msg)  ERROR_OBJ (ret_value, ECMA_ERROR_RANGE, msg)
#define REFERENCE_ERROR_OBJ(ret_value, msg)  ERROR_OBJ (ret_value, ECMA_ERROR_REFERENCE, msg)
#define SYNTAX_ERROR_OBJ(ret_value, msg)  ERROR_OBJ (ret_value, ECMA_ERROR_SYNTAX, msg)
#define TYPE_ERROR_OBJ(ret_value, msg) ERROR_OBJ (ret_value, ECMA_ERROR_TYPE, msg)

/**
 * @}
 * @}
 */

#endif /* !ECMA_EXCEPTIONS_H */
