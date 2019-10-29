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

#include <math.h>

#include "ecma-typedarray-object.h"
#include "ecma-arraybuffer-object.h"
#include "ecma-function-object.h"
#include "ecma-builtin-helpers.h"
#include "ecma-try-catch-macro.h"
#include "ecma-objects.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "jcontext.h"

#if ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY)

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmatypedarrayobject ECMA TypedArray object related routines
 * @{
 */

/**
 * Read an int8_t value from the given arraybuffer
 */
static ecma_number_t
ecma_typedarray_get_int8_element (lit_utf8_byte_t *src) /**< the location in the internal arraybuffer */
{
  int8_t num = (int8_t) *src;
  return (ecma_number_t) num;
} /* ecma_typedarray_get_int8_element */

/**
 * Read an uint8_t value from the given arraybuffer
 */
static ecma_number_t
ecma_typedarray_get_uint8_element (lit_utf8_byte_t *src) /**< the location in the internal arraybuffer */
{
  uint8_t num = (uint8_t) *src;
  return (ecma_number_t) num;
} /* ecma_typedarray_get_uint8_element */

/**
 * Read an int16_t value from the given arraybuffer
 */
static ecma_number_t
ecma_typedarray_get_int16_element (lit_utf8_byte_t *src) /**< the location in the internal arraybuffer */
{
  int16_t num;
  memcpy (&num, src, sizeof (int16_t));
  return (ecma_number_t) num;
} /* ecma_typedarray_get_int16_element */

/**
 * Read an uint16_t value from the given arraybuffer
 */
static ecma_number_t
ecma_typedarray_get_uint16_element (lit_utf8_byte_t *src) /**< the location in the internal arraybuffer */
{
  uint16_t num;
  memcpy (&num, src, sizeof (uint16_t));
  return (ecma_number_t) num;
} /* ecma_typedarray_get_uint16_element */

/**
 * Read an int32_t value from the given arraybuffer
 */
static ecma_number_t
ecma_typedarray_get_int32_element (lit_utf8_byte_t *src) /**< the location in the internal arraybuffer */
{
  int32_t num;
  memcpy (&num, src, sizeof (int32_t));
  return (ecma_number_t) num;
} /* ecma_typedarray_get_int32_element */

/**
 * Read an uint32_t value from the given arraybuffer
 */
static ecma_number_t
ecma_typedarray_get_uint32_element (lit_utf8_byte_t *src) /**< the location in the internal arraybuffer */
{
  uint32_t num;
  memcpy (&num, src, sizeof (uint32_t));
  return (ecma_number_t) num;
} /* ecma_typedarray_get_uint32_element */

/**
 * Read a float value from the given arraybuffer
 */
static ecma_number_t
ecma_typedarray_get_float_element (lit_utf8_byte_t *src) /**< the location in the internal arraybuffer */
{
  float num;
  memcpy (&num, src, sizeof (float));
  return (ecma_number_t) num;
} /* ecma_typedarray_get_float_element */

/**
 * Read a double value from the given arraybuffer
 */
static ecma_number_t
ecma_typedarray_get_double_element (lit_utf8_byte_t *src) /**< the location in the internal arraybuffer */
{
  double num;
  memcpy (&num, src, sizeof (double));
  return (ecma_number_t) num;
} /* ecma_typedarray_get_double_element */


/**
 * Normalize the given ecma_number_t to an uint32_t value
 */
static uint32_t
ecma_typedarray_setter_number_to_uint32 (ecma_number_t value) /**< the number value to normalize */
{
  uint32_t uint32_value = 0;

  if (!ecma_number_is_nan (value) && !ecma_number_is_infinity (value))
  {
    bool is_negative = false;

    if (value < 0)
    {
      is_negative = true;
      value = -value;
    }

    if (value > ((ecma_number_t) 0xffffffff))
    {
      value = (ecma_number_t) (fmod (value, (ecma_number_t) 0x100000000));
    }

    uint32_value = (uint32_t) value;

    if (is_negative)
    {
      uint32_value = (uint32_t) (-(int32_t) uint32_value);
    }
  }

  return uint32_value;
} /* ecma_typedarray_setter_number_to_uint32 */

/**
 * Write an int8_t value into the given arraybuffer
 */
static void
ecma_typedarray_set_int8_element (lit_utf8_byte_t *dst_p, /**< the location in the internal arraybuffer */
                                  ecma_number_t value) /**< the number value to set */
{
  int8_t num = (int8_t) ecma_typedarray_setter_number_to_uint32 (value);
  *dst_p = (lit_utf8_byte_t) num;
} /* ecma_typedarray_set_int8_element */


/**
 * Write an uint8_t value into the given arraybuffer
 */
static void
ecma_typedarray_set_uint8_element (lit_utf8_byte_t *dst_p, /**< the location in the internal arraybuffer */
                                   ecma_number_t value) /**< the number value to set */
{
  uint8_t num = (uint8_t) ecma_typedarray_setter_number_to_uint32 (value);
  *dst_p = (lit_utf8_byte_t) num;
} /* ecma_typedarray_set_uint8_element */

/**
 * Write an uint8_t clamped value into the given arraybuffer
 */
static void
ecma_typedarray_set_uint8_clamped_element (lit_utf8_byte_t *dst_p, /**< the location in the internal arraybuffer */
                                           ecma_number_t value) /**< the number value to set */
{
  uint8_t clamped;

  if (value > 255)
  {
    clamped = 255;
  }
  else if (value <= 0)
  {
    clamped = 0;
  }
  else
  {
    clamped = (uint8_t) value;

    if (clamped + 0.5 < value
        || (clamped + 0.5 == value && (clamped % 2) == 1))
    {
      clamped ++;
    }
  }

  *dst_p = (lit_utf8_byte_t) clamped;
} /* ecma_typedarray_set_uint8_clamped_element */

/**
 * Write an int16_t value into the given arraybuffer
 */
static void
ecma_typedarray_set_int16_element (lit_utf8_byte_t *dst_p, /**< the location in the internal arraybuffer */
                                   ecma_number_t value) /**< the number value to set */
{
  int16_t num = (int16_t) ecma_typedarray_setter_number_to_uint32 (value);
  memcpy (dst_p, &num, sizeof (int16_t));
} /* ecma_typedarray_set_int16_element */


/**
 * Write an uint8_t value into the given arraybuffer
 */
static void
ecma_typedarray_set_uint16_element (lit_utf8_byte_t *dst_p, /**< the location in the internal arraybuffer */
                                    ecma_number_t value) /**< the number value to set */
{
  uint16_t num = (uint16_t) ecma_typedarray_setter_number_to_uint32 (value);
  memcpy (dst_p, &num, sizeof (uint16_t));
} /* ecma_typedarray_set_uint16_element */

/**
 * Write an int32_t value into the given arraybuffer
 */
static void
ecma_typedarray_set_int32_element (lit_utf8_byte_t *dst_p, /**< the location in the internal arraybuffer */
                                   ecma_number_t value) /**< the number value to set */
{
  int32_t num = (int32_t) ecma_typedarray_setter_number_to_uint32 (value);
  memcpy (dst_p, &num, sizeof (int32_t));
} /* ecma_typedarray_set_int32_element */


/**
 * Write an uint32_t value into the given arraybuffer
 */
static void
ecma_typedarray_set_uint32_element (lit_utf8_byte_t *dst_p, /**< the location in the internal arraybuffer */
                                    ecma_number_t value) /**< the number value to set */
{
  uint32_t num = (uint32_t) ecma_typedarray_setter_number_to_uint32 (value);
  memcpy (dst_p, &num, sizeof (uint32_t));
} /* ecma_typedarray_set_uint32_element */


/**
 * Write a float value into the given arraybuffer
 */
static void
ecma_typedarray_set_float_element (lit_utf8_byte_t *dst_p, /**< the location in the internal arraybuffer */
                                   ecma_number_t value) /**< the number value to set */
{
  float num = (float) value;
  memcpy (dst_p, &num, sizeof (float));
} /* ecma_typedarray_set_float_element */

#if ENABLED (JERRY_NUMBER_TYPE_FLOAT64)
/**
 * Write a double value into the given arraybuffer
 */
static void
ecma_typedarray_set_double_element (lit_utf8_byte_t *dst_p, /**< the location in the internal arraybuffer */
                                    ecma_number_t value) /**< the number value to set */
{
  double num = (double) value;
  memcpy (dst_p, &num, sizeof (double));
} /* ecma_typedarray_set_double_element */
#endif /* ENABLED (JERRY_NUMBER_TYPE_FLOAT64) */


/**
 * Builtin id of the first %TypedArray% builtin routine intrinsic object
 */
#define ECMA_FIRST_TYPEDARRAY_BUILTIN_ROUTINE_ID ECMA_BUILTIN_ID_INT8ARRAY

#if ENABLED (JERRY_NUMBER_TYPE_FLOAT64)
/**
 * Builtin id of the last %TypedArray% builtin routine intrinsic object
 */
#define ECMA_LAST_TYPEDARRAY_BUILTIN_ROUTINE_ID ECMA_BUILTIN_ID_FLOAT64ARRAY
#else /* !ENABLED (JERRY_NUMBER_TYPE_FLOAT64) */
/**
 * Builtin id of the last %TypedArray% builtin routine intrinsic object
 */
#define ECMA_LAST_TYPEDARRAY_BUILTIN_ROUTINE_ID ECMA_BUILTIN_ID_FLOAT32ARRAY
#endif /* ENABLED (JERRY_NUMBER_TYPE_FLOAT64) */

/**
 * Builtin id of the first %TypedArray% builtin prototype intrinsic object
 */
#define ECMA_FIRST_TYPEDARRAY_BUILTIN_PROTOTYPE_ID ECMA_BUILTIN_ID_INT8ARRAY_PROTOTYPE

/**
 * List of typedarray getters based on their builtin id
 */
static const ecma_typedarray_getter_fn_t ecma_typedarray_getters[] =
{
  ecma_typedarray_get_int8_element,   /**< Int8Array */
  ecma_typedarray_get_uint8_element,  /**< Uint8Array */
  ecma_typedarray_get_uint8_element,  /**< Uint8ClampedArray */
  ecma_typedarray_get_int16_element,  /**< Int16Array */
  ecma_typedarray_get_uint16_element, /**< Int32Array */
  ecma_typedarray_get_int32_element,  /**< Uint32Array */
  ecma_typedarray_get_uint32_element, /**< Uint32Array */
  ecma_typedarray_get_float_element,  /**< Float32Array */
#if ENABLED (JERRY_NUMBER_TYPE_FLOAT64)
  ecma_typedarray_get_double_element, /**< Float64Array */
#endif /* ENABLED (JERRY_NUMBER_TYPE_FLOAT64) */
};

/**
 * List of typedarray setters based on their builtin id
 */
static const ecma_typedarray_setter_fn_t ecma_typedarray_setters[] =
{
  ecma_typedarray_set_int8_element,          /**< Int8Array */
  ecma_typedarray_set_uint8_element,         /**< Uint8Array */
  ecma_typedarray_set_uint8_clamped_element, /**< Uint8ClampedArray */
  ecma_typedarray_set_int16_element,         /**< Int16Array */
  ecma_typedarray_set_uint16_element,        /**< Int32Array */
  ecma_typedarray_set_int32_element,         /**< Uint32Array */
  ecma_typedarray_set_uint32_element,        /**< Uint32Array */
  ecma_typedarray_set_float_element,         /**< Float32Array */
#if ENABLED (JERRY_NUMBER_TYPE_FLOAT64)
  ecma_typedarray_set_double_element,        /**< Float64Array */
#endif /* ENABLED (JERRY_NUMBER_TYPE_FLOAT64) */
};

/**
 * List of typedarray element shift sizes based on their builtin id
 */
static const uint8_t ecma_typedarray_element_shift_sizes[] =
{
  0, /**< Int8Array */
  0, /**< Uint8Array */
  0, /**< Uint8ClampedArray */
  1, /**< Int16Array */
  1, /**< Uint16Array */
  2, /**< Int32Array */
  2, /**< Uint32Array */
  2, /**< Float32Array */
#if ENABLED (JERRY_NUMBER_TYPE_FLOAT64)
  3, /**< Float64Array */
#endif /* ENABLED (JERRY_NUMBER_TYPE_FLOAT64) */
};

/**
 * List of typedarray class magic strings based on their builtin id
 */
static const uint16_t ecma_typedarray_magic_string_list[] =
{
  (uint16_t) LIT_MAGIC_STRING_INT8_ARRAY_UL,          /**< Int8Array */
  (uint16_t) LIT_MAGIC_STRING_UINT8_ARRAY_UL,         /**< Uint8Array */
  (uint16_t) LIT_MAGIC_STRING_UINT8_CLAMPED_ARRAY_UL, /**< Uint8ClampedArray */
  (uint16_t) LIT_MAGIC_STRING_INT16_ARRAY_UL,         /**< Int16Array */
  (uint16_t) LIT_MAGIC_STRING_UINT16_ARRAY_UL,        /**< Uint16Array */
  (uint16_t) LIT_MAGIC_STRING_INT32_ARRAY_UL,         /**< Int32Array */
  (uint16_t) LIT_MAGIC_STRING_UINT32_ARRAY_UL,        /**< Uint32Array */
  (uint16_t) LIT_MAGIC_STRING_FLOAT32_ARRAY_UL,       /**< Float32Array */
#if ENABLED (JERRY_NUMBER_TYPE_FLOAT64)
  (uint16_t) LIT_MAGIC_STRING_FLOAT64_ARRAY_UL,       /**< Float64Array */
#endif /* ENABLED (JERRY_NUMBER_TYPE_FLOAT64) */
};

/**
 * Get typedarray's getter function callback
 *
 * @return ecma_typedarray_getter_fn_t: the getter function for the given builtin TypedArray id
 */
inline ecma_typedarray_getter_fn_t JERRY_ATTR_ALWAYS_INLINE
ecma_get_typedarray_getter_fn (ecma_typedarray_type_t typedarray_id)
{
  return ecma_typedarray_getters[typedarray_id];
} /* ecma_get_typedarray_getter_fn */

/**
 * get typedarray's element value
 *
 * @return ecma_number_t: the value of the element
 */
inline ecma_number_t JERRY_ATTR_ALWAYS_INLINE
ecma_get_typedarray_element (lit_utf8_byte_t *src_p,
                             ecma_typedarray_type_t typedarray_id)
{
  return ecma_typedarray_getters[typedarray_id](src_p);
} /* ecma_get_typedarray_element */

/**
 * Get typedarray's setter function callback
 *
 * @return ecma_typedarray_setter_fn_t: the setter function for the given builtin TypedArray id
 */
inline ecma_typedarray_setter_fn_t JERRY_ATTR_ALWAYS_INLINE
ecma_get_typedarray_setter_fn (ecma_typedarray_type_t typedarray_id)
{
  return ecma_typedarray_setters[typedarray_id];
} /* ecma_get_typedarray_setter_fn */

/**
 * set typedarray's element value
 */
inline void JERRY_ATTR_ALWAYS_INLINE
ecma_set_typedarray_element (lit_utf8_byte_t *dst_p,
                             ecma_number_t value,
                             ecma_typedarray_type_t typedarray_id)
{
  ecma_typedarray_setters[typedarray_id](dst_p, value);
} /* ecma_set_typedarray_element */

/**
 * Get the element shift size of a TypedArray type.
 *
 * @return uint8_t
 */
inline uint8_t JERRY_ATTR_ALWAYS_INLINE
ecma_typedarray_helper_get_shift_size (ecma_typedarray_type_t typedarray_id)
{
  return ecma_typedarray_element_shift_sizes[typedarray_id];
} /* ecma_typedarray_helper_get_shift_size */

/**
 * Check if the builtin is a TypedArray type.
 *
 * @return bool: - true if based on the given id it is a TypedArray
 *               - false if based on the given id it is not a TypedArray
 */
bool
ecma_typedarray_helper_is_typedarray (ecma_builtin_id_t builtin_id) /**< the builtin id of a type **/
{
  return ((builtin_id >= ECMA_FIRST_TYPEDARRAY_BUILTIN_ROUTINE_ID)
          && (builtin_id <= ECMA_LAST_TYPEDARRAY_BUILTIN_ROUTINE_ID));
} /* ecma_typedarray_helper_is_typedarray */

/**
 * Get the prototype ID of a TypedArray type.
 *
 * @return ecma_builtin_id_t
 */
ecma_builtin_id_t
ecma_typedarray_helper_get_prototype_id (ecma_typedarray_type_t typedarray_id) /**< the id of the typedarray **/
{
  return (ecma_builtin_id_t) (ECMA_FIRST_TYPEDARRAY_BUILTIN_PROTOTYPE_ID + typedarray_id);
} /* ecma_typedarray_helper_get_prototype_id */

/**
 * Get the built-in TypedArray type of the given object.
 *
 * @return ecma_typedarray_type_t
 */
ecma_typedarray_type_t
ecma_get_typedarray_id (ecma_object_t *obj_p) /**< typedarray object **/
{
  JERRY_ASSERT (ecma_object_is_typedarray (obj_p));

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) obj_p;

  return (ecma_typedarray_type_t) ext_object_p->u.pseudo_array.extra_info;
} /* ecma_get_typedarray_id */

/**
 * Get the built-in TypedArray type of the given object.
 *
 * @return ecma_typedarray_type_t
 */
ecma_typedarray_type_t
ecma_typedarray_helper_builtin_to_typedarray_id (ecma_builtin_id_t builtin_id)
{
  JERRY_ASSERT (ecma_typedarray_helper_is_typedarray (builtin_id));

  return (ecma_typedarray_type_t) (builtin_id - ECMA_FIRST_TYPEDARRAY_BUILTIN_ROUTINE_ID);
} /* ecma_typedarray_helper_builtin_to_typedarray_id */

/**
 * Create a TypedArray object by given array_length
 *
 * See also: ES2015 22.2.1.2.1
 *
 * @return ecma value of the new typedarray object
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_typedarray_create_object_with_length (ecma_length_t array_length, /**< length of the typedarray */
                                           ecma_object_t *proto_p, /**< prototype object */
                                           uint8_t element_size_shift, /**< the size shift of the element length */
                                           ecma_typedarray_type_t typedarray_id) /**< id of the typedarray */
{
  if (array_length > (UINT32_MAX >> element_size_shift))
  {
    return ecma_raise_range_error (ECMA_ERR_MSG ("Maximum typedarray size is reached."));
  }

  ecma_length_t byte_length = array_length << element_size_shift;

  if (byte_length > UINT32_MAX - sizeof (ecma_extended_object_t) - JMEM_ALIGNMENT + 1)
  {
    return ecma_raise_range_error (ECMA_ERR_MSG ("Maximum typedarray size is reached."));
  }

  ecma_object_t *new_arraybuffer_p = ecma_arraybuffer_new_object (byte_length);
  ecma_object_t *object_p = ecma_create_object (proto_p,
                                                sizeof (ecma_extended_object_t),
                                                ECMA_OBJECT_TYPE_PSEUDO_ARRAY);

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
  ext_object_p->u.pseudo_array.u1.class_id = ecma_typedarray_magic_string_list[typedarray_id];
  ext_object_p->u.pseudo_array.type = ECMA_PSEUDO_ARRAY_TYPEDARRAY;
  ext_object_p->u.pseudo_array.extra_info = (uint8_t) typedarray_id;
  ext_object_p->u.pseudo_array.u2.arraybuffer = ecma_make_object_value (new_arraybuffer_p);

  ecma_deref_object (new_arraybuffer_p);

  return ecma_make_object_value (object_p);
} /* ecma_typedarray_create_object_with_length */

/**
 * Create a TypedArray object by given buffer, offset, and array_length
 *
 * See also: ES2015 22.2.1.5
 *
 * @return ecma value of the new typedarray object
 *         Returned value must be freed with ecma_free_value
 */
static ecma_value_t
ecma_typedarray_create_object_with_buffer (ecma_object_t *arraybuffer_p, /**< the arraybuffer inside */
                                           ecma_length_t byte_offset, /**< the byte offset of the arraybuffer */
                                           ecma_length_t array_length, /**< length of the typedarray */
                                           ecma_object_t *proto_p, /**< prototype object */
                                           uint8_t element_size_shift, /**< the size shift of the element length */
                                           ecma_typedarray_type_t typedarray_id) /**< id of the typedarray */
{
  if (ecma_arraybuffer_is_detached (arraybuffer_p))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("ArrayBuffer has been detached."));
  }
  ecma_length_t expected_length = (ecma_arraybuffer_get_length (arraybuffer_p) >> element_size_shift);

  bool needs_ext_typedarray_obj = (byte_offset != 0 || array_length != expected_length);

  size_t object_size = (needs_ext_typedarray_obj ? sizeof (ecma_extended_typedarray_object_t)
                                                 : sizeof (ecma_extended_object_t));

  ecma_object_t *object_p = ecma_create_object (proto_p, object_size, ECMA_OBJECT_TYPE_PSEUDO_ARRAY);

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
  ext_object_p->u.pseudo_array.u1.class_id = ecma_typedarray_magic_string_list[typedarray_id];
  ext_object_p->u.pseudo_array.type = ECMA_PSEUDO_ARRAY_TYPEDARRAY;
  ext_object_p->u.pseudo_array.extra_info = (uint8_t) typedarray_id;
  ext_object_p->u.pseudo_array.u2.arraybuffer = ecma_make_object_value (arraybuffer_p);

  if (needs_ext_typedarray_obj)
  {
    ext_object_p->u.pseudo_array.type = ECMA_PSEUDO_ARRAY_TYPEDARRAY_WITH_INFO;

    ecma_extended_typedarray_object_t *typedarray_info_p = (ecma_extended_typedarray_object_t *) object_p;
    typedarray_info_p->array_length = array_length;
    typedarray_info_p->byte_offset = byte_offset;
  }

  return ecma_make_object_value (object_p);
} /* ecma_typedarray_create_object_with_buffer */

/**
 * Create a TypedArray object by given another TypedArray object
 *
 * See also: ES2015 22.2.1.3
 *
 * @return ecma value of the new typedarray object
 *         Returned value must be freed with ecma_free_value
 */
static ecma_value_t
ecma_typedarray_create_object_with_typedarray (ecma_object_t *typedarray_p, /**< a typedarray object */
                                               ecma_object_t *proto_p, /**< prototype object */
                                               uint8_t element_size_shift, /**< the size shift of the element length */
                                               ecma_typedarray_type_t typedarray_id) /**< id of the typedarray */
{
  ecma_length_t array_length = ecma_typedarray_get_length (typedarray_p);
  ecma_object_t *src_arraybuffer_p = ecma_typedarray_get_arraybuffer (typedarray_p);
  if (ecma_arraybuffer_is_detached (src_arraybuffer_p))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Invalid detached ArrayBuffer."));
  }

  ecma_value_t new_typedarray = ecma_typedarray_create_object_with_length (array_length,
                                                                           proto_p,
                                                                           element_size_shift,
                                                                           typedarray_id);

  if (ECMA_IS_VALUE_ERROR (new_typedarray))
  {
    return new_typedarray;
  }

  ecma_object_t *new_typedarray_p = ecma_get_object_from_value (new_typedarray);

  lit_utf8_byte_t *src_buf_p = ecma_arraybuffer_get_buffer (src_arraybuffer_p);

  ecma_object_t *dst_arraybuffer_p = ecma_typedarray_get_arraybuffer (new_typedarray_p);
  lit_utf8_byte_t *dst_buf_p = ecma_arraybuffer_get_buffer (dst_arraybuffer_p);

  src_buf_p += ecma_typedarray_get_offset (typedarray_p);

  ecma_typedarray_type_t src_id = ecma_get_typedarray_id (typedarray_p);

  if (src_id == typedarray_id)
  {
    memcpy (dst_buf_p, src_buf_p, array_length << element_size_shift);
  }
  else
  {
    uint32_t src_element_size = 1u << ecma_typedarray_get_element_size_shift (typedarray_p);
    uint32_t dst_element_size = 1u << element_size_shift;
    ecma_typedarray_getter_fn_t src_typedarray_getter_cb = ecma_get_typedarray_getter_fn (src_id);
    ecma_typedarray_setter_fn_t target_typedarray_setter_cb = ecma_get_typedarray_setter_fn (typedarray_id);


    for (uint32_t i = 0; i < array_length; i++)
    {
      /* Convert values from source to destination format. */
      ecma_number_t tmp = src_typedarray_getter_cb (src_buf_p);
      target_typedarray_setter_cb (dst_buf_p, tmp);

      src_buf_p += src_element_size;
      dst_buf_p += dst_element_size;
    }
  }

  return new_typedarray;
} /* ecma_typedarray_create_object_with_typedarray */

/**
 * Create a TypedArray object by transforming from an array-like object
 *
 * See also: ES2015 22.2.2.1
 *
 * @return ecma value of the new typedarray object
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_typedarray_from (ecma_value_t items_val, /**< the source array-like object */
                         ecma_value_t map_fn_val, /**< map function */
                         ecma_value_t this_val, /**< this_arg for the above map function */
                         ecma_object_t *proto_p, /**< prototype object */
                         uint8_t element_size_shift, /**< the size shift of the element length */
                         ecma_typedarray_type_t typedarray_id) /**< id of the typedarray */
{
  /* 3 */
  JERRY_ASSERT (ecma_op_is_callable (map_fn_val) || ecma_is_value_undefined (map_fn_val));

  ecma_object_t *func_object_p = NULL;
  if (!ecma_is_value_undefined (map_fn_val))
  {
    func_object_p = ecma_get_object_from_value (map_fn_val);
  }

  /* 10 */
  ecma_value_t arraylike_object_val = ecma_op_to_object (items_val);

  if (ECMA_IS_VALUE_ERROR (arraylike_object_val))
  {
    return arraylike_object_val;
  }

  ecma_object_t *arraylike_object_p = ecma_get_object_from_value (arraylike_object_val);

  /* 12 */
  uint32_t len;
  ecma_value_t len_value = ecma_op_object_get_length (arraylike_object_p, &len);

  ecma_value_t ret_value = ECMA_VALUE_ERROR;
  if (ECMA_IS_VALUE_ERROR (len_value))
  {
    ecma_deref_object (arraylike_object_p);
    return len_value;
  }

  /* 14 */
  ecma_value_t new_typedarray = ecma_typedarray_create_object_with_length (len,
                                                                           proto_p,
                                                                           element_size_shift,
                                                                           typedarray_id);

  if (ECMA_IS_VALUE_ERROR (new_typedarray))
  {
    ecma_deref_object (arraylike_object_p);
    return new_typedarray;
  }

  ecma_object_t *new_typedarray_p = ecma_get_object_from_value (new_typedarray);
  ecma_typedarray_info_t info = ecma_typedarray_get_info (new_typedarray_p);
  ecma_typedarray_setter_fn_t setter_cb = ecma_get_typedarray_setter_fn (info.id);
  ecma_number_t num_var;

  /* 17 */
  for (uint32_t index = 0; index < len; index++)
  {
    /* 17.b */
    ecma_value_t current_value = ecma_op_object_find_by_uint32_index (arraylike_object_p, index);

    if (ECMA_IS_VALUE_ERROR (current_value))
    {
      goto cleanup;
    }

    if (ecma_is_value_found (current_value))
    {
      ecma_value_t mapped_value;
      if (func_object_p != NULL)
      {
        /* 17.d 17.f */
        ecma_value_t current_index = ecma_make_uint32_value (index);
        ecma_value_t call_args[] = { current_value, current_index };

        ecma_value_t cb_value = ecma_op_function_call (func_object_p, this_val, call_args, 2);

        ecma_free_value (current_index);
        ecma_free_value (current_value);

        if (ECMA_IS_VALUE_ERROR (cb_value))
        {
          goto cleanup;
        }

        mapped_value = cb_value;
      }
      else
      {
        mapped_value = current_value;
      }

      ecma_value_t mapped_number = ecma_get_number (mapped_value, &num_var);
      ecma_free_value (mapped_value);

      if (ECMA_IS_VALUE_ERROR (mapped_number))
      {
        goto cleanup;
      }

      if (index >= info.length)
      {
        ecma_raise_type_error (ECMA_ERR_MSG ("Invalid argument type."));
        goto cleanup;
      }

      ecma_length_t byte_pos = (index << info.shift);
      setter_cb (info.buffer_p + byte_pos, num_var);
    }
  }

  ecma_ref_object (new_typedarray_p);
  ret_value = ecma_make_object_value (new_typedarray_p);

cleanup:
  ecma_deref_object (new_typedarray_p);
  ecma_deref_object (arraylike_object_p);

  return ret_value;
} /* ecma_op_typedarray_from */

/**
 * Get the arraybuffer of the typedarray object
 *
 * @return the pointer to the internal arraybuffer
 */
inline ecma_object_t * JERRY_ATTR_ALWAYS_INLINE
ecma_typedarray_get_arraybuffer (ecma_object_t *typedarray_p) /**< the pointer to the typedarray object */
{
  JERRY_ASSERT (ecma_object_is_typedarray (typedarray_p));

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) typedarray_p;

  return ecma_get_object_from_value (ext_object_p->u.pseudo_array.u2.arraybuffer);
} /* ecma_typedarray_get_arraybuffer */

/**
 * Get the element size shift in the typedarray object
 *
 * @return the size shift of the element, size is 1 << shift
 */
uint8_t
ecma_typedarray_get_element_size_shift (ecma_object_t *typedarray_p) /**< the pointer to the typedarray object */
{
  JERRY_ASSERT (ecma_object_is_typedarray (typedarray_p));

  return ecma_typedarray_helper_get_shift_size (ecma_get_typedarray_id (typedarray_p));
} /* ecma_typedarray_get_element_size_shift */


/**
 * Get the array length of the typedarray object
 *
 * @return the array length
 */
ecma_length_t
ecma_typedarray_get_length (ecma_object_t *typedarray_p) /**< the pointer to the typedarray object */
{
  JERRY_ASSERT (ecma_object_is_typedarray (typedarray_p));

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) typedarray_p;

  if (ext_object_p->u.pseudo_array.type == ECMA_PSEUDO_ARRAY_TYPEDARRAY)
  {
    ecma_object_t *arraybuffer_p = ecma_get_object_from_value (ext_object_p->u.pseudo_array.u2.arraybuffer);
    ecma_length_t buffer_length = ecma_arraybuffer_get_length (arraybuffer_p);
    uint8_t shift = ecma_typedarray_get_element_size_shift (typedarray_p);

    return buffer_length >> shift;
  }

  ecma_object_t *arraybuffer_p = ecma_typedarray_get_arraybuffer (typedarray_p);
  if (ecma_arraybuffer_is_detached (arraybuffer_p))
  {
    return 0;
  }

  ecma_extended_typedarray_object_t *info_p = (ecma_extended_typedarray_object_t *) ext_object_p;

  return info_p->array_length;
} /* ecma_typedarray_get_length */

/**
 * Get the offset of the internal arraybuffer
 *
 * @return the offset
 */
ecma_length_t
ecma_typedarray_get_offset (ecma_object_t *typedarray_p) /**< the pointer to the typedarray object */
{
  JERRY_ASSERT (ecma_object_is_typedarray (typedarray_p));

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) typedarray_p;

  if (ext_object_p->u.pseudo_array.type == ECMA_PSEUDO_ARRAY_TYPEDARRAY)
  {
    return 0;
  }

  ecma_object_t *arraybuffer_p = ecma_typedarray_get_arraybuffer (typedarray_p);
  if (ecma_arraybuffer_is_detached (arraybuffer_p))
  {
    return 0;
  }

  ecma_extended_typedarray_object_t *info_p = (ecma_extended_typedarray_object_t *) ext_object_p;

  return info_p->byte_offset;
} /* ecma_typedarray_get_offset */

/**
 * Utility function: return the pointer of the data buffer referenced by the typed array
 *
 * @return pointer to the data buffer
 */
lit_utf8_byte_t *
ecma_typedarray_get_buffer (ecma_object_t *typedarray_p) /**< the pointer to the typed array object */
{
  ecma_object_t *arraybuffer_p = ecma_typedarray_get_arraybuffer (typedarray_p);

  return ecma_arraybuffer_get_buffer (arraybuffer_p) + ecma_typedarray_get_offset (typedarray_p);
} /* ecma_typedarray_get_buffer */

/**
 * Create a new typedarray object.
 *
 * The struct of the typedarray object
 *   ecma_object_t
 *   extend_part
 *   typedarray_info
 *
 * @return ecma value of the new typedarray object
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_create_typedarray (const ecma_value_t *arguments_list_p, /**< the arg list passed to typedarray construct */
                           ecma_length_t arguments_list_len, /**< the length of the arguments_list_p */
                           ecma_object_t *proto_p, /**< prototype object */
                           uint8_t element_size_shift, /**< the size shift of the element length */
                           ecma_typedarray_type_t typedarray_id) /**< id of the typedarray */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  ecma_value_t ret = ECMA_VALUE_EMPTY;

  if (arguments_list_len == 0)
  {
    /* 22.2.1.1 */
    ret = ecma_typedarray_create_object_with_length (0, proto_p, element_size_shift, typedarray_id);
  }
  else if (!ecma_is_value_object (arguments_list_p[0]))
  {
    /* 22.2.1.2 */
    if (ecma_is_value_undefined (arguments_list_p[0]))
    {
      return ecma_raise_type_error (ECMA_ERR_MSG ("length argument is undefined"));
    }

    ECMA_OP_TO_NUMBER_TRY_CATCH (num, arguments_list_p[0], ret);

    uint32_t length = ecma_number_to_uint32 (num);

    if (num != ((ecma_number_t) length))
    {
      ret = ecma_raise_range_error (ECMA_ERR_MSG ("Invalid typedarray length."));
    }
    else
    {
      ret = ecma_typedarray_create_object_with_length (length,
                                                       proto_p,
                                                       element_size_shift,
                                                       typedarray_id);
    }

    ECMA_OP_TO_NUMBER_FINALIZE (num);
  }
  else if (ecma_is_value_object (arguments_list_p[0]))
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (arguments_list_p[0]);
    if (ecma_object_is_typedarray (obj_p))
    {
      /* 22.2.1.3 */
      ecma_object_t *typedarray_p = obj_p;
      ret = ecma_typedarray_create_object_with_typedarray (typedarray_p,
                                                           proto_p,
                                                           element_size_shift,
                                                           typedarray_id);
    }
    else if (ecma_object_class_is (obj_p, LIT_MAGIC_STRING_ARRAY_BUFFER_UL))
    {
      /* 22.2.1.5 */
      ecma_object_t *arraybuffer_p = obj_p;
      ecma_value_t arg2 = ((arguments_list_len > 1) ? arguments_list_p[1]
                                                    : ECMA_VALUE_UNDEFINED);

      ecma_value_t arg3 = ((arguments_list_len > 2) ? arguments_list_p[2]
                                                    : ECMA_VALUE_UNDEFINED);

      ECMA_OP_TO_NUMBER_TRY_CATCH (num2, arg2, ret);
      uint32_t offset = ecma_number_to_uint32 (num2);

      if (ecma_number_is_negative (ecma_number_to_int32 (num2)) || (offset % (uint32_t) (1 << element_size_shift) != 0))
      {
        ret = ecma_raise_range_error (ECMA_ERR_MSG ("Invalid offset."));
      }
      else if (ecma_arraybuffer_is_detached (arraybuffer_p))
      {
        ret = ecma_raise_type_error (ECMA_ERR_MSG ("Invalid detached ArrayBuffer."));
      }
      else
      {
        ecma_length_t buf_byte_length = ecma_arraybuffer_get_length (arraybuffer_p);
        ecma_length_t new_byte_length = 0;

        if (ecma_is_value_undefined (arg3))
        {
          if (buf_byte_length % (uint32_t) (1 << element_size_shift) != 0)
          {
            ret = ecma_raise_range_error (ECMA_ERR_MSG ("Invalid length."));
          }
          else if (buf_byte_length < offset)
          {
            ret = ecma_raise_range_error (ECMA_ERR_MSG ("Invalid length."));
          }
          else
          {
            new_byte_length = (ecma_length_t) (buf_byte_length - offset);
          }
        }
        else
        {
          uint32_t new_length;
          if (ECMA_IS_VALUE_ERROR (ecma_op_to_length (arg3, &new_length)))
          {
            return ECMA_VALUE_ERROR;
          }

          if (new_length > (UINT32_MAX >> element_size_shift))
          {
            ret = ecma_raise_range_error (ECMA_ERR_MSG ("Maximum typedarray size is reached."));
          }
          else
          {
            new_byte_length = (ecma_length_t) new_length << element_size_shift;

            if (((ecma_number_t) new_byte_length + offset) > buf_byte_length)
            {
              ret = ecma_raise_range_error (ECMA_ERR_MSG ("Invalid length."));
            }
          }
        }

        if (ecma_is_value_empty (ret))
        {
          ecma_length_t array_length = new_byte_length >> element_size_shift;
          ret = ecma_typedarray_create_object_with_buffer (arraybuffer_p,
                                                           offset,
                                                           array_length,
                                                           proto_p,
                                                           element_size_shift,
                                                           typedarray_id);
        }
      }

      ECMA_OP_TO_NUMBER_FINALIZE (num2);
    }
    else
    {
      /* 22.2.1.4 */
      ecma_value_t undef = ECMA_VALUE_UNDEFINED;
      ret = ecma_op_typedarray_from (arguments_list_p[0],
                                     undef,
                                     undef,
                                     proto_p,
                                     element_size_shift,
                                     typedarray_id);
    }
  }

  return ret;
} /* ecma_op_create_typedarray */

/**
 * Check if the object is typedarray
 *
 * @return true - if object is a TypedArray object
 *         false - otherwise
 */
bool
ecma_object_is_typedarray (ecma_object_t *obj_p) /**< the target object need to be checked */
{
  JERRY_ASSERT (!ecma_is_lexical_environment (obj_p));

  if (ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_PSEUDO_ARRAY)
  {
    ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) obj_p;

    return ((ext_object_p->u.pseudo_array.type == ECMA_PSEUDO_ARRAY_TYPEDARRAY)
            || (ext_object_p->u.pseudo_array.type == ECMA_PSEUDO_ARRAY_TYPEDARRAY_WITH_INFO));
  }

  return false;
} /* ecma_object_is_typedarray */

/**
 * Check if the value is typedarray
 *
 * @return true - if value is a TypedArray object
 *         false - otherwise
 */
bool
ecma_is_typedarray (ecma_value_t value) /**< the target need to be checked */
{
  if (!ecma_is_value_object (value))
  {
    return false;
  }

  return ecma_object_is_typedarray (ecma_get_object_from_value (value));
} /* ecma_is_typedarray */

/**
 * List names of a TypedArray object's integer indexed properties
 *
 * @return void
 */
void
ecma_op_typedarray_list_lazy_property_names (ecma_object_t *obj_p, /**< a TypedArray object */
                                             ecma_collection_t *main_collection_p) /**< 'main' collection */
{
  JERRY_ASSERT (ecma_object_is_typedarray (obj_p));

  ecma_length_t array_length = ecma_typedarray_get_length (obj_p);

  for (ecma_length_t i = 0; i < array_length; i++)
  {
    ecma_string_t *name_p = ecma_new_ecma_string_from_uint32 (i);

    ecma_collection_push_back (main_collection_p, ecma_make_string_value (name_p));
  }
} /* ecma_op_typedarray_list_lazy_property_names */

/**
 * Define the integer number property value of the typedarray
 *
 * See also: ES2015 9.4.5.3: 3.c
 *
 * @return boolean, false if failed
 */
bool
ecma_op_typedarray_define_index_prop (ecma_object_t *obj_p, /**< a TypedArray object */
                                      uint32_t index, /**< the index number */
                                      const ecma_property_descriptor_t *property_desc_p) /**< the description of
                                                                                               the prop */
{
  JERRY_ASSERT (ecma_object_is_typedarray (obj_p));

  ecma_length_t array_length = ecma_typedarray_get_length (obj_p);

  if ((index >= array_length)
      || (property_desc_p->flags & (ECMA_PROP_IS_GET_DEFINED | ECMA_PROP_IS_SET_DEFINED))
      || ((property_desc_p->flags & (ECMA_PROP_IS_CONFIGURABLE_DEFINED | ECMA_PROP_IS_CONFIGURABLE))
           == (ECMA_PROP_IS_CONFIGURABLE_DEFINED | ECMA_PROP_IS_CONFIGURABLE))
      || ((property_desc_p->flags & ECMA_PROP_IS_ENUMERABLE_DEFINED)
          && !(property_desc_p->flags & ECMA_PROP_IS_ENUMERABLE))
      || ((property_desc_p->flags & ECMA_PROP_IS_WRITABLE_DEFINED)
          && !(property_desc_p->flags & ECMA_PROP_IS_WRITABLE)))
  {
    return false;
  }

  if (property_desc_p->flags & ECMA_PROP_IS_VALUE_DEFINED)
  {
    ecma_number_t num_var;
    ecma_value_t error = ecma_get_number (property_desc_p->value, &num_var);

    if (ECMA_IS_VALUE_ERROR (error))
    {
      ecma_free_value (JERRY_CONTEXT (error_value));
      return false;
    }
    ecma_typedarray_info_t info = ecma_typedarray_get_info (obj_p);

    if (index >= info.length)
    {
      return false;
    }

    lit_utf8_byte_t *src_buffer = info.buffer_p + (index << info.shift);
    ecma_set_typedarray_element (src_buffer, num_var, info.id);
  }

  return true;
} /* ecma_op_typedarray_define_index_prop */

/**
 * Create a typedarray object based on the "type" and arraylength
 * The "type" is same with arg1
 *
 * @return ecma_value_t
 */
ecma_value_t
ecma_op_create_typedarray_with_type_and_length (ecma_object_t *obj_p, /**< TypedArray object
                                                                        *  indicates the type */
                                                ecma_length_t array_length) /**< length of the typedarray */
{
  JERRY_ASSERT (ecma_object_is_typedarray (obj_p));

#if ENABLED (JERRY_ES2015)
  ecma_value_t constructor_value = ecma_op_object_get_by_magic_id (obj_p, LIT_MAGIC_STRING_CONSTRUCTOR);

  if (ECMA_IS_VALUE_ERROR (constructor_value)
      || !ecma_is_constructor (constructor_value))
  {
    ecma_free_value (constructor_value);
    return ecma_raise_type_error (ECMA_ERR_MSG ("object.constructor is not a constructor."));
  }

  ecma_object_t *constructor_object_p = ecma_get_object_from_value (constructor_value);
  ecma_value_t constructor_prototype = ecma_op_object_get_by_magic_id (constructor_object_p,
                                                                       LIT_MAGIC_STRING_PROTOTYPE);

  ecma_deref_object (constructor_object_p);

  if (ECMA_IS_VALUE_ERROR (constructor_prototype))
  {
    return constructor_prototype;
  }
#endif /* ENABLED (JERRY_ES2015) */

  ecma_typedarray_type_t typedarray_id = ecma_get_typedarray_id (obj_p);
  ecma_object_t *proto_p = ecma_builtin_get (ecma_typedarray_helper_get_prototype_id (typedarray_id));
  uint8_t element_size_shift = ecma_typedarray_helper_get_shift_size (typedarray_id);

  ecma_value_t new_obj = ecma_typedarray_create_object_with_length (array_length,
                                                                    proto_p,
                                                                    element_size_shift,
                                                                    typedarray_id);

#if ENABLED (JERRY_ES2015)
  ecma_object_t *constructor_prototype_object_p = ecma_get_object_from_value (constructor_prototype);
  ecma_object_t *new_obj_p = ecma_get_object_from_value (new_obj);
  ECMA_SET_NON_NULL_POINTER (new_obj_p->u2.prototype_cp, constructor_prototype_object_p);

  ecma_deref_object (constructor_prototype_object_p);
#endif /* ENABLED (JERRY_ES2015) */

  return new_obj;
} /* ecma_op_create_typedarray_with_type_and_length */

/**
 * Method for getting the additional typedArray informations.
 */
ecma_typedarray_info_t
ecma_typedarray_get_info (ecma_object_t *typedarray_p)
{
  ecma_typedarray_info_t info;

  info.id = ecma_get_typedarray_id (typedarray_p);
  info.length = ecma_typedarray_get_length (typedarray_p);
  info.shift = ecma_typedarray_get_element_size_shift (typedarray_p);
  info.element_size = (uint8_t) (1 << info.shift);
  info.offset = ecma_typedarray_get_offset (typedarray_p);
  info.array_buffer_p = ecma_typedarray_get_arraybuffer (typedarray_p);
  info.buffer_p = ecma_arraybuffer_get_buffer (info.array_buffer_p) + info.offset;

  return info;
} /* ecma_typedarray_get_info */

/**
 * @}
 * @}
 */
#endif /* ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY) */
