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

#include "ecma-alloc.h"
#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-number-object.h"
#include "ecma-objects.h"
#include "ecma-try-catch-macro.h"
#include "jrt.h"

#ifndef CONFIG_DISABLE_NUMBER_BUILTIN

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-number.inc.h"
#define BUILTIN_UNDERSCORED_ID number
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup number ECMA Number object built-in
 * @{
 */

/**
 * Handle calling [[Call]] of built-in Number object
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_number_dispatch_call (const ecma_value_t *arguments_list_p, /**< arguments list */
                                   ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  if (arguments_list_len == 0)
  {
    ret_value = ecma_make_integer_value (0);
  }
  else
  {
    ret_value = ecma_op_to_number (arguments_list_p[0]);
  }

  return ret_value;
} /* ecma_builtin_number_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in Number object
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_number_dispatch_construct (const ecma_value_t *arguments_list_p, /**< arguments list */
                                        ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  if (arguments_list_len == 0)
  {
    ecma_value_t completion = ecma_op_create_number_object (ecma_make_integer_value (0));
    return completion;
  }
  else
  {
    return ecma_op_create_number_object (arguments_list_p[0]);
  }
} /* ecma_builtin_number_dispatch_construct */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_NUMBER_BUILTIN */
