/* Copyright 2015 Samsung Electronics Co., Ltd.
 * Copyright 2015 University of Szeged.
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
#include "ecma-globals.h"
#include "ecma-helpers.h"

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_DATE_BUILTIN

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-date.inc.h"
#define BUILTIN_UNDERSCORED_ID date
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup date ECMA Date object built-in
 * @{
 */

/**
 * The Date object's 'parse' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.4.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_parse (ecma_value_t this_arg, /**< this argument */
                         ecma_value_t arg) /**< string */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg);
} /* ecma_builtin_date_parse */

/**
 * The Date object's 'UTC' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.4.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_utc (ecma_value_t this_arg, /**< this argument */
                       const ecma_value_t args[], /**< arguments list */
                       ecma_length_t args_number) /**< number of arguments */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, args, args_number);
} /* ecma_builtin_date_utc */

/**
 * The Date object's 'now' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.4.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_now (ecma_value_t this_arg) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg);
} /* ecma_builtin_date_now */

/**
 * Handle calling [[Call]] of built-in Date object
 *
 * @return completion-value
 */
ecma_completion_value_t
ecma_builtin_date_dispatch_call (const ecma_value_t *arguments_list_p, /**< arguments list */
                                 ecma_length_t arguments_list_len) /**< number of arguments */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (arguments_list_p, arguments_list_len);
} /* ecma_builtin_date_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in Date object
 *
 * @return completion-value
 */
ecma_completion_value_t
ecma_builtin_date_dispatch_construct (const ecma_value_t *arguments_list_p, /**< arguments list */
                                      ecma_length_t arguments_list_len) /**< number of arguments */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (arguments_list_p, arguments_list_len);
} /* ecma_builtin_date_dispatch_construct */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_DATE_BUILTIN */
