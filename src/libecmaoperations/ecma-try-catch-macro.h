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
  if ( unlikely( ecma_is_completion_value_throw( var) ) ) \
  { \
    return_value = ecma_copy_completion_value( var); \
  } \
  else \
  { \
    JERRY_ASSERT( ecma_is_completion_value_normal( var) )

/**
 * The macro marks end of code block that is executed if no exception
 * was catched by corresponding ECMA_TRY_CATCH and frees variable,
 * initialized by the ECMA_TRY_CATCH.
 *
 * Note:
 *      Each ECMA_TRY_CATCH should be followed by ECMA_FINALIZE with same
 *      argument as corresponding ECMA_TRY_CATCH's first argument.
 */
#define ECMA_FINALIZE(var) } \
  ecma_free_completion_value( var)

#endif /* !ECMA_TRY_CATCH_MACRO_H */
