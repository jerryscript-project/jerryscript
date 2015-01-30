/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-string-object.h"
#include "ecma-try-catch-macro.h"
#include "globals.h"

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
static void
ecma_builtin_object_prototype_object_to_string (ecma_completion_value_t &ret_value, /**< out: completion value */
                                                const ecma_value_t& this_arg) /**< this argument */
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
    ecma_completion_value_t this_to_obj_completion;
    ecma_op_to_object (this_to_obj_completion, this_arg);

    if (!ecma_is_completion_value_normal (this_to_obj_completion))
    {
      ret_value = this_to_obj_completion;
      return;
    }

    ecma_value_t obj_this_value;
    ecma_get_completion_value_value (obj_this_value, this_to_obj_completion);

    JERRY_ASSERT (ecma_is_value_object (obj_this_value));
    ecma_property_t *class_prop_p = ecma_get_internal_property (ecma_get_object_from_value (obj_this_value),
                                                                ECMA_INTERNAL_PROPERTY_CLASS);
    type_string = (ecma_magic_string_id_t) class_prop_p->u.internal_property.value;

    ecma_free_completion_value (this_to_obj_completion);
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

  ecma_make_normal_completion_value (ret_value, ecma_value_t (ret_string_p));
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
static void
ecma_builtin_object_prototype_object_value_of (ecma_completion_value_t &ret_value, /**< out: completion value */
                                               const ecma_value_t& this_arg) /**< this argument */
{
  ecma_op_to_object (ret_value, this_arg);
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
static void
ecma_builtin_object_prototype_object_to_locale_string (ecma_completion_value_t &ret_value, /**< out: completion value */
                                                       const ecma_value_t& this_arg) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (ret_value, this_arg);
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
static void
ecma_builtin_object_prototype_object_has_own_property (ecma_completion_value_t &ret_value, /**< out: completion value */
                                                       const ecma_value_t& this_arg, /**< this argument */
                                                       const ecma_value_t& arg) /**< first argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (ret_value, this_arg, arg);
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
static void
ecma_builtin_object_prototype_object_is_prototype_of (ecma_completion_value_t &ret_value, /**< out: completion value */
                                                      const ecma_value_t& this_arg, /**< this argument */
                                                      const ecma_value_t& arg) /**< routine's first argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (ret_value, this_arg, arg);
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
static void
ecma_builtin_object_prototype_object_property_is_enumerable (ecma_completion_value_t &ret_value, /**< out: completion
                                                                                                  *        value */
                                                             const ecma_value_t& this_arg, /**< this argument */
                                                             const ecma_value_t& arg) /**< routine's first argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (ret_value, this_arg, arg);
} /* ecma_builtin_object_prototype_object_property_is_enumerable */

/**
 * @}
 * @}
 * @}
 */
