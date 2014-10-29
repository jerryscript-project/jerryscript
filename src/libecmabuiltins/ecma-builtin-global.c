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

#include "ecma-alloc.h"
#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-try-catch-macro.h"
#include "globals.h"

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-global.inc.h"
#define BUILTIN_UNDERSCORED_ID global
#include "ecma-builtin-internal-routines-template.inc.h"

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
 * The Global object's 'eval' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.2.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_eval (ecma_value_t this_arg, /**< this argument */
                                 ecma_value_t x) /**< routine's first argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, x);
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
ecma_builtin_global_object_parse_int (ecma_value_t this_arg, /**< this argument */
                                      ecma_value_t string, /**< routine's first argument */
                                      ecma_value_t radix) /**< routine's second argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, string, radix);
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
ecma_builtin_global_object_parse_float (ecma_value_t this_arg, /**< this argument */
                                        ecma_value_t string) /**< routine's first argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, string);
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
ecma_builtin_global_object_is_nan (ecma_value_t this_arg __unused, /**< this argument */
                                   ecma_value_t arg) /**< routine's first argument */
{
  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (num_value, ecma_op_to_number (arg), ret_value);

  ecma_number_t *num_p = ECMA_GET_POINTER (num_value.u.value.value);

  bool is_nan = ecma_number_is_nan (*num_p);

  ret_value = ecma_make_simple_completion_value (is_nan ? ECMA_SIMPLE_VALUE_TRUE
                                                        : ECMA_SIMPLE_VALUE_FALSE);

  ECMA_FINALIZE (num_value);

  return ret_value;
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
ecma_builtin_global_object_is_finite (ecma_value_t this_arg __unused, /**< this argument */
                                      ecma_value_t arg) /**< routine's first argument */
{
  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (num_value, ecma_op_to_number (arg), ret_value);

  ecma_number_t *num_p = ECMA_GET_POINTER (num_value.u.value.value);

  bool is_finite = !(ecma_number_is_nan (*num_p)
                     || ecma_number_is_infinity (*num_p));

  ret_value = ecma_make_simple_completion_value (is_finite ? ECMA_SIMPLE_VALUE_TRUE
                                                           : ECMA_SIMPLE_VALUE_FALSE);

  ECMA_FINALIZE (num_value);

  return ret_value;
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
ecma_builtin_global_object_decode_uri (ecma_value_t this_arg, /**< this argument */
                                       ecma_value_t encoded_uri) /**< routine's first argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, encoded_uri);
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
ecma_builtin_global_object_decode_uri_component (ecma_value_t this_arg, /**< this argument */
                                                 ecma_value_t encoded_uri_component) /**< routine's first argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, encoded_uri_component);
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
ecma_builtin_global_object_encode_uri (ecma_value_t this_arg, /**< this argument */
                                       ecma_value_t uri) /**< routine's first argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, uri);
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
ecma_builtin_global_object_encode_uri_component (ecma_value_t this_arg, /**< this argument */
                                                 ecma_value_t uri_component) /**< routine's first argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, uri_component);
} /* ecma_builtin_global_object_encode_uri_component */

/**
 * @}
 * @}
 * @}
 */
