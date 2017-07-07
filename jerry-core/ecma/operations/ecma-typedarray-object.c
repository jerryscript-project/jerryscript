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

#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmatypedarrayobject ECMA TypedArray object related routines
 * @{
 */

/**
 * Convert the value in location,
 * cast it from type to ecma_number_t
 * then assign to variale number
 */
#define GET_ELEMENT(type, location) \
  ((ecma_number_t) (*((type *) location)));

/**
 * set typedarray's element value
 *
 * @return ecma_number_t: the value of the element
 */
ecma_number_t
ecma_get_typedarray_element (lit_utf8_byte_t *src, /**< the location in the internal arraybuffer */
                             lit_magic_string_id_t class_id) /**< class name of the typedarray */
{
  switch (class_id)
  {
    case LIT_MAGIC_STRING_INT8_ARRAY_UL:
    {
      return GET_ELEMENT (int8_t, src);
    }
    case LIT_MAGIC_STRING_UINT8_ARRAY_UL:
    case LIT_MAGIC_STRING_UINT8_CLAMPED_ARRAY_UL:
    {
      return GET_ELEMENT (uint8_t, src);
    }
    case LIT_MAGIC_STRING_INT16_ARRAY_UL:
    {
      return GET_ELEMENT (int16_t, src);
    }
    case LIT_MAGIC_STRING_UINT16_ARRAY_UL:
    {
      return GET_ELEMENT (uint16_t, src);
    }
    case LIT_MAGIC_STRING_INT32_ARRAY_UL:
    {
      return GET_ELEMENT (int32_t, src);
    }
    case LIT_MAGIC_STRING_UINT32_ARRAY_UL:
    {
      return GET_ELEMENT (uint32_t, src);
    }
    case LIT_MAGIC_STRING_FLOAT32_ARRAY_UL:
    {
      return GET_ELEMENT (float, src);
    }
#if CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64
    case LIT_MAGIC_STRING_FLOAT64_ARRAY_UL:
    {
      return GET_ELEMENT (double, src);
    }
#endif /* CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64 */
    default:
    {
      JERRY_UNREACHABLE ();
      return 0;
    }
  }
} /* ecma_get_typedarray_element */

#undef GET_ELEMENT

/**
 * set typedarray's element value
 */
void
ecma_set_typedarray_element (lit_utf8_byte_t *dst_p, /**< the location in the internal arraybuffer */
                             ecma_number_t value, /**< the number value to set */
                             lit_magic_string_id_t class_id) /**< class name of the typedarray */
{
  switch (class_id)
  {
    case LIT_MAGIC_STRING_UINT8_CLAMPED_ARRAY_UL:
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
      *((uint8_t *) dst_p) = clamped;
      return;
    }
    case LIT_MAGIC_STRING_FLOAT32_ARRAY_UL:
    {
      *((float *) dst_p) = (float) value;
      return;
    }
#if CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64
    case LIT_MAGIC_STRING_FLOAT64_ARRAY_UL:
    {
      *((double *) dst_p) = (double) value;
      return;
    }
#endif /* CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64 */
    default:
    {
      break;
    }
  }

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

  switch (class_id)
  {
    case LIT_MAGIC_STRING_INT8_ARRAY_UL:
    {
      *((int8_t *) dst_p) = (int8_t) uint32_value;
      break;
    }
    case LIT_MAGIC_STRING_UINT8_ARRAY_UL:
    {
      *((uint8_t *) dst_p) = (uint8_t) uint32_value;
      break;
    }
    case LIT_MAGIC_STRING_INT16_ARRAY_UL:
    {
      *((int16_t *) dst_p) = (int16_t) uint32_value;
      break;
    }
    case LIT_MAGIC_STRING_UINT16_ARRAY_UL:
    {
      *((uint16_t *) dst_p) = (uint16_t) uint32_value;
      break;
    }
    case LIT_MAGIC_STRING_INT32_ARRAY_UL:
    {
      *((int32_t *) dst_p) = (int32_t) uint32_value;
      break;
    }
    case LIT_MAGIC_STRING_UINT32_ARRAY_UL:
    {
      *((uint32_t *) dst_p) = uint32_value;
      break;
    }
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }
} /* ecma_set_typedarray_element */

/**
 * Create a TypedArray object by given array_length
 *
 * See also: ES2015 22.2.1.2.1
 *
 * @return ecma value of the new typedarray object
 *         Returned value must be freed with ecma_free_value
 */
static ecma_value_t
ecma_typedarray_create_object_with_length (ecma_length_t array_length, /**< length of the typedarray */
                                           ecma_object_t *proto_p, /**< prototype object */
                                           uint8_t element_size_shift, /**< the size shift of the element length */
                                           lit_magic_string_id_t class_id) /**< class name of the typedarray */
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

  ecma_value_t new_arraybuffer_p = ecma_make_object_value (ecma_arraybuffer_new_object (byte_length));
  ecma_object_t *object_p = ecma_create_object (proto_p,
                                                sizeof (ecma_extended_object_t),
                                                ECMA_OBJECT_TYPE_PSEUDO_ARRAY);

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
  ext_object_p->u.pseudo_array.u1.class_id = class_id;
  ext_object_p->u.pseudo_array.type = ECMA_PSEUDO_ARRAY_TYPEDARRAY;
  ext_object_p->u.pseudo_array.extra_info = element_size_shift;
  ext_object_p->u.pseudo_array.u2.arraybuffer = new_arraybuffer_p;

  ecma_free_value (new_arraybuffer_p);

  return ecma_make_object_value (object_p);
} /* !ecma_typedarray_create_object_with_length */

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
                                           lit_magic_string_id_t class_id) /**< class name of the typedarray */
{
  ecma_length_t expected_length = (ecma_arraybuffer_get_length (arraybuffer_p) >> element_size_shift);

  bool needs_ext_typedarray_obj = (byte_offset != 0 || array_length != expected_length);

  size_t object_size = (needs_ext_typedarray_obj ? sizeof (ecma_extended_typedarray_object_t)
                                                 : sizeof (ecma_extended_object_t));

  ecma_object_t *object_p = ecma_create_object (proto_p, object_size, ECMA_OBJECT_TYPE_PSEUDO_ARRAY);

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
  ext_object_p->u.pseudo_array.u1.class_id = class_id;
  ext_object_p->u.pseudo_array.type = ECMA_PSEUDO_ARRAY_TYPEDARRAY;
  ext_object_p->u.pseudo_array.extra_info = element_size_shift;
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
                                               lit_magic_string_id_t class_id) /**< class name of the typedarray */
{
  ecma_length_t array_length = ecma_typedarray_get_length (typedarray_p);

  ecma_value_t new_typedarray = ecma_typedarray_create_object_with_length (array_length,
                                                                           proto_p,
                                                                           element_size_shift,
                                                                           class_id);

  if (ECMA_IS_VALUE_ERROR (new_typedarray))
  {
    return new_typedarray;
  }

  ecma_object_t *new_typedarray_p = ecma_get_object_from_value (new_typedarray);

  ecma_object_t *src_arraybuffer_p = ecma_typedarray_get_arraybuffer (typedarray_p);
  lit_utf8_byte_t *src_buf_p = ecma_arraybuffer_get_buffer (src_arraybuffer_p);

  ecma_object_t *dst_arraybuffer_p = ecma_typedarray_get_arraybuffer (new_typedarray_p);
  lit_utf8_byte_t *dst_buf_p = ecma_arraybuffer_get_buffer (dst_arraybuffer_p);

  src_buf_p += ecma_typedarray_get_offset (typedarray_p);

  lit_magic_string_id_t src_class_id = ecma_object_get_class_name (typedarray_p);

  if (src_class_id == class_id)
  {
    memcpy (dst_buf_p, src_buf_p, array_length << element_size_shift);
  }
  else
  {
    uint32_t src_element_size = 1u << ecma_typedarray_get_element_size_shift (typedarray_p);
    uint32_t dst_element_size = 1u << element_size_shift;

    for (uint32_t i = 0; i < array_length; i++)
    {
      /* Convert values from source to destination format. */
      ecma_number_t tmp = ecma_get_typedarray_element (src_buf_p, src_class_id);
      ecma_set_typedarray_element (dst_buf_p, tmp, class_id);

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
                         lit_magic_string_id_t class_id) /**< class name of the typedarray */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 3 */
  JERRY_ASSERT (ecma_op_is_callable (map_fn_val) || ecma_is_value_undefined (map_fn_val));

  ecma_object_t *func_object_p = NULL;
  if (!ecma_is_value_undefined (map_fn_val))
  {
    func_object_p = ecma_get_object_from_value (map_fn_val);
  }

  /* 10 */
  ECMA_TRY_CATCH (arraylike_object_val,
                  ecma_op_to_object (items_val),
                  ret_value);

  ecma_object_t *arraylike_object_p = ecma_get_object_from_value (arraylike_object_val);

  /* 12 */
  ecma_string_t magic_string_length;
  ecma_init_ecma_length_string (&magic_string_length);

  ECMA_TRY_CATCH (len_value,
                  ecma_op_object_get (arraylike_object_p, &magic_string_length),
                  ret_value);

  ECMA_OP_TO_NUMBER_TRY_CATCH (len_number, len_value, ret_value);

  uint32_t len = ecma_number_to_uint32 (len_number);

  /* 14 */
  ecma_value_t new_typedarray = ecma_typedarray_create_object_with_length (len,
                                                                           proto_p,
                                                                           element_size_shift,
                                                                           class_id);

  if (ECMA_IS_VALUE_ERROR (new_typedarray))
  {
    ret_value = ecma_copy_value (new_typedarray);
  }

  ecma_object_t *new_typedarray_p = ecma_get_object_from_value (new_typedarray);

  /* 17 */
  ecma_value_t current_index;

  for (uint32_t index = 0; index < len && ecma_is_value_empty (ret_value); index++)
  {
    /* 17.a */
    ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (index);

    /* 17.b */
    ECMA_TRY_CATCH (current_value, ecma_op_object_find (arraylike_object_p, index_str_p), ret_value);

    if (ecma_is_value_found (current_value))
    {
      if (func_object_p != NULL)
      {
        /* 17.d 17.f */
        current_index = ecma_make_uint32_value (index);
        ecma_value_t call_args[] = { current_value, current_index};

        ECMA_TRY_CATCH (mapped_value, ecma_op_function_call (func_object_p, this_val, call_args, 2), ret_value);

        bool set_status = ecma_op_typedarray_set_index_prop (new_typedarray_p, index, mapped_value);

        if (!set_status)
        {
          ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Invalid argument type."));;
        }

        ECMA_FINALIZE (mapped_value);
      }
      else
      {
        /* 17.e 17.f */
        bool set_status = ecma_op_typedarray_set_index_prop (new_typedarray_p, index, current_value);

        if (!set_status)
        {
          ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Invalid argument type."));;
        }
      }
    }

    ECMA_FINALIZE (current_value);

    ecma_deref_ecma_string (index_str_p);
  }

  if (ecma_is_value_empty (ret_value))
  {
    ret_value = ecma_make_object_value (new_typedarray_p);
  }
  else
  {
    ecma_deref_object (new_typedarray_p);
  }

  ECMA_OP_TO_NUMBER_FINALIZE (len_number);
  ECMA_FINALIZE (len_value);
  ECMA_FINALIZE (arraylike_object_val);

  return ret_value;
} /* ecma_op_typedarray_from */

/**
 * Get the arraybuffer of the typedarray object
 *
 * @return the pointer to the internal arraybuffer
 */
inline ecma_object_t * __attr_always_inline___
ecma_typedarray_get_arraybuffer (ecma_object_t *typedarray_p) /**< the pointer to the typedarray object */
{
  JERRY_ASSERT (ecma_is_typedarray (ecma_make_object_value (typedarray_p)));

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
  JERRY_ASSERT (ecma_is_typedarray (ecma_make_object_value (typedarray_p)));

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) typedarray_p;

  return ext_object_p->u.pseudo_array.extra_info;
} /* ecma_typedarray_get_element_size_shift */


/**
 * Get the array length of the typedarray object
 *
 * @return the array length
 */
ecma_length_t
ecma_typedarray_get_length (ecma_object_t *typedarray_p) /**< the pointer to the typedarray object */
{
  JERRY_ASSERT (ecma_is_typedarray (ecma_make_object_value (typedarray_p)));

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) typedarray_p;

  if (ext_object_p->u.pseudo_array.type == ECMA_PSEUDO_ARRAY_TYPEDARRAY)
  {
    ecma_object_t *arraybuffer_p = ecma_get_object_from_value (ext_object_p->u.pseudo_array.u2.arraybuffer);
    ecma_length_t buffer_length = ecma_arraybuffer_get_length (arraybuffer_p);
    uint8_t shift = ext_object_p->u.pseudo_array.extra_info;

    return buffer_length >> shift;
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
  JERRY_ASSERT (ecma_is_typedarray (ecma_make_object_value (typedarray_p)));

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) typedarray_p;

  if (ext_object_p->u.pseudo_array.type == ECMA_PSEUDO_ARRAY_TYPEDARRAY)
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
                           lit_magic_string_id_t class_id) /**< class name of the typedarray */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  ecma_value_t ret = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  if (arguments_list_len == 0)
  {
    /* 22.2.1.1 */
    ret = ecma_typedarray_create_object_with_length (0, proto_p, element_size_shift, class_id);
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
                                                       class_id);
    }

    ECMA_OP_TO_NUMBER_FINALIZE (num);
  }
  else if (ecma_is_value_object (arguments_list_p[0]))
  {
    if (ecma_is_typedarray (arguments_list_p[0]))
    {
      /* 22.2.1.3 */
      ecma_object_t *typedarray_p = ecma_get_object_from_value (arguments_list_p[0]);
      ret = ecma_typedarray_create_object_with_typedarray (typedarray_p,
                                                           proto_p,
                                                           element_size_shift,
                                                           class_id);
    }
    else if (ecma_is_arraybuffer (arguments_list_p[0]))
    {
      /* 22.2.1.5 */
      ecma_object_t *arraybuffer_p = ecma_get_object_from_value (arguments_list_p[0]);
      ecma_value_t arg2 = ((arguments_list_len > 1) ? arguments_list_p[1]
                                                    : ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED));

      ecma_value_t arg3 = ((arguments_list_len > 2) ? arguments_list_p[2]
                                                    : ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED));

      ECMA_OP_TO_NUMBER_TRY_CATCH (num2, arg2, ret);

      uint32_t offset = ecma_number_to_uint32 (num2);

      if (ecma_number_is_negative (ecma_number_to_int32 (num2)) || (offset % (uint32_t) (1 << element_size_shift) != 0))
      {
        ret = ecma_raise_range_error (ECMA_ERR_MSG ("Invalid offset."));
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
          ECMA_OP_TO_NUMBER_TRY_CATCH (num3, arg3, ret);
          int32_t new_length = ecma_number_to_int32 (num3);
          new_length = (new_length > 0) ? new_length : 0;

          if ((uint32_t) new_length > (UINT32_MAX >> element_size_shift))
          {
            ret = ecma_raise_range_error (ECMA_ERR_MSG ("Maximum typedarray size is reached."));
          }
          else
          {
            new_byte_length = (ecma_length_t) new_length << element_size_shift;

            if (new_byte_length + offset > buf_byte_length)
            {
              ret = ecma_raise_range_error (ECMA_ERR_MSG ("Invalid length."));
            }
          }

          ECMA_OP_TO_NUMBER_FINALIZE (num3);
        }

        if (ecma_is_value_empty (ret))
        {
          ecma_length_t array_length = new_byte_length >> element_size_shift;
          ret = ecma_typedarray_create_object_with_buffer (arraybuffer_p,
                                                           offset,
                                                           array_length,
                                                           proto_p,
                                                           element_size_shift,
                                                           class_id);
        }
      }

      ECMA_OP_TO_NUMBER_FINALIZE (num2);
    }
    else
    {
      /* 22.2.1.4 */
      ecma_value_t undef = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
      ret = ecma_op_typedarray_from (arguments_list_p[0],
                                     undef,
                                     undef,
                                     proto_p,
                                     element_size_shift,
                                     class_id);
    }
  }

  return ret;
} /* ecma_op_create_typedarray */

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

  ecma_object_t *obj_p = ecma_get_object_from_value (value);

  if (ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_PSEUDO_ARRAY)
  {
    ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) obj_p;

    return ((ext_object_p->u.pseudo_array.type == ECMA_PSEUDO_ARRAY_TYPEDARRAY)
            || (ext_object_p->u.pseudo_array.type == ECMA_PSEUDO_ARRAY_TYPEDARRAY_WITH_INFO));
  }

  return false;
} /* ecma_is_typedarray */

/**
 * List names of a TypedArray object's integer indexed properties
 *
 * @return void
 */
void
ecma_op_typedarray_list_lazy_property_names (ecma_object_t *obj_p, /**< a TypedArray object */
                                             ecma_collection_header_t *main_collection_p) /**< 'main'
                                                                                           *   collection */
{
  JERRY_ASSERT (ecma_is_typedarray (ecma_make_object_value (obj_p)));

  ecma_length_t array_length = ecma_typedarray_get_length (obj_p);

  for (ecma_length_t i = 0; i < array_length; i++)
  {
    ecma_string_t *name_p = ecma_new_ecma_string_from_uint32 (i);

    ecma_append_to_values_collection (main_collection_p, ecma_make_string_value (name_p), true);

    ecma_deref_ecma_string (name_p);
  }
} /* ecma_op_typedarray_list_lazy_property_names */

/**
 * Get the integer number property value of the typedarray
 *
 * See also: ES2015 9.4.5.8
 *
 * @return ecma value
 *         returns undefined if index is greater or equal than length
 */
ecma_value_t
ecma_op_typedarray_get_index_prop (ecma_object_t *obj_p, /**< a TypedArray object */
                                   uint32_t index) /**< the index number */
{
  JERRY_ASSERT (ecma_is_typedarray (ecma_make_object_value (obj_p)));

  ecma_length_t array_length = ecma_typedarray_get_length (obj_p);

  if (index >= array_length)
  {
    return ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
  }

  ecma_object_t *arraybuffer_p = ecma_typedarray_get_arraybuffer (obj_p);
  ecma_length_t offset = ecma_typedarray_get_offset (obj_p);
  uint8_t shift = ecma_typedarray_get_element_size_shift (obj_p);
  ecma_length_t byte_pos = (index << shift) + offset;
  lit_magic_string_id_t class_id = ecma_object_get_class_name (obj_p);
  lit_utf8_byte_t *target_p = ecma_arraybuffer_get_buffer (arraybuffer_p) + byte_pos;
  ecma_number_t value = ecma_get_typedarray_element (target_p, class_id);

  return ecma_make_number_value (value);
} /* ecma_op_typedarray_get_index_prop */

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
  JERRY_ASSERT (ecma_is_typedarray (ecma_make_object_value (obj_p)));

  ecma_length_t array_length = ecma_typedarray_get_length (obj_p);

  if (index >= array_length)
  {
    return false;
  }

  if (property_desc_p->is_get_defined
      || property_desc_p->is_set_defined)
  {
    return false;
  }

  if ((property_desc_p->is_configurable_defined && property_desc_p->is_configurable)
      || (property_desc_p->is_enumerable_defined && !property_desc_p->is_enumerable)
      || (property_desc_p->is_writable_defined && !property_desc_p->is_writable))
  {
    return false;
  }

  if (property_desc_p->is_value_defined)
  {
    return ecma_op_typedarray_set_index_prop (obj_p, index, property_desc_p->value);
  }

  return true;
} /* ecma_op_typedarray_define_index_prop */

/**
 * Set the integer number property value of the typedarray
 *
 * See also: ES2015 9.4.5.9
 *
 * @return boolean, false if failed
 */
bool
ecma_op_typedarray_set_index_prop (ecma_object_t *obj_p, /**< a TypedArray object */
                                   uint32_t index, /**< the index number */
                                   ecma_value_t value) /**< value of the property */
{
  JERRY_ASSERT (ecma_is_typedarray (ecma_make_object_value (obj_p)));

  ecma_length_t array_length = ecma_typedarray_get_length (obj_p);

  if (index >= array_length)
  {
    return false;
  }

  ecma_value_t error = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_OP_TO_NUMBER_TRY_CATCH (value_num, value, error);

  ecma_object_t *arraybuffer_p = ecma_typedarray_get_arraybuffer (obj_p);
  ecma_length_t offset = ecma_typedarray_get_offset (obj_p);
  uint8_t shift = ecma_typedarray_get_element_size_shift (obj_p);
  ecma_length_t byte_pos = (index << shift) + offset;
  lit_magic_string_id_t class_id = ecma_object_get_class_name (obj_p);
  lit_utf8_byte_t *target_p = ecma_arraybuffer_get_buffer (arraybuffer_p) + byte_pos;
  ecma_set_typedarray_element (target_p, value_num, class_id);

  ECMA_OP_TO_NUMBER_FINALIZE (value_num);

  if (ecma_is_value_empty (error))
  {
    return true;
  }

  return false;
} /* ecma_op_typedarray_set_index_prop */

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
  JERRY_ASSERT (ecma_is_typedarray (ecma_make_object_value (obj_p)));

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) obj_p;
  lit_magic_string_id_t class_id = ext_object_p->u.pseudo_array.u1.class_id;
  ecma_object_t *proto_p;
  uint8_t element_size_shift = 0;

  switch (class_id)
  {
    case LIT_MAGIC_STRING_INT8_ARRAY_UL:
    {
      proto_p = ecma_builtin_get (ECMA_BUILTIN_ID_INT8ARRAY_PROTOTYPE);
      element_size_shift = 0;
      break;
    }
    case LIT_MAGIC_STRING_UINT8_ARRAY_UL:
    {
      proto_p = ecma_builtin_get (ECMA_BUILTIN_ID_UINT8ARRAY_PROTOTYPE);
      element_size_shift = 0;
      break;
    }
    case LIT_MAGIC_STRING_UINT8_CLAMPED_ARRAY_UL:
    {
      proto_p = ecma_builtin_get (ECMA_BUILTIN_ID_UINT8CLAMPEDARRAY_PROTOTYPE);
      element_size_shift = 0;
      break;
    }
    case LIT_MAGIC_STRING_INT16_ARRAY_UL:
    {
      proto_p = ecma_builtin_get (ECMA_BUILTIN_ID_INT16ARRAY_PROTOTYPE);
      element_size_shift = 1;
      break;
    }
    case LIT_MAGIC_STRING_UINT16_ARRAY_UL:
    {
      proto_p = ecma_builtin_get (ECMA_BUILTIN_ID_UINT16ARRAY_PROTOTYPE);
      element_size_shift = 1;
      break;
    }
    case LIT_MAGIC_STRING_INT32_ARRAY_UL:
    {
      proto_p = ecma_builtin_get (ECMA_BUILTIN_ID_INT32ARRAY_PROTOTYPE);
      element_size_shift = 2;
      break;
    }
    case LIT_MAGIC_STRING_UINT32_ARRAY_UL:
    {
      proto_p = ecma_builtin_get (ECMA_BUILTIN_ID_UINT32ARRAY_PROTOTYPE);
      element_size_shift = 2;
      break;
    }
    case LIT_MAGIC_STRING_FLOAT32_ARRAY_UL:
    {
      proto_p = ecma_builtin_get (ECMA_BUILTIN_ID_FLOAT32ARRAY_PROTOTYPE);
      element_size_shift = 2;
      break;
    }
#if CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64
    case LIT_MAGIC_STRING_FLOAT64_ARRAY_UL:
    {
      proto_p = ecma_builtin_get (ECMA_BUILTIN_ID_FLOAT64ARRAY_PROTOTYPE);
      element_size_shift = 3;
      break;
    }
#endif /* CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64 */
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }

  ecma_value_t new_obj = ecma_typedarray_create_object_with_length (array_length,
                                                                    proto_p,
                                                                    element_size_shift,
                                                                    class_id);

  ecma_deref_object (proto_p);

  return new_obj;
} /* ecma_op_create_typedarray_with_type_and_length */

/**
 * @}
 * @}
 */
#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
