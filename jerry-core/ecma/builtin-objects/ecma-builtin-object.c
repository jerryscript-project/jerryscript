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
#include "ecma-builtin-helpers.h"
#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-objects-general.h"
#include "jrt.h"
#include "ecma-builtin-object.h"

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

/**
 * This object has a custom dispatch function.
 */
#define BUILTIN_CUSTOM_DISPATCH

/**
 * List of built-in routine identifiers.
 */
enum
{
  ECMA_OBJECT_ROUTINE_START = ECMA_BUILTIN_ID__COUNT - 1,
  ECMA_OBJECT_ROUTINE_DEFINE_PROPERTY,
  ECMA_OBJECT_ROUTINE_GET_OWN_PROPERTY_DESCRIPTOR,
  ECMA_OBJECT_ROUTINE_DEFINE_PROPERTIES,
  ECMA_OBJECT_ROUTINE_CREATE,
  ECMA_OBJECT_ROUTINE_SEAL,
  ECMA_OBJECT_ROUTINE_FREEZE,
  ECMA_OBJECT_ROUTINE_PREVENT_EXTENSIONS,
  ECMA_OBJECT_ROUTINE_IS_SEALED,
  ECMA_OBJECT_ROUTINE_IS_FROZEN,
  ECMA_OBJECT_ROUTINE_IS_EXTENSIBLE,
  ECMA_OBJECT_ROUTINE_GET_OWN_PROPERTY_NAMES,
  ECMA_OBJECT_ROUTINE_GET_OWN_PROPERTY_SYMBOLS,
  ECMA_OBJECT_ROUTINE_KEYS,
  ECMA_OBJECT_ROUTINE_GET_PROTOTYPE_OF,
  ECMA_OBJECT_ROUTINE_SET_PROTOTYPE_OF,
  ECMA_OBJECT_ROUTINE_ASSIGN,
  ECMA_OBJECT_ROUTINE_IS,
};

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-object.inc.h"
#define BUILTIN_UNDERSCORED_ID object
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup object ECMA Object object built-in
 * @{
 */

/**
 * Handle calling [[Call]] of built-in Object object
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_object_dispatch_call (const ecma_value_t *arguments_list_p, /**< arguments list */
                                   ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  if (arguments_list_len == 0
      || ecma_is_value_undefined (arguments_list_p[0])
      || ecma_is_value_null (arguments_list_p[0]))
  {
    return ecma_builtin_object_dispatch_construct (arguments_list_p, arguments_list_len);
  }

  return ecma_op_to_object (arguments_list_p[0]);
} /* ecma_builtin_object_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in Object object
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_object_dispatch_construct (const ecma_value_t *arguments_list_p, /**< arguments list */
                                        ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  if (arguments_list_len == 0)
  {
    ecma_object_t *obj_p = ecma_op_create_object_object_noarg ();

    return ecma_make_object_value (obj_p);
  }

  return ecma_op_create_object_object_arg (arguments_list_p[0]);
} /* ecma_builtin_object_dispatch_construct */

/**
 * The Object object's 'getPrototypeOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.3.2
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_object_object_get_prototype_of (ecma_value_t arg) /**< routine's argument */
{
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;
  bool was_object = ecma_is_value_object (arg);

  /* 1. */
  if (!was_object)
  {
#if ENABLED (JERRY_ES2015_BUILTIN)
    arg = ecma_op_to_object (arg);
    if (ECMA_IS_VALUE_ERROR (arg))
    {
      return arg;
    }
#else /* !ENABLED (JERRY_ES2015_BUILTIN) */
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument is not an object."));
#endif /* ENABLED (JERRY_ES2015_BUILTIN) */
  }
  /* 2. */
  ecma_object_t *obj_p = ecma_get_object_from_value (arg);
  jmem_cpointer_t prototype_cp = obj_p->u2.prototype_cp;

  if (prototype_cp != JMEM_CP_NULL)
  {
    ecma_object_t *prototype_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, prototype_cp);
    ret_value = ecma_make_object_value (prototype_p);
    ecma_ref_object (prototype_p);
  }
  else
  {
    ret_value = ECMA_VALUE_NULL;
  }

#if ENABLED (JERRY_ES2015_BUILTIN)
  if (!was_object)
  {
    ecma_deref_object (obj_p);
  }
#endif /* ENABLED (JERRY_ES2015_BUILTIN) */

  return ret_value;
} /* ecma_builtin_object_object_get_prototype_of */

#if ENABLED (JERRY_ES2015_BUILTIN)
/**
 * [[SetPrototypeOf]]
 *
 * See also:
 *          ES2015 9.1.2
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_set_prototype_of (ecma_value_t o_value, /**< O */
                       ecma_value_t v_value) /**< V */
{
  /* 1. */
  JERRY_ASSERT (ecma_is_value_object (o_value));
  JERRY_ASSERT (ecma_is_value_object (v_value) || ecma_is_value_null (v_value));

  ecma_object_t *o_p = ecma_get_object_from_value (o_value);

  jmem_cpointer_t v_cp;

  if (ecma_is_value_null (v_value))
  {
    v_cp = JMEM_CP_NULL;
  }
  else
  {
    ECMA_SET_NON_NULL_POINTER (v_cp, ecma_get_object_from_value (v_value));
  }


  /* 3., 4. */
  if (v_cp == o_p->u2.prototype_cp)
  {
    ecma_ref_object (o_p);
    return ecma_make_object_value (o_p);
  }

  /* 2., 5. */
  if (!ecma_get_object_extensible (o_p))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("cannot set prototype."));
  }

  /* 6., 7., 8. */
  jmem_cpointer_t p_cp = v_cp;
  while (p_cp != JMEM_CP_NULL)
  {
    ecma_object_t *p_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, p_cp);

    /* b. */
    if (p_p == o_p)
    {
      return ecma_raise_type_error (ECMA_ERR_MSG ("cannot set prototype."));
    }

    /* c.i. TODO: es2015-subset profile does not support having a different
     * [[GetPrototypeOf]] internal method */

    /* c.ii. */
    p_cp = p_p->u2.prototype_cp;
  }

  /* 9. */
  o_p->u2.prototype_cp = v_cp;

  /* 10. */
  ecma_ref_object (o_p);
  return ecma_make_object_value (o_p);
} /* ecma_set_prototype_of */

/**
 * The Object object's 'setPrototypeOf' routine
 *
 * See also:
 *          ES2015 19.1.2.18
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_object_object_set_prototype_of (ecma_value_t arg1, /**< routine's first argument */
                                             ecma_value_t arg2) /**< routine's second argument */
{
  /* 1., 2. */
  if (ECMA_IS_VALUE_ERROR (ecma_op_check_object_coercible (arg1)))
  {
    return ECMA_VALUE_ERROR;
  }

  /* 3. */
  if (!ecma_is_value_object (arg2) && !ecma_is_value_null (arg2))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("proto is neither Object nor Null."));
  }

  /* 4. */
  if (!ecma_is_value_object (arg1))
  {
    return ecma_copy_value (arg1);
  }

  /* 6. TODO: es2015-subset profile does not support having a different
   * [[SetPrototypeOf]] internal method */

  /* 5-8. */
  return ecma_set_prototype_of (arg1, arg2);
} /* ecma_builtin_object_object_set_prototype_of */
#endif /* ENABLED (JERRY_ES2015_BUILTIN) */

/**
 * The Object object's 'getOwnPropertyNames' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.3.4
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_object_object_get_own_property_names (ecma_object_t *obj_p) /**< routine's argument */
{
  return ecma_builtin_helper_object_get_properties (obj_p, ECMA_LIST_NO_OPTS);
} /* ecma_builtin_object_object_get_own_property_names */

#if ENABLED (JERRY_ES2015)
/**
 * The Object object's 'getOwnPropertySymbols' routine
 *
 * See also:
 *          ECMA-262 v6, 19.1.2.7
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_object_object_get_own_property_symbols (ecma_object_t *obj_p) /**< routine's argument */
{
  return ecma_builtin_helper_object_get_properties (obj_p, ECMA_LIST_SYMBOLS);
} /* ecma_builtin_object_object_get_own_property_symbols */
#endif /* ENABLED (JERRY_ES2015) */

/**
 * The Object object's 'seal' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.3.8
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_object_object_seal (ecma_object_t *obj_p) /**< routine's argument */
{
  ecma_collection_t *props_p = ecma_op_object_get_property_names (obj_p, ECMA_LIST_CONVERT_FAST_ARRAYS);

  ecma_value_t *buffer_p = props_p->buffer_p;

  for (uint32_t i = 0; i < props_p->item_count; i++)
  {
    ecma_string_t *property_name_p = ecma_get_string_from_value (buffer_p[i]);

    /* 2.a */
    ecma_property_descriptor_t prop_desc;

    if (!ecma_op_object_get_own_property_descriptor (obj_p, property_name_p, &prop_desc))
    {
      continue;
    }

    /* 2.b */
    prop_desc.flags &= (uint16_t) ~ECMA_PROP_IS_CONFIGURABLE;
    prop_desc.flags |= ECMA_PROP_IS_THROW;

    /* 2.c */
    ecma_value_t define_own_prop_ret = ecma_op_object_define_own_property (obj_p,
                                                                           property_name_p,
                                                                           &prop_desc);

    ecma_free_property_descriptor (&prop_desc);

    if (ECMA_IS_VALUE_ERROR (define_own_prop_ret))
    {
      ecma_collection_free (props_p);
      return define_own_prop_ret;
    }

    ecma_free_value (define_own_prop_ret);
  }

  ecma_collection_free (props_p);

  /* 3. */
  ecma_set_object_extensible (obj_p, false);

  /* 4. */
  ecma_ref_object (obj_p);
  return ecma_make_object_value (obj_p);
} /* ecma_builtin_object_object_seal */

/**
 * The Object object's 'freeze' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.3.9
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_object_object_freeze (ecma_object_t *obj_p) /**< routine's argument */
{
  ecma_collection_t *props_p = ecma_op_object_get_property_names (obj_p, ECMA_LIST_CONVERT_FAST_ARRAYS);

  ecma_value_t *buffer_p = props_p->buffer_p;

  for (uint32_t i = 0; i < props_p->item_count; i++)
  {
    ecma_string_t *property_name_p = ecma_get_string_from_value (buffer_p[i]);

    /* 2.a */
    ecma_property_descriptor_t prop_desc;

    if (!ecma_op_object_get_own_property_descriptor (obj_p, property_name_p, &prop_desc))
    {
      continue;
    }

    /* 2.b */
    if ((prop_desc.flags & (ECMA_PROP_IS_WRITABLE_DEFINED | ECMA_PROP_IS_WRITABLE))
        == (ECMA_PROP_IS_WRITABLE_DEFINED | ECMA_PROP_IS_WRITABLE))
    {
      prop_desc.flags &= (uint16_t) ~ECMA_PROP_IS_WRITABLE;
    }

    /* 2.c */
    prop_desc.flags &= (uint16_t) ~ECMA_PROP_IS_CONFIGURABLE;
    prop_desc.flags |= ECMA_PROP_IS_THROW;

    /* 2.d */
    ecma_value_t define_own_prop_ret = ecma_op_object_define_own_property (obj_p,
                                                                           property_name_p,
                                                                           &prop_desc);

    ecma_free_property_descriptor (&prop_desc);

    if (ECMA_IS_VALUE_ERROR (define_own_prop_ret))
    {
      ecma_collection_free (props_p);
      return define_own_prop_ret;
    }

    ecma_free_value (define_own_prop_ret);
  }

  ecma_collection_free (props_p);

  /* 3. */
  ecma_set_object_extensible (obj_p, false);

  /* 4. */
  ecma_ref_object (obj_p);
  return ecma_make_object_value (obj_p);
} /* ecma_builtin_object_object_freeze */

/**
 * The Object object's 'preventExtensions' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.3.10
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_object_object_prevent_extensions (ecma_object_t *obj_p) /**< routine's argument */
{
  ecma_set_object_extensible (obj_p, false);
  ecma_ref_object (obj_p);

  return ecma_make_object_value (obj_p);
} /* ecma_builtin_object_object_prevent_extensions */

/**
 * The Object object's 'isSealed' and 'isFrozen' routines
 *
 * See also:
 *         ECMA-262 v5, 15.2.3.11
 *         ECMA-262 v5, 15.2.3.12
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_object_frozen_or_sealed_helper (ecma_object_t *obj_p, /**< routine's argument */
                                             int mode) /**< routine mode */
{
  JERRY_ASSERT (mode == ECMA_OBJECT_ROUTINE_IS_FROZEN || mode == ECMA_OBJECT_ROUTINE_IS_SEALED);

  /* 3. */
  if (ecma_get_object_extensible (obj_p))
  {
    return ECMA_VALUE_FALSE;
  }

  /* the value can be updated in the loop below */
  ecma_value_t ret_value = ECMA_VALUE_TRUE;

  /* 2. */
  ecma_collection_t *props_p = ecma_op_object_get_property_names (obj_p, ECMA_LIST_NO_OPTS);

  ecma_value_t *buffer_p = props_p->buffer_p;

  for (uint32_t i = 0; i < props_p->item_count; i++)
  {
    ecma_string_t *property_name_p = ecma_get_string_from_value (buffer_p[i]);

    /* 2.a */
    ecma_property_t property = ecma_op_object_get_own_property (obj_p,
                                                                property_name_p,
                                                                NULL,
                                                                ECMA_PROPERTY_GET_NO_OPTIONS);

    /* 2.b for isFrozen */
    if (mode == ECMA_OBJECT_ROUTINE_IS_FROZEN
        && ECMA_PROPERTY_GET_TYPE (property) != ECMA_PROPERTY_TYPE_NAMEDACCESSOR
        && ecma_is_property_writable (property))
    {
      ret_value = ECMA_VALUE_FALSE;
      break;
    }

    /* 2.b for isSealed, 2.c for isFrozen */
    if (ecma_is_property_configurable (property))
    {
      ret_value = ECMA_VALUE_FALSE;
      break;
    }
  }

  ecma_collection_free (props_p);

  return ret_value;
} /* ecma_builtin_object_frozen_or_sealed_helper */

/**
 * The Object object's 'isExtensible' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.3.13
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_object_object_is_extensible (ecma_object_t *obj_p) /**< routine's argument */
{
  return ecma_make_boolean_value (ecma_get_object_extensible (obj_p));
} /* ecma_builtin_object_object_is_extensible */

/**
 * The Object object's 'keys' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.3.14
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_object_object_keys (ecma_object_t *obj_p) /**< routine's argument */
{
  return ecma_builtin_helper_object_get_properties (obj_p, ECMA_LIST_ENUMERABLE);
} /* ecma_builtin_object_object_keys */

/**
 * The Object object's 'getOwnPropertyDescriptor' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.3.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_object_object_get_own_property_descriptor (ecma_object_t *obj_p, /**< routine's first argument */
                                                        ecma_string_t *name_str_p) /**< routine's second argument */
{
  /* 3. */
  ecma_property_descriptor_t prop_desc;

  if (ecma_op_object_get_own_property_descriptor (obj_p, name_str_p, &prop_desc))
  {
    /* 4. */
    ecma_object_t *desc_obj_p = ecma_op_from_property_descriptor (&prop_desc);

    ecma_free_property_descriptor (&prop_desc);

    return ecma_make_object_value (desc_obj_p);
  }

  return ECMA_VALUE_UNDEFINED;
} /* ecma_builtin_object_object_get_own_property_descriptor */

/**
 * The Object object's 'defineProperties' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.3.7
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_object_object_define_properties (ecma_object_t *obj_p, /**< routine's first argument */
                                              ecma_value_t arg2) /**< routine's second argument */
{
  /* 2. */
  ecma_value_t props = ecma_op_to_object (arg2);

  if (ECMA_IS_VALUE_ERROR (props))
  {
    return props;
  }

  ecma_object_t *props_p = ecma_get_object_from_value (props);
  /* 3. */
  ecma_collection_t *prop_names_p = ecma_op_object_get_property_names (props_p, ECMA_LIST_CONVERT_FAST_ARRAYS
                                                                                | ECMA_LIST_ENUMERABLE);
  ecma_value_t ret_value = ECMA_VALUE_ERROR;

  ecma_value_t *buffer_p = prop_names_p->buffer_p;

  /* 4. */
  JMEM_DEFINE_LOCAL_ARRAY (property_descriptors, prop_names_p->item_count, ecma_property_descriptor_t);
  uint32_t property_descriptor_number = 0;

  for (uint32_t i = 0; i < prop_names_p->item_count; i++)
  {
    /* 5.a */
    ecma_value_t desc_obj = ecma_op_object_get (props_p, ecma_get_string_from_value (buffer_p[i]));

    if (ECMA_IS_VALUE_ERROR (desc_obj))
    {
      goto cleanup;
    }

    /* 5.b */
    ecma_value_t conv_result = ecma_op_to_property_descriptor (desc_obj,
                                                               &property_descriptors[property_descriptor_number]);

    property_descriptors[property_descriptor_number].flags |= ECMA_PROP_IS_THROW;

    ecma_free_value (desc_obj);

    if (ECMA_IS_VALUE_ERROR (conv_result))
    {
      goto cleanup;
    }

    property_descriptor_number++;

    ecma_free_value (conv_result);
  }

  /* 6. */
  buffer_p = prop_names_p->buffer_p;

  for (uint32_t i = 0; i < prop_names_p->item_count; i++)
  {
    ecma_value_t define_own_prop_ret =  ecma_op_object_define_own_property (obj_p,
                                                                            ecma_get_string_from_value (buffer_p[i]),
                                                                            &property_descriptors[i]);
    if (ECMA_IS_VALUE_ERROR (define_own_prop_ret))
    {
      goto cleanup;
    }

    ecma_free_value (define_own_prop_ret);
  }

  ecma_ref_object (obj_p);
  ret_value = ecma_make_object_value (obj_p);

cleanup:
  /* Clean up. */
  for (uint32_t index = 0;
       index < property_descriptor_number;
       index++)
  {
    ecma_free_property_descriptor (&property_descriptors[index]);
  }

  JMEM_FINALIZE_LOCAL_ARRAY (property_descriptors);

  ecma_collection_free (prop_names_p);

  ecma_deref_object (props_p);

  return ret_value;
} /* ecma_builtin_object_object_define_properties */

/**
 * The Object object's 'create' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.3.5
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_object_object_create (ecma_value_t arg1, /**< routine's first argument */
                                   ecma_value_t arg2) /**< routine's second argument */
{
  /* 1. */
  if (!ecma_is_value_object (arg1) && !ecma_is_value_null (arg1))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument is not an object."));
  }

  ecma_object_t *obj_p = NULL;

  if (!ecma_is_value_null (arg1))
  {
    obj_p = ecma_get_object_from_value (arg1);
  }
  /* 2-3. */
  ecma_object_t *result_obj_p = ecma_op_create_object_object_noarg_and_set_prototype (obj_p);

  /* 4. */
  if (!ecma_is_value_undefined (arg2))
  {
    ecma_value_t obj = ecma_builtin_object_object_define_properties (result_obj_p, arg2);

    if (ECMA_IS_VALUE_ERROR (obj))
    {
      ecma_deref_object (result_obj_p);
      return obj;
    }

    ecma_free_value (obj);
  }

  /* 5. */
  return ecma_make_object_value (result_obj_p);
} /* ecma_builtin_object_object_create */

/**
 * The Object object's 'defineProperty' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.3.6
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_object_object_define_property (ecma_object_t *obj_p, /**< routine's first argument */
                                            ecma_string_t *name_str_p, /**< routine's second argument */
                                            ecma_value_t arg3) /**< routine's third argument */
{
  ecma_property_descriptor_t prop_desc;

  ecma_value_t conv_result = ecma_op_to_property_descriptor (arg3, &prop_desc);

  if (ECMA_IS_VALUE_ERROR (conv_result))
  {
    return conv_result;
  }

  prop_desc.flags |= ECMA_PROP_IS_THROW;

  ecma_value_t define_own_prop_ret = ecma_op_object_define_own_property (obj_p,
                                                                         name_str_p,
                                                                         &prop_desc);

  ecma_free_property_descriptor (&prop_desc);
  ecma_free_value (conv_result);

  if (ECMA_IS_VALUE_ERROR (define_own_prop_ret))
  {
    return define_own_prop_ret;
  }

  ecma_ref_object (obj_p);
  ecma_free_value (define_own_prop_ret);

  return ecma_make_object_value (obj_p);
} /* ecma_builtin_object_object_define_property */


#if ENABLED (JERRY_ES2015_BUILTIN)
/**
 * The Object object's 'assign' routine
 *
 * See also:
 *          ECMA-262 v6, 19.1.2.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_object_object_assign (const ecma_value_t arguments_list_p[], /**< arguments list */
                                   ecma_length_t arguments_list_len) /**< number of arguments */
{
  ecma_value_t target = arguments_list_len > 0 ? arguments_list_p[0] : ECMA_VALUE_UNDEFINED;

  /* 1. */
  ecma_value_t to_value = ecma_op_to_object (target);

  if (ECMA_IS_VALUE_ERROR (to_value))
  {
    return to_value;
  }

  ecma_object_t *to_obj_p = ecma_get_object_from_value (to_value);

  /* 2. */
  if (arguments_list_len == 1)
  {
    return to_value;
  }

  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  /* 4-5. */
  for (uint32_t i = 1; i < arguments_list_len && ecma_is_value_empty (ret_value); i++)
  {
    ecma_value_t next_source = arguments_list_p[i];

    /* 5.a */
    if (ecma_is_value_undefined (next_source) || ecma_is_value_null (next_source))
    {
      continue;
    }

    /* 5.b.i */
    ecma_value_t from_value = ecma_op_to_object (next_source);
    /* null and undefied cases are handled above, so this must be a valid object */
    JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (from_value));

    ecma_object_t *from_obj_p = ecma_get_object_from_value (from_value);

    /* 5.b.iii */
    /* TODO: extends this collection if symbols will be supported */
    ecma_collection_t *props_p = ecma_op_object_get_property_names (from_obj_p, ECMA_LIST_CONVERT_FAST_ARRAYS
                                                                                | ECMA_LIST_ENUMERABLE);

    ecma_value_t *buffer_p = props_p->buffer_p;

    for (uint32_t j = 0; (j < props_p->item_count) && ecma_is_value_empty (ret_value); j++)
    {
      ecma_string_t *property_name_p = ecma_get_string_from_value (buffer_p[j]);

      /* 5.c.i-ii */
      ecma_property_descriptor_t prop_desc;

      if (!ecma_op_object_get_own_property_descriptor (from_obj_p, property_name_p, &prop_desc))
      {
        continue;
      }

      /* 5.c.iii */
      if ((prop_desc.flags & ECMA_PROP_IS_ENUMERABLE)
          && (((prop_desc.flags & ECMA_PROP_IS_VALUE_DEFINED) && !ecma_is_value_undefined (prop_desc.value))
              || (prop_desc.flags & ECMA_PROP_IS_GET_DEFINED)))
      {
        /* 5.c.iii.1 */
        ecma_value_t prop_value = ecma_op_object_get (from_obj_p, property_name_p);

        /* 5.c.iii.2 */
        if (ECMA_IS_VALUE_ERROR (prop_value))
        {
          ret_value = prop_value;
        }
        else
        {
          /* 5.c.iii.3 */
          ecma_value_t status = ecma_op_object_put (to_obj_p, property_name_p, prop_value, true);

          /* 5.c.iii.4 */
          if (ECMA_IS_VALUE_ERROR (status))
          {
            ret_value = status;
          }
        }

        ecma_free_value (prop_value);
        ecma_free_property_descriptor (&prop_desc);
      }
    }

    ecma_deref_object (from_obj_p);
    ecma_collection_free (props_p);
  }

  /* 6. */
  if (ecma_is_value_empty (ret_value))
  {
    return to_value;
  }

  ecma_deref_object (to_obj_p);
  return ret_value;
} /* ecma_builtin_object_object_assign */
#endif /* ENABLED (JERRY_ES2015_BUILTIN) */

#if ENABLED (JERRY_ES2015_BUILTIN)
/**
 * The Object object's 'is' routine
 *
 * See also:
 *          ECMA-262 v6, 19.1.2.10
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_object_object_is (ecma_value_t arg1, /**< routine's first argument */
                               ecma_value_t arg2) /**< routine's second argument */
{
  return ecma_op_same_value (arg1, arg2) ? ECMA_VALUE_TRUE : ECMA_VALUE_FALSE;
} /* ecma_builtin_object_object_is */
#endif /* ENABLED (JERRY_ES2015_BUILTIN) */


/**
 * Dispatcher of the built-in's routines
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_object_dispatch_routine (uint16_t builtin_routine_id, /**< built-in wide routine
                                                                    *   identifier */
                                      ecma_value_t this_arg, /**< 'this' argument value */
                                      const ecma_value_t arguments_list_p[], /**< list of arguments
                                                                              *   passed to routine */
                                      ecma_length_t arguments_number) /**< length of arguments' list */
{
  JERRY_UNUSED (this_arg);
  JERRY_UNUSED (arguments_list_p);
  JERRY_UNUSED (arguments_number);

  ecma_value_t arg1 = arguments_list_p[0];
  ecma_value_t arg2 = arguments_list_p[1];

  /* No specialization for the arguments */
  switch (builtin_routine_id)
  {
    case ECMA_OBJECT_ROUTINE_CREATE:
    {
      return ecma_builtin_object_object_create (arg1, arg2);
    }
    case ECMA_OBJECT_ROUTINE_GET_PROTOTYPE_OF:
    {
      return ecma_builtin_object_object_get_prototype_of (arg1);
    }
#if ENABLED (JERRY_ES2015_BUILTIN)
    case ECMA_OBJECT_ROUTINE_SET_PROTOTYPE_OF:
    {
      return ecma_builtin_object_object_set_prototype_of (arg1, arg2);
    }
    case ECMA_OBJECT_ROUTINE_ASSIGN:
    {
      return ecma_builtin_object_object_assign (arguments_list_p, arguments_number);
    }
    case ECMA_OBJECT_ROUTINE_IS:
    {
      return ecma_builtin_object_object_is (arg1, arg2);
    }
#endif /* ENABLED (JERRY_ES2015_BUILTIN) */
    default:
    {
      break;
    }
  }

  if (!ecma_is_value_object (arg1))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument is not an object."));
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (arg1);

  if (builtin_routine_id <= ECMA_OBJECT_ROUTINE_GET_OWN_PROPERTY_DESCRIPTOR)
  {
    ecma_string_t *prop_name_p = ecma_op_to_prop_name (arg2);

    if (prop_name_p == NULL)
    {
      return ECMA_VALUE_ERROR;
    }

    ecma_value_t ret_value;

    if (builtin_routine_id == ECMA_OBJECT_ROUTINE_DEFINE_PROPERTY)
    {
      ret_value = ecma_builtin_object_object_define_property (obj_p, prop_name_p, arguments_list_p[2]);
    }
    else
    {
      JERRY_ASSERT (builtin_routine_id == ECMA_OBJECT_ROUTINE_GET_OWN_PROPERTY_DESCRIPTOR);
      ret_value = ecma_builtin_object_object_get_own_property_descriptor (obj_p, prop_name_p);
    }

    ecma_deref_ecma_string (prop_name_p);
    return ret_value;
  }

  switch (builtin_routine_id)
  {
    case ECMA_OBJECT_ROUTINE_SEAL:
    {
      return ecma_builtin_object_object_seal (obj_p);
    }
    case ECMA_OBJECT_ROUTINE_FREEZE:
    {
      return ecma_builtin_object_object_freeze (obj_p);
    }
    case ECMA_OBJECT_ROUTINE_PREVENT_EXTENSIONS:
    {
      return ecma_builtin_object_object_prevent_extensions (obj_p);
    }
    case ECMA_OBJECT_ROUTINE_IS_SEALED:
    case ECMA_OBJECT_ROUTINE_IS_FROZEN:
    {
      return ecma_builtin_object_frozen_or_sealed_helper (obj_p,
                                                          builtin_routine_id);
    }
    case ECMA_OBJECT_ROUTINE_IS_EXTENSIBLE:
    {
      return ecma_builtin_object_object_is_extensible (obj_p);
    }
    case ECMA_OBJECT_ROUTINE_GET_OWN_PROPERTY_NAMES:
    {
      return ecma_builtin_object_object_get_own_property_names (obj_p);
    }
#if ENABLED (JERRY_ES2015)
    case ECMA_OBJECT_ROUTINE_GET_OWN_PROPERTY_SYMBOLS:
    {
      return ecma_builtin_object_object_get_own_property_symbols (obj_p);
    }
#endif /* ENABLED (JERRY_ES2015) */
    case ECMA_OBJECT_ROUTINE_KEYS:
    {
      return ecma_builtin_object_object_keys (obj_p);
    }
    case ECMA_OBJECT_ROUTINE_DEFINE_PROPERTIES:
    {
      return ecma_builtin_object_object_define_properties (obj_p, arg2);
    }
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }
} /* ecma_builtin_object_dispatch_routine */

/**
 * @}
 * @}
 * @}
 */
