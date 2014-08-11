/* Copyright 2014 Samsung Electronics Co., Ltd.
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

/**
 * Implementation of ECMA GetValue and PutValue
 */

#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-global-object.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "ecma-objects-properties.h"
#include "ecma-operations.h"

/** \addtogroup ecma ---TODO---
 * @{
 *
 * \addtogroup ecmaoperations ECMA-defined operations
 * @{
 */

/**
 * GetValue operation.
 *
 * See also: ECMA-262 v5, 8.7.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_op_get_value (ecma_reference_t ref) /**< ECMA-reference */
{
  const ecma_value_t base = ref.base;
  const bool is_unresolvable_reference = ecma_is_value_undefined (base);
  const bool has_primitive_base = (ecma_is_value_boolean (base)
                                   || base.value_type == ECMA_TYPE_NUMBER
                                   || base.value_type == ECMA_TYPE_STRING);
  const bool has_object_base = (base.value_type == ECMA_TYPE_OBJECT
                                && !((ecma_object_t*)ECMA_GET_POINTER(base.value))->is_lexical_environment);
  const bool is_property_reference = has_primitive_base || has_object_base;

  // GetValue_3
  if (is_unresolvable_reference)
  {
    return ecma_make_throw_value (ecma_new_standard_error (ECMA_ERROR_REFERENCE));
  }

  // GetValue_4
  if (is_property_reference)
  {
    if (!has_primitive_base) // GetValue_4.a
    {
      ecma_object_t *obj_p = ECMA_GET_POINTER(base.value);
      JERRY_ASSERT(obj_p != NULL && !obj_p->is_lexical_environment);

      // GetValue_4.b case 1
      /* return [[Get]](base as this, ref.referenced_name_p) */
      JERRY_UNIMPLEMENTED();
    } else
    { // GetValue_4.b case 2
      /*
           ecma_object_t *obj_p = ecma_ToObject (base);
           JERRY_ASSERT(obj_p != NULL && !obj_p->is_lexical_environment);
           ecma_property_t *property = obj_p->[[GetProperty]](ref.referenced_name_p);
           if (property->Type == ECMA_PROPERTY_NAMEDDATA)
           {
           return ecma_make_completion_value (ECMA_COMPLETION_TYPE_NORMAL,
           ecma_copy_value (property->u.named_data_property.value),
           ECMA_TARGET_ID_RESERVED);
           } else
           {
           JERRY_ASSERT(property->Type == ECMA_PROPERTY_NAMEDACCESSOR);

           ecma_object_t *getter = ECMA_GET_POINTER(property->u.named_accessor_property.get_p);

           if (getter == NULL)
           {
           return ecma_make_completion_value (ECMA_COMPLETION_TYPE_NORMAL,
           ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED),
           ECMA_TARGET_ID_RESERVED);
           } else
           {
           return [[Call]](getter, base as this);
           }
           }
       */
      JERRY_UNIMPLEMENTED();
    }
  } else
  {
    // GetValue_5
    ecma_object_t *lex_env_p = ECMA_GET_POINTER(base.value);

    JERRY_ASSERT(lex_env_p != NULL && lex_env_p->is_lexical_environment);

    return ecma_op_get_binding_value (lex_env_p, ref.referenced_name_p, ref.is_strict);
  }
} /* ecma_op_get_value */

/**
 * PutValue operation.
 *
 * See also: ECMA-262 v5, 8.7.1

 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_op_put_value (ecma_reference_t ref, /**< ECMA-reference */
                   ecma_value_t value) /**< ECMA-value */
{
  const ecma_value_t base = ref.base;
  const bool is_unresolvable_reference = ecma_is_value_undefined (base);
  const bool has_primitive_base = (ecma_is_value_boolean (base)
                                   || base.value_type == ECMA_TYPE_NUMBER
                                   || base.value_type == ECMA_TYPE_STRING);
  const bool has_object_base = (base.value_type == ECMA_TYPE_OBJECT
                                && !((ecma_object_t*)ECMA_GET_POINTER(base.value))->is_lexical_environment);
  const bool is_property_reference = has_primitive_base || has_object_base;

  if (is_unresolvable_reference) // PutValue_3
  {
    if (ref.is_strict) // PutValue_3.a
    {
      return ecma_make_throw_value (ecma_new_standard_error (ECMA_ERROR_REFERENCE));
    } else // PutValue_3.b
    {
      ecma_object_t *global_object_p = ecma_get_global_object ();

      ecma_completion_value_t completion = ecma_op_object_put (global_object_p,
                                                               ref.referenced_name_p,
                                                               value,
                                                               false);

      ecma_deref_object (global_object_p);

      JERRY_ASSERT(ecma_is_completion_value_normal_true (completion)
                   || ecma_is_completion_value_normal_false (completion));

      return ecma_make_empty_completion_value ();
    }
  } else if (is_property_reference) // PutValue_4
  {
    if (!has_primitive_base) // PutValue_4.a
    {
      // PutValue_4.b case 1

      /* return [[Put]](base as this, ref.referenced_name_p, value, ref.is_strict); */
      JERRY_UNIMPLEMENTED();
    } else
    {
      // PutValue_4.b case 2

      /*
      // PutValue_sub_1
      ecma_object_t *obj_p = ecma_ToObject (base);
      JERRY_ASSERT(obj_p != NULL && !obj_p->is_lexical_environment);

      // PutValue_sub_2
      if (!obj_p->[[CanPut]](ref.referenced_name_p))
      {
      // PutValue_sub_2.a
      if (ref.is_strict)
      {
      return ecma_make_throw_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
      } else
      { // PutValue_sub_2.b
      return ecma_make_completion_value (ECMA_COMPLETION_TYPE_NORMAL,
      ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY),
      ECMA_TARGET_ID_RESERVED);
      }
      }

      // PutValue_sub_3
      ecma_property_t *own_prop = obj_p->[[GetOwnProperty]](ref.referenced_name_p);

      // PutValue_sub_4
      if (ecma_OpIsDataDescriptor (own_prop))
      {
      // PutValue_sub_4.a
      if (ref.is_strict)
      {
      return ecma_make_throw_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
      } else
      { // PutValue_sub_4.b
      return ecma_make_completion_value (ECMA_COMPLETION_TYPE_NORMAL,
      ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY),
      ECMA_TARGET_ID_RESERVED);
      }
      }

      // PutValue_sub_5
      ecma_property_t *prop = obj_p->[[GetProperty]](ref.referenced_name_p);

      // PutValue_sub_6
      if (ecma_OpIsAccessorDescriptor (prop))
      {
      // PutValue_sub_6.a
      ecma_object_t *setter = ECMA_GET_POINTER(property->u.named_accessor_property.set_p);
      JERRY_ASSERT(setter != NULL);

      // PutValue_sub_6.b
      return [[Call]](setter, base as this, value);
      } else // PutValue_sub_7
      {
      // PutValue_sub_7.a
      if (ref.is_strict)
      {
      return ecma_make_throw_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
      }
      }

      // PutValue_sub_8
      return ecma_make_completion_value (ECMA_COMPLETION_TYPE_NORMAL,
      ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY),
      ECMA_TARGET_ID_RESERVED);
       */

      JERRY_UNIMPLEMENTED();
    }
  } else
  {
    // PutValue_7
    ecma_object_t *lex_env_p = ECMA_GET_POINTER(base.value);

    JERRY_ASSERT(lex_env_p != NULL && lex_env_p->is_lexical_environment);

    return ecma_op_set_mutable_binding (lex_env_p, ref.referenced_name_p, value, ref.is_strict);
  }
} /* ecma_op_put_value */

/**
 * @}
 * @}
 */
