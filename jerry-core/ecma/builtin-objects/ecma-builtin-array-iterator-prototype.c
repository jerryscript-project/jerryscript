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
#include "ecma-iterator-object.h"
#include "ecma-typedarray-object.h"

#if ENABLED (JERRY_ES2015)

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-array-iterator-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID array_iterator_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup %arrayiteratorprototype% ECMA %ArrayIteratorPrototype% object built-in
 * @{
 */

/**
 * The %ArrayIteratorPrototype% object's 'next' routine
 *
 * See also:
 *          ECMA-262 v6, 22.1.5.2.1
 *
 * Note:
 *     Returned value must be freed with ecma_free_value.
 *
 * @return iterator result object, if success
 *         error - otherwise
 */
static ecma_value_t
ecma_builtin_array_iterator_prototype_object_next (ecma_value_t this_val) /**< this argument */
{
  /* 1 - 2. */
  if (!ecma_is_value_object (this_val))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not an object."));
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (this_val);
  ecma_iterator_object_t *iterator_obj_p = (ecma_iterator_object_t *) obj_p;

  /* 3. */
  if (!ecma_object_class_is (obj_p, LIT_MAGIC_STRING_ARRAY_ITERATOR_UL))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not an iterator."));
  }

  ecma_value_t iterated_value = iterator_obj_p->header.u.class_prop.u.iterated_value;

  /* 4 - 5 */
  if (ecma_is_value_empty (iterated_value))
  {
    return ecma_create_iter_result_object (ECMA_VALUE_UNDEFINED, ECMA_VALUE_TRUE);
  }

  ecma_object_t *array_object_p = ecma_get_object_from_value (iterated_value);

  /* 8 - 9. */
  uint32_t length;
  ecma_value_t len_value = ecma_op_object_get_length (array_object_p, &length);

  if (ECMA_IS_VALUE_ERROR (len_value))
  {
    return len_value;
  }

  uint32_t index = iterator_obj_p->index;

  /* 11. */
  iterator_obj_p->index++;

  if (index >= length)
  {
    iterator_obj_p->header.u.class_prop.u.iterated_value = ECMA_VALUE_EMPTY;
    return ecma_create_iter_result_object (ECMA_VALUE_UNDEFINED, ECMA_VALUE_TRUE);
  }

  /* 7. */
  ecma_array_iterator_type_t iterator_type;
  iterator_type = (ecma_array_iterator_type_t) iterator_obj_p->header.u.class_prop.extra_info;

  if (iterator_type == ECMA_ITERATOR_KEYS)
  {
    /* 12. */
    return ecma_create_iter_result_object (ecma_make_uint32_value (index), ECMA_VALUE_FALSE);
  }

  /* 14. */
  ecma_value_t get_value = ecma_op_object_get_by_uint32_index (array_object_p, index);

  /* 15. */
  if (ECMA_IS_VALUE_ERROR (get_value))
  {
    return get_value;
  }

  ecma_value_t result;

  /* 16. */
  if (iterator_type == ECMA_ITERATOR_VALUES)
  {
    result = ecma_create_iter_result_object (get_value, ECMA_VALUE_FALSE);
  }
  else
  {
    /* 17.a */
    JERRY_ASSERT (iterator_type == ECMA_ITERATOR_KEYS_VALUES);

    /* 17.b */
    ecma_value_t entry_array_value;
    entry_array_value = ecma_create_array_from_iter_element (get_value,
                                                             ecma_make_uint32_value (index));

    result = ecma_create_iter_result_object (entry_array_value, ECMA_VALUE_FALSE);
    ecma_free_value (entry_array_value);
  }

  ecma_free_value (get_value);

  return result;
} /* ecma_builtin_array_iterator_prototype_object_next */

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_ES2015) */
