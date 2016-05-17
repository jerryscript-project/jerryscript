/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
 * Copyright 2015-2016 University of Szeged
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
#include "ecma-builtins.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "jrt-bit-fields.h"

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 */

static ecma_value_t
ecma_builtin_dispatch_routine (ecma_builtin_id_t builtin_object_id,
                               uint16_t builtin_routine_id,
                               ecma_value_t this_arg_value,
                               const ecma_value_t arguments_list[],
                               ecma_length_t arguments_number);
static void ecma_instantiate_builtin (ecma_builtin_id_t id);

/**
 * Pointer to instances of built-in objects
 */
static ecma_object_t *ecma_builtin_objects[ECMA_BUILTIN_ID__COUNT];

/**
 * Check if passed object is the instance of specified built-in.
 */
bool
ecma_builtin_is (ecma_object_t *obj_p, /**< pointer to an object */
                 ecma_builtin_id_t builtin_id) /**< id of built-in to check on */
{
  JERRY_ASSERT (obj_p != NULL && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT (builtin_id < ECMA_BUILTIN_ID__COUNT);

  if (ecma_builtin_objects[builtin_id] == NULL)
  {
    /* If a built-in object is not instantiated,
     * the specified object cannot be the built-in object */
    return false;
  }
  else
  {
    return (obj_p == ecma_builtin_objects[builtin_id]);
  }
} /* ecma_builtin_is */

/**
 * Get reference to specified built-in object
 *
 * @return pointer to the object's instance
 */
ecma_object_t *
ecma_builtin_get (ecma_builtin_id_t builtin_id) /**< id of built-in to check on */
{
  JERRY_ASSERT (builtin_id < ECMA_BUILTIN_ID__COUNT);

  if (unlikely (ecma_builtin_objects[builtin_id] == NULL))
  {
    ecma_instantiate_builtin (builtin_id);
  }

  ecma_ref_object (ecma_builtin_objects[builtin_id]);

  return ecma_builtin_objects[builtin_id];
} /* ecma_builtin_get */

/**
 * Initialize specified built-in object.
 *
 * Warning:
 *         the routine should be called only from ecma_init_builtins
 *
 * @return pointer to the object
 */
static ecma_object_t *
ecma_builtin_init_object (ecma_builtin_id_t obj_builtin_id, /**< built-in ID */
                          ecma_object_t *prototype_obj_p, /**< prototype object */
                          ecma_object_type_t obj_type, /**< object's type */
                          bool is_extensible) /**< value of object's [[Extensible]] property */
{
  ecma_object_t *object_obj_p = ecma_create_object (prototype_obj_p, is_extensible, obj_type);

  /*
   * [[Class]] property of built-in object is not stored explicitly.
   *
   * See also: ecma_object_get_class_name
   */

  ecma_property_t *built_in_id_prop_p = ecma_create_internal_property (object_obj_p,
                                                                       ECMA_INTERNAL_PROPERTY_BUILT_IN_ID);
  ecma_set_internal_property_value (built_in_id_prop_p, obj_builtin_id);

  ecma_set_object_is_builtin (object_obj_p);

  /** Initializing [[PrimitiveValue]] properties of built-in prototype objects */
  switch (obj_builtin_id)
  {
#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_STRING_BUILTIN
    case ECMA_BUILTIN_ID_STRING_PROTOTYPE:
    {
      ecma_string_t *prim_prop_str_value_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);

      ecma_property_t *prim_value_prop_p;
      prim_value_prop_p = ecma_create_internal_property (object_obj_p,
                                                         ECMA_INTERNAL_PROPERTY_PRIMITIVE_STRING_VALUE);
      ECMA_SET_INTERNAL_VALUE_POINTER (ECMA_PROPERTY_VALUE_PTR (prim_value_prop_p)->value, prim_prop_str_value_p);
      break;
    }
#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_STRING_BUILTIN */

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_NUMBER_BUILTIN
    case ECMA_BUILTIN_ID_NUMBER_PROTOTYPE:
    {
      ecma_number_t *prim_prop_num_value_p = ecma_alloc_number ();
      *prim_prop_num_value_p = ECMA_NUMBER_ZERO;

      ecma_property_t *prim_value_prop_p;
      prim_value_prop_p = ecma_create_internal_property (object_obj_p,
                                                         ECMA_INTERNAL_PROPERTY_PRIMITIVE_NUMBER_VALUE);
      ECMA_SET_INTERNAL_VALUE_POINTER (ECMA_PROPERTY_VALUE_PTR (prim_value_prop_p)->value, prim_prop_num_value_p);
      break;
    }
#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_NUMBER_BUILTIN */

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_BOOLEAN_BUILTIN
    case ECMA_BUILTIN_ID_BOOLEAN_PROTOTYPE:
    {
      ecma_property_t *prim_value_prop_p;
      prim_value_prop_p = ecma_create_internal_property (object_obj_p,
                                                         ECMA_INTERNAL_PROPERTY_PRIMITIVE_BOOLEAN_VALUE);
      ecma_set_internal_property_value (prim_value_prop_p, ECMA_SIMPLE_VALUE_FALSE);
      break;
    }
#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_BOOLEAN_BUILTIN */

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_DATE_BUILTIN
    case ECMA_BUILTIN_ID_DATE_PROTOTYPE:
    {
      ecma_number_t *prim_prop_num_value_p = ecma_alloc_number ();
      *prim_prop_num_value_p = ecma_number_make_nan ();

      ecma_property_t *prim_value_prop_p;
      prim_value_prop_p = ecma_create_internal_property (object_obj_p,
                                                         ECMA_INTERNAL_PROPERTY_PRIMITIVE_NUMBER_VALUE);
      ECMA_SET_INTERNAL_VALUE_POINTER (ECMA_PROPERTY_VALUE_PTR (prim_value_prop_p)->value, prim_prop_num_value_p);
      break;
    }
#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_DATE_BUILTIN */

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN
    case ECMA_BUILTIN_ID_REGEXP_PROTOTYPE:
    {
      ecma_property_t *bytecode_prop_p;
      bytecode_prop_p = ecma_create_internal_property (object_obj_p,
                                                       ECMA_INTERNAL_PROPERTY_REGEXP_BYTECODE);
      ecma_set_internal_property_value (bytecode_prop_p, ECMA_NULL_POINTER);
      break;
    }
#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN */
    default:
    {
      break;
    }
  }

  return object_obj_p;
} /* ecma_builtin_init_object */

/**
 * Initialize ECMA built-ins components
 */
void
ecma_init_builtins (void)
{
  for (ecma_builtin_id_t id = (ecma_builtin_id_t) 0;
       id < ECMA_BUILTIN_ID__COUNT;
       id = (ecma_builtin_id_t) (id + 1))
  {
    ecma_builtin_objects[id] = NULL;
  }
} /* ecma_init_builtins */

/**
 * Instantiate specified ECMA built-in object
 */
static void
ecma_instantiate_builtin (ecma_builtin_id_t id) /**< built-in id */
{
  switch (id)
  {
#define BUILTIN(builtin_id, \
                object_type, \
                object_prototype_builtin_id, \
                is_extensible, \
                is_static, \
                lowercase_name) \
    case builtin_id: \
    { \
      JERRY_ASSERT (ecma_builtin_objects[builtin_id] == NULL); \
      \
      ecma_object_t *prototype_obj_p; \
      if (object_prototype_builtin_id == ECMA_BUILTIN_ID__COUNT) \
      { \
        prototype_obj_p = NULL; \
      } \
      else \
      { \
        if (ecma_builtin_objects[object_prototype_builtin_id] == NULL) \
        { \
          ecma_instantiate_builtin (object_prototype_builtin_id); \
        } \
        prototype_obj_p = ecma_builtin_objects[object_prototype_builtin_id]; \
        JERRY_ASSERT (prototype_obj_p != NULL); \
      } \
      \
      ecma_object_t *builtin_obj_p = ecma_builtin_init_object (builtin_id, \
                                                               prototype_obj_p, \
                                                               object_type, \
                                                               is_extensible); \
      ecma_builtin_objects[builtin_id] = builtin_obj_p; \
      \
      break; \
    }
#include "ecma-builtins.inc.h"

    default:
    {
      JERRY_ASSERT (id < ECMA_BUILTIN_ID__COUNT);

      JERRY_UNIMPLEMENTED ("The built-in is not implemented.");
    }
  }
} /* ecma_instantiate_builtin */

/**
 * Finalize ECMA built-in objects
 */
void
ecma_finalize_builtins (void)
{
  for (ecma_builtin_id_t id = (ecma_builtin_id_t) 0;
       id < ECMA_BUILTIN_ID__COUNT;
       id = (ecma_builtin_id_t) (id + 1))
  {
    if (ecma_builtin_objects[id] != NULL)
    {
      ecma_deref_object (ecma_builtin_objects[id]);
      ecma_builtin_objects[id] = NULL;
    }
  }
} /* ecma_finalize_builtins */

/**
 * If the property's name is one of built-in properties of the object
 * that is not instantiated yet, instantiate the property and
 * return pointer to the instantiated property.
 *
 * @return pointer property, if one was instantiated,
 *         NULL - otherwise.
 */
ecma_property_t *
ecma_builtin_try_to_instantiate_property (ecma_object_t *object_p, /**< object */
                                          ecma_string_t *string_p) /**< property's name */
{
  JERRY_ASSERT (ecma_get_object_is_builtin (object_p));

  const ecma_object_type_t type = ecma_get_object_type (object_p);

  if (type == ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION)
  {
    ecma_string_t *magic_string_length_p = ecma_get_magic_string (LIT_MAGIC_STRING_LENGTH);

    bool is_length_property = ecma_compare_ecma_strings (string_p, magic_string_length_p);

    ecma_deref_ecma_string (magic_string_length_p);

    if (is_length_property)
    {
      /*
       * Lazy instantiation of 'length' property
       *
       * Note:
       *      We don't need to mark that the property was already lazy instantiated,
       *      as it is non-configurable and so can't be deleted
       */

      ecma_property_t *desc_prop_p = ecma_get_internal_property (object_p,
                                                                 ECMA_INTERNAL_PROPERTY_BUILT_IN_ROUTINE_DESC);
      uint64_t builtin_routine_desc = ecma_get_internal_property_value (desc_prop_p);

      JERRY_STATIC_ASSERT (sizeof (uint8_t) * JERRY_BITSINBYTE == ECMA_BUILTIN_ROUTINE_ID_LENGTH_VALUE_WIDTH,
                           bits_in_uint8_t_must_be_equal_to_ECMA_BUILTIN_ROUTINE_ID_LENGTH_VALUE_WIDTH);
      uint8_t length_prop_value = (uint8_t) JRT_EXTRACT_BIT_FIELD (uint64_t, builtin_routine_desc,
                                                                   ECMA_BUILTIN_ROUTINE_ID_LENGTH_VALUE_POS,
                                                                   ECMA_BUILTIN_ROUTINE_ID_LENGTH_VALUE_WIDTH);

      ecma_property_t *len_prop_p = ecma_create_named_data_property (object_p,
                                                                     string_p,
                                                                     false, false, false);

      ecma_set_named_data_property_value (len_prop_p, ecma_make_uint32_value (length_prop_value));

      JERRY_ASSERT (!ecma_is_property_configurable (len_prop_p));
      return len_prop_p;
    }

    return NULL;
  }
  else
  {
    ecma_property_t *built_in_id_prop_p = ecma_get_internal_property (object_p,
                                                                      ECMA_INTERNAL_PROPERTY_BUILT_IN_ID);
    ecma_builtin_id_t builtin_id = (ecma_builtin_id_t) ecma_get_internal_property_value (built_in_id_prop_p);

    JERRY_ASSERT (ecma_builtin_is (object_p, builtin_id));

    switch (builtin_id)
    {
#define BUILTIN(builtin_id, \
                object_type, \
                object_prototype_builtin_id, \
                is_extensible, \
                is_static, \
                lowercase_name) \
      case builtin_id: \
      { \
        return ecma_builtin_ ## lowercase_name ## _try_to_instantiate_property (object_p, \
                                                                                string_p); \
      }
#include "ecma-builtins.inc.h"

      case ECMA_BUILTIN_ID__COUNT:
      {
        JERRY_UNREACHABLE ();
      }

      default:
      {
#ifdef CONFIG_ECMA_COMPACT_PROFILE
        JERRY_UNREACHABLE ();
#else /* !CONFIG_ECMA_COMPACT_PROFILE */
        JERRY_UNIMPLEMENTED ("The built-in is not implemented.");
#endif /* CONFIG_ECMA_COMPACT_PROFILE */
      }
    }

    JERRY_UNREACHABLE ();
  }
} /* ecma_builtin_try_to_instantiate_property */

/**
 * List names of a built-in object's lazy instantiated properties
 *
 * See also:
 *          ecma_builtin_try_to_instantiate_property
 */
void
ecma_builtin_list_lazy_property_names (ecma_object_t *object_p, /**< a built-in object */
                                       bool separate_enumerable, /**< true -  list enumerable properties into
                                                                  *           main collection, and non-enumerable
                                                                  *           to collection of 'skipped non-enumerable'
                                                                  *           properties,
                                                                  *   false - list all properties into main collection.
                                                                  */
                                       ecma_collection_header_t *main_collection_p, /**< 'main' collection */
                                       ecma_collection_header_t *non_enum_collection_p) /**< skipped 'non-enumerable'
                                                                                         *   collection */
{
  const ecma_object_type_t type = ecma_get_object_type (object_p);

  if (type == ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION)
  {
    ecma_collection_header_t *for_enumerable_p = main_collection_p;
    (void) for_enumerable_p;

    ecma_collection_header_t *for_non_enumerable_p = separate_enumerable ? non_enum_collection_p : main_collection_p;

    /* 'length' property is non-enumerable (ECMA-262 v5, 15) */
    ecma_string_t *name_p = ecma_get_magic_string (LIT_MAGIC_STRING_LENGTH);
    ecma_append_to_values_collection (for_non_enumerable_p, ecma_make_string_value (name_p), true);
    ecma_deref_ecma_string (name_p);
  }
  else
  {
    ecma_property_t *built_in_id_prop_p = ecma_get_internal_property (object_p,
                                                                      ECMA_INTERNAL_PROPERTY_BUILT_IN_ID);
    ecma_builtin_id_t builtin_id = (ecma_builtin_id_t) ecma_get_internal_property_value (built_in_id_prop_p);

    JERRY_ASSERT (ecma_builtin_is (object_p, builtin_id));

    switch (builtin_id)
    {
#define BUILTIN(builtin_id, \
                object_type, \
                object_prototype_builtin_id, \
                is_extensible, \
                is_static, \
                lowercase_name) \
      case builtin_id: \
                       { \
                         ecma_builtin_ ## lowercase_name ## _list_lazy_property_names (object_p, \
                                                                                       separate_enumerable, \
                                                                                       main_collection_p, \
                                                                                       non_enum_collection_p); \
                         return; \
                       }
#include "ecma-builtins.inc.h"

      case ECMA_BUILTIN_ID__COUNT:
      {
        JERRY_UNREACHABLE ();
      }

      default:
      {
#ifdef CONFIG_ECMA_COMPACT_PROFILE
        JERRY_UNREACHABLE ();
#else /* !CONFIG_ECMA_COMPACT_PROFILE */
        JERRY_UNIMPLEMENTED ("The built-in is not implemented.");
#endif /* CONFIG_ECMA_COMPACT_PROFILE */
      }
    }

    JERRY_UNREACHABLE ();
  }
} /* ecma_builtin_list_lazy_property_names */

/**
 * Construct a Function object for specified built-in routine
 *
 * See also: ECMA-262 v5, 15
 *
 * @return pointer to constructed Function object
 */
ecma_object_t *
ecma_builtin_make_function_object_for_routine (ecma_builtin_id_t builtin_id, /**< identifier of built-in object
                                                                                  that initially contains property
                                                                                  with the routine */
                                               uint16_t routine_id, /**< builtin-wide identifier of the built-in
                                                                         object's routine property */
                                               uint8_t length_prop_value) /**< value of 'length' property
                                                                               of function object to create */
{
  ecma_object_t *prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE);

  ecma_object_t *func_obj_p = ecma_create_object (prototype_obj_p, true, ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION);

  ecma_deref_object (prototype_obj_p);

  ecma_set_object_is_builtin (func_obj_p);

  uint64_t packed_value = JRT_SET_BIT_FIELD_VALUE (uint64_t, 0ull,
                                                   builtin_id,
                                                   ECMA_BUILTIN_ROUTINE_ID_BUILT_IN_OBJECT_ID_POS,
                                                   ECMA_BUILTIN_ROUTINE_ID_BUILT_IN_OBJECT_ID_WIDTH);
  packed_value = JRT_SET_BIT_FIELD_VALUE (uint64_t, packed_value,
                                          routine_id,
                                          ECMA_BUILTIN_ROUTINE_ID_BUILT_IN_ROUTINE_ID_POS,
                                          ECMA_BUILTIN_ROUTINE_ID_BUILT_IN_ROUTINE_ID_WIDTH);
  packed_value = JRT_SET_BIT_FIELD_VALUE (uint64_t, packed_value,
                                          length_prop_value,
                                          ECMA_BUILTIN_ROUTINE_ID_LENGTH_VALUE_POS,
                                          ECMA_BUILTIN_ROUTINE_ID_LENGTH_VALUE_WIDTH);

  ecma_property_t *routine_desc_prop_p = ecma_create_internal_property (func_obj_p,
                                                                        ECMA_INTERNAL_PROPERTY_BUILT_IN_ROUTINE_DESC);

  JERRY_ASSERT ((uint32_t) packed_value == packed_value);
  ecma_set_internal_property_value (routine_desc_prop_p, (uint32_t) packed_value);

  return func_obj_p;
} /* ecma_builtin_make_function_object_for_routine */

/**
 * Handle calling [[Call]] of built-in object
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_dispatch_call (ecma_object_t *obj_p, /**< built-in object */
                            ecma_value_t this_arg_value, /**< 'this' argument value */
                            const ecma_value_t *arguments_list_p, /**< arguments list */
                            ecma_length_t arguments_list_len) /**< arguments list length */
{
  JERRY_ASSERT (ecma_get_object_is_builtin (obj_p));

  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  if (ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION)
  {
    ecma_property_t *desc_prop_p = ecma_get_internal_property (obj_p,
                                                               ECMA_INTERNAL_PROPERTY_BUILT_IN_ROUTINE_DESC);
    uint64_t builtin_routine_desc = ecma_get_internal_property_value (desc_prop_p);

    uint64_t built_in_id_field = JRT_EXTRACT_BIT_FIELD (uint64_t, builtin_routine_desc,
                                                        ECMA_BUILTIN_ROUTINE_ID_BUILT_IN_OBJECT_ID_POS,
                                                        ECMA_BUILTIN_ROUTINE_ID_BUILT_IN_OBJECT_ID_WIDTH);
    JERRY_ASSERT (built_in_id_field < ECMA_BUILTIN_ID__COUNT);

    uint64_t routine_id_field = JRT_EXTRACT_BIT_FIELD (uint64_t, builtin_routine_desc,
                                                       ECMA_BUILTIN_ROUTINE_ID_BUILT_IN_ROUTINE_ID_POS,
                                                       ECMA_BUILTIN_ROUTINE_ID_BUILT_IN_ROUTINE_ID_WIDTH);
    JERRY_ASSERT ((uint16_t) routine_id_field == routine_id_field);

    ecma_builtin_id_t built_in_id = (ecma_builtin_id_t) built_in_id_field;
    uint16_t routine_id = (uint16_t) routine_id_field;

    ret_value =  ecma_builtin_dispatch_routine (built_in_id,
                                                routine_id,
                                                this_arg_value,
                                                arguments_list_p,
                                                arguments_list_len);
  }
  else
  {
    JERRY_ASSERT (ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_FUNCTION);

    ecma_property_t *built_in_id_prop_p = ecma_get_internal_property (obj_p,
                                                                      ECMA_INTERNAL_PROPERTY_BUILT_IN_ID);
    ecma_builtin_id_t builtin_id = (ecma_builtin_id_t) ecma_get_internal_property_value (built_in_id_prop_p);

    JERRY_ASSERT (ecma_builtin_is (obj_p, builtin_id));

    switch (builtin_id)
    {
#define BUILTIN(builtin_id, \
                object_type, \
                object_prototype_builtin_id, \
                is_extensible, \
                is_static, \
                lowercase_name) \
      case builtin_id: \
      { \
        if (object_type == ECMA_OBJECT_TYPE_FUNCTION) \
        { \
          ret_value = ecma_builtin_ ## lowercase_name ## _dispatch_call (arguments_list_p, \
                                                                         arguments_list_len); \
        } \
        break; \
      }
#include "ecma-builtins.inc.h"

      case ECMA_BUILTIN_ID__COUNT:
      {
        JERRY_UNREACHABLE ();
      }

      default:
      {
#ifdef CONFIG_ECMA_COMPACT_PROFILE
        JERRY_UNREACHABLE ();
#else /* !CONFIG_ECMA_COMPACT_PROFILE */
        JERRY_UNIMPLEMENTED ("The built-in is not implemented.");
#endif /* CONFIG_ECMA_COMPACT_PROFILE */
      }
    }
  }

  JERRY_ASSERT (!ecma_is_value_empty (ret_value));

  return ret_value;
} /* ecma_builtin_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in object
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_dispatch_construct (ecma_object_t *obj_p, /**< built-in object */
                                 const ecma_value_t *arguments_list_p, /**< arguments list */
                                 ecma_length_t arguments_list_len) /**< arguments list length */
{
  JERRY_ASSERT (ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_FUNCTION);
  JERRY_ASSERT (ecma_get_object_is_builtin (obj_p));

  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ecma_property_t *built_in_id_prop_p = ecma_get_internal_property (obj_p,
                                                                    ECMA_INTERNAL_PROPERTY_BUILT_IN_ID);
  ecma_builtin_id_t builtin_id = (ecma_builtin_id_t) ecma_get_internal_property_value (built_in_id_prop_p);

  JERRY_ASSERT (ecma_builtin_is (obj_p, builtin_id));

  JERRY_ASSERT (ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_FUNCTION);
  switch (builtin_id)
  {
#define BUILTIN(builtin_id, \
                object_type, \
                object_prototype_builtin_id, \
                is_extensible, \
                is_static, \
                lowercase_name) \
    case builtin_id: \
      { \
        if (object_type == ECMA_OBJECT_TYPE_FUNCTION) \
        { \
          ret_value = ecma_builtin_ ## lowercase_name ## _dispatch_construct (arguments_list_p, \
                                                                              arguments_list_len); \
        } \
        break; \
      }
#include "ecma-builtins.inc.h"

    case ECMA_BUILTIN_ID__COUNT:
    {
      JERRY_UNREACHABLE ();
    }

    default:
    {
#ifdef CONFIG_ECMA_COMPACT_PROFILE
      JERRY_UNREACHABLE ();
#else /* !CONFIG_ECMA_COMPACT_PROFILE */
      JERRY_UNIMPLEMENTED ("The built-in is not implemented.");
#endif /* !CONFIG_ECMA_COMPACT_PROFILE */
    }
  }

  JERRY_ASSERT (!ecma_is_value_empty (ret_value));

  return ret_value;
} /* ecma_builtin_dispatch_construct */

/**
 * Dispatcher of built-in routines
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_dispatch_routine (ecma_builtin_id_t builtin_object_id, /**< built-in object' identifier */
                               uint16_t builtin_routine_id, /**< builtin-wide identifier
                                                             *   of the built-in object's
                                                             *   routine property */
                               ecma_value_t this_arg_value, /**< 'this' argument value */
                               const ecma_value_t arguments_list[], /**< list of arguments passed to routine */
                               ecma_length_t arguments_number) /**< length of arguments' list */
{
  switch (builtin_object_id)
  {
#define BUILTIN(builtin_id, \
                object_type, \
                object_prototype_builtin_id, \
                is_extensible, \
                is_static, \
                lowercase_name) \
    case builtin_id: \
      { \
        return ecma_builtin_ ## lowercase_name ## _dispatch_routine (builtin_routine_id, \
                                                                     this_arg_value, \
                                                                     arguments_list, \
                                                                     arguments_number); \
      }
#include "ecma-builtins.inc.h"

    case ECMA_BUILTIN_ID__COUNT:
    {
      JERRY_UNREACHABLE ();
    }

    default:
    {
#ifdef CONFIG_ECMA_COMPACT_PROFILE
      JERRY_UNREACHABLE ();
#else /* !CONFIG_ECMA_COMPACT_PROFILE */
      JERRY_UNIMPLEMENTED ("The built-in is not implemented.");
#endif /* CONFIG_ECMA_COMPACT_PROFILE */
    }
  }

  JERRY_UNREACHABLE ();
} /* ecma_builtin_dispatch_routine */

/**
 * @}
 * @}
 */
