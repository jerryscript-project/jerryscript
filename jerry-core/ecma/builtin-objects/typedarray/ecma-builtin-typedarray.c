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

#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-typedarray-object.h"
#include "ecma-try-catch-macro.h"
#include "jrt.h"

#ifndef CONFIG_DISABLE_TYPEDARRAY_BUILTIN

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-typedarray.inc.h"
#define BUILTIN_UNDERSCORED_ID typedarray
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup typedarray ECMA %TypedArray% object built-in
 * @{
 */

/**
 * The %TypedArray%.from routine
 *
 * See also:
 *         ES2015 22.2.2.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_from (ecma_value_t this_arg, /**< 'this' argument */
                              const ecma_value_t *arguments_list_p, /**< arguments list */
                              ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_UNUSED (this_arg);
  JERRY_UNUSED (arguments_list_p);
  JERRY_UNUSED (arguments_list_len);

  /* TODO: inplement 'from' */

  return ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
} /* ecma_builtin_typedarray_from */

/**
 * The %TypedArray%.of routine
 *
 * See also:
 *         ES2015 22.2.2.2
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_of (ecma_value_t this_arg, /**< 'this' argument */
                            const ecma_value_t *arguments_list_p, /**< arguments list */
                            ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_UNUSED (this_arg);
  JERRY_UNUSED (arguments_list_p);
  JERRY_UNUSED (arguments_list_len);

  /* TODO: inplement 'of' */

  return ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
} /* ecma_builtin_typedarray_of */

/**
 * Handle calling [[Call]] of built-in %TypedArray% object
 *
 * ES2015 22.2.1 If %TypedArray% is directly called or
 * called as part of a new expression an exception is thrown
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_typedarray_dispatch_call (const ecma_value_t *arguments_list_p, /**< arguments list */
                                       ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  return ecma_raise_type_error (ECMA_ERR_MSG ("TypedArray intrinstic cannot be directly called"));
} /* ecma_builtin_typedarray_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in %TypedArray% object
 *
 * ES2015 22.2.1 If %TypedArray% is directly called or
 * called as part of a new expression an exception is thrown
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_typedarray_dispatch_construct (const ecma_value_t *arguments_list_p, /**< arguments list */
                                            ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  return ecma_raise_type_error (ECMA_ERR_MSG ("TypedArray intrinstic cannot be called by a 'new' expression"));
} /* ecma_builtin_typedarray_dispatch_construct */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_TYPEDARRAY_BUILTIN */
