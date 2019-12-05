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
#include "ecma-builtins.h"
#include "ecma-builtin-helpers.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-iterator-object.h"
#include "ecma-objects.h"
#include "ecma-array-object.h"
#include "jrt.h"

#if ENABLED (JERRY_BUILTIN_ARRAY)

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-array.inc.h"
#define BUILTIN_UNDERSCORED_ID array
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup array ECMA Array object built-in
 * @{
 */

/**
 * The Array object's 'isArray' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.3.2
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_object_is_array (ecma_value_t this_arg, /**< 'this' argument */
                                    ecma_value_t arg) /**< first argument */
{
  JERRY_UNUSED (this_arg);

  return ecma_make_boolean_value (ecma_is_value_array (arg));
} /* ecma_builtin_array_object_is_array */

#if ENABLED (JERRY_ES2015)
/**
 * The Array object's 'from' routine
 *
 * See also:
 *          ECMA-262 v6, 22.1.2.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_object_from (ecma_value_t this_arg, /**< 'this' argument */
                                const ecma_value_t *arguments_list_p, /**< arguments list */
                                ecma_length_t arguments_list_len) /**< number of arguments */
{
  /* 1. */
  ecma_value_t constructor = this_arg;
  ecma_value_t call_this_arg = ECMA_VALUE_UNDEFINED;
  ecma_value_t items = arguments_list_p[0];
  ecma_value_t mapfn = (arguments_list_len > 1) ? arguments_list_p[1] : ECMA_VALUE_UNDEFINED;

  /* 2. */
  ecma_object_t *mapfn_obj_p = NULL;

  /* 3. */
  if (!ecma_is_value_undefined (mapfn))
  {
    /* 3.a */
    if (!ecma_op_is_callable (mapfn))
    {
      return ecma_raise_type_error (ECMA_ERR_MSG ("Callback function is not callable."));
    }

    /* 3.b */
    if (arguments_list_len > 2)
    {
      call_this_arg = arguments_list_p[2];
    }

    /* 3.c */
    mapfn_obj_p = ecma_get_object_from_value (mapfn);
  }

  /* 4. */
  ecma_value_t using_iterator = ecma_op_get_method_by_symbol_id (items, LIT_MAGIC_STRING_ITERATOR);

  /* 5. */
  if (ECMA_IS_VALUE_ERROR (using_iterator))
  {
    return using_iterator;
  }

  ecma_value_t ret_value = ECMA_VALUE_ERROR;

  /* 6. */
  if (!ecma_is_value_undefined (using_iterator))
  {
    ecma_value_t array;

    /* 6.a */
    if (ecma_is_constructor (constructor))
    {
      ecma_object_t *constructor_obj_p = ecma_get_object_from_value (constructor);

      array = ecma_op_function_construct (constructor_obj_p, ECMA_VALUE_UNDEFINED, NULL, 0);

      if (ecma_is_value_undefined (array) || ecma_is_value_null (array))
      {
        ecma_free_value (using_iterator);
        return ecma_raise_type_error (ECMA_ERR_MSG ("Cannot convert undefined or null to object"));
      }
    }
    else
    {
      /* 6.b */
      array = ecma_op_create_array_object (NULL, 0, false);
    }

    /* 6.c */
    if (ECMA_IS_VALUE_ERROR (array))
    {
      ecma_free_value (using_iterator);
      return array;
    }

    ecma_object_t *array_obj_p = ecma_get_object_from_value (array);

    /* 6.d */
    ecma_value_t iterator = ecma_op_get_iterator (items, using_iterator);
    ecma_free_value (using_iterator);

    /* 6.e */
    if (ECMA_IS_VALUE_ERROR (iterator))
    {
      ecma_free_value (array);
      return iterator;
    }

    /* 6.f */
    uint32_t k = 0;

    /* 6.g */
    while (true)
    {
      /* 6.g.ii */
      ecma_value_t next = ecma_op_iterator_step (iterator);

      /* 6.g.iii */
      if (ECMA_IS_VALUE_ERROR (next))
      {
        goto iterator_cleanup;
      }

      /* 6.g.iii */
      if (ecma_is_value_false (next))
      {
        /* 6.g.iv.1 */
        ecma_value_t len_value = ecma_make_uint32_value (k);
        ecma_value_t set_status = ecma_op_object_put (array_obj_p,
                                                      ecma_get_magic_string (LIT_MAGIC_STRING_LENGTH),
                                                      len_value,
                                                      true);
        ecma_free_value (len_value);

        /* 6.g.iv.2 */
        if (ECMA_IS_VALUE_ERROR (set_status))
        {
          goto iterator_cleanup;
        }

        ecma_free_value (iterator);
        /* 6.g.iv.3 */
        return array;
      }

      /* 6.g.v */
      ecma_value_t next_value = ecma_op_iterator_value (next);

      ecma_free_value (next);

      /* 6.g.vi */
      if (ECMA_IS_VALUE_ERROR (next_value))
      {
        goto iterator_cleanup;
      }

      ecma_value_t mapped_value;
      /* 6.g.vii */
      if (mapfn_obj_p != NULL)
      {
        /* 6.g.vii.1 */
        ecma_value_t args_p[2] = { next_value, ecma_make_uint32_value (k) };
        /* 6.g.vii.3 */
        mapped_value = ecma_op_function_call (mapfn_obj_p, call_this_arg, args_p, 2);
        ecma_free_value (args_p[1]);
        ecma_free_value (next_value);

        /* 6.g.vii.2 */
        if (ECMA_IS_VALUE_ERROR (mapped_value))
        {
          ecma_op_iterator_close (iterator);
          goto iterator_cleanup;
        }
      }
      else
      {
        /* 6.g.viii */
        mapped_value = next_value;
      }

      /* 6.g.ix */
      const uint32_t flags = ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE | ECMA_IS_THROW;
      ecma_value_t set_status = ecma_builtin_helper_def_prop_by_index (array_obj_p, k, mapped_value, flags);

      ecma_free_value (mapped_value);

      /* 6.g.x */
      if (ECMA_IS_VALUE_ERROR (set_status))
      {
        ecma_op_iterator_close (iterator);
        goto iterator_cleanup;
      }

      /* 6.g.xi */
      k++;
    }

iterator_cleanup:
    ecma_free_value (iterator);
    ecma_free_value (array);

    return ret_value;
  }

  /* 8. */
  ecma_value_t array_like = ecma_op_to_object (items);

  /* 9. */
  if (ECMA_IS_VALUE_ERROR (array_like))
  {
    return array_like;
  }

  ecma_object_t *array_like_obj_p = ecma_get_object_from_value (array_like);

  /* 10. */
  uint32_t len;
  ecma_value_t len_value = ecma_op_object_get_length (array_like_obj_p, &len);

  /* 11. */
  if (ECMA_IS_VALUE_ERROR (len_value))
  {
    goto cleanup;
  }

  len_value = ecma_make_uint32_value (len);

  /* 12. */
  ecma_value_t array;

  /* 12.a */
  if (ecma_is_constructor (constructor))
  {
    ecma_object_t *constructor_obj_p = ecma_get_object_from_value (constructor);

    array = ecma_op_function_construct (constructor_obj_p, ECMA_VALUE_UNDEFINED, &len_value, 1);

    if (ecma_is_value_undefined (array) || ecma_is_value_null (array))
    {
      ecma_free_value (len_value);
      ecma_raise_type_error (ECMA_ERR_MSG ("Cannot convert undefined or null to object"));
      goto cleanup;
    }
  }
  else
  {
    /* 13.a */
    array = ecma_op_create_array_object (&len_value, 1, true);
  }

  ecma_free_value (len_value);

  /* 14. */
  if (ECMA_IS_VALUE_ERROR (array))
  {
    goto cleanup;
  }

  ecma_object_t *array_obj_p = ecma_get_object_from_value (array);

  /* 15. */
  uint32_t k = 0;

  /* 16. */
  while (k < len)
  {
    /* 16.b */
    ecma_value_t k_value = ecma_op_object_get_by_uint32_index (array_like_obj_p, k);

    /* 16.c */
    if (ECMA_IS_VALUE_ERROR (k_value))
    {
      goto construct_cleanup;
    }

    ecma_value_t mapped_value;
    /* 16.d */
    if (mapfn_obj_p != NULL)
    {
      /* 16.d.i */
      ecma_value_t args_p[2] = { k_value, ecma_make_uint32_value (k) };
      mapped_value = ecma_op_function_call (mapfn_obj_p, call_this_arg, args_p, 2);
      ecma_free_value (args_p[1]);
      ecma_free_value (k_value);

      /* 16.d.ii */
      if (ECMA_IS_VALUE_ERROR (mapped_value))
      {
        goto construct_cleanup;
      }
    }
    else
    {
      /* 16.e */
      mapped_value = k_value;
    }

    /* 16.f */
    const uint32_t flags = ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE | ECMA_IS_THROW;
    ecma_value_t set_status = ecma_builtin_helper_def_prop_by_index (array_obj_p, k, mapped_value, flags);

    ecma_free_value (mapped_value);

    /* 16.g */
    if (ECMA_IS_VALUE_ERROR (set_status))
    {
      goto construct_cleanup;
    }

    /* 16.h */
    k++;
  }

  /* 17. */
  len_value = ecma_make_uint32_value (k);
  ecma_value_t set_status = ecma_op_object_put (array_obj_p,
                                                ecma_get_magic_string (LIT_MAGIC_STRING_LENGTH),
                                                len_value,
                                                true);
  ecma_free_value (len_value);

  /* 18. */
  if (ECMA_IS_VALUE_ERROR (set_status))
  {
    goto construct_cleanup;
  }

  /* 19. */
  ecma_deref_object (array_like_obj_p);
  return ecma_make_object_value (array_obj_p);

construct_cleanup:
  ecma_deref_object (array_obj_p);
cleanup:
  ecma_deref_object (array_like_obj_p);
  return ret_value;
} /* ecma_builtin_array_object_from */
#endif /* ENABLED (JERRY_ES2015) */

/**
 * Handle calling [[Call]] of built-in Array object
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_array_dispatch_call (const ecma_value_t *arguments_list_p, /**< arguments list */
                                  ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  return ecma_builtin_array_dispatch_construct (arguments_list_p, arguments_list_len);
} /* ecma_builtin_array_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in Array object
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_array_dispatch_construct (const ecma_value_t *arguments_list_p, /**< arguments list */
                                       ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  return ecma_op_create_array_object (arguments_list_p, arguments_list_len, true);
} /* ecma_builtin_array_dispatch_construct */

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_BUILTIN_ARRAY) */
