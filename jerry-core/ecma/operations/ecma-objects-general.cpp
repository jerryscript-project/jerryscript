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

#include "ecma-builtin-helpers.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-objects-general.h"
#include "ecma-try-catch-macro.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmageneralobjectsinternalops General ECMA objects' operations
 * @{
 */

/**
 * Reject sequence
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_reject (bool is_throw) /**< Throw flag */
{
  if (is_throw)
  {
    return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE);
  }
} /* ecma_reject */

/**
 * 'Object' object creation operation with no arguments.
 *
 * See also: ECMA-262 v5, 15.2.2.1
 *
 * @return pointer to newly created 'Object' object
 */
ecma_object_t*
ecma_op_create_object_object_noarg (void)
{
  ecma_object_t *object_prototype_p = ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE);

  // 3., 4., 6., 7.
  ecma_object_t *obj_p = ecma_op_create_object_object_noarg_and_set_prototype (object_prototype_p);

  ecma_deref_object (object_prototype_p);

  return obj_p;
} /* ecma_op_create_object_object_noarg */

/**
 * 'Object' object creation operation with one argument.
 *
 * See also: ECMA-262 v5, 15.2.2.1
 *
 * @return pointer to newly created 'Object' object
 */
ecma_completion_value_t
ecma_op_create_object_object_arg (ecma_value_t value) /**< argument of constructor */
{
  ecma_check_value_type_is_spec_defined (value);

  if (ecma_is_value_object (value)
      || ecma_is_value_number (value)
      || ecma_is_value_string (value)
      || ecma_is_value_boolean (value))
  {
    // 1.b, 1.c, 1.d
    return ecma_op_to_object (value);
  }
  else
  {
    // 2.
    JERRY_ASSERT (ecma_is_value_undefined (value)
                  || ecma_is_value_null (value));

    ecma_object_t *obj_p = ecma_op_create_object_object_noarg ();

    return ecma_make_normal_completion_value (ecma_make_object_value (obj_p));
  }
} /* ecma_op_create_object_object_arg */

/**
 * Object creation operation with no arguments.
 * It sets the given prototype to the newly created object.
 *
 * See also: ECMA-262 v5, 15.2.2.1, 15.2.3.5
 *
 * @return pointer to newly created object
 */
ecma_object_t*
ecma_op_create_object_object_noarg_and_set_prototype (ecma_object_t *object_prototype_p) /**< pointer to prototype of
                                                                                              the object
                                                                                              (can be NULL) */
{
  ecma_object_t *obj_p = ecma_create_object (object_prototype_p, true, ECMA_OBJECT_TYPE_GENERAL);

  /*
   * [[Class]] property of ECMA_OBJECT_TYPE_GENERAL type objects
   * without ECMA_INTERNAL_PROPERTY_CLASS internal property
   * is "Object".
   *
   * See also: ecma_object_get_class_name
   */

  return obj_p;
} /* ecma_op_create_object_object_noarg_and_set_prototype */

/**
 * [[Get]] ecma general object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *          ECMA-262 v5, 8.12.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_general_object_get (ecma_object_t *obj_p, /**< the object */
                            ecma_string_t *property_name_p) /**< property name */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT (property_name_p != NULL);

  // 1.
  const ecma_property_t* prop_p = ecma_op_object_get_property (obj_p, property_name_p);

  // 2.
  if (prop_p == NULL)
  {
    return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_UNDEFINED);
  }

  // 3.
  if (prop_p->type == ECMA_PROPERTY_NAMEDDATA)
  {
    return ecma_make_normal_completion_value (ecma_copy_value (ecma_get_named_data_property_value (prop_p),
                                                               true));
  }
  else
  {
    // 4.
    ecma_object_t *getter_p = ecma_get_named_accessor_property_getter (prop_p);

    // 5.
    if (getter_p == NULL)
    {
      return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }
    else
    {
      return ecma_op_function_call (getter_p,
                                    ecma_make_object_value (obj_p),
                                    NULL);
    }
  }

  JERRY_UNREACHABLE ();
} /* ecma_op_general_object_get */

/**
 * [[GetOwnProperty]] ecma general object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *          ECMA-262 v5, 8.12.2
 *
 * @return pointer to a property - if it exists,
 *         NULL (i.e. ecma-undefined) - otherwise.
 */
ecma_property_t*
ecma_op_general_object_get_own_property (ecma_object_t *obj_p, /**< the object */
                                         ecma_string_t *property_name_p) /**< property name */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT (property_name_p != NULL);

  return ecma_find_named_property (obj_p, property_name_p);
} /* ecma_op_general_object_get_own_property */

/**
 * [[GetProperty]] ecma general object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *          ECMA-262 v5, 8.12.2
 *
 * @return pointer to a property - if it exists,
 *         NULL (i.e. ecma-undefined) - otherwise.
 */
ecma_property_t*
ecma_op_general_object_get_property (ecma_object_t *obj_p, /**< the object */
                                     ecma_string_t *property_name_p) /**< property name */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT (property_name_p != NULL);

  // 1.
  ecma_property_t *prop_p = ecma_op_object_get_own_property (obj_p, property_name_p);

  // 2.
  if (prop_p != NULL)
  {
    return prop_p;
  }

  // 3.
  ecma_object_t *prototype_p = ecma_get_object_prototype (obj_p);

  // 4., 5.
  if (prototype_p != NULL)
  {
    return ecma_op_object_get_property (prototype_p, property_name_p);
  }
  else
  {
    return NULL;
  }
} /* ecma_op_general_object_get_property */

/**
 * [[Put]] ecma general object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *          ECMA-262 v5, 8.12.5
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_general_object_put (ecma_object_t *obj_p, /**< the object */
                            ecma_string_t *property_name_p, /**< property name */
                            ecma_value_t value, /**< ecma-value */
                            bool is_throw) /**< flag that controls failure handling */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT (property_name_p != NULL);

  // 1.
  if (!ecma_op_object_can_put (obj_p, property_name_p))
  {
    if (is_throw)
    {
      // a.
      return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
    }
    else
    {
      // b.
      return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE);
    }
  }

  // 2.
  ecma_property_t *own_desc_p = ecma_op_object_get_own_property (obj_p, property_name_p);

  // 3.
  if (own_desc_p != NULL
      && own_desc_p->type == ECMA_PROPERTY_NAMEDDATA)
  {
    // a.
    ecma_property_descriptor_t value_desc = ecma_make_empty_property_descriptor ();
    {
      value_desc.is_value_defined = true;
      value_desc.value = value;
    }

    // b., c.
    return ecma_op_object_define_own_property (obj_p,
                                               property_name_p,
                                               &value_desc,
                                               is_throw);
  }

  // 4.
  ecma_property_t *desc_p = ecma_op_object_get_property (obj_p, property_name_p);

  // 5.
  if (desc_p != NULL
      && desc_p->type == ECMA_PROPERTY_NAMEDACCESSOR)
  {
    // a.
    ecma_object_t *setter_p = ecma_get_named_accessor_property_setter (desc_p);
    JERRY_ASSERT (setter_p != NULL);

    ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

    ECMA_TRY_CATCH (call_ret,
                    ecma_op_function_call_array_args (setter_p,
                                                      ecma_make_object_value (obj_p),
                                                      &value,
                                                      1),
                    ret_value);

    ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);

    ECMA_FINALIZE (call_ret);

    return ret_value;
  }
  else
  {
    // 6.a., 6.b.
    return ecma_builtin_helper_def_prop (obj_p,
                                         property_name_p,
                                         value,
                                         true, /* Writable */
                                         true, /* Enumerable */
                                         true, /* Configurable */
                                         is_throw); /* Failure handling */
  }

  JERRY_UNREACHABLE ();
} /* ecma_op_general_object_put */

/**
 * [[CanPut]] ecma general object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *          ECMA-262 v5, 8.12.4
 *
 * @return true - if [[Put]] with the given property name can be performed;
 *         false - otherwise.
 */
bool
ecma_op_general_object_can_put (ecma_object_t *obj_p, /**< the object */
                                ecma_string_t *property_name_p) /**< property name */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT (property_name_p != NULL);

  // 1.
  ecma_property_t *prop_p = ecma_op_object_get_own_property (obj_p, property_name_p);

  // 2.
  if (prop_p != NULL)
  {
    // a.
    if (prop_p->type == ECMA_PROPERTY_NAMEDACCESSOR)
    {
      ecma_object_t *setter_p = ecma_get_named_accessor_property_setter (prop_p);

      // i.
      if (setter_p == NULL)
      {
        return false;
      }

      // ii.
      return true;
    }
    else
    {
      // b.

      JERRY_ASSERT (prop_p->type == ECMA_PROPERTY_NAMEDDATA);

      return ecma_is_property_writable (prop_p);
    }
  }

  // 3.
  ecma_object_t *proto_p = ecma_get_object_prototype (obj_p);

  // 4.
  if (proto_p == NULL)
  {
    return ecma_get_object_extensible (obj_p);
  }

  // 5.
  ecma_property_t *inherited_p = ecma_op_object_get_property (proto_p, property_name_p);

  // 6.
  if (inherited_p == NULL)
  {
    return ecma_get_object_extensible (obj_p);
  }

  // 7.
  if (inherited_p->type == ECMA_PROPERTY_NAMEDACCESSOR)
  {
    ecma_object_t *setter_p = ecma_get_named_accessor_property_setter (inherited_p);

    // a.
    if (setter_p == NULL)
    {
      return false;
    }

    // b.
    return true;
  }
  else
  {
    // 8.
    JERRY_ASSERT (inherited_p->type == ECMA_PROPERTY_NAMEDDATA);

    // a.
    if (!ecma_get_object_extensible (obj_p))
    {
      return false;
    }
    else
    {
      // b.
      return ecma_is_property_writable (inherited_p);
    }
  }

  JERRY_UNREACHABLE ();
} /* ecma_op_general_object_can_put */

/**
 * [[Delete]] ecma general object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *          ECMA-262 v5, 8.12.7
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_general_object_delete (ecma_object_t *obj_p, /**< the object */
                               ecma_string_t *property_name_p, /**< property name */
                               bool is_throw) /**< flag that controls failure handling */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT (property_name_p != NULL);

  // 1.
  ecma_property_t *desc_p = ecma_op_object_get_own_property (obj_p, property_name_p);

  // 2.
  if (desc_p == NULL)
  {
    return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
  }

  // 3.
  if (ecma_is_property_configurable (desc_p))
  {
    // a.
    ecma_delete_property (obj_p, desc_p);

    // b.
    return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
  }
  else if (is_throw)
  {
    // 4.
    return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    // 5.
    return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE);
  }

  JERRY_UNREACHABLE ();
} /* ecma_op_general_object_delete */

/**
 * [[DefaultValue]] ecma general object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *          ECMA-262 v5, 8.12.8
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_general_object_default_value (ecma_object_t *obj_p, /**< the object */
                                      ecma_preferred_type_hint_t hint) /**< hint on preferred result type */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));

  if (hint == ECMA_PREFERRED_TYPE_NO)
  {
    if (ecma_object_get_class_name (obj_p) == LIT_MAGIC_STRING_DATE_UL)
    {
      hint = ECMA_PREFERRED_TYPE_STRING;
    }
    else
    {
      hint = ECMA_PREFERRED_TYPE_NUMBER;
    }
  }

  for (uint32_t i = 1; i <= 2; i++)
  {
    lit_magic_string_id_t function_name_magic_string_id;

    if ((i == 1 && hint == ECMA_PREFERRED_TYPE_STRING)
        || (i == 2 && hint == ECMA_PREFERRED_TYPE_NUMBER))
    {
      function_name_magic_string_id = LIT_MAGIC_STRING_TO_STRING_UL;
    }
    else
    {
      function_name_magic_string_id = LIT_MAGIC_STRING_VALUE_OF_UL;
    }

    ecma_string_t *function_name_p = ecma_get_magic_string (function_name_magic_string_id);

    ecma_completion_value_t function_value_get_completion = ecma_op_object_get (obj_p, function_name_p);

    ecma_deref_ecma_string (function_name_p);

    if (!ecma_is_completion_value_normal (function_value_get_completion))
    {
      return function_value_get_completion;
    }

    ecma_completion_value_t call_completion = ecma_make_empty_completion_value ();

    if (ecma_op_is_callable (ecma_get_completion_value_value (function_value_get_completion)))
    {
      ecma_object_t *func_obj_p = ecma_get_object_from_completion_value (function_value_get_completion);

      call_completion = ecma_op_function_call (func_obj_p,
                                               ecma_make_object_value (obj_p),
                                               NULL);
    }

    ecma_free_completion_value (function_value_get_completion);

    if (!ecma_is_completion_value_normal (call_completion))
    {
      return call_completion;
    }

    if (!ecma_is_completion_value_empty (call_completion)
        && !ecma_is_value_object (ecma_get_completion_value_value (call_completion)))
    {
      return call_completion;
    }

    ecma_free_completion_value (call_completion);
  }

  return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
} /* ecma_op_general_object_default_value */

/**
 * [[DefineOwnProperty]] ecma general object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *          ECMA-262 v5, 8.12.9
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_general_object_define_own_property (ecma_object_t *obj_p, /**< the object */
                                            ecma_string_t *property_name_p, /**< property name */
                                            const ecma_property_descriptor_t* property_desc_p, /**< property
                                                                                                *   descriptor */
                                            bool is_throw) /**< flag that controls failure handling */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT (property_name_p != NULL);

  const bool is_property_desc_generic_descriptor = (!property_desc_p->is_value_defined
                                                    && !property_desc_p->is_writable_defined
                                                    && !property_desc_p->is_get_defined
                                                    && !property_desc_p->is_set_defined);
  const bool is_property_desc_data_descriptor = (property_desc_p->is_value_defined
                                                 || property_desc_p->is_writable_defined);
  const bool is_property_desc_accessor_descriptor = (property_desc_p->is_get_defined
                                                     || property_desc_p->is_set_defined);

  // 1.
  ecma_property_t *current_p = ecma_op_object_get_own_property (obj_p, property_name_p);

  // 2.
  bool extensible = ecma_get_object_extensible (obj_p);

  if (current_p == NULL)
  {
    // 3.
    if (!extensible)
    {
      return ecma_reject (is_throw);
    }

    // 4.

    // a.
    if (is_property_desc_generic_descriptor
        || is_property_desc_data_descriptor)
    {
      ecma_property_t *new_prop_p = ecma_create_named_data_property (obj_p,
                                                                     property_name_p,
                                                                     property_desc_p->is_writable,
                                                                     property_desc_p->is_enumerable,
                                                                     property_desc_p->is_configurable);

      ecma_named_data_property_assign_value (obj_p, new_prop_p, property_desc_p->value);
    }
    else
    {
      // b.
      JERRY_ASSERT (is_property_desc_accessor_descriptor);

      ecma_create_named_accessor_property (obj_p,
                                           property_name_p,
                                           property_desc_p->get_p,
                                           property_desc_p->set_p,
                                           property_desc_p->is_enumerable,
                                           property_desc_p->is_configurable);

    }

    return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
  }

  // 5.
  if (is_property_desc_generic_descriptor
      && !property_desc_p->is_enumerable_defined
      && !property_desc_p->is_configurable_defined)
  {
    return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
  }

  // 6.
  const bool is_current_data_descriptor = (current_p->type == ECMA_PROPERTY_NAMEDDATA);
  const bool is_current_accessor_descriptor = (current_p->type == ECMA_PROPERTY_NAMEDACCESSOR);

  JERRY_ASSERT (is_current_data_descriptor || is_current_accessor_descriptor);

  bool is_every_field_in_desc_also_occurs_in_current_desc_with_same_value = true;
  if (property_desc_p->is_value_defined)
  {
    if (!is_current_data_descriptor
        || !ecma_op_same_value (property_desc_p->value,
                                ecma_get_named_data_property_value (current_p)))
    {
      is_every_field_in_desc_also_occurs_in_current_desc_with_same_value = false;
    }
  }

  if (property_desc_p->is_writable_defined)
  {
    if (!is_current_data_descriptor
        || property_desc_p->is_writable != ecma_is_property_writable (current_p))
    {
      is_every_field_in_desc_also_occurs_in_current_desc_with_same_value = false;
    }
  }

  if (property_desc_p->is_get_defined)
  {
    if (!is_current_accessor_descriptor
        || property_desc_p->get_p != ecma_get_named_accessor_property_getter (current_p))
    {
      is_every_field_in_desc_also_occurs_in_current_desc_with_same_value = false;
    }
  }

  if (property_desc_p->is_set_defined)
  {
    if (!is_current_accessor_descriptor
        || property_desc_p->set_p != ecma_get_named_accessor_property_setter (current_p))
    {
      is_every_field_in_desc_also_occurs_in_current_desc_with_same_value = false;
    }
  }

  if (property_desc_p->is_enumerable_defined)
  {
    if (property_desc_p->is_enumerable != ecma_is_property_enumerable (current_p))
    {
      is_every_field_in_desc_also_occurs_in_current_desc_with_same_value = false;
    }
  }

  if (property_desc_p->is_configurable_defined)
  {
    if (property_desc_p->is_configurable != ecma_is_property_configurable (current_p))
    {
      is_every_field_in_desc_also_occurs_in_current_desc_with_same_value = false;
    }
  }

  if (is_every_field_in_desc_also_occurs_in_current_desc_with_same_value)
  {
    return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
  }

  // 7.
  if (!ecma_is_property_configurable (current_p))
  {
    if (property_desc_p->is_configurable
        || (property_desc_p->is_enumerable_defined
            && property_desc_p->is_enumerable != ecma_is_property_enumerable (current_p)))
    {
      // a., b.
      return ecma_reject (is_throw);
    }
  }

  // 8.
  if (is_property_desc_generic_descriptor)
  {
    // no action required
  }
  else if (is_property_desc_data_descriptor != is_current_data_descriptor)
  {
    // 9.
    if (!ecma_is_property_configurable (current_p))
    {
      // a.
      return ecma_reject (is_throw);
    }

    bool was_enumerable = ecma_is_property_enumerable (current_p);
    bool was_configurable = ecma_is_property_configurable (current_p);

    ecma_delete_property (obj_p, current_p);

    if (is_current_data_descriptor)
    {
      // b.

      current_p = ecma_create_named_accessor_property (obj_p,
                                                       property_name_p,
                                                       NULL,
                                                       NULL,
                                                       was_enumerable,
                                                       was_configurable);
    }
    else
    {
      // c.

      current_p = ecma_create_named_data_property (obj_p,
                                                   property_name_p,
                                                   false,
                                                   was_enumerable,
                                                   was_configurable);
    }
  }
  else if (is_property_desc_data_descriptor && is_current_data_descriptor)
  {
    // 10.
    if (!ecma_is_property_configurable (current_p))
    {
      // a.
      if (!ecma_is_property_writable (current_p))
      {
        // i.
        if (property_desc_p->is_writable)
        {
          return ecma_reject (is_throw);
        }

        // ii.
        if (property_desc_p->is_value_defined
            && !ecma_op_same_value (property_desc_p->value,
                                    ecma_get_named_data_property_value (current_p)))
        {
          return ecma_reject (is_throw);
        }
      }
    }
  }
  else
  {
    JERRY_ASSERT (is_property_desc_accessor_descriptor && is_current_accessor_descriptor);

    // 11.

    if (!ecma_is_property_configurable (current_p))
    {
      // a.

      if ((property_desc_p->is_get_defined
           && property_desc_p->get_p != ecma_get_named_accessor_property_getter (current_p))
          || (property_desc_p->is_set_defined
              && property_desc_p->set_p != ecma_get_named_accessor_property_setter (current_p)))
      {
        // i., ii.
        return ecma_reject (is_throw);
      }
    }
  }

  // 12.
  if (property_desc_p->is_value_defined)
  {
    JERRY_ASSERT (current_p->type == ECMA_PROPERTY_NAMEDDATA);

    ecma_named_data_property_assign_value (obj_p, current_p, property_desc_p->value);
  }

  if (property_desc_p->is_writable_defined)
  {
    JERRY_ASSERT (current_p->type == ECMA_PROPERTY_NAMEDDATA);

    ecma_set_property_writable_attr (current_p, property_desc_p->is_writable);
  }

  if (property_desc_p->is_get_defined)
  {
    JERRY_ASSERT (current_p->type == ECMA_PROPERTY_NAMEDACCESSOR);

    ecma_set_named_accessor_property_getter (obj_p, current_p, property_desc_p->get_p);
  }

  if (property_desc_p->is_set_defined)
  {
    JERRY_ASSERT (current_p->type == ECMA_PROPERTY_NAMEDACCESSOR);

    ecma_set_named_accessor_property_setter (obj_p, current_p, property_desc_p->set_p);
  }

  if (property_desc_p->is_enumerable_defined)
  {
    ecma_set_property_enumerable_attr (current_p, property_desc_p->is_enumerable);
  }

  if (property_desc_p->is_configurable_defined)
  {
    ecma_set_property_configurable_attr (current_p, property_desc_p->is_configurable);
  }

  return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
} /* ecma_op_general_object_define_own_property */

/**
 * @}
 * @}
 */
