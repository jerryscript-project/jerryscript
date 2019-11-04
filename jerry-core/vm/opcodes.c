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
#include "ecma-builtins.h"
#include "ecma-builtin-helpers.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-iterator-object.h"
#include "ecma-lex-env.h"
#include "ecma-objects.h"
#include "ecma-spread-object.h"
#include "ecma-try-catch-macro.h"
#include "jcontext.h"
#include "opcodes.h"
#include "vm-defines.h"

/** \addtogroup vm Virtual machine
 * @{
 *
 * \addtogroup vm_opcodes Opcodes
 * @{
 */

/**
 * 'Variable declaration' opcode handler.
 *
 * See also: ECMA-262 v5, 10.5 - Declaration binding instantiation (block 8).
 */
inline void JERRY_ATTR_ALWAYS_INLINE
vm_var_decl (ecma_object_t *lex_env_p, /**< target lexical environment */
             ecma_string_t *var_name_str_p, /**< variable name */
             bool is_configurable_bindings) /**< true if the binding can be deleted */
{
  if (!ecma_op_has_binding (lex_env_p, var_name_str_p))
  {
    ecma_value_t completion_value = ecma_op_create_mutable_binding (lex_env_p,
                                                                    var_name_str_p,
                                                                    is_configurable_bindings);

    JERRY_ASSERT (ecma_is_value_empty (completion_value));

    /* Skipping SetMutableBinding as we have already checked that there were not
     * any binding with specified name in current lexical environment
     * and CreateMutableBinding sets the created binding's value to undefined */
    JERRY_ASSERT (ecma_is_value_undefined (ecma_op_get_binding_value (lex_env_p,
                                                                      var_name_str_p,
                                                                      vm_is_strict_mode ())));
  }
} /* vm_var_decl */

/**
 * Set var binding to a function literal value.
 */
inline void JERRY_ATTR_ALWAYS_INLINE
vm_set_var (ecma_object_t *lex_env_p, /**< target lexical environment */
            ecma_string_t *var_name_str_p, /**< variable name */
            bool is_strict, /**< true, if the engine is in strict mode */
            ecma_value_t lit_value) /**< function value */
{
  ecma_value_t put_value_result;
  put_value_result = ecma_op_put_value_lex_env_base (lex_env_p, var_name_str_p, is_strict, lit_value);

  JERRY_ASSERT (ecma_is_value_boolean (put_value_result)
                || ecma_is_value_empty (put_value_result)
                || ECMA_IS_VALUE_ERROR (put_value_result));

  if (ECMA_IS_VALUE_ERROR (put_value_result))
  {
    ecma_free_value (JERRY_CONTEXT (error_value));
  }

  ecma_free_value (lit_value);
} /* vm_set_var */

/**
 * 'typeof' opcode handler.
 *
 * See also: ECMA-262 v5, 11.4.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
opfunc_typeof (ecma_value_t left_value) /**< left value */
{
  return ecma_make_magic_string_value (ecma_get_typeof_lit_id (left_value));
} /* opfunc_typeof */

/**
 * Update getter or setter for object literals.
 */
void
opfunc_set_accessor (bool is_getter, /**< is getter accessor */
                     ecma_value_t object, /**< object value */
                     ecma_string_t *accessor_name_p, /**< accessor name */
                     ecma_value_t accessor) /**< accessor value */
{
  ecma_object_t *object_p = ecma_get_object_from_value (object);

  JERRY_ASSERT (!ecma_op_object_is_fast_array (object_p));

  ecma_property_t *property_p = ecma_find_named_property (object_p, accessor_name_p);

  if (property_p != NULL
      && ECMA_PROPERTY_GET_TYPE (*property_p) != ECMA_PROPERTY_TYPE_NAMEDACCESSOR)
  {
    ecma_delete_property (object_p, ECMA_PROPERTY_VALUE_PTR (property_p));
    property_p = NULL;
  }

  if (property_p == NULL)
  {
    ecma_object_t *getter_func_p = NULL;
    ecma_object_t *setter_func_p = NULL;

    if (is_getter)
    {
      getter_func_p = ecma_get_object_from_value (accessor);
    }
    else
    {
      setter_func_p = ecma_get_object_from_value (accessor);
    }

    ecma_create_named_accessor_property (object_p,
                                         accessor_name_p,
                                         getter_func_p,
                                         setter_func_p,
                                         ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE,
                                         NULL);
  }
  else if (is_getter)
  {
    ecma_object_t *getter_func_p = ecma_get_object_from_value (accessor);

    ecma_set_named_accessor_property_getter (object_p,
                                             ECMA_PROPERTY_VALUE_PTR (property_p),
                                             getter_func_p);
  }
  else
  {
    ecma_object_t *setter_func_p = ecma_get_object_from_value (accessor);

    ecma_set_named_accessor_property_setter (object_p,
                                             ECMA_PROPERTY_VALUE_PTR (property_p),
                                             setter_func_p);
  }
} /* opfunc_set_accessor */

/**
 * Deletes an object property.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
vm_op_delete_prop (ecma_value_t object, /**< base object */
                   ecma_value_t property, /**< property name */
                   bool is_strict) /**< strict mode */
{
  if (ecma_is_value_undefined (object))
  {
    return ECMA_VALUE_TRUE;
  }

  ecma_value_t check_coercible = ecma_op_check_object_coercible (object);
  if (ECMA_IS_VALUE_ERROR (check_coercible))
  {
    return check_coercible;
  }
  JERRY_ASSERT (check_coercible == ECMA_VALUE_EMPTY);

  ecma_string_t *name_string_p = ecma_op_to_prop_name (property);

  if (JERRY_UNLIKELY (name_string_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_value_t obj_value = ecma_op_to_object (object);
  /* The ecma_op_check_object_coercible call already checked the op_to_object error cases. */
  JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (obj_value));
  JERRY_ASSERT (ecma_is_value_object (obj_value));
  ecma_object_t *obj_p = ecma_get_object_from_value (obj_value);
  JERRY_ASSERT (!ecma_is_lexical_environment (obj_p));

  ecma_value_t delete_op_ret = ecma_op_object_delete (obj_p, name_string_p, is_strict);
  JERRY_ASSERT (ecma_is_value_boolean (delete_op_ret) || (is_strict == true && ECMA_IS_VALUE_ERROR (delete_op_ret)));
  ecma_deref_object (obj_p);
  ecma_deref_ecma_string (name_string_p);

  return delete_op_ret;
} /* vm_op_delete_prop */

/**
 * Deletes a variable.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
vm_op_delete_var (ecma_value_t name_literal, /**< name literal */
                  ecma_object_t *lex_env_p) /**< lexical environment */
{
  ecma_value_t completion_value = ECMA_VALUE_EMPTY;

  ecma_string_t *var_name_str_p = ecma_get_string_from_value (name_literal);

  ecma_object_t *ref_base_lex_env_p = ecma_op_resolve_reference_base (lex_env_p, var_name_str_p);

  if (ref_base_lex_env_p == NULL)
  {
    completion_value = ECMA_VALUE_TRUE;
  }
  else
  {
    JERRY_ASSERT (ecma_is_lexical_environment (ref_base_lex_env_p));

    completion_value = ecma_op_delete_binding (ref_base_lex_env_p, var_name_str_p);
  }

  return completion_value;
} /* vm_op_delete_var */

/**
 * 'for-in' opcode handler
 *
 * See also:
 *          ECMA-262 v5, 12.6.4
 *
 * @return chain list of property names
 */
ecma_collection_t *
opfunc_for_in (ecma_value_t left_value, /**< left value */
               ecma_value_t *result_obj_p) /**< expression object */
{
  /* 3. */
  if (ecma_is_value_undefined (left_value)
      || ecma_is_value_null (left_value))
  {
    return NULL;
  }

  /* 4. */
  ecma_value_t obj_expr_value = ecma_op_to_object (left_value);
  /* ecma_op_to_object will only raise error on null/undefined values but those are handled above. */
  JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (obj_expr_value));
  ecma_object_t *obj_p = ecma_get_object_from_value (obj_expr_value);
  ecma_collection_t *prop_names_p = ecma_op_object_get_property_names (obj_p, ECMA_LIST_ENUMERABLE_PROTOTYPE);

  if (prop_names_p->item_count != 0)
  {
    *result_obj_p = ecma_make_object_value (obj_p);
    return prop_names_p;
  }

  ecma_deref_object (obj_p);
  ecma_collection_destroy (prop_names_p);

  return NULL;
} /* opfunc_for_in */

#if ENABLED (JERRY_ES2015)
/**
 * 'VM_OC_APPEND_ARRAY' opcode handler specialized for spread objects
 *
 * @return ECMA_VALUE_ERROR - if the operation failed
 *         ECMA_VALUE_EMPTY, otherwise
 */
static ecma_value_t JERRY_ATTR_NOINLINE
opfunc_append_to_spread_array (ecma_value_t *stack_top_p, /**< current stack top */
                               uint16_t values_length) /**< number of elements to set */
{
  JERRY_ASSERT (!(values_length & OPFUNC_HAS_SPREAD_ELEMENT));

  ecma_object_t *array_obj_p = ecma_get_object_from_value (stack_top_p[-1]);
  JERRY_ASSERT (ecma_get_object_type (array_obj_p) == ECMA_OBJECT_TYPE_ARRAY);

  ecma_extended_object_t *ext_array_obj_p = (ecma_extended_object_t *) array_obj_p;
  uint32_t old_length = ext_array_obj_p->u.array.length;

  for (uint32_t i = 0, j = old_length; i < values_length; i++)
  {
    if (ecma_is_value_array_hole (stack_top_p[i]))
    {
      continue;
    }
    if (ecma_op_is_spread_object (stack_top_p[i]))
    {
      ecma_value_t ret_value = ECMA_VALUE_ERROR;
      ecma_object_t *spread_object_p = ecma_get_object_from_value (stack_top_p[i]);
      ecma_value_t spread_value = ecma_op_spread_object_get_spreaded_element (spread_object_p);

      ecma_value_t iterator = ecma_op_get_iterator (spread_value, ECMA_VALUE_EMPTY);

      if (!ECMA_IS_VALUE_ERROR (iterator))
      {
        while (true)
        {
          ecma_value_t next = ecma_op_iterator_step (iterator);

          if (ECMA_IS_VALUE_ERROR (next))
          {
            break;
          }

          if (ecma_is_value_false (next))
          {
            j--;
            ret_value = ECMA_VALUE_EMPTY;
            break;
          }

          ecma_value_t value = ecma_op_iterator_value (next);

          ecma_free_value (next);

          if (ECMA_IS_VALUE_ERROR (value))
          {
            break;
          }

          ecma_value_t put_comp;
          put_comp = ecma_builtin_helper_def_prop_by_index (array_obj_p,
                                                            i + j,
                                                            value,
                                                            ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);

          j++;
          JERRY_ASSERT (ecma_is_value_true (put_comp));
          ecma_free_value (value);
        }
      }

      ecma_free_value (iterator);
      ecma_free_value (spread_value);

      if (ECMA_IS_VALUE_ERROR (ret_value))
      {
        for (uint32_t k = i; k < values_length; k++)
        {
          ecma_free_value (stack_top_p[k]);
        }

        return ret_value;
      }
      else
      {
        ecma_deref_object (spread_object_p);
      }
    }
    else
    {
      ecma_value_t put_comp = ecma_builtin_helper_def_prop_by_index (array_obj_p,
                                                                     i + j,
                                                                     stack_top_p[i],
                                                                     ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);
      JERRY_ASSERT (ecma_is_value_true (put_comp));
      ecma_free_value (stack_top_p[i]);
    }
  }

  return ECMA_VALUE_EMPTY;
} /* opfunc_append_to_spread_array */
#endif /* ENABLED (JERRY_ES2015) */

/**
 * 'VM_OC_APPEND_ARRAY' opcode handler, for setting array object properties
 *
 * @return ECMA_VALUE_ERROR - if the operation failed
 *         ECMA_VALUE_EMPTY, otherwise
 */
ecma_value_t JERRY_ATTR_NOINLINE
opfunc_append_array (ecma_value_t *stack_top_p, /**< current stack top */
                     uint16_t values_length) /**< number of elements to set
                                              *   with potential OPFUNC_HAS_SPREAD_ELEMENT flag */
{
#if ENABLED (JERRY_ES2015)
  if (values_length >= OPFUNC_HAS_SPREAD_ELEMENT)
  {
    return opfunc_append_to_spread_array (stack_top_p, (uint16_t) (values_length & ~OPFUNC_HAS_SPREAD_ELEMENT));
  }
#endif /* ENABLED (JERRY_ES2015) */

  ecma_object_t *array_obj_p = ecma_get_object_from_value (stack_top_p[-1]);
  JERRY_ASSERT (ecma_get_object_type (array_obj_p) == ECMA_OBJECT_TYPE_ARRAY);

  ecma_extended_object_t *ext_array_obj_p = (ecma_extended_object_t *) array_obj_p;
  uint32_t old_length = ext_array_obj_p->u.array.length;

  if (JERRY_LIKELY (ecma_op_array_is_fast_array (ext_array_obj_p)))
  {
    uint32_t filled_holes = 0;
    ecma_value_t *values_p = ecma_fast_array_extend (array_obj_p, old_length + values_length);

    for (uint32_t i = 0; i < values_length; i++)
    {
      values_p[old_length + i] = stack_top_p[i];

      if (!ecma_is_value_array_hole (stack_top_p[i]))
      {
        filled_holes++;

        if (ecma_is_value_object (stack_top_p[i]))
        {
          ecma_deref_object (ecma_get_object_from_value (stack_top_p[i]));
        }
      }
    }

    ext_array_obj_p->u.array.u.hole_count -= filled_holes * ECMA_FAST_ARRAY_HOLE_ONE;

    if (JERRY_UNLIKELY ((values_length - filled_holes) > ECMA_FAST_ARRAY_MAX_NEW_HOLES_COUNT))
    {
      ecma_fast_array_convert_to_normal (array_obj_p);
    }
  }
  else
  {
    for (uint32_t i = 0; i < values_length; i++)
    {
      if (!ecma_is_value_array_hole (stack_top_p[i]))
      {
        ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (old_length + i);

        ecma_property_value_t *prop_value_p;

        prop_value_p = ecma_create_named_data_property (array_obj_p,
                                                        index_str_p,
                                                        ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE,
                                                        NULL);

        ecma_deref_ecma_string (index_str_p);
        prop_value_p->value = stack_top_p[i];

        if (ecma_is_value_object (stack_top_p[i]))
        {
          ecma_free_value (stack_top_p[i]);
        }

      }

      ext_array_obj_p->u.array.length = old_length + values_length;
    }
  }

  return ECMA_VALUE_EMPTY;
} /* opfunc_append_array */

/**
 * @}
 * @}
 */
