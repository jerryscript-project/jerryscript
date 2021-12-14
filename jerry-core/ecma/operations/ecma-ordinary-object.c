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

#include "ecma-ordinary-object.h"

#include "ecma-arguments-object.h"
#include "ecma-array-object.h"
#include "ecma-builtin-helpers.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-lcache.h"
#include "ecma-lex-env.h"
#include "ecma-objects-general.h"
#include "ecma-objects.h"
#include "ecma-promise-object.h"
#include "ecma-string-object.h"
#include "ecma-typedarray-object.h"

#include "jcontext.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaordinaryobject ECMA Ordinary object related routines
 * @{
 */

/**
 * Helper function to get the prototype of the given object
 *
 * @return compressed pointer
 */
jmem_cpointer_t
ecma_object_get_prototype_of (ecma_object_t *obj_p) /**< object */
{
  JERRY_ASSERT (!ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT (!ECMA_OBJECT_IS_PROXY (obj_p));

  return obj_p->u2.prototype_cp;
} /* ecma_object_get_prototype_of */

/**
 * Ordinary object [[GetPrototypeOf]] operation
 *
 * Note: returned valid object must be freed.
 *
 * @return ecma_object_t * - prototype of the input object.
 *         NULL - the input object does not have a prototype.
 */
ecma_object_t *
ecma_ordinary_object_get_prototype_of (ecma_object_t *obj_p) /**< the object */
{
  jmem_cpointer_t proto_cp = ecma_object_get_prototype_of (obj_p);

  if (proto_cp == JMEM_CP_NULL)
  {
    return NULL;
  }

  ecma_object_t *proto_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, proto_cp);
  ecma_ref_object (proto_p);

  return proto_p;
} /* ecma_ordinary_object_get_prototype_of */

/**
 * Ordinary object [[SetPrototypeOf]] operation
 *
 * See also:
 *          ECMAScript v6, 9.1.2
 *
 * @return ECMA_VALUE_FALSE - if the operation fails
 *         ECMA_VALUE_TRUE - otherwise
 */
ecma_value_t
ecma_ordinary_object_set_prototype_of (ecma_object_t *obj_p, /**< base object */
                                       ecma_value_t proto) /**< prototype object */
{
  JERRY_ASSERT (!ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT (!ECMA_OBJECT_IS_PROXY (obj_p));

  /* 1. */
  JERRY_ASSERT (ecma_is_value_object (proto) || ecma_is_value_null (proto));

  /* 3. */
  ecma_object_t *current_proto_p = ECMA_GET_POINTER (ecma_object_t, ecma_object_get_prototype_of (obj_p));
  ecma_object_t *new_proto_p = ecma_is_value_null (proto) ? NULL : ecma_get_object_from_value (proto);

  /* 4. */
  if (new_proto_p == current_proto_p)
  {
    return ECMA_VALUE_TRUE;
  }

  /* 2 - 5. */
  if (ecma_is_value_false (ecma_ordinary_object_is_extensible (obj_p)))
  {
    return ECMA_VALUE_FALSE;
  }

  /**
   * When the prototype of a fast array changes, it is required to convert the
   * array to a "normal" array. This ensures that all [[Get]]/[[Set]]/etc.
   * calls works as expected.
   */
  if (ecma_op_object_is_fast_array (obj_p))
  {
    ecma_fast_array_convert_to_normal (obj_p);
  }

  /* 6. */
  ecma_object_t *iter_p = new_proto_p;

  /* 7 - 8. */
  while (true)
  {
    /* 8.a */
    if (iter_p == NULL)
    {
      break;
    }

    /* 8.b */
    if (obj_p == iter_p)
    {
      return ECMA_VALUE_FALSE;
    }

    /* 8.c.i */
#if JERRY_BUILTIN_PROXY
    if (ECMA_OBJECT_IS_PROXY (iter_p))
    {
      /**
       * Prevent setting 'Object.prototype.__proto__'
       * to avoid circular referencing in the prototype chain.
       */
      if (obj_p == ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE))
      {
        return ECMA_VALUE_FALSE;
      }
      break;
    }
#endif /* JERRY_BUILTIN_PROXY */

    /* 8.c.ii */
    iter_p = ECMA_GET_POINTER (ecma_object_t, ecma_object_get_prototype_of (iter_p));
  }

  /* 9. */
  ECMA_SET_POINTER (obj_p->u2.prototype_cp, new_proto_p);

  /* 10. */
  return ECMA_VALUE_TRUE;
} /* ecma_ordinary_object_set_prototype_of */

/**
 * Ordinary object's [[IsExtensible]] operation
 *
 * See also:
 *          ECMAScript v6, 9.1.2
 *
 * @return ECMA_VALUE_TRUE  - if object is extensible
 *         ECMA_VALUE_FALSE - otherwise
 */
ecma_value_t
ecma_ordinary_object_is_extensible (ecma_object_t *object_p) /**< object */
{
  JERRY_ASSERT (!ECMA_OBJECT_IS_PROXY (object_p));

  bool is_extensible = (object_p->type_flags_refs & ECMA_OBJECT_FLAG_EXTENSIBLE) != 0;

  JERRY_ASSERT (!ecma_op_object_is_fast_array (object_p) || is_extensible);

  return ecma_make_boolean_value (is_extensible);
} /* ecma_ordinary_object_is_extensible */

/**
 * Set value of [[Extensible]] object's internal property.
 *
 * @return ECMA_VALUE_TRUE
 */
ecma_value_t
ecma_ordinary_object_prevent_extensions (ecma_object_t *object_p) /**< object */
{
  if (JERRY_UNLIKELY (ecma_op_object_is_fast_array (object_p)))
  {
    ecma_fast_array_convert_to_normal (object_p);
  }

  object_p->type_flags_refs &= (ecma_object_descriptor_t) ~ECMA_OBJECT_FLAG_EXTENSIBLE;

  return ECMA_VALUE_TRUE;
} /* ecma_ordinary_object_prevent_extensions */

/**
 * Ordinary object's [[GetOwnProperty]] operation
 *
 * See also:
 *          ECMAScript v6, 9.1.7.1
 *
 * @return ecma property t
 */
ecma_property_descriptor_t
ecma_ordinary_object_get_own_property (ecma_object_t *object_p, /**< the object */
                                       ecma_string_t *property_name_p) /**< property name */
{
  ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();

  prop_desc.u.property_p = ecma_find_named_property (object_p, property_name_p);

  if (prop_desc.u.property_p != NULL)
  {
    prop_desc.flags =
      ECMA_PROP_DESC_PROPERTY_FOUND | ECMA_PROPERTY_TO_PROPERTY_DESCRIPTOR_FLAGS (prop_desc.u.property_p);
  }

  return prop_desc;
} /* ecma_ordinary_object_get_own_property */

/**
 * Ordinary object [[DefineOwnProperty]] operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *          ECMA-262 v5, 8.12.9
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_ordinary_object_define_own_property (ecma_object_t *object_p, /**< the object */
                                          ecma_string_t *property_name_p, /**< property name */
                                          const ecma_property_descriptor_t *property_desc_p) /**< property
                                                                                              *   descriptor */
{
  JERRY_ASSERT (object_p != NULL && !ecma_is_lexical_environment (object_p));
  JERRY_ASSERT (!ecma_op_object_is_fast_array (object_p));
  JERRY_ASSERT (!ECMA_OBJECT_IS_PROXY (object_p));
  JERRY_ASSERT (property_name_p != NULL);

  ecma_property_descriptor_t current_prop = ecma_internal_method_get_own_property (object_p, property_name_p);

  bool is_extensible = (object_p->type_flags_refs & ECMA_OBJECT_FLAG_EXTENSIBLE) != 0;
  bool is_valid = ecma_op_validate_and_apply_property_descriptor (object_p,
                                                                  property_name_p,
                                                                  property_desc_p,
                                                                  &current_prop,
                                                                  is_extensible);

  ecma_free_virtual_property_descriptor (&current_prop);

  if (!is_valid)
  {
    return ecma_raise_property_redefinition (property_name_p, property_desc_p->flags);
  }

  return ECMA_VALUE_TRUE;
} /* ecma_ordinary_object_define_own_property */

/**
 * Ordinary object's [[HasProperty]] operation
 *
 * See also:
 *          ECMAScript v6, 9.1.7.1
 *
 * @return ECMA_VALUE_ERROR - if the operation fails
 *         ECMA_VALUE_{TRUE_FALSE} - whether the property is found
 */
ecma_value_t
ecma_ordinary_object_has_property (ecma_object_t *object_p, /**< the object */
                                   ecma_string_t *property_name_p) /**< property name */
{
  while (true)
  {
    /* 2. */
    ecma_property_descriptor_t has_own = ecma_internal_method_get_own_property (object_p, property_name_p);
    JERRY_ASSERT (!ecma_property_descriptor_error (&has_own));

    if (JERRY_UNLIKELY (has_own.flags & ECMA_PROP_DESC_VIRTUAL_NOT_FOUND_AND_STOP))
    {
      return ECMA_VALUE_FALSE;
    }

    if (ecma_property_descriptor_found (&has_own))
    {
      ecma_free_virtual_property_descriptor (&has_own);
      return ECMA_VALUE_TRUE;
    }

    jmem_cpointer_t proto_cp = ecma_object_get_prototype_of (object_p);

    if (proto_cp == JMEM_CP_NULL)
    {
      return ECMA_VALUE_FALSE;
    }

    object_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, proto_cp);

    if (ECMA_OBJECT_GET_OWN_MAY_ABRUPT (object_p))
    {
      return ecma_internal_method_has_property (object_p, property_name_p);
    }
  }
} /* ecma_ordinary_object_has_property */

/**
 * Ordinary object's [[Get]] operation
 *
 * See also:
 *          ECMAScript v6, 10.1.8.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_ordinary_object_get (ecma_object_t *object_p, /**< the object */
                          ecma_string_t *property_name_p, /**< property name */
                          ecma_value_t receiver) /**< receiver */
{
  ecma_property_descriptor_t prop_desc;

  while (true)
  {
    /* 2. */
    prop_desc = ecma_internal_method_get_own_property (object_p, property_name_p);

    JERRY_ASSERT (!ecma_property_descriptor_error (&prop_desc));

    /* 3. */
    if (JERRY_LIKELY (ecma_property_descriptor_found (&prop_desc)))
    {
      break;
    }

    /* 3.a */
    jmem_cpointer_t proto_cp = ecma_object_get_prototype_of (object_p);

    /* 3.b */
    if (proto_cp == JMEM_CP_NULL)
    {
      return ECMA_VALUE_UNDEFINED;
    }

    /* 3.c */
    object_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, proto_cp);

    if (ECMA_OBJECT_GET_OWN_MAY_ABRUPT (object_p))
    {
      return ecma_internal_method_get (object_p, property_name_p, receiver);
    }
  }

  /* 4. - 8. */
  return ecma_property_descriptor_get (&prop_desc, receiver);
} /* ecma_ordinary_object_get */

/**
 * OrdinarySetWithOwnDescriptor abstract operation
 *
 * See also:
 *          ECMAScript v6, 10.1.9.2
 *
 * @return ecma value t
 */
static inline ecma_value_t JERRY_ATTR_ALWAYS_INLINE
ecma_ordinary_object_set_with_own_descriptor (ecma_string_t *property_name_p, /**< property name */
                                              ecma_value_t value, /**< ecma value */
                                              ecma_value_t receiver, /**< receiver */
                                              ecma_property_descriptor_t *own_desc_p) /**< property descriptor */
{
  /* 3. */
  if (ecma_property_descriptor_is_data_descriptor (own_desc_p))
  {
    /* 3.a */
    if (!ecma_property_descriptor_is_writable (own_desc_p))
    {
      return ECMA_VALUE_FALSE;
    }

    /* 3.b */
    if (!ecma_is_value_object (receiver))
    {
      return ECMA_VALUE_FALSE;
    }

    ecma_object_t *receiver_obj_p = ecma_get_object_from_value (receiver);

    /* 3.c */
    ecma_property_descriptor_t exsisting_desc = ecma_internal_method_get_own_property (receiver_obj_p, property_name_p);

    if (ecma_property_descriptor_error (&exsisting_desc))
    {
      return ECMA_VALUE_ERROR;
    }

    /* 3.d */
    if (ecma_property_descriptor_found (&exsisting_desc))
    {
      /* 3.d.i - 3.d.ii */
      if (ecma_property_descriptor_is_accessor_descriptor (&exsisting_desc)
          || !ecma_property_descriptor_is_writable (&exsisting_desc))
      {
        ecma_free_property_descriptor (&exsisting_desc);
        return ECMA_VALUE_FALSE;
      }

      /* 3.d.iii */
      ecma_property_descriptor_t value_desc = ecma_make_empty_define_property_descriptor ();
      value_desc.flags = (JERRY_PROP_IS_VALUE_DEFINED | JERRY_PROP_IS_WRITABLE_DEFINED | JERRY_PROP_IS_WRITABLE);
      value_desc.value = value;

      /* 3.d.iiv */
      ecma_value_t define_value_result =
        ecma_internal_method_define_own_property (receiver_obj_p, property_name_p, &value_desc);
      ecma_free_property_descriptor (&exsisting_desc);

      return define_value_result;
    }

    /* 3.e */
    ecma_property_descriptor_t value_desc;
    value_desc.flags = (JERRY_PROP_IS_CONFIGURABLE | JERRY_PROP_IS_CONFIGURABLE_DEFINED | JERRY_PROP_IS_ENUMERABLE
                        | JERRY_PROP_IS_ENUMERABLE_DEFINED | JERRY_PROP_IS_WRITABLE | JERRY_PROP_IS_WRITABLE_DEFINED
                        | JERRY_PROP_IS_VALUE_DEFINED);
    value_desc.value = value;

    /* 3.e.i */
    return ecma_internal_method_define_own_property (receiver_obj_p, property_name_p, &value_desc);
  }

  /* 4. */
  JERRY_ASSERT (ecma_property_descriptor_is_accessor_descriptor (own_desc_p));
  JERRY_ASSERT (own_desc_p->u.property_p != NULL);

  /* 5. */
  jmem_cpointer_t setter_cp =
    ecma_get_named_accessor_property (ECMA_PROPERTY_VALUE_PTR (own_desc_p->u.property_p))->setter_cp;

  /* 6. */
  if (setter_cp == JMEM_CP_NULL)
  {
    return ECMA_VALUE_FALSE;
  }

  /* 7. */
  ecma_object_t *setter_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, setter_cp);

  ecma_value_t setter_result = ecma_internal_method_call (setter_p, receiver, &value, 1);

  if (ECMA_IS_VALUE_ERROR (setter_result))
  {
    return setter_result;
  }

  ecma_free_value (setter_result);

  /* 8. */
  return ECMA_VALUE_TRUE;
} /* ecma_ordinary_object_set_with_own_descriptor */

/**
 * Ordinary object's [[Set]] operation
 *
 * See also:
 *          ECMAScript v6, 10.1.9.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_ordinary_object_set (ecma_object_t *object_p, /**< the object */
                          ecma_string_t *property_name_p, /**< property name */
                          ecma_value_t value, /**< ecma value */
                          ecma_value_t receiver, /**< receiver */
                          bool is_throw) /**< flag that controls failure handling */
{
  /* OrdinarySet 2. */
  ecma_property_descriptor_t own_desc = ecma_internal_method_get_own_property (object_p, property_name_p);

  JERRY_ASSERT (!ecma_property_descriptor_error (&own_desc));

  if ((own_desc.flags & (ECMA_PROP_DESC_VIRTUAL_FOUND | ECMA_PROP_DESC_DATA_WRITABLE))
        == (ECMA_PROP_DESC_FOUND | ECMA_PROP_DESC_DATA_WRITABLE)
      && ecma_make_object_value (object_p) == receiver)
  {
    ecma_named_data_property_assign_value (object_p, ECMA_PROPERTY_VALUE_PTR (own_desc.u.property_p), value);
    return ECMA_VALUE_TRUE;
  }

  ecma_object_t *obj_iter_p = object_p;
  /* OrdinarySetWithOwnDescriptor 2. */
  while (!ecma_property_descriptor_found (&own_desc))
  {
    /* OrdinarySetWithOwnDescriptor 2.a */
    jmem_cpointer_t parent_cp = ecma_object_get_prototype_of (obj_iter_p);

    /* OrdinarySetWithOwnDescriptor 2.a.c */
    if (parent_cp == JMEM_CP_NULL)
    {
      /* No property found on the prototype chain */
      if (JERRY_UNLIKELY (ecma_get_object_base_type (object_p) != ECMA_OBJECT_BASE_TYPE_GENERAL
                          || ecma_make_object_value (object_p) != receiver))
      {
        own_desc.flags = ECMA_PROP_DESC_VIRTUAL | ECMA_PROP_DESC_DATA_CONFIGURABLE_ENUMERABLE_WRITABLE;
        own_desc.value = ECMA_VALUE_UNDEFINED;
        break;
      }

      if (JERRY_UNLIKELY (!(object_p->type_flags_refs & ECMA_OBJECT_FLAG_EXTENSIBLE)))
      {
        return ecma_raise_readonly_assignment (property_name_p, is_throw);
      }

      ecma_property_value_t *new_prop_value_p;
      new_prop_value_p = ecma_create_named_data_property (object_p,
                                                          property_name_p,
                                                          ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE,
                                                          NULL);

      new_prop_value_p->value = ecma_copy_value_if_not_object (value);
      return ECMA_VALUE_TRUE;
    }

    /* OrdinarySetWithOwnDescriptor 2.a.b */
    obj_iter_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, parent_cp);

    if (ecma_get_object_base_type (obj_iter_p) != ECMA_OBJECT_BASE_TYPE_GENERAL)
    {
      return ecma_internal_method_set (obj_iter_p, property_name_p, value, receiver, is_throw);
    }

    /* OrdinarySet 2. */
    own_desc = ecma_internal_method_get_own_property (obj_iter_p, property_name_p);
    JERRY_ASSERT (!ecma_property_descriptor_error (&own_desc));
  }

  /* OrdinarySet 3. */
  ecma_value_t result = ecma_ordinary_object_set_with_own_descriptor (property_name_p, value, receiver, &own_desc);
  ecma_free_property_descriptor (&own_desc);

  if (ecma_is_value_false (result))
  {
    return ecma_raise_readonly_assignment (property_name_p, is_throw);
  }

  return result;
} /* ecma_ordinary_object_set */

/**
 * Ordinary object [[Delete]] operation
 *
 * 10.1.10.1 OrdinaryDelete
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_ordinary_object_delete (ecma_object_t *obj_p, /**< the object */
                             ecma_string_t *property_name_p, /**< property name */
                             bool is_throw) /**< flag that controls failure handling */
{
  JERRY_ASSERT (obj_p != NULL && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT (property_name_p != NULL);

  /* 2. */
  ecma_property_descriptor_t prop_desc = ecma_internal_method_get_own_property (obj_p, property_name_p);

  /* 3. */
  if (!ecma_property_descriptor_found (&prop_desc))
  {
    return ECMA_VALUE_TRUE;
  }

  /* 4. */
  if (!ecma_property_descriptor_is_configurable (&prop_desc))
  {
    /* 5. */
    ecma_free_property_descriptor (&prop_desc);
    return ecma_raise_non_configurable_property (property_name_p, is_throw);
  }

  uint8_t property_flag = *prop_desc.u.property_p;

  /* 4.a. */
  ecma_delete_property (obj_p, ECMA_PROPERTY_VALUE_PTR (prop_desc.u.property_p));

  if (property_flag & ECMA_PROPERTY_FLAG_BUILT_IN)
  {
    ecma_internal_method_delete_lazy_property (obj_p, property_name_p);
  }

  ecma_free_property_descriptor (&prop_desc);

  /* 4.b */
  return ECMA_VALUE_TRUE;
} /* ecma_ordinary_object_delete */

/**
 * Helper routine for heapsort algorithm.
 */
static void
ecma_op_object_heap_sort_shift_down (ecma_value_t *buffer_p, /**< array of items */
                                     uint32_t item_count, /**< number of items */
                                     uint32_t item_index) /**< index of updated item */
{
  while (true)
  {
    uint32_t highest_index = item_index;
    uint32_t current_index = (item_index << 1) + 1;

    if (current_index >= item_count)
    {
      return;
    }

    uint32_t value = ecma_string_get_array_index (ecma_get_string_from_value (buffer_p[highest_index]));
    uint32_t left_value = ecma_string_get_array_index (ecma_get_string_from_value (buffer_p[current_index]));

    if (value < left_value)
    {
      highest_index = current_index;
      value = left_value;
    }

    current_index++;

    if (current_index < item_count
        && value < ecma_string_get_array_index (ecma_get_string_from_value (buffer_p[current_index])))
    {
      highest_index = current_index;
    }

    if (highest_index == item_index)
    {
      return;
    }

    ecma_value_t tmp = buffer_p[highest_index];
    buffer_p[highest_index] = buffer_p[item_index];
    buffer_p[item_index] = tmp;

    item_index = highest_index;
  }
} /* ecma_op_object_heap_sort_shift_down */

/**
 * Ordinary object's [[OwnPropertyKeys]] internal method
 *
 * Order of names in the collection:
 *  - integer indices in ascending order
 *  - other indices in creation order (for built-ins: the order of the properties are listed in specification).
 *
 * Note:
 *      Implementation of the routine assumes that new properties are appended to beginning of corresponding object's
 *      property list, and the list is not reordered (in other words, properties are stored in order that is reversed
 *      to the properties' addition order).
 *
 * @return collection of property names
 */
ecma_collection_t *
ecma_ordinary_object_own_property_keys (ecma_object_t *obj_p, /**< object */
                                        jerry_property_filter_t filter) /**< name filters */
{
  ecma_collection_t *prop_names_p = ecma_new_collection ();
  ecma_property_counter_t prop_counter = { 0, 0, 0 };

  ecma_internal_method_list_lazy_property_keys (obj_p, prop_names_p, &prop_counter, filter);

  jmem_cpointer_t prop_iter_cp = obj_p->u1.property_list_cp;

#if JERRY_PROPERTY_HASHMAP
  if (prop_iter_cp != JMEM_CP_NULL)
  {
    ecma_property_header_t *prop_iter_p = ECMA_GET_NON_NULL_POINTER (ecma_property_header_t, prop_iter_cp);

    if (prop_iter_p->types[0] == ECMA_PROPERTY_TYPE_HASHMAP)
    {
      prop_iter_cp = prop_iter_p->next_property_cp;
    }
  }
#endif /* JERRY_PROPERTY_HASHMAP */

  jmem_cpointer_t counter_prop_iter_cp = prop_iter_cp;

  uint32_t array_index_named_props = 0;
  uint32_t string_named_props = 0;
#if JERRY_ESNEXT
  uint32_t symbol_named_props = 0;
#endif /* JERRY_ESNEXT */

  while (counter_prop_iter_cp != JMEM_CP_NULL)
  {
    ecma_property_header_t *prop_iter_p = ECMA_GET_NON_NULL_POINTER (ecma_property_header_t, counter_prop_iter_cp);
    JERRY_ASSERT (ECMA_PROPERTY_IS_PROPERTY_PAIR (prop_iter_p));

    for (int i = 0; i < ECMA_PROPERTY_PAIR_ITEM_COUNT; i++)
    {
      ecma_property_t *property_p = prop_iter_p->types + i;

      if (!ECMA_PROPERTY_IS_RAW (*property_p) || (*property_p & ECMA_PROPERTY_FLAG_BUILT_IN))
      {
        continue;
      }

      ecma_property_pair_t *prop_pair_p = (ecma_property_pair_t *) prop_iter_p;

      if (ECMA_PROPERTY_GET_NAME_TYPE (*property_p) == ECMA_DIRECT_STRING_MAGIC
          && prop_pair_p->names_cp[i] >= LIT_NON_INTERNAL_MAGIC_STRING__COUNT
          && prop_pair_p->names_cp[i] < LIT_MAGIC_STRING__COUNT)
      {
        continue;
      }

      ecma_string_t *name_p = ecma_string_from_property_name (*property_p, prop_pair_p->names_cp[i]);

      if (ecma_string_get_array_index (name_p) != ECMA_STRING_NOT_ARRAY_INDEX)
      {
        array_index_named_props++;
      }
#if JERRY_ESNEXT
      else if (ecma_prop_name_is_symbol (name_p))
      {
        if (!(name_p->u.hash & ECMA_SYMBOL_FLAG_PRIVATE_KEY))
        {
          symbol_named_props++;
        }
      }
#endif /* JERRY_ESNEXT */
      else
      {
        string_named_props++;
      }

      ecma_deref_ecma_string (name_p);
    }

    counter_prop_iter_cp = prop_iter_p->next_property_cp;
  }

  if (filter & JERRY_PROPERTY_FILTER_EXCLUDE_INTEGER_INDICES)
  {
    JERRY_ASSERT (prop_counter.array_index_named_props == 0);
    array_index_named_props = 0;
  }

  if (filter & JERRY_PROPERTY_FILTER_EXCLUDE_STRINGS)
  {
    JERRY_ASSERT (prop_counter.string_named_props == 0);
    string_named_props = 0;
  }

#if JERRY_ESNEXT
  if (filter & JERRY_PROPERTY_FILTER_EXCLUDE_SYMBOLS)
  {
    JERRY_ASSERT (prop_counter.symbol_named_props == 0);
    symbol_named_props = 0;
  }

  uint32_t total = array_index_named_props + string_named_props + symbol_named_props;
#else /* !JERRY_ESNEXT */
  uint32_t total = array_index_named_props + string_named_props;
#endif /* JERRY_ESNEXT */

  if (total == 0)
  {
    return prop_names_p;
  }

  ecma_collection_reserve (prop_names_p, total);
  prop_names_p->item_count += total;

  ecma_value_t *buffer_p = prop_names_p->buffer_p;
  ecma_value_t *array_index_current_p = buffer_p + array_index_named_props + prop_counter.array_index_named_props;
  ecma_value_t *string_current_p = array_index_current_p + string_named_props + prop_counter.string_named_props;

#if JERRY_ESNEXT
  ecma_value_t *symbol_current_p = string_current_p + symbol_named_props + prop_counter.symbol_named_props;

  if (prop_counter.symbol_named_props > 0 && (array_index_named_props + string_named_props) > 0)
  {
    memmove ((void *) string_current_p,
             (void *) (buffer_p + prop_counter.array_index_named_props + prop_counter.string_named_props),
             prop_counter.symbol_named_props * sizeof (ecma_value_t));
  }
#endif /* JERRY_ESNEXT */

  if (prop_counter.string_named_props > 0 && array_index_named_props > 0)
  {
    memmove ((void *) array_index_current_p,
             (void *) (buffer_p + prop_counter.array_index_named_props),
             prop_counter.string_named_props * sizeof (ecma_value_t));
  }

  while (prop_iter_cp != JMEM_CP_NULL)
  {
    ecma_property_header_t *prop_iter_p = ECMA_GET_NON_NULL_POINTER (ecma_property_header_t, prop_iter_cp);
    JERRY_ASSERT (ECMA_PROPERTY_IS_PROPERTY_PAIR (prop_iter_p));

    for (int i = 0; i < ECMA_PROPERTY_PAIR_ITEM_COUNT; i++)
    {
      ecma_property_t *property_p = prop_iter_p->types + i;

      if (!ECMA_PROPERTY_IS_RAW (*property_p) || (*property_p & ECMA_PROPERTY_FLAG_BUILT_IN))
      {
        continue;
      }

      ecma_property_pair_t *prop_pair_p = (ecma_property_pair_t *) prop_iter_p;

      if (ECMA_PROPERTY_GET_NAME_TYPE (*property_p) == ECMA_DIRECT_STRING_MAGIC
          && prop_pair_p->names_cp[i] >= LIT_NON_INTERNAL_MAGIC_STRING__COUNT
          && prop_pair_p->names_cp[i] < LIT_MAGIC_STRING__COUNT)
      {
        continue;
      }

      ecma_string_t *name_p = ecma_string_from_property_name (*property_p, prop_pair_p->names_cp[i]);

      if (ecma_string_get_array_index (name_p) != ECMA_STRING_NOT_ARRAY_INDEX)
      {
        if (!(filter & JERRY_PROPERTY_FILTER_EXCLUDE_INTEGER_INDICES))
        {
          *(--array_index_current_p) = ecma_make_string_value (name_p);
          continue;
        }
      }
#if JERRY_ESNEXT
      else if (ecma_prop_name_is_symbol (name_p))
      {
        if (!(filter & JERRY_PROPERTY_FILTER_EXCLUDE_SYMBOLS) && !(name_p->u.hash & ECMA_SYMBOL_FLAG_PRIVATE_KEY))
        {
          *(--symbol_current_p) = ecma_make_symbol_value (name_p);
          continue;
        }
      }
#endif /* JERRY_ESNEXT */
      else
      {
        if (!(filter & JERRY_PROPERTY_FILTER_EXCLUDE_STRINGS))
        {
          *(--string_current_p) = ecma_make_string_value (name_p);
          continue;
        }
      }

      ecma_deref_ecma_string (name_p);
    }

    prop_iter_cp = prop_iter_p->next_property_cp;
  }

  if (array_index_named_props > 1 || (array_index_named_props == 1 && prop_counter.array_index_named_props > 0))
  {
    uint32_t prev_value = 0;
    ecma_value_t *array_index_p = buffer_p + prop_counter.array_index_named_props;
    ecma_value_t *array_index_end_p = array_index_p + array_index_named_props;

    if (prop_counter.array_index_named_props > 0)
    {
      prev_value = ecma_string_get_array_index (ecma_get_string_from_value (array_index_p[-1]));
    }

    do
    {
      uint32_t value = ecma_string_get_array_index (ecma_get_string_from_value (*array_index_p++));

      if (value < prev_value)
      {
        uint32_t array_props = prop_counter.array_index_named_props + array_index_named_props;
        uint32_t i = (array_props >> 1) - 1;

        do
        {
          ecma_op_object_heap_sort_shift_down (buffer_p, array_props, i);
        } while (i-- > 0);

        i = array_props - 1;

        do
        {
          ecma_value_t tmp = buffer_p[i];
          buffer_p[i] = buffer_p[0];
          buffer_p[0] = tmp;

          ecma_op_object_heap_sort_shift_down (buffer_p, i, 0);
        } while (--i > 0);

        break;
      }

      prev_value = value;
    } while (array_index_p < array_index_end_p);
  }

  return prop_names_p;
} /* ecma_ordinary_object_own_property_keys */

/**
 * Handle calling [[Call]] of ordinary object
 *
 * @return ecma value
 */
ecma_value_t
ecma_ordinary_object_call (ecma_object_t *obj_p, /**< object */
                           ecma_value_t this_value, /**< 'this' argument value */
                           const ecma_value_t *arguments_list_p, /**< arguments list */
                           uint32_t arguments_list_len) /**< arguments list length */
{
  JERRY_UNUSED_4 (obj_p, this_value, arguments_list_p, arguments_list_len);

  return ecma_raise_type_error (ECMA_ERR_EXPECTED_A_FUNCTION);
} /* ecma_ordinary_object_call */

/**
 * Handle calling [[Construct]] of ordinary object
 *
 * @return ecma value
 */
ecma_value_t
ecma_ordinary_object_construct (ecma_object_t *obj_p, /**< the object */
                                ecma_object_t *new_target_p, /**< mew target */
                                const ecma_value_t *arguments_list_p, /**< arguments list */
                                uint32_t arguments_list_len) /**< arguments list length */
{
  JERRY_UNUSED_4 (obj_p, new_target_p, arguments_list_p, arguments_list_len);

  return ecma_raise_type_error (ECMA_ERR_EXPECTED_A_FUNCTION);
} /* ecma_ordinary_object_construct */

/**
 * List lazy instantiated property names of ordinary object's
 */
void
ecma_ordinary_object_list_lazy_property_keys (ecma_object_t *obj_p, /**< the object */
                                              ecma_collection_t *prop_names_p, /**< property names */
                                              ecma_property_counter_t *prop_counter_p, /**< property counter */
                                              jerry_property_filter_t filter) /**< name filter */
{
  JERRY_UNUSED_4 (obj_p, prop_names_p, prop_counter_p, filter);
} /* ecma_ordinary_object_list_lazy_property_keys */

/**
 * Delete configurable properties of ordinary object's
 */
void
ecma_ordinary_object_delete_lazy_property (ecma_object_t *obj_p, /**< the object */
                                           ecma_string_t *property_name_p) /**< property name */
{
  JERRY_UNUSED_2 (obj_p, property_name_p);
} /* ecma_ordinary_object_delete_lazy_property */

/**
 * @}
 * @}
 */
