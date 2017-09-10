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
 */
#define ECMA_TRY_CATCH(var, op, return_value) \
  JERRY_ASSERT (return_value == ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY)); \
  ecma_value_t var ## _completion = op; \
  if (ECMA_IS_VALUE_ERROR (var ## _completion)) \
  { \
    return_value = var ## _completion; \
  } \
  else \
  { \
    ecma_value_t var = var ## _completion; \
    JERRY_UNUSED (var);

/**
 * The macro marks end of code block that is defined by corresponding
 * ECMA_TRY_CATCH and frees variable, initialized by the ECMA_TRY_CATCH.
 *
 * Note:
 *      Each ECMA_TRY_CATCH should be followed by ECMA_FINALIZE with same argument
 *      as corresponding ECMA_TRY_CATCH's first argument.
 */
#define ECMA_FINALIZE(var) \
    ecma_free_value (var ## _completion); \
  }

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
  JERRY_ASSERT (return_value == ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY)); \
  ecma_number_t num_var = ecma_number_make_nan (); \
  if (ecma_is_value_number (value)) \
  { \
    num_var = ecma_get_number_from_value (value); \
  } \
  else \
  { \
    ECMA_TRY_CATCH (to_number_value, \
                    ecma_op_to_number (value), \
                    return_value); \
    \
    num_var = ecma_get_number_from_value (to_number_value); \
    \
    ECMA_FINALIZE (to_number_value); \
  } \
  \
  if (ecma_is_value_empty (return_value)) \
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
