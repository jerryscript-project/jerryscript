/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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
ecma_op_get_value_lex_env_base (ecma_object_t *ref_base_lex_env_p, /**< reference's base (lexical environment) */
                                ecma_string_t *var_name_string_p, /**< variable name */
                                bool is_strict) /**< flag indicating strict mode */
{
  const bool is_unresolvable_reference = (ref_base_lex_env_p == NULL);

  // 3.
  if (unlikely (is_unresolvable_reference))
  {
    return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_REFERENCE));
  }

  // 5.
  JERRY_ASSERT (ref_base_lex_env_p != NULL
                && ecma_is_lexical_environment (ref_base_lex_env_p));

  // 5.a
  return ecma_op_get_binding_value (ref_base_lex_env_p,
                                    var_name_string_p,
                                    is_strict);
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
                                   || ecma_is_value_number (base)
                                   || ecma_is_value_string (base));
  const bool has_object_base = (ecma_is_value_object (base)
                                && !(ecma_is_lexical_environment (ecma_get_object_from_value (base))));
  const bool is_property_reference = has_primitive_base || has_object_base;

  JERRY_ASSERT (!is_unresolvable_reference);
  JERRY_ASSERT (is_property_reference);

  ecma_string_t *referenced_name_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                                ref.referenced_name_cp);

  // 4.a
  if (!has_primitive_base)
  {
    // 4.b case 1

    ecma_object_t *obj_p = ecma_get_object_from_value (base);
    JERRY_ASSERT (obj_p != NULL
                  && !ecma_is_lexical_environment (obj_p));

    return ecma_op_object_get (obj_p, referenced_name_p);
  }
  else
  {
    // 4.b case 2
    ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

    // 1.
    ECMA_TRY_CATCH (obj_base, ecma_op_to_object (base), ret_value);

    ecma_object_t *obj_p = ecma_get_object_from_value (obj_base);
    JERRY_ASSERT (obj_p != NULL
                  && !ecma_is_lexical_environment (obj_p));


    // 2.
    ecma_property_t *prop_p = ecma_op_object_get_property (obj_p, referenced_name_p);

    if (prop_p == NULL)
    {
      // 3.
      ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }
    else if (prop_p->type == ECMA_PROPERTY_NAMEDDATA)
    {
      // 4.
      ecma_value_t prop_value = ecma_copy_value (ecma_get_named_data_property_value (prop_p), true);
      ret_value = ecma_make_normal_completion_value (prop_value);
    }
    else
    {
      // 5.
      JERRY_ASSERT (prop_p->type == ECMA_PROPERTY_NAMEDACCESSOR);

      ecma_object_t *obj_p = ecma_get_named_accessor_property_getter (prop_p);

      // 6.
      if (obj_p == NULL)
      {
        ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_UNDEFINED);
      }
      else
      {
        // 7.
        ret_value = ecma_op_function_call (obj_p,
                                           base,
                                           NULL);
      }
    }

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
ecma_op_put_value_lex_env_base (ecma_object_t *ref_base_lex_env_p, /**< reference's base (lexical environment) */
                                ecma_string_t *var_name_string_p, /**< variable name */
                                bool is_strict, /**< flag indicating strict mode */
                                ecma_value_t value) /**< ECMA-value */
{
  const bool is_unresolvable_reference = (ref_base_lex_env_p == NULL);

  // 3.
  if (unlikely (is_unresolvable_reference))
  {
    // 3.a.
    if (is_strict)
    {
      return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_REFERENCE));
    }
    else
    {
      // 3.b.
      ecma_object_t *global_object_p = ecma_builtin_get (ECMA_BUILTIN_ID_GLOBAL);

      ecma_completion_value_t completion = ecma_op_object_put (global_object_p,
                                                               var_name_string_p,
                                                               value,
                                                               false);

      ecma_deref_object (global_object_p);

      JERRY_ASSERT (ecma_is_completion_value_normal_true (completion)
                    || ecma_is_completion_value_normal_false (completion));

      return ecma_make_empty_completion_value ();
    }
  }

  // 5.
  JERRY_ASSERT (ref_base_lex_env_p != NULL
                && ecma_is_lexical_environment (ref_base_lex_env_p));

  // 5.a
  return ecma_op_set_mutable_binding (ref_base_lex_env_p,
                                      var_name_string_p,
                                      value,
                                      is_strict);
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
    return ecma_make_empty_completion_value ();
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
                                   || ecma_is_value_number (base)
                                   || ecma_is_value_string (base));
  const bool has_object_base = (ecma_is_value_object (base)
                                && !(ecma_is_lexical_environment (ecma_get_object_from_value (base))));
  const bool is_property_reference = has_primitive_base || has_object_base;

  JERRY_ASSERT (!is_unresolvable_reference);
  JERRY_ASSERT (is_property_reference);

  // 4.a
  if (!has_primitive_base)
  {
    // 4.b case 1

    ecma_object_t *obj_p = ecma_get_object_from_value (base);
    JERRY_ASSERT (obj_p != NULL
                  && !ecma_is_lexical_environment (obj_p));

    ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

    ECMA_TRY_CATCH (put_ret_value,
                    ecma_op_object_put (obj_p,
                                        ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                                   ref.referenced_name_cp),
                                        value,
                                        ref.is_strict),
                    ret_value);

    ret_value = ecma_make_empty_completion_value ();

    ECMA_FINALIZE (put_ret_value);

    return ret_value;
  }
  else
  {
    // 4.b case 2
    ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

    // sub_1.
    ECMA_TRY_CATCH (obj_base, ecma_op_to_object (base), ret_value);

    ecma_object_t *obj_p = ecma_get_object_from_value (obj_base);
    JERRY_ASSERT (obj_p != NULL
                  && !ecma_is_lexical_environment (obj_p));

    ecma_string_t *referenced_name_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                                  ref.referenced_name_cp);

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

        ecma_object_t *setter_p = ecma_get_named_accessor_property_setter (prop_p);
        JERRY_ASSERT (setter_p != NULL);

        ECMA_TRY_CATCH (call_ret,
                        ecma_op_function_call_array_args (setter_p, base, &value, 1),
                        ret_value);

        ret_value = ecma_make_empty_completion_value ();

        ECMA_FINALIZE (call_ret);
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
