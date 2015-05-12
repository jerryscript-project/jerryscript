/* Copyright 2015 Samsung Electronics Co., Ltd.
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

#include "ecma-array-prototype.h"

#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-function-object.h"
#include "ecma-exceptions.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-try-catch-macro.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup arrayprototype ECMA Array.prototype object built-in operations
 * @{
 */

/**
 * The Array.prototype.toString's separator creation routine
 *
 * See also:
 *          ECMA-262 v5.1, 15.4.4.2 4th step
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_op_array_get_separator_string (ecma_value_t separator) /** < possible separator */
{
  if (ecma_is_value_undefined (separator))
  {
    ecma_string_t *comma_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_COMMA_CHAR);
    return ecma_make_normal_completion_value (ecma_make_string_value (comma_string_p));
  }
  else
  {
    return ecma_op_to_string (separator);
  }
} /* ecma_op_array_get_separator_string */

/**
 * The Array.prototype's 'toString' single element operation routine
 *
 * See also:
 *          ECMA-262 v5.1, 15.4.4.2
 *
 * @return ecma_completion_value_t value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_op_array_get_to_string_at_index (ecma_object_t *obj_p, /** < this object */
                                      uint32_t index) /** < array index */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();
  ecma_string_t *index_string_p = ecma_new_ecma_string_from_uint32 (index);

  ECMA_TRY_CATCH (index_value,
                  ecma_op_object_get (obj_p, index_string_p),
                  ret_value);

  if (ecma_is_value_undefined (index_value)
      || ecma_is_value_null (index_value))
  {
    ecma_string_t *empty_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING__EMPTY);
    ret_value = ecma_make_normal_completion_value (ecma_make_string_value (empty_string_p));
  }
  else
  {
    ret_value = ecma_op_to_string (index_value);
  }

  ECMA_FINALIZE (index_value)

  ecma_deref_ecma_string (index_string_p);

  return ret_value;
} /* ecma_op_array_get_to_string_at_index */

/**
 * @}
 * @}
 * @}
 */
