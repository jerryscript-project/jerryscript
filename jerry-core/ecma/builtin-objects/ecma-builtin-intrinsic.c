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
#include "ecma-builtin-helpers.h"
#include "ecma-container-object.h"
#include "ecma-array-object.h"
#include "ecma-typedarray-object.h"
#include "ecma-gc.h"
#include "lit-char-helpers.h"

#if ENABLED (JERRY_ESNEXT)

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
  ECMA_INTRINSIC_ARRAY_PROTOTYPE_VALUES,
  ECMA_INTRINSIC_TYPEDARRAY_PROTOTYPE_VALUES,
  ECMA_INTRINSIC_MAP_PROTOTYPE_ENTRIES,
  ECMA_INTRINSIC_SET_PROTOTYPE_VALUES,
  ECMA_INTRINSIC_ARRAY_TO_STRING,
  ECMA_INTRINSIC_DATE_TO_UTC_STRING,
  ECMA_INTRINSIC_PARSE_FLOAT,
  ECMA_INTRINSIC_PARSE_INT
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
 * The Map.prototype entries and [@@iterator] routines
 *
 * See also:
 *          ECMA-262 v6, 23.1.3.4
 *          ECMA-262 v6, 23.1.3.12
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_intrinsic_map_prototype_entries (ecma_value_t this_value)
{
  ecma_extended_object_t *map_object_p = ecma_op_container_get_object (this_value, LIT_MAGIC_STRING_MAP_UL);

  if (map_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  return ecma_op_container_create_iterator (this_value,
                                            ECMA_BUILTIN_ID_MAP_ITERATOR_PROTOTYPE,
                                            ECMA_PSEUDO_MAP_ITERATOR,
                                            ECMA_ITERATOR_ENTRIES);
} /* ecma_builtin_intrinsic_map_prototype_entries */

/**
 * The Set.prototype values, keys and [@@iterator] routines
 *
 * See also:
 *          ECMA-262 v6, 23.2.3.8
 *          ECMA-262 v6, 23.2.3.10
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_intrinsic_set_prototype_values (ecma_value_t this_value)
{
  ecma_extended_object_t *map_object_p = ecma_op_container_get_object (this_value, LIT_MAGIC_STRING_SET_UL);

  if (map_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  return ecma_op_container_create_iterator (this_value,
                                            ECMA_BUILTIN_ID_SET_ITERATOR_PROTOTYPE,
                                            ECMA_PSEUDO_SET_ITERATOR,
                                            ECMA_ITERATOR_VALUES);
} /* ecma_builtin_intrinsic_set_prototype_values */

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
                                         uint32_t arguments_number) /**< length of arguments' list */
{
  JERRY_UNUSED (arguments_number);

  switch (builtin_routine_id)
  {
    case ECMA_INTRINSIC_ARRAY_PROTOTYPE_VALUES:
    {
      return ecma_builtin_intrinsic_array_prototype_values (this_arg);
    }
    case ECMA_INTRINSIC_TYPEDARRAY_PROTOTYPE_VALUES:
    {
      return ecma_typedarray_iterators_helper (this_arg, ECMA_ITERATOR_VALUES);
    }
    case ECMA_INTRINSIC_SET_PROTOTYPE_VALUES:
    {
      return ecma_builtin_intrinsic_set_prototype_values (this_arg);
    }
    case ECMA_INTRINSIC_MAP_PROTOTYPE_ENTRIES:
    {
      return ecma_builtin_intrinsic_map_prototype_entries (this_arg);
    }
    case ECMA_INTRINSIC_ARRAY_TO_STRING:
    {
      ecma_value_t this_obj = ecma_op_to_object (this_arg);
      if (ECMA_IS_VALUE_ERROR (this_obj))
      {
        return this_obj;
      }

      ecma_value_t result = ecma_array_object_to_string (this_obj);
      ecma_deref_object (ecma_get_object_from_value (this_obj));

      return result;
    }
    case ECMA_INTRINSIC_DATE_TO_UTC_STRING:
    {
      if (!ecma_is_value_object (this_arg)
          || !ecma_object_class_is (ecma_get_object_from_value (this_arg), LIT_MAGIC_STRING_DATE_UL))
      {
        return ecma_raise_type_error (ECMA_ERR_MSG ("'this' is not a Date object"));
      }

      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) ecma_get_object_from_value (this_arg);
      ecma_number_t *prim_value_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_number_t,
                                                                     ext_object_p->u.class_prop.u.value);

      if (ecma_number_is_nan (*prim_value_p))
      {
        return ecma_make_magic_string_value (LIT_MAGIC_STRING_INVALID_DATE_UL);
      }

      return ecma_date_value_to_utc_string (*prim_value_p);
    }
    default:
    {
      JERRY_ASSERT (builtin_routine_id == ECMA_INTRINSIC_PARSE_INT
                    || builtin_routine_id == ECMA_INTRINSIC_PARSE_FLOAT);

      ecma_string_t *str_p = ecma_op_to_string (arguments_list_p[0]);

      if (JERRY_UNLIKELY (str_p == NULL))
      {
        return ECMA_VALUE_ERROR;
      }

      ecma_value_t result;
      ECMA_STRING_TO_UTF8_STRING (str_p, string_buff, string_buff_size);

      if (builtin_routine_id == ECMA_INTRINSIC_PARSE_INT)
      {
        result = ecma_number_parse_int (string_buff,
                                        string_buff_size,
                                        arguments_list_p[1]);
      }
      else
      {
        JERRY_ASSERT (builtin_routine_id == ECMA_INTRINSIC_PARSE_FLOAT);
        result = ecma_number_parse_float (string_buff,
                                          string_buff_size);
      }

      ECMA_FINALIZE_UTF8_STRING (string_buff, string_buff_size);
      ecma_deref_ecma_string (str_p);

      return result;
    }
  }
} /* ecma_builtin_intrinsic_dispatch_routine */

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_ESNEXT) */
