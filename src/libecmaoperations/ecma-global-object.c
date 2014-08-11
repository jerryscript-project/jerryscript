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
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-global-object.h"
#include "ecma-helpers.h"
#include "ecma-magic-strings.h"

/** \addtogroup ecma ---TODO---
 * @{
 *
 * \addtogroup ecmaglobalobject ECMA Global object related routines
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
ecma_get_global_object( void)
{
  JERRY_ASSERT( ecma_global_object_p != NULL );

  ecma_ref_object( ecma_global_object_p);

  return ecma_global_object_p;
} /* ecma_get_global_object */

/**
 * The Global Object construction routine.
 *
 * See also: ECMA-262 v5, 15.1
 *
 * @return pointer to the constructed object
 */
ecma_object_t*
ecma_op_create_global_object( void)
{
  JERRY_ASSERT( ecma_global_object_p == NULL );

  ecma_object_t *glob_obj_p = ecma_create_object( NULL, true, ECMA_OBJECT_TYPE_GENERAL);

  ecma_property_t *undefined_prop_p = ecma_create_named_data_property( glob_obj_p,
                                                                       ecma_get_magic_string( ECMA_MAGIC_STRING_UNDEFINED),
                                                                       ECMA_PROPERTY_NOT_WRITABLE,
                                                                       ECMA_PROPERTY_NOT_ENUMERABLE,
                                                                       ECMA_PROPERTY_NOT_CONFIGURABLE);
  JERRY_ASSERT( ecma_is_value_undefined( undefined_prop_p->u.named_data_property.value) );

  TODO( /* Define NaN, Infinity, eval, parseInt, parseFloat, isNaN, isFinite properties */ );

  ecma_ref_object( glob_obj_p);
  ecma_global_object_p = glob_obj_p;

  return glob_obj_p;
} /* ecma_op_create_global_object */

/**
 * Remove global reference to the global object.
 *
 * Warning:
 *         the routine should be called only from ecma_finalize
 */
void
ecma_finalize_global_object(void)
{
  ecma_deref_object( ecma_global_object_p);
  ecma_global_object_p = NULL;
} /* ecma_free_global_object */

/**
 * @}
 * @}
 */
