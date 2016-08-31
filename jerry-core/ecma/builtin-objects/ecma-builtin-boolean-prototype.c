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

#include "ecma-alloc.h"
#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-string-object.h"
#include "ecma-try-catch-macro.h"
#include "jrt.h"

#ifndef CONFIG_DISABLE_BOOLEAN_BUILTIN

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-boolean-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID boolean_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup booleanprototype ECMA Boolean.prototype object built-in
 * @{
 */

/**
 * The Boolean.prototype object's 'toString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.6.4.2
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_boolean_prototype_object_to_string (ecma_value_t this_arg) /**< this argument */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_TRY_CATCH (value_of_ret,
                  ecma_builtin_boolean_prototype_object_value_of (this_arg),
                  ret_value);

  ecma_string_t *ret_str_p;

  if (ecma_is_value_true (value_of_ret))
  {
    ret_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_TRUE);
  }
  else
  {
    JERRY_ASSERT (ecma_is_value_boolean (value_of_ret));

    ret_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_FALSE);
  }

  ret_value = ecma_make_string_value (ret_str_p);

  ECMA_FINALIZE (value_of_ret);

  return ret_value;
} /* ecma_builtin_boolean_prototype_object_to_string */

/**
 * The Boolean.prototype object's 'valueOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.6.4.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_boolean_prototype_object_value_of (ecma_value_t this_arg) /**< this argument */
{
  if (ecma_is_value_boolean (this_arg))
  {
    return this_arg;
  }
  else if (ecma_is_value_object (this_arg))
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (this_arg);

    if (ecma_object_get_class_name (obj_p) == LIT_MAGIC_STRING_BOOLEAN_UL)
    {
      ecma_value_t *prim_value_prop_p = ecma_get_internal_property (obj_p,
                                                                    ECMA_INTERNAL_PROPERTY_ECMA_VALUE);

      JERRY_ASSERT (ecma_is_value_boolean (*prim_value_prop_p));

      return *prim_value_prop_p;
    }
  }

  return ecma_raise_type_error (ECMA_ERR_MSG (""));
} /* ecma_builtin_boolean_prototype_object_value_of */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_BOOLEAN_BUILTIN */
