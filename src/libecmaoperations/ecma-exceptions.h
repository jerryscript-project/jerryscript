/* Copyright 2014 Samsung Electronics Co., Ltd.
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
#include "globals.h"

/** \addtogroup ecma ---TODO---
 * @{
 */

/**
 * \addtogroup exceptions Exceptions
 * @{
 */

/**
 * Native errors.
 *
 * See also: 15.11.6
 */
typedef enum
{
  ECMA_ERROR_EVAL, /**< EvalError */
  ECMA_ERROR_RANGE, /**< RangeError */
  ECMA_ERROR_REFERENCE, /**< ReferenceError */
  ECMA_ERROR_SYNTAX, /**< SyntaxError */
  ECMA_ERROR_TYPE, /**< TypeError */
  ECMA_ERROR_URI /**< URIError */
} ecma_standard_error_t;

extern ecma_object_t *ecma_new_standard_error( ecma_standard_error_t error_type);

/**
 * @}
 * @}
 */

#endif /* !ECMA_EXCEPTIONS_H */
