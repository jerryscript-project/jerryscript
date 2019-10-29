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

#include "ecma-array-object.h"
#include "ecma-helpers.h"
#include "jcontext.h"
#include "vm.h"

/**
 * Check whether currently executed code is strict mode code
 *
 * @return true - current code is executed in strict mode,
 *         false - otherwise
 */
bool
vm_is_strict_mode (void)
{
  JERRY_ASSERT (JERRY_CONTEXT (vm_top_context_p) != NULL);

  return JERRY_CONTEXT (vm_top_context_p)->bytecode_header_p->status_flags & CBC_CODE_FLAGS_STRICT_MODE;
} /* vm_is_strict_mode */

/**
 * Check whether currently performed call (on top of call-stack) is performed in form,
 * meeting conditions of 'Direct Call to Eval' (see also: ECMA-262 v5, 15.1.2.1.1)
 *
 * Warning:
 *         the function should only be called from implementation
 *         of built-in 'eval' routine of Global object
 *
 * @return true - currently performed call is performed through 'eval' identifier,
 *                without 'this' argument,
 *         false - otherwise
 */
inline bool JERRY_ATTR_ALWAYS_INLINE
vm_is_direct_eval_form_call (void)
{
  return (JERRY_CONTEXT (status_flags) & ECMA_STATUS_DIRECT_EVAL) != 0;
} /* vm_is_direct_eval_form_call */

/**
 * Get backtrace. The backtrace is an array of strings where
 * each string contains the position of the corresponding frame.
 * The array length is zero if the backtrace is not available.
 *
 * @return array ecma value
 */
ecma_value_t
vm_get_backtrace (uint32_t max_depth) /**< maximum backtrace depth, 0 = unlimited */
{
#if ENABLED (JERRY_LINE_INFO)
  ecma_value_t result_array = ecma_op_create_array_object (NULL, 0, false);

  if (max_depth == 0)
  {
    max_depth = UINT32_MAX;
  }

  vm_frame_ctx_t *context_p = JERRY_CONTEXT (vm_top_context_p);
  ecma_object_t *array_p = ecma_get_object_from_value (result_array);
  JERRY_ASSERT (ecma_op_object_is_fast_array (array_p));
  uint32_t index = 0;

  while (context_p != NULL)
  {
    if (context_p->resource_name == ECMA_VALUE_UNDEFINED)
    {
      context_p = context_p->prev_context_p;
      continue;
    }

    ecma_string_t *str_p = ecma_get_string_from_value (context_p->resource_name);

    if (ecma_string_is_empty (str_p))
    {
      const lit_utf8_byte_t unknown_str[] = "<unknown>:";
      str_p = ecma_new_ecma_string_from_utf8 (unknown_str, sizeof (unknown_str) - 1);
    }
    else
    {
      ecma_ref_ecma_string (str_p);
      str_p = ecma_append_magic_string_to_string (str_p, LIT_MAGIC_STRING_COLON_CHAR);
    }

    ecma_string_t *line_str_p = ecma_new_ecma_string_from_uint32 (context_p->current_line);
    str_p = ecma_concat_ecma_strings (str_p, line_str_p);
    ecma_deref_ecma_string (line_str_p);

    ecma_fast_array_set_property (array_p, index, ecma_make_string_value (str_p));
    ecma_deref_ecma_string (str_p);

    context_p = context_p->prev_context_p;
    index++;

    if (index >= max_depth)
    {
      break;
    }
  }

  return result_array;
#else /* !ENABLED (JERRY_LINE_INFO) */
  JERRY_UNUSED (max_depth);

  return ecma_op_create_array_object (NULL, 0, false);
#endif /* ENABLED (JERRY_LINE_INFO) */
} /* vm_get_backtrace */
