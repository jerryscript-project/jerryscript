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

#include "ecma-objects-general.h"

#include "ecma-arguments-object.h"
#include "ecma-array-object.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-proxy-object.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaobjectsinternalops ECMA objects' operations
 * @{
 */

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
  return ecma_create_object (ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE), 0, ECMA_OBJECT_TYPE_GENERAL);
} /* ecma_op_create_object_object_noarg */

/**
 * Property invocation order during [[DefaultValue]] operation with string hint
 */
static const lit_magic_string_id_t to_primitive_string_hint_method_names[2] = {
  LIT_MAGIC_STRING_TO_STRING_UL, /**< toString operation */
  LIT_MAGIC_STRING_VALUE_OF_UL, /**< valueOf operation */
};

/**
 * Property invocation order during [[DefaultValue]] operation with non string hint
 */
static const lit_magic_string_id_t to_primitive_non_string_hint_method_names[2] = {
  LIT_MAGIC_STRING_VALUE_OF_UL, /**< valueOf operation */
  LIT_MAGIC_STRING_TO_STRING_UL, /**< toString operation */
};

#if JERRY_ESNEXT
/**
 * Hints for the ecma general object's toPrimitve operation
 */
static const lit_magic_string_id_t hints[3] = {
  LIT_MAGIC_STRING_DEFAULT, /**< "default" hint */
  LIT_MAGIC_STRING_NUMBER, /**< "number" hint */
  LIT_MAGIC_STRING_STRING, /**< "string" hint */
};
#endif /* JERRY_ESNEXT */

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
  JERRY_ASSERT (obj_p != NULL && !ecma_is_lexical_environment (obj_p));

#if JERRY_ESNEXT
  ecma_value_t obj_value = ecma_make_object_value (obj_p);

  ecma_value_t exotic_to_prim = ecma_op_get_method_by_symbol_id (obj_value, LIT_GLOBAL_SYMBOL_TO_PRIMITIVE);

  if (ECMA_IS_VALUE_ERROR (exotic_to_prim))
  {
    return exotic_to_prim;
  }

  if (!ecma_is_value_undefined (exotic_to_prim))
  {
    ecma_object_t *call_func_p = ecma_get_object_from_value (exotic_to_prim);
    ecma_value_t argument = ecma_make_magic_string_value (hints[hint]);

    ecma_value_t result = ecma_internal_method_call (call_func_p, obj_value, &argument, 1);

    ecma_free_value (exotic_to_prim);

    if (ECMA_IS_VALUE_ERROR (result) || !ecma_is_value_object (result))
    {
      return result;
    }

    ecma_free_value (result);

    return ecma_raise_type_error (ECMA_ERR_RESULT_OF_DEFAULTVALUE_IS_INVALID);
  }

  ecma_free_value (exotic_to_prim);

  if (hint == ECMA_PREFERRED_TYPE_NO)
  {
    hint = ECMA_PREFERRED_TYPE_NUMBER;
  }
#else /* !JERRY_ESNEXT */
  if (hint == ECMA_PREFERRED_TYPE_NO)
  {
    hint = ECMA_PREFERRED_TYPE_NUMBER;

#if JERRY_BUILTIN_DATE
    if (ecma_object_class_is (obj_p, ECMA_OBJECT_CLASS_DATE))
    {
      hint = ECMA_PREFERRED_TYPE_STRING;
    }
#endif /* JERRY_BUILTIN_DATE */
  }
#endif /* JERRY_ESNEXT */

  return ecma_op_general_object_ordinary_value (obj_p, hint);
} /* ecma_op_general_object_default_value */

/**
 * Ecma general object's OrdinaryToPrimitive operation
 *
 * See also:
 *          ECMA-262 v6 7.1.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_general_object_ordinary_value (ecma_object_t *obj_p, /**< the object */
                                       ecma_preferred_type_hint_t hint) /**< hint on preferred result type */
{
  const lit_magic_string_id_t *function_name_ids_p =
    (hint == ECMA_PREFERRED_TYPE_STRING ? to_primitive_string_hint_method_names
                                        : to_primitive_non_string_hint_method_names);

  for (uint32_t i = 0; i < 2; i++)
  {
    ecma_value_t function_value = ecma_op_object_get_by_magic_id (obj_p, function_name_ids_p[i]);

    if (ECMA_IS_VALUE_ERROR (function_value))
    {
      return function_value;
    }

    ecma_value_t call_completion = ECMA_VALUE_EMPTY;

    if (ecma_op_is_callable (function_value))
    {
      ecma_object_t *func_obj_p = ecma_get_object_from_value (function_value);

      call_completion = ecma_internal_method_call (func_obj_p, ecma_make_object_value (obj_p), NULL, 0);
    }

    ecma_free_value (function_value);

    if (ECMA_IS_VALUE_ERROR (call_completion)
        || (!ecma_is_value_empty (call_completion) && !ecma_is_value_object (call_completion)))
    {
      return call_completion;
    }

    ecma_free_value (call_completion);
  }

  return ecma_raise_type_error (ECMA_ERR_RESULT_OF_DEFAULTVALUE_IS_INVALID);
} /* ecma_op_general_object_ordinary_value */

/**
 * ValidateAndApplyPropertyDescriptor abstract operation
 *
 * See also:
 *          ECMAScript v12, 10.1.6.3
 *
 * @return bool
 */
bool
ecma_op_validate_and_apply_property_descriptor (ecma_object_t *object_p, /**< object */
                                                ecma_string_t *prop_name_p, /**< property name */
                                                const ecma_property_descriptor_t *desc_p, /**< target descriptor */
                                                const ecma_property_descriptor_t *current_p, /**< current property */
                                                bool is_extensible) /**< true - if target object is extensible
                                                                         false - otherwise */
{
  JERRY_ASSERT (desc_p != NULL);
  JERRY_ASSERT (object_p == NULL || !ECMA_OBJECT_IS_PROXY (object_p));

  const uint32_t accessor_desc_flags = (JERRY_PROP_IS_SET_DEFINED | JERRY_PROP_IS_GET_DEFINED);
  const uint32_t data_desc_flags = (JERRY_PROP_IS_VALUE_DEFINED | JERRY_PROP_IS_WRITABLE_DEFINED);

  bool desc_is_accessor = (desc_p->flags & accessor_desc_flags) != 0;
  bool desc_is_data = (desc_p->flags & data_desc_flags) != 0;

  /* 2. */
  if (!ecma_property_descriptor_found (current_p))
  {
    /* 2.a */
    if (!is_extensible)
    {
      return false;
    }

    if (object_p != NULL)
    {
      uint8_t prop_attributes =
        (uint8_t) (desc_p->flags & (JERRY_PROP_IS_CONFIGURABLE | JERRY_PROP_IS_ENUMERABLE | JERRY_PROP_IS_WRITABLE));

      /* 2.c */
      if (desc_is_data || !desc_is_accessor)
      {
        /* 2.c.i */
        ecma_property_value_t *value_p = ecma_create_named_data_property (object_p, prop_name_p, prop_attributes, NULL);

        value_p->value = ecma_copy_value_if_not_object (desc_p->value);
      }
      /* 2.d */
      else
      {
        /* 2.d.i */
        JERRY_ASSERT (desc_is_accessor);

        ecma_object_t *getter_p = ecma_property_descriptor_accessor_getter (desc_p);
        ecma_object_t *setter_p = ecma_property_descriptor_accessor_setter (desc_p);
        /* 2.d.ii */
        ecma_create_named_accessor_property (object_p, prop_name_p, getter_p, setter_p, prop_attributes, NULL);
      }
    }

    return true;
  }

  /* 3. */
  if (desc_p->flags == 0)
  {
    return true;
  }

  bool target_is_configurable = ((desc_p->flags & (JERRY_PROP_IS_CONFIGURABLE_DEFINED | JERRY_PROP_IS_CONFIGURABLE))
                                 == (JERRY_PROP_IS_CONFIGURABLE_DEFINED | JERRY_PROP_IS_CONFIGURABLE));

  /* 4. */
  if (!ecma_property_descriptor_is_configurable (current_p))
  {
    if (target_is_configurable)
    {
      return false;
    }

    if ((desc_p->flags & JERRY_PROP_IS_ENUMERABLE_DEFINED)
        && (ecma_property_descriptor_is_enumerable (current_p) != !!(desc_p->flags & JERRY_PROP_IS_ENUMERABLE)))
    {
      return false;
    }
  }

  bool desc_is_generic = !desc_is_data && !desc_is_accessor;
  bool current_is_virtual = (current_p->flags & ECMA_PROP_DESC_VIRTUAL);
  bool current_is_data = ecma_property_descriptor_is_data_descriptor (current_p);

  ecma_property_value_t *prop_value_p = ECMA_PROPERTY_VALUE_PTR (current_p->u.property_p);

  /* 5. */
  if (desc_is_generic)
  {
    if (object_p != NULL && !current_is_virtual)
    {
      ecma_set_property_configurable_and_enumerable_flags (current_p->u.property_p, desc_p->flags);
    }

    return true;
  }

  /* 6. */
  if (current_is_data != desc_is_data)
  {
    if (current_is_virtual)
    {
      return false;
    }

    /* 6.a */
    if (!ecma_property_descriptor_is_configurable (current_p))
    {
      return false;
    }

    if (object_p != NULL)
    {
      /* 6.b */
      if (current_is_data)
      {
        ecma_object_t *getter_p = ecma_property_descriptor_accessor_getter (desc_p);
        ecma_object_t *setter_p = ecma_property_descriptor_accessor_setter (desc_p);

#if JERRY_CPOINTER_32_BIT
        ecma_getter_setter_pointers_t *getter_setter_pair_p = jmem_pools_alloc (sizeof (ecma_getter_setter_pointers_t));
#else /* !JERRY_CPOINTER_32_BIT */
        ecma_getter_setter_pointers_t *getter_setter_pair_p = &prop_value_p->getter_setter_pair;
#endif /* JERRY_CPOINTER_32_BIT */

        ecma_free_value_if_not_object (prop_value_p->value);
        *current_p->u.property_p =
          (uint8_t) (*current_p->u.property_p & ~(ECMA_PROPERTY_FLAG_DATA | ECMA_PROPERTY_FLAG_WRITABLE));

#if JERRY_CPOINTER_32_BIT
        ECMA_SET_NON_NULL_POINTER (prop_value_p->getter_setter_pair_cp, getter_setter_pair_p);
#endif /* JERRY_CPOINTER_32_BIT */

        ECMA_SET_POINTER (getter_setter_pair_p->getter_cp, getter_p);
        ECMA_SET_POINTER (getter_setter_pair_p->setter_cp, setter_p);
      }
      else
      {
        /* 6.c */
        ecma_value_t data_desc_value = ecma_copy_value_if_not_object (desc_p->value);
#if JERRY_CPOINTER_32_BIT
        ecma_getter_setter_pointers_t *getter_setter_pair_p =
          ECMA_GET_NON_NULL_POINTER (ecma_getter_setter_pointers_t, prop_value_p->getter_setter_pair_cp);
        jmem_pools_free (getter_setter_pair_p, sizeof (ecma_getter_setter_pointers_t));
#endif /* JERRY_CPOINTER_32_BIT */

        *current_p->u.property_p |= ECMA_PROPERTY_FLAG_DATA;
        prop_value_p->value = data_desc_value;

        if (desc_p->flags & JERRY_PROP_IS_WRITABLE_DEFINED)
        {
          ecma_set_property_writable_attr (current_p->u.property_p, desc_p->flags & JERRY_PROP_IS_WRITABLE);
        }
      }

      ecma_set_property_configurable_and_enumerable_flags (current_p->u.property_p, desc_p->flags);
      return true;
    }

    return true;
  }

  /* 7. */
  if (current_is_data)
  {
    /* 7.a */
    if (!ecma_property_descriptor_is_configurable (current_p) && !ecma_property_descriptor_is_writable (current_p))
    {
      bool target_is_writable = ((desc_p->flags & (JERRY_PROP_IS_WRITABLE_DEFINED | JERRY_PROP_IS_WRITABLE))
                                 == (JERRY_PROP_IS_WRITABLE_DEFINED | JERRY_PROP_IS_WRITABLE));
      /* 7.a.i */
      if (target_is_writable
          || ((desc_p->flags & JERRY_PROP_IS_WRITABLE_DEFINED)
              && ((desc_p->flags ^ (current_p->flags)) & JERRY_PROP_IS_WRITABLE) != 0))

      {
        return false;
      }

      /* 7.a.ii */
      if ((desc_p->flags & JERRY_PROP_IS_VALUE_DEFINED)
          && !ecma_op_same_value (desc_p->value, ecma_property_descriptor_value (current_p)))
      {
        return false;
      }
    }

    if (object_p != NULL && !current_is_virtual)
    {
      if (desc_p->flags & JERRY_PROP_IS_VALUE_DEFINED)
      {
        ecma_free_value_if_not_object (prop_value_p->value);
        prop_value_p->value = ecma_copy_value_if_not_object (desc_p->value);
      }

      if (desc_p->flags & JERRY_PROP_IS_WRITABLE_DEFINED)
      {
        ecma_set_property_writable_attr (current_p->u.property_p, desc_p->flags & JERRY_PROP_IS_WRITABLE);
      }

      ecma_set_property_configurable_and_enumerable_flags (current_p->u.property_p, desc_p->flags);
    }

    return true;
  }

  /* 8.a */
  JERRY_ASSERT (!current_is_data);
  JERRY_ASSERT ((desc_p->flags & (JERRY_PROP_IS_GET_DEFINED | JERRY_PROP_IS_SET_DEFINED)) != 0);

  ecma_object_t *desc_getter_p = ecma_property_descriptor_accessor_getter (desc_p);
  ecma_object_t *desc_setter_p = ecma_property_descriptor_accessor_setter (desc_p);

  /* 8.b */
  if (!ecma_property_descriptor_is_configurable (current_p))
  {
    ecma_object_t *current_getter_p = ecma_property_descriptor_accessor_getter (current_p);
    ecma_object_t *current_setter_p = ecma_property_descriptor_accessor_setter (current_p);

    /* 8.b.i */
    if ((desc_p->flags & JERRY_PROP_IS_SET_DEFINED) && desc_setter_p != current_setter_p)
    {
      return false;
    }

    /* 8.b.ii */
    if ((desc_p->flags & JERRY_PROP_IS_GET_DEFINED) && desc_getter_p != current_getter_p)
    {
      return false;
    }
  }

  if (object_p != NULL)
  {
#if JERRY_CPOINTER_32_BIT
    ecma_getter_setter_pointers_t *getter_setter_pair_p =
      ECMA_GET_NON_NULL_POINTER (ecma_getter_setter_pointers_t, prop_value_p->getter_setter_pair_cp);
    ECMA_SET_NON_NULL_POINTER (prop_value_p->getter_setter_pair_cp, getter_setter_pair_p);
#else /* !JERRY_CPOINTER_32_BIT */
    ecma_getter_setter_pointers_t *getter_setter_pair_p = &prop_value_p->getter_setter_pair;
#endif /* JERRY_CPOINTER_32_BIT */

    if (desc_p->flags & JERRY_PROP_IS_GET_DEFINED)
    {
      ECMA_SET_POINTER (getter_setter_pair_p->getter_cp, desc_getter_p);
    }

    if (desc_p->flags & JERRY_PROP_IS_SET_DEFINED)
    {
      ECMA_SET_POINTER (getter_setter_pair_p->setter_cp, desc_setter_p);
    }

    ecma_set_property_configurable_and_enumerable_flags (current_p->u.property_p, desc_p->flags);
  }

  /* 10. */
  return true;
} /* ecma_op_validate_and_apply_property_descriptor */

#if JERRY_ESNEXT
/**
 * CompletePropertyDescriptor method for proxy internal method
 *
 * See also:
 *          ECMA-262 v6, 6.2.4.5
 */
void
ecma_op_to_complete_property_descriptor (ecma_property_descriptor_t *desc_p) /**< target descriptor */
{
  /* 4. */
  if (!(desc_p->flags & (JERRY_PROP_IS_GET_DEFINED | JERRY_PROP_IS_SET_DEFINED)))
  {
    /* a. */
    desc_p->flags |= JERRY_PROP_IS_VALUE_DEFINED;
  }
  /* 5. */
  else
  {
    desc_p->flags |= (JERRY_PROP_IS_GET_DEFINED | JERRY_PROP_IS_SET_DEFINED);
  }
} /* ecma_op_to_complete_property_descriptor */
#endif /* JERRY_ESNEXT */

/**
 * @}
 * @}
 */
