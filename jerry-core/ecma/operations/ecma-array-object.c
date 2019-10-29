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
#include "ecma-array-object.h"
#include "ecma-builtin-helpers.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-property-hashmap.h"
#include "ecma-helpers.h"
#include "ecma-number-arithmetic.h"
#include "ecma-objects.h"
#include "ecma-objects-general.h"
#include "ecma-function-object.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaarrayobject ECMA Array object related routines
 * @{
 */

#if ENABLED (JERRY_CPOINTER_32_BIT)
/**
 * Maximum length of the array length to allocate fast mode access for it
 * e.g. new Array(5000) is constructed as fast mode access array,
 * but new Array(50000000) is consturcted as normal property list based array
 */
#define ECMA_FAST_ARRAY_MAX_INITIAL_LENGTH (1 << 17)
#else /* ENABLED (JERRY_CPOINTER_32_BIT) */
/**
 * Maximum length of the array length to allocate fast mode access for it
 * e.g. new Array(5000) is constructed as fast mode access array,
 * but new Array(50000000) is consturcted as normal property list based array
 */
#define ECMA_FAST_ARRAY_MAX_INITIAL_LENGTH (1 << 13)
#endif /* ENABLED (JERRY_CPOINTER_32_BIT) */

/**
 * Property name type flag for array indices.
 */
#define ECMA_FAST_ARRAY_UINT32_DIRECT_STRING_PROP_TYPE 0x80

/**
 * Property attribute for the array 'length' virtual property to indicate fast access mode array
 */
#define ECMA_FAST_ARRAY_FLAG (ECMA_DIRECT_STRING_MAGIC << ECMA_PROPERTY_NAME_TYPE_SHIFT)

/**
 * Allocate a new array object with the given length
 *
 * @return pointer to the constructed array object
 */
ecma_object_t *
ecma_op_new_array_object (ecma_length_t length) /**< length of the new array */
{
#if ENABLED (JERRY_BUILTIN_ARRAY)
  ecma_object_t *array_prototype_object_p = ecma_builtin_get (ECMA_BUILTIN_ID_ARRAY_PROTOTYPE);
#else /* !ENABLED (JERRY_BUILTIN_ARRAY) */
  ecma_object_t *array_prototype_object_p = ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE);
#endif /* (ENABLED (JERRY_BUILTIN_ARRAY)) */

  ecma_object_t *object_p = ecma_create_object (array_prototype_object_p,
                                                sizeof (ecma_extended_object_t),
                                                ECMA_OBJECT_TYPE_ARRAY);

  /*
   * [[Class]] property is not stored explicitly for objects of ECMA_OBJECT_TYPE_ARRAY type.
   *
   * See also: ecma_object_get_class_name
   */

  ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) object_p;
  ext_obj_p->u.array.length = length;
  ext_obj_p->u.array.u.length_prop = ECMA_PROPERTY_FLAG_WRITABLE | ECMA_PROPERTY_TYPE_VIRTUAL;

  return object_p;
} /* ecma_op_new_array_object */

/**
 * Check whether the given object is fast-access mode array
 *
 * @return true - if the object is fast-access mode array
 *         false, otherwise
 */
inline bool JERRY_ATTR_ALWAYS_INLINE
ecma_op_object_is_fast_array (ecma_object_t *object_p) /**< ecma-object */
{
  return (ecma_get_object_type (object_p) == ECMA_OBJECT_TYPE_ARRAY &&
          ecma_op_array_is_fast_array ((ecma_extended_object_t *) object_p));
} /* ecma_op_object_is_fast_array */

/**
 * Check whether the given array object is fast-access mode array
 *
 * @return true - if the array object is fast-access mode array
 *         false, otherwise
 */
inline bool JERRY_ATTR_ALWAYS_INLINE
ecma_op_array_is_fast_array (ecma_extended_object_t *array_p) /**< ecma-array-object */
{
  JERRY_ASSERT (ecma_get_object_type ((ecma_object_t *) array_p) == ECMA_OBJECT_TYPE_ARRAY);

  return array_p->u.array.u.length_prop & ECMA_FAST_ARRAY_FLAG;
} /* ecma_op_array_is_fast_array */

/**
 * Allocate a new fast access mode array object with the given length
 *
 * @return NULL - if the allocation of the underlying buffer failed
 *         pointer to the constructed fast access mode array object otherwise
 */
ecma_object_t *
ecma_op_new_fast_array_object (ecma_length_t length) /**< length of the new fast access mode array */
{
  const uint32_t aligned_length = ECMA_FAST_ARRAY_ALIGN_LENGTH (length);
  ecma_value_t *values_p = NULL;

  if (length != 0)
  {
    values_p = (ecma_value_t *) jmem_heap_alloc_block_null_on_error (aligned_length * sizeof (ecma_value_t));

    if (JERRY_UNLIKELY (values_p == NULL))
    {
      return NULL;
    }
  }

  ecma_object_t *object_p = ecma_op_new_array_object (length);
  ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) object_p;

  ext_obj_p->u.array.u.length_prop = (uint8_t) (ext_obj_p->u.array.u.length_prop | ECMA_FAST_ARRAY_FLAG);
  ext_obj_p->u.array.u.hole_count += length * ECMA_FAST_ARRAY_HOLE_ONE;

  JERRY_ASSERT (object_p->u1.property_list_cp == JMEM_CP_NULL);

  for (uint32_t i = 0; i < aligned_length; i++)
  {
    values_p[i] = ECMA_VALUE_ARRAY_HOLE;
  }

  ECMA_SET_POINTER (object_p->u1.property_list_cp, values_p);
  return object_p;
} /* ecma_op_new_fast_array_object */

/**
 * Converts a fast access mode array back to a normal property list based array
 */
void
ecma_fast_array_convert_to_normal (ecma_object_t *object_p) /**< fast access mode array object */
{
  JERRY_ASSERT (ecma_op_object_is_fast_array (object_p));

  ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) object_p;

  if (object_p->u1.property_list_cp == JMEM_CP_NULL)
  {
    ext_obj_p->u.array.u.length_prop = (uint8_t) (ext_obj_p->u.array.u.length_prop & ~ECMA_FAST_ARRAY_FLAG);
    return;
  }

  uint32_t length = ext_obj_p->u.array.length;
  const uint32_t aligned_length = ECMA_FAST_ARRAY_ALIGN_LENGTH (length);
  ecma_value_t *values_p = ECMA_GET_NON_NULL_POINTER (ecma_value_t, object_p->u1.property_list_cp);

  ecma_ref_object (object_p);

  ecma_property_pair_t *property_pair_p = NULL;
  jmem_cpointer_t next_property_pair_cp = JMEM_CP_NULL;

  uint32_t prop_index = 1;
  int32_t index = (int32_t) (length - 1);

  while (index >= 0)
  {
    if (ecma_is_value_array_hole (values_p[index]))
    {
      index--;
      continue;
    }

    if (prop_index == 1)
    {
      property_pair_p = ecma_alloc_property_pair ();
      property_pair_p->header.next_property_cp = next_property_pair_cp;
      property_pair_p->names_cp[0] = LIT_INTERNAL_MAGIC_STRING_DELETED;
      property_pair_p->header.types[0] = ECMA_PROPERTY_TYPE_DELETED;
      ECMA_SET_NON_NULL_POINTER (next_property_pair_cp, property_pair_p);
    }

    JERRY_ASSERT (index <= ECMA_DIRECT_STRING_MAX_IMM);

    property_pair_p->names_cp[prop_index] = (jmem_cpointer_t) index;
    property_pair_p->header.types[prop_index] = (ecma_property_t) (ECMA_PROPERTY_TYPE_NAMEDDATA
                                                                   | ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE
                                                                   | ECMA_FAST_ARRAY_UINT32_DIRECT_STRING_PROP_TYPE);

    property_pair_p->values[prop_index].value = values_p[index];

    index--;
    prop_index = !prop_index;
  }

  ext_obj_p->u.array.u.length_prop = (uint8_t) (ext_obj_p->u.array.u.length_prop & ~ECMA_FAST_ARRAY_FLAG);
  jmem_heap_free_block (values_p, aligned_length * sizeof (ecma_value_t));
  ECMA_SET_POINTER (object_p->u1.property_list_cp, property_pair_p);

  ecma_deref_object (object_p);
} /* ecma_fast_array_convert_to_normal */

/**
 * [[Put]] operation for a fast access mode array
 *
 * @return false - If the property name is not array index, or the requested index to be set
 *                 would result too much array hole in the underlying buffer. The these cases
 *                 the array is converted back to normal property list based array.
 *         true - If the indexed property can be set with/without resizing the underlying buffer.
 */
bool
ecma_fast_array_set_property (ecma_object_t *object_p, /**< fast access mode array object */
                              uint32_t index, /**< property name index */
                              ecma_value_t value) /**< value to be set */
{
  JERRY_ASSERT (ecma_op_object_is_fast_array (object_p));
  ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) object_p;
  uint32_t old_length = ext_obj_p->u.array.length;

  ecma_value_t *values_p;

  if (JERRY_LIKELY (index < old_length))
  {
    JERRY_ASSERT (object_p->u1.property_list_cp != JMEM_CP_NULL);

    values_p = ECMA_GET_NON_NULL_POINTER (ecma_value_t, object_p->u1.property_list_cp);

    if (ecma_is_value_array_hole (values_p[index]))
    {
      ext_obj_p->u.array.u.hole_count -= ECMA_FAST_ARRAY_HOLE_ONE;
    }
    else
    {
      ecma_free_value_if_not_object (values_p[index]);
    }

    values_p[index] = ecma_copy_value_if_not_object (value);

    return true;
  }

  uint32_t old_holes = ext_obj_p->u.array.u.hole_count;
  uint32_t new_holes = index - old_length;

  if (JERRY_UNLIKELY (new_holes > ECMA_FAST_ARRAY_MAX_NEW_HOLES_COUNT
                      || ((old_holes >> ECMA_FAST_ARRAY_HOLE_SHIFT) + new_holes) > ECMA_FAST_ARRAY_MAX_HOLE_COUNT))
  {
    ecma_fast_array_convert_to_normal (object_p);

    return false;
  }

  uint32_t new_length = index + 1;

  JERRY_ASSERT (new_length < UINT32_MAX);

  const uint32_t aligned_length = ECMA_FAST_ARRAY_ALIGN_LENGTH (old_length);

  if (JERRY_LIKELY (index < aligned_length))
  {
    JERRY_ASSERT (object_p->u1.property_list_cp != JMEM_CP_NULL);

    values_p = ECMA_GET_NON_NULL_POINTER (ecma_value_t, object_p->u1.property_list_cp);
    /* This area is filled with ECMA_VALUE_ARRAY_HOLE, but not counted in u.array.u.hole_count */
    JERRY_ASSERT (ecma_is_value_array_hole (values_p[index]));
    ext_obj_p->u.array.u.hole_count += new_holes * ECMA_FAST_ARRAY_HOLE_ONE;
    ext_obj_p->u.array.length = new_length;
  }
  else
  {
    values_p = ecma_fast_array_extend (object_p, new_length);
    ext_obj_p->u.array.u.hole_count -= ECMA_FAST_ARRAY_HOLE_ONE;
  }

  values_p[index] = ecma_copy_value_if_not_object (value);

  return true;
} /* ecma_fast_array_set_property */


/**
 * Extend the underlying buffer of a fast mode access array for the given new length
 *
 * @return pointer to the extended underlying buffer
 */
ecma_value_t *
ecma_fast_array_extend (ecma_object_t *object_p, /**< fast access mode array object */
                        uint32_t new_length) /**< new length of the fast access mode array */
{
  JERRY_ASSERT (ecma_op_object_is_fast_array (object_p));
  ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) object_p;
  uint32_t old_length = ext_obj_p->u.array.length;

  JERRY_ASSERT (old_length < new_length);

  ecma_ref_object (object_p);

  ecma_value_t *new_values_p;
  const uint32_t old_length_aligned = ECMA_FAST_ARRAY_ALIGN_LENGTH (old_length);
  const uint32_t new_length_aligned = ECMA_FAST_ARRAY_ALIGN_LENGTH (new_length);

  if (object_p->u1.property_list_cp == JMEM_CP_NULL)
  {
    new_values_p = jmem_heap_alloc_block (new_length_aligned * sizeof (ecma_value_t));
  }
  else
  {
    ecma_value_t *values_p = ECMA_GET_NON_NULL_POINTER (ecma_value_t, object_p->u1.property_list_cp);
    new_values_p = (ecma_value_t *) jmem_heap_realloc_block (values_p,
                                                             old_length_aligned * sizeof (ecma_value_t),
                                                             new_length_aligned * sizeof (ecma_value_t));
  }

  for (uint32_t i = old_length; i < new_length_aligned; i++)
  {
    new_values_p[i] = ECMA_VALUE_ARRAY_HOLE;
  }

  ext_obj_p->u.array.u.hole_count += (new_length - old_length) * ECMA_FAST_ARRAY_HOLE_ONE;
  ext_obj_p->u.array.length = new_length;

  ECMA_SET_NON_NULL_POINTER (object_p->u1.property_list_cp, new_values_p);

  ecma_deref_object (object_p);
  return new_values_p;
} /* ecma_fast_array_extend */

/**
 * Delete the array object's property referenced by its value pointer.
 *
 * Note: specified property must be owned by specified object.
 */
void
ecma_array_object_delete_property (ecma_object_t *object_p, /**< object */
                                   ecma_string_t *property_name_p, /**< property name */
                                   ecma_property_value_t *prop_value_p) /**< property value reference */
{
  JERRY_ASSERT (ecma_get_object_type (object_p) == ECMA_OBJECT_TYPE_ARRAY);
  ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) object_p;

  if (!ecma_op_object_is_fast_array (object_p))
  {
    ecma_delete_property (object_p, prop_value_p);
    return;
  }

  JERRY_ASSERT (object_p->u1.property_list_cp != JMEM_CP_NULL);

  uint32_t index = ecma_string_get_array_index (property_name_p);

  JERRY_ASSERT (index != ECMA_STRING_NOT_ARRAY_INDEX);
  JERRY_ASSERT (index < ext_obj_p->u.array.length);

  ecma_value_t *values_p = ECMA_GET_NON_NULL_POINTER (ecma_value_t, object_p->u1.property_list_cp);

  if (ecma_is_value_array_hole (values_p[index]))
  {
    return;
  }

  ecma_free_value_if_not_object (values_p[index]);

  values_p[index] = ECMA_VALUE_ARRAY_HOLE;
  ext_obj_p->u.array.u.hole_count += ECMA_FAST_ARRAY_HOLE_ONE;
} /* ecma_array_object_delete_property */

/**
 * Low level delete of fast access mode array items
 *
 * @return the updated value of new_length
 */
uint32_t
ecma_delete_fast_array_properties (ecma_object_t *object_p, /**< fast access mode array */
                                   uint32_t new_length) /**< new length of the fast access mode array */
{
  JERRY_ASSERT (ecma_op_object_is_fast_array (object_p));

  ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) object_p;

  ecma_ref_object (object_p);
  ecma_value_t *values_p = ECMA_GET_NON_NULL_POINTER (ecma_value_t, object_p->u1.property_list_cp);

  uint32_t old_length = ext_obj_p->u.array.length;
  const uint32_t old_aligned_length = ECMA_FAST_ARRAY_ALIGN_LENGTH (old_length);
  JERRY_ASSERT (new_length < old_length);

  for (uint32_t i = new_length; i < old_length; i++)
  {
    if (ecma_is_value_array_hole (values_p[i]))
    {
      ext_obj_p->u.array.u.hole_count -= ECMA_FAST_ARRAY_HOLE_ONE;
    }
    else
    {
      ecma_free_value_if_not_object (values_p[i]);
    }
  }

  jmem_cpointer_t new_property_list_cp;

  if (new_length == 0)
  {
    jmem_heap_free_block (values_p, old_aligned_length * sizeof (ecma_value_t));
    new_property_list_cp = JMEM_CP_NULL;
  }
  else
  {
    const uint32_t new_aligned_length = ECMA_FAST_ARRAY_ALIGN_LENGTH (new_length);

    ecma_value_t *new_values_p;
    new_values_p = (ecma_value_t *) jmem_heap_realloc_block (values_p,
                                                             old_aligned_length * sizeof (ecma_value_t),
                                                             new_aligned_length * sizeof (ecma_value_t));

    for (uint32_t i = new_length; i < new_aligned_length; i++)
    {
      new_values_p[i] = ECMA_VALUE_ARRAY_HOLE;
    }

    ECMA_SET_NON_NULL_POINTER (new_property_list_cp, new_values_p);
  }

  ext_obj_p->u.array.length = new_length;
  object_p->u1.property_list_cp = new_property_list_cp;

  ecma_deref_object (object_p);

  return new_length;
} /* ecma_delete_fast_array_properties */

/**
 * Update the length of a fast access mode array to a new length
 *
 * Note: if the new length would result too much array hole in the underlying arraybuffer
 *       the array is converted back to normal property list based array
 */
static void
ecma_fast_array_set_length (ecma_object_t *object_p, /**< fast access mode array object */
                            uint32_t new_length) /**< new length of the fast access mode array object*/
{
  JERRY_ASSERT (ecma_op_object_is_fast_array (object_p));

  ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) object_p;
  uint32_t old_length = ext_obj_p->u.array.length;

  JERRY_ASSERT (new_length >= old_length);

  if (new_length == old_length)
  {
    return;
  }

  uint32_t old_holes = ext_obj_p->u.array.u.hole_count;
  uint32_t new_holes = new_length - old_length;

  if (JERRY_UNLIKELY (new_holes > ECMA_FAST_ARRAY_MAX_NEW_HOLES_COUNT
                      || ((old_holes >> ECMA_FAST_ARRAY_HOLE_SHIFT) + new_holes) > ECMA_FAST_ARRAY_MAX_HOLE_COUNT))
  {
    ecma_fast_array_convert_to_normal (object_p);
  }
  else
  {
    ecma_fast_array_extend (object_p, new_length);
  }

  return;
} /* ecma_fast_array_set_length */

/**
 * Get collection of property names of a fast access mode array object
 *
 * Note: Since the fast array object only contains indexed, enumerable, writable, configurable properties
 *       we can return a collection of non-array hole array indices
 *
 * @return collection of strings - property names
 */
ecma_collection_t *
ecma_fast_array_get_property_names (ecma_object_t *object_p, /**< fast access mode array object */
                                    uint32_t opts) /**< any combination of ecma_list_properties_options_t values */
{
  JERRY_ASSERT (ecma_op_object_is_fast_array (object_p));

  ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) object_p;
  ecma_collection_t *ret_p = ecma_new_collection ();

#if ENABLED (JERRY_ES2015)
  if (opts & ECMA_LIST_SYMBOLS)
  {
    return ret_p;
  }
#endif /* ENABLED (JERRY_ES2015) */

  uint32_t length = ext_obj_p->u.array.length;

  bool append_length = (!(opts & ECMA_LIST_ENUMERABLE) && !(opts & ECMA_LIST_ARRAY_INDICES));

  if (length == 0)
  {
    if (append_length)
    {
      ecma_collection_push_back (ret_p, ecma_make_magic_string_value (LIT_MAGIC_STRING_LENGTH));
    }

    return ret_p;
  }

  ecma_value_t *values_p = ECMA_GET_NON_NULL_POINTER (ecma_value_t, object_p->u1.property_list_cp);

  for (uint32_t i = 0; i < length; i++)
  {
    if (ecma_is_value_array_hole (values_p[i]))
    {
      continue;
    }

    ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (i);

    ecma_collection_push_back (ret_p, ecma_make_string_value (index_str_p));
  }

  if (append_length)
  {
    ecma_collection_push_back (ret_p, ecma_make_magic_string_value (LIT_MAGIC_STRING_LENGTH));
  }

  if (opts & ECMA_LIST_CONVERT_FAST_ARRAYS)
  {
    ecma_fast_array_convert_to_normal (object_p);
  }

  return ret_p;
} /* ecma_fast_array_get_property_names */

/**
 * Array object creation operation.
 *
 * See also: ECMA-262 v5, 15.4.2.1
 *           ECMA-262 v5, 15.4.2.2
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_create_array_object (const ecma_value_t *arguments_list_p, /**< list of arguments that
                                                                    *   are passed to Array constructor */
                             ecma_length_t arguments_list_len, /**< length of the arguments' list */
                             bool is_treat_single_arg_as_length) /**< if the value is true,
                                                                  *   arguments_list_len is 1
                                                                  *   and single argument is Number,
                                                                  *   then treat the single argument
                                                                  *   as new Array's length rather
                                                                  *   than as single item of the Array */
{
  JERRY_ASSERT (arguments_list_len == 0
                || arguments_list_p != NULL);

  uint32_t length;
  const ecma_value_t *array_items_p;
  ecma_length_t array_items_count;

  if (is_treat_single_arg_as_length
      && arguments_list_len == 1
      && ecma_is_value_number (arguments_list_p[0]))
  {
    ecma_number_t num = ecma_get_number_from_value (arguments_list_p[0]);
    uint32_t num_uint32 = ecma_number_to_uint32 (num);

    if (num != ((ecma_number_t) num_uint32))
    {
      return ecma_raise_range_error (ECMA_ERR_MSG ("Invalid array length."));
    }
    else
    {
      length = num_uint32;
      array_items_p = NULL;
      array_items_count = 0;
    }

    if (length > ECMA_FAST_ARRAY_MAX_INITIAL_LENGTH)
    {
      return ecma_make_object_value (ecma_op_new_array_object (length));
    }

    ecma_object_t *object_p = ecma_op_new_fast_array_object (length);

    if (object_p == NULL)
    {
      return ecma_make_object_value (ecma_op_new_array_object (length));
    }

    JERRY_ASSERT (ecma_op_object_is_fast_array (object_p));

    return ecma_make_object_value (object_p);
  }
  else
  {
    length = arguments_list_len;
    array_items_p = arguments_list_p;
    array_items_count = arguments_list_len;
  }

  ecma_object_t *object_p = ecma_op_new_fast_array_object (length);

  /* At this point we were not able to allocate a length * sizeof (ecma_value_t) amount of memory,
     so we can terminate the engine since converting it into normal property list based array
     would consume more memory. */
  if (object_p == NULL)
  {
    jerry_fatal (ERR_OUT_OF_MEMORY);
  }

  if (length == 0)
  {
    return ecma_make_object_value (object_p);
  }

  ecma_value_t *values_p = ECMA_GET_NON_NULL_POINTER (ecma_value_t, object_p->u1.property_list_cp);
  ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) object_p;

  for (uint32_t index = 0;
       index < array_items_count;
       index++)
  {
    JERRY_ASSERT (!ecma_is_value_array_hole (array_items_p[index]));
    values_p[index] = ecma_copy_value_if_not_object (array_items_p[index]);
  }

  ext_obj_p->u.array.u.hole_count -= ECMA_FAST_ARRAY_HOLE_ONE * array_items_count;

  return ecma_make_object_value (object_p);
} /* ecma_op_create_array_object */

#if ENABLED (JERRY_ES2015)
/**
 * Array object creation with custom prototype.
 *
 * See also: ECMA-262 v6, 9.4.2.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_create_array_object_by_constructor (const ecma_value_t *arguments_list_p, /**< list of arguments that
                                                                                   *   are passed to
                                                                                   *   Array constructor */
                                            ecma_length_t arguments_list_len, /**< length of the arguments' list */
                                            bool is_treat_single_arg_as_length, /**< if the value is true,
                                                                                 *   arguments_list_len is 1
                                                                                 *   and single argument is Number,
                                                                                 *   then treat the single argument
                                                                                 *   as new Array's length rather
                                                                                 *   than as single item of the
                                                                                 *   Array */
                                            ecma_object_t *object_p) /**< The object from whom the new array object
                                                                      *   is being created */
{
  /* TODO: Use @@species after Symbol has been implemented */
  JERRY_UNUSED (object_p);

  return ecma_op_create_array_object (arguments_list_p,
                                      arguments_list_len,
                                      is_treat_single_arg_as_length);
} /* ecma_op_create_array_object_by_constructor */
#endif /* ENABLED (JERRY_ES2015) */

/**
 * Low level delete of array items from new_length to old_length
 *
 * Note: new_length must be less than old_length
 *
 * @return the updated value of new_length
 */
static uint32_t
ecma_delete_array_properties (ecma_object_t *object_p, /**< object */
                              uint32_t new_length, /**< new length */
                              uint32_t old_length) /**< old length */
{
  JERRY_ASSERT (new_length < old_length);
  JERRY_ASSERT (ecma_get_object_type (object_p) == ECMA_OBJECT_TYPE_ARRAY);

  if (ecma_op_object_is_fast_array (object_p))
  {
    return ecma_delete_fast_array_properties (object_p, new_length);
  }

  /* First the minimum value of new_length is updated. */
  jmem_cpointer_t current_prop_cp = object_p->u1.property_list_cp;

  if (current_prop_cp == JMEM_CP_NULL)
  {
    return new_length;
  }

  ecma_property_header_t *current_prop_p;

#if ENABLED (JERRY_PROPRETY_HASHMAP)
  current_prop_p = ECMA_GET_NON_NULL_POINTER (ecma_property_header_t, current_prop_cp);

  if (current_prop_p->types[0] == ECMA_PROPERTY_TYPE_HASHMAP)
  {
    current_prop_cp = current_prop_p->next_property_cp;
  }
#endif /* ENABLED (JERRY_PROPRETY_HASHMAP) */

  while (current_prop_cp != JMEM_CP_NULL)
  {
    current_prop_p = ECMA_GET_NON_NULL_POINTER (ecma_property_header_t, current_prop_cp);
    JERRY_ASSERT (ECMA_PROPERTY_IS_PROPERTY_PAIR (current_prop_p));

    ecma_property_pair_t *prop_pair_p = (ecma_property_pair_t *) current_prop_p;

    for (int i = 0; i < ECMA_PROPERTY_PAIR_ITEM_COUNT; i++)
    {
      if (ECMA_PROPERTY_IS_NAMED_PROPERTY (current_prop_p->types[i])
          && !ecma_is_property_configurable (current_prop_p->types[i]))
      {
        uint32_t index = ecma_string_get_property_index (current_prop_p->types[i],
                                                         prop_pair_p->names_cp[i]);

        if (index < old_length && index >= new_length)
        {
          JERRY_ASSERT (index != ECMA_STRING_NOT_ARRAY_INDEX);

          new_length = index + 1;

          if (new_length == old_length)
          {
            /* Early return. */
            return new_length;
          }
        }
      }
    }

    current_prop_cp = current_prop_p->next_property_cp;
  }

  /* Second all properties between new_length and old_length are deleted. */
  current_prop_cp = object_p->u1.property_list_cp;
  ecma_property_header_t *prev_prop_p = NULL;

#if ENABLED (JERRY_PROPRETY_HASHMAP)
  JERRY_ASSERT (current_prop_cp != JMEM_CP_NULL);

  ecma_property_hashmap_delete_status hashmap_status = ECMA_PROPERTY_HASHMAP_DELETE_NO_HASHMAP;
  current_prop_p = ECMA_GET_NON_NULL_POINTER (ecma_property_header_t, current_prop_cp);

  if (current_prop_p->types[0] == ECMA_PROPERTY_TYPE_HASHMAP)
  {
    prev_prop_p = current_prop_p;
    current_prop_cp = current_prop_p->next_property_cp;
    hashmap_status = ECMA_PROPERTY_HASHMAP_DELETE_HAS_HASHMAP;
  }
#endif /* ENABLED (JERRY_PROPRETY_HASHMAP) */

  while (current_prop_cp != JMEM_CP_NULL)
  {
    current_prop_p = ECMA_GET_NON_NULL_POINTER (ecma_property_header_t, current_prop_cp);

    JERRY_ASSERT (ECMA_PROPERTY_IS_PROPERTY_PAIR (current_prop_p));
    ecma_property_pair_t *prop_pair_p = (ecma_property_pair_t *) current_prop_p;

    for (int i = 0; i < ECMA_PROPERTY_PAIR_ITEM_COUNT; i++)
    {
      if (ECMA_PROPERTY_IS_NAMED_PROPERTY (current_prop_p->types[i])
          && ecma_is_property_configurable (current_prop_p->types[i]))
      {
        uint32_t index = ecma_string_get_property_index (current_prop_p->types[i],
                                                         prop_pair_p->names_cp[i]);

        if (index < old_length && index >= new_length)
        {
          JERRY_ASSERT (index != ECMA_STRING_NOT_ARRAY_INDEX);

#if ENABLED (JERRY_PROPRETY_HASHMAP)
          if (hashmap_status == ECMA_PROPERTY_HASHMAP_DELETE_HAS_HASHMAP)
          {
            hashmap_status = ecma_property_hashmap_delete (object_p,
                                                           prop_pair_p->names_cp[i],
                                                           current_prop_p->types + i);
          }
#endif /* ENABLED (JERRY_PROPRETY_HASHMAP) */

          ecma_free_property (object_p, prop_pair_p->names_cp[i], current_prop_p->types + i);
          current_prop_p->types[i] = ECMA_PROPERTY_TYPE_DELETED;
          prop_pair_p->names_cp[i] = LIT_INTERNAL_MAGIC_STRING_DELETED;
        }
      }
    }

    if (current_prop_p->types[0] == ECMA_PROPERTY_TYPE_DELETED
        && current_prop_p->types[1] == ECMA_PROPERTY_TYPE_DELETED)
    {
      if (prev_prop_p == NULL)
      {
        object_p->u1.property_list_cp = current_prop_p->next_property_cp;
      }
      else
      {
        prev_prop_p->next_property_cp = current_prop_p->next_property_cp;
      }

      jmem_cpointer_t next_prop_cp = current_prop_p->next_property_cp;
      ecma_dealloc_property_pair ((ecma_property_pair_t *) current_prop_p);
      current_prop_cp = next_prop_cp;
    }
    else
    {
      prev_prop_p = current_prop_p;
      current_prop_cp = current_prop_p->next_property_cp;
    }
  }

#if ENABLED (JERRY_PROPRETY_HASHMAP)
  if (hashmap_status == ECMA_PROPERTY_HASHMAP_DELETE_RECREATE_HASHMAP)
  {
    ecma_property_hashmap_free (object_p);
    ecma_property_hashmap_create (object_p);
  }
#endif /* ENABLED (JERRY_PROPRETY_HASHMAP) */

  return new_length;
} /* ecma_delete_array_properties */

/**
 * Update the length of an array to a new length
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_array_object_set_length (ecma_object_t *object_p, /**< the array object */
                                 ecma_value_t new_value, /**< new length value */
                                 uint32_t flags) /**< configuration options */
{
  bool is_throw = (flags & ECMA_ARRAY_OBJECT_SET_LENGTH_FLAG_IS_THROW);

  ecma_value_t completion = ecma_op_to_number (new_value);

  if (ECMA_IS_VALUE_ERROR (completion))
  {
    return completion;
  }

  JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (completion)
                && ecma_is_value_number (completion));

  ecma_number_t new_len_num = ecma_get_number_from_value (completion);

  ecma_free_value (completion);

  if (ecma_is_value_object (new_value))
  {
    ecma_value_t compared_num_val = ecma_op_to_number (new_value);

    if (ECMA_IS_VALUE_ERROR (compared_num_val))
    {
      return compared_num_val;
    }

    new_len_num = ecma_get_number_from_value (compared_num_val);
    ecma_free_value (compared_num_val);
  }

  uint32_t new_len_uint32 = ecma_number_to_uint32 (new_len_num);

  if (((ecma_number_t) new_len_uint32) != new_len_num)
  {
    return ecma_raise_range_error (ECMA_ERR_MSG ("Invalid array length."));
  }

  if (flags & ECMA_ARRAY_OBJECT_SET_LENGTH_FLAG_REJECT)
  {
    return ecma_reject (is_throw);
  }

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

  uint32_t old_len_uint32 = ext_object_p->u.array.length;

  if (new_len_num == old_len_uint32)
  {
    /* Only the writable flag must be updated. */
    if (flags & ECMA_ARRAY_OBJECT_SET_LENGTH_FLAG_WRITABLE_DEFINED)
    {
      if (!(flags & ECMA_ARRAY_OBJECT_SET_LENGTH_FLAG_WRITABLE))
      {
        uint8_t new_prop_value = (uint8_t) (ext_object_p->u.array.u.length_prop & ~ECMA_PROPERTY_FLAG_WRITABLE);
        ext_object_p->u.array.u.length_prop = new_prop_value;
      }
      else if (!ecma_is_property_writable (ext_object_p->u.array.u.length_prop))
      {
        return ecma_reject (is_throw);
      }
    }
    return ECMA_VALUE_TRUE;
  }
  else if (!ecma_is_property_writable (ext_object_p->u.array.u.length_prop))
  {
    return ecma_reject (is_throw);
  }

  uint32_t current_len_uint32 = new_len_uint32;

  if (new_len_uint32 < old_len_uint32)
  {
    current_len_uint32 = ecma_delete_array_properties (object_p, new_len_uint32, old_len_uint32);
  }
  else if (ecma_op_object_is_fast_array (object_p))
  {
    ecma_fast_array_set_length (object_p, new_len_uint32);
  }

  ext_object_p->u.array.length = current_len_uint32;

  if ((flags & ECMA_ARRAY_OBJECT_SET_LENGTH_FLAG_WRITABLE_DEFINED)
      && !(flags & ECMA_ARRAY_OBJECT_SET_LENGTH_FLAG_WRITABLE))
  {
    uint8_t new_prop_value = (uint8_t) (ext_object_p->u.array.u.length_prop & ~ECMA_PROPERTY_FLAG_WRITABLE);
    ext_object_p->u.array.u.length_prop = new_prop_value;
  }

  if (current_len_uint32 == new_len_uint32)
  {
    return ECMA_VALUE_TRUE;
  }
  return ecma_reject (is_throw);
} /* ecma_op_array_object_set_length */

/**
 * Property descriptor bitset for fast array data properties.
 * If the property desciptor fields contains all the flags below
 * attempt to stay fast access array during [[DefineOwnProperty]] operation.
 */
#define ECMA_FAST_ARRAY_DATA_PROP_FLAGS (ECMA_PROP_IS_VALUE_DEFINED \
                                         | ECMA_PROP_IS_ENUMERABLE_DEFINED \
                                         | ECMA_PROP_IS_ENUMERABLE \
                                         | ECMA_PROP_IS_CONFIGURABLE_DEFINED \
                                         | ECMA_PROP_IS_CONFIGURABLE \
                                         | ECMA_PROP_IS_WRITABLE_DEFINED \
                                         | ECMA_PROP_IS_WRITABLE)

/**
 * [[DefineOwnProperty]] ecma array object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *          ECMA-262 v5, 15.4.5.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_array_object_define_own_property (ecma_object_t *object_p, /**< the array object */
                                          ecma_string_t *property_name_p, /**< property name */
                                          const ecma_property_descriptor_t *property_desc_p) /**< property descriptor */
{
  if (ecma_string_is_length (property_name_p))
  {
    JERRY_ASSERT ((property_desc_p->flags & ECMA_PROP_IS_CONFIGURABLE_DEFINED)
                  || !(property_desc_p->flags & ECMA_PROP_IS_CONFIGURABLE));
    JERRY_ASSERT ((property_desc_p->flags & ECMA_PROP_IS_ENUMERABLE_DEFINED)
                  || !(property_desc_p->flags & ECMA_PROP_IS_ENUMERABLE));
    JERRY_ASSERT ((property_desc_p->flags & ECMA_PROP_IS_WRITABLE_DEFINED)
                  || !(property_desc_p->flags & ECMA_PROP_IS_WRITABLE));

    uint32_t flags = 0;

    if (property_desc_p->flags & ECMA_PROP_IS_THROW)
    {
      flags |= ECMA_ARRAY_OBJECT_SET_LENGTH_FLAG_IS_THROW;
    }

    /* Only the writable and data properties can be modified. */
    if (property_desc_p->flags & (ECMA_PROP_IS_CONFIGURABLE
                                  | ECMA_PROP_IS_ENUMERABLE
                                  | ECMA_PROP_IS_GET_DEFINED
                                  | ECMA_PROP_IS_SET_DEFINED))
    {
      flags |= ECMA_ARRAY_OBJECT_SET_LENGTH_FLAG_REJECT;
    }

    if (property_desc_p->flags & ECMA_PROP_IS_WRITABLE_DEFINED)
    {
      flags |= ECMA_ARRAY_OBJECT_SET_LENGTH_FLAG_WRITABLE_DEFINED;
    }

    if (property_desc_p->flags & ECMA_PROP_IS_WRITABLE)
    {
      flags |= ECMA_ARRAY_OBJECT_SET_LENGTH_FLAG_WRITABLE;
    }

    if (property_desc_p->flags & ECMA_PROP_IS_VALUE_DEFINED)
    {
      return ecma_op_array_object_set_length (object_p, property_desc_p->value, flags);
    }

    ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
    ecma_value_t length_value = ecma_make_uint32_value (ext_object_p->u.array.length);

    ecma_value_t result = ecma_op_array_object_set_length (object_p, length_value, flags);

    ecma_fast_free_value (length_value);
    return result;
  }

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

  if (ecma_op_object_is_fast_array (object_p))
  {
    if ((property_desc_p->flags & ECMA_FAST_ARRAY_DATA_PROP_FLAGS) == ECMA_FAST_ARRAY_DATA_PROP_FLAGS)
    {
      uint32_t index = ecma_string_get_array_index (property_name_p);

      if (JERRY_UNLIKELY (index == ECMA_STRING_NOT_ARRAY_INDEX))
      {
        ecma_fast_array_convert_to_normal (object_p);
      }
      else if (ecma_fast_array_set_property (object_p, index, property_desc_p->value))
      {
        return ECMA_VALUE_TRUE;
      }

      JERRY_ASSERT (!ecma_op_array_is_fast_array (ext_object_p));
    }
    else
    {
      ecma_fast_array_convert_to_normal (object_p);
    }
  }

  JERRY_ASSERT (!ecma_op_object_is_fast_array (object_p));
  uint32_t index = ecma_string_get_array_index (property_name_p);

  if (index == ECMA_STRING_NOT_ARRAY_INDEX)
  {
    return ecma_op_general_object_define_own_property (object_p, property_name_p, property_desc_p);
  }

  bool update_length = (index >= ext_object_p->u.array.length);

  if (update_length && !ecma_is_property_writable (ext_object_p->u.array.u.length_prop))
  {
    return ecma_reject (property_desc_p->flags & ECMA_PROP_IS_THROW);
  }

  ecma_property_descriptor_t prop_desc;

  prop_desc = *property_desc_p;
  prop_desc.flags &= (uint16_t) ~ECMA_PROP_IS_THROW;

  ecma_value_t completition = ecma_op_general_object_define_own_property (object_p,
                                                                          property_name_p,
                                                                          &prop_desc);
  JERRY_ASSERT (ecma_is_value_boolean (completition));

  if (ecma_is_value_false (completition))
  {
    return ecma_reject (property_desc_p->flags & ECMA_PROP_IS_THROW);
  }

  if (update_length)
  {
    ext_object_p->u.array.length = index + 1;
  }

  return ECMA_VALUE_TRUE;
} /* ecma_op_array_object_define_own_property */

/**
 * Get the length of the an array object
 *
 * @return the array length
 */
extern inline uint32_t JERRY_ATTR_ALWAYS_INLINE
ecma_array_get_length (ecma_object_t *array_p) /**< array object */
{
  JERRY_ASSERT (ecma_get_object_type (array_p) == ECMA_OBJECT_TYPE_ARRAY);

  return ((ecma_extended_object_t *) array_p)->u.array.length;
} /* ecma_array_get_length */

/**
 * List names of a String object's lazy instantiated properties
 *
 * @return string values collection
 */
void
ecma_op_array_list_lazy_property_names (ecma_object_t *obj_p, /**< a String object */
                                        bool separate_enumerable, /**< true -  list enumerable properties
                                                                   *           into main collection,
                                                                   *           and non-enumerable to collection of
                                                                   *           'skipped non-enumerable' properties,
                                                                   *   false - list all properties into main
                                                                   *           collection.
                                                                   */
                                        ecma_collection_t *main_collection_p, /**< 'main'  collection */
                                        ecma_collection_t *non_enum_collection_p) /**< skipped
                                                                                   *   'non-enumerable'
                                                                                   *   collection */
{
  JERRY_ASSERT (ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_ARRAY);

  ecma_collection_t *for_non_enumerable_p = separate_enumerable ? non_enum_collection_p : main_collection_p;

  ecma_collection_push_back (for_non_enumerable_p, ecma_make_magic_string_value (LIT_MAGIC_STRING_LENGTH));
} /* ecma_op_array_list_lazy_property_names */

/**
 * @}
 * @}
 */
