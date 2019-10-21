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

#include "ecma-arraybuffer-object.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-helpers.h"
#include "ecma-dataview-object.h"
#include "ecma-typedarray-object.h"
#include "ecma-objects.h"

#if ENABLED (JERRY_ES2015_BUILTIN_DATAVIEW)

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmadataviewobject ECMA builtin DataView helper functions
 * @{
 */

/**
 * Handle calling [[Construct]] of built-in DataView like objects
 *
 * See also:
 *          ECMA-262 v6, 24.2.2.1
 *
 * @return created DataView object as an ecma-value - if success
 *         raised error - otherwise
 */
ecma_value_t
ecma_op_dataview_create (const ecma_value_t *arguments_list_p, /**< arguments list */
                         ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  ecma_value_t buffer = arguments_list_len > 0 ? arguments_list_p[0] : ECMA_VALUE_UNDEFINED;

  /* 2. */
  if (!ecma_is_value_object (buffer))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument buffer is not an object."));
  }

  ecma_object_t *buffer_p = ecma_get_object_from_value (buffer);

  /* 3. */
  if (!ecma_object_class_is (buffer_p, LIT_MAGIC_STRING_ARRAY_BUFFER_UL))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument buffer is not an arraybuffer."));
  }

  /* 4 - 6. */
  int32_t offset = 0;

  if (arguments_list_len > 1)
  {
    ecma_number_t number_offset;
    ecma_value_t number_offset_value = ecma_get_number (arguments_list_p[1], &number_offset);

    if (ECMA_IS_VALUE_ERROR (number_offset_value))
    {
      return number_offset_value;
    }

    offset = ecma_number_to_int32 (number_offset);

    /* 7. */
    if (number_offset != offset || offset < 0)
    {
      return ecma_raise_range_error (ECMA_ERR_MSG ("Start offset is outside the bounds of the buffer."));
    }
  }

  /* 8. */
  if (ecma_arraybuffer_is_detached (buffer_p))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("ArrayBuffer has been detached."));
  }

  /* 9. */
  ecma_length_t buffer_byte_length = ecma_arraybuffer_get_length (buffer_p);

  /* 10. */
  if ((ecma_length_t) offset > buffer_byte_length)
  {
    return ecma_raise_range_error (ECMA_ERR_MSG ("Start offset is outside the bounds of the buffer."));
  }

  /* 11 - 12. */
  uint32_t viewByteLength;
  if (arguments_list_len > 2)
  {
    /* 12.a */
    ecma_value_t byte_length_value = ecma_op_to_length (arguments_list_p[2], &viewByteLength);

    /* 12.b */
    if (ECMA_IS_VALUE_ERROR (byte_length_value))
    {
      return byte_length_value;
    }

    /* 12.c */
    if ((ecma_number_t) offset + viewByteLength > buffer_byte_length)
    {
      return ecma_raise_range_error (ECMA_ERR_MSG ("Start offset is outside the bounds of the buffer."));
    }
  }
  else
  {
    /* 11.a */
    viewByteLength = (uint32_t) (buffer_byte_length - (ecma_length_t) offset);
  }

  /* 13. */
  ecma_object_t *object_p = ecma_create_object (ecma_builtin_get (ECMA_BUILTIN_ID_DATAVIEW_PROTOTYPE),
                                                sizeof (ecma_dataview_object_t),
                                                ECMA_OBJECT_TYPE_CLASS);

  ecma_dataview_object_t *dataview_obj_p = (ecma_dataview_object_t *) object_p;
  dataview_obj_p->header.u.class_prop.class_id = LIT_MAGIC_STRING_DATAVIEW_UL;
  dataview_obj_p->header.u.class_prop.u.length = viewByteLength;
  dataview_obj_p->buffer_p = buffer_p;
  dataview_obj_p->byte_offset = (uint32_t) offset;

  return ecma_make_object_value (object_p);
} /* ecma_op_dataview_create */

/**
 * Get the DataView object pointer
 *
 * Note:
 *   If the function returns with NULL, the error object has
 *   already set, and the caller must return with ECMA_VALUE_ERROR
 *
 * @return pointer to the dataView if this_arg is a valid dataView object
 *         NULL otherwise
 */
ecma_dataview_object_t *
ecma_op_dataview_get_object (ecma_value_t this_arg) /**< this argument */
{
  if (ecma_is_value_object (this_arg))
  {
    ecma_dataview_object_t *dataview_object_p = (ecma_dataview_object_t *) ecma_get_object_from_value (this_arg);

    if (ecma_get_object_type (&dataview_object_p->header.object) == ECMA_OBJECT_TYPE_CLASS
        && dataview_object_p->header.u.class_prop.class_id == LIT_MAGIC_STRING_DATAVIEW_UL)
    {
      return dataview_object_p;
    }
  }

  ecma_raise_type_error (ECMA_ERR_MSG ("Expected a DataView object."));
  return NULL;
} /* ecma_op_dataview_get_object */

/**
 * Helper union to specify the system's endiannes
 */
typedef union
{
  uint32_t number; /**< for write numeric data */
  char data[sizeof (uint32_t)]; /**< for read numeric data */
} ecma_dataview_endiannes_check_t;

/**
 * Helper function to check the current system endiannes
 *
 * @return true - if the current system has little endian byteorder
 *         false - otherwise
 */
static bool
ecma_dataview_check_little_endian (void)
{
  ecma_dataview_endiannes_check_t checker;
  checker.number = 0x01;

  return checker.data[0] == 0x01;
} /* ecma_dataview_check_little_endian */

/**
 * Helper function for swap bytes if the system's endiannes
 * does not match with the requested endiannes.
 */
static void
ecma_dataview_swap_order (bool system_is_little_endian, /**< true - if the system has little endian byteorder
                                                         *   false - otherwise */
                          bool is_little_endian, /**< true - if little endian byteorder is requested
                                                  *   false - otherwise */
                          uint32_t element_size, /**< element size byte according to the Table 49.*/
                          lit_utf8_byte_t *block_p) /**< data block */
{
  if (system_is_little_endian ^ is_little_endian)
  {
    for (uint32_t i = 0; i < element_size / 2; i++)
    {
      lit_utf8_byte_t tmp = block_p[i];
      block_p[i] = block_p[element_size - i -  1];
      block_p[element_size - i - 1] = tmp;
    }
  }
} /* ecma_dataview_swap_order */

/**
 * GetViewValue and SetViewValue abstact operation
 *
 * See also:
 *          ECMA-262 v6, 24.2.1.1
 *          ECMA-262 v6, 24.2.1.2
 *
 * @return ecma value
 */
ecma_value_t
ecma_op_dataview_get_set_view_value (ecma_value_t view, /**< the operation's 'view' argument */
                                     ecma_value_t request_index, /**< the operation's 'requestIndex' argument */
                                     ecma_value_t is_little_endian_value, /**< the operation's
                                                                           *   'isLittleEndian' argument */
                                     ecma_value_t value_to_set, /**< the operation's 'value' argument */
                                     ecma_typedarray_type_t id) /**< the operation's 'type' argument */
{
  /* 1 - 2. */
  ecma_dataview_object_t *view_p = ecma_op_dataview_get_object (view);

  if (JERRY_UNLIKELY (view_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  /* 3 - 5. */
  ecma_number_t number_index;
  ecma_value_t number_index_value = ecma_get_number (request_index, &number_index);

  if (ECMA_IS_VALUE_ERROR (number_index_value))
  {
    return number_index_value;
  }

  int32_t get_index = ecma_number_to_int32 (number_index);

  /* 6. */
  if (number_index != get_index || get_index < 0)
  {
    return ecma_raise_range_error (ECMA_ERR_MSG ("Start offset is outside the bounds of the buffer."));
  }

  /* 7. */
  bool is_little_endian = ecma_op_to_boolean (is_little_endian_value);

  /* 8. TODO: Throw TypeError, when Detached ArrayBuffer will be supported. */

  /* 9. */
  ecma_object_t *buffer_p = view_p->buffer_p;
  JERRY_ASSERT (ecma_object_class_is (buffer_p, LIT_MAGIC_STRING_ARRAY_BUFFER_UL));
  if (ecma_arraybuffer_is_detached (buffer_p))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("ArrayBuffer has been detached."));
  }

  /* 10. */
  uint32_t view_offset = view_p->byte_offset;

  /* 11. */
  uint32_t view_size = view_p->header.u.class_prop.u.length;

  /* 12. */
  uint8_t element_size = (uint8_t) (1 << (ecma_typedarray_helper_get_shift_size (id)));

  /* 13. */
  if ((uint32_t) get_index + element_size > view_size)
  {
    return ecma_raise_range_error (ECMA_ERR_MSG ("Start offset is outside the bounds of the buffer."));
  }

  /* 14. */
  uint32_t buffer_index = (uint32_t) get_index + view_offset;
  lit_utf8_byte_t *block_p = ecma_arraybuffer_get_buffer (buffer_p) + buffer_index;

  bool system_is_little_endian = ecma_dataview_check_little_endian ();

  if (ecma_is_value_empty (value_to_set))
  {
    JERRY_VLA (lit_utf8_byte_t, swap_block_p, element_size);
    memcpy (swap_block_p, block_p, element_size * sizeof (lit_utf8_byte_t));
    ecma_dataview_swap_order (system_is_little_endian, is_little_endian, element_size, swap_block_p);
    return ecma_make_number_value (ecma_get_typedarray_element (swap_block_p, id));
  }

  if (ecma_is_value_number (value_to_set))
  {
    ecma_set_typedarray_element (block_p, ecma_get_number_from_value (value_to_set), id);
    ecma_dataview_swap_order (system_is_little_endian, is_little_endian, element_size, block_p);
  }

  return ECMA_VALUE_UNDEFINED;
} /* ecma_op_dataview_get_set_view_value */

/**
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_ES2015_BUILTIN_DATAVIEW */
