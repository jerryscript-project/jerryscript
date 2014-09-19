/* Copyright 2014 Samsung Electronics Co., Ltd.
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
                                                 ECMA_OBJECT_PROPERTIES_CP_POS,
                                                 ECMA_OBJECT_PROPERTIES_CP_WIDTH);
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
                                                                  ECMA_OBJECT_PROPERTIES_CP_POS,
                                                                  ECMA_OBJECT_PROPERTIES_CP_WIDTH);
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
                                                                  ECMA_NULL_POINTER,
                                                                  ECMA_OBJECT_PROPERTIES_CP_POS,
                                                                  ECMA_OBJECT_PROPERTIES_CP_WIDTH);
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

  ecma_property_t *provide_this_prop_p = ecma_create_internal_property (new_lexical_environment_p,
                                                                        ECMA_INTERNAL_PROPERTY_PROVIDE_THIS);
  provide_this_prop_p->u.internal_property.value = provide_this;

  ecma_property_t *binding_object_prop_p = ecma_create_internal_property (new_lexical_environment_p,
                                                                          ECMA_INTERNAL_PROPERTY_BINDING_OBJECT);
  ECMA_SET_POINTER(binding_object_prop_p->u.internal_property.value, binding_obj_p);
  ecma_gc_update_may_ref_younger_object_flag_by_object (new_lexical_environment_p, binding_obj_p);

  return new_lexical_environment_p;
} /* ecma_create_object_lex_env */

/**
 * Check if the object is lexical environment.
 */
bool
ecma_is_lexical_environment (ecma_object_t *object_p) /**< object or lexical environment */
{
  JERRY_ASSERT (object_p != NULL);

  return jrt_extract_bit_field (object_p->container,
                                ECMA_OBJECT_IS_LEXICAL_ENVIRONMENT_POS,
                                ECMA_OBJECT_IS_LEXICAL_ENVIRONMENT_WIDTH);
} /* ecma_is_lexical_environment */

/**
 * Get value of [[Extensible]] object's internal property.
 */
bool
ecma_get_object_extensible (ecma_object_t *object_p) /**< object */
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
ecma_object_type_t
ecma_get_object_type (ecma_object_t *object_p) /**< object */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (!ecma_is_lexical_environment (object_p));

  return jrt_extract_bit_field (object_p->container,
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
ecma_object_t*
ecma_get_object_prototype (ecma_object_t *object_p) /**< object */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (!ecma_is_lexical_environment (object_p));

  JERRY_ASSERT (sizeof (uintptr_t) * JERRY_BITSINBYTE >= ECMA_OBJECT_PROPERTIES_CP_WIDTH);
  uintptr_t prototype_object_cp = (uintptr_t) jrt_extract_bit_field (object_p->container,
                                                                     ECMA_OBJECT_OBJ_PROTOTYPE_OBJECT_CP_POS,
                                                                     ECMA_OBJECT_OBJ_PROTOTYPE_OBJECT_CP_WIDTH);
  return ECMA_GET_POINTER (prototype_object_cp);
} /* ecma_get_object_prototype */

/**
 * Check if the object is a built-in object
 *
 * @return true / false
 */
bool
ecma_get_object_is_builtin (ecma_object_t *object_p) /**< object */
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
ecma_lexical_environment_type_t
ecma_get_lex_env_type (ecma_object_t *object_p) /**< lexical environment */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (ecma_is_lexical_environment (object_p));

  return jrt_extract_bit_field (object_p->container,
                                ECMA_OBJECT_LEX_ENV_TYPE_POS,
                                ECMA_OBJECT_LEX_ENV_TYPE_WIDTH);
} /* ecma_get_lex_env_type */

/**
 * Get outer reference of lexical environment.
 */
ecma_object_t*
ecma_get_lex_env_outer_reference (ecma_object_t *object_p) /**< lexical environment */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (ecma_is_lexical_environment (object_p));

  JERRY_ASSERT (sizeof (uintptr_t) * JERRY_BITSINBYTE >= ECMA_OBJECT_PROPERTIES_CP_WIDTH);
  uintptr_t outer_reference_cp = (uintptr_t) jrt_extract_bit_field (object_p->container,
                                                                    ECMA_OBJECT_LEX_ENV_OUTER_REFERENCE_CP_POS,
                                                                    ECMA_OBJECT_LEX_ENV_OUTER_REFERENCE_CP_WIDTH);
  return ECMA_GET_POINTER (outer_reference_cp);
} /* ecma_get_lex_env_outer_reference */

/**
 * Get object's/lexical environment's property list.
 */
ecma_property_t*
ecma_get_property_list (ecma_object_t *object_p) /**< object or lexical environment */
{
  JERRY_ASSERT (object_p != NULL);

  JERRY_ASSERT (sizeof (uintptr_t) * JERRY_BITSINBYTE >= ECMA_OBJECT_PROPERTIES_CP_WIDTH);
  uintptr_t properties_cp = (uintptr_t) jrt_extract_bit_field (object_p->container,
                                                               ECMA_OBJECT_PROPERTIES_CP_POS,
                                                               ECMA_OBJECT_PROPERTIES_CP_WIDTH);
  return ECMA_GET_POINTER (properties_cp);
} /* ecma_get_property_list */

/**
 * Set object's/lexical environment's property list.
 */
static void
ecma_set_property_list (ecma_object_t *object_p, /**< object or lexical environment */
                        ecma_property_t *property_list_p) /**< properties' list */
{
  JERRY_ASSERT (object_p != NULL);

  uint64_t properties_cp;
  ECMA_SET_POINTER (properties_cp, property_list_p);

  object_p->container = jrt_set_bit_field_value (object_p->container,
                                                 properties_cp,
                                                 ECMA_OBJECT_PROPERTIES_CP_POS,
                                                 ECMA_OBJECT_PROPERTIES_CP_WIDTH);
} /* ecma_set_property_list */

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
  ecma_property_t *new_property_p = ecma_alloc_property ();

  new_property_p->type = ECMA_PROPERTY_INTERNAL;

  ecma_property_t *list_head_p = ecma_get_property_list (object_p);
  ECMA_SET_POINTER(new_property_p->next_property_p, list_head_p);
  ecma_set_property_list (object_p, new_property_p);

  new_property_p->u.internal_property.type = property_id;
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
       property_p = ECMA_GET_POINTER(property_p->next_property_p))
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
                                 ecma_property_writable_value_t writable, /**< 'writable' attribute */
                                 ecma_property_enumerable_value_t enumerable, /**< 'enumerable' attribute */
                                 ecma_property_configurable_value_t configurable) /**< 'configurable' attribute */
{
  JERRY_ASSERT(obj_p != NULL && name_p != NULL);

  ecma_property_t *prop_p = ecma_alloc_property ();

  prop_p->type = ECMA_PROPERTY_NAMEDDATA;

  ecma_ref_ecma_string (name_p);
  ECMA_SET_NON_NULL_POINTER(prop_p->u.named_data_property.name_p, name_p);

  prop_p->u.named_data_property.writable = writable;
  prop_p->u.named_data_property.enumerable = enumerable;
  prop_p->u.named_data_property.configurable = configurable;

  prop_p->u.named_data_property.value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

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
                                     ecma_property_enumerable_value_t enumerable, /**< 'enumerable' attribute */
                                     ecma_property_configurable_value_t configurable) /**< 'configurable' attribute */
{
  JERRY_ASSERT(obj_p != NULL && name_p != NULL);

  ecma_property_t *prop_p = ecma_alloc_property ();

  prop_p->type = ECMA_PROPERTY_NAMEDACCESSOR;

  ecma_ref_ecma_string (name_p);
  ECMA_SET_NON_NULL_POINTER(prop_p->u.named_accessor_property.name_p, name_p);

  ECMA_SET_POINTER(prop_p->u.named_accessor_property.get_p, get_p);
  ecma_gc_update_may_ref_younger_object_flag_by_object (obj_p, get_p);

  ECMA_SET_POINTER(prop_p->u.named_accessor_property.set_p, set_p);
  ecma_gc_update_may_ref_younger_object_flag_by_object (obj_p, set_p);

  prop_p->u.named_accessor_property.enumerable = enumerable;
  prop_p->u.named_accessor_property.configurable = configurable;

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
                          const ecma_string_t *name_p) /**< property's name */
{
  JERRY_ASSERT(obj_p != NULL);
  JERRY_ASSERT(name_p != NULL);

  for (ecma_property_t *property_p = ecma_get_property_list (obj_p);
       property_p != NULL;
       property_p = ECMA_GET_POINTER(property_p->next_property_p))
  {
    ecma_string_t *property_name_p;

    if (property_p->type == ECMA_PROPERTY_NAMEDDATA)
    {
      property_name_p = ECMA_GET_POINTER(property_p->u.named_data_property.name_p);
    }
    else if (property_p->type == ECMA_PROPERTY_NAMEDACCESSOR)
    {
      property_name_p = ECMA_GET_POINTER(property_p->u.named_accessor_property.name_p);
    }
    else
    {
      continue;
    }

    JERRY_ASSERT(property_name_p != NULL);

    if (ecma_compare_ecma_strings (name_p, property_name_p))
    {
      return property_p;
    }
  }

  return NULL;
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
                         const ecma_string_t *name_p) /**< property's name */
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
                              const ecma_string_t *name_p) /**< property's name */
{
  JERRY_ASSERT(obj_p != NULL);
  JERRY_ASSERT(name_p != NULL);

  ecma_property_t *property_p = ecma_find_named_property (obj_p, name_p);

  JERRY_ASSERT(property_p != NULL && property_p->type == ECMA_PROPERTY_NAMEDDATA);

  return property_p;
} /* ecma_get_named_data_property */

/**
 * Free the named data property and values it references.
 */
void
ecma_free_named_data_property (ecma_property_t *property_p) /**< the property */
{
  JERRY_ASSERT(property_p->type == ECMA_PROPERTY_NAMEDDATA);

  ecma_deref_ecma_string (ECMA_GET_POINTER (property_p->u.named_data_property.name_p));
  ecma_free_value (property_p->u.named_data_property.value, false);

  ecma_dealloc_property (property_p);
} /* ecma_free_named_data_property */

/**
 * Free the named accessor property and values it references.
 */
void
ecma_free_named_accessor_property (ecma_property_t *property_p) /**< the property */
{
  JERRY_ASSERT(property_p->type == ECMA_PROPERTY_NAMEDACCESSOR);

  ecma_deref_ecma_string (ECMA_GET_POINTER (property_p->u.named_accessor_property.name_p));

  ecma_dealloc_property (property_p);
} /* ecma_free_named_accessor_property */

/**
 * Free the internal property and values it references.
 */
void
ecma_free_internal_property (ecma_property_t *property_p) /**< the property */
{
  JERRY_ASSERT(property_p->type == ECMA_PROPERTY_INTERNAL);

  ecma_internal_property_id_t property_id = property_p->u.internal_property.type;
  uint32_t property_value = property_p->u.internal_property.value;

  switch (property_id)
  {
    case ECMA_INTERNAL_PROPERTY_NUMBER_INDEXED_ARRAY_VALUES: /* a collection */
    case ECMA_INTERNAL_PROPERTY_STRING_INDEXED_ARRAY_VALUES: /* a collection */
    {
      ecma_free_values_collection (ECMA_GET_POINTER(property_value), true);

      break;
    }

    case ECMA_INTERNAL_PROPERTY_FORMAL_PARAMETERS: /* a strings' collection */
    {
      if (property_value != ECMA_NULL_POINTER)
      {
        ecma_free_values_collection (ECMA_GET_POINTER(property_value), false);
      }
      break;
    }

    case ECMA_INTERNAL_PROPERTY_SCOPE: /* a lexical environment */
    case ECMA_INTERNAL_PROPERTY_PARAMETERS_MAP: /* an object */
    case ECMA_INTERNAL_PROPERTY_BINDING_OBJECT: /* an object */
    case ECMA_INTERNAL_PROPERTY_PROTOTYPE: /* the property's value is located in ecma_object_t */
    case ECMA_INTERNAL_PROPERTY_EXTENSIBLE: /* the property's value is located in ecma_object_t */
    case ECMA_INTERNAL_PROPERTY_PROVIDE_THIS: /* a boolean */
    case ECMA_INTERNAL_PROPERTY_CLASS: /* an enum */
    case ECMA_INTERNAL_PROPERTY_CODE: /* an integer */
    case ECMA_INTERNAL_PROPERTY_BUILT_IN_ROUTINE_ID: /* an integer */
    {
      break;
    }
  }

  ecma_dealloc_property (property_p);
} /* ecma_free_internal_property */

/**
 * Free the property and values it references.
 */
void
ecma_free_property (ecma_property_t *prop_p) /**< property */
{
  switch ((ecma_property_type_t) prop_p->type)
  {
    case ECMA_PROPERTY_NAMEDDATA:
    {
      ecma_free_named_data_property (prop_p);

      break;
    }

    case ECMA_PROPERTY_NAMEDACCESSOR:
    {
      ecma_free_named_accessor_property (prop_p);

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
    next_prop_p = ECMA_GET_POINTER(cur_prop_p->next_property_p);

    if (cur_prop_p == prop_p)
    {
      ecma_free_property (prop_p);

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
 * Construct empty property descriptor, i.e.:
 *  property descriptor with all is_defined flags set to false and the rest - to default value.
 */
ecma_property_descriptor_t
ecma_make_empty_property_descriptor (void)
{
  ecma_property_descriptor_t prop_desc = (ecma_property_descriptor_t)
  {
    .is_value_defined = false,
    .value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED),

    .is_writable_defined = false,
    .writable = ECMA_PROPERTY_NOT_WRITABLE,

    .is_enumerable_defined = false,
    .enumerable = ECMA_PROPERTY_NOT_ENUMERABLE,

    .is_configurable_defined = false,
    .configurable = ECMA_PROPERTY_NOT_CONFIGURABLE,

    .is_get_defined = false,
    .get_p = NULL,

    .is_set_defined = false,
    .set_p = NULL
  };

  return prop_desc;
} /* ecma_make_empty_property_descriptor */

/**
 * @}
 * @}
 */
