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

#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "ecma-objects.h"
#include "ecma-function-object.h"
#include "ecma-objects-general.h"
#include "ecma-operations.h"
#include "ecma-try-catch-macro.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaoperations ECMA-defined operations
 * @{
 */

/**
 * GetValue operation part (lexical environment base or unresolvable reference).
 *
 * See also: ECMA-262 v5, 8.7.1, sections 3 and 5
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_op_get_value_lex_env_base (ecma_reference_t ref) /**< ECMA-reference */
{
  const ecma_value_t base = ref.base;
  const bool is_unresolvable_reference = ecma_is_value_undefined (base);

  // 3.
  if (unlikely (is_unresolvable_reference))
  {
    return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_REFERENCE));
  }

  // 5.
  ecma_object_t *lex_env_p = ECMA_GET_POINTER(base.value);
  JERRY_ASSERT(lex_env_p != NULL
               && ecma_is_lexical_environment (lex_env_p));

  // 5.a
  return ecma_op_get_binding_value (lex_env_p,
                                    ECMA_GET_POINTER (ref.referenced_name_cp),
                                    ref.is_strict);
} /* ecma_op_get_value_lex_env_base */

/**
 * GetValue operation part (object base).
 *
 * See also: ECMA-262 v5, 8.7.1, section 4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_op_get_value_object_base (ecma_reference_t ref) /**< ECMA-reference */
{
  const ecma_value_t base = ref.base;
  const bool is_unresolvable_reference = ecma_is_value_undefined (base);
  const bool has_primitive_base = (ecma_is_value_boolean (base)
                                   || base.value_type == ECMA_TYPE_NUMBER
                                   || base.value_type == ECMA_TYPE_STRING);
  const bool has_object_base = (base.value_type == ECMA_TYPE_OBJECT
                                && !(ecma_is_lexical_environment ((ecma_object_t*)ECMA_GET_POINTER(base.value))));
  const bool is_property_reference = has_primitive_base || has_object_base;

  JERRY_ASSERT (!is_unresolvable_reference);
  JERRY_ASSERT (is_property_reference);

  // 4.a
  if (!has_primitive_base)
  {
    // 4.b case 1

    ecma_object_t *obj_p = ECMA_GET_POINTER(base.value);
    JERRY_ASSERT(obj_p != NULL
                 && !ecma_is_lexical_environment (obj_p));

    return ecma_op_object_get (obj_p, ECMA_GET_POINTER (ref.referenced_name_cp));
  }
  else
  {
    // 4.b case 2
    ecma_completion_value_t ret_value;

    ECMA_TRY_CATCH (obj_base, ecma_op_to_object (base), ret_value);

    ecma_object_t *obj_p = ECMA_GET_POINTER (obj_base.u.value.value);
    JERRY_ASSERT (obj_p != NULL
                  && !ecma_is_lexical_environment (obj_p));

    ret_value = ecma_op_object_get (obj_p, ECMA_GET_POINTER (ref.referenced_name_cp));

    ECMA_FINALIZE (obj_base);

    return ret_value;
  }
} /* ecma_op_get_value_object_base */

/**
 * PutValue operation part (lexical environment base or unresolvable reference).
 *
 * See also: ECMA-262 v5, 8.7.2, sections 3 and 5
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_op_put_value_lex_env_base (ecma_reference_t ref, /**< ECMA-reference */
                                ecma_value_t value) /**< ECMA-value */
{
  const ecma_value_t base = ref.base;
  const bool is_unresolvable_reference = ecma_is_value_undefined (base);

  // 3.
  if (unlikely (is_unresolvable_reference))
  {
    // 3.a.
    if (ref.is_strict)
    {
      return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_REFERENCE));
    }
    else
    {
      // 3.b.
      ecma_object_t *global_object_p = ecma_builtin_get (ECMA_BUILTIN_ID_GLOBAL);

      ecma_completion_value_t completion = ecma_op_object_put (global_object_p,
                                                               ECMA_GET_POINTER (ref.referenced_name_cp),
                                                               value,
                                                               false);

      ecma_deref_object (global_object_p);

      JERRY_ASSERT(ecma_is_completion_value_normal_true (completion)
                   || ecma_is_completion_value_normal_false (completion));

      return ecma_make_empty_completion_value ();
    }
  }

  // 5.
  ecma_object_t *lex_env_p = ECMA_GET_POINTER(base.value);
  JERRY_ASSERT(lex_env_p != NULL
               && ecma_is_lexical_environment (lex_env_p));

  // 5.a
  return ecma_op_set_mutable_binding (lex_env_p,
                                      ECMA_GET_POINTER (ref.referenced_name_cp),
                                      value,
                                      ref.is_strict);
} /* ecma_op_put_value_lex_env_base */

/**
 * Reject sequence for PutValue
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
static ecma_completion_value_t
ecma_reject_put (bool is_throw) /**< Throw flag */
{
  if (is_throw)
  {
    return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_EMPTY);
  }
} /* ecma_reject_put */

/**
 * PutValue operation part (object base).
 *
 * See also: ECMA-262 v5, 8.7.2, section 4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_op_put_value_object_base (ecma_reference_t ref, /**< ECMA-reference */
                               ecma_value_t value) /**< ECMA-value */
{
  const ecma_value_t base = ref.base;
  const bool is_unresolvable_reference = ecma_is_value_undefined (base);
  const bool has_primitive_base = (ecma_is_value_boolean (base)
                                   || base.value_type == ECMA_TYPE_NUMBER
                                   || base.value_type == ECMA_TYPE_STRING);
  const bool has_object_base = (base.value_type == ECMA_TYPE_OBJECT
                                && !(ecma_is_lexical_environment ((ecma_object_t*)ECMA_GET_POINTER(base.value))));
  const bool is_property_reference = has_primitive_base || has_object_base;

  JERRY_ASSERT (!is_unresolvable_reference);
  JERRY_ASSERT (is_property_reference);

  // 4.a
  if (!has_primitive_base)
  {
    // 4.b case 1

    ecma_object_t *obj_p = ECMA_GET_POINTER(base.value);
    JERRY_ASSERT (obj_p != NULL
                  && !ecma_is_lexical_environment (obj_p));

    ecma_completion_value_t ret_value;

    ECMA_TRY_CATCH (put_completion,
                    ecma_op_object_put (obj_p,
                                        ECMA_GET_POINTER (ref.referenced_name_cp),
                                        value,
                                        ref.is_strict),
                    ret_value);

    ret_value = ecma_make_empty_completion_value ();

    ECMA_FINALIZE (put_completion);

    return ret_value;
  }
  else
  {
    // 4.b case 2
    ecma_completion_value_t ret_value;

    // sub_1.
    ECMA_TRY_CATCH (obj_base, ecma_op_to_object (base), ret_value);

    ecma_object_t *obj_p = ECMA_GET_POINTER (obj_base.u.value.value);
    JERRY_ASSERT (obj_p != NULL
                  && !ecma_is_lexical_environment (obj_p));

    ecma_string_t *referenced_name_p = ECMA_GET_POINTER (ref.referenced_name_cp);

    // sub_2.
    if (!ecma_op_object_can_put (obj_p, referenced_name_p))
    {
      ret_value = ecma_reject_put (ref.is_strict);
    }
    else
    {
      // sub_3.
      ecma_property_t *own_prop_p = ecma_op_object_get_own_property (obj_p, referenced_name_p);

      // sub_5.
      ecma_property_t *prop_p = ecma_op_object_get_property (obj_p, referenced_name_p);

      // sub_4., sub_7
      if ((own_prop_p != NULL
           && own_prop_p->type == ECMA_PROPERTY_NAMEDDATA)
          || (prop_p == NULL)
          || (prop_p->type != ECMA_PROPERTY_NAMEDACCESSOR))
      {
        ret_value = ecma_reject_put (ref.is_strict);
      }
      else
      {
        // sub_6.
        JERRY_ASSERT (prop_p != NULL && prop_p->type == ECMA_PROPERTY_NAMEDACCESSOR);

        ecma_object_t *setter_p = ECMA_GET_POINTER(prop_p->u.named_accessor_property.set_p);
        JERRY_ASSERT (setter_p != NULL);

        ECMA_TRY_CATCH (call_completion,
                        ecma_op_function_call (setter_p, base, &value, 1),
                        ret_value);

        ret_value = ecma_make_empty_completion_value ();

        ECMA_FINALIZE (call_completion);
      }
    }

    ECMA_FINALIZE (obj_base);

    return ret_value;
  }
} /* ecma_op_put_value_object_base */

/**
 * @}
 * @}
 */
