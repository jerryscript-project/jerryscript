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

#include "ecma-alloc.h"
#include "ecma-builtins.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "jcontext.h"
#include "jrt-bit-fields.h"

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 */

static void ecma_instantiate_builtin (ecma_builtin_id_t id);

/**
 * Helper definition for ecma_builtin_property_list_references.
 */
typedef const ecma_builtin_property_descriptor_t *ecma_builtin_property_list_reference_t;

/**
 * Property descriptor lists for all built-ins.
 */
static const ecma_builtin_property_list_reference_t ecma_builtin_property_list_references[] =
{
#define BUILTIN(builtin_id, \
                object_type, \
                object_prototype_builtin_id, \
                is_extensible, \
                is_static, \
                lowercase_name) \
  ecma_builtin_ ## lowercase_name ## _property_descriptor_list,
#include "ecma-builtins.inc.h"
};

/**
 * Get the number of properties of a built-in object.
 *
 * @return the number of properties
 */
static size_t
ecma_builtin_get_property_count (ecma_builtin_id_t builtin_id)
{
  const ecma_builtin_property_descriptor_t *property_list_p = ecma_builtin_property_list_references[builtin_id];

  const ecma_builtin_property_descriptor_t *curr_property_p = property_list_p;

  while (curr_property_p->magic_string_id != LIT_MAGIC_STRING__COUNT)
  {
    curr_property_p++;
  }

  return (size_t) (curr_property_p - property_list_p);
} /* ecma_builtin_get_property_count */

/**
 * Check if passed object is the instance of specified built-in.
 */
bool
ecma_builtin_is (ecma_object_t *obj_p, /**< pointer to an object */
                 ecma_builtin_id_t builtin_id) /**< id of built-in to check on */
{
  JERRY_ASSERT (obj_p != NULL && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT (builtin_id < ECMA_BUILTIN_ID__COUNT);

  if (JERRY_CONTEXT (ecma_builtin_objects)[builtin_id] == NULL)
  {
    /* If a built-in object is not instantiated,
     * the specified object cannot be the built-in object */
    return false;
  }
  else
  {
    return (obj_p == JERRY_CONTEXT (ecma_builtin_objects)[builtin_id]);
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

  if (unlikely (JERRY_CONTEXT (ecma_builtin_objects)[builtin_id] == NULL))
  {
    ecma_instantiate_builtin (builtin_id);
  }

  ecma_ref_object (JERRY_CONTEXT (ecma_builtin_objects)[builtin_id]);

  return JERRY_CONTEXT (ecma_builtin_objects)[builtin_id];
} /* ecma_builtin_get */

/**
 * Checks whether the given function is a built-in routine
 *
 * @return true - if the function object is a built-in routine
 *         false - otherwise
 */
inline bool __attr_always_inline___
ecma_builtin_function_is_routine (ecma_object_t *func_obj_p) /**< function object */
{
  JERRY_ASSERT (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_FUNCTION);
  JERRY_ASSERT (ecma_get_object_is_builtin (func_obj_p));

  ecma_extended_object_t *ext_func_obj_p = (ecma_extended_object_t *) func_obj_p;
  return (ext_func_obj_p->u.built_in.routine_id >= ECMA_BUILTIN_ID__COUNT);
} /* ecma_builtin_function_is_routine */

/**
 * Initialize specified built-in object.
 *
 * @return pointer to the object
 */
static ecma_object_t *
ecma_builtin_init_object (ecma_builtin_id_t obj_builtin_id, /**< built-in ID */
                          ecma_object_t *prototype_obj_p, /**< prototype object */
                          ecma_object_type_t obj_type, /**< object's type */
                          bool is_extensible) /**< value of object's [[Extensible]] property */
{
  bool is_extended_built_in = (obj_type == ECMA_OBJECT_TYPE_CLASS
                               || obj_type == ECMA_OBJECT_TYPE_ARRAY);

  size_t ext_object_size = (is_extended_built_in ? sizeof (ecma_extended_built_in_object_t)
                                                 : sizeof (ecma_extended_object_t));

  size_t property_count = ecma_builtin_get_property_count (obj_builtin_id);

  if (property_count > 32)
  {
    /* Only 64 extra properties supported at the moment.
     * This can be extended to 256 later. */
    JERRY_ASSERT (property_count <= (32 + 64));

    ext_object_size += sizeof (uint32_t) * 2;
  }

  ecma_object_t *obj_p = ecma_create_object (prototype_obj_p, ext_object_size, obj_type);

  if (!is_extensible)
  {
    ecma_set_object_extensible (obj_p, false);
  }

  /*
   * [[Class]] property of built-in object is not stored explicitly.
   *
   * See also: ecma_object_get_class_name
   */

  ecma_set_object_is_builtin (obj_p);
  ecma_built_in_props_t *built_in_props_p;

  if (is_extended_built_in)
  {
    built_in_props_p = &((ecma_extended_built_in_object_t *) obj_p)->built_in;
  }
  else
  {
    built_in_props_p = &((ecma_extended_object_t *) obj_p)->u.built_in;
  }

  built_in_props_p->id = obj_builtin_id;
  built_in_props_p->routine_id = obj_builtin_id;
  built_in_props_p->instantiated_bitset[0] = 0;

  if (property_count > 32)
  {
    built_in_props_p->length_and_bitset_size = 1 << ECMA_BUILT_IN_BITSET_SHIFT;

    uint32_t *instantiated_bitset_p = built_in_props_p->instantiated_bitset;
    instantiated_bitset_p[1] = 0;
    instantiated_bitset_p[2] = 0;
  }
  else
  {
    built_in_props_p->length_and_bitset_size = 0;
  }

  /** Initializing [[PrimitiveValue]] properties of built-in prototype objects */
  switch (obj_builtin_id)
  {
#ifndef CONFIG_DISABLE_ARRAY_BUILTIN
    case ECMA_BUILTIN_ID_ARRAY_PROTOTYPE:
    {
      JERRY_ASSERT (obj_type == ECMA_OBJECT_TYPE_ARRAY);
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) obj_p;

      ext_object_p->u.array.length = 0;
      ext_object_p->u.array.length_prop = ECMA_PROPERTY_FLAG_WRITABLE | ECMA_PROPERTY_TYPE_VIRTUAL;
      break;
    }
#endif /* !CONFIG_DISABLE_ARRAY_BUILTIN */

#ifndef CONFIG_DISABLE_STRING_BUILTIN
    case ECMA_BUILTIN_ID_STRING_PROTOTYPE:
    {
      JERRY_ASSERT (obj_type == ECMA_OBJECT_TYPE_CLASS);
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) obj_p;

      ext_object_p->u.class_prop.class_id = LIT_MAGIC_STRING_STRING_UL;
      ecma_string_t *prim_prop_str_value_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
      ext_object_p->u.class_prop.u.value = ecma_make_string_value (prim_prop_str_value_p);
      break;
    }
#endif /* !CONFIG_DISABLE_STRING_BUILTIN */

#ifndef CONFIG_DISABLE_NUMBER_BUILTIN
    case ECMA_BUILTIN_ID_NUMBER_PROTOTYPE:
    {
      JERRY_ASSERT (obj_type == ECMA_OBJECT_TYPE_CLASS);
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) obj_p;

      ext_object_p->u.class_prop.class_id = LIT_MAGIC_STRING_NUMBER_UL;
      ext_object_p->u.class_prop.u.value = ecma_make_integer_value (0);
      break;
    }
#endif /* !CONFIG_DISABLE_NUMBER_BUILTIN */

#ifndef CONFIG_DISABLE_BOOLEAN_BUILTIN
    case ECMA_BUILTIN_ID_BOOLEAN_PROTOTYPE:
    {
      JERRY_ASSERT (obj_type == ECMA_OBJECT_TYPE_CLASS);
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) obj_p;

      ext_object_p->u.class_prop.class_id = LIT_MAGIC_STRING_BOOLEAN_UL;
      ext_object_p->u.class_prop.u.value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE);
      break;
    }
#endif /* !CONFIG_DISABLE_BOOLEAN_BUILTIN */

#ifndef CONFIG_DISABLE_DATE_BUILTIN
    case ECMA_BUILTIN_ID_DATE_PROTOTYPE:
    {
      JERRY_ASSERT (obj_type == ECMA_OBJECT_TYPE_CLASS);
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) obj_p;

      ext_object_p->u.class_prop.class_id = LIT_MAGIC_STRING_DATE_UL;

      ecma_number_t *prim_prop_num_value_p = ecma_alloc_number ();
      *prim_prop_num_value_p = ecma_number_make_nan ();
      ECMA_SET_INTERNAL_VALUE_POINTER (ext_object_p->u.class_prop.u.value, prim_prop_num_value_p);
      break;
    }
#endif /* !CONFIG_DISABLE_DATE_BUILTIN */

#ifndef CONFIG_DISABLE_REGEXP_BUILTIN
    case ECMA_BUILTIN_ID_REGEXP_PROTOTYPE:
    {
      JERRY_ASSERT (obj_type == ECMA_OBJECT_TYPE_CLASS);
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) obj_p;

      ext_object_p->u.class_prop.class_id = LIT_MAGIC_STRING_REGEXP_UL;
      ext_object_p->u.class_prop.u.value = ECMA_NULL_POINTER;
      break;
    }
#endif /* !CONFIG_DISABLE_REGEXP_BUILTIN */
    default:
    {
      JERRY_ASSERT (obj_type != ECMA_OBJECT_TYPE_CLASS);
      break;
    }
  }

  return obj_p;
} /* ecma_builtin_init_object */

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
      JERRY_ASSERT (JERRY_CONTEXT (ecma_builtin_objects)[builtin_id] == NULL); \
      \
      ecma_object_t *prototype_obj_p; \
      if (object_prototype_builtin_id == ECMA_BUILTIN_ID__COUNT) \
      { \
        prototype_obj_p = NULL; \
      } \
      else \
      { \
        if (JERRY_CONTEXT (ecma_builtin_objects)[object_prototype_builtin_id] == NULL) \
        { \
          ecma_instantiate_builtin (object_prototype_builtin_id); \
        } \
        prototype_obj_p = JERRY_CONTEXT (ecma_builtin_objects)[object_prototype_builtin_id]; \
        JERRY_ASSERT (prototype_obj_p != NULL); \
      } \
      \
      ecma_object_t *builtin_obj_p = ecma_builtin_init_object (builtin_id, \
                                                               prototype_obj_p, \
                                                               object_type, \
                                                               is_extensible); \
      JERRY_CONTEXT (ecma_builtin_objects)[builtin_id] = builtin_obj_p; \
      \
      break; \
    }
#include "ecma-builtins.inc.h"

    default:
    {
      JERRY_ASSERT (id < ECMA_BUILTIN_ID__COUNT);

      JERRY_UNREACHABLE (); /* The built-in is not implemented. */
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
    if (JERRY_CONTEXT (ecma_builtin_objects)[id] != NULL)
    {
      ecma_deref_object (JERRY_CONTEXT (ecma_builtin_objects)[id]);
      JERRY_CONTEXT (ecma_builtin_objects)[id] = NULL;
    }
  }
} /* ecma_finalize_builtins */

/**
 * Construct a Function object for specified built-in routine
 *
 * See also: ECMA-262 v5, 15
 *
 * @return pointer to constructed Function object
 */
static ecma_object_t *
ecma_builtin_make_function_object_for_routine (ecma_builtin_id_t builtin_id, /**< identifier of built-in object */
                                               uint16_t routine_id, /**< builtin-wide identifier of the built-in
                                                                     *   object's routine property */
                                               uint8_t length_prop_value) /**< value of 'length' property */
{
  JERRY_ASSERT (length_prop_value < (1 << ECMA_BUILT_IN_BITSET_SHIFT));

  ecma_object_t *prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE);

  size_t ext_object_size = sizeof (ecma_extended_object_t);

  size_t property_count = ecma_builtin_get_property_count (builtin_id);

  if (property_count > 32)
  {
    /* Only 64 extra properties supported at the moment.
     * This can be extended to 256 later. */
    JERRY_ASSERT (property_count <= (32 + 64));

    ext_object_size += sizeof (uint32_t) * 2;
  }

  ecma_object_t *func_obj_p = ecma_create_object (prototype_obj_p,
                                                  ext_object_size,
                                                  ECMA_OBJECT_TYPE_FUNCTION);

  ecma_deref_object (prototype_obj_p);

  ecma_set_object_is_builtin (func_obj_p);

  JERRY_ASSERT (routine_id >= ECMA_BUILTIN_ID__COUNT);

  ecma_extended_object_t *ext_func_obj_p = (ecma_extended_object_t *) func_obj_p;
  ext_func_obj_p->u.built_in.id = builtin_id;
  ext_func_obj_p->u.built_in.routine_id = routine_id;
  ext_func_obj_p->u.built_in.instantiated_bitset[0] = 0;

  if (property_count > 32)
  {
    length_prop_value = (uint8_t) (length_prop_value | (1 << ECMA_BUILT_IN_BITSET_SHIFT));

    uint32_t *instantiated_bitset_p = ext_func_obj_p->u.built_in.instantiated_bitset;
    instantiated_bitset_p[1] = 0;
    instantiated_bitset_p[2] = 0;
  }

  ext_func_obj_p->u.built_in.length_and_bitset_size = length_prop_value;

  return func_obj_p;
} /* ecma_builtin_make_function_object_for_routine */

/**
 * Construct a Function object for specified built-in accessor getter
 *
 * @return pointer to constructed accessor getter Function object
 */
static ecma_object_t *
ecma_builtin_make_function_object_for_getter_accessor (ecma_builtin_id_t builtin_id, /**< id of built-in object */
                                                       uint16_t routine_id) /**< builtin-wide id of the built-in
                                                                            *   object's routine property */
{
  return ecma_builtin_make_function_object_for_routine (builtin_id, routine_id, 0);
} /* ecma_builtin_make_function_object_for_getter_accessor */

/**
 * Construct a Function object for specified built-in accessor setter
 *
 * @return pointer to constructed accessor getter Function object
 */
static ecma_object_t *
ecma_builtin_make_function_object_for_setter_accessor (ecma_builtin_id_t builtin_id, /**< id of built-in object */
                                                       uint16_t routine_id) /**< builtin-wide id of the built-in
                                                                            *   object's routine property */
{
  return ecma_builtin_make_function_object_for_routine (builtin_id, routine_id, 1);
} /* ecma_builtin_make_function_object_for_setter_accessor */

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

  if (ecma_get_object_type (object_p) == ECMA_OBJECT_TYPE_FUNCTION
      && ecma_builtin_function_is_routine (object_p))
  {
    if (ecma_string_is_length (string_p))
    {
      /*
       * Lazy instantiation of 'length' property
       *
       * Note:
       *      We don't need to mark that the property was already lazy instantiated,
       *      as it is non-configurable and so can't be deleted
       */

      ecma_property_t *len_prop_p;
      ecma_property_value_t *len_prop_value_p = ecma_create_named_data_property (object_p,
                                                                                 string_p,
                                                                                 ECMA_PROPERTY_FIXED,
                                                                                 &len_prop_p);

      ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) object_p;

      uint8_t length = ext_obj_p->u.built_in.length_and_bitset_size & ((1 << ECMA_BUILT_IN_BITSET_SHIFT) - 1);

      len_prop_value_p->value = ecma_make_integer_value (length);

      return len_prop_p;
    }

    return NULL;
  }

  lit_magic_string_id_t magic_string_id = ecma_get_string_magic (string_p);

  if (magic_string_id == LIT_MAGIC_STRING__COUNT)
  {
    return NULL;
  }

  ecma_built_in_props_t *built_in_props_p;
  ecma_object_type_t object_type = ecma_get_object_type (object_p);

  if (object_type == ECMA_OBJECT_TYPE_CLASS || object_type == ECMA_OBJECT_TYPE_ARRAY)
  {
    built_in_props_p = &((ecma_extended_built_in_object_t *) object_p)->built_in;
  }
  else
  {
    built_in_props_p = &((ecma_extended_object_t *) object_p)->u.built_in;
  }

  ecma_builtin_id_t builtin_id = (ecma_builtin_id_t) built_in_props_p->id;

  JERRY_ASSERT (builtin_id < ECMA_BUILTIN_ID__COUNT);
  JERRY_ASSERT (ecma_builtin_is (object_p, builtin_id));

  const ecma_builtin_property_descriptor_t *property_list_p = ecma_builtin_property_list_references[builtin_id];

  const ecma_builtin_property_descriptor_t *curr_property_p = property_list_p;

  while (curr_property_p->magic_string_id != magic_string_id)
  {
    if (curr_property_p->magic_string_id == LIT_MAGIC_STRING__COUNT)
    {
      return NULL;
    }
    curr_property_p++;
  }

  uint32_t index = (uint32_t) (curr_property_p - property_list_p);

  uint32_t *bitset_p = built_in_props_p->instantiated_bitset + (index >> 5);

  uint32_t bit_for_index = (uint32_t) (1u << (index & 0x1f));

  if (*bitset_p & bit_for_index)
  {
    /* This property was instantiated before. */
    return NULL;
  }

  *bitset_p |= bit_for_index;

  ecma_value_t value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
  bool is_accessor = false;
  ecma_object_t *getter_p = NULL;
  ecma_object_t *setter_p = NULL;

  switch (curr_property_p->type)
  {
    case ECMA_BUILTIN_PROPERTY_SIMPLE:
    {
      value = ecma_make_simple_value (curr_property_p->value);
      break;
    }
    case ECMA_BUILTIN_PROPERTY_NUMBER:
    {
      ecma_number_t num = 0.0;

      if (curr_property_p->value < ECMA_BUILTIN_NUMBER_MAX)
      {
        num = curr_property_p->value;
      }
      else if (curr_property_p->value < ECMA_BUILTIN_NUMBER_NAN)
      {
        static const ecma_number_t builtin_number_list[] =
        {
          ECMA_NUMBER_MAX_VALUE,
          ECMA_NUMBER_MIN_VALUE,
          ECMA_NUMBER_E,
          ECMA_NUMBER_PI,
          ECMA_NUMBER_LN10,
          ECMA_NUMBER_LN2,
          ECMA_NUMBER_LOG2E,
          ECMA_NUMBER_LOG10E,
          ECMA_NUMBER_SQRT2,
          ECMA_NUMBER_SQRT_1_2
        };

        num = builtin_number_list[curr_property_p->value - ECMA_BUILTIN_NUMBER_MAX];
      }
      else
      {
        switch (curr_property_p->value)
        {
          case ECMA_BUILTIN_NUMBER_NAN:
          {
            num = ecma_number_make_nan ();
            break;
          }
          case ECMA_BUILTIN_NUMBER_POSITIVE_INFINITY:
          {
            num = ecma_number_make_infinity (false);
            break;
          }
          case ECMA_BUILTIN_NUMBER_NEGATIVE_INFINITY:
          {
            num = ecma_number_make_infinity (true);
            break;
          }
          default:
          {
            JERRY_UNREACHABLE ();
            break;
          }
        }
      }

      value = ecma_make_number_value (num);
      break;
    }
    case ECMA_BUILTIN_PROPERTY_STRING:
    {
      value = ecma_make_string_value (ecma_get_magic_string (curr_property_p->value));
      break;
    }
    case ECMA_BUILTIN_PROPERTY_OBJECT:
    {
      value = ecma_make_object_value (ecma_builtin_get (curr_property_p->value));
      break;
    }
    case ECMA_BUILTIN_PROPERTY_ROUTINE:
    {
      ecma_object_t *func_obj_p;
      func_obj_p = ecma_builtin_make_function_object_for_routine (builtin_id,
                                                                  ECMA_GET_ROUTINE_ID (curr_property_p->value),
                                                                  ECMA_GET_ROUTINE_LENGTH (curr_property_p->value));
      value = ecma_make_object_value (func_obj_p);
      break;
    }
    case ECMA_BUILTIN_PROPERTY_ACCESSOR_READ_WRITE:
    {
      is_accessor = true;
      uint16_t getter_id = ECMA_ACCESSOR_READ_WRITE_GET_GETTER_ID (curr_property_p->value);
      uint16_t setter_id = ECMA_ACCESSOR_READ_WRITE_GET_SETTER_ID (curr_property_p->value);
      getter_p = ecma_builtin_make_function_object_for_getter_accessor (builtin_id, getter_id);
      setter_p = ecma_builtin_make_function_object_for_setter_accessor (builtin_id, setter_id);
      break;
    }
    case ECMA_BUILTIN_PROPERTY_ACCESSOR_READ_ONLY:
    {
      is_accessor = true;
      getter_p = ecma_builtin_make_function_object_for_getter_accessor (builtin_id,
                                                                        curr_property_p->value);
      break;
    }
    default:
    {
      JERRY_UNREACHABLE ();
      return NULL;
    }
  }

  ecma_property_t *prop_p;

  if (is_accessor)
  {
    ecma_create_named_accessor_property (object_p,
                                         string_p,
                                         getter_p,
                                         setter_p,
                                         curr_property_p->attributes,
                                         &prop_p);

    if (setter_p)
    {
      ecma_deref_object (setter_p);
    }
    if (getter_p)
    {
      ecma_deref_object (getter_p);
    }
  }
  else
  {
    ecma_property_value_t *prop_value_p = ecma_create_named_data_property (object_p,
                                                                           string_p,
                                                                           curr_property_p->attributes,
                                                                           &prop_p);
    prop_value_p->value = value;

    /* Reference count of objects must be decreased. */
    if (ecma_is_value_object (value))
    {
      ecma_free_value (value);
    }
  }

  return prop_p;
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
  JERRY_ASSERT (ecma_get_object_is_builtin (object_p));

  if (ecma_get_object_type (object_p) == ECMA_OBJECT_TYPE_FUNCTION
      && ecma_builtin_function_is_routine (object_p))
  {
    ecma_collection_header_t *for_enumerable_p = main_collection_p;
    JERRY_UNUSED (for_enumerable_p);

    ecma_collection_header_t *for_non_enumerable_p = separate_enumerable ? non_enum_collection_p : main_collection_p;

    /* 'length' property is non-enumerable (ECMA-262 v5, 15) */
    ecma_string_t *name_p = ecma_new_ecma_length_string ();
    ecma_append_to_values_collection (for_non_enumerable_p, ecma_make_string_value (name_p), true);
    ecma_deref_ecma_string (name_p);
  }
  else
  {
    ecma_built_in_props_t *built_in_props_p;
    ecma_object_type_t object_type = ecma_get_object_type (object_p);

    if (object_type == ECMA_OBJECT_TYPE_CLASS || object_type == ECMA_OBJECT_TYPE_ARRAY)
    {
      built_in_props_p = &((ecma_extended_built_in_object_t *) object_p)->built_in;
    }
    else
    {
      built_in_props_p = &((ecma_extended_object_t *) object_p)->u.built_in;
    }

    ecma_builtin_id_t builtin_id = (ecma_builtin_id_t) built_in_props_p->id;

    JERRY_ASSERT (builtin_id < ECMA_BUILTIN_ID__COUNT);
    JERRY_ASSERT (ecma_builtin_is (object_p, builtin_id));

    const ecma_builtin_property_descriptor_t *curr_property_p = ecma_builtin_property_list_references[builtin_id];

    ecma_length_t index = 0;
    uint32_t *bitset_p = built_in_props_p->instantiated_bitset;

    ecma_collection_header_t *for_non_enumerable_p = (separate_enumerable ? non_enum_collection_p
                                                                          : main_collection_p);

    while (curr_property_p->magic_string_id != LIT_MAGIC_STRING__COUNT)
    {
      if (index == 32)
      {
        bitset_p++;
        index = 0;
      }

      ecma_string_t *name_p = ecma_get_magic_string (curr_property_p->magic_string_id);

      uint32_t bit_for_index = (uint32_t) 1u << index;

      if (!(*bitset_p & bit_for_index) || ecma_op_object_has_own_property (object_p, name_p))
      {
        ecma_append_to_values_collection (for_non_enumerable_p,
                                          ecma_make_string_value (name_p),
                                          true);
      }

      ecma_deref_ecma_string (name_p);

      curr_property_p++;
      index++;
    }
  }
} /* ecma_builtin_list_lazy_property_names */

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
      JERRY_UNREACHABLE (); /* The built-in is not implemented. */
    }
  }

  JERRY_UNREACHABLE ();
} /* ecma_builtin_dispatch_routine */

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
  JERRY_ASSERT (ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_FUNCTION);
  JERRY_ASSERT (ecma_get_object_is_builtin (obj_p));

  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
  ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

  if (ecma_builtin_function_is_routine (obj_p))
  {
    ret_value = ecma_builtin_dispatch_routine (ext_obj_p->u.built_in.id,
                                               ext_obj_p->u.built_in.routine_id,
                                               this_arg_value,
                                               arguments_list_p,
                                               arguments_list_len);
  }
  else
  {
    switch ((ecma_builtin_id_t) ext_obj_p->u.built_in.id)
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
        JERRY_UNREACHABLE (); /* The built-in is not implemented. */
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

  ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

  switch (ext_obj_p->u.built_in.id)
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
      JERRY_UNREACHABLE (); /* The built-in is not implemented. */
    }
  }

  JERRY_ASSERT (!ecma_is_value_empty (ret_value));

  return ret_value;
} /* ecma_builtin_dispatch_construct */

/**
 * @}
 * @}
 */
