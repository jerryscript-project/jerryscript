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

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_STRING_BUILTIN

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-string.inc.h"
#define BUILTIN_UNDERSCORED_ID string
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup string ECMA String object built-in
 * @{
 */

/**
 * The String object's 'fromCharCode' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.3.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_object_from_char_code (const ecma_value_t& this_arg __unused, /**< 'this' argument */
                                           const ecma_value_t args[], /**< arguments list */
                                           ecma_length_t args_number) /**< number of arguments */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  size_t zt_str_buffer_size = sizeof (ecma_char_t) * (args_number + 1u);

  ecma_char_t *ret_zt_str_p = (ecma_char_t*) mem_heap_alloc_block (zt_str_buffer_size,
                                                                   MEM_HEAP_ALLOC_SHORT_TERM);
  ret_zt_str_p [args_number] = ECMA_CHAR_NULL;

  for (ecma_length_t arg_index = 0;
       arg_index < args_number;
       arg_index++)
  {
    ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, args[arg_index], ret_value);

    uint32_t uint32_char_code = ecma_number_to_uint32 (arg_num);
    uint16_t uint16_char_code = (uint16_t) uint32_char_code;

#if CONFIG_ECMA_CHAR_ENCODING == CONFIG_ECMA_CHAR_ASCII
    if ((uint16_char_code >> JERRY_BITSINBYTE) != 0)
    {
      ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
    }
    else
    {
      ret_zt_str_p [arg_index] = (ecma_char_t) uint16_char_code;
    }
#elif CONFIG_ECMA_CHAR_ENCODING == CONFIG_ECMA_CHAR_UTF16
    ret_zt_str_p [arg_index] = (ecma_char_t) uint16_char_code;
#endif /* CONFIG_ECMA_CHAR_ENCODING == CONFIG_ECMA_CHAR_UTF16 */

    ECMA_OP_TO_NUMBER_FINALIZE (arg_num);

    if (ecma_is_completion_value_throw (ret_value))
    {
      mem_heap_free_block (ret_zt_str_p);

      return ret_value;
    }

    JERRY_ASSERT (ecma_is_completion_value_empty (ret_value));
  }

  ecma_string_t *ret_str_p = ecma_new_ecma_string (ret_zt_str_p);

  mem_heap_free_block (ret_zt_str_p);

  return ecma_make_normal_completion_value (ecma_make_string_value (ret_str_p));
} /* ecma_builtin_string_object_from_char_code */

/**
 * Handle calling [[Call]] of built-in String object
 *
 * @return completion-value
 */
ecma_completion_value_t
ecma_builtin_string_dispatch_call (const ecma_value_t *arguments_list_p, /**< arguments list */
                                   ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  ecma_completion_value_t ret_value;

  if (arguments_list_len == 0)
  {
    ecma_string_t *str_p = ecma_new_ecma_string_from_magic_string_id (ECMA_MAGIC_STRING__EMPTY);
    ecma_value_t str_value = ecma_make_string_value (str_p);

    ret_value = ecma_make_normal_completion_value (str_value);
  }
  else
  {
    ret_value = ecma_op_to_string (arguments_list_p [0]);
  }

  return ret_value;
} /* ecma_builtin_string_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in String object
 *
 * @return completion-value
 */
ecma_completion_value_t
ecma_builtin_string_dispatch_construct (const ecma_value_t *arguments_list_p, /**< arguments list */
                                        ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  return ecma_op_create_string_object (arguments_list_p, arguments_list_len);
} /* ecma_builtin_string_dispatch_construct */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_STRING_BUILTIN */
