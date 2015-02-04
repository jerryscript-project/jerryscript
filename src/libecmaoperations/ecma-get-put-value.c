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
void
ecma_op_get_value_lex_env_base (ecma_completion_value_t &ret_value, /**< out: completion value */
                                const ecma_object_ptr_t& ref_base_lex_env_p, /**< reference's base
                                                                              *   (lexical environment) */
                                ecma_string_t *var_name_string_p, /**< variable name */
                                bool is_strict) /**< flag indicating strict mode */
{
  const bool is_unresolvable_reference = (ref_base_lex_env_p.is_null ());

  // 3.
  if (unlikely (is_unresolvable_reference))
  {
    ecma_object_ptr_t exception_obj_p;
    ecma_new_standard_error (exception_obj_p, ECMA_ERROR_REFERENCE);
    ecma_make_throw_obj_completion_value (ret_value, exception_obj_p);
    return;
  }

  // 5.
  JERRY_ASSERT(ref_base_lex_env_p.is_not_null ()
               && ecma_is_lexical_environment (ref_base_lex_env_p));

  // 5.a
  ecma_op_get_binding_value (ret_value,
                             ref_base_lex_env_p,
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
void
ecma_op_get_value_object_base (ecma_completion_value_t &ret_value, /**< out: completion value */
                               const ecma_reference_t& ref) /**< ECMA-reference */
{
  const ecma_value_t& base = ref.base;
  const bool is_unresolvable_reference = ecma_is_value_undefined (base);
  const bool has_primitive_base = (ecma_is_value_boolean (base)
                                   || ecma_is_value_number (base)
                                   || ecma_is_value_string (base));
  bool has_object_base = false;
  if (ecma_is_value_object (base))
  {
    ecma_object_ptr_t base_obj_p;
    ecma_get_object_from_value (base_obj_p, base);

    has_object_base = !ecma_is_lexical_environment (base_obj_p);
  }
  const bool is_property_reference = has_primitive_base || has_object_base;

  JERRY_ASSERT (!is_unresolvable_reference);
  JERRY_ASSERT (is_property_reference);

  // 4.a
  if (!has_primitive_base)
  {
    // 4.b case 1

    ecma_object_ptr_t obj_p;
    ecma_get_object_from_value (obj_p, base);
    JERRY_ASSERT(obj_p.is_not_null ()
                 && !ecma_is_lexical_environment (obj_p));

    ecma_op_object_get (ret_value, obj_p, ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                                     ref.referenced_name_cp));
  }
  else
  {
    // 4.b case 2
    ECMA_TRY_CATCH (ret_value, ecma_op_to_object, obj_base, base);

    ecma_object_ptr_t obj_p;
    ecma_get_object_from_value (obj_p, obj_base);
    JERRY_ASSERT (obj_p.is_not_null ()
                  && !ecma_is_lexical_environment (obj_p));

    ecma_op_object_get (ret_value, obj_p, ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                                     ref.referenced_name_cp));

    ECMA_FINALIZE (obj_base);
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
void
ecma_op_put_value_lex_env_base (ecma_completion_value_t &ret_value, /**< out: completion value */
                                const ecma_object_ptr_t& ref_base_lex_env_p, /**< reference's base
                                                                              *   (lexical environment) */
                                ecma_string_t *var_name_string_p, /**< variable name */
                                bool is_strict, /**< flag indicating strict mode */
                                const ecma_value_t& value) /**< ECMA-value */
{
  const bool is_unresolvable_reference = (ref_base_lex_env_p.is_null ());

  // 3.
  if (unlikely (is_unresolvable_reference))
  {
    // 3.a.
    if (is_strict)
    {
      ecma_object_ptr_t exception_obj_p;
      ecma_new_standard_error (exception_obj_p, ECMA_ERROR_REFERENCE);
      ecma_make_throw_obj_completion_value (ret_value, exception_obj_p);
      return;
    }
    else
    {
      // 3.b.
      ecma_object_ptr_t global_object_p;
      ecma_builtin_get (global_object_p, ECMA_BUILTIN_ID_GLOBAL);

      ecma_completion_value_t completion;
      ecma_op_object_put (completion,
                          global_object_p,
                          var_name_string_p,
                          value,
                          false);

      ecma_deref_object (global_object_p);

      JERRY_ASSERT(ecma_is_completion_value_normal_true (completion)
                   || ecma_is_completion_value_normal_false (completion));

      ecma_make_empty_completion_value (ret_value);
      return;
    }
  }

  // 5.
  JERRY_ASSERT(ref_base_lex_env_p.is_not_null ()
               && ecma_is_lexical_environment (ref_base_lex_env_p));

  // 5.a
  ecma_op_set_mutable_binding (ret_value,
                               ref_base_lex_env_p,
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
static void
ecma_reject_put (ecma_completion_value_t &ret_value, /**< out: completion value */
                 bool is_throw) /**< Throw flag */
{
  if (is_throw)
  {
    ecma_object_ptr_t exception_obj_p;
    ecma_new_standard_error (exception_obj_p, ECMA_ERROR_TYPE);
    ecma_make_throw_obj_completion_value (ret_value, exception_obj_p);
  }
  else
  {
    ecma_make_simple_completion_value (ret_value, ECMA_SIMPLE_VALUE_EMPTY);
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
void
ecma_op_put_value_object_base (ecma_completion_value_t &ret_value, /**< out: completion value */
                               const ecma_reference_t& ref, /**< ECMA-reference */
                               const ecma_value_t& value) /**< ECMA-value */
{
  const ecma_value_t& base = ref.base;
  const bool is_unresolvable_reference = ecma_is_value_undefined (base);
  const bool has_primitive_base = (ecma_is_value_boolean (base)
                                   || ecma_is_value_number (base)
                                   || ecma_is_value_string (base));
  bool has_object_base = false;
  if (ecma_is_value_object (base))
  {
    ecma_object_ptr_t base_obj_p;
    ecma_get_object_from_value (base_obj_p, base);

    has_object_base = !ecma_is_lexical_environment (base_obj_p);
  }
  const bool is_property_reference = has_primitive_base || has_object_base;

  JERRY_ASSERT (!is_unresolvable_reference);
  JERRY_ASSERT (is_property_reference);

  // 4.a
  if (!has_primitive_base)
  {
    // 4.b case 1

    ecma_object_ptr_t obj_p;
    ecma_get_object_from_value (obj_p, base);
    JERRY_ASSERT (obj_p.is_not_null ()
                  && !ecma_is_lexical_environment (obj_p));

    ECMA_TRY_CATCH (ret_value, ecma_op_object_put, put_ret_value, obj_p,
                    ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                               ref.referenced_name_cp),
                    value, ref.is_strict);

    ecma_make_empty_completion_value (ret_value);

    ECMA_FINALIZE (put_ret_value);
  }
  else
  {
    // 4.b case 2

    // sub_1.
    ECMA_TRY_CATCH (ret_value, ecma_op_to_object, obj_base, base);

    ecma_object_ptr_t obj_p;
    ecma_get_object_from_value (obj_p, obj_base);
    JERRY_ASSERT (obj_p.is_not_null ()
                  && !ecma_is_lexical_environment (obj_p));

    ecma_string_t *referenced_name_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                                  ref.referenced_name_cp);

    // sub_2.
    if (!ecma_op_object_can_put (obj_p, referenced_name_p))
    {
      ecma_reject_put (ret_value, ref.is_strict);
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
        ecma_reject_put (ret_value, ref.is_strict);
      }
      else
      {
        // sub_6.
        JERRY_ASSERT (prop_p != NULL && prop_p->type == ECMA_PROPERTY_NAMEDACCESSOR);

        ecma_object_ptr_t setter_p;
        setter_p.unpack_from (prop_p->u.named_accessor_property.set_p);
        JERRY_ASSERT (setter_p.is_not_null ());

        ECMA_TRY_CATCH (ret_value, ecma_op_function_call, call_ret, setter_p, base, &value, 1);

        ecma_make_empty_completion_value (ret_value);

        ECMA_FINALIZE (call_ret);
      }
    }

    ECMA_FINALIZE (obj_base);
  }
} /* ecma_op_put_value_object_base */

/**
 * @}
 * @}
 */
