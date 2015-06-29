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
#include "ecma-boolean-object.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-objects-general.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabooleanobject ECMA Boolean object related routines
 * @{
 */

/**
 * Boolean object creation operation.
 *
 * See also: ECMA-262 v5, 15.6.2.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_create_boolean_object (ecma_value_t arg) /**< argument passed to the Boolean constructor */
{
  ecma_completion_value_t conv_to_boolean_completion = ecma_op_to_boolean (arg);

  if (!ecma_is_completion_value_normal (conv_to_boolean_completion))
  {
    return conv_to_boolean_completion;
  }

  ecma_simple_value_t bool_value = (ecma_is_value_true (ecma_get_completion_value_value (conv_to_boolean_completion)) ?
                                    ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE);

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_BOOLEAN_BUILTIN
  ecma_object_t *prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_BOOLEAN_PROTOTYPE);
#else /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_BOOLEAN_BUILTIN */
  ecma_object_t *prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE);
#endif /* CONFIG_ECMA_COMPACT_PROFILE_DISABLE_BOOLEAN_BUILTIN */

  ecma_object_t *obj_p = ecma_create_object (prototype_obj_p,
                                             true,
                                             ECMA_OBJECT_TYPE_GENERAL);
  ecma_deref_object (prototype_obj_p);

  ecma_property_t *class_prop_p = ecma_create_internal_property (obj_p, ECMA_INTERNAL_PROPERTY_CLASS);
  class_prop_p->u.internal_property.value = LIT_MAGIC_STRING_BOOLEAN_UL;

  ecma_property_t *prim_value_prop_p = ecma_create_internal_property (obj_p,
                                                                      ECMA_INTERNAL_PROPERTY_PRIMITIVE_BOOLEAN_VALUE);
  prim_value_prop_p->u.internal_property.value = bool_value;

  return ecma_make_normal_completion_value (ecma_make_object_value (obj_p));
} /* ecma_op_create_boolean_object */
