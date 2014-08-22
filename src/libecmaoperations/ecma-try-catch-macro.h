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
  ecma_completion_value_t var = op; \
  if (unlikely (ecma_is_completion_value_throw (var))) \
  { \
    return_value = ecma_copy_completion_value (var); \
  } \
  else \
  { \
    JERRY_ASSERT(ecma_is_completion_value_normal (var))

/**
 * The macro defines function call block that executes function call 'op',
 * assigns call's completion value to 'var', and checks for exceptions
 * that might be thrown during initialization.
 *
 * If no return values is not return completion value,
 * then code after the function call block is executed.
 * Otherwise, completion value is just copied to return_value.
 *
 * Note:
 *      Each ECMA_FUNCTION_CALL should have it's own corresponding ECMA_FINALIZE
 *      statement with same argument as corresponding ECMA_FUNCTION_CALL's first argument.
 */
#define ECMA_FUNCTION_CALL(var, op, return_value) \
  ecma_completion_value_t var = op; \
  if (unlikely (!ecma_is_completion_value_return (var))) \
  { \
    return_value = ecma_copy_completion_value (var); \
  } \
  else \
  { \
    JERRY_ASSERT(!ecma_is_completion_value_normal (var))

/**
 * The define is not used. It is just for vera++ style checker that wants to find closing pair for all opening braces
 */
#define ECMA_FUNCTION_CALL_CLOSING_BRACKET_FOR_VERA_STYLE_CHECKER }

/**
 * The macro marks end of code block that is defined by corresponding
 * ECMA_TRY_CATCH / ECMA_FUNCTION_CALL and frees variable, initialized
 * by the ECMA_TRY_CATCH / ECMA_FUNCTION_CALL.
 *
 * Note:
 *      Each ECMA_TRY_CATCH / ECMA_FUNCTION_CALL should be followed by ECMA_FINALIZE with same argument
 *      as corresponding ECMA_TRY_CATCH's / ECMA_FUNCTION_CALL's first argument.
 */
#define ECMA_FINALIZE(var) } \
  ecma_free_completion_value (var)

#endif /* !ECMA_TRY_CATCH_MACRO_H */
