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

#include "ecma-helpers.h"
#include "interpreter.h"

#define INIT_OP_FUNC(name) [ __op__idx_##name ] = opfunc_##name,
static const opfunc __opfuncs[LAST_OP] = {
  OP_LIST (INIT_OP_FUNC)
};
#undef INIT_OP_FUNC

JERRY_STATIC_ASSERT (sizeof (OPCODE) <= 4);

const OPCODE *__program = NULL;

/**
 * Initialize interpreter.
 */
void
init_int( const OPCODE *program_p) /**< pointer to byte-code program */
{
  JERRY_ASSERT( __program == NULL );

  __program = program_p;
} /* init_int */

bool
run_int (void)
{
  JERRY_ASSERT( __program != NULL );

  struct __int_data int_data;
  int_data.pos = 0;
  int_data.this_binding_p = NULL;
  int_data.lex_env_p = ecma_CreateLexicalEnvironment( NULL,
                                                      ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE);

  ecma_CompletionValue_t completion = run_int_from_pos( &int_data);

  switch ( (ecma_CompletionType_t)completion.type )
  {
    case ECMA_COMPLETION_TYPE_NORMAL:
      {
        JERRY_UNREACHABLE();
      }
    case ECMA_COMPLETION_TYPE_EXIT:
      {
        return ecma_IsValueTrue( completion.value);
      }
    case ECMA_COMPLETION_TYPE_BREAK:
    case ECMA_COMPLETION_TYPE_CONTINUE:
    case ECMA_COMPLETION_TYPE_RETURN:
      {
        TODO( Throw SyntaxError );

        JERRY_UNIMPLEMENTED();
      }
    case ECMA_COMPLETION_TYPE_THROW:
      {
        TODO( Handle unhandled exception );

        JERRY_UNIMPLEMENTED();
      }
  }

  JERRY_UNREACHABLE();
}

ecma_CompletionValue_t
run_int_from_pos (struct __int_data *int_data)
{
  ecma_CompletionValue_t completion;

  while ( true )
    {
      do
        {
          const OPCODE *curr = &__program[int_data->pos];
          completion = __opfuncs[curr->op_idx](*curr, int_data);
        } while ( completion.type == ECMA_COMPLETION_TYPE_NORMAL );

      if ( completion.type == ECMA_COMPLETION_TYPE_BREAK )
        {
          JERRY_UNIMPLEMENTED();

          continue;
        }
      else if ( completion.type == ECMA_COMPLETION_TYPE_CONTINUE )
        {
          JERRY_UNIMPLEMENTED();

          continue;
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
try_get_string_by_idx(T_IDX idx, /**< literal id */
                      ecma_Char_t *buffer_p, /**< buffer */
                      ssize_t buffer_size) /**< buffer size */
{
  TODO( Actual string literal retrievement );

  ssize_t req_length = 2; // TODO

  JERRY_ASSERT( idx < 'z' - 'a' + 1 );

  if ( buffer_size < req_length )
  {
    return -req_length;
  }

  // TODO
  buffer_p[0] = (ecma_Char_t) ('a' + idx);
  buffer_p[1] = 0;

  return req_length;
} /* try_get_string_by_idx */

/**
 * Get number literal value.
 *
 * @return value of number literal, corresponding to specified literal id
 */
ecma_Number_t
get_number_by_idx(T_IDX idx) /**< literal id */
{
  TODO( Actual number literal retrievement );

  return (float)idx;
} /* get_number_by_idx */
