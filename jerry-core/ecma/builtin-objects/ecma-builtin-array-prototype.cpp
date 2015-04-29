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
#include "ecma-objects.h"
#include "ecma-string-object.h"
#include "ecma-try-catch-macro.h"
#include "jrt.h"

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ARRAY_BUILTIN

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-array-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID array_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup arrayprototype ECMA Array.prototype object built-in
 * @{
 */

/**
 * The Array.prototype object's 'toString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_array_prototype_object_to_string (ecma_value_t this_arg) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
} /* ecma_builtin_array_prototype_object_to_string */

/**
 * The Array.prototype object's 'push' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.7
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_array_prototype_object_push (ecma_value_t this_arg, /**< this argument */
                                          const ecma_value_t *argument_list_p, /**< arguments list */
                                          ecma_length_t arguments_number) /**< number of arguments */
{
  JERRY_ASSERT (argument_list_p == NULL || arguments_number > 0);

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value();

  // 1.
  ECMA_TRY_CATCH (obj_this_value, ecma_op_to_object (this_arg), ret_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this_value);

  // 2.
  ecma_string_t *length_str_p = ecma_new_ecma_string_from_magic_string_id (ECMA_MAGIC_STRING_LENGTH);

  ECMA_TRY_CATCH (length_value, ecma_op_object_get (obj_p, length_str_p), ret_value);

  // 3.
  ECMA_OP_TO_NUMBER_TRY_CATCH (length_var, length_value, ret_value);

  uint32_t n = ecma_number_to_uint32 (length_var);

  // 5.
  for (uint32_t index = 0;
       index < arguments_number;
       ++index, ++n)
  {
    // a.
    ecma_value_t e_value = argument_list_p[index];

    // b.
    ecma_string_t *n_str_p = ecma_new_ecma_string_from_uint32 (n);

    ecma_completion_value_t completion = ecma_op_object_put (obj_p, n_str_p, e_value, true);

    ecma_deref_ecma_string (n_str_p);

    if (unlikely (ecma_is_completion_value_throw (completion)
                  || ecma_is_completion_value_exit (completion)))
    {
      ret_value = completion;
      break;
    }
    else
    {
      JERRY_ASSERT (ecma_is_completion_value_normal (completion));
      ecma_free_completion_value (completion);
    }
  }

  // 6.
  if (ecma_is_completion_value_empty (ret_value))
  {
    ecma_number_t *num_length_p = ecma_alloc_number ();
    *num_length_p = ecma_uint32_to_number (n);

    ecma_value_t num_length_value = ecma_make_number_value (num_length_p);

    ecma_completion_value_t completion = ecma_op_object_put (obj_p,
                                                             length_str_p,
                                                             num_length_value,
                                                             true);

    if (unlikely (ecma_is_completion_value_throw (completion)
                  || ecma_is_completion_value_exit(completion)))
    {
      ret_value = completion;

      ecma_dealloc_number (num_length_p);
    }
    else
    {
      JERRY_ASSERT (ecma_is_completion_value_normal (completion));
      ecma_free_completion_value (completion);

      ret_value = ecma_make_normal_completion_value (num_length_value);
    }
  }

  ECMA_OP_TO_NUMBER_FINALIZE (length_var);

  ECMA_FINALIZE (length_value);

  ecma_deref_ecma_string (length_str_p);

  ECMA_FINALIZE (obj_this_value);

  return ret_value;
} /* ecma_builtin_array_prototype_object_push */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ARRAY_BUILTIN */
