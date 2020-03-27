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
#include "ecma-array-object.h"
#include "ecma-gc.h"
#include "lit-char-helpers.h"

#if ENABLED (JERRY_ES2015)

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

/**
 * This object has a custom dispatch function.
 */
#define BUILTIN_CUSTOM_DISPATCH

/**
 * List of built-in routine identifiers.
 */
enum
{
  ECMA_INTRINSIC_ROUTINE_START = ECMA_BUILTIN_ID__COUNT - 1,
  ECMA_INTRINSIC_PARSE_FLOAT,
  ECMA_INTRINSIC_PARSE_INT,
  ECMA_INTRINSIC_ARRAY_PROTOTYPE_VALUES
};

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-intrinsic.inc.h"
#define BUILTIN_UNDERSCORED_ID intrinsic
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup intrinsic ECMA Intrinsic object built-in
 * @{
 */

/**
 * The %ArrayProto_values% intrinsic routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.4
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_intrinsic_array_prototype_values (ecma_value_t this_value) /**< this argument */
{
  ecma_value_t this_obj = ecma_op_to_object (this_value);

  if (ECMA_IS_VALUE_ERROR (this_obj))
  {
    return this_obj;
  }

  ecma_object_t *this_obj_p = ecma_get_object_from_value (this_obj);

  ecma_value_t ret_value = ecma_op_create_array_iterator (this_obj_p, ECMA_ITERATOR_VALUES);

  ecma_deref_object (this_obj_p);

  return ret_value;
} /* ecma_builtin_intrinsic_array_prototype_values */

/**
 * Dispatcher of the built-in's routines
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_intrinsic_dispatch_routine (uint16_t builtin_routine_id, /**< built-in wide routine identifier */
                                         ecma_value_t this_arg, /**< 'this' argument value */
                                         const ecma_value_t arguments_list_p[], /**< list of arguments
                                                                                 *   passed to routine */
                                         ecma_length_t arguments_number) /**< length of arguments' list */
{
  JERRY_UNUSED (arguments_number);

  ecma_value_t routine_arg_1 = arguments_list_p[0];
  ecma_value_t routine_arg_2 = arguments_list_p[1];

  if (builtin_routine_id == ECMA_INTRINSIC_ARRAY_PROTOTYPE_VALUES)
  {
    return ecma_builtin_intrinsic_array_prototype_values (this_arg);
  }
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;
  if (builtin_routine_id <= ECMA_INTRINSIC_PARSE_INT)
  {
    ecma_string_t *str_p = ecma_op_to_string (routine_arg_1);

    if (JERRY_UNLIKELY (str_p == NULL))
    {
      return ECMA_VALUE_ERROR;
    }

    ECMA_STRING_TO_UTF8_STRING (str_p, string_buff, string_buff_size);

    if (builtin_routine_id == ECMA_INTRINSIC_PARSE_INT)
    {
      ret_value = ecma_number_parse_int (string_buff,
                                         string_buff_size,
                                         routine_arg_2);
    }
    else
    {
      JERRY_ASSERT (builtin_routine_id == ECMA_INTRINSIC_PARSE_FLOAT);
      ret_value = ecma_number_parse_float (string_buff,
                                           string_buff_size);
    }
    ECMA_FINALIZE_UTF8_STRING (string_buff, string_buff_size);
    ecma_deref_ecma_string (str_p);
  }

  return ret_value;
} /* ecma_builtin_intrinsic_dispatch_routine */

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_ES2015) */
