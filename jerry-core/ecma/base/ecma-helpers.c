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

#include "ecma-alloc.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-lcache.h"
#include "ecma-property-hashmap.h"
#include "jrt-bit-fields.h"
#include "byte-code.h"
#include "re-compiler.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmahelpers Helpers for operations with ECMA data types
 * @{
 */

/**
 * The ecma property types must be lower than the container mask.
 */
JERRY_STATIC_ASSERT (ECMA_PROPERTY_TYPE_MASK >= ECMA_PROPERTY_TYPE__MAX,
                     ecma_property_types_must_be_lower_than_the_container_mask);

/**
 * The ecma object types must be lower than the container mask.
 */
JERRY_STATIC_ASSERT (ECMA_OBJECT_TYPE_MASK >= ECMA_OBJECT_TYPE__MAX,
                     ecma_object_types_must_be_lower_than_the_container_mask);

/**
 * The ecma lexical environment types must be lower than the container mask.
 */
JERRY_STATIC_ASSERT (ECMA_OBJECT_TYPE_MASK >= ECMA_LEXICAL_ENVIRONMENT_TYPE__MAX,
                     ecma_lexical_environment_types_must_be_lower_than_the_container_mask);

/**
 * The ecma built in flag must follow the object type.
 */
JERRY_STATIC_ASSERT (ECMA_OBJECT_TYPE_MASK + 1 == ECMA_OBJECT_FLAG_BUILT_IN_OR_LEXICAL_ENV,
                     ecma_built_in_flag_must_follow_the_object_type);

/**
 * The ecma gc visited flag must follow the built in flag.
 */
JERRY_STATIC_ASSERT (ECMA_OBJECT_FLAG_GC_VISITED == (ECMA_OBJECT_FLAG_BUILT_IN_OR_LEXICAL_ENV << 1),
                     ecma_gc_visited_flag_must_follow_the_built_in_flag);

/**
 * The ecma extensible flag must follow the gc visited flag.
 */
JERRY_STATIC_ASSERT (ECMA_OBJECT_FLAG_EXTENSIBLE == (ECMA_OBJECT_FLAG_GC_VISITED << 1),
                     ecma_extensible_flag_must_follow_the_gc_visited_flag);

/**
 * The ecma object ref one must follow the extensible flag.
 */
JERRY_STATIC_ASSERT (ECMA_OBJECT_REF_ONE == (ECMA_OBJECT_FLAG_EXTENSIBLE << 1),
                     ecma_object_ref_one_must_follow_the_extensible_flag);

/**
 * The ecma object max ref does not fill the remaining bits.
 */
JERRY_STATIC_ASSERT ((ECMA_OBJECT_MAX_REF | (ECMA_OBJECT_REF_ONE - 1)) == UINT16_MAX,
                     ecma_object_max_ref_does_not_fill_the_remaining_bits);

/**
 * Create an object with specified prototype object
 * (or NULL prototype if there is not prototype for the object)
 * and value of 'Extensible' attribute.
 *
 * Reference counter's value will be set to one.
 *
 * @return pointer to the object's descriptor
 */
ecma_object_t *
ecma_create_object (ecma_object_t *prototype_object_p, /**< pointer to prototybe of the object (or NULL) */
                    bool is_extended, /**< extended object */
                    bool is_extensible, /**< value of extensible attribute */
                    ecma_object_type_t type) /**< object type */
{
  ecma_object_t *new_object_p = (is_extended ? ((ecma_object_t *) ecma_alloc_extended_object ())
                                             : ecma_alloc_object ());

  uint16_t type_flags = (uint16_t) type;

  if (is_extensible)
  {
    type_flags = (uint16_t) (type_flags | ECMA_OBJECT_FLAG_EXTENSIBLE);
  }

  new_object_p->type_flags_refs = type_flags;

  ecma_init_gc_info (new_object_p);

  new_object_p->property_list_or_bound_object_cp = JMEM_CP_NULL;

  ECMA_SET_POINTER (new_object_p->prototype_or_outer_reference_cp,
                    prototype_object_p);

  return new_object_p;
} /* ecma_create_object */

/**
 * Create a declarative lexical environment with specified outer lexical environment
 * (or NULL if the environment is not nested).
 *
 * See also: ECMA-262 v5, 10.2.1.1
 *
 * Reference counter's value will be set to one.
 *
 * @return pointer to the descriptor of lexical environment
 */
ecma_object_t *
ecma_create_decl_lex_env (ecma_object_t *outer_lexical_environment_p) /**< outer lexical environment */
{
  ecma_object_t *new_lexical_environment_p = ecma_alloc_object ();

  uint16_t type = ECMA_OBJECT_FLAG_BUILT_IN_OR_LEXICAL_ENV | ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE;
  new_lexical_environment_p->type_flags_refs = type;

  ecma_init_gc_info (new_lexical_environment_p);

  new_lexical_environment_p->property_list_or_bound_object_cp = JMEM_CP_NULL;

  ECMA_SET_POINTER (new_lexical_environment_p->prototype_or_outer_reference_cp,
                    outer_lexical_environment_p);

  return new_lexical_environment_p;
} /* ecma_create_decl_lex_env */

/**
 * Create a object lexical environment with specified outer lexical environment
 * (or NULL if the environment is not nested), binding object and provideThis flag.
 *
 * See also: ECMA-262 v5, 10.2.1.2
 *
 * Reference counter's value will be set to one.
 *
 * @return pointer to the descriptor of lexical environment
 */
ecma_object_t *
ecma_create_object_lex_env (ecma_object_t *outer_lexical_environment_p, /**< outer lexical environment */
                            ecma_object_t *binding_obj_p, /**< binding object */
                            bool provide_this) /**< provideThis flag */
{
  JERRY_ASSERT (binding_obj_p != NULL
                && !ecma_is_lexical_environment (binding_obj_p));

  ecma_object_t *new_lexical_environment_p = ecma_alloc_object ();

  uint16_t type;

  if (provide_this)
  {
    type = ECMA_OBJECT_FLAG_BUILT_IN_OR_LEXICAL_ENV | ECMA_LEXICAL_ENVIRONMENT_THIS_OBJECT_BOUND;
  }
  else
  {
    type = ECMA_OBJECT_FLAG_BUILT_IN_OR_LEXICAL_ENV | ECMA_LEXICAL_ENVIRONMENT_OBJECT_BOUND;
  }

  new_lexical_environment_p->type_flags_refs = type;

  ecma_init_gc_info (new_lexical_environment_p);

  ECMA_SET_NON_NULL_POINTER (new_lexical_environment_p->property_list_or_bound_object_cp,
                             binding_obj_p);

  ECMA_SET_POINTER (new_lexical_environment_p->prototype_or_outer_reference_cp,
                    outer_lexical_environment_p);

  return new_lexical_environment_p;
} /* ecma_create_object_lex_env */

/**
 * Check if the object is lexical environment.
 */
inline bool __attr_pure___
ecma_is_lexical_environment (const ecma_object_t *object_p) /**< object or lexical environment */
{
  JERRY_ASSERT (object_p != NULL);

  uint32_t full_type = object_p->type_flags_refs & (ECMA_OBJECT_FLAG_BUILT_IN_OR_LEXICAL_ENV | ECMA_OBJECT_TYPE_MASK);

  return full_type >= (ECMA_OBJECT_FLAG_BUILT_IN_OR_LEXICAL_ENV | ECMA_LEXICAL_ENVIRONMENT_TYPE_START);
} /* ecma_is_lexical_environment */

/**
 * Get value of [[Extensible]] object's internal property.
 */
inline bool __attr_pure___
ecma_get_object_extensible (const ecma_object_t *object_p) /**< object */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (!ecma_is_lexical_environment (object_p));

  return (object_p->type_flags_refs & ECMA_OBJECT_FLAG_EXTENSIBLE) != 0;
} /* ecma_get_object_extensible */

/**
 * Set value of [[Extensible]] object's internal property.
 */
inline void
ecma_set_object_extensible (ecma_object_t *object_p, /**< object */
                            bool is_extensible) /**< value of [[Extensible]] */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (!ecma_is_lexical_environment (object_p));

  if (is_extensible)
  {
    object_p->type_flags_refs = (uint16_t) (object_p->type_flags_refs | ECMA_OBJECT_FLAG_EXTENSIBLE);
  }
  else
  {
    object_p->type_flags_refs = (uint16_t) (object_p->type_flags_refs & ~ECMA_OBJECT_FLAG_EXTENSIBLE);
  }
} /* ecma_set_object_extensible */

/**
 * Get object's internal implementation-defined type.
 */
inline ecma_object_type_t __attr_pure___
ecma_get_object_type (const ecma_object_t *object_p) /**< object */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (!ecma_is_lexical_environment (object_p));

  return (ecma_object_type_t) (object_p->type_flags_refs & ECMA_OBJECT_TYPE_MASK);
} /* ecma_get_object_type */

/**
 * Set object's internal implementation-defined type.
 */
inline void
ecma_set_object_type (ecma_object_t *object_p, /**< object */
                      ecma_object_type_t type) /**< type */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (!(object_p->type_flags_refs & ECMA_OBJECT_FLAG_BUILT_IN_OR_LEXICAL_ENV));

  object_p->type_flags_refs = (uint16_t) ((object_p->type_flags_refs & ~ECMA_OBJECT_TYPE_MASK) | type);
} /* ecma_set_object_type */

/**
 * Get object's prototype.
 */
inline ecma_object_t *__attr_pure___
ecma_get_object_prototype (const ecma_object_t *object_p) /**< object */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (!ecma_is_lexical_environment (object_p));

  return ECMA_GET_POINTER (ecma_object_t,
                           object_p->prototype_or_outer_reference_cp);
} /* ecma_get_object_prototype */

/**
 * Check if the object is a built-in object
 *
 * @return true / false
 */
inline bool __attr_pure___
ecma_get_object_is_builtin (const ecma_object_t *object_p) /**< object */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (!ecma_is_lexical_environment (object_p));

  return (object_p->type_flags_refs & ECMA_OBJECT_FLAG_BUILT_IN_OR_LEXICAL_ENV) != 0;
} /* ecma_get_object_is_builtin */

/**
 * Set flag indicating whether the object is a built-in object
 */
inline void
ecma_set_object_is_builtin (ecma_object_t *object_p) /**< object */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (!(object_p->type_flags_refs & ECMA_OBJECT_FLAG_BUILT_IN_OR_LEXICAL_ENV));
  JERRY_ASSERT ((object_p->type_flags_refs & ECMA_OBJECT_TYPE_MASK) < ECMA_LEXICAL_ENVIRONMENT_TYPE_START);

  object_p->type_flags_refs = (uint16_t) (object_p->type_flags_refs | ECMA_OBJECT_FLAG_BUILT_IN_OR_LEXICAL_ENV);
} /* ecma_set_object_is_builtin */

/**
 * Get type of lexical environment.
 */
inline ecma_lexical_environment_type_t __attr_pure___
ecma_get_lex_env_type (const ecma_object_t *object_p) /**< lexical environment */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (ecma_is_lexical_environment (object_p));

  return (ecma_lexical_environment_type_t) (object_p->type_flags_refs & ECMA_OBJECT_TYPE_MASK);
} /* ecma_get_lex_env_type */

/**
 * Get outer reference of lexical environment.
 */
inline ecma_object_t *__attr_pure___
ecma_get_lex_env_outer_reference (const ecma_object_t *object_p) /**< lexical environment */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (ecma_is_lexical_environment (object_p));

  return ECMA_GET_POINTER (ecma_object_t,
                           object_p->prototype_or_outer_reference_cp);
} /* ecma_get_lex_env_outer_reference */

/**
 * Get object's/lexical environment's property list.
 *
 * See also:
 *          ecma_op_object_get_property_names
 */
inline ecma_property_header_t *__attr_pure___
ecma_get_property_list (const ecma_object_t *object_p) /**< object or lexical environment */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (!ecma_is_lexical_environment (object_p)
                || ecma_get_lex_env_type (object_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE);

  return ECMA_GET_POINTER (ecma_property_header_t,
                           object_p->property_list_or_bound_object_cp);
} /* ecma_get_property_list */

/**
 * Get lexical environment's 'provideThis' property
 */
inline bool __attr_pure___
ecma_get_lex_env_provide_this (const ecma_object_t *object_p) /**< object-bound lexical environment */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (ecma_is_lexical_environment (object_p));
  JERRY_ASSERT (ecma_get_lex_env_type (object_p) == ECMA_LEXICAL_ENVIRONMENT_OBJECT_BOUND
                || ecma_get_lex_env_type (object_p) == ECMA_LEXICAL_ENVIRONMENT_THIS_OBJECT_BOUND);

  return ecma_get_lex_env_type (object_p) == ECMA_LEXICAL_ENVIRONMENT_THIS_OBJECT_BOUND;
} /* ecma_get_lex_env_provide_this */

/**
 * Get lexical environment's bound object.
 */
inline ecma_object_t *__attr_pure___
ecma_get_lex_env_binding_object (const ecma_object_t *object_p) /**< object-bound lexical environment */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (ecma_is_lexical_environment (object_p));
  JERRY_ASSERT (ecma_get_lex_env_type (object_p) == ECMA_LEXICAL_ENVIRONMENT_OBJECT_BOUND
                || ecma_get_lex_env_type (object_p) == ECMA_LEXICAL_ENVIRONMENT_THIS_OBJECT_BOUND);

  return ECMA_GET_NON_NULL_POINTER (ecma_object_t,
                                    object_p->property_list_or_bound_object_cp);
} /* ecma_get_lex_env_binding_object */

/**
 * Create a property in an object and link it into
 * the object's properties' linked-list (at start of the list).
 *
 * @return pointer to the newly created property value
 */
static ecma_property_value_t *
ecma_create_property (ecma_object_t *object_p, /**< the object */
                      ecma_string_t *name_p, /**< property name */
                      uint8_t type_and_flags, /**< type and flags, see ecma_property_info_t */
                      ecma_property_value_t value, /**< property value */
                      ecma_property_t **out_prop_p) /**< [out] the property is also returned
                                                     *         if this field is non-NULL */
{
  JERRY_ASSERT (ECMA_PROPERTY_PAIR_ITEM_COUNT == 2);

  jmem_cpointer_t *property_list_head_p = &object_p->property_list_or_bound_object_cp;

  if (*property_list_head_p != ECMA_NULL_POINTER)
  {
    /* If the first entry is free (deleted), it is reused. */
    ecma_property_header_t *first_property_p = ECMA_GET_NON_NULL_POINTER (ecma_property_header_t,
                                                                          *property_list_head_p);
    bool has_hashmap = false;

    if (ECMA_PROPERTY_GET_TYPE (first_property_p->types[0]) == ECMA_PROPERTY_TYPE_HASHMAP)
    {
      property_list_head_p = &first_property_p->next_property_cp;
      first_property_p = ECMA_GET_NON_NULL_POINTER (ecma_property_header_t,
                                                    *property_list_head_p);
      has_hashmap = true;
    }

    JERRY_ASSERT (ECMA_PROPERTY_IS_PROPERTY_PAIR (first_property_p));

    if (first_property_p->types[0] == ECMA_PROPERTY_TYPE_DELETED)
    {
      first_property_p->types[0] = type_and_flags;

      ecma_property_pair_t *first_property_pair_p = (ecma_property_pair_t *) first_property_p;
      ECMA_SET_POINTER (first_property_pair_p->names_cp[0], name_p);

      ecma_property_t *property_p = first_property_p->types + 0;

      JERRY_ASSERT (ECMA_PROPERTY_VALUE_PTR (property_p) == first_property_pair_p->values + 0);

      if (out_prop_p != NULL)
      {
        *out_prop_p = property_p;
      }

      first_property_pair_p->values[0] = value;

      /* The property must be fully initialized before ecma_property_hashmap_insert
       * is called, because the insert operation may reallocate the hashmap, and
       * that triggers garbage collection which scans all properties of all objects.
       * A not fully initialized but queued property may cause a crash. */

      if (has_hashmap && name_p != NULL)
      {
        ecma_property_hashmap_insert (object_p,
                                      name_p,
                                      first_property_pair_p,
                                      0);
      }

      return first_property_pair_p->values + 0;
    }
  }

  /* Otherwise we create a new property pair and use its second value. */
  ecma_property_pair_t *first_property_pair_p = ecma_alloc_property_pair ();

  /* Need to query property_list_head_p again and recheck the existennce
   * of property hasmap, because ecma_alloc_property_pair may delete them. */
  property_list_head_p = &object_p->property_list_or_bound_object_cp;
  bool has_hashmap = false;

  if (*property_list_head_p != ECMA_NULL_POINTER)
  {
    ecma_property_header_t *first_property_p = ECMA_GET_NON_NULL_POINTER (ecma_property_header_t,
                                                                          *property_list_head_p);

    if (ECMA_PROPERTY_GET_TYPE (first_property_p->types[0]) == ECMA_PROPERTY_TYPE_HASHMAP)
    {
      property_list_head_p = &first_property_p->next_property_cp;
      has_hashmap = true;
    }
  }

  /* Just copy the previous value (no need to decompress, compress). */
  first_property_pair_p->header.next_property_cp = *property_list_head_p;
  first_property_pair_p->header.types[0] = ECMA_PROPERTY_TYPE_DELETED;
  first_property_pair_p->header.types[1] = type_and_flags;
  first_property_pair_p->names_cp[0] = ECMA_NULL_POINTER;
  ECMA_SET_POINTER (first_property_pair_p->names_cp[1], name_p);

  ECMA_SET_NON_NULL_POINTER (*property_list_head_p, &first_property_pair_p->header);

  ecma_property_t *property_p = first_property_pair_p->header.types + 1;

  JERRY_ASSERT (ECMA_PROPERTY_VALUE_PTR (property_p) == first_property_pair_p->values + 1);

  if (out_prop_p != NULL)
  {
    *out_prop_p = property_p;
  }

  first_property_pair_p->values[1] = value;

  /* See the comment before the other ecma_property_hashmap_insert above. */

  if (has_hashmap && name_p != NULL)
  {
    ecma_property_hashmap_insert (object_p,
                                  name_p,
                                  first_property_pair_p,
                                  1);
  }

  return first_property_pair_p->values + 1;
} /* ecma_create_property */

/**
 * Create internal property in an object and link it into
 * the object's properties' linked-list (at start of the list).
 *
 * @return pointer to the newly created property value
 */
ecma_value_t *
ecma_create_internal_property (ecma_object_t *object_p, /**< the object */
                               ecma_internal_property_id_t property_id) /**< internal property identifier */
{
  JERRY_ASSERT (ecma_find_internal_property (object_p, property_id) == NULL);

  uint8_t id_byte = (uint8_t) (property_id << ECMA_PROPERTY_FLAG_SHIFT);
  uint8_t type_and_flags = (uint8_t) (ECMA_PROPERTY_TYPE_INTERNAL | id_byte);

  ecma_property_value_t value;
  value.value = ECMA_NULL_POINTER;

  ecma_property_value_t *prop_value_p = ecma_create_property (object_p, NULL, type_and_flags, value, NULL);
  return &prop_value_p->value;
} /* ecma_create_internal_property */

/**
 * Find internal property in the object's property set.
 *
 * @return pointer to the property, if it is found,
 *         NULL - otherwise.
 */
ecma_value_t *
ecma_find_internal_property (ecma_object_t *object_p, /**< object descriptor */
                             ecma_internal_property_id_t property_id) /**< internal property identifier */
{
  JERRY_ASSERT (object_p != NULL);

  ecma_property_header_t *prop_iter_p = ecma_get_property_list (object_p);

  if (prop_iter_p != NULL
      && ECMA_PROPERTY_GET_TYPE (prop_iter_p->types[0]) == ECMA_PROPERTY_TYPE_HASHMAP)
  {
    prop_iter_p = ECMA_GET_POINTER (ecma_property_header_t,
                                    prop_iter_p->next_property_cp);
  }

  while (prop_iter_p != NULL)
  {
    JERRY_ASSERT (ECMA_PROPERTY_IS_PROPERTY_PAIR (prop_iter_p));

    if (ECMA_PROPERTY_GET_TYPE (prop_iter_p->types[0]) == ECMA_PROPERTY_TYPE_INTERNAL
        && ECMA_PROPERTY_GET_INTERNAL_PROPERTY_TYPE (prop_iter_p->types + 0) == property_id)
    {
      ecma_property_pair_t *prop_pair_p = (ecma_property_pair_t *) prop_iter_p;
      return &prop_pair_p->values[0].value;
    }

    if (ECMA_PROPERTY_GET_TYPE (prop_iter_p->types[1]) == ECMA_PROPERTY_TYPE_INTERNAL
        && ECMA_PROPERTY_GET_INTERNAL_PROPERTY_TYPE (prop_iter_p->types + 1) == property_id)
    {
      ecma_property_pair_t *prop_pair_p = (ecma_property_pair_t *) prop_iter_p;
      return &prop_pair_p->values[1].value;
    }

    prop_iter_p = ECMA_GET_POINTER (ecma_property_header_t,
                                    prop_iter_p->next_property_cp);
  }

  return NULL;
} /* ecma_find_internal_property */

/**
 * Get an internal property.
 *
 * Warning:
 *         the property must exist
 *
 * @return pointer to the property
 */
inline ecma_value_t * __attr_always_inline___
ecma_get_internal_property (ecma_object_t *object_p, /**< object descriptor */
                            ecma_internal_property_id_t property_id) /**< internal property identifier */
{
  ecma_value_t *property_p = ecma_find_internal_property (object_p, property_id);

  JERRY_ASSERT (property_p != NULL);

  return property_p;
} /* ecma_get_internal_property */

/**
 * Create named data property with given name, attributes and undefined value
 * in the specified object.
 *
 * @return pointer to the newly created property value
 */
ecma_property_value_t *
ecma_create_named_data_property (ecma_object_t *object_p, /**< object */
                                 ecma_string_t *name_p, /**< property name */
                                 uint8_t prop_attributes, /**< property attributes (See: ecma_property_flags_t) */
                                 ecma_property_t **out_prop_p) /**< [out] the property is also returned
                                                                *         if this field is non-NULL */
{
  JERRY_ASSERT (object_p != NULL && name_p != NULL);
  JERRY_ASSERT (ecma_find_named_property (object_p, name_p) == NULL);
  JERRY_ASSERT ((prop_attributes & ~ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE) == 0);

  uint8_t type_and_flags = ECMA_PROPERTY_TYPE_NAMEDDATA | prop_attributes;

  ecma_ref_ecma_string (name_p);

  ecma_property_value_t value;
  value.value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

  return ecma_create_property (object_p, name_p, type_and_flags, value, out_prop_p);
} /* ecma_create_named_data_property */

/**
 * Create named accessor property with given name, attributes, getter and setter.
 *
 * @return pointer to the newly created property value
 */
ecma_property_value_t *
ecma_create_named_accessor_property (ecma_object_t *object_p, /**< object */
                                     ecma_string_t *name_p, /**< property name */
                                     ecma_object_t *get_p, /**< getter */
                                     ecma_object_t *set_p, /**< setter */
                                     uint8_t prop_attributes) /**< property attributes */
{
  JERRY_ASSERT (object_p != NULL && name_p != NULL);
  JERRY_ASSERT (ecma_find_named_property (object_p, name_p) == NULL);
  JERRY_ASSERT ((prop_attributes & ~ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE) == 0);

  uint8_t type_and_flags = ECMA_PROPERTY_TYPE_NAMEDACCESSOR | prop_attributes;

  ecma_ref_ecma_string (name_p);

  ecma_property_value_t value;
#ifdef JERRY_CPOINTER_32_BIT
  ecma_getter_setter_pointers_t *getter_setter_pair_p;
  getter_setter_pair_p = jmem_pools_alloc (sizeof (ecma_getter_setter_pointers_t));
  ECMA_SET_POINTER (getter_setter_pair_p->getter_p, get_p);
  ECMA_SET_POINTER (getter_setter_pair_p->setter_p, set_p);
  ECMA_SET_POINTER (value.getter_setter_pair_cp, getter_setter_pair_p);
#else /* !JERRY_CPOINTER_32_BIT */
  ECMA_SET_POINTER (value.getter_setter_pair.getter_p, get_p);
  ECMA_SET_POINTER (value.getter_setter_pair.setter_p, set_p);
#endif /* JERRY_CPOINTER_32_BIT */

  return ecma_create_property (object_p, name_p, type_and_flags, value, NULL);
} /* ecma_create_named_accessor_property */

/**
 * Find named data property or named access property in specified object.
 *
 * @return pointer to the property, if it is found,
 *         NULL - otherwise.
 */
ecma_property_t *
ecma_find_named_property (ecma_object_t *obj_p, /**< object to find property in */
                          ecma_string_t *name_p) /**< property's name */
{
  JERRY_ASSERT (obj_p != NULL);
  JERRY_ASSERT (name_p != NULL);

  ecma_property_t *property_p = ecma_lcache_lookup (obj_p, name_p);

  if (property_p != NULL)
  {
    return property_p;
  }

  ecma_property_header_t *prop_iter_p = ecma_get_property_list (obj_p);

#ifndef CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE
  if (prop_iter_p != NULL
      && ECMA_PROPERTY_GET_TYPE (prop_iter_p->types[0]) == ECMA_PROPERTY_TYPE_HASHMAP)
  {
    ecma_string_t *property_real_name_p;
    property_p = ecma_property_hashmap_find ((ecma_property_hashmap_t *) prop_iter_p,
                                             name_p,
                                             &property_real_name_p);

    if (property_p != NULL
        && !ecma_is_property_lcached (property_p))
    {
      ecma_lcache_insert (obj_p, property_real_name_p, property_p);
    }

    return property_p;
  }
#endif /* !CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE */

  property_p = NULL;
  ecma_string_t *property_name_p = NULL;

  uint32_t steps = 0;

  while (prop_iter_p != NULL)
  {
    JERRY_ASSERT (ECMA_PROPERTY_IS_PROPERTY_PAIR (prop_iter_p));

    ecma_property_pair_t *prop_pair_p = (ecma_property_pair_t *) prop_iter_p;

    JERRY_ASSERT (ECMA_PROPERTY_PAIR_ITEM_COUNT == 2);

    if (prop_pair_p->names_cp[0] != ECMA_NULL_POINTER)
    {
      property_name_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                   prop_pair_p->names_cp[0]);

      if (ecma_compare_ecma_strings (name_p, property_name_p))
      {
        property_p = prop_iter_p->types + 0;
        break;
      }
    }

    if (prop_pair_p->names_cp[1] != ECMA_NULL_POINTER)
    {
      property_name_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                   prop_pair_p->names_cp[1]);

      if (ecma_compare_ecma_strings (name_p, property_name_p))
      {
        property_p = prop_iter_p->types + 1;
        break;
      }
    }

    steps++;

    prop_iter_p = ECMA_GET_POINTER (ecma_property_header_t,
                                    prop_iter_p->next_property_cp);
  }

  if (steps > (ECMA_PROPERTY_HASMAP_MINIMUM_SIZE / 4))
  {
    ecma_property_hashmap_create (obj_p);
  }

  if (property_p != NULL
      && !ecma_is_property_lcached (property_p))
  {
    ecma_lcache_insert (obj_p, property_name_p, property_p);
  }

  return property_p;
} /* ecma_find_named_property */

/**
 * Get named data property or named access property in specified object.
 *
 * Warning:
 *         the property must exist
 *
 * @return pointer to the property, if it is found,
 *         NULL - otherwise.
 */
ecma_property_value_t *
ecma_get_named_data_property (ecma_object_t *obj_p, /**< object to find property in */
                              ecma_string_t *name_p) /**< property's name */
{
  JERRY_ASSERT (obj_p != NULL);
  JERRY_ASSERT (name_p != NULL);

  ecma_property_t *property_p = ecma_find_named_property (obj_p, name_p);

  JERRY_ASSERT (property_p != NULL
                && ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDDATA);

  return ECMA_PROPERTY_VALUE_PTR (property_p);
} /* ecma_get_named_data_property */

/**
 * Free the internal property and values it references.
 */
static void
ecma_free_internal_property (ecma_property_t *property_p) /**< the property */
{
  JERRY_ASSERT (property_p != NULL && ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_INTERNAL);

  uint32_t property_value = ECMA_PROPERTY_VALUE_PTR (property_p)->value;

  switch (ECMA_PROPERTY_GET_INTERNAL_PROPERTY_TYPE (property_p))
  {
    case ECMA_INTERNAL_PROPERTY_ECMA_VALUE: /* ecma-value property except object */
    {
      JERRY_ASSERT (!ecma_is_value_object (property_value));
      ecma_free_value (property_value);

      break;
    }

    case ECMA_INTERNAL_PROPERTY_DATE_FLOAT: /* pointer to a ecma_number_t */
    {
      ecma_number_t *num_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_number_t, property_value);
      ecma_dealloc_number (num_p);

      break;
    }

    case ECMA_INTERNAL_PROPERTY_NATIVE_HANDLE: /* an external pointer */
    case ECMA_INTERNAL_PROPERTY_FREE_CALLBACK: /* an external pointer */
    {
      ecma_free_external_pointer_in_property (property_p);

      break;
    }

    case ECMA_INTERNAL_PROPERTY_SCOPE: /* a lexical environment */
    case ECMA_INTERNAL_PROPERTY_PARAMETERS_MAP: /* an object */
    case ECMA_INTERNAL_PROPERTY_CLASS: /* an enum */
    case ECMA_INTERNAL_PROPERTY_INSTANTIATED_MASK_32_63: /* an integer (bit-mask) */
    case ECMA_INTERNAL_PROPERTY_BOUND_FUNCTION_TARGET_FUNCTION:
    {
      break;
    }

    case ECMA_INTERNAL_PROPERTY_BOUND_FUNCTION_BOUND_THIS:
    {
      ecma_free_value_if_not_object (property_value);
      break;
    }

    case ECMA_INTERNAL_PROPERTY_BOUND_FUNCTION_BOUND_ARGS:
    {
      if (property_value != ECMA_NULL_POINTER)
      {
        ecma_free_values_collection (ECMA_GET_INTERNAL_VALUE_POINTER (ecma_collection_header_t, property_value),
                                     false);
      }

      break;
    }

    case ECMA_INTERNAL_PROPERTY__COUNT: /* not a real internal property type,
                                         * but number of the real internal property types */
    {
      JERRY_UNREACHABLE ();
      break;
    }

    case ECMA_INTERNAL_PROPERTY_REGEXP_BYTECODE: /* compressed pointer to a regexp bytecode array */
    {
      ecma_compiled_code_t *bytecode_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_compiled_code_t, property_value);

      if (bytecode_p != NULL)
      {
        ecma_bytecode_deref (bytecode_p);
      }
      break;
    }
  }
} /* ecma_free_internal_property */

/**
 * Free property values and change their type to deleted.
 */
void
ecma_free_property (ecma_object_t *object_p, /**< object the property belongs to */
                    ecma_string_t *name_p, /**< name of the property or NULL */
                    ecma_property_t *property_p) /**< property */
{
  JERRY_ASSERT (object_p != NULL && property_p != NULL);

  switch (ECMA_PROPERTY_GET_TYPE (*property_p))
  {
    case ECMA_PROPERTY_TYPE_NAMEDDATA:
    {
      ecma_free_value_if_not_object (ECMA_PROPERTY_VALUE_PTR (property_p)->value);

      if (ecma_is_property_lcached (property_p))
      {
        ecma_lcache_invalidate (object_p, name_p, property_p);
      }
      break;
    }
    case ECMA_PROPERTY_TYPE_NAMEDACCESSOR:
    {
#ifdef JERRY_CPOINTER_32_BIT
      ecma_getter_setter_pointers_t *getter_setter_pair_p;
      getter_setter_pair_p = ECMA_GET_POINTER (ecma_getter_setter_pointers_t,
                                               ECMA_PROPERTY_VALUE_PTR (property_p)->getter_setter_pair_cp);
      jmem_pools_free (getter_setter_pair_p, sizeof (ecma_getter_setter_pointers_t));
#endif /* JERRY_CPOINTER_32_BIT */

      if (ecma_is_property_lcached (property_p))
      {
        ecma_lcache_invalidate (object_p, name_p, property_p);
      }
      break;
    }
    case ECMA_PROPERTY_TYPE_INTERNAL:
    {
      JERRY_ASSERT (name_p == NULL);
      ecma_free_internal_property (property_p);
      break;
    }
    default:
    {
      JERRY_UNREACHABLE ();
      break;
    }
  }

  *property_p = ECMA_PROPERTY_TYPE_DELETED;
} /* ecma_free_property */

/**
 * Delete the object's property referenced by its value pointer.
 *
 * Note: specified property must be owned by specified object.
 */
void
ecma_delete_property (ecma_object_t *object_p, /**< object */
                      ecma_property_value_t *prop_value_p) /**< property value reference */
{
  ecma_property_header_t *cur_prop_p = ecma_get_property_list (object_p);
  ecma_property_header_t *prev_prop_p = NULL;
  bool has_hashmap = false;

  if (cur_prop_p != NULL
      && ECMA_PROPERTY_GET_TYPE (cur_prop_p->types[0]) == ECMA_PROPERTY_TYPE_HASHMAP)
  {
    prev_prop_p = cur_prop_p;
    cur_prop_p = ECMA_GET_POINTER (ecma_property_header_t,
                                   cur_prop_p->next_property_cp);
    has_hashmap = true;
  }

  while (true)
  {
    JERRY_ASSERT (cur_prop_p != NULL);
    JERRY_ASSERT (ECMA_PROPERTY_IS_PROPERTY_PAIR (cur_prop_p));

    ecma_property_pair_t *prop_pair_p = (ecma_property_pair_t *) cur_prop_p;

    for (int i = 0; i < ECMA_PROPERTY_PAIR_ITEM_COUNT; i++)
    {
      if ((prop_pair_p->values + i) == prop_value_p)
      {
        ecma_string_t *name_p = ECMA_GET_POINTER (ecma_string_t, prop_pair_p->names_cp[i]);

        ecma_free_property (object_p, name_p, cur_prop_p->types + i);

        if (name_p != NULL)
        {
          if (has_hashmap)
          {
            ecma_property_hashmap_delete (object_p,
                                          name_p,
                                          cur_prop_p->types + i);
          }

          ecma_deref_ecma_string (name_p);
        }

        prop_pair_p->names_cp[i] = ECMA_NULL_POINTER;

        JERRY_ASSERT (ECMA_PROPERTY_PAIR_ITEM_COUNT == 2);

        if (cur_prop_p->types[1 - i] != ECMA_PROPERTY_TYPE_DELETED)
        {
          /* The other property is still valid. */
          return;
        }

        JERRY_ASSERT (cur_prop_p->types[i] == ECMA_PROPERTY_TYPE_DELETED);

        if (prev_prop_p == NULL)
        {
          object_p->property_list_or_bound_object_cp = cur_prop_p->next_property_cp;
        }
        else
        {
          prev_prop_p->next_property_cp = cur_prop_p->next_property_cp;
        }

        ecma_dealloc_property_pair ((ecma_property_pair_t *) cur_prop_p);
        return;
      }
    }

    prev_prop_p = cur_prop_p;
    cur_prop_p = ECMA_GET_POINTER (ecma_property_header_t,
                                   cur_prop_p->next_property_cp);
  }
} /* ecma_delete_property */

/**
 * Check whether the object contains a property
 */
static void
ecma_assert_object_contains_the_property (const ecma_object_t *object_p, /**< ecma-object */
                                          const ecma_property_value_t *prop_value_p, /**< property value */
                                          ecma_property_types_t type) /**< expected property type */
{
#ifndef JERRY_NDEBUG
  ecma_property_header_t *prop_iter_p = ecma_get_property_list (object_p);

  JERRY_ASSERT (prop_iter_p != NULL);

  if (ECMA_PROPERTY_GET_TYPE (prop_iter_p->types[0]) == ECMA_PROPERTY_TYPE_HASHMAP)
  {
    prop_iter_p = ECMA_GET_POINTER (ecma_property_header_t,
                                    prop_iter_p->next_property_cp);
  }

  while (prop_iter_p != NULL)
  {
    JERRY_ASSERT (ECMA_PROPERTY_IS_PROPERTY_PAIR (prop_iter_p));

    ecma_property_pair_t *prop_pair_p = (ecma_property_pair_t *) prop_iter_p;

    for (int i = 0; i < ECMA_PROPERTY_PAIR_ITEM_COUNT; i++)
    {
      if ((prop_pair_p->values + i) == prop_value_p)
      {
        JERRY_ASSERT (ECMA_PROPERTY_GET_TYPE (prop_pair_p->header.types[i]) == type);
        return;
      }
    }

    prop_iter_p = ECMA_GET_POINTER (ecma_property_header_t,
                                    prop_iter_p->next_property_cp);
  }

  JERRY_UNREACHABLE ();

#else /* JERRY_NDEBUG */
  JERRY_UNUSED (object_p);
  JERRY_UNUSED (prop_value_p);
  JERRY_UNUSED (type);
#endif /* !JERRY_NDEBUG */
} /* ecma_assert_object_contains_the_property */

/**
 * Assign value to named data property
 *
 * Note:
 *      value previously stored in the property is freed
 */
inline void __attr_always_inline___
ecma_named_data_property_assign_value (ecma_object_t *obj_p, /**< object */
                                       ecma_property_value_t *prop_value_p, /**< property value reference */
                                       ecma_value_t value) /**< value to assign */
{
  ecma_assert_object_contains_the_property (obj_p, prop_value_p, ECMA_PROPERTY_TYPE_NAMEDDATA);

  ecma_value_assign_value (&prop_value_p->value, value);
} /* ecma_named_data_property_assign_value */

/**
 * Get getter of named accessor property
 *
 * @return pointer to object - getter of the property
 */
ecma_object_t *
ecma_get_named_accessor_property_getter (const ecma_property_value_t *prop_value_p) /**< property value reference */
{
#ifdef JERRY_CPOINTER_32_BIT
  ecma_getter_setter_pointers_t *getter_setter_pair_p;
  getter_setter_pair_p = ECMA_GET_POINTER (ecma_getter_setter_pointers_t,
                                           prop_value_p->getter_setter_pair_cp);
  return ECMA_GET_POINTER (ecma_object_t, getter_setter_pair_p->getter_p);
#else /* !JERRY_CPOINTER_32_BIT */
  return ECMA_GET_POINTER (ecma_object_t, prop_value_p->getter_setter_pair.getter_p);
#endif /* JERRY_CPOINTER_32_BIT */
} /* ecma_get_named_accessor_property_getter */

/**
 * Get setter of named accessor property
 *
 * @return pointer to object - setter of the property
 */
ecma_object_t *
ecma_get_named_accessor_property_setter (const ecma_property_value_t *prop_value_p) /**< property value reference */
{
#ifdef JERRY_CPOINTER_32_BIT
  ecma_getter_setter_pointers_t *getter_setter_pair_p;
  getter_setter_pair_p = ECMA_GET_POINTER (ecma_getter_setter_pointers_t,
                                           prop_value_p->getter_setter_pair_cp);
  return ECMA_GET_POINTER (ecma_object_t, getter_setter_pair_p->setter_p);
#else /* !JERRY_CPOINTER_32_BIT */
  return ECMA_GET_POINTER (ecma_object_t, prop_value_p->getter_setter_pair.setter_p);
#endif /* JERRY_CPOINTER_32_BIT */
} /* ecma_get_named_accessor_property_setter */

/**
 * Set getter of named accessor property
 */
void
ecma_set_named_accessor_property_getter (ecma_object_t *object_p, /**< the property's container */
                                         ecma_property_value_t *prop_value_p, /**< property value reference */
                                         ecma_object_t *getter_p) /**< getter object */
{
  ecma_assert_object_contains_the_property (object_p, prop_value_p, ECMA_PROPERTY_TYPE_NAMEDACCESSOR);

#ifdef JERRY_CPOINTER_32_BIT
  ecma_getter_setter_pointers_t *getter_setter_pair_p;
  getter_setter_pair_p = ECMA_GET_POINTER (ecma_getter_setter_pointers_t,
                                           prop_value_p->getter_setter_pair_cp);
  ECMA_SET_POINTER (getter_setter_pair_p->getter_p, getter_p);
#else /* !JERRY_CPOINTER_32_BIT */
  ECMA_SET_POINTER (prop_value_p->getter_setter_pair.getter_p, getter_p);
#endif /* JERRY_CPOINTER_32_BIT */
} /* ecma_set_named_accessor_property_getter */

/**
 * Set setter of named accessor property
 */
void
ecma_set_named_accessor_property_setter (ecma_object_t *object_p, /**< the property's container */
                                         ecma_property_value_t *prop_value_p, /**< property value reference */
                                         ecma_object_t *setter_p) /**< setter object */
{
  ecma_assert_object_contains_the_property (object_p, prop_value_p, ECMA_PROPERTY_TYPE_NAMEDACCESSOR);

#ifdef JERRY_CPOINTER_32_BIT
  ecma_getter_setter_pointers_t *getter_setter_pair_p;
  getter_setter_pair_p = ECMA_GET_POINTER (ecma_getter_setter_pointers_t,
                                           prop_value_p->getter_setter_pair_cp);
  ECMA_SET_POINTER (getter_setter_pair_p->setter_p, setter_p);
#else /* !JERRY_CPOINTER_32_BIT */
  ECMA_SET_POINTER (prop_value_p->getter_setter_pair.setter_p, setter_p);
#endif /* JERRY_CPOINTER_32_BIT */
} /* ecma_set_named_accessor_property_setter */

/**
 * Get property's 'Writable' attribute value
 *
 * @return true - property is writable,
 *         false - otherwise.
 */
inline bool __attr_always_inline___
ecma_is_property_writable (ecma_property_t property) /**< property */
{
  JERRY_ASSERT (ECMA_PROPERTY_GET_TYPE (property) == ECMA_PROPERTY_TYPE_NAMEDDATA
                || ECMA_PROPERTY_GET_TYPE (property) == ECMA_PROPERTY_TYPE_VIRTUAL);

  return (property & ECMA_PROPERTY_FLAG_WRITABLE) != 0;
} /* ecma_is_property_writable */

/**
 * Set property's 'Writable' attribute value
 */
void
ecma_set_property_writable_attr (ecma_property_t *property_p, /**< [in,out] property */
                                 bool is_writable) /**< new value for writable flag */
{
  JERRY_ASSERT (ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDDATA);

  if (is_writable)
  {
    *property_p = (uint8_t) (*property_p | ECMA_PROPERTY_FLAG_WRITABLE);
  }
  else
  {
    *property_p = (uint8_t) (*property_p & ~ECMA_PROPERTY_FLAG_WRITABLE);
  }
} /* ecma_set_property_writable_attr */

/**
 * Get property's 'Enumerable' attribute value
 *
 * @return true - property is enumerable,
 *         false - otherwise.
 */
inline bool __attr_always_inline___
ecma_is_property_enumerable (ecma_property_t property) /**< property */
{
  JERRY_ASSERT (ECMA_PROPERTY_GET_TYPE (property) == ECMA_PROPERTY_TYPE_NAMEDDATA
                || ECMA_PROPERTY_GET_TYPE (property) == ECMA_PROPERTY_TYPE_NAMEDACCESSOR
                || ECMA_PROPERTY_GET_TYPE (property) == ECMA_PROPERTY_TYPE_VIRTUAL);

  return (property & ECMA_PROPERTY_FLAG_ENUMERABLE) != 0;
} /* ecma_is_property_enumerable */

/**
 * Set property's 'Enumerable' attribute value
 */
void
ecma_set_property_enumerable_attr (ecma_property_t *property_p, /**< [in,out] property */
                                   bool is_enumerable) /**< new value for enumerable flag */
{
  JERRY_ASSERT (ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDDATA
                || ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDACCESSOR);

  if (is_enumerable)
  {
    *property_p = (uint8_t) (*property_p | ECMA_PROPERTY_FLAG_ENUMERABLE);
  }
  else
  {
    *property_p = (uint8_t) (*property_p & ~ECMA_PROPERTY_FLAG_ENUMERABLE);
  }
} /* ecma_set_property_enumerable_attr */

/**
 * Get property's 'Configurable' attribute value
 *
 * @return true - property is configurable,
 *         false - otherwise.
 */
inline bool __attr_always_inline___
ecma_is_property_configurable (ecma_property_t property) /**< property */
{
  JERRY_ASSERT (ECMA_PROPERTY_GET_TYPE (property) == ECMA_PROPERTY_TYPE_NAMEDDATA
                || ECMA_PROPERTY_GET_TYPE (property) == ECMA_PROPERTY_TYPE_NAMEDACCESSOR
                || ECMA_PROPERTY_GET_TYPE (property) == ECMA_PROPERTY_TYPE_VIRTUAL);

  return (property & ECMA_PROPERTY_FLAG_CONFIGURABLE) != 0;
} /* ecma_is_property_configurable */

/**
 * Set property's 'Configurable' attribute value
 */
void
ecma_set_property_configurable_attr (ecma_property_t *property_p, /**< [in,out] property */
                                     bool is_configurable) /**< new value for configurable flag */
{
  JERRY_ASSERT (ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDDATA
                || ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDACCESSOR);

  if (is_configurable)
  {
    *property_p = (uint8_t) (*property_p | ECMA_PROPERTY_FLAG_CONFIGURABLE);
  }
  else
  {
    *property_p = (uint8_t) (*property_p & ~ECMA_PROPERTY_FLAG_CONFIGURABLE);
  }
} /* ecma_set_property_configurable_attr */

/**
 * Check whether the property is registered in LCache
 *
 * @return true / false
 */
inline bool __attr_always_inline___
ecma_is_property_lcached (ecma_property_t *property_p) /**< property */
{
  JERRY_ASSERT (ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDDATA
                || ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDACCESSOR);

  return (*property_p & ECMA_PROPERTY_FLAG_LCACHED) != 0;
} /* ecma_is_property_lcached */

/**
 * Set value of flag indicating whether the property is registered in LCache
 */
inline void __attr_always_inline___
ecma_set_property_lcached (ecma_property_t *property_p, /**< property */
                           bool is_lcached) /**< new value for lcached flag */
{
  JERRY_ASSERT (ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDDATA
                || ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDACCESSOR);

  if (is_lcached)
  {
    *property_p = (uint8_t) (*property_p | ECMA_PROPERTY_FLAG_LCACHED);
  }
  else
  {
    *property_p = (uint8_t) (*property_p & ~ECMA_PROPERTY_FLAG_LCACHED);
  }
} /* ecma_set_property_lcached */

/**
 * Construct empty property descriptor, i.e.:
 *  property descriptor with all is_defined flags set to false and the rest - to default value.
 */
ecma_property_descriptor_t
ecma_make_empty_property_descriptor (void)
{
  ecma_property_descriptor_t prop_desc;

  prop_desc.is_value_defined = false;
  prop_desc.value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
  prop_desc.is_writable_defined = false;
  prop_desc.is_writable = false;
  prop_desc.is_enumerable_defined = false;
  prop_desc.is_enumerable = false;
  prop_desc.is_configurable_defined = false;
  prop_desc.is_configurable = false;
  prop_desc.is_get_defined = false;
  prop_desc.get_p = NULL;
  prop_desc.is_set_defined = false;
  prop_desc.set_p = NULL;

  return prop_desc;
} /* ecma_make_empty_property_descriptor */

/**
 * Free values contained in the property descriptor
 * and make it empty property descriptor
 */
void
ecma_free_property_descriptor (ecma_property_descriptor_t *prop_desc_p) /**< property descriptor */
{
  if (prop_desc_p->is_value_defined)
  {
    ecma_free_value (prop_desc_p->value);
  }

  if (prop_desc_p->is_get_defined
      && prop_desc_p->get_p != NULL)
  {
    ecma_deref_object (prop_desc_p->get_p);
  }

  if (prop_desc_p->is_set_defined
      && prop_desc_p->set_p != NULL)
  {
    ecma_deref_object (prop_desc_p->set_p);
  }

  *prop_desc_p = ecma_make_empty_property_descriptor ();
} /* ecma_free_property_descriptor */

/**
 * Increase reference counter of Compact
 * Byte Code or regexp byte code.
 */
void
ecma_bytecode_ref (ecma_compiled_code_t *bytecode_p) /**< byte code pointer */
{
  /* Abort program if maximum reference number is reached. */
  if (bytecode_p->refs >= UINT16_MAX)
  {
    jerry_fatal (ERR_REF_COUNT_LIMIT);
  }

  bytecode_p->refs++;
} /* ecma_bytecode_ref */

/**
 * Decrease reference counter of Compact
 * Byte Code or regexp byte code.
 */
void
ecma_bytecode_deref (ecma_compiled_code_t *bytecode_p) /**< byte code pointer */
{
  JERRY_ASSERT (bytecode_p->refs > 0);

  bytecode_p->refs--;

  if (bytecode_p->refs > 0)
  {
    /* Non-zero reference counter. */
    return;
  }

  if (bytecode_p->status_flags & CBC_CODE_FLAGS_FUNCTION)
  {
    jmem_cpointer_t *literal_start_p = NULL;
    uint32_t literal_end;
    uint32_t const_literal_end;

    if (bytecode_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
    {
      uint8_t *byte_p = (uint8_t *) bytecode_p;
      literal_start_p = (jmem_cpointer_t *) (byte_p + sizeof (cbc_uint16_arguments_t));

      cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) bytecode_p;
      literal_end = args_p->literal_end;
      const_literal_end = args_p->const_literal_end;
    }
    else
    {
      uint8_t *byte_p = (uint8_t *) bytecode_p;
      literal_start_p = (jmem_cpointer_t *) (byte_p + sizeof (cbc_uint8_arguments_t));

      cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) bytecode_p;
      literal_end = args_p->literal_end;
      const_literal_end = args_p->const_literal_end;
    }

    for (uint32_t i = const_literal_end; i < literal_end; i++)
    {
      jmem_cpointer_t bytecode_cpointer = literal_start_p[i];
      ecma_compiled_code_t *bytecode_literal_p = ECMA_GET_NON_NULL_POINTER (ecma_compiled_code_t,
                                                                            bytecode_cpointer);

      /* Self references are ignored. */
      if (bytecode_literal_p != bytecode_p)
      {
        ecma_bytecode_deref (bytecode_literal_p);
      }
    }
  }
  else
  {
#ifndef CONFIG_DISABLE_REGEXP_BUILTIN
    re_compiled_code_t *re_bytecode_p = (re_compiled_code_t *) bytecode_p;

    ecma_deref_ecma_string (ECMA_GET_NON_NULL_POINTER (ecma_string_t, re_bytecode_p->pattern_cp));
#endif /* !CONFIG_DISABLE_REGEXP_BUILTIN */
  }

  jmem_heap_free_block (bytecode_p,
                        ((size_t) bytecode_p->size) << JMEM_ALIGNMENT_LOG);
} /* ecma_bytecode_deref */

/**
 * @}
 * @}
 */
