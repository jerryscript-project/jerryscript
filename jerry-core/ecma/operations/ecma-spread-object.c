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

#include "ecma-spread-object.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaspreadeanobject ECMA Spread object related routines
 * @{
 */

/**
 * Spread object creation operation.
 *
 * @return pseudo array object as an ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_create_spread_object (ecma_value_t element) /**< value to spread */
{
  ecma_object_t *object_p = ecma_create_object (NULL,
                                                sizeof (ecma_extended_object_t),
                                                ECMA_OBJECT_TYPE_PSEUDO_ARRAY);

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
  ext_object_p->u.pseudo_array.type = ECMA_PSEUDO_SPREAD_OBJECT;
  ext_object_p->u.pseudo_array.u2.spread_value = ecma_copy_value_if_not_object (element);

  return ecma_make_object_value (object_p);
} /* ecma_op_create_spread_object */

/**
 * Spread object creation operation.
 *
 * @return true - if the argument is a spread object
 *         false, otherwise
 */
bool
ecma_op_is_spread_object (ecma_value_t value) /**< value to check */
{
  if (ecma_is_value_object (value))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (value);

    if (ecma_get_object_type (object_p) == ECMA_OBJECT_TYPE_PSEUDO_ARRAY)
    {
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
      return ext_object_p->u.pseudo_array.type == ECMA_PSEUDO_SPREAD_OBJECT;
    }
  }

  return false;
} /* ecma_op_is_spread_object */

/**
 * Get the referenced element by the spread object
 *
 * @return element referenced by the spread object as an ecma-value
 */
ecma_value_t
ecma_op_spread_object_get_spreaded_element (ecma_object_t *object_p) /**< spread object */
{
  JERRY_ASSERT (ecma_op_is_spread_object (ecma_make_object_value (object_p)));

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
  return ecma_copy_value (ext_object_p->u.pseudo_array.u2.spread_value);
} /* ecma_op_spread_object_get_spreaded_element */

/**
 * @}
 * @}
 */
