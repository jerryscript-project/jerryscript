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
                                                            ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE,
                                                            false); /* Failure handling */

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
                                uint8_t iterator_type, /**< itertator type, see ecma_pseudo_array_type_t */
                                uint8_t extra_info) /**< extra information */
{
  /* 1. */
  JERRY_ASSERT (ecma_is_value_object (iterated_value));
  JERRY_ASSERT (iterator_type >= ECMA_PSEUDO_ARRAY_ITERATOR && iterator_type <= ECMA_PSEUDO_ARRAY__MAX);

  ecma_object_t *iterated_obj_p = ecma_get_object_from_value (iterated_value);
  /* 2. */
  ecma_object_t *object_p = ecma_create_object (prototype_obj_p,
                                                sizeof (ecma_extended_object_t),
                                                ECMA_OBJECT_TYPE_PSEUDO_ARRAY);

  ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) object_p;
  ext_obj_p->u.pseudo_array.type = iterator_type;

  /* 3. */
  ECMA_SET_NON_NULL_POINTER (ext_obj_p->u.pseudo_array.u2.iterated_value_cp, iterated_obj_p);
  /* 4. */
  ext_obj_p->u.pseudo_array.u1.iterator_index = 0;
  /* 5. */
  ext_obj_p->u.pseudo_array.extra_info = extra_info;

  /* 6. */
  return ecma_make_object_value (object_p);
} /* ecma_op_create_iterator_object */

#endif /* ENABLED (JERRY_ES2015_BUILTIN_ITERATOR) */
/**
 * @}
 * @}
 */
