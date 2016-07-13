/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged.
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
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_reject (bool is_throw) /**< Throw flag */
{
  if (is_throw)
  {
    return ecma_raise_type_error (ECMA_ERR_MSG (""));
  }
  else
  {
    return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE);
  }
} /* ecma_reject */

/**
 * 'Object' object creation operation with no arguments.
 *
 * See also: ECMA-262 v5, 15.2.2.1
 *
 * @return pointer to newly created 'Object' object
 */
ecma_object_t *
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
ecma_value_t
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

    return ecma_make_object_value (obj_p);
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
ecma_object_t *
ecma_op_create_object_object_noarg_and_set_prototype (ecma_object_t *object_prototype_p) /**< pointer to prototype of
                                                                                              the object
                                                                                              (can be NULL) */
{
  ecma_object_t *obj_p = ecma_create_object (object_prototype_p, false, true, ECMA_OBJECT_TYPE_GENERAL);

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
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_general_object_get (ecma_object_t *obj_p, /**< the object */
                            ecma_string_t *property_name_p) /**< property name */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT (property_name_p != NULL);

  // 1.
  const ecma_property_t *prop_p = ecma_op_object_get_property (obj_p, property_name_p);

  // 2.
  if (prop_p == NULL)
  {
    return ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
  }

  // 3.
  if (ECMA_PROPERTY_GET_TYPE (prop_p) == ECMA_PROPERTY_TYPE_NAMEDDATA)
  {
    return ecma_copy_value (ecma_get_named_data_property_value (prop_p));
  }
  else
  {
    // 4.
    ecma_object_t *getter_p = ecma_get_named_accessor_property_getter (prop_p);

    // 5.
    if (getter_p == NULL)
    {
      return ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }
    else
    {
      return ecma_op_function_call (getter_p,
                                    ecma_make_object_value (obj_p),
                                    NULL,
                                    0);
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
ecma_property_t *
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
ecma_property_t *
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
 *          Also incorporates [[CanPut]] ECMA-262 v5, 8.12.4
 *
 * @return ecma value
 *         The returned value must be freed with ecma_free_value.
 *
 *         Returns with ECMA_SIMPLE_VALUE_TRUE if the operation is
 *         successful. Otherwise it returns with an error object
 *         or ECMA_SIMPLE_VALUE_FALSE.
 *
 *         Note: even if is_throw is false, the setter can throw an
 *         error, and this function returns with that error.
 */
ecma_value_t
ecma_op_general_object_put (ecma_object_t *obj_p, /**< the object */
                            ecma_string_t *property_name_p, /**< property name */
                            ecma_value_t value, /**< ecma value */
                            bool is_throw) /**< flag that controls failure handling */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT (property_name_p != NULL);

  ecma_object_t *setter_p = NULL;

  ecma_property_t *prop_p = ecma_op_object_get_own_property (obj_p, property_name_p);

  if (prop_p != NULL)
  {
    if (ECMA_PROPERTY_GET_TYPE (prop_p) == ECMA_PROPERTY_TYPE_NAMEDDATA)
    {
      if (ecma_is_property_writable (prop_p))
      {
        const ecma_object_type_t type = ecma_get_object_type (obj_p);

        if (type == ECMA_OBJECT_TYPE_ARGUMENTS
            || (type == ECMA_OBJECT_TYPE_ARRAY && ecma_string_is_length (property_name_p)))
        {
          /* These cases cannot be optimized. */
          ecma_property_descriptor_t value_desc = ecma_make_empty_property_descriptor ();

          value_desc.is_value_defined = true;
          value_desc.value = value;

          return ecma_op_object_define_own_property (obj_p,
                                                     property_name_p,
                                                     &value_desc,
                                                     is_throw);
        }

        /* There is no need for special casing arrays here because changing the
         * value of an existing property never changes the length of an array. */
        ecma_named_data_property_assign_value (obj_p, prop_p, value);
        return ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
      }
    }
    else
    {
      JERRY_ASSERT (ECMA_PROPERTY_GET_TYPE (prop_p) == ECMA_PROPERTY_TYPE_NAMEDACCESSOR);

      setter_p = ecma_get_named_accessor_property_setter (prop_p);
    }
  }
  else
  {
    ecma_object_t *proto_p = ecma_get_object_prototype (obj_p);
    bool create_new_property = true;

    if (proto_p != NULL)
    {
      ecma_property_t *inherited_prop_p = ecma_op_object_get_property (proto_p, property_name_p);

      if (inherited_prop_p != NULL)
      {
        if (ECMA_PROPERTY_GET_TYPE (inherited_prop_p) == ECMA_PROPERTY_TYPE_NAMEDACCESSOR)
        {
          setter_p = ecma_get_named_accessor_property_setter (inherited_prop_p);
          create_new_property = false;
        }
        else
        {
          create_new_property = ecma_is_property_writable (inherited_prop_p);
        }
      }
    }

    if (create_new_property
        && ecma_get_object_extensible (obj_p))
    {
      const ecma_object_type_t type = ecma_get_object_type (obj_p);

      if (type == ECMA_OBJECT_TYPE_ARGUMENTS)
      {
        return ecma_builtin_helper_def_prop (obj_p,
                                             property_name_p,
                                             value,
                                             true, /* Writable */
                                             true, /* Enumerable */
                                             true, /* Configurable */
                                             is_throw); /* Failure handling */
      }

      uint32_t index;

      if (type == ECMA_OBJECT_TYPE_ARRAY
          && ecma_string_get_array_index (property_name_p, &index))
      {
        /* Since the length of an array is a non-configurable named data
         * property, the prop_p must be a non-NULL pointer for all arrays. */

        JERRY_ASSERT (!ecma_string_is_length (property_name_p));

        ecma_string_t magic_string_length;
        ecma_init_ecma_length_string (&magic_string_length);

        ecma_property_t *len_prop_p = ecma_op_object_get_own_property (obj_p, &magic_string_length);

        JERRY_ASSERT (len_prop_p != NULL
                      && ECMA_PROPERTY_GET_TYPE (len_prop_p) == ECMA_PROPERTY_TYPE_NAMEDDATA);

        uint32_t old_len = ecma_get_uint32_from_value (ecma_get_named_data_property_value (len_prop_p));

        if (index < UINT32_MAX
            && index >= old_len)
        {
          if (!ecma_is_property_writable (len_prop_p))
          {
            return ecma_reject (is_throw);
          }

          ecma_property_value_t *len_prop_value_p = ECMA_PROPERTY_VALUE_PTR (len_prop_p);
          ecma_value_assign_uint32 (&len_prop_value_p->value, index + 1);
        }
      }

      ecma_property_t *new_prop_p;
      new_prop_p = ecma_create_named_data_property (obj_p,
                                                    property_name_p,
                                                    ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);

      JERRY_ASSERT (ecma_is_value_undefined (ecma_get_named_data_property_value (new_prop_p)));
      ecma_set_named_data_property_value (new_prop_p, ecma_copy_value_if_not_object (value));
      return ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
    }
  }

  if (setter_p == NULL)
  {
    return ecma_reject (is_throw);
  }

  ecma_value_t ret_value = ecma_op_function_call (setter_p,
                                                  ecma_make_object_value (obj_p),
                                                  &value,
                                                  1);

  if (!ECMA_IS_VALUE_ERROR (ret_value))
  {
    ecma_fast_free_value (ret_value);
    ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
  }

  return ret_value;
} /* ecma_op_general_object_put */

/**
 * [[Delete]] ecma general object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *          ECMA-262 v5, 8.12.7
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
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
    return ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
  }

  // 3.
  if (ecma_is_property_configurable (desc_p))
  {
    // a.
    ecma_delete_property (obj_p, desc_p);

    // b.
    return ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
  }
  else if (is_throw)
  {
    // 4.
    return ecma_raise_type_error (ECMA_ERR_MSG (""));
  }
  else
  {
    // 5.
    return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE);
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
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
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

    ecma_value_t function_value_get_completion = ecma_op_object_get (obj_p, function_name_p);

    ecma_deref_ecma_string (function_name_p);

    if (ECMA_IS_VALUE_ERROR (function_value_get_completion))
    {
      return function_value_get_completion;
    }

    ecma_value_t call_completion = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

    if (ecma_op_is_callable (function_value_get_completion))
    {
      ecma_object_t *func_obj_p = ecma_get_object_from_value (function_value_get_completion);

      call_completion = ecma_op_function_call (func_obj_p,
                                               ecma_make_object_value (obj_p),
                                               NULL,
                                               0);
    }

    ecma_free_value (function_value_get_completion);

    if (ECMA_IS_VALUE_ERROR (call_completion)
        || (!ecma_is_value_empty (call_completion)
           && !ecma_is_value_object (call_completion)))
    {
      return call_completion;
    }

    ecma_free_value (call_completion);
  }

  return ecma_raise_type_error (ECMA_ERR_MSG (""));
} /* ecma_op_general_object_default_value */

/**
 * Special type for ecma_op_general_object_define_own_property.
 */
#define ECMA_PROPERTY_TYPE_GENERIC ECMA_PROPERTY_TYPE_DELETED

/**
 * [[DefineOwnProperty]] ecma general object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *          ECMA-262 v5, 8.12.9
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_general_object_define_own_property (ecma_object_t *obj_p, /**< the object */
                                            ecma_string_t *property_name_p, /**< property name */
                                            const ecma_property_descriptor_t *property_desc_p, /**< property
                                                                                                *   descriptor */
                                            bool is_throw) /**< flag that controls failure handling */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT (property_name_p != NULL);

  ecma_property_types_t property_desc_type = ECMA_PROPERTY_TYPE_GENERIC;

  if (property_desc_p->is_value_defined
      || property_desc_p->is_writable_defined)
  {
    /* A property descriptor cannot be both named data and named accessor. */
    JERRY_ASSERT (!property_desc_p->is_get_defined
                  && !property_desc_p->is_set_defined);
    property_desc_type = ECMA_PROPERTY_TYPE_NAMEDDATA;
  }
  else if (property_desc_p->is_get_defined
           || property_desc_p->is_set_defined)
  {
    property_desc_type = ECMA_PROPERTY_TYPE_NAMEDACCESSOR;
  }

  /* These three asserts ensures that a new property is created with the appropriate default flags.
   * E.g. if is_configurable_defined is false, the newly created property must be non-configurable. */
  JERRY_ASSERT (property_desc_p->is_configurable_defined || !property_desc_p->is_configurable);
  JERRY_ASSERT (property_desc_p->is_enumerable_defined || !property_desc_p->is_enumerable);
  JERRY_ASSERT (property_desc_p->is_writable_defined || !property_desc_p->is_writable);

  // 1.
  ecma_property_t *current_p = ecma_op_object_get_own_property (obj_p, property_name_p);

  if (current_p == NULL)
  {
    // 3.
    if (!ecma_get_object_extensible (obj_p))
    {
      // 2.
      return ecma_reject (is_throw);
    }

    // 4.

    if (property_desc_type != ECMA_PROPERTY_TYPE_NAMEDACCESSOR)
    {
      // a.

      JERRY_ASSERT (property_desc_type == ECMA_PROPERTY_TYPE_GENERIC
                    || property_desc_type == ECMA_PROPERTY_TYPE_NAMEDDATA);

      uint8_t prop_attributes = 0;

      if (property_desc_p->is_configurable)
      {
        prop_attributes = (uint8_t) (prop_attributes | ECMA_PROPERTY_FLAG_CONFIGURABLE);
      }
      if (property_desc_p->is_enumerable)
      {
        prop_attributes = (uint8_t) (prop_attributes | ECMA_PROPERTY_FLAG_ENUMERABLE);
      }
      if (property_desc_p->is_writable)
      {
        prop_attributes = (uint8_t) (prop_attributes | ECMA_PROPERTY_FLAG_WRITABLE);
      }

      ecma_property_t *new_prop_p = ecma_create_named_data_property (obj_p,
                                                                     property_name_p,
                                                                     prop_attributes);

      JERRY_ASSERT (property_desc_p->is_value_defined
                    || ecma_is_value_undefined (property_desc_p->value));

      ecma_named_data_property_assign_value (obj_p, new_prop_p, property_desc_p->value);
    }
    else
    {
      // b.

      uint8_t prop_attributes = 0;

      if (property_desc_p->is_configurable)
      {
        prop_attributes = (uint8_t) (prop_attributes | ECMA_PROPERTY_FLAG_CONFIGURABLE);
      }
      if (property_desc_p->is_enumerable)
      {
        prop_attributes = (uint8_t) (prop_attributes | ECMA_PROPERTY_FLAG_ENUMERABLE);
      }

      ecma_create_named_accessor_property (obj_p,
                                           property_name_p,
                                           property_desc_p->get_p,
                                           property_desc_p->set_p,
                                           prop_attributes);
    }

    return ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
  }

  // 6.
  ecma_property_types_t current_property_type = ECMA_PROPERTY_GET_TYPE (current_p);
  const bool is_current_configurable = ecma_is_property_configurable (current_p);

  JERRY_ASSERT (current_property_type == ECMA_PROPERTY_TYPE_NAMEDDATA
                || current_property_type == ECMA_PROPERTY_TYPE_NAMEDACCESSOR);

  // 7. a., b.
  if (!is_current_configurable
      && (property_desc_p->is_configurable
          || (property_desc_p->is_enumerable_defined
              && (property_desc_p->is_enumerable != ecma_is_property_enumerable (current_p)))))
  {
    return ecma_reject (is_throw);
  }

  // 8.
  if (property_desc_type == ECMA_PROPERTY_TYPE_GENERIC)
  {
    /* No action required. */
  }
  else if (likely (property_desc_type == current_property_type))
  {
    /* If property is configurable, there is no need for checks. */
    if (unlikely (!is_current_configurable))
    {
      if (property_desc_type == ECMA_PROPERTY_TYPE_NAMEDDATA)
      {
        // 10. a. i. & ii.
        if (!ecma_is_property_writable (current_p)
            && (property_desc_p->is_writable
                || (property_desc_p->is_value_defined
                    && !ecma_op_same_value (property_desc_p->value,
                                            ecma_get_named_data_property_value (current_p)))))
        {
          return ecma_reject (is_throw);
        }
      }
      else
      {
        // 11.

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
  }
  else
  {
    // 9.
    if (!is_current_configurable)
    {
      // a.
      return ecma_reject (is_throw);
    }

    /* The following implementation can be optimized by directly overwriting
     * the fields of current_p if this code path is performance critical. */
    uint8_t prop_attributes = ECMA_PROPERTY_FLAG_CONFIGURABLE;

    if (ecma_is_property_enumerable (current_p))
    {
      prop_attributes = (uint8_t) (prop_attributes | ECMA_PROPERTY_FLAG_ENUMERABLE);
    }

    ecma_delete_property (obj_p, current_p);

    if (property_desc_type == ECMA_PROPERTY_TYPE_NAMEDACCESSOR)
    {
      // b.

      current_p = ecma_create_named_accessor_property (obj_p,
                                                       property_name_p,
                                                       NULL,
                                                       NULL,
                                                       prop_attributes);
    }
    else
    {
      // c.

      current_p = ecma_create_named_data_property (obj_p,
                                                   property_name_p,
                                                   prop_attributes);
    }
  }

  // 12.
  if (property_desc_type == ECMA_PROPERTY_TYPE_NAMEDDATA)
  {
    JERRY_ASSERT (ECMA_PROPERTY_GET_TYPE (current_p) == ECMA_PROPERTY_TYPE_NAMEDDATA);

    if (property_desc_p->is_value_defined)
    {
      ecma_named_data_property_assign_value (obj_p, current_p, property_desc_p->value);
    }

    if (property_desc_p->is_writable_defined)
    {
      ecma_set_property_writable_attr (current_p, property_desc_p->is_writable);
    }
  }
  else if (property_desc_type == ECMA_PROPERTY_TYPE_NAMEDACCESSOR)
  {
    JERRY_ASSERT (ECMA_PROPERTY_GET_TYPE (current_p) == ECMA_PROPERTY_TYPE_NAMEDACCESSOR);

    if (property_desc_p->is_get_defined)
    {
      ecma_set_named_accessor_property_getter (obj_p, current_p, property_desc_p->get_p);
    }

    if (property_desc_p->is_set_defined)
    {
      ecma_set_named_accessor_property_setter (obj_p, current_p, property_desc_p->set_p);
    }
  }

  if (property_desc_p->is_enumerable_defined)
  {
    ecma_set_property_enumerable_attr (current_p, property_desc_p->is_enumerable);
  }

  if (property_desc_p->is_configurable_defined)
  {
    ecma_set_property_configurable_attr (current_p, property_desc_p->is_configurable);
  }

  return ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
} /* ecma_op_general_object_define_own_property */

#undef ECMA_PROPERTY_TYPE_GENERIC

/**
 * @}
 * @}
 */
