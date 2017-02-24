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

#ifndef CONFIG_DISABLE_TYPEDARRAY_BUILTIN

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
#define GET_ELEMENT(type, location, number) \
  type v = *((type *) location); \
  number = (ecma_number_t) v;

/**
 * Cast the number from ecma_numbet_t into type,
 * and store the number in the location
 */
#define SET_ELEMENT(type, location, number) \
  int64_t v = (int64_t) number; \
  *((type *) location) = (type) v;

/**
 * set typedarray's element value
 *
 * @return ecma_number_t: the value of the element
 */
static ecma_number_t
get_typedarray_element (lit_utf8_byte_t *src, /**< the location in the internal arraybuffer */
                        lit_magic_string_id_t class_id) /**< class name of the typedarray */
{
  ecma_number_t ret_num;

  switch (class_id)
  {
    case LIT_MAGIC_STRING_INT8_ARRAY_UL:
    {
      GET_ELEMENT (int8_t, src, ret_num);
      break;
    }
    case LIT_MAGIC_STRING_UINT8_ARRAY_UL:
    case LIT_MAGIC_STRING_UINT8_CLAMPED_ARRAY_UL:
    {
      GET_ELEMENT (uint8_t, src, ret_num);
      break;
    }
    case LIT_MAGIC_STRING_INT16_ARRAY_UL:
    {
      GET_ELEMENT (int16_t, src, ret_num);
      break;
    }
    case LIT_MAGIC_STRING_UINT16_ARRAY_UL:
    {
      GET_ELEMENT (uint16_t, src, ret_num);
      break;
    }
    case LIT_MAGIC_STRING_INT32_ARRAY_UL:
    {
      GET_ELEMENT (int32_t, src, ret_num);
      break;
    }
    case LIT_MAGIC_STRING_UINT32_ARRAY_UL:
    {
      GET_ELEMENT (uint32_t, src, ret_num);
      break;
    }
    case LIT_MAGIC_STRING_FLOAT32_ARRAY_UL:
    {
      GET_ELEMENT (float, src, ret_num);
      break;
    }
#if CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64
    case LIT_MAGIC_STRING_FLOAT64_ARRAY_UL:
    {
      GET_ELEMENT (double, src, ret_num);
      break;
    }
#endif /* CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64 */
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }

  return ret_num;
} /* get_typedarray_element */

/**
 * set typedarray's element value
 */
static void
set_typedarray_element (lit_utf8_byte_t *dst, /**< the location in the internal arraybuffer */
                        ecma_number_t value, /**< the number value to set */
                        lit_magic_string_id_t class_id) /**< class name of the typedarray */
{
  switch (class_id)
  {
    case LIT_MAGIC_STRING_INT8_ARRAY_UL:
    {
      SET_ELEMENT (int8_t, dst, value);
      break;
    }
    case LIT_MAGIC_STRING_UINT8_ARRAY_UL:
    {
      SET_ELEMENT (uint8_t, dst, value);
      break;
    }
    case LIT_MAGIC_STRING_UINT8_CLAMPED_ARRAY_UL:
    {
      uint8_t clamped;

      if (value > 255)
      {
        clamped = 255;
      }
      else if (value < 0)
      {
        clamped = 0;
      }
      else
      {
        clamped = (uint8_t) value;

        if (clamped + 0.5 < value
            || (clamped + 0.5 == value && clamped % 2 == 1))
        {
          clamped ++;
        }
      }
      *((uint8_t *) dst) = clamped;
      break;
    }
    case LIT_MAGIC_STRING_INT16_ARRAY_UL:
    {
      SET_ELEMENT (int16_t, dst, value);
      break;
    }
    case LIT_MAGIC_STRING_UINT16_ARRAY_UL:
    {
      SET_ELEMENT (uint16_t, dst, value);
      break;
    }
    case LIT_MAGIC_STRING_INT32_ARRAY_UL:
    {
      SET_ELEMENT (int32_t, dst, value);
      break;
    }
    case LIT_MAGIC_STRING_UINT32_ARRAY_UL:
    {
      SET_ELEMENT (uint32_t, dst, value);
      break;
    }
    case LIT_MAGIC_STRING_FLOAT32_ARRAY_UL:
    {
      *((float *) dst) = (float) value;
      break;
    }
#if CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64
    case LIT_MAGIC_STRING_FLOAT64_ARRAY_UL:
    {
      *((double *) dst) = (double) value;
      break;
    }
#endif /* CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64 */
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }
} /* set_typedarray_element */

/**
 * Create a TypedArray object by given array_length
 *
 * See also: ES2015 22.2.1.2.1
 *
 * @return pointer to the new typedarray object
 */
static ecma_object_t *
ecma_typedarray_create_object_with_length (ecma_length_t array_length, /**< length of the typedarray */
                                           ecma_object_t *proto_p, /**< prototype object */
                                           uint8_t element_size_shift, /**< the size shift of the element length */
                                           lit_magic_string_id_t class_id) /**< class name of the typedarray */
{
  ecma_length_t byte_length = array_length << element_size_shift;

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

  return object_p;
} /* !ecma_typedarray_create_object_with_length */

/**
 * Create a TypedArray object by given buffer, offset, and array_length
 *
 * See also: ES2015 22.2.1.5
 *
 * @return pointer to the new typedarray object
 */
static ecma_object_t *
ecma_typedarray_create_object_with_buffer (ecma_object_t *arraybuffer_p, /**< the arraybuffer inside */
                                           ecma_length_t byte_offset, /**< the byte offset of the arraybuffer */
                                           ecma_length_t array_length, /**< length of the typedarray */
                                           ecma_object_t *proto_p, /**< prototype object */
                                           uint8_t element_size_shift, /**< the size shift of the element length */
                                           lit_magic_string_id_t class_id) /**< class name of the typedarray */
{
  ecma_object_t *object_p;
  if (byte_offset == 0 && array_length == ecma_arraybuffer_get_length (arraybuffer_p) >> element_size_shift)
  {
    object_p = ecma_create_object (proto_p,
                                   sizeof (ecma_extended_object_t),
                                   ECMA_OBJECT_TYPE_PSEUDO_ARRAY);

    ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
    ext_object_p->u.pseudo_array.u1.class_id = class_id;
    ext_object_p->u.pseudo_array.type = ECMA_PSEUDO_ARRAY_TYPEDARRAY;
    ext_object_p->u.pseudo_array.extra_info = element_size_shift;
    ext_object_p->u.pseudo_array.u2.arraybuffer = ecma_make_object_value (arraybuffer_p);

    return object_p;
  }

  object_p = ecma_create_object (proto_p,
                                 sizeof (ecma_extended_typedarray_object_t),
                                 ECMA_OBJECT_TYPE_PSEUDO_ARRAY);

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
  ext_object_p->u.pseudo_array.u1.class_id = class_id;
  ext_object_p->u.pseudo_array.type = ECMA_PSEUDO_ARRAY_TYPEDARRAY_WITH_INFO;
  ext_object_p->u.pseudo_array.extra_info = element_size_shift;
  ext_object_p->u.pseudo_array.u2.arraybuffer = ecma_make_object_value (arraybuffer_p);

  ecma_extended_typedarray_object_t *typedarray_info_p = (ecma_extended_typedarray_object_t *) ext_object_p;
  typedarray_info_p->array_length = array_length;
  typedarray_info_p->byte_offset = byte_offset;

  return object_p;
} /* ecma_typedarray_create_object_with_buffer */

/**
 * Create a TypedArray object by given another TypedArray object
 *
 * See also: ES2015 22.2.1.3
 *
 * @return pointer to the new typedarray object
 */
static ecma_object_t *
ecma_typedarray_create_object_with_typedarray (ecma_object_t *typedarray_p, /**< a typedarray object */
                                               ecma_object_t *proto_p, /**< prototype object */
                                               uint8_t element_size_shift, /**< the size shift of the element length */
                                               lit_magic_string_id_t class_id) /**< class name of the typedarray */
{
  ecma_length_t array_length = ecma_typedarray_get_length (typedarray_p);
  ecma_length_t byte_offset = ecma_typedarray_get_offset (typedarray_p);
  ecma_object_t *src_arraybuffer_p = ecma_typedarray_get_arraybuffer (typedarray_p);
  lit_magic_string_id_t src_class_id = ecma_object_get_class_name (typedarray_p);
  ecma_object_t *arraybuffer_p = NULL;

  if (src_class_id == class_id)
  {
    arraybuffer_p = ecma_arraybuffer_new_object_by_clone_arraybuffer (src_arraybuffer_p,
                                                                      byte_offset);
  }
  else
  {
    ecma_length_t byte_length = array_length << element_size_shift;
    arraybuffer_p = ecma_arraybuffer_new_object (byte_length);
    lit_utf8_byte_t *src_buf = ecma_arraybuffer_get_buffer (src_arraybuffer_p);
    lit_utf8_byte_t *dst_buf = ecma_arraybuffer_get_buffer (arraybuffer_p);
    uint8_t src_element_size_shift = ecma_typedarray_get_element_size_shift (typedarray_p);

    for (uint32_t i = 0; i < array_length; i++)
    {
      ecma_number_t tmp = get_typedarray_element (src_buf,
                                                  ecma_object_get_class_name (typedarray_p));
      /** check dst type and set value to dst typedarray */
      set_typedarray_element (dst_buf, tmp, class_id);

      src_buf = src_buf + (1 << src_element_size_shift);
      dst_buf = dst_buf + (1 << element_size_shift);
    }
  }

  ecma_object_t *object_p = ecma_typedarray_create_object_with_buffer (arraybuffer_p,
                                                                       0,
                                                                       array_length,
                                                                       proto_p,
                                                                       element_size_shift,
                                                                       class_id);

  ecma_deref_object (arraybuffer_p);

  return object_p;
} /* ecma_typedarray_create_object_with_typedarray */

/**
 * Create a TypedArray object by transforming from an array-like object
 *
 * See also: ES2015 22.2.2.1
 *
 * @return ecma value
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
  ECMA_TRY_CATCH (arraylike_val,
                  ecma_op_to_object (items_val),
                  ret_value);

  ecma_object_t *arraylike_p = ecma_get_object_from_value (arraylike_val);

  /* 12 */
  ecma_string_t *magic_string_length_p = ecma_new_ecma_length_string ();

  ECMA_TRY_CATCH (len_value,
                  ecma_op_object_get (arraylike_p, magic_string_length_p),
                  ret_value);

  ECMA_OP_TO_NUMBER_TRY_CATCH (len_number, len_value, ret_value);

  uint32_t len = ecma_number_to_uint32 (len_number);

  /* 14 */
  ecma_object_t *new_typedarray_p = ecma_typedarray_create_object_with_length (len,
                                                                               proto_p,
                                                                               element_size_shift,
                                                                               class_id);

  /* 17 */
  ecma_value_t current_index;

  for (uint32_t index = 0; index < len && ecma_is_value_empty (ret_value); index++)
  {
    /* 17.a */
    ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (index);

    /* 17.b */
    ECMA_TRY_CATCH (current_value, ecma_op_object_find (arraylike_p, index_str_p), ret_value);

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
  ecma_deref_ecma_string (magic_string_length_p);
  ECMA_FINALIZE (arraylike_val);

  return ret_value;
} /* ecma_op_typedarray_from */

/**
 * Get the array length of the typedarray object
 *
 * @return the pointer to the internal arraybuffer
 */
ecma_object_t *
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
 * Create a new typedarray object.
 *
 * The struct of the typedarray object
 *   ecma_object_t
 *   extend_part
 *   typedarray_info
 *
 * @return ecma value
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
    ret = ecma_make_object_value (ecma_typedarray_create_object_with_length (0, proto_p, element_size_shift, class_id));
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
      ret = ecma_make_object_value (ecma_typedarray_create_object_with_length (length,
                                                                               proto_p,
                                                                               element_size_shift,
                                                                               class_id));
    }

    ECMA_OP_TO_NUMBER_FINALIZE (num);
  }
  else if (ecma_is_value_object (arguments_list_p[0]))
  {
    if (ecma_is_typedarray (arguments_list_p[0]))
    {
      /* 22.2.1.3 */
      ecma_object_t *typedarray_p = ecma_get_object_from_value (arguments_list_p[0]);
      ret = ecma_make_object_value (ecma_typedarray_create_object_with_typedarray (typedarray_p,
                                                                                   proto_p,
                                                                                   element_size_shift,
                                                                                   class_id));
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
          new_byte_length = (ecma_length_t) new_length << element_size_shift;

          if (new_byte_length + offset > buf_byte_length)
          {
            ret = ecma_raise_range_error (ECMA_ERR_MSG ("Invalid length."));
          }

          ECMA_OP_TO_NUMBER_FINALIZE (num3);
        }

        if (ecma_is_value_empty (ret))
        {
          ecma_length_t array_length = new_byte_length >> element_size_shift;
          ret = ecma_make_object_value (ecma_typedarray_create_object_with_buffer (arraybuffer_p,
                                                                                   offset,
                                                                                   array_length,
                                                                                   proto_p,
                                                                                   element_size_shift,
                                                                                   class_id));
        }
      }

      ECMA_OP_TO_NUMBER_FINALIZE (num2);
    }
    else
    {
      /* 22.2.1.4 */
      ecma_value_t undef_val = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
      ret = ecma_op_typedarray_from (arguments_list_p[0],
                                     undef_val,
                                     undef_val,
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
 * @return True: it is a typedarray object
 *         False: it is not object or it is not a typedarray
 */
bool
ecma_is_typedarray (ecma_value_t target) /**< the target need to be checked */
{
  if (!ecma_is_value_object (target))
  {
    return false;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (target);

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
 *         if failed, return undefined value
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
  ecma_number_t value = get_typedarray_element (target_p, class_id);

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

  ecma_value_t error_val = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_OP_TO_NUMBER_TRY_CATCH (value_num, value, error_val);

  ecma_object_t *arraybuffer_p = ecma_typedarray_get_arraybuffer (obj_p);
  ecma_length_t offset = ecma_typedarray_get_offset (obj_p);
  uint8_t shift = ecma_typedarray_get_element_size_shift (obj_p);
  ecma_length_t byte_pos = (index << shift) + offset;
  lit_magic_string_id_t class_id = ecma_object_get_class_name (obj_p);
  lit_utf8_byte_t *target_p = ecma_arraybuffer_get_buffer (arraybuffer_p) + byte_pos;
  set_typedarray_element (target_p, value_num, class_id);

  ECMA_OP_TO_NUMBER_FINALIZE (value_num);

  if (ecma_is_value_empty (error_val))
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

  ecma_object_t *new_obj_p = ecma_typedarray_create_object_with_length (array_length,
                                                                        proto_p,
                                                                        element_size_shift,
                                                                        class_id);

  ecma_deref_object (proto_p);

  return ecma_make_object_value (new_obj_p);
} /* ecma_op_create_typedarray_with_type_and_length */

/**
 * @}
 * @}
 */
#endif /* !CONFIG_DISABLE_TYPEDARRAY_BUILTIN */
