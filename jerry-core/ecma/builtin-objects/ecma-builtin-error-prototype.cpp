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
#include "jrt.h"
#include "lit-magic-strings.h"

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ERROR_BUILTINS

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-error-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID error_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup errorprototype ECMA Error.prototype object built-in
 * @{
 */

/**
 * The Error.prototype object's 'toString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.11.4.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_error_prototype_object_to_string (ecma_value_t this_arg) /**< this argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  // 2.
  if (!ecma_is_value_object (this_arg))
  {
    ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (this_arg);
    ecma_string_t *name_magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_NAME);

    ECMA_TRY_CATCH (name_get_ret_value,
                    ecma_op_object_get (obj_p, name_magic_string_p),
                    ret_value);

    ecma_completion_value_t name_to_str_completion;

    if (ecma_is_value_undefined (name_get_ret_value))
    {
      ecma_string_t *error_magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_ERROR_UL);

      name_to_str_completion = ecma_make_normal_completion_value (ecma_make_string_value (error_magic_string_p));
    }
    else
    {
      name_to_str_completion = ecma_op_to_string (name_get_ret_value);
    }

    if (unlikely (!ecma_is_completion_value_normal (name_to_str_completion)))
    {
      ret_value = ecma_copy_completion_value (name_to_str_completion);
    }
    else
    {
      ecma_string_t *message_magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_MESSAGE);

      ECMA_TRY_CATCH (msg_get_ret_value,
                      ecma_op_object_get (obj_p, message_magic_string_p),
                      ret_value);

      ecma_completion_value_t msg_to_str_completion;

      if (ecma_is_value_undefined (msg_get_ret_value))
      {
        ecma_string_t *empty_magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);

        msg_to_str_completion = ecma_make_normal_completion_value (ecma_make_string_value (empty_magic_string_p));
      }
      else
      {
        msg_to_str_completion = ecma_op_to_string (msg_get_ret_value);
      }

      if (unlikely (!ecma_is_completion_value_normal (msg_to_str_completion)))
      {
        ret_value = ecma_copy_completion_value (msg_to_str_completion);
      }
      else
      {
        ecma_string_t *name_string_p = ecma_get_string_from_completion_value (name_to_str_completion);
        ecma_string_t *msg_string_p = ecma_get_string_from_completion_value (msg_to_str_completion);

        ecma_string_t *ret_str_p;

        if (ecma_string_get_length (name_string_p) == 0)
        {
          ret_str_p = ecma_copy_or_ref_ecma_string (msg_string_p);
        }
        else if (ecma_string_get_length (msg_string_p) == 0)
        {
          ret_str_p = ecma_copy_or_ref_ecma_string (name_string_p);
        }
        else
        {
          const lit_utf8_size_t size = (ecma_string_get_size (name_string_p) +
                                        ecma_string_get_size (msg_string_p) +
                                        lit_get_magic_string_size (LIT_MAGIC_STRING_COLON_CHAR) +
                                        lit_get_magic_string_size (LIT_MAGIC_STRING_SPACE_CHAR));

          const ssize_t buffer_size = (ssize_t) size;
          ssize_t buffer_size_left = buffer_size;

          MEM_DEFINE_LOCAL_ARRAY (ret_str_buffer, buffer_size, lit_utf8_byte_t);
          lit_utf8_byte_t *ret_str_buffer_p = ret_str_buffer;

          ssize_t bytes = ecma_string_to_utf8_string (name_string_p, ret_str_buffer_p, buffer_size_left);
          JERRY_ASSERT (bytes >= 0 && buffer_size_left - bytes >= 0);

          buffer_size_left -= bytes;
          ret_str_buffer_p = ret_str_buffer + buffer_size - buffer_size_left;

          ret_str_buffer_p = lit_copy_magic_string_to_buffer (LIT_MAGIC_STRING_COLON_CHAR,
                                                              ret_str_buffer_p,
                                                              buffer_size_left);
          buffer_size_left = buffer_size - (ret_str_buffer_p - ret_str_buffer);
          JERRY_ASSERT (buffer_size_left >= 0);

          ret_str_buffer_p = lit_copy_magic_string_to_buffer (LIT_MAGIC_STRING_SPACE_CHAR,
                                                              ret_str_buffer_p,
                                                              buffer_size_left);
          buffer_size_left = buffer_size - (ret_str_buffer_p - ret_str_buffer);
          JERRY_ASSERT (buffer_size_left >= 0);

          bytes = ecma_string_to_utf8_string (msg_string_p, ret_str_buffer_p, buffer_size_left);
          JERRY_ASSERT (bytes >= 0 && buffer_size_left - bytes >= 0);

          buffer_size_left -= bytes;
          JERRY_ASSERT (buffer_size_left >= 0);

          ret_str_p = ecma_new_ecma_string_from_utf8 (ret_str_buffer,
                                                      (jerry_api_size_t) (buffer_size - buffer_size_left));

          MEM_FINALIZE_LOCAL_ARRAY (ret_str_buffer);
        }

        ret_value = ecma_make_normal_completion_value (ecma_make_string_value (ret_str_p));
      }

      ecma_free_completion_value (msg_to_str_completion);

      ECMA_FINALIZE (msg_get_ret_value);

      ecma_deref_ecma_string (message_magic_string_p);
    }

    ecma_free_completion_value (name_to_str_completion);

    ECMA_FINALIZE (name_get_ret_value);

    ecma_deref_ecma_string (name_magic_string_p);
  }

  return ret_value;
} /* ecma_builtin_error_prototype_object_to_string */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ERROR_BUILTINS */
