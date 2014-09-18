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

#include "globals.h"
#include "ecma-builtins.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-magic-strings.h"

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup global ECMA Global object built-in
 * @{
 */

/**
 * Global object
 */
static ecma_object_t* ecma_global_object_p = NULL;

/**
 * Get Global object
 *
 * @return pointer to the Global object
 *         caller should free the reference by calling ecma_deref_object
 */
ecma_object_t*
ecma_builtin_get_global_object (void)
{
  JERRY_ASSERT(ecma_global_object_p != NULL);

  ecma_ref_object (ecma_global_object_p);

  return ecma_global_object_p;
} /* ecma_builtin_get_global_object */

/**
 * Check if passed object is the Global object.
 *
 * @return true - if passed pointer points to the Global object,
 *         false - otherwise.
 */
bool
ecma_builtin_is_global_object (ecma_object_t *object_p) /**< an object */
{
  return (object_p == ecma_global_object_p);
} /* ecma_builtin_is_global_object */

/**
 * Initialize Global object.
 *
 * Warning:
 *         the routine should be called only from ecma_init_builtins
 */
void
ecma_builtin_init_global_object (void)
{
  JERRY_ASSERT(ecma_global_object_p == NULL);

  ecma_object_t *glob_obj_p = ecma_create_object (NULL, true, ECMA_OBJECT_TYPE_GENERAL);

  ecma_string_t* undefined_identifier_p = ecma_get_magic_string (ECMA_MAGIC_STRING_UNDEFINED);
  ecma_property_t *undefined_prop_p = ecma_create_named_data_property (glob_obj_p,
                                                                       undefined_identifier_p,
                                                                       ECMA_PROPERTY_NOT_WRITABLE,
                                                                       ECMA_PROPERTY_NOT_ENUMERABLE,
                                                                       ECMA_PROPERTY_NOT_CONFIGURABLE);
  ecma_deref_ecma_string (undefined_identifier_p);
  JERRY_ASSERT(ecma_is_value_undefined (undefined_prop_p->u.named_data_property.value));

  TODO(/* Define NaN, Infinity, eval, parseInt, parseFloat, isNaN, isFinite properties */);

  ecma_global_object_p = glob_obj_p;
} /* ecma_builtin_init_global_object */

/**
 * Remove global reference to the Global object.
 *
 * Warning:
 *         the routine should be called only from ecma_finalize_builtins
 */
void
ecma_builtin_finalize_global_object (void)
{
  JERRY_ASSERT (ecma_global_object_p != NULL);

  ecma_deref_object (ecma_global_object_p);
  ecma_global_object_p = NULL;
} /* ecma_builtin_finalize_global_object */

/**
 * The Global object's 'eval' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.2.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_eval (ecma_value_t x) /**< routine's first argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (x);
} /* ecma_builtin_global_object_eval */

/**
 * The Global object's 'parseInt' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.2.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_parse_int (ecma_value_t string, /**< routine's first argument */
                                      ecma_value_t radix) /**< routine's second argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (string, radix);
} /* ecma_builtin_global_object_parse_int */

/**
 * The Global object's 'parseFloat' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.2.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_parse_float (ecma_value_t string) /**< routine's first argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (string);
} /* ecma_builtin_global_object_parse_float */

/**
 * The Global object's 'isNaN' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.2.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_is_nan (ecma_value_t number) /**< routine's first argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (number);
} /* ecma_builtin_global_object_is_nan */

/**
 * The Global object's 'isFinite' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.2.5
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_is_finite (ecma_value_t number) /**< routine's first argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (number);
} /* ecma_builtin_global_object_is_finite */

/**
 * The Global object's 'decodeURI' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.3.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_decode_uri (ecma_value_t encoded_uri) /**< routine's first argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (encoded_uri);
} /* ecma_builtin_global_object_decode_uri */

/**
 * The Global object's 'decodeURIComponent' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.3.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_decode_uri_component (ecma_value_t encoded_uri_component) /**< routine's first argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (encoded_uri_component);
} /* ecma_builtin_global_object_decode_uri_component */

/**
 * The Global object's 'encodeURI' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.3.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_encode_uri (ecma_value_t uri) /**< routine's first argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (uri);
} /* ecma_builtin_global_object_encode_uri */

/**
 * The Global object's 'encodeURIComponent' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.3.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_encode_uri_component (ecma_value_t uri_component) /**< routine's first argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (uri_component);
} /* ecma_builtin_global_object_encode_uri_component */

/**
 * Dispatcher of the Global object's built-in routines
 *
 * @return completion-value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_builtin_global_dispatch_routine (ecma_builtin_global_property_id_t builtin_routine_id, /**< identifier of
                                                                                                 the Global object's
                                                                                                 initial property that
                                                                                                 corresponds to
                                                                                                 routine to be called */
                                      ecma_value_t arguments_list [], /**< list of arguments passed to routine */
                                      ecma_length_t arguments_number) /**< length of arguments' list */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();
  const ecma_value_t value_undefined = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

  switch (builtin_routine_id)
  {
    case ECMA_BUILTIN_GLOBAL_PROPERTY_ID_EVAL:
    {
      ecma_value_t arg = (arguments_number >= 1 ? arguments_list[0] : value_undefined);

      return ecma_builtin_global_object_eval (arg);
    }

    case ECMA_BUILTIN_GLOBAL_PROPERTY_ID_PARSE_INT:
    {
      ecma_value_t arg1 = (arguments_number >= 1 ? arguments_list[0] : value_undefined);
      ecma_value_t arg2 = (arguments_number >= 2 ? arguments_list[1] : value_undefined);

      return ecma_builtin_global_object_parse_int (arg1, arg2);
    }

    case ECMA_BUILTIN_GLOBAL_PROPERTY_ID_PARSE_FLOAT:
    {
      ecma_value_t arg = (arguments_number >= 1 ? arguments_list[0] : value_undefined);

      return ecma_builtin_global_object_parse_float (arg);
    }

    case ECMA_BUILTIN_GLOBAL_PROPERTY_ID_IS_NAN:
    {
      ecma_value_t arg = (arguments_number >= 1 ? arguments_list[0] : value_undefined);

      return ecma_builtin_global_object_is_nan (arg);
    }

    case ECMA_BUILTIN_GLOBAL_PROPERTY_ID_IS_FINITE:
    {
      ecma_value_t arg = (arguments_number >= 1 ? arguments_list[0] : value_undefined);

      return ecma_builtin_global_object_is_finite (arg);
    }

    case ECMA_BUILTIN_GLOBAL_PROPERTY_ID_DECODE_URI:
    {
      ecma_value_t arg = (arguments_number >= 1 ? arguments_list[0] : value_undefined);

      return ecma_builtin_global_object_decode_uri (arg);
    }

    case ECMA_BUILTIN_GLOBAL_PROPERTY_ID_DECODE_URI_COMPONENT:
    {
      ecma_value_t arg = (arguments_number >= 1 ? arguments_list[0] : value_undefined);

      return ecma_builtin_global_object_decode_uri_component (arg);
    }

    case ECMA_BUILTIN_GLOBAL_PROPERTY_ID_ENCODE_URI:
    {
      ecma_value_t arg = (arguments_number >= 1 ? arguments_list[0] : value_undefined);

      return ecma_builtin_global_object_encode_uri (arg);
    }

    case ECMA_BUILTIN_GLOBAL_PROPERTY_ID_ENCODE_URI_COMPONENT:
    {
      ecma_value_t arg = (arguments_number >= 1 ? arguments_list[0] : value_undefined);

      return ecma_builtin_global_object_encode_uri_component (arg);
    }

    default:
    {
      JERRY_UNREACHABLE ();
    }
  }

  JERRY_ASSERT (!ecma_is_completion_value_empty (ret_value));
} /* ecma_builtin_global_dispatch_routine */

/**
 * @}
 * @}
 * @}
 */
