/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
 * Copyright 2015-2016 University of Szeged.
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
#include "ecma-builtin-helpers.h"
#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-string-object.h"
#include "ecma-try-catch-macro.h"
#include "jrt.h"

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-object-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID object_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup objectprototype ECMA Object.prototype object built-in
 * @{
 */

/**
 * The Object.prototype object's 'toString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.4.2
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_object_prototype_object_to_string (ecma_value_t this_arg) /**< this argument */
{
  return ecma_builtin_helper_object_to_string (this_arg);
} /* ecma_builtin_object_prototype_object_to_string */

/**
 * The Object.prototype object's 'valueOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.4.4
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_object_prototype_object_value_of (ecma_value_t this_arg) /**< this argument */
{
  return ecma_op_to_object (this_arg);
} /* ecma_builtin_object_prototype_object_value_of */

/**
 * The Object.prototype object's 'toLocaleString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.4.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_object_prototype_object_to_locale_string (ecma_value_t this_arg) /**< this argument */
{
  ecma_value_t return_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
  /* 1. */
  ECMA_TRY_CATCH (obj_val,
                  ecma_op_to_object (this_arg),
                  return_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_val);
  ecma_string_t *to_string_magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_TO_STRING_UL);

  /* 2. */
  ECMA_TRY_CATCH (to_string_val,
                  ecma_op_object_get (obj_p, to_string_magic_string_p),
                  return_value);

  /* 3. */
  if (!ecma_op_is_callable (to_string_val))
  {
    return_value = ecma_raise_type_error (ECMA_ERR_MSG (""));
  }
  else
  {
    /* 4. */
    ecma_object_t *to_string_func_obj_p = ecma_get_object_from_value (to_string_val);
    return_value = ecma_op_function_call (to_string_func_obj_p, this_arg, NULL, 0);
  }
  ECMA_FINALIZE (to_string_val);

  ecma_deref_ecma_string (to_string_magic_string_p);

  ECMA_FINALIZE (obj_val);

  return return_value;
} /* ecma_builtin_object_prototype_object_to_locale_string */

/**
 * The Object.prototype object's 'hasOwnProperty' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.4.5
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_object_prototype_object_has_own_property (ecma_value_t this_arg, /**< this argument */
                                                       ecma_value_t arg) /**< first argument */
{
  ecma_value_t return_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (to_string_val,
                  ecma_op_to_string (arg),
                  return_value);

  /* 2. */
  ECMA_TRY_CATCH (obj_val,
                  ecma_op_to_object (this_arg),
                  return_value);

  ecma_string_t *property_name_string_p = ecma_get_string_from_value (to_string_val);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_val);

  /* 3. */
  return_value = ecma_make_boolean_value (ecma_op_object_has_own_property (obj_p, property_name_string_p));

  ECMA_FINALIZE (obj_val);

  ECMA_FINALIZE (to_string_val);

  return return_value;
} /* ecma_builtin_object_prototype_object_has_own_property */

/**
 * The Object.prototype object's 'isPrototypeOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.4.6
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_object_prototype_object_is_prototype_of (ecma_value_t this_arg, /**< this argument */
                                                      ecma_value_t arg) /**< routine's first argument */
{
  /* 1. Is the argument an object? */
  if (!ecma_is_value_object (arg))
  {
    return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE);
  }

  ecma_value_t return_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 2. ToObject(this) */
  ECMA_TRY_CATCH (obj_value,
                  ecma_op_to_object (this_arg),
                  return_value);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_value);

  /* 3. Compare prototype to object */
  ECMA_TRY_CATCH (v_obj_value,
                  ecma_op_to_object (arg),
                  return_value);

  ecma_object_t *v_obj_p = ecma_get_object_from_value (v_obj_value);

  bool is_prototype_of = ecma_op_object_is_prototype_of (obj_p, v_obj_p);
  return_value = ecma_make_simple_value (is_prototype_of ? ECMA_SIMPLE_VALUE_TRUE
                                                         : ECMA_SIMPLE_VALUE_FALSE);
  ECMA_FINALIZE (v_obj_value);

  ECMA_FINALIZE (obj_value);

  return return_value;
} /* ecma_builtin_object_prototype_object_is_prototype_of */

/**
 * The Object.prototype object's 'propertyIsEnumerable' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.4.7
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_object_prototype_object_property_is_enumerable (ecma_value_t this_arg, /**< this argument */
                                                             ecma_value_t arg) /**< routine's first argument */
{
  ecma_value_t return_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (to_string_val,
                  ecma_op_to_string (arg),
                  return_value);

  /* 2. */
  ECMA_TRY_CATCH (obj_val,
                  ecma_op_to_object (this_arg),
                  return_value);

  ecma_string_t *property_name_string_p = ecma_get_string_from_value (to_string_val);

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_val);

  /* 3. */
  ecma_property_t property = ecma_op_object_get_own_property (obj_p,
                                                              property_name_string_p,
                                                              NULL,
                                                              ECMA_PROPERTY_GET_NO_OPTIONS);

  /* 4. */
  if (property != ECMA_PROPERTY_TYPE_NOT_FOUND)
  {
    bool is_enumerable = ecma_is_property_enumerable (property);

    return_value = ecma_make_boolean_value (is_enumerable);
  }
  else
  {
    return_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE);
  }

  ECMA_FINALIZE (obj_val);

  ECMA_FINALIZE (to_string_val);

  return return_value;
} /* ecma_builtin_object_prototype_object_property_is_enumerable */

/**
 * @}
 * @}
 * @}
 */
