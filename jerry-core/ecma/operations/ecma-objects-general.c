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
 * \addtogroup ecmaobjectsinternalops ECMA objects' operations
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
    return ecma_raise_type_error (ECMA_ERR_MSG ("Invalid argument type."));
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

  /* 3., 4., 6., 7. */
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
    /* 1.b, 1.c, 1.d */
    return ecma_op_to_object (value);
  }
  else
  {
    /* 2. */
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
  ecma_object_t *obj_p = ecma_create_object (object_prototype_p, 0, ECMA_OBJECT_TYPE_GENERAL);

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

  /* 1. */
  ecma_property_ref_t property_ref;

  ecma_property_t property = ecma_op_object_get_own_property (obj_p,
                                                              property_name_p,
                                                              &property_ref,
                                                              ECMA_PROPERTY_GET_NO_OPTIONS);

  /* 2. */
  if (property == ECMA_PROPERTY_TYPE_NOT_FOUND && property != ECMA_PROPERTY_TYPE_NOT_FOUND_AND_STOP)
  {
    return ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
  }

  /* 3. */
  if (ecma_is_property_configurable (property))
  {
    /* a. */
    ecma_delete_property (obj_p, property_ref.value_p);

    /* b. */
    return ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
  }
  else if (is_throw)
  {
    /* 4. */
    return ecma_raise_type_error (ECMA_ERR_MSG ("Expected a configurable property."));
  }
  else
  {
    /* 5. */
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
    if (ecma_object_class_is (obj_p, LIT_MAGIC_STRING_DATE_UL))
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

  return ecma_raise_type_error (ECMA_ERR_MSG ("Invalid argument type in [[DefaultValue]]."));
} /* ecma_op_general_object_default_value */

/**
 * Special type for ecma_op_general_object_define_own_property.
 */
#define ECMA_PROPERTY_TYPE_GENERIC ECMA_PROPERTY_TYPE_SPECIAL

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
ecma_op_general_object_define_own_property (ecma_object_t *object_p, /**< the object */
                                            ecma_string_t *property_name_p, /**< property name */
                                            const ecma_property_descriptor_t *property_desc_p, /**< property
                                                                                                *   descriptor */
                                            bool is_throw) /**< flag that controls failure handling */
{
  JERRY_ASSERT (object_p != NULL
                && !ecma_is_lexical_environment (object_p));
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

  /* 1. */
  /* This #def just gets around the syntax/style checker... */
#define extended_property_ref_initialization { { 0 } , 0 }
  ecma_extended_property_ref_t ext_property_ref = extended_property_ref_initialization;
  ecma_property_t current_prop;

  current_prop = ecma_op_object_get_own_property (object_p,
                                                  property_name_p,
                                                  &ext_property_ref.property_ref,
                                                  ECMA_PROPERTY_GET_VALUE | ECMA_PROPERTY_GET_EXT_REFERENCE);

  if (current_prop == ECMA_PROPERTY_TYPE_NOT_FOUND || current_prop == ECMA_PROPERTY_TYPE_NOT_FOUND_AND_STOP)
  {
    /* 3. */
    if (!ecma_get_object_extensible (object_p))
    {
      /* 2. */
      return ecma_reject (is_throw);
    }

    /* 4. */

    if (property_desc_type != ECMA_PROPERTY_TYPE_NAMEDACCESSOR)
    {
      /* a. */

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

      ecma_property_value_t *new_prop_value_p = ecma_create_named_data_property (object_p,
                                                                                 property_name_p,
                                                                                 prop_attributes,
                                                                                 NULL);

      JERRY_ASSERT (property_desc_p->is_value_defined
                    || ecma_is_value_undefined (property_desc_p->value));

      new_prop_value_p->value = ecma_copy_value_if_not_object (property_desc_p->value);
    }
    else
    {
      /* b. */

      uint8_t prop_attributes = 0;

      if (property_desc_p->is_configurable)
      {
        prop_attributes = (uint8_t) (prop_attributes | ECMA_PROPERTY_FLAG_CONFIGURABLE);
      }
      if (property_desc_p->is_enumerable)
      {
        prop_attributes = (uint8_t) (prop_attributes | ECMA_PROPERTY_FLAG_ENUMERABLE);
      }

      ecma_create_named_accessor_property (object_p,
                                           property_name_p,
                                           property_desc_p->get_p,
                                           property_desc_p->set_p,
                                           prop_attributes,
                                           NULL);
    }

    return ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
  }

  /* 6. */
  ecma_property_types_t current_property_type = ECMA_PROPERTY_GET_TYPE (current_prop);
  const bool is_current_configurable = ecma_is_property_configurable (current_prop);

  JERRY_ASSERT (current_property_type == ECMA_PROPERTY_TYPE_NAMEDDATA
                || current_property_type == ECMA_PROPERTY_TYPE_NAMEDACCESSOR
                || current_property_type == ECMA_PROPERTY_TYPE_VIRTUAL);

  /* 7. a., b. */
  if (!is_current_configurable
      && (property_desc_p->is_configurable
          || (property_desc_p->is_enumerable_defined
              && (property_desc_p->is_enumerable != ecma_is_property_enumerable (current_prop)))))
  {
    if (current_property_type == ECMA_PROPERTY_TYPE_VIRTUAL)
    {
      ecma_free_value (ext_property_ref.property_ref.virtual_value);
    }
    return ecma_reject (is_throw);
  }

  if (current_property_type == ECMA_PROPERTY_TYPE_VIRTUAL)
  {
    JERRY_ASSERT (!is_current_configurable && !ecma_is_property_writable (current_prop));

    ecma_value_t result = ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);

    if (property_desc_type == ECMA_PROPERTY_TYPE_NAMEDACCESSOR
        || property_desc_p->is_writable
        || (property_desc_p->is_value_defined
            && !ecma_op_same_value (property_desc_p->value,
                                    ext_property_ref.property_ref.virtual_value)))
    {
      result = ecma_reject (is_throw);
    }

    ecma_free_value (ext_property_ref.property_ref.virtual_value);
    return result;
  }

  /* 8. */
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
        /* 10. a. i. & ii. */
        if (!ecma_is_property_writable (current_prop)
            && (property_desc_p->is_writable
                || (property_desc_p->is_value_defined
                    && !ecma_op_same_value (property_desc_p->value,
                                            ext_property_ref.property_ref.value_p->value))))
        {
          return ecma_reject (is_throw);
        }
      }
      else
      {
        /* 11. */

        /* a. */
        ecma_property_value_t *value_p = ext_property_ref.property_ref.value_p;

        if ((property_desc_p->is_get_defined
             && property_desc_p->get_p != ecma_get_named_accessor_property_getter (value_p))
            || (property_desc_p->is_set_defined
                && property_desc_p->set_p != ecma_get_named_accessor_property_setter (value_p)))
        {
          /* i., ii. */
          return ecma_reject (is_throw);
        }
      }
    }
  }
  else
  {
    /* 9. */
    if (!is_current_configurable)
    {
      /* a. */
      return ecma_reject (is_throw);
    }

    ecma_property_value_t *value_p = ext_property_ref.property_ref.value_p;

    if (property_desc_type == ECMA_PROPERTY_TYPE_NAMEDACCESSOR)
    {
      JERRY_ASSERT (current_property_type == ECMA_PROPERTY_TYPE_NAMEDDATA);
      ecma_free_value_if_not_object (value_p->value);

#ifdef JERRY_CPOINTER_32_BIT
      ecma_getter_setter_pointers_t *getter_setter_pair_p;
      getter_setter_pair_p = jmem_pools_alloc (sizeof (ecma_getter_setter_pointers_t));
      getter_setter_pair_p->getter_p = JMEM_CP_NULL;
      getter_setter_pair_p->setter_p = JMEM_CP_NULL;
      ECMA_SET_POINTER (value_p->getter_setter_pair_cp, getter_setter_pair_p);
#else /* !JERRY_CPOINTER_32_BIT */
      value_p->getter_setter_pair.getter_p = JMEM_CP_NULL;
      value_p->getter_setter_pair.setter_p = JMEM_CP_NULL;
#endif /* JERRY_CPOINTER_32_BIT */
    }
    else
    {
      JERRY_ASSERT (current_property_type == ECMA_PROPERTY_TYPE_NAMEDACCESSOR);
#ifdef JERRY_CPOINTER_32_BIT
      ecma_getter_setter_pointers_t *getter_setter_pair_p;
      getter_setter_pair_p = ECMA_GET_POINTER (ecma_getter_setter_pointers_t,
                                               value_p->getter_setter_pair_cp);
      jmem_pools_free (getter_setter_pair_p, sizeof (ecma_getter_setter_pointers_t));
#endif /* JERRY_CPOINTER_32_BIT */
      value_p->value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }

    /* Update flags */
    ecma_property_t prop_flags = *(ext_property_ref.property_p);
    prop_flags = (ecma_property_t) (prop_flags & ~(ECMA_PROPERTY_TYPE_MASK | ECMA_PROPERTY_FLAG_WRITABLE));
    prop_flags = (ecma_property_t) (prop_flags | property_desc_type);
    *(ext_property_ref.property_p) = prop_flags;
  }

  /* 12. */
  if (property_desc_type == ECMA_PROPERTY_TYPE_NAMEDDATA)
  {
    JERRY_ASSERT (ECMA_PROPERTY_GET_TYPE (*ext_property_ref.property_p) == ECMA_PROPERTY_TYPE_NAMEDDATA);

    if (property_desc_p->is_value_defined)
    {
      ecma_named_data_property_assign_value (object_p,
                                             ext_property_ref.property_ref.value_p,
                                             property_desc_p->value);
    }

    if (property_desc_p->is_writable_defined)
    {
      ecma_set_property_writable_attr (ext_property_ref.property_p, property_desc_p->is_writable);
    }
  }
  else if (property_desc_type == ECMA_PROPERTY_TYPE_NAMEDACCESSOR)
  {
    JERRY_ASSERT (ECMA_PROPERTY_GET_TYPE (*ext_property_ref.property_p) == ECMA_PROPERTY_TYPE_NAMEDACCESSOR);

    if (property_desc_p->is_get_defined)
    {
      ecma_set_named_accessor_property_getter (object_p,
                                               ext_property_ref.property_ref.value_p,
                                               property_desc_p->get_p);
    }

    if (property_desc_p->is_set_defined)
    {
      ecma_set_named_accessor_property_setter (object_p,
                                               ext_property_ref.property_ref.value_p,
                                               property_desc_p->set_p);
    }
  }

  if (property_desc_p->is_enumerable_defined)
  {
    ecma_set_property_enumerable_attr (ext_property_ref.property_p, property_desc_p->is_enumerable);
  }

  if (property_desc_p->is_configurable_defined)
  {
    ecma_set_property_configurable_attr (ext_property_ref.property_p, property_desc_p->is_configurable);
  }

  return ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
} /* ecma_op_general_object_define_own_property */

#undef ECMA_PROPERTY_TYPE_GENERIC

/**
 * @}
 * @}
 */
