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
#include "ecma-array-object.h"
#include "ecma-iterator-object.h"
#include "ecma-builtin-helpers.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-number-arithmetic.h"
#include "ecma-objects.h"
#include "ecma-objects-general.h"
#include "ecma-function-object.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaiteratorobject ECMA iterator object related routines
 * @{
 */

#if ENABLED (JERRY_ES2015_BUILTIN_ITERATOR)
/**
 * Implementation of 'CreateArrayFromList' specialized for iterators
 *
 * See also:
 *          ECMA-262 v6, 7.3.16.
 *
 * Note:
 *      Returned value must be freed with ecma_free_value.
 *
 * @return new array object
 */
ecma_value_t
ecma_create_array_from_iter_element (ecma_value_t value, /**< value */
                                     ecma_value_t index_value) /**< iterator index */
{
  /* 2. */
  ecma_value_t new_array = ecma_op_create_array_object (NULL, 0, false);
  JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (new_array));
  ecma_object_t *new_array_p = ecma_get_object_from_value (new_array);

  /* 3 - 4. */
  for (uint32_t index = 0; index < 2; index++)
  {
    ecma_string_t *index_string_p = ecma_new_ecma_string_from_uint32 (index);

    /* 4.a */
    ecma_value_t completion = ecma_builtin_helper_def_prop (new_array_p,
                                                            index_string_p,
                                                            (index == 0) ? index_value : value,
                                                            ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);

    /* 4.b */
    JERRY_ASSERT (ecma_is_value_true (completion));

    ecma_deref_ecma_string (index_string_p);
  }

  /* 5. */
  return new_array;
} /* ecma_create_array_from_iter_element */

/**
 * CreateIterResultObject operation
 *
 * See also:
 *          ECMA-262 v6, 7.4.7.
 *
 * Note:
 *      Returned value must be freed with ecma_free_value.
 *
 * @return iterator result object
 */
ecma_value_t
ecma_create_iter_result_object (ecma_value_t value, /**< value */
                                ecma_value_t done) /**< ECMA_VALUE_{TRUE,FALSE} based
                                                    *   on the iterator index */
{
  /* 1. */
  JERRY_ASSERT (ecma_is_value_boolean (done));

  /* 2. */
  ecma_object_t *object_p = ecma_create_object (ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE),
                                                0,
                                                ECMA_OBJECT_TYPE_GENERAL);

  /* 3. */
  ecma_property_value_t *prop_value_p;
  prop_value_p = ecma_create_named_data_property (object_p,
                                                  ecma_get_magic_string (LIT_MAGIC_STRING_VALUE),
                                                  ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE,
                                                  NULL);

  prop_value_p->value = ecma_copy_value_if_not_object (value);

  /* 4. */
  prop_value_p = ecma_create_named_data_property (object_p,
                                                  ecma_get_magic_string (LIT_MAGIC_STRING_DONE),
                                                  ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE,
                                                  NULL);
  prop_value_p->value = done;

  /* 5. */
  return ecma_make_object_value (object_p);
} /* ecma_create_iter_result_object */

/**
 * General iterator object creation operation.
 *
 * See also: ECMA-262 v6, 21.1.5.1, 22.1.5.1, 23.1.5.1
 *
 * Note:
 *      Returned value must be freed with ecma_free_value.
 *
 * @return iterator object
 */
ecma_value_t
ecma_op_create_iterator_object (ecma_value_t iterated_value, /**< value from create iterator */
                                ecma_object_t *prototype_obj_p, /**< prototype object */
                                uint8_t iterator_type, /**< iterator type, see ecma_pseudo_array_type_t */
                                uint8_t extra_info) /**< extra information */
{
  /* 1. */
  JERRY_ASSERT (iterator_type >= ECMA_PSEUDO_ARRAY_ITERATOR && iterator_type <= ECMA_PSEUDO_ARRAY__MAX);

  /* 2. */
  ecma_object_t *object_p = ecma_create_object (prototype_obj_p,
                                                sizeof (ecma_extended_object_t),
                                                ECMA_OBJECT_TYPE_PSEUDO_ARRAY);

  ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) object_p;
  ext_obj_p->u.pseudo_array.type = iterator_type;

  /* 3. */
  ext_obj_p->u.pseudo_array.u2.iterated_value = iterated_value;
  /* 4. */
  ext_obj_p->u.pseudo_array.u1.iterator_index = 0;
  /* 5. */
  ext_obj_p->u.pseudo_array.extra_info = extra_info;

  /* 6. */
  return ecma_make_object_value (object_p);
} /* ecma_op_create_iterator_object */

/**
 * GetIterator operation
 *
 * See also: ECMA-262 v6, 7.4.1
 *
 * Note:
 *      Returned value must be freed with ecma_free_value.
 *
 * @return iterator object - if success
 *         raised error - otherwise
 */
ecma_value_t
ecma_op_get_iterator (ecma_value_t value, /**< value to get iterator from */
                      ecma_value_t method) /**< provided method argument */
{
  /* 1. */
  if (ECMA_IS_VALUE_ERROR (value))
  {
    return value;
  }

  /* 2. */
  bool has_method = !ecma_is_value_empty (method);

  if (!has_method)
  {
    /* 2.a */
    method = ecma_op_get_method_by_symbol_id (value, LIT_MAGIC_STRING_ITERATOR);

    /* 2.b */
    if (ECMA_IS_VALUE_ERROR (method))
    {
      return method;
    }
  }

  /* 3. */
  if (!ecma_is_value_object (method) || !ecma_op_is_callable (method))
  {
    ecma_free_value (method);
    return ecma_raise_type_error (ECMA_ERR_MSG ("object is not iterable"));
  }

  ecma_object_t *method_obj_p = ecma_get_object_from_value (method);
  ecma_value_t iterator = ecma_op_function_call (method_obj_p, value, NULL, 0);

  if (!has_method)
  {
    ecma_deref_object (method_obj_p);
  }

  /* 4. */
  if (ECMA_IS_VALUE_ERROR (iterator))
  {
    return iterator;
  }

  /* 5. */
  if (!ecma_is_value_object (iterator))
  {
    ecma_free_value (iterator);
    return ecma_raise_type_error (ECMA_ERR_MSG ("Iterator is not an object."));
  }

  /* 6. */
  return iterator;
} /* ecma_op_get_iterator */

/**
 * IteratorNext operation
 *
 * See also: ECMA-262 v6, 7.4.2
 *
 * Note:
 *      Returned value must be freed with ecma_free_value.
 *
 * @return iterator result object - if success
 *         raised error - otherwise
 */
static ecma_value_t
ecma_op_iterator_next (ecma_value_t iterator, /**< iterator value */
                       ecma_value_t value) /**< the routines's value argument */
{
  JERRY_ASSERT (ecma_is_value_object (iterator));

  /* 1 - 2. */
  ecma_object_t *obj_p = ecma_get_object_from_value (iterator);

  ecma_value_t next = ecma_op_object_get_by_magic_id (obj_p, LIT_MAGIC_STRING_NEXT);

  if (ECMA_IS_VALUE_ERROR (next))
  {
    return next;
  }

  if (!ecma_is_value_object (next) || !ecma_op_is_callable (next))
  {
    ecma_free_value (next);
    return ecma_raise_type_error (ECMA_ERR_MSG ("Next is not callable."));
  }

  ecma_object_t *next_obj_p = ecma_get_object_from_value (next);

  bool has_value = !ecma_is_value_empty (value);

  ecma_value_t result;
  if (has_value)
  {
    result = ecma_op_function_call (next_obj_p, iterator, &value, 1);
  }
  else
  {
    result = ecma_op_function_call (next_obj_p, iterator, NULL, 0);
  }

  ecma_free_value (next);

  /* 3. */
  if (ECMA_IS_VALUE_ERROR (result))
  {
    return result;
  }

  /* 4. */
  if (!ecma_is_value_object (result))
  {
    ecma_free_value (result);
    return ecma_raise_type_error (ECMA_ERR_MSG ("Iterator result is not an object."));
  }

  /* 5. */
  return result;
} /* ecma_op_iterator_next */

/**
 * IteratorComplete operation
 *
 * See also: ECMA-262 v6, 7.4.3
 *
 * Note:
 *      Returned value must be freed with ecma_free_value.
 *
 * @return ECMA_VALUE_{FALSE, TRUE} - if success
 *         raised error - otherwise
 */
static ecma_value_t
ecma_op_iterator_complete (ecma_value_t iter_result) /**< iterator value */
{
  /* 1. */
  JERRY_ASSERT (ecma_is_value_object (iter_result));

  /* 2. */
  ecma_object_t *obj_p = ecma_get_object_from_value (iter_result);

  ecma_value_t done = ecma_op_object_get_by_magic_id (obj_p, LIT_MAGIC_STRING_DONE);

  if (ECMA_IS_VALUE_ERROR (done))
  {
    return done;
  }

  bool is_done = ecma_op_to_boolean (done);

  ecma_free_value (done);

  return ecma_make_boolean_value (is_done);
} /* ecma_op_iterator_complete */

/**
 * IteratorValue operation
 *
 * See also: ECMA-262 v6, 7.4.4
 *
 * Note:
 *      Returned value must be freed with ecma_free_value.
 *
 * @return value of the iterator result object
 */
ecma_value_t
ecma_op_iterator_value (ecma_value_t iter_result) /**< iterator value */
{
  /* 1. */
  JERRY_ASSERT (ecma_is_value_object (iter_result));

  /* 2. */
  ecma_object_t *obj_p = ecma_get_object_from_value (iter_result);
  return ecma_op_object_get_by_magic_id (obj_p, LIT_MAGIC_STRING_VALUE);
} /* ecma_op_iterator_value */

/**
 * IteratorStep operation
 *
 * See also: ECMA-262 v6, 7.4.5
 *
 * Note:
 *      Returned value must be freed with ecma_free_value.
 *
 * @return iterator object or ECMA_VALUE_FALSE - if success
 *         raised error - otherwise
 */
ecma_value_t
ecma_op_iterator_step (ecma_value_t iterator) /**< iterator value */
{
  /* 1. */
  ecma_value_t result = ecma_op_iterator_next (iterator, ECMA_VALUE_EMPTY);

  /* 2. */
  if (ECMA_IS_VALUE_ERROR (result))
  {
    return result;
  }

  /* 3. */
  ecma_value_t done = ecma_op_iterator_complete (result);

  /* 4. */
  if (ECMA_IS_VALUE_ERROR (done))
  {
    ecma_free_value (result);
    return done;
  }

  ecma_free_value (done);

  /* 5. */
  if (ecma_is_value_true (done))
  {
    ecma_free_value (result);
    return ECMA_VALUE_FALSE;
  }

  /* 6. */
  return result;
} /* ecma_op_iterator_step */

#endif /* ENABLED (JERRY_ES2015_BUILTIN_ITERATOR) */
/**
 * @}
 * @}
 */
