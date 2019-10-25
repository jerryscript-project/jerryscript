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
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-string-object.h"
#if ENABLED (JERRY_ES2015)
#include "ecma-symbol-object.h"
#endif /* ENABLED (JERRY_ES2015) */
#include "ecma-try-catch-macro.h"
#include "jrt.h"

#if ENABLED (JERRY_BUILTIN_STRING)

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
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_object_from_char_code (ecma_value_t this_arg, /**< 'this' argument */
                                           const ecma_value_t args[], /**< arguments list */
                                           ecma_length_t args_number) /**< number of arguments */
{
  JERRY_UNUSED (this_arg);

  if (args_number == 0)
  {
    return ecma_make_magic_string_value (LIT_MAGIC_STRING__EMPTY);
  }

  ecma_value_t ret_value = ECMA_VALUE_EMPTY;
  ecma_string_t *ret_string_p = NULL;
  lit_utf8_size_t utf8_buf_size = args_number * LIT_CESU8_MAX_BYTES_IN_CODE_UNIT;

  JMEM_DEFINE_LOCAL_ARRAY (utf8_buf_p,
                           utf8_buf_size,
                           lit_utf8_byte_t);

  lit_utf8_size_t utf8_buf_used = 0;

  for (ecma_length_t arg_index = 0;
       arg_index < args_number && ecma_is_value_empty (ret_value);
       arg_index++)
  {
    ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, args[arg_index], ret_value);

    uint32_t uint32_char_code = ecma_number_to_uint32 (arg_num);
    ecma_char_t code_unit = (uint16_t) uint32_char_code;

    JERRY_ASSERT (utf8_buf_used <= utf8_buf_size - LIT_UTF8_MAX_BYTES_IN_CODE_UNIT);
    utf8_buf_used += lit_code_unit_to_utf8 (code_unit, utf8_buf_p + utf8_buf_used);
    JERRY_ASSERT (utf8_buf_used <= utf8_buf_size);

    ECMA_OP_TO_NUMBER_FINALIZE (arg_num);
  }

  if (ecma_is_value_empty (ret_value))
  {
    ret_string_p = ecma_new_ecma_string_from_utf8 (utf8_buf_p, utf8_buf_used);
  }

  JMEM_FINALIZE_LOCAL_ARRAY (utf8_buf_p);

  if (ecma_is_value_empty (ret_value))
  {
    ret_value = ecma_make_string_value (ret_string_p);
  }

  return ret_value;
} /* ecma_builtin_string_object_from_char_code */

/**
 * Handle calling [[Call]] of built-in String object
 *
 * See also:
 *          ECMA-262 v6, 21.1.1.1
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_string_dispatch_call (const ecma_value_t *arguments_list_p, /**< arguments list */
                                   ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  /* 1. */
  if (arguments_list_len == 0)
  {
    ret_value = ecma_make_magic_string_value (LIT_MAGIC_STRING__EMPTY);
  }
#if ENABLED (JERRY_ES2015)
  /* 2.a */
  else if (ecma_is_value_symbol (arguments_list_p[0]))
  {
    ret_value = ecma_get_symbol_descriptive_string (arguments_list_p[0]);
  }
#endif /* ENABLED (JERRY_ES2015) */
  /* 2.b */
  else
  {
    ecma_string_t *str_p = ecma_op_to_string (arguments_list_p[0]);
    if (JERRY_UNLIKELY (str_p == NULL))
    {
      return ECMA_VALUE_ERROR;
    }

    ret_value = ecma_make_string_value (str_p);
  }

  return ret_value;
} /* ecma_builtin_string_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in String object
 *
 * @return ecma value
 */
ecma_value_t
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

#endif /* ENABLED (JERRY_BUILTIN_STRING) */
