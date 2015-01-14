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

#ifndef ECMA_TRY_CATCH_MACRO_H
#define ECMA_TRY_CATCH_MACRO_H

#include "ecma-helpers.h"

/**
 * The macro defines try-block that initializes variable 'var' with 'op'
 * and checks for exceptions that might be thrown during initialization.
 *
 * If no exception was thrown, then code after the try-block is executed.
 * Otherwise, throw-completion value is just copied to return_value.
 *
 * Note:
 *      Each ECMA_TRY_CATCH should have it's own corresponding ECMA_FINALIZE
 *      statement with same argument as corresponding ECMA_TRY_CATCH's first argument.
 *
 * See also:
 *          ECMA_TRY_CATCH_STACKED
 */
#define ECMA_TRY_CATCH(var, op, return_value) \
  ecma_completion_value_t var = op; \
  if (unlikely (ecma_is_completion_value_throw (var))) \
  { \
    return_value = var; \
  } \
  else \
  { \
    JERRY_ASSERT(ecma_is_completion_value_normal (var))

/**
 * The macro marks end of code block that is defined by corresponding
 * ECMA_TRY_CATCH and frees variable, initialized by the ECMA_TRY_CATCH.
 *
 * Note:
 *      Each ECMA_TRY_CATCH should be followed by ECMA_FINALIZE with same argument
 *      as corresponding ECMA_TRY_CATCH's first argument.
 *
 * See also:
 *          ECMA_FINALIZE_STACKED
 */
#define ECMA_FINALIZE(var) ecma_free_completion_value (var); \
  }

/**
 * ECMA_TRY_CATCH for stack convention
 */
#define ECMA_TRY_CATCH_STACKED(var, op, return_value, frame_p) \
  ecma_completion_type_t var ## completion_type = op; \
  if (unlikely (var ## completion_type != ECMA_COMPLETION_TYPE_NORMAL)) \
  { \
    JERRY_ASSERT (var ## completion_type == ECMA_COMPLETION_TYPE_THROW); \
    return_value = ecma_make_completion_value (var ## completion_type, \
                                               ecma_copy_value (ecma_stack_top_value (frame_p), true)); \
  } \
  else \
  { \
    ecma_value_t var = ecma_stack_top_value (frame_p); \

/**
 * ECMA_FINALIZE for stack convention
 */
#define ECMA_FINALIZE_STACKED(var, frame_p) (void) var; \
  } \
  (void) var ## completion_type; \
  ecma_stack_pop (frame_p) \

/**
 * The macro defines try-block that tries to perform ToNumber operation on given value
 * and checks for exceptions that might be thrown during the operation.
 *
 * If no exception was thrown, then code after the try-block is executed.
 * Otherwise, throw-completion value is just copied to return_value.
 *
 * Note:
 *      Each ECMA_OP_TO_NUMBER_TRY_CATCH should have it's own corresponding ECMA_OP_TO_NUMBER_FINALIZE
 *      statement with same argument as corresponding ECMA_OP_TO_NUMBER_TRY_CATCH's first argument.
 */
#define ECMA_OP_TO_NUMBER_TRY_CATCH(num_var, value, return_value) \
  JERRY_ASSERT (ecma_is_completion_value_empty (return_value)); \
  ecma_number_t num_var = ecma_number_make_nan (); \
  if (ecma_is_value_number (value)) \
  { \
    num_var = *ecma_get_number_from_value (value); \
  } \
  else \
  { \
    ECMA_TRY_CATCH (to_number_completion_value, \
                    ecma_op_to_number (value), \
                    return_value); \
    \
    num_var = *ecma_get_number_from_completion_value (to_number_completion_value); \
    \
    ECMA_FINALIZE (to_number_completion_value); \
  } \
  \
  if (ecma_is_completion_value_empty (return_value)) \
  {

/**
 * The macro marks end of code block that is defined by corresponding ECMA_OP_TO_NUMBER_TRY_CATCH.
 *
 * Note:
 *      Each ECMA_OP_TO_NUMBER_TRY_CATCH should be followed by ECMA_OP_TO_NUMBER_FINALIZE
 *      with same argument as corresponding ECMA_OP_TO_NUMBER_TRY_CATCH's first argument.
 */
#define ECMA_OP_TO_NUMBER_FINALIZE(num_var) } \
  else \
  { \
    JERRY_ASSERT (ecma_number_is_nan (num_var)); \
  }

#endif /* !ECMA_TRY_CATCH_MACRO_H */
