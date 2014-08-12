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

/** \addtogroup ecma ---TODO---
 * @{
 *
 * \addtogroup ecmahelpers Helpers for operations with ECMA data types
 * @{
 */

#include "ecma-alloc.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "jerry-libc.h"

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

  object_p->properties_p = ECMA_NULL_POINTER;
  object_p->is_lexical_environment = false;

  object_p->u.object.extensible = is_extensible;
  ECMA_SET_POINTER(object_p->u.object.prototype_object_p, prototype_object_p);
  object_p->u.object.type = type;

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

  new_lexical_environment_p->is_lexical_environment = true;
  new_lexical_environment_p->u.lexical_environment.type = ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE;

  new_lexical_environment_p->properties_p = ECMA_NULL_POINTER;

  ECMA_SET_POINTER(new_lexical_environment_p->u.lexical_environment.outer_reference_p, outer_lexical_environment_p);

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
  JERRY_ASSERT(binding_obj_p != NULL);

  ecma_object_t *new_lexical_environment_p = ecma_alloc_object ();
  ecma_init_gc_info (new_lexical_environment_p);

  new_lexical_environment_p->is_lexical_environment = true;
  new_lexical_environment_p->u.lexical_environment.type = ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND;

  new_lexical_environment_p->properties_p = ECMA_NULL_POINTER;

  ECMA_SET_POINTER(new_lexical_environment_p->u.lexical_environment.outer_reference_p, outer_lexical_environment_p);

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

  ecma_property_t *list_head_p = ECMA_GET_POINTER(object_p->properties_p);
  ECMA_SET_POINTER(new_property_p->next_property_p, list_head_p);
  ECMA_SET_NON_NULL_POINTER(object_p->properties_p, new_property_p);

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

  for (ecma_property_t *property_p = ECMA_GET_POINTER(object_p->properties_p);
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
                                 const ecma_char_t *name_p, /**< property name */
                                 ecma_property_writable_value_t writable, /**< 'writable' attribute */
                                 ecma_property_enumerable_value_t enumerable, /**< 'enumerable' attribute */
                                 ecma_property_configurable_value_t configurable) /**< 'configurable' attribute */
{
  JERRY_ASSERT(obj_p != NULL && name_p != NULL);

  ecma_property_t *prop_p = ecma_alloc_property ();

  prop_p->type = ECMA_PROPERTY_NAMEDDATA;

  ECMA_SET_NON_NULL_POINTER(prop_p->u.named_data_property.name_p, ecma_new_ecma_string (name_p));

  prop_p->u.named_data_property.writable = writable;
  prop_p->u.named_data_property.enumerable = enumerable;
  prop_p->u.named_data_property.configurable = configurable;

  prop_p->u.named_data_property.value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

  ecma_property_t *list_head_p = ECMA_GET_POINTER(obj_p->properties_p);

  ECMA_SET_POINTER(prop_p->next_property_p, list_head_p);
  ECMA_SET_NON_NULL_POINTER(obj_p->properties_p, prop_p);

  return prop_p;
} /* ecma_create_named_data_property */

/**
 * Create named accessor property with given name, attributes, getter and setter.
 *
 * @return pointer to newly created property
 */
ecma_property_t*
ecma_create_named_accessor_property (ecma_object_t *obj_p, /**< object */
                                     const ecma_char_t *name_p, /**< property name */
                                     ecma_object_t *get_p, /**< getter */
                                     ecma_object_t *set_p, /**< setter */
                                     ecma_property_enumerable_value_t enumerable, /**< 'enumerable' attribute */
                                     ecma_property_configurable_value_t configurable) /**< 'configurable' attribute */
{
  JERRY_ASSERT(obj_p != NULL && name_p != NULL);

  ecma_property_t *prop_p = ecma_alloc_property ();

  prop_p->type = ECMA_PROPERTY_NAMEDACCESSOR;

  ECMA_SET_NON_NULL_POINTER(prop_p->u.named_accessor_property.name_p, ecma_new_ecma_string (name_p));

  ECMA_SET_POINTER(prop_p->u.named_accessor_property.get_p, get_p);
  ecma_gc_update_may_ref_younger_object_flag_by_object (obj_p, get_p);

  ECMA_SET_POINTER(prop_p->u.named_accessor_property.set_p, set_p);
  ecma_gc_update_may_ref_younger_object_flag_by_object (obj_p, set_p);

  prop_p->u.named_accessor_property.enumerable = enumerable;
  prop_p->u.named_accessor_property.configurable = configurable;

  ecma_property_t *list_head_p = ECMA_GET_POINTER(obj_p->properties_p);
  ECMA_SET_POINTER(prop_p->next_property_p, list_head_p);
  ECMA_SET_NON_NULL_POINTER(obj_p->properties_p, prop_p);

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
                          const ecma_char_t *name_p) /**< property's name */
{
  JERRY_ASSERT(obj_p != NULL);
  JERRY_ASSERT(name_p != NULL);

  for (ecma_property_t *property_p = ECMA_GET_POINTER(obj_p->properties_p);
       property_p != NULL;
       property_p = ECMA_GET_POINTER(property_p->next_property_p))
  {
    ecma_array_first_chunk_t *property_name_p;

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

    if (ecma_compare_zt_string_to_ecma_string (name_p, property_name_p))
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
                         const ecma_char_t *name_p) /**< property's name */
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
                              const ecma_char_t *name_p) /**< property's name */
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

  ecma_free_array (ECMA_GET_POINTER(property_p->u.named_data_property.name_p));
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

  ecma_free_array (ECMA_GET_POINTER(property_p->u.named_accessor_property.name_p));

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
    case ECMA_INTERNAL_PROPERTY_NUMBER_INDEXED_ARRAY_VALUES: /* an array */
    case ECMA_INTERNAL_PROPERTY_STRING_INDEXED_ARRAY_VALUES: /* an array */
    case ECMA_INTERNAL_PROPERTY_FORMAL_PARAMETERS: /* an array */
    {
      ecma_free_array (ECMA_GET_POINTER(property_value));
      break;
    }

    case ECMA_INTERNAL_PROPERTY_SCOPE: /* a lexical environment */
    case ECMA_INTERNAL_PROPERTY_BINDING_OBJECT: /* an object */
    case ECMA_INTERNAL_PROPERTY_PROTOTYPE: /* the property's value is located in ecma_object_t */
    case ECMA_INTERNAL_PROPERTY_EXTENSIBLE: /* the property's value is located in ecma_object_t */
    case ECMA_INTERNAL_PROPERTY_PROVIDE_THIS: /* a boolean */
    case ECMA_INTERNAL_PROPERTY_CLASS: /* an enum */
    case ECMA_INTERNAL_PROPERTY_CODE: /* an integer */
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
  for (ecma_property_t *cur_prop_p = ECMA_GET_POINTER(obj_p->properties_p), *prev_prop_p = NULL, *next_prop_p;
       cur_prop_p != NULL;
       prev_prop_p = cur_prop_p, cur_prop_p = next_prop_p)
  {
    next_prop_p = ECMA_GET_POINTER(cur_prop_p->next_property_p);

    if (cur_prop_p == prop_p)
    {
      ecma_free_property (prop_p);

      if (prev_prop_p == NULL)
      {
        ECMA_SET_POINTER(obj_p->properties_p, next_prop_p);
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
 * Allocate new ecma-string and fill it with characters from specified buffer
 *
 * @return Pointer to first chunk of an array, containing allocated string
 */
ecma_array_first_chunk_t*
ecma_new_ecma_string (const ecma_char_t *string_p) /**< zero-terminated string of ecma-characters */
{
  ecma_length_t length = 0;

  /*
   * TODO: Do not precalculate length.
   */
  if (string_p != NULL)
  {
    const ecma_char_t *iter_p = string_p;
    while (*iter_p++)
    {
      length++;
    }
  }

  ecma_array_first_chunk_t *string_first_chunk_p = ecma_alloc_array_first_chunk ();

  string_first_chunk_p->header.unit_number = length;
  uint8_t *copy_pointer = (uint8_t*) string_p;
  size_t chars_left = length;
  size_t chars_to_copy = JERRY_MIN(length, sizeof (string_first_chunk_p->data) / sizeof (ecma_char_t));
  __memcpy (string_first_chunk_p->data, copy_pointer, chars_to_copy * sizeof (ecma_char_t));
  chars_left -= chars_to_copy;
  copy_pointer += chars_to_copy * sizeof (ecma_char_t);

  ecma_array_non_first_chunk_t *string_non_first_chunk_p;

  JERRY_STATIC_ASSERT(ECMA_POINTER_FIELD_WIDTH <= sizeof (uint16_t) * JERRY_BITSINBYTE);
  uint16_t *next_chunk_compressed_pointer_p = &string_first_chunk_p->header.next_chunk_p;

  while (chars_left > 0)
  {
    string_non_first_chunk_p = ecma_alloc_array_non_first_chunk ();

    size_t chars_to_copy = JERRY_MIN(chars_left, sizeof (string_non_first_chunk_p->data) / sizeof (ecma_char_t));
    __memcpy (string_non_first_chunk_p->data, copy_pointer, chars_to_copy * sizeof (ecma_char_t));
    chars_left -= chars_to_copy;
    copy_pointer += chars_to_copy * sizeof (ecma_char_t);

    ECMA_SET_NON_NULL_POINTER(*next_chunk_compressed_pointer_p, string_non_first_chunk_p);
    next_chunk_compressed_pointer_p = &string_non_first_chunk_p->next_chunk_p;
  }

  *next_chunk_compressed_pointer_p = ECMA_NULL_POINTER;

  return string_first_chunk_p;
} /* ecma_new_ecma_string */

/**
 * Copy ecma-string's contents to a buffer.
 *
 * Buffer will contain length of string, in characters, followed by string's characters.
 *
 * @return number of bytes, actually copied to the buffer, if string's content was copied successfully;
 *         negative number, which is calculated as negation of buffer size, that is required
 *         to hold the string's content (in case size of buffer is insuficcient).
 */
ssize_t
ecma_copy_ecma_string_chars_to_buffer (ecma_array_first_chunk_t *first_chunk_p, /**< first chunk of ecma-string */
                                       uint8_t *buffer_p, /**< destination buffer */
                                       size_t buffer_size) /**< size of buffer */
{
  ecma_length_t string_length = first_chunk_p->header.unit_number;
  size_t required_buffer_size = sizeof (ecma_length_t) + sizeof (ecma_char_t) * string_length;

  if (required_buffer_size > buffer_size)
  {
    return - (ssize_t) required_buffer_size;
  }

  *(ecma_length_t*) buffer_p = string_length;

  size_t chars_left = string_length;
  uint8_t *dest_pointer = buffer_p + sizeof (ecma_length_t);
  size_t copy_chunk_chars = JERRY_MIN(sizeof (first_chunk_p->data) / sizeof (ecma_char_t),
                                      chars_left);
  __memcpy (dest_pointer, first_chunk_p->data, copy_chunk_chars * sizeof (ecma_char_t));
  dest_pointer += copy_chunk_chars * sizeof (ecma_char_t);
  chars_left -= copy_chunk_chars;

  ecma_array_non_first_chunk_t *non_first_chunk_p = ECMA_GET_POINTER(first_chunk_p->header.next_chunk_p);

  while (chars_left > 0)
  {
    JERRY_ASSERT(chars_left < string_length);

    copy_chunk_chars = JERRY_MIN(sizeof (non_first_chunk_p->data) / sizeof (ecma_char_t),
                                 chars_left);
    __memcpy (dest_pointer, non_first_chunk_p->data, copy_chunk_chars * sizeof (ecma_char_t));
    dest_pointer += copy_chunk_chars * sizeof (ecma_char_t);
    chars_left -= copy_chunk_chars;

    non_first_chunk_p = ECMA_GET_POINTER(non_first_chunk_p->next_chunk_p);
  }

  return (ssize_t) required_buffer_size;
} /* ecma_copy_ecma_string_chars_to_buffer */

/**
 * Duplicate an ecma-string.
 *
 * @return pointer to new ecma-string's first chunk
 */
ecma_array_first_chunk_t*
ecma_duplicate_ecma_string (ecma_array_first_chunk_t *first_chunk_p) /**< first chunk of string to duplicate */
{
  JERRY_ASSERT(first_chunk_p != NULL);

  ecma_array_first_chunk_t *first_chunk_copy_p = ecma_alloc_array_first_chunk ();
  __memcpy (first_chunk_copy_p, first_chunk_p, sizeof (ecma_array_first_chunk_t));

  ecma_array_non_first_chunk_t *non_first_chunk_p, *non_first_chunk_copy_p;
  non_first_chunk_p = ECMA_GET_POINTER(first_chunk_p->header.next_chunk_p);
  uint16_t *next_pointer_p = &first_chunk_copy_p->header.next_chunk_p;

  while (non_first_chunk_p != NULL)
  {
    non_first_chunk_copy_p = ecma_alloc_array_non_first_chunk ();
    ECMA_SET_POINTER(*next_pointer_p, non_first_chunk_copy_p);
    next_pointer_p = &non_first_chunk_copy_p->next_chunk_p;

    __memcpy (non_first_chunk_copy_p, non_first_chunk_p, sizeof (ecma_array_non_first_chunk_t));

    non_first_chunk_p = ECMA_GET_POINTER(non_first_chunk_p->next_chunk_p);
  }

  *next_pointer_p = ECMA_NULL_POINTER;

  return first_chunk_copy_p;
} /* ecma_duplicate_ecma_string */

/**
 * Compare zero-terminated string to ecma-string
 *
 * @return true - if strings are equal;
 *         false - otherwise.
 */
bool
ecma_compare_ecma_string_to_ecma_string (const ecma_array_first_chunk_t *string1_p, /* ecma-string */
                                         const ecma_array_first_chunk_t *string2_p) /* ecma-string */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS(string1_p, string2_p);
} /* ecma_compare_ecma_string_to_ecma_string */

/**
 * Compare zero-terminated string to ecma-string
 *
 * @return true - if strings are equal;
 *         false - otherwise.
 */
bool
ecma_compare_zt_string_to_ecma_string (const ecma_char_t *string_p, /**< zero-terminated string */
                                       const ecma_array_first_chunk_t *ecma_string_p) /* ecma-string */
{
  JERRY_ASSERT(string_p != NULL);
  JERRY_ASSERT(ecma_string_p != NULL);

  const ecma_char_t *str_iter_p = string_p;
  ecma_length_t ecma_str_len = ecma_string_p->header.unit_number;
  const ecma_char_t *current_chunk_chars_cur = (const ecma_char_t*) ecma_string_p->data;
  const ecma_char_t *current_chunk_chars_end = (const ecma_char_t*) (ecma_string_p->data +
                                                                     sizeof (ecma_string_p->data));

  JERRY_STATIC_ASSERT(ECMA_POINTER_FIELD_WIDTH <= sizeof (uint16_t) * JERRY_BITSINBYTE);
  const uint16_t *next_chunk_compressed_pointer_p = &ecma_string_p->header.next_chunk_p;

  for (ecma_length_t str_index = 0;
       str_index < ecma_str_len;
       str_index++, str_iter_p++, current_chunk_chars_cur++)
  {
    JERRY_ASSERT(current_chunk_chars_cur <= current_chunk_chars_end);

    if (current_chunk_chars_cur == current_chunk_chars_end)
    {
      /* switching to next chunk */
      ecma_array_non_first_chunk_t *next_chunk_p = ECMA_GET_POINTER(*next_chunk_compressed_pointer_p);

      JERRY_ASSERT(next_chunk_p != NULL);

      current_chunk_chars_cur = (const ecma_char_t*) next_chunk_p->data;
      current_chunk_chars_end = (const ecma_char_t*) (next_chunk_p->data + sizeof (next_chunk_p->data));

      next_chunk_compressed_pointer_p = &next_chunk_p->next_chunk_p;
    }

    if (*str_iter_p != *current_chunk_chars_cur)
    {
      /*
       * Either *str_iter_p is 0 (zero-terminated string is shorter),
       * or the character is just different.
       *
       * In both cases strings are not equal.
       */
      return false;
    }
  }

  /*
   * Now, we have reached end of ecma-string.
   *
   * If we have also reached end of zero-terminated string, than strings are equal.
   * Otherwise zero-terminated string is longer.
   */
  return (*str_iter_p == 0);
} /* ecma_compare_zt_string_to_ecma_string */

/**
 * Free all chunks of an array
 */
void
ecma_free_array (ecma_array_first_chunk_t *first_chunk_p) /**< first chunk of the array */
{
  JERRY_ASSERT(first_chunk_p != NULL);

  ecma_array_non_first_chunk_t *non_first_chunk_p = ECMA_GET_POINTER(first_chunk_p->header.next_chunk_p);

  ecma_dealloc_array_first_chunk (first_chunk_p);

  while (non_first_chunk_p != NULL)
  {
    ecma_array_non_first_chunk_t *next_chunk_p = ECMA_GET_POINTER(non_first_chunk_p->next_chunk_p);

    ecma_dealloc_array_non_first_chunk (non_first_chunk_p);

    non_first_chunk_p = next_chunk_p;
  }
} /* ecma_free_array */

/**
 * Construct empty property descriptor.
 *
 * @return property descriptor with all *_defined properties set to false,
 *         and rest properties set to default values (ECMA-262 v5, Table 7).
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
