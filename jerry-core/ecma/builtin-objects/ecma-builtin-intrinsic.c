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

#include "ecma-builtins.h"
#include "ecma-array-object.h"
#include "ecma-gc.h"

#if ENABLED (JERRY_ES2015)

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-intrinsic.inc.h"
#define BUILTIN_UNDERSCORED_ID intrinsic
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup intrinsic ECMA Intrinsic object built-in
 * @{
 */

/**
 * The %ArrayProto_values% intrinsic routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.4
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_intrinsic_array_prototype_values (ecma_value_t this_value) /**< this argument */
{
  ecma_value_t this_obj = ecma_op_to_object (this_value);

  if (ECMA_IS_VALUE_ERROR (this_obj))
  {
    return this_obj;
  }

  ecma_object_t *this_obj_p = ecma_get_object_from_value (this_obj);

  ecma_value_t ret_value = ecma_op_create_array_iterator (this_obj_p, ECMA_ITERATOR_VALUES);

  ecma_deref_object (this_obj_p);

  return ret_value;
} /* ecma_builtin_intrinsic_array_prototype_values */

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_ES2015) */
