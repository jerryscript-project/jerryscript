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
#include "ecma-globals.h"
#include "ecma-magic-strings.h"

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 */

/**
 * Dispatcher of built-in routines
 *
 * @return completion-value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_builtin_dispatch_routine (ecma_builtin_id_t builtin_object_id, /**< built-in object' identifier */
                               uint32_t builtin_routine_id, /**< identifier of the built-in object's routine
                                                                 property (one of ecma_builtin_{*}_property_id_t) */
                               ecma_value_t arguments_list [], /**< list of arguments passed to routine */
                               ecma_length_t arguments_number) /**< length of arguments' list */
{
  switch (builtin_object_id)
  {
    case ECMA_BUILTIN_ID_GLOBAL:
    {
      JERRY_ASSERT (builtin_routine_id < ECMA_BUILTIN_GLOBAL_PROPERTY_ID__COUNT);
      return ecma_builtin_global_dispatch_routine ((ecma_builtin_global_property_id_t) builtin_routine_id,
                                                   arguments_list, arguments_number);
    }
    case ECMA_BUILTIN_ID_OBJECT:
    case ECMA_BUILTIN_ID_OBJECT_PROTOTYPE:
    case ECMA_BUILTIN_ID_FUNCTION:
    case ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE:
    case ECMA_BUILTIN_ID_ARRAY:
    case ECMA_BUILTIN_ID_ARRAY_PROTOTYPE:
    case ECMA_BUILTIN_ID_STRING:
    case ECMA_BUILTIN_ID_STRING_PROTOTYPE:
    case ECMA_BUILTIN_ID_BOOLEAN:
    case ECMA_BUILTIN_ID_BOOLEAN_PROTOTYPE:
    case ECMA_BUILTIN_ID_NUMBER:
    case ECMA_BUILTIN_ID_NUMBER_PROTOTYPE:
    case ECMA_BUILTIN_ID_DATE:
    case ECMA_BUILTIN_ID_REGEXP:
    case ECMA_BUILTIN_ID_REGEXP_PROTOTYPE:
    case ECMA_BUILTIN_ID_ERROR:
    case ECMA_BUILTIN_ID_ERROR_PROTOTYPE:
    case ECMA_BUILTIN_ID_EVAL_ERROR:
    case ECMA_BUILTIN_ID_RANGE_ERROR:
    case ECMA_BUILTIN_ID_REFERENCE_ERROR:
    case ECMA_BUILTIN_ID_SYNTAX_ERROR:
    case ECMA_BUILTIN_ID_TYPE_ERROR:
    case ECMA_BUILTIN_ID_SYNTAX_URI_ERROR:
    case ECMA_BUILTIN_ID_MATH:
    case ECMA_BUILTIN_ID_JSON:
    {
      JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (builtin_routine_id,
                                           arguments_list,
                                           arguments_number);
    }
  }

  JERRY_UNREACHABLE ();
} /* ecma_builtin_dispatch_routine */


/**
 * @}
 * @}
 */
