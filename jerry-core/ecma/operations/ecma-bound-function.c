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

#include "ecma-bound-function.h"

#include "ecma-builtins.h"
#include "ecma-function-object.h"
#include "ecma-globals.h"
#include "ecma-objects.h"
#include "ecma-ordinary-object.h"

#include "jcontext.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaboundfunctionobject ECMA bound function object related routines
 * @{
 */

/**
 * ecma bound function object's [[GetOwnProperty]] internal method
 *
 * @return ecma property descriptor t
 */
ecma_property_descriptor_t
ecma_bound_function_get_own_property (ecma_object_t *obj_p, /**< the object */
                                      ecma_string_t *property_name_p) /**< property name */
{
  ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();

  prop_desc.u.property_p = ecma_find_named_property (obj_p, property_name_p);

  if (prop_desc.u.property_p != NULL)
  {
    prop_desc.flags =
      ECMA_PROP_DESC_PROPERTY_FOUND | ECMA_PROPERTY_TO_PROPERTY_DESCRIPTOR_FLAGS (prop_desc.u.property_p);
    return prop_desc;
  }

  if (ecma_string_is_length (property_name_p))
  {
    ecma_bound_function_t *bound_func_p = (ecma_bound_function_t *) obj_p;
    ecma_value_t args_len_or_this = bound_func_p->header.u.bound_function.args_len_or_this;
    ecma_number_t length = 0;
    ecma_integer_value_t args_length = 1;
    uint8_t length_attributes;

    if (ecma_is_value_integer_number (args_len_or_this))
    {
      args_length = ecma_get_integer_from_value (args_len_or_this);
    }

#if JERRY_ESNEXT
    if (ECMA_GET_FIRST_BIT_FROM_POINTER_TAG (bound_func_p->header.u.bound_function.target_function))
    {
      return prop_desc;
    }

    prop_desc.flags = ECMA_PROP_DESC_PROPERTY_FOUND | ECMA_PROP_DESC_DATA_CONFIGURABLE;
    length_attributes = ECMA_PROPERTY_BUILT_IN_CONFIGURABLE;
    length = ecma_get_number_from_value (bound_func_p->target_length) - (args_length - 1);
#else /* !JERRY_ESNEXT */
    prop_desc.flags = ECMA_PROP_DESC_PROPERTY_FOUND | ECMA_PROP_DESC_DATA;
    length_attributes = ECMA_PROPERTY_BUILT_IN_FIXED;

    ecma_object_t *target_func_p;
    target_func_p =
      ECMA_GET_NON_NULL_POINTER_FROM_POINTER_TAG (ecma_object_t, bound_func_p->header.u.bound_function.target_function);

    if (ecma_object_get_class_name (target_func_p) == LIT_MAGIC_STRING_FUNCTION_UL)
    {
      /* The property_name_p argument contains the 'length' string. */
      ecma_value_t get_len_value =
        ecma_internal_method_get (target_func_p, property_name_p, ecma_make_object_value (target_func_p));

      JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (get_len_value));
      JERRY_ASSERT (ecma_is_value_integer_number (get_len_value));

      length = (ecma_number_t) (ecma_get_integer_from_value (get_len_value) - (args_length - 1));
    }
#endif /* JERRY_ESNEXT */

    if (length < 0)
    {
      length = 0;
    }

    ecma_property_value_t *prop_value_p =
      ecma_create_named_data_property (obj_p, property_name_p, length_attributes, &prop_desc.u.property_p);
    prop_value_p->value = ecma_make_number_value (length);
    return prop_desc;
  }

#if !JERRY_ESNEXT
  if (ecma_compare_ecma_string_to_magic_id (property_name_p, LIT_MAGIC_STRING_CALLER)
      || ecma_compare_ecma_string_to_magic_id (property_name_p, LIT_MAGIC_STRING_ARGUMENTS))
  {
    ecma_object_t *thrower_p = ecma_builtin_get (ECMA_BUILTIN_ID_TYPE_ERROR_THROWER);

    prop_desc.flags = ECMA_PROP_DESC_PROPERTY_FOUND;
    /* The string_p argument contans the name. */
    ecma_create_named_accessor_property (obj_p,
                                         property_name_p,
                                         thrower_p,
                                         thrower_p,
                                         ECMA_PROPERTY_BUILT_IN_FIXED,
                                         &prop_desc.u.property_p);
    return prop_desc;
  }
#endif /* !JERRY_ESNEXT */

  return prop_desc;
} /* ecma_bound_function_get_own_property */

/**
 * Append the bound arguments into the given collection
 *
 * Note:
 *       - The whole bound chain is resolved
 *       - The first element of the collection contains the bounded this value
 *
 * @return target function of the bound function
 */
JERRY_ATTR_NOINLINE static ecma_object_t *
ecma_op_bound_function_get_argument_list (ecma_object_t *func_obj_p, /**< bound bunction object */
                                          ecma_collection_t *list_p) /**< list of arguments */
{
  JERRY_ASSERT (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION);

  ecma_bound_function_t *bound_func_p = (ecma_bound_function_t *) func_obj_p;

  func_obj_p =
    ECMA_GET_NON_NULL_POINTER_FROM_POINTER_TAG (ecma_object_t, bound_func_p->header.u.bound_function.target_function);

  ecma_value_t args_len_or_this = bound_func_p->header.u.bound_function.args_len_or_this;

  uint32_t args_length = 1;

  if (ecma_is_value_integer_number (args_len_or_this))
  {
    args_length = (uint32_t) ecma_get_integer_from_value (args_len_or_this);
  }

  /* 5. */
  if (args_length != 1)
  {
    const ecma_value_t *args_p = (const ecma_value_t *) (bound_func_p + 1);
    list_p->buffer_p[0] = *args_p;

    if (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION)
    {
      func_obj_p = ecma_op_bound_function_get_argument_list (func_obj_p, list_p);
    }
    ecma_collection_append (list_p, args_p + 1, args_length - 1);
  }
  else
  {
    list_p->buffer_p[0] = args_len_or_this;
  }

  return func_obj_p;
} /* ecma_op_bound_function_get_argument_list */

/**
 * ecma bound function object's [[Call]] internal method
 *
 * @return ecma value t
 */
ecma_value_t
ecma_bound_function_call (ecma_object_t *func_obj_p, /**< Function object */
                          ecma_value_t this_value, /**< this value */
                          const ecma_value_t *arguments_list_p, /**< arguments list */
                          uint32_t arguments_list_len) /**< length of arguments list */
{
  JERRY_UNUSED (this_value);

  JERRY_CONTEXT (status_flags) &= (uint32_t) ~ECMA_STATUS_DIRECT_EVAL;

  ecma_collection_t *bound_arg_list_p = ecma_new_collection ();
  ecma_collection_push_back (bound_arg_list_p, ECMA_VALUE_EMPTY);

  ecma_object_t *target_obj_p = ecma_op_bound_function_get_argument_list (func_obj_p, bound_arg_list_p);

  ecma_collection_append (bound_arg_list_p, arguments_list_p, arguments_list_len);

  JERRY_ASSERT (!ecma_is_value_empty (bound_arg_list_p->buffer_p[0]));

  ecma_value_t ret_value = ecma_internal_method_call (target_obj_p,
                                                      bound_arg_list_p->buffer_p[0],
                                                      bound_arg_list_p->buffer_p + 1,
                                                      (uint32_t) (bound_arg_list_p->item_count - 1));

  ecma_collection_destroy (bound_arg_list_p);

  return ret_value;
} /* ecma_bound_function_call */

/**
 * ecma bound function object's [[Construct]] internal method
 *
 * @return ecma property descriptor t
 */
ecma_value_t
ecma_bound_function_construct (ecma_object_t *func_obj_p, /**< Function object */
                               ecma_object_t *new_target_p, /**< new target */
                               const ecma_value_t *arguments_list_p, /**< arguments list */
                               uint32_t arguments_list_len) /**< length of arguments list */
{
  ecma_collection_t *bound_arg_list_p = ecma_new_collection ();
  ecma_collection_push_back (bound_arg_list_p, ECMA_VALUE_EMPTY);

  ecma_object_t *target_obj_p = ecma_op_bound_function_get_argument_list (func_obj_p, bound_arg_list_p);

  ecma_collection_append (bound_arg_list_p, arguments_list_p, arguments_list_len);

  if (func_obj_p == new_target_p)
  {
    new_target_p = target_obj_p;
  }

  ecma_value_t ret_value = ecma_internal_method_construct (target_obj_p,
                                                           new_target_p,
                                                           bound_arg_list_p->buffer_p + 1,
                                                           (uint32_t) (bound_arg_list_p->item_count - 1));

  ecma_collection_destroy (bound_arg_list_p);

  return ret_value;
} /* ecma_bound_function_construct */

/**
 * List lazy instantiated property names of bound function object
 */
void
ecma_bound_function_list_lazy_property_keys (ecma_object_t *object_p, /**< function object */
                                             ecma_collection_t *prop_names_p, /**< prop name collection */
                                             ecma_property_counter_t *prop_counter_p, /**< property counters */
                                             jerry_property_filter_t filter) /**< property name
                                                                              *   filter options */
{
  if (filter & JERRY_PROPERTY_FILTER_EXCLUDE_STRINGS)
  {
    return;
  }

#if JERRY_ESNEXT
  /* Unintialized 'length' property is non-enumerable (ECMA-262 v6, 19.2.4.1) */
  ecma_bound_function_t *bound_func_p = (ecma_bound_function_t *) object_p;
  if (!ECMA_GET_FIRST_BIT_FROM_POINTER_TAG (bound_func_p->header.u.bound_function.target_function))
  {
    ecma_collection_push_back (prop_names_p, ecma_make_magic_string_value (LIT_MAGIC_STRING_LENGTH));
    prop_counter_p->string_named_props++;
  }
#else /* !JERRY_ESNEXT */
  JERRY_UNUSED (object_p);
  /* 'length' property is non-enumerable (ECMA-262 v5, 13.2.5) */
  ecma_collection_push_back (prop_names_p, ecma_make_magic_string_value (LIT_MAGIC_STRING_LENGTH));
  prop_counter_p->string_named_props++;
#endif /* JERRY_ESNEXT */

  /* 'caller' property is non-enumerable (ECMA-262 v5, 13.2.5) */
  ecma_collection_push_back (prop_names_p, ecma_make_magic_string_value (LIT_MAGIC_STRING_CALLER));

  /* 'arguments' property is non-enumerable (ECMA-262 v5, 13.2.5) */
  ecma_collection_push_back (prop_names_p, ecma_make_magic_string_value (LIT_MAGIC_STRING_ARGUMENTS));

  prop_counter_p->string_named_props += 2;
} /* ecma_bound_object_list_lazy_property_keys */

/**
 * Delete configurable properties of bound functions.
 */
void
ecma_bound_function_delete_lazy_property (ecma_object_t *object_p, /**< object */
                                          ecma_string_t *property_name_p) /**< property name */
{
  JERRY_UNUSED (property_name_p);

  ecma_bound_function_t *bound_func_p = (ecma_bound_function_t *) object_p;

  JERRY_ASSERT (ecma_compare_ecma_string_to_magic_id (property_name_p, LIT_MAGIC_STRING_LENGTH));
  JERRY_ASSERT (!ECMA_GET_FIRST_BIT_FROM_POINTER_TAG (bound_func_p->header.u.bound_function.target_function));

  ECMA_SET_FIRST_BIT_TO_POINTER_TAG (bound_func_p->header.u.bound_function.target_function);
} /* ecma_bound_function_delete_lazy_property */

/**
 * @}
 * @}
 */
