/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-try-catch-macro.h"

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN
#include "ecma-regexp-object.h"
#include "re-compiler.h"

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-regexp-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID regexp_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup regexp ECMA RegExp.prototype object built-in
 * @{
 */

/**
 * The RegExp.prototype object's 'exec' routine
 *
 * See also:
 *          ECMA-262 v5, 15.10.6.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_regexp_prototype_exec (ecma_value_t this_arg, /**< this argument */
                                    ecma_value_t arg) /**< routine's argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (obj_this, ecma_op_to_object (this_arg), ret_value);
  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
  ecma_property_t *bytecode_prop = ecma_get_internal_property (obj_p, ECMA_INTERNAL_PROPERTY_REGEXP_BYTECODE);
  regexp_bytecode_t *bytecode_p = ECMA_GET_POINTER (regexp_bytecode_t, bytecode_prop->u.internal_property.value);

  ECMA_TRY_CATCH (input_str_value,
                  ecma_op_to_string (arg),
                  ret_value);

  ecma_string_t *input_str_p = ecma_get_string_from_value (input_str_value);

  /* Convert ecma_String_t* to regexp_bytecode_t* */
  int32_t chars = ecma_string_get_length (input_str_p);

  MEM_DEFINE_LOCAL_ARRAY (input_zt_str_p, chars + 1, ecma_char_t);

  ssize_t zt_str_size = (ssize_t) sizeof (ecma_char_t) * (chars + 1);
  ecma_string_to_zt_string (input_str_p, input_zt_str_p, zt_str_size);

  /* FIXME: call of ecma_make_normal_completion_value should be in an ECMA_TRY_CATCH */
  ret_value = ecma_regexp_exec_helper (bytecode_p, input_zt_str_p);

  MEM_FINALIZE_LOCAL_ARRAY (input_zt_str_p);

  ECMA_FINALIZE (input_str_value);

  ECMA_FINALIZE (obj_this);
  return ret_value;
} /* ecma_builtin_regexp_prototype_exec */

/**
 * @}
 * @}
 * @}
 */

#endif /* CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN */
