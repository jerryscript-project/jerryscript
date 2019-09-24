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

#include "ecma-builtin-helpers.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-arraybuffer-object.h"
#include "ecma-try-catch-macro.h"
#include "jrt.h"
#include "jrt-libc-includes.h"

#if ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY)

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-arraybuffer-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID arraybuffer_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup arraybufferprototype ECMA ArrayBuffer.prototype object built-in
 * @{
 */

/**
 * The ArrayBuffer.prototype.bytelength accessor
 *
 * See also:
 *          ES2015, 24.1.4.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_arraybuffer_prototype_bytelength_getter (ecma_value_t this_arg) /**< this argument */
{
  if (ecma_is_value_object (this_arg))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (this_arg);

    if (ecma_object_class_is (object_p, LIT_MAGIC_STRING_ARRAY_BUFFER_UL))
    {
      if (ecma_arraybuffer_is_detached (object_p))
      {
        return ecma_raise_type_error (ECMA_ERR_MSG ("ArrayBuffer has been detached."));
      }
      ecma_length_t len = ecma_arraybuffer_get_length (object_p);

      return ecma_make_uint32_value (len);
    }
  }

  return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not a ArrayBuffer object."));
} /* ecma_builtin_arraybuffer_prototype_bytelength_getter */

/**
 * The ArrayBuffer.prototype object's 'slice' routine
 *
 * See also:
 *          ES2015, 24.1.4.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_arraybuffer_prototype_object_slice (ecma_value_t this_arg, /**< this argument */
                                                 ecma_value_t arg1, /**< routine's first argument */
                                                 ecma_value_t arg2) /**< routine's second argument */
{
  if (!ecma_is_value_object (this_arg))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not object."));
  }

  ecma_object_t *object_p = ecma_get_object_from_value (this_arg);

  if (!ecma_object_class_is (object_p, LIT_MAGIC_STRING_ARRAY_BUFFER_UL))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not an ArrayBuffer object."));
  }

  if (ecma_arraybuffer_is_detached (object_p))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("ArrayBuffer has been detached."));
  }

  ecma_length_t len = ecma_arraybuffer_get_length (object_p);

  ecma_length_t start = 0, end = len;

  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  ECMA_OP_TO_NUMBER_TRY_CATCH (start_num,
                               arg1,
                               ret_value);

  start = ecma_builtin_helper_array_index_normalize (start_num, len, false);

  if (!ecma_is_value_undefined (arg2))
  {
    ECMA_OP_TO_NUMBER_TRY_CATCH (end_num,
                                 arg2,
                                 ret_value);

    end = ecma_builtin_helper_array_index_normalize (end_num, len, false);

    ECMA_OP_TO_NUMBER_FINALIZE (end_num);
  }

  ECMA_OP_TO_NUMBER_FINALIZE (start_num);

  if (ret_value != ECMA_VALUE_EMPTY)
  {
    return ret_value;
  }

  JERRY_ASSERT (start <= len && end <= len);
  ecma_length_t new_len = (end >= start) ? (end - start) : 0;
  ecma_object_t *new_arraybuffer_p = ecma_arraybuffer_new_object (new_len);
  lit_utf8_byte_t *old_buf = ecma_arraybuffer_get_buffer (object_p);
  lit_utf8_byte_t *new_buf = ecma_arraybuffer_get_buffer (new_arraybuffer_p);

  memcpy (new_buf, old_buf + start, new_len);

  return ecma_make_object_value (new_arraybuffer_p);
} /* ecma_builtin_arraybuffer_prototype_object_slice */

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY) */
