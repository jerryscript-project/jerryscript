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
 #include "ecma-conversion.h"
 #include "ecma-exceptions.h"
 #include "ecma-gc.h"
 #include "ecma-globals.h"
 #include "ecma-helpers.h"
 #include "ecma-objects.h"
 #include "ecma-symbol-object.h"
 #include "ecma-try-catch-macro.h"
 #include "jrt.h"

#if ENABLED (JERRY_ES2015)

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-symbol-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID symbol_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup symbolprototype ECMA Symbol prototype object built-in
 * @{
 */

/**
 * The Symbol.prototype object's 'toString' routine
 *
 * See also:
 *          ECMA-262 v6, 19.4.3.2
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_symbol_prototype_object_to_string (ecma_value_t this_arg) /**< this argument */
{
  return ecma_symbol_to_string_helper (this_arg, true);
} /* ecma_builtin_symbol_prototype_object_to_string */

/**
 * The Symbol.prototype object's 'valueOf' routine
 *
 * See also:
 *          ECMA-262 v6, 19.4.3.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_symbol_prototype_object_value_of (ecma_value_t this_arg) /**< this argument */
{
  return ecma_symbol_to_string_helper (this_arg, false);
} /* ecma_builtin_symbol_prototype_object_value_of */

/**
 * The Symbol.prototype object's '@@toPrimitive' routine
 *
 * See also:
 *          ECMA-262 v6, 19.4.3.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_symbol_prototype_object_to_primitive (ecma_value_t this_arg) /**< this argument */
{
  return ecma_builtin_symbol_prototype_object_value_of (this_arg);
} /* ecma_builtin_symbol_prototype_object_to_primitive */

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_ES2015) */
