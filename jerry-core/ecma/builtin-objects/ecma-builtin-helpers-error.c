/* Copyright 2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged
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
#include "ecma-builtin-helpers.h"
#include "ecma-objects.h"
#include "ecma-try-catch-macro.h"
#include "jrt.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltinhelpers ECMA builtin helper operations
 * @{
 */

/**
 * Handle calling [[Call]] of a built-in error object
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_helper_error_dispatch_call (ecma_standard_error_t error_type, /**< native error type */
                                         const ecma_value_t *arguments_list_p, /**< arguments list */
                                         ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  if (arguments_list_len != 0
      && !ecma_is_value_undefined (arguments_list_p[0]))
  {
    ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

    ECMA_TRY_CATCH (msg_str_value,
                    ecma_op_to_string (arguments_list_p[0]),
                    ret_value);

    ecma_string_t *message_string_p = ecma_get_string_from_value (msg_str_value);
    ecma_object_t *new_error_object_p = ecma_new_standard_error_with_message (error_type,
                                                                              message_string_p);
    ret_value = ecma_make_object_value (new_error_object_p);

    ECMA_FINALIZE (msg_str_value);

    return ret_value;
  }
  else
  {
    ecma_object_t *new_error_object_p = ecma_new_standard_error (error_type);

    return ecma_make_object_value (new_error_object_p);
  }
} /* ecma_builtin_helper_error_dispatch_call */

/**
 * @}
 * @}
 */

