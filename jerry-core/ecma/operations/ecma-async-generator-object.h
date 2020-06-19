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

#ifndef ECMA_ASYNC_GENERATOR_OBJECT_H
#define ECMA_ASYNC_GENERATOR_OBJECT_H

#include "ecma-globals.h"
#include "vm-defines.h"

#if ENABLED (JERRY_ESNEXT)

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaasyncgeneratorobject ECMA AsyncGenerator object related routines
 * @{
 */

/**
 * AsyncGenerator command types.
 */
typedef enum
{
  ECMA_ASYNC_GENERATOR_DO_NEXT, /**< async generator next operation */
  ECMA_ASYNC_GENERATOR_DO_THROW, /**< async generator throw operation */
  ECMA_ASYNC_GENERATOR_DO_RETURN, /**< async generator return operation */
} ecma_async_generator_operation_type_t;

ecma_value_t ecma_async_generator_enqueue (vm_executable_object_t *async_generator_object_p,
                                           ecma_async_generator_operation_type_t operation, ecma_value_t value);

void ecma_async_generator_run (vm_executable_object_t *async_generator_object_p);
void ecma_async_generator_finalize (vm_executable_object_t *async_generator_object_p, ecma_value_t value);

#endif /* ENABLED (JERRY_ESNEXT) */

/**
 * @}
 * @}
 */

#endif /* !ECMA_ASYNC_GENERATOR_OBJECT_H */
