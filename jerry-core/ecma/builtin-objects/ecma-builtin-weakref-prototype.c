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

#include "ecma-container-object.h"
#include "ecma-exceptions.h"

#if JERRY_BUILTIN_WEAKREF

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

/**
 * This object has a custom dispatch function.
 */
#define BUILTIN_INC_HEADER_NAME "ecma-builtin-weakref-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID weakref_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup weakref ECMA WeakRef object built-in
 * @{
 */

 /**
  * Deref checks weakRef target
  *
  * @return weakRef target
  *         error - otherwise
  */
static ecma_value_t
ecma_builtin_weakref_prototype_object_deref (ecma_value_t this_arg) /**< this argument */
{
  if (!ecma_is_value_object (this_arg))
  {
    return ecma_raise_type_error ("Target is not Object");
  }

  ecma_object_t *object_p = ecma_get_object_from_value (this_arg);
  ecma_extended_object_t *this_ext_obj = (ecma_extended_object_t *) object_p;

  if (!ecma_object_class_is (object_p, ECMA_OBJECT_CLASS_WEAKREF))
  {
    return ecma_raise_type_error ("Target is not weakRef");
  }

  return ecma_copy_value (this_ext_obj->u.cls.u3.target);
} /* ecma_builtin_weakref_prototype_object_deref */

/**
 * @}
 * @}
 * @}
 */

#endif /* JERRY_BUILTIN_WEAKREF */
