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

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmahelpers Helpers for operations with ECMA data types
 * @{
 */

#include "ecma-alloc.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-lcache.h"
#include "jrt-bit-fields.h"

/**
 * Create an object with specified prototype object
 * (or NULL prototype if there is not prototype for the object)
 * and value of 'Extensible' attribute.
 *
 * Reference counter's value will be set to one.
 *
 * @return pointer to the object's descriptor
 */
ecma_object_t*
ecma_create_object (ecma_object_t *prototype_object_p, /**< pointer to prototybe of the object (or NULL) */
                    bool is_extensible, /**< value of extensible attribute */
                    ecma_object_type_t type) /**< object type */
{
  ecma_object_t *object_p = ecma_alloc_object ();

  ecma_init_gc_info (object_p);

  object_p->container = jrt_set_bit_field_value (object_p->container,
                                                 ECMA_NULL_POINTER,
                                                 ECMA_OBJECT_PROPERTIES_OR_BOUND_OBJECT_CP_POS,
                                                 ECMA_OBJECT_PROPERTIES_OR_BOUND_OBJECT_CP_WIDTH);
  object_p->container = jrt_set_bit_field_value (object_p->container,
                                                 false,
                                                 ECMA_OBJECT_IS_LEXICAL_ENVIRONMENT_POS,
                                                 ECMA_OBJECT_IS_LEXICAL_ENVIRONMENT_WIDTH);
  object_p->container = jrt_set_bit_field_value (object_p->container,
                                                 is_extensible,
                                                 ECMA_OBJECT_OBJ_EXTENSIBLE_POS,
                                                 ECMA_OBJECT_OBJ_EXTENSIBLE_WIDTH);
  object_p->container = jrt_set_bit_field_value (object_p->container,
                                                 type,
                                                 ECMA_OBJECT_OBJ_TYPE_POS,
                                                 ECMA_OBJECT_OBJ_TYPE_WIDTH);

  uint64_t prototype_object_cp;
  ECMA_SET_POINTER (prototype_object_cp, prototype_object_p);

  object_p->container = jrt_set_bit_field_value (object_p->container,
                                                 prototype_object_cp,
                                                 ECMA_OBJECT_OBJ_PROTOTYPE_OBJECT_CP_POS,
                                                 ECMA_OBJECT_OBJ_PROTOTYPE_OBJECT_CP_WIDTH);
  ecma_gc_update_may_ref_younger_object_flag_by_object (object_p, prototype_object_p);

  ecma_set_object_is_builtin (object_p, false);

  return object_p;
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
ecma_object_t*
ecma_create_decl_lex_env (ecma_object_t *outer_lexical_environment_p) /**< outer lexical environment */
{
  ecma_object_t *new_lexical_environment_p = ecma_alloc_object ();

  ecma_init_gc_info (new_lexical_environment_p);

  new_lexical_environment_p->container = jrt_set_bit_field_value (new_lexical_environment_p->container,
                                                                  ECMA_NULL_POINTER,
                                                                  ECMA_OBJECT_PROPERTIES_OR_BOUND_OBJECT_CP_POS,
                                                                  ECMA_OBJECT_PROPERTIES_OR_BOUND_OBJECT_CP_WIDTH);
  new_lexical_environment_p->container = jrt_set_bit_field_value (new_lexical_environment_p->container,
                                                                  true,
                                                                  ECMA_OBJECT_IS_LEXICAL_ENVIRONMENT_POS,
                                                                  ECMA_OBJECT_IS_LEXICAL_ENVIRONMENT_WIDTH);

  new_lexical_environment_p->container = jrt_set_bit_field_value (new_lexical_environment_p->container,
                                                                  ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE,
                                                                  ECMA_OBJECT_LEX_ENV_TYPE_POS,
                                                                  ECMA_OBJECT_LEX_ENV_TYPE_WIDTH);

  uint64_t outer_reference_cp;
  ECMA_SET_POINTER (outer_reference_cp, outer_lexical_environment_p);
  new_lexical_environment_p->container = jrt_set_bit_field_value (new_lexical_environment_p->container,
                                                                  outer_reference_cp,
                                                                  ECMA_OBJECT_LEX_ENV_OUTER_REFERENCE_CP_POS,
                                                                  ECMA_OBJECT_LEX_ENV_OUTER_REFERENCE_CP_WIDTH);
  ecma_gc_update_may_ref_younger_object_flag_by_object (new_lexical_environment_p, outer_lexical_environment_p);

  /*
   * Declarative lexical environments do not really have the flag,
   * but to not leave the value initialized, setting the flag to false.
   */
  new_lexical_environment_p->container = jrt_set_bit_field_value (new_lexical_environment_p->container,
                                                                  false,
                                                                  ECMA_OBJECT_LEX_ENV_PROVIDE_THIS_POS,
                                                                  ECMA_OBJECT_LEX_ENV_PROVIDE_THIS_WIDTH);

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
ecma_object_t*
ecma_create_object_lex_env (ecma_object_t *outer_lexical_environment_p, /**< outer lexical environment */
                            ecma_object_t *binding_obj_p, /**< binding object */
                            bool provide_this) /**< provideThis flag */
{
  JERRY_ASSERT(binding_obj_p != NULL
               && !ecma_is_lexical_environment (binding_obj_p));

  ecma_object_t *new_lexical_environment_p = ecma_alloc_object ();

  ecma_init_gc_info (new_lexical_environment_p);

  new_lexical_environment_p->container = jrt_set_bit_field_value (new_lexical_environment_p->container,
                                                                  true,
                                                                  ECMA_OBJECT_IS_LEXICAL_ENVIRONMENT_POS,
                                                                  ECMA_OBJECT_IS_LEXICAL_ENVIRONMENT_WIDTH);

  new_lexical_environment_p->container = jrt_set_bit_field_value (new_lexical_environment_p->container,
                                                                  ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND,
                                                                  ECMA_OBJECT_LEX_ENV_TYPE_POS,
                                                                  ECMA_OBJECT_LEX_ENV_TYPE_WIDTH);

  uint64_t outer_reference_cp;
  ECMA_SET_POINTER (outer_reference_cp, outer_lexical_environment_p);
  new_lexical_environment_p->container = jrt_set_bit_field_value (new_lexical_environment_p->container,
                                                                  outer_reference_cp,
                                                                  ECMA_OBJECT_LEX_ENV_OUTER_REFERENCE_CP_POS,
                                                                  ECMA_OBJECT_LEX_ENV_OUTER_REFERENCE_CP_WIDTH);
  ecma_gc_update_may_ref_younger_object_flag_by_object (new_lexical_environment_p, outer_lexical_environment_p);

  new_lexical_environment_p->container = jrt_set_bit_field_value (new_lexical_environment_p->container,
                                                                  provide_this,
                                                                  ECMA_OBJECT_LEX_ENV_PROVIDE_THIS_POS,
                                                                  ECMA_OBJECT_LEX_ENV_PROVIDE_THIS_WIDTH);

  uint64_t bound_object_cp;
  ECMA_SET_NON_NULL_POINTER (bound_object_cp, binding_obj_p);
  new_lexical_environment_p->container = jrt_set_bit_field_value (new_lexical_environment_p->container,
                                                                  bound_object_cp,
                                                                  ECMA_OBJECT_PROPERTIES_OR_BOUND_OBJECT_CP_POS,
                                                                  ECMA_OBJECT_PROPERTIES_OR_BOUND_OBJECT_CP_WIDTH);
  ecma_gc_update_may_ref_younger_object_flag_by_object (new_lexical_environment_p, binding_obj_p);

  return new_lexical_environment_p;
} /* ecma_create_object_lex_env */

/**
 * Check if the object is lexical environment.
 */
bool __attribute_pure__
ecma_is_lexical_environment (const ecma_object_t *object_p) /**< object or lexical environment */
{
  JERRY_ASSERT (object_p != NULL);

  return jrt_extract_bit_field (object_p->container,
                                ECMA_OBJECT_IS_LEXICAL_ENVIRONMENT_POS,
                                ECMA_OBJECT_IS_LEXICAL_ENVIRONMENT_WIDTH);
} /* ecma_is_lexical_environment */

/**
 * Get value of [[Extensible]] object's internal property.
 */
bool __attribute_pure__
ecma_get_object_extensible (const ecma_object_t *object_p) /**< object */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (!ecma_is_lexical_environment (object_p));

  return jrt_extract_bit_field (object_p->container,
                                ECMA_OBJECT_OBJ_EXTENSIBLE_POS,
                                ECMA_OBJECT_OBJ_EXTENSIBLE_WIDTH);
} /* ecma_get_object_extensible */

/**
 * Set value of [[Extensible]] object's internal property.
 */
void
ecma_set_object_extensible (ecma_object_t *object_p, /**< object */
                            bool is_extensible) /**< value of [[Extensible]] */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (!ecma_is_lexical_environment (object_p));

  object_p->container = jrt_set_bit_field_value (object_p->container,
                                                 is_extensible,
                                                 ECMA_OBJECT_OBJ_EXTENSIBLE_POS,
                                                 ECMA_OBJECT_OBJ_EXTENSIBLE_WIDTH);
} /* ecma_set_object_extensible */

/**
 * Get object's internal implementation-defined type.
 */
ecma_object_type_t __attribute_pure__
ecma_get_object_type (const ecma_object_t *object_p) /**< object */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (!ecma_is_lexical_environment (object_p));

  return (ecma_object_type_t) jrt_extract_bit_field (object_p->container,
                                                     ECMA_OBJECT_OBJ_TYPE_POS,
                                                     ECMA_OBJECT_OBJ_TYPE_WIDTH);
} /* ecma_get_object_type */

/**
 * Set object's internal implementation-defined type.
 */
void
ecma_set_object_type (ecma_object_t *object_p, /**< object */
                      ecma_object_type_t type) /**< type */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (!ecma_is_lexical_environment (object_p));

  object_p->container = jrt_set_bit_field_value (object_p->container,
                                                 type,
                                                 ECMA_OBJECT_OBJ_TYPE_POS,
                                                 ECMA_OBJECT_OBJ_TYPE_WIDTH);
} /* ecma_set_object_type */

/**
 * Get object's prototype.
 */
ecma_object_t* __attribute_pure__
ecma_get_object_prototype (const ecma_object_t *object_p) /**< object */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (!ecma_is_lexical_environment (object_p));

  JERRY_ASSERT (sizeof (uintptr_t) * JERRY_BITSINBYTE >= ECMA_OBJECT_OBJ_PROTOTYPE_OBJECT_CP_WIDTH);
  uintptr_t prototype_object_cp = (uintptr_t) jrt_extract_bit_field (object_p->container,
                                                                     ECMA_OBJECT_OBJ_PROTOTYPE_OBJECT_CP_POS,
                                                                     ECMA_OBJECT_OBJ_PROTOTYPE_OBJECT_CP_WIDTH);
  return ECMA_GET_POINTER (ecma_object_t,
                           prototype_object_cp);
} /* ecma_get_object_prototype */

/**
 * Check if the object is a built-in object
 *
 * @return true / false
 */
bool __attribute_pure__
ecma_get_object_is_builtin (const ecma_object_t *object_p) /**< object */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (!ecma_is_lexical_environment (object_p));

  const uint32_t offset = ECMA_OBJECT_OBJ_IS_BUILTIN_POS;
  const uint32_t width = ECMA_OBJECT_OBJ_IS_BUILTIN_WIDTH;

  JERRY_ASSERT (sizeof (uintptr_t) * JERRY_BITSINBYTE >= width);

  uintptr_t flag_value = (uintptr_t) jrt_extract_bit_field (object_p->container,
                                                            offset,
                                                            width);

  return (bool) flag_value;
} /* ecma_get_object_is_builtin */

/**
 * Set flag indicating whether the object is a built-in object
 */
void
ecma_set_object_is_builtin (ecma_object_t *object_p, /**< object */
                            bool is_builtin) /**< value of flag */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (!ecma_is_lexical_environment (object_p));

  const uint32_t offset = ECMA_OBJECT_OBJ_IS_BUILTIN_POS;
  const uint32_t width = ECMA_OBJECT_OBJ_IS_BUILTIN_WIDTH;

  object_p->container = jrt_set_bit_field_value (object_p->container,
                                                 (uintptr_t) is_builtin,
                                                 offset,
                                                 width);
} /* ecma_set_object_is_builtin */

/**
 * Get type of lexical environment.
 */
ecma_lexical_environment_type_t __attribute_pure__
ecma_get_lex_env_type (const ecma_object_t *object_p) /**< lexical environment */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (ecma_is_lexical_environment (object_p));

  return (ecma_lexical_environment_type_t) jrt_extract_bit_field (object_p->container,
                                                                  ECMA_OBJECT_LEX_ENV_TYPE_POS,
                                                                  ECMA_OBJECT_LEX_ENV_TYPE_WIDTH);
} /* ecma_get_lex_env_type */

/**
 * Get outer reference of lexical environment.
 */
ecma_object_t* __attribute_pure__
ecma_get_lex_env_outer_reference (const ecma_object_t *object_p) /**< lexical environment */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (ecma_is_lexical_environment (object_p));

  JERRY_ASSERT (sizeof (uintptr_t) * JERRY_BITSINBYTE >= ECMA_OBJECT_LEX_ENV_OUTER_REFERENCE_CP_WIDTH);
  uintptr_t outer_reference_cp = (uintptr_t) jrt_extract_bit_field (object_p->container,
                                                                    ECMA_OBJECT_LEX_ENV_OUTER_REFERENCE_CP_POS,
                                                                    ECMA_OBJECT_LEX_ENV_OUTER_REFERENCE_CP_WIDTH);
  return ECMA_GET_POINTER (ecma_object_t,
                           outer_reference_cp);
} /* ecma_get_lex_env_outer_reference */

/**
 * Get object's/lexical environment's property list.
 */
ecma_property_t* __attribute_pure__
ecma_get_property_list (const ecma_object_t *object_p) /**< object or lexical environment */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (!ecma_is_lexical_environment (object_p) ||
                ecma_get_lex_env_type (object_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE);

  JERRY_ASSERT (sizeof (uintptr_t) * JERRY_BITSINBYTE >= ECMA_OBJECT_PROPERTIES_OR_BOUND_OBJECT_CP_WIDTH);
  uintptr_t properties_cp = (uintptr_t) jrt_extract_bit_field (object_p->container,
                                                               ECMA_OBJECT_PROPERTIES_OR_BOUND_OBJECT_CP_POS,
                                                               ECMA_OBJECT_PROPERTIES_OR_BOUND_OBJECT_CP_WIDTH);
  return ECMA_GET_POINTER (ecma_property_t,
                           properties_cp);
} /* ecma_get_property_list */

/**
 * Set object's/lexical environment's property list.
 */
static void
ecma_set_property_list (ecma_object_t *object_p, /**< object or lexical environment */
                        ecma_property_t *property_list_p) /**< properties' list */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (!ecma_is_lexical_environment (object_p) ||
                ecma_get_lex_env_type (object_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE);

  uint64_t properties_cp;
  ECMA_SET_POINTER (properties_cp, property_list_p);

  object_p->container = jrt_set_bit_field_value (object_p->container,
                                                 properties_cp,
                                                 ECMA_OBJECT_PROPERTIES_OR_BOUND_OBJECT_CP_POS,
                                                 ECMA_OBJECT_PROPERTIES_OR_BOUND_OBJECT_CP_WIDTH);
} /* ecma_set_property_list */

/**
 * Get lexical environment's 'provideThis' property
 */
bool __attribute_pure__
ecma_get_lex_env_provide_this (const ecma_object_t *object_p) /**< object-bound lexical environment */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (ecma_is_lexical_environment (object_p) &&
                ecma_get_lex_env_type (object_p) == ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND);

  JERRY_ASSERT (sizeof (uintptr_t) * JERRY_BITSINBYTE >= ECMA_OBJECT_PROPERTIES_OR_BOUND_OBJECT_CP_WIDTH);
  bool provide_this = (jrt_extract_bit_field (object_p->container,
                                              ECMA_OBJECT_LEX_ENV_PROVIDE_THIS_POS,
                                              ECMA_OBJECT_LEX_ENV_PROVIDE_THIS_WIDTH) != 0);

  return provide_this;
} /* ecma_get_lex_env_provide_this */

/**
 * Get lexical environment's bound object.
 */
ecma_object_t* __attribute_pure__
ecma_get_lex_env_binding_object (const ecma_object_t *object_p) /**< object-bound lexical environment */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (ecma_is_lexical_environment (object_p) &&
                ecma_get_lex_env_type (object_p) == ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND);

  JERRY_ASSERT (sizeof (uintptr_t) * JERRY_BITSINBYTE >= ECMA_OBJECT_PROPERTIES_OR_BOUND_OBJECT_CP_WIDTH);
  uintptr_t object_cp = (uintptr_t) jrt_extract_bit_field (object_p->container,
                                                           ECMA_OBJECT_PROPERTIES_OR_BOUND_OBJECT_CP_POS,
                                                           ECMA_OBJECT_PROPERTIES_OR_BOUND_OBJECT_CP_WIDTH);
  return ECMA_GET_NON_NULL_POINTER (ecma_object_t, object_cp);
} /* ecma_get_lex_env_binding_object */

/**
 * Create internal property in an object and link it into
 * the object's properties' linked-list (at start of the list).
 *
 * @return pointer to newly created property
 */
ecma_property_t*
ecma_create_internal_property (ecma_object_t *object_p, /**< the object */
                               ecma_internal_property_id_t property_id) /**< internal property identifier */
{
  JERRY_ASSERT (ecma_find_internal_property (object_p, property_id) == NULL);

  ecma_property_t *new_property_p = ecma_alloc_property ();

  new_property_p->type = ECMA_PROPERTY_INTERNAL;

  ecma_property_t *list_head_p = ecma_get_property_list (object_p);
  ECMA_SET_POINTER(new_property_p->next_property_p, list_head_p);
  ecma_set_property_list (object_p, new_property_p);

  JERRY_STATIC_ASSERT (ECMA_INTERNAL_PROPERTY__COUNT <= (1ull << ECMA_PROPERTY_INTERNAL_PROPERTY_TYPE_WIDTH));
  JERRY_ASSERT (property_id < ECMA_INTERNAL_PROPERTY__COUNT);

  new_property_p->u.internal_property.type = property_id & ((1ull << ECMA_PROPERTY_INTERNAL_PROPERTY_TYPE_WIDTH) - 1);
  new_property_p->u.internal_property.value = ECMA_NULL_POINTER;

  return new_property_p;
} /* ecma_create_internal_property */

/**
 * Find internal property in the object's property set.
 *
 * @return pointer to the property, if it is found,
 *         NULL - otherwise.
 */
ecma_property_t*
ecma_find_internal_property (ecma_object_t *object_p, /**< object descriptor */
                             ecma_internal_property_id_t property_id) /**< internal property identifier */
{
  JERRY_ASSERT(object_p != NULL);

  JERRY_ASSERT(property_id != ECMA_INTERNAL_PROPERTY_PROTOTYPE
               && property_id != ECMA_INTERNAL_PROPERTY_EXTENSIBLE);

  for (ecma_property_t *property_p = ecma_get_property_list (object_p);
       property_p != NULL;
       property_p = ECMA_GET_POINTER (ecma_property_t, property_p->next_property_p))
  {
    if (property_p->type == ECMA_PROPERTY_INTERNAL)
    {
      if (property_p->u.internal_property.type == property_id)
      {
        return property_p;
      }
    }
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
ecma_property_t*
ecma_get_internal_property (ecma_object_t *object_p, /**< object descriptor */
                            ecma_internal_property_id_t property_id) /**< internal property identifier */
{
  ecma_property_t *property_p = ecma_find_internal_property (object_p, property_id);

  JERRY_ASSERT(property_p != NULL);

  return property_p;
} /* ecma_get_internal_property */

/**
 * Create named data property with given name, attributes and undefined value
 * in the specified object.
 *
 * @return pointer to newly created property
 */
ecma_property_t*
ecma_create_named_data_property (ecma_object_t *obj_p, /**< object */
                                 ecma_string_t *name_p, /**< property name */
                                 bool is_writable, /**< 'Writable' attribute */
                                 bool is_enumerable, /**< 'Enumerable' attribute */
                                 bool is_configurable) /**< 'Configurable' attribute */
{
  JERRY_ASSERT(obj_p != NULL && name_p != NULL);
  JERRY_ASSERT(ecma_find_named_property (obj_p, name_p) == NULL);

  ecma_property_t *prop_p = ecma_alloc_property ();

  prop_p->type = ECMA_PROPERTY_NAMEDDATA;

  name_p = ecma_copy_or_ref_ecma_string (name_p);
  ECMA_SET_NON_NULL_POINTER(prop_p->u.named_data_property.name_p, name_p);

  prop_p->u.named_data_property.writable = is_writable ? ECMA_PROPERTY_WRITABLE : ECMA_PROPERTY_NOT_WRITABLE;
  prop_p->u.named_data_property.enumerable = is_enumerable ? ECMA_PROPERTY_ENUMERABLE : ECMA_PROPERTY_NOT_ENUMERABLE;
  prop_p->u.named_data_property.configurable = (is_configurable ?
                                                ECMA_PROPERTY_CONFIGURABLE : ECMA_PROPERTY_NOT_CONFIGURABLE);

  prop_p->u.named_data_property.is_lcached = false;

  prop_p->u.named_data_property.value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

  ecma_lcache_invalidate (obj_p, name_p, NULL);

  ecma_property_t *list_head_p = ecma_get_property_list (obj_p);
  ECMA_SET_POINTER(prop_p->next_property_p, list_head_p);
  ecma_set_property_list (obj_p, prop_p);

  return prop_p;
} /* ecma_create_named_data_property */

/**
 * Create named accessor property with given name, attributes, getter and setter.
 *
 * @return pointer to newly created property
 */
ecma_property_t*
ecma_create_named_accessor_property (ecma_object_t *obj_p, /**< object */
                                     ecma_string_t *name_p, /**< property name */
                                     ecma_object_t *get_p, /**< getter */
                                     ecma_object_t *set_p, /**< setter */
                                     bool is_enumerable, /**< 'enumerable' attribute */
                                     bool is_configurable) /**< 'configurable' attribute */
{
  JERRY_ASSERT(obj_p != NULL && name_p != NULL);
  JERRY_ASSERT(ecma_find_named_property (obj_p, name_p) == NULL);

  ecma_property_t *prop_p = ecma_alloc_property ();

  prop_p->type = ECMA_PROPERTY_NAMEDACCESSOR;

  name_p = ecma_copy_or_ref_ecma_string (name_p);
  ECMA_SET_NON_NULL_POINTER(prop_p->u.named_accessor_property.name_p, name_p);

  ECMA_SET_POINTER(prop_p->u.named_accessor_property.get_p, get_p);
  ecma_gc_update_may_ref_younger_object_flag_by_object (obj_p, get_p);

  ECMA_SET_POINTER(prop_p->u.named_accessor_property.set_p, set_p);
  ecma_gc_update_may_ref_younger_object_flag_by_object (obj_p, set_p);

  prop_p->u.named_accessor_property.enumerable = (is_enumerable ?
                                                  ECMA_PROPERTY_ENUMERABLE : ECMA_PROPERTY_NOT_ENUMERABLE);
  prop_p->u.named_accessor_property.configurable = (is_configurable ?
                                                    ECMA_PROPERTY_CONFIGURABLE : ECMA_PROPERTY_NOT_CONFIGURABLE);

  prop_p->u.named_accessor_property.is_lcached = false;

  ecma_lcache_invalidate (obj_p, name_p, NULL);

  ecma_property_t *list_head_p = ecma_get_property_list (obj_p);
  ECMA_SET_POINTER(prop_p->next_property_p, list_head_p);
  ecma_set_property_list (obj_p, prop_p);

  return prop_p;
} /* ecma_create_named_accessor_property */

/**
 * Find named data property or named access property in specified object.
 *
 * @return pointer to the property, if it is found,
 *         NULL - otherwise.
 */
ecma_property_t*
ecma_find_named_property (ecma_object_t *obj_p, /**< object to find property in */
                          ecma_string_t *name_p) /**< property's name */
{
  JERRY_ASSERT(obj_p != NULL);
  JERRY_ASSERT(name_p != NULL);

  ecma_property_t *property_p;

  if (ecma_lcache_lookup (obj_p, name_p, &property_p))
  {
    return property_p;
  }

  for (property_p = ecma_get_property_list (obj_p);
       property_p != NULL;
       property_p = ECMA_GET_POINTER (ecma_property_t, property_p->next_property_p))
  {
    ecma_string_t *property_name_p;

    if (property_p->type == ECMA_PROPERTY_NAMEDDATA)
    {
      property_name_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                   property_p->u.named_data_property.name_p);
    }
    else if (property_p->type == ECMA_PROPERTY_NAMEDACCESSOR)
    {
      property_name_p = ECMA_GET_NON_NULL_POINTER(ecma_string_t,
                                                  property_p->u.named_accessor_property.name_p);
    }
    else
    {
      continue;
    }

    JERRY_ASSERT(property_name_p != NULL);

    if (ecma_compare_ecma_strings (name_p, property_name_p))
    {
      break;
    }
  }

  ecma_lcache_insert (obj_p, name_p, property_p);

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
ecma_property_t*
ecma_get_named_property (ecma_object_t *obj_p, /**< object to find property in */
                         ecma_string_t *name_p) /**< property's name */
{
  JERRY_ASSERT(obj_p != NULL);
  JERRY_ASSERT(name_p != NULL);

  ecma_property_t *property_p = ecma_find_named_property (obj_p, name_p);

  JERRY_ASSERT(property_p != NULL);

  return property_p;
} /* ecma_get_named_property */

/**
 * Get named data property in specified object.
 *
 * Warning:
 *         the property must exist and be named data property
 *
 * @return pointer to the property, if it is found,
 *         NULL - otherwise.
 */
ecma_property_t*
ecma_get_named_data_property (ecma_object_t *obj_p, /**< object to find property in */
                              ecma_string_t *name_p) /**< property's name */
{
  JERRY_ASSERT (obj_p != NULL);
  JERRY_ASSERT (name_p != NULL);

  ecma_property_t *property_p = ecma_find_named_property (obj_p, name_p);

  JERRY_ASSERT (property_p != NULL && property_p->type == ECMA_PROPERTY_NAMEDDATA);

  return property_p;
} /* ecma_get_named_data_property */

/**
 * Free the named data property and values it references.
 */
static void
ecma_free_named_data_property (ecma_object_t *object_p, /**< object the property belongs to */
                               ecma_property_t *property_p) /**< the property */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (property_p != NULL && property_p->type == ECMA_PROPERTY_NAMEDDATA);

  ecma_lcache_invalidate (object_p, NULL, property_p);

  ecma_deref_ecma_string (ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                     property_p->u.named_data_property.name_p));

  ecma_value_t v = property_p->u.named_data_property.value;
  ecma_free_value (v, false);

  ecma_dealloc_property (property_p);
} /* ecma_free_named_data_property */

/**
 * Free the named accessor property and values it references.
 */
static void
ecma_free_named_accessor_property (ecma_object_t *object_p, /**< object the property belongs to */
                                   ecma_property_t *property_p) /**< the property */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (property_p != NULL && property_p->type == ECMA_PROPERTY_NAMEDACCESSOR);

  ecma_lcache_invalidate (object_p, NULL, property_p);

  ecma_deref_ecma_string (ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                     property_p->u.named_accessor_property.name_p));

  ecma_dealloc_property (property_p);
} /* ecma_free_named_accessor_property */

/**
 * Free the internal property and values it references.
 */
static void
ecma_free_internal_property (ecma_property_t *property_p) /**< the property */
{
  JERRY_ASSERT (property_p != NULL && property_p->type == ECMA_PROPERTY_INTERNAL);

  ecma_internal_property_id_t property_id = (ecma_internal_property_id_t) property_p->u.internal_property.type;
  uint32_t property_value = property_p->u.internal_property.value;

  switch (property_id)
  {
    case ECMA_INTERNAL_PROPERTY_NUMBER_INDEXED_ARRAY_VALUES: /* a collection */
    case ECMA_INTERNAL_PROPERTY_STRING_INDEXED_ARRAY_VALUES: /* a collection */
    {
      ecma_free_values_collection (ECMA_GET_NON_NULL_POINTER (ecma_collection_header_t,
                                                              property_value),
                                   true);

      break;
    }

    case ECMA_INTERNAL_PROPERTY_FORMAL_PARAMETERS: /* a strings' collection */
    {
      if (property_value != ECMA_NULL_POINTER)
      {
        ecma_free_values_collection (ECMA_GET_NON_NULL_POINTER (ecma_collection_header_t,
                                                                property_value),
                                     false);
      }
      break;
    }

    case ECMA_INTERNAL_PROPERTY_PRIMITIVE_STRING_VALUE: /* compressed pointer to a ecma_string_t */
    {
      ecma_string_t *str_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                        property_value);
      ecma_deref_ecma_string (str_p);

      break;
    }

    case ECMA_INTERNAL_PROPERTY_PRIMITIVE_NUMBER_VALUE: /* pointer to a ecma_number_t */
    {
      ecma_number_t *num_p = ECMA_GET_NON_NULL_POINTER (ecma_number_t,
                                                        property_value);
      ecma_dealloc_number (num_p);

      break;
    }

    case ECMA_INTERNAL_PROPERTY_PRIMITIVE_BOOLEAN_VALUE: /* a simple boolean value */
    case ECMA_INTERNAL_PROPERTY_SCOPE: /* a lexical environment */
    case ECMA_INTERNAL_PROPERTY_PARAMETERS_MAP: /* an object */
    case ECMA_INTERNAL_PROPERTY_PROTOTYPE: /* the property's value is located in ecma_object_t */
    case ECMA_INTERNAL_PROPERTY_EXTENSIBLE: /* the property's value is located in ecma_object_t */
    case ECMA_INTERNAL_PROPERTY_CLASS: /* an enum */
    case ECMA_INTERNAL_PROPERTY_CODE: /* an integer */
    case ECMA_INTERNAL_PROPERTY_BUILT_IN_ID: /* an integer */
    case ECMA_INTERNAL_PROPERTY_BUILT_IN_ROUTINE_ID: /* an integer */
    case ECMA_INTERNAL_PROPERTY_NON_INSTANTIATED_BUILT_IN_MASK_0_31: /* an integer (bit-mask) */
    case ECMA_INTERNAL_PROPERTY_NON_INSTANTIATED_BUILT_IN_MASK_32_63: /* an integer (bit-mask) */
    {
      break;
    }

    case ECMA_INTERNAL_PROPERTY__COUNT: /* not a real internal property type,
                                         * but number of the real internal property types */
    {
      JERRY_UNREACHABLE ();
    }
  }

  ecma_dealloc_property (property_p);
} /* ecma_free_internal_property */

/**
 * Free the property and values it references.
 */
void
ecma_free_property (ecma_object_t *object_p, /**< object the property belongs to */
                    ecma_property_t *prop_p) /**< property */
{
  switch ((ecma_property_type_t) prop_p->type)
  {
    case ECMA_PROPERTY_NAMEDDATA:
    {
      ecma_free_named_data_property (object_p, prop_p);

      break;
    }

    case ECMA_PROPERTY_NAMEDACCESSOR:
    {
      ecma_free_named_accessor_property (object_p, prop_p);

      break;
    }

    case ECMA_PROPERTY_INTERNAL:
    {
      ecma_free_internal_property (prop_p);

      break;
    }
  }
} /* ecma_free_property */

/**
 * Delete the object's property.
 *
 * Warning: specified property must be owned by specified object.
 */
void
ecma_delete_property (ecma_object_t *obj_p, /**< object */
                      ecma_property_t *prop_p) /**< property */
{
  for (ecma_property_t *cur_prop_p = ecma_get_property_list (obj_p), *prev_prop_p = NULL, *next_prop_p;
       cur_prop_p != NULL;
       prev_prop_p = cur_prop_p, cur_prop_p = next_prop_p)
  {
    next_prop_p = ECMA_GET_POINTER (ecma_property_t,
                                    cur_prop_p->next_property_p);

    if (cur_prop_p == prop_p)
    {
      ecma_free_property (obj_p, prop_p);

      if (prev_prop_p == NULL)
      {
        ecma_set_property_list (obj_p, next_prop_p);
      }
      else
      {
        ECMA_SET_POINTER(prev_prop_p->next_property_p, next_prop_p);
      }

      return;
    }
  }

  JERRY_UNREACHABLE();
} /* ecma_delete_property */

/**
 * Get value field of named data property
 *
 * @return ecma-value
 */
ecma_value_t
ecma_get_named_data_property_value (const ecma_property_t *prop_p) /**< property */
{
  JERRY_ASSERT (prop_p->type == ECMA_PROPERTY_NAMEDDATA);

  return prop_p->u.named_data_property.value;
} /* ecma_get_named_data_property_value */

/**
 * Set value field of named data property
 */
void
ecma_set_named_data_property_value (ecma_property_t *prop_p, /**< property */
                                    const ecma_value_t& value) /**< value to set */
{
  JERRY_ASSERT (prop_p->type == ECMA_PROPERTY_NAMEDDATA);

  /* 'May ref younger' flag should be updated upon assignment of object value */
  JERRY_ASSERT (!ecma_is_value_object (value));

  prop_p->u.named_data_property.value = value;
} /* ecma_set_named_data_property_value */

/**
 * Assign value to named data property
 *
 * Note:
 *      value previously stored in the property is freed
 */
void
ecma_named_data_property_assign_value (ecma_object_t *obj_p, /**< object */
                                       ecma_property_t *prop_p, /**< property */
                                       const ecma_value_t& value) /**< value to assign */
{
  JERRY_ASSERT (prop_p->type == ECMA_PROPERTY_NAMEDDATA);
#ifndef JERRY_NDEBUG
  ecma_property_t *prop_iter_p;
  for (prop_iter_p = ecma_get_property_list (obj_p);
       prop_iter_p != NULL;
       prop_iter_p = ECMA_GET_POINTER (ecma_property_t, prop_iter_p->next_property_p))
  {
    if (prop_iter_p == prop_p)
    {
      break;
    }
  }

  JERRY_ASSERT (prop_iter_p != NULL);
#endif /* !JERRY_NDEBUG */

  if (ecma_is_value_number (value)
      && ecma_is_value_number (prop_p->u.named_data_property.value))
  {
    const ecma_number_t *num_src_p = ecma_get_number_from_value (value);
    ecma_number_t *num_dst_p = ecma_get_number_from_value (prop_p->u.named_data_property.value);

    *num_dst_p = *num_src_p;
  }
  else
  {
    ecma_value_t v = ecma_get_named_data_property_value (prop_p);
    ecma_free_value (v, false);

    prop_p->u.named_data_property.value = ecma_copy_value (value, false);
    ecma_gc_update_may_ref_younger_object_flag_by_value (obj_p,
                                                         prop_p->u.named_data_property.value);
  }
} /* ecma_named_data_property_assign_value */

/**
 * Get property's 'Writable' attribute value
 *
 * @return true - property is writable,
 *         false - otherwise.
 */
bool
ecma_is_property_writable (ecma_property_t* prop_p) /**< property */
{
  JERRY_ASSERT (prop_p->type == ECMA_PROPERTY_NAMEDDATA);

  return (prop_p->u.named_data_property.writable == ECMA_PROPERTY_WRITABLE);
} /* ecma_is_property_writable */

/**
 * Set property's 'Writable' attribute value
 */
void
ecma_set_property_writable_attr (ecma_property_t* prop_p, /**< property */
                                 bool is_writable) /**< should the property
                                                    *  be writable? */
{
  JERRY_ASSERT (prop_p->type == ECMA_PROPERTY_NAMEDDATA);

  prop_p->u.named_data_property.writable = is_writable ? ECMA_PROPERTY_WRITABLE : ECMA_PROPERTY_NOT_WRITABLE;
} /* ecma_set_property_writable_attr */

/**
 * Get property's 'Enumerable' attribute value
 *
 * @return true - property is enumerable,
 *         false - otherwise.
 */
bool
ecma_is_property_enumerable (ecma_property_t* prop_p) /**< property */
{
  if (prop_p->type == ECMA_PROPERTY_NAMEDDATA)
  {
    return (prop_p->u.named_data_property.enumerable == ECMA_PROPERTY_ENUMERABLE);
  }
  else
  {
    JERRY_ASSERT (prop_p->type == ECMA_PROPERTY_NAMEDACCESSOR);

    return (prop_p->u.named_accessor_property.enumerable == ECMA_PROPERTY_ENUMERABLE);
  }
} /* ecma_is_property_enumerable */

/**
 * Set property's 'Enumerable' attribute value
 */
void
ecma_set_property_enumerable_attr (ecma_property_t* prop_p, /**< property */
                                   bool is_enumerable) /**< should the property
                                                        *  be enumerable? */
{
  if (prop_p->type == ECMA_PROPERTY_NAMEDDATA)
  {
    prop_p->u.named_data_property.enumerable = (is_enumerable ?
                                                ECMA_PROPERTY_ENUMERABLE : ECMA_PROPERTY_NOT_ENUMERABLE);
  }
  else
  {
    JERRY_ASSERT (prop_p->type == ECMA_PROPERTY_NAMEDACCESSOR);

    prop_p->u.named_accessor_property.enumerable = (is_enumerable ?
                                                    ECMA_PROPERTY_ENUMERABLE : ECMA_PROPERTY_NOT_ENUMERABLE);
  }
} /* ecma_set_property_enumerable_attr */

/**
 * Get property's 'Configurable' attribute value
 *
 * @return true - property is configurable,
 *         false - otherwise.
 */
bool
ecma_is_property_configurable (ecma_property_t* prop_p) /**< property */
{
  if (prop_p->type == ECMA_PROPERTY_NAMEDDATA)
  {
    return (prop_p->u.named_data_property.configurable == ECMA_PROPERTY_CONFIGURABLE);
  }
  else
  {
    JERRY_ASSERT (prop_p->type == ECMA_PROPERTY_NAMEDACCESSOR);

    return (prop_p->u.named_accessor_property.configurable == ECMA_PROPERTY_CONFIGURABLE);
  }
} /* ecma_is_property_configurable */

/**
 * Set property's 'Configurable' attribute value
 */
void
ecma_set_property_configurable_attr (ecma_property_t* prop_p, /**< property */
                                     bool is_configurable) /**< should the property
                                                            *  be configurable? */
{
  if (prop_p->type == ECMA_PROPERTY_NAMEDDATA)
  {
    prop_p->u.named_data_property.configurable = (is_configurable ?
                                                  ECMA_PROPERTY_CONFIGURABLE : ECMA_PROPERTY_NOT_CONFIGURABLE);
  }
  else
  {
    JERRY_ASSERT (prop_p->type == ECMA_PROPERTY_NAMEDACCESSOR);

    prop_p->u.named_accessor_property.configurable = (is_configurable ?
                                                      ECMA_PROPERTY_CONFIGURABLE : ECMA_PROPERTY_NOT_CONFIGURABLE);
  }
} /* ecma_set_property_configurable_attr */

/**
 * Check whether the property is registered in LCache
 *
 * @return true / false
 */
bool
ecma_is_property_lcached (ecma_property_t *prop_p) /**< property */
{
  JERRY_ASSERT (prop_p->type == ECMA_PROPERTY_NAMEDDATA
                || prop_p->type == ECMA_PROPERTY_NAMEDACCESSOR);

  if (prop_p->type == ECMA_PROPERTY_NAMEDDATA)
  {
    return prop_p->u.named_data_property.is_lcached;
  }
  else
  {
    return prop_p->u.named_accessor_property.is_lcached;
  }
} /* ecma_is_property_lcached */

/**
 * Set value of flag indicating whether the property is registered in LCache
 */
void
ecma_set_property_lcached (ecma_property_t *prop_p, /**< property */
                           bool is_lcached) /**< contained (true) or not (false) */
{
  JERRY_ASSERT (prop_p->type == ECMA_PROPERTY_NAMEDDATA
                || prop_p->type == ECMA_PROPERTY_NAMEDACCESSOR);

  if (prop_p->type == ECMA_PROPERTY_NAMEDDATA)
  {
    prop_p->u.named_data_property.is_lcached = is_lcached;
  }
  else
  {
    prop_p->u.named_accessor_property.is_lcached = is_lcached;
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
    ecma_free_value (prop_desc_p->value, true);
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
 * @}
 * @}
 */
