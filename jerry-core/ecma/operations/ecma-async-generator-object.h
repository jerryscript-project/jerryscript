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

/**
 * AsyncGenerator yield iterator states.
 */
typedef enum
{
  ECMA_ASYNC_YIELD_ITERATOR_AWAIT_NEXT, /**< wait for an iterator result object */
  ECMA_ASYNC_YIELD_ITERATOR_AWAIT_NEXT_RETURN, /**< wait for an iterator result object after a return operation */
  ECMA_ASYNC_YIELD_ITERATOR_AWAIT_RETURN, /**< wait for the argument passed to return operation */
  ECMA_ASYNC_YIELD_ITERATOR_AWAIT_NEXT_VALUE, /**< wait for the value property of an iterator result object */
  ECMA_ASYNC_YIELD_ITERATOR_AWAIT_OPERATION, /**< wait for the generator operation (next/throw/return) */
  ECMA_ASYNC_YIELD_ITERATOR_AWAIT_CLOSE, /**< wait for the result of iterator close operation */
} ecma_async_yield_iterator_states_t;

/**
 * Get the state of an async yield iterator.
 */
#define ECMA_ASYNC_YIELD_ITERATOR_GET_STATE(async_generator_object_p) \
  ((async_generator_object_p)->extended_object.u.class_prop.extra_info >> ECMA_ASYNC_YIELD_ITERATOR_STATE_SHIFT)

/**
 * Set the state of an async yield iterator.
 */
#define ECMA_ASYNC_YIELD_ITERATOR_SET_STATE(async_generator_object_p, to) \
  do \
  { \
    uint16_t extra_info = (async_generator_object_p)->extended_object.u.class_prop.extra_info; \
    extra_info &= ((1 << ECMA_ASYNC_YIELD_ITERATOR_STATE_SHIFT) - 1); \
    extra_info |= (ECMA_ASYNC_YIELD_ITERATOR_AWAIT_ ## to) << ECMA_ASYNC_YIELD_ITERATOR_STATE_SHIFT; \
    (async_generator_object_p)->extended_object.u.class_prop.extra_info = extra_info; \
  } \
  while (false)

/**
 * Helper value for ECMA_ASYNC_YIELD_ITERATOR_END.
 */
#define ECMA_ASYNC_YIELD_ITERATOR_END_MASK \
  (((1 << ECMA_ASYNC_YIELD_ITERATOR_STATE_SHIFT) - 1) - ECMA_GENERATOR_ITERATE_AND_YIELD)

/**
 * Return from yield iterator.
 */
#define ECMA_ASYNC_YIELD_ITERATOR_END(async_generator_object_p) \
  ((async_generator_object_p)->extended_object.u.class_prop.extra_info &= ECMA_ASYNC_YIELD_ITERATOR_END_MASK)

/**
 * Helper macro for ECMA_ASYNC_YIELD_ITERATOR_CHANGE_STATE.
 */
#define ECMA_ASYNC_YIELD_ITERATOR_CS1(from, to) \
  ((ECMA_ASYNC_YIELD_ITERATOR_AWAIT_ ## from) ^ (ECMA_ASYNC_YIELD_ITERATOR_AWAIT_ ## to))

/**
 * Helper macro for ECMA_ASYNC_YIELD_ITERATOR_CHANGE_STATE.
 */
#define ECMA_ASYNC_YIELD_ITERATOR_CS2(from, to) \
  (ECMA_ASYNC_YIELD_ITERATOR_CS1(from, to) << ECMA_ASYNC_YIELD_ITERATOR_STATE_SHIFT)

/**
 * Change the state of an async yield iterator.
 */
#define ECMA_ASYNC_YIELD_ITERATOR_CHANGE_STATE(async_generator_object_p, from, to) \
  ((async_generator_object_p)->extended_object.u.class_prop.extra_info ^= ECMA_ASYNC_YIELD_ITERATOR_CS2 (from, to))

ecma_value_t ecma_async_generator_enqueue (vm_executable_object_t *async_generator_object_p,
                                           ecma_async_generator_operation_type_t operation, ecma_value_t value);

void ecma_async_generator_run (vm_executable_object_t *async_generator_object_p);
void ecma_async_generator_finalize (vm_executable_object_t *async_generator_object_p, ecma_value_t value);

ecma_value_t ecma_async_yield_continue_await (vm_executable_object_t *async_generator_object_p, ecma_value_t value);

#endif /* ENABLED (JERRY_ESNEXT) */

/**
 * @}
 * @}
 */

#endif /* !ECMA_ASYNC_GENERATOR_OBJECT_H */
