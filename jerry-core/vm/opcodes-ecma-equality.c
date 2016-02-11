/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
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

#include "ecma-builtins.h"
#include "ecma-comparison.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "ecma-try-catch-macro.h"
#include "opcodes.h"
#include "vm-defines.h"

/** \addtogroup vm Virtual machine
 * @{
 *
 * \addtogroup vm_opcodes Opcodes
 * @{
 */

/**
 * 'Equals' opcode handler.
 *
 * See also: ECMA-262 v5, 11.9.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_equal_value (ecma_value_t left_value, /**< left value */
                    ecma_value_t right_value) /**< right value */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (compare_result,
                  ecma_op_abstract_equality_compare (left_value,
                                                     right_value),
                  ret_value);

  JERRY_ASSERT (ecma_is_value_boolean (compare_result));

  ret_value = ecma_make_normal_completion_value (compare_result);

  ECMA_FINALIZE (compare_result);

  return ret_value;
} /* opfunc_equal_value */

/**
 * 'Does-not-equals' opcode handler.
 *
 * See also: ECMA-262 v5, 11.9.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_not_equal_value (ecma_value_t left_value, /**< left value */
                        ecma_value_t right_value) /**< right value */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (compare_result,
                  ecma_op_abstract_equality_compare (left_value, right_value),
                  ret_value);

  JERRY_ASSERT (ecma_is_value_boolean (compare_result));

  bool is_equal = ecma_is_value_true (compare_result);

  ret_value = ecma_make_normal_completion_value (ecma_make_simple_value (is_equal ? ECMA_SIMPLE_VALUE_FALSE
                                                                                  : ECMA_SIMPLE_VALUE_TRUE));

  ECMA_FINALIZE (compare_result);

  return ret_value;
} /* opfunc_not_equal_value */

/**
 * 'Strict Equals' opcode handler.
 *
 * See also: ECMA-262 v5, 11.9.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_equal_value_type (ecma_value_t left_value, /**< left value */
                         ecma_value_t right_value) /**< right value */
{
  bool is_equal = ecma_op_strict_equality_compare (left_value, right_value);

  return ecma_make_normal_completion_value (ecma_make_simple_value (is_equal ? ECMA_SIMPLE_VALUE_TRUE
                                                                             : ECMA_SIMPLE_VALUE_FALSE));
} /* opfunc_equal_value_type */

/**
 * 'Strict Does-not-equals' opcode handler.
 *
 * See also: ECMA-262 v5, 11.9.5
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_not_equal_value_type (ecma_value_t left_value, /**< left value */
                             ecma_value_t right_value) /**< right value */
{
  bool is_equal = ecma_op_strict_equality_compare (left_value, right_value);

  return ecma_make_normal_completion_value (ecma_make_simple_value (is_equal ? ECMA_SIMPLE_VALUE_FALSE
                                                                             : ECMA_SIMPLE_VALUE_TRUE));
} /* opfunc_not_equal_value_type */

/**
 * @}
 * @}
 */
