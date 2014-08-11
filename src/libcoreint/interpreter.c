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

#include "deserializer.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "ecma-operations.h"
#include "globals.h"
#include "interpreter.h"
#include "jerry-libc.h"

#define INIT_OP_FUNC(name) [ __op__idx_##name ] = opfunc_##name,
static const opfunc __opfuncs[LAST_OP] =
{
  OP_LIST (INIT_OP_FUNC)
};
#undef INIT_OP_FUNC

JERRY_STATIC_ASSERT (sizeof (OPCODE) <= 4);

const OPCODE *__program = NULL;

/**
 * Initialize interpreter.
 */
void
init_int (const OPCODE *program_p) /**< pointer to byte-code program */
{
  JERRY_ASSERT (__program == NULL);

  __program = program_p;
} /* init_int */

bool
run_int (void)
{
  JERRY_ASSERT (__program != NULL);

  const opcode_counter_t start_pos = 0;
  ecma_value_t this_binding_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
  ecma_object_t *lex_env_p = ecma_op_create_global_environment ();
  FIXME (Strict mode);
  const bool is_strict = false;

  ecma_completion_value_t completion = run_int_from_pos (start_pos,
                                                         this_binding_value,
                                                         lex_env_p,
                                                         is_strict);

  switch ((ecma_completion_type_t) completion.type)
  {
    case ECMA_COMPLETION_TYPE_NORMAL:
    {
      JERRY_UNREACHABLE ();
    }
    case ECMA_COMPLETION_TYPE_EXIT:
    {
      ecma_deref_object (lex_env_p);
      ecma_finalize ();
      ecma_gc_run (ECMA_GC_GEN_COUNT - 1);

      return ecma_is_value_true (completion.value);
    }
    case ECMA_COMPLETION_TYPE_BREAK:
    case ECMA_COMPLETION_TYPE_CONTINUE:
    case ECMA_COMPLETION_TYPE_RETURN:
    {
      TODO (Throw SyntaxError);

      JERRY_UNIMPLEMENTED ();
    }
    case ECMA_COMPLETION_TYPE_THROW:
    {
      TODO (Handle unhandled exception);

      JERRY_UNIMPLEMENTED ();
    }
  }

  JERRY_UNREACHABLE ();
}

ecma_completion_value_t
run_int_from_pos (opcode_counter_t start_pos,
                  ecma_value_t this_binding_value,
                  ecma_object_t *lex_env_p,
                  bool is_strict)
{
  ecma_completion_value_t completion;

  const OPCODE *curr = &__program[start_pos];
  JERRY_ASSERT (curr->op_idx == __op__idx_reg_var_decl);

  const T_IDX min_reg_num = curr->data.reg_var_decl.min;
  const T_IDX max_reg_num = curr->data.reg_var_decl.max;
  JERRY_ASSERT (max_reg_num >= min_reg_num);

  const uint32_t regs_num = (uint32_t) (max_reg_num - min_reg_num + 1);

  ecma_value_t regs[ regs_num ];

  /* memseting with zero initializes each 'register' to empty value */
  __memset (regs, 0, sizeof (regs));
  JERRY_ASSERT (ecma_is_value_empty (regs[0]));

  struct __int_data int_data;
  int_data.pos = (opcode_counter_t) (start_pos + 1);
  int_data.this_binding = this_binding_value;
  int_data.lex_env_p = lex_env_p;
  int_data.is_strict = is_strict;
  int_data.min_reg_num = min_reg_num;
  int_data.max_reg_num = max_reg_num;
  int_data.regs_p = regs;

  while (true)
  {
    do
    {
      const OPCODE *curr = &__program[int_data.pos];
      completion = __opfuncs[curr->op_idx] (*curr, &int_data);

      JERRY_ASSERT (!ecma_is_completion_value_normal (completion)
                    || ecma_is_empty_completion_value (completion));
    }
    while (completion.type == ECMA_COMPLETION_TYPE_NORMAL);

    if (completion.type == ECMA_COMPLETION_TYPE_BREAK)
    {
      JERRY_UNIMPLEMENTED ();

      continue;
    }
    else if (completion.type == ECMA_COMPLETION_TYPE_CONTINUE)
    {
      JERRY_UNIMPLEMENTED ();

      continue;
    }

    for (uint32_t reg_index = 0;
         reg_index < regs_num;
         reg_index++)
    {
      ecma_free_value (regs[ reg_index ], true);
    }

    return completion;
  }
}

/**
 * Copy zero-terminated string literal value to specified buffer.
 *
 * @return upon success - number of bytes actually copied,
 *         otherwise (buffer size is not enough) - negated minimum required buffer size.
 */
ssize_t
try_get_string_by_idx (T_IDX idx, /**< literal id */
                       ecma_char_t *buffer_p, /**< buffer */
                       ssize_t buffer_size) /**< buffer size */
{
  TODO (Actual string literal retrievement);

  const ecma_char_t *str_p = deserialize_string_by_id (idx);
  JERRY_ASSERT (str_p != NULL);

  FIXME (ecma_char_t strlen);
  
  ssize_t req_length = (ssize_t)__strlen ((const char*)str_p) + 1;

  if (buffer_size < req_length)
  {
    return -req_length;
  }

  FIXME (ecma_char_t strncpy);

  __strncpy ((char*)buffer_p, (const char*)str_p, (size_t)req_length);

  return req_length;
} /* try_get_string_by_idx */

/**
 * Get number literal value.
 *
 * @return value of number literal, corresponding to specified literal id
 */
ecma_number_t
get_number_by_idx (T_IDX idx) /**< literal id */
{
  TODO (Actual number literal retrievement);

  FIXME (/* conversion of value returned from deserialize_num_by_id to ecma_number_t */);
  ecma_number_t num = (ecma_number_t) deserialize_num_by_id (idx);

  return num;
} /* get_number_by_idx */

/**
 * Get specified opcode from the program.
 */
OPCODE
read_opcode (opcode_counter_t counter) /**< opcode counter */
{
  return __program[ counter ];
} /* read_opcode */
