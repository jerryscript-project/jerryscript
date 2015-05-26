/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
 * Copyright 2015 University of Szeged.
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
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-string-object.h"
#include "ecma-try-catch-macro.h"
#include "jrt.h"

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-object-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID object_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup objectprototype ECMA Object.prototype object built-in
 * @{
 */

/**
 * The Object.prototype object's 'toString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.4.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_object_prototype_object_to_string (ecma_value_t this_arg) /**< this argument */
{
  ecma_magic_string_id_t type_string;

  if (ecma_is_value_undefined (this_arg))
  {
    type_string = ECMA_MAGIC_STRING_UNDEFINED_UL;
  }
  else if (ecma_is_value_null (this_arg))
  {
    type_string = ECMA_MAGIC_STRING_NULL_UL;
  }
  else
  {
    ecma_completion_value_t obj_this = ecma_op_to_object (this_arg);

    if (!ecma_is_completion_value_normal (obj_this))
    {
      return obj_this;
    }

    JERRY_ASSERT (ecma_is_value_object (ecma_get_completion_value_value (obj_this)));

    ecma_object_t *obj_p = ecma_get_object_from_completion_value (obj_this);

    ecma_property_t *class_prop_p = ecma_get_internal_property (obj_p,
                                                                ECMA_INTERNAL_PROPERTY_CLASS);
    type_string = (ecma_magic_string_id_t) class_prop_p->u.internal_property.value;

    ecma_free_completion_value (obj_this);
  }

  ecma_string_t *ret_string_p;

  /* Building string "[object #type#]" where type is 'Undefined',
     'Null' or one of possible object's classes.
     The string with null character is maximum 19 characters long. */
  const ssize_t buffer_size = 19;
  MEM_DEFINE_LOCAL_ARRAY (str_buffer, buffer_size, ecma_char_t);

  const ecma_char_t *left_square_zt_str_p = ecma_get_magic_string_zt (ECMA_MAGIC_STRING_LEFT_SQUARE_CHAR);
  const ecma_char_t *object_zt_str_p = ecma_get_magic_string_zt (ECMA_MAGIC_STRING_OBJECT);
  const ecma_char_t *space_zt_str_p = ecma_get_magic_string_zt (ECMA_MAGIC_STRING_SPACE_CHAR);
  const ecma_char_t *type_name_zt_str_p = ecma_get_magic_string_zt (type_string);
  const ecma_char_t *right_square_zt_str_p = ecma_get_magic_string_zt (ECMA_MAGIC_STRING_RIGHT_SQUARE_CHAR);

  ecma_char_t *buffer_ptr = str_buffer;
  ssize_t buffer_size_left = buffer_size;
  buffer_ptr = ecma_copy_zt_string_to_buffer (left_square_zt_str_p,
                                              buffer_ptr,
                                              buffer_size_left);
  buffer_size_left = buffer_size - (buffer_ptr - str_buffer) * (ssize_t) sizeof (ecma_char_t);
  buffer_ptr = ecma_copy_zt_string_to_buffer (object_zt_str_p,
                                              buffer_ptr,
                                              buffer_size_left);
  buffer_size_left = buffer_size - (buffer_ptr - str_buffer) * (ssize_t) sizeof (ecma_char_t);
  buffer_ptr = ecma_copy_zt_string_to_buffer (space_zt_str_p,
                                              buffer_ptr,
                                              buffer_size_left);
  buffer_size_left = buffer_size - (buffer_ptr - str_buffer) * (ssize_t) sizeof (ecma_char_t);
  buffer_ptr = ecma_copy_zt_string_to_buffer (type_name_zt_str_p,
                                              buffer_ptr,
                                              buffer_size_left);
  buffer_size_left = buffer_size - (buffer_ptr - str_buffer) * (ssize_t) sizeof (ecma_char_t);
  buffer_ptr = ecma_copy_zt_string_to_buffer (right_square_zt_str_p,
                                              buffer_ptr,
                                              buffer_size_left);
  buffer_size_left = buffer_size - (buffer_ptr - str_buffer) * (ssize_t) sizeof (ecma_char_t);

  JERRY_ASSERT (buffer_size_left >= 0);

  ret_string_p = ecma_new_ecma_string (str_buffer);

  MEM_FINALIZE_LOCAL_ARRAY (str_buffer);

  return ecma_make_normal_completion_value (ecma_make_string_value (ret_string_p));
} /* ecma_builtin_object_prototype_object_to_string */

/**
 * The Object.prototype object's 'valueOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.4.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_object_prototype_object_value_of (ecma_value_t this_arg) /**< this argument */
{
  return ecma_op_to_object (this_arg);
} /* ecma_builtin_object_prototype_object_value_of */

/**
 * The Object.prototype object's 'toLocaleString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.4.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_object_prototype_object_to_locale_string (ecma_value_t this_arg) /**< this argument */
{
  ecma_completion_value_t return_value = ecma_make_empty_completion_value ();
  /* 1. */
  ECMA_TRY_CATCH (obj_val,
                  ecma_op_to_object (this_arg),
                  return_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_val);
  ecma_string_t *to_string_magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_TO_STRING_UL);

  /* 2. */
  ECMA_TRY_CATCH (to_string_val,
                  ecma_op_object_get (obj_p, to_string_magic_string_p),
                  return_value);

  /* 3. */
  if (!ecma_op_is_callable (to_string_val))
  {
    return_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    /* 4. */
    ecma_object_t *to_string_func_obj_p = ecma_get_object_from_value (to_string_val);
    return_value = ecma_op_function_call (to_string_func_obj_p, this_arg, NULL, 0);
  }
  ECMA_FINALIZE (to_string_val);

  ecma_deref_ecma_string (to_string_magic_string_p);

  ECMA_FINALIZE (obj_val);

  return return_value;
} /* ecma_builtin_object_prototype_object_to_locale_string */

/**
 * The Object.prototype object's 'hasOwnProperty' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.4.5
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_object_prototype_object_has_own_property (ecma_value_t this_arg, /**< this argument */
                                                       ecma_value_t arg) /**< first argument */
{
  ecma_completion_value_t return_value = ecma_make_empty_completion_value ();

  /* 1. */
  ECMA_TRY_CATCH (to_string_val,
                  ecma_op_to_string (arg),
                  return_value);

  /* 2. */
  ECMA_TRY_CATCH (obj_val,
                  ecma_op_to_object (this_arg),
                  return_value);

  ecma_string_t *property_name_string_p = ecma_get_string_from_value (to_string_val);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_val);

  /* 3. */
  ecma_property_t *property_p = ecma_op_object_get_own_property (obj_p, property_name_string_p);

  if (property_p != NULL)
  {
    return_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
  }
  else
  {
    return_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE);
  }
  ECMA_FINALIZE (obj_val);

  ECMA_FINALIZE (to_string_val);

  return return_value;
} /* ecma_builtin_object_prototype_object_has_own_property */

/**
 * The Object.prototype object's 'isPrototypeOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.4.6
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_object_prototype_object_is_prototype_of (ecma_value_t this_arg, /**< this argument */
                                                      ecma_value_t arg) /**< routine's first argument */
{
  /* 1. Is the argument an object? */
  if (!ecma_is_value_object (arg))
  {
    return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE);
  }

  ecma_completion_value_t return_value = ecma_make_empty_completion_value ();

  /* 2. ToObject(this) */
  ECMA_TRY_CATCH (obj_value,
                  ecma_op_to_object (this_arg),
                  return_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_value);

  /* 3. Compare prototype to object */
  ECMA_TRY_CATCH (v_obj_value,
                  ecma_op_to_object (arg),
                  return_value);

  ecma_object_t *v_obj_p = ecma_get_object_from_value (v_obj_value);

  bool is_prototype_of = ecma_op_object_is_prototype_of (obj_p, v_obj_p);
  return_value = ecma_make_simple_completion_value (is_prototype_of
                                                    ? ECMA_SIMPLE_VALUE_TRUE
                                                    : ECMA_SIMPLE_VALUE_FALSE);
  ECMA_FINALIZE (v_obj_value);

  ECMA_FINALIZE (obj_value);

  return return_value;
} /* ecma_builtin_object_prototype_object_is_prototype_of */

/**
 * The Object.prototype object's 'propertyIsEnumerable' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.4.7
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_object_prototype_object_property_is_enumerable (ecma_value_t this_arg, /**< this argument */
                                                             ecma_value_t arg) /**< routine's first argument */
{
  ecma_completion_value_t return_value = ecma_make_empty_completion_value ();

  /* 1. */
  ECMA_TRY_CATCH (to_string_val,
                  ecma_op_to_string (arg),
                  return_value);

  /* 2. */
  ECMA_TRY_CATCH (obj_val,
                  ecma_op_to_object (this_arg),
                  return_value);

  ecma_string_t *property_name_string_p = ecma_get_string_from_value (to_string_val);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_val);

  /* 3. */
  ecma_property_t *property_p = ecma_op_object_get_own_property (obj_p, property_name_string_p);

  /* 4. */
  if (property_p != NULL)
  {
    bool is_enumerable = ecma_is_property_enumerable (property_p);

    return_value = ecma_make_simple_completion_value (is_enumerable
                                                      ? ECMA_SIMPLE_VALUE_TRUE
                                                      : ECMA_SIMPLE_VALUE_FALSE);
  }
  else
  {
    return_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE);
  }

  ECMA_FINALIZE (obj_val);

  ECMA_FINALIZE (to_string_val);

  return return_value;
} /* ecma_builtin_object_prototype_object_property_is_enumerable */

/**
 * @}
 * @}
 * @}
 */
