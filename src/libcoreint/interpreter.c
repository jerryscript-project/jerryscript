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

#include "ecma-builtins.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "ecma-operations.h"
#include "globals.h"
#include "interpreter.h"
#include "jerry-libc.h"
#include "mem-allocator.h"

#define __INIT_OP_FUNC(name, arg1, arg2, arg3) [ __op__idx_##name ] = opfunc_##name,
static const opfunc __opfuncs[LAST_OP] =
{
  OP_LIST (INIT_OP_FUNC)
};
#undef __INIT_OP_FUNC

JERRY_STATIC_ASSERT (sizeof (opcode_t) <= 4);

const opcode_t *__program = NULL;

#ifdef MEM_STATS
#define __OP_FUNC_NAME(name, arg1, arg2, arg3) #name,
static const char *__op_names[LAST_OP] =
{
  OP_LIST (OP_FUNC_NAME)
};
#undef __OP_FUNC_NAME

static uint32_t interp_mem_stats_print_indentation = 0;
static bool interp_mem_stats_enabled = false;

static void
interp_mem_stats_print_legend (void)
{
  if (likely (!interp_mem_stats_enabled))
  {
    return;
  }

  __printf ("----- Legend of memory usage trace during interpretation -----\n\n"
            "\tEntering block = beginning execution of initial (global) scope or function.\n\n"
            "\tInformation on each value is formatted as following: (p -> n ( [+-]c, local l, peak g), where:\n"
            "\t p     - value just before starting of item's execution;\n"
            "\t n     - value just after end of item's execution;\n"
            "\t [+-c] - difference between n and p;\n"
            "\t l     - temporary usage of memory during item's execution;\n"
            "\t g     - global peak of the value during program's execution.\n\n"
            "\tChunks are items allocated in a pool."
            " If there is no pool with a free chunk upon chunk allocation request,\n"
            "\tthen new pool is allocated on the heap (that causes increase of number of allocated heap bytes).\n\n");
}

static void
interp_mem_get_stats (mem_heap_stats_t *out_heap_stats_p,
                      mem_pools_stats_t *out_pool_stats_p,
                      bool reset_peak_before,
                      bool reset_peak_after)
{
  if (likely (!interp_mem_stats_enabled))
  {
    return;
  }

  ecma_gc_run (ECMA_GC_GEN_2);

  if (reset_peak_before)
  {
    mem_heap_stats_reset_peak ();
    mem_pools_stats_reset_peak ();
  }

  mem_heap_get_stats (out_heap_stats_p);
  mem_pools_get_stats (out_pool_stats_p);

  if (reset_peak_after)
  {
    mem_heap_stats_reset_peak ();
    mem_pools_stats_reset_peak ();
  }
}

static void
interp_mem_stats_context_enter (int_data_t *int_data_p,
                                opcode_counter_t block_position)
{
  if (likely (!interp_mem_stats_enabled))
  {
    return;
  }

  char indent_prefix[interp_mem_stats_print_indentation + 2];
  __memset (indent_prefix, ' ', sizeof (indent_prefix));
  indent_prefix [interp_mem_stats_print_indentation] = '|';
  indent_prefix [interp_mem_stats_print_indentation + 1] = '\0';
  
  int_data_p->context_peak_allocated_heap_bytes = 0;
  int_data_p->context_peak_waste_heap_bytes = 0;
  int_data_p->context_peak_pools_count = 0;
  int_data_p->context_peak_allocated_pool_chunks = 0;

  interp_mem_get_stats (&int_data_p->heap_stats_context_enter,
                        &int_data_p->pools_stats_context_enter,
                        false, false);

  __printf ("\n%s--- Beginning interpretation of a block at position %u ---\n"
            "%s Allocated heap bytes:  %5u\n"
            "%s Waste heap bytes:      %5u\n"
            "%s Pools:                 %5u\n"
            "%s Allocated pool chunks: %5u\n\n",
            indent_prefix, (uint32_t) block_position,
            indent_prefix, int_data_p->heap_stats_context_enter.allocated_bytes,
            indent_prefix, int_data_p->heap_stats_context_enter.waste_bytes,
            indent_prefix, int_data_p->pools_stats_context_enter.pools_count,
            indent_prefix, int_data_p->pools_stats_context_enter.allocated_chunks);
}

static void
interp_mem_stats_context_exit (int_data_t *int_data_p,
                               opcode_counter_t block_position)
{
  if (likely (!interp_mem_stats_enabled))
  {
    return;
  }

  char indent_prefix[interp_mem_stats_print_indentation + 2];
  __memset (indent_prefix, ' ', sizeof (indent_prefix));
  indent_prefix [interp_mem_stats_print_indentation] = '|';
  indent_prefix [interp_mem_stats_print_indentation + 1] = '\0';
  
  mem_heap_stats_t heap_stats_context_exit;
  mem_pools_stats_t pools_stats_context_exit;

  interp_mem_get_stats (&heap_stats_context_exit,
                        &pools_stats_context_exit,
                        false, true);

  int_data_p->context_peak_allocated_heap_bytes -= JERRY_MAX (int_data_p->heap_stats_context_enter.allocated_bytes,
                                                              heap_stats_context_exit.allocated_bytes);
  int_data_p->context_peak_waste_heap_bytes -= JERRY_MAX (int_data_p->heap_stats_context_enter.waste_bytes,
                                                          heap_stats_context_exit.waste_bytes);
  int_data_p->context_peak_pools_count -= JERRY_MAX (int_data_p->pools_stats_context_enter.pools_count,
                                                     pools_stats_context_exit.pools_count);
  int_data_p->context_peak_allocated_pool_chunks -= JERRY_MAX (int_data_p->pools_stats_context_enter.allocated_chunks,
                                                               pools_stats_context_exit.allocated_chunks);

  __printf ("%sAllocated heap bytes in the context:  %5u -> %5u (%+5d, local %5u, peak %5u)\n",
            indent_prefix,
            int_data_p->heap_stats_context_enter.allocated_bytes,
            heap_stats_context_exit.allocated_bytes,
            heap_stats_context_exit.allocated_bytes - int_data_p->heap_stats_context_enter.allocated_bytes,
            int_data_p->context_peak_allocated_heap_bytes,
            heap_stats_context_exit.global_peak_allocated_bytes);

  __printf ("%sWaste heap bytes in the context:      %5u -> %5u (%+5d, local %5u, peak %5u)\n",
            indent_prefix,
            int_data_p->heap_stats_context_enter.waste_bytes,
            heap_stats_context_exit.waste_bytes,
            heap_stats_context_exit.waste_bytes - int_data_p->heap_stats_context_enter.waste_bytes,
            int_data_p->context_peak_waste_heap_bytes,
            heap_stats_context_exit.global_peak_waste_bytes);

  __printf ("%sPools count in the context:           %5u -> %5u (%+5d, local %5u, peak %5u)\n",
            indent_prefix,
            int_data_p->pools_stats_context_enter.pools_count,
            pools_stats_context_exit.pools_count,
            pools_stats_context_exit.pools_count - int_data_p->pools_stats_context_enter.pools_count,
            int_data_p->context_peak_pools_count,
            pools_stats_context_exit.global_peak_pools_count);

  __printf ("%sAllocated pool chunks in the context: %5u -> %5u (%+5d, local %5u, peak %5u)\n",
            indent_prefix,
            int_data_p->pools_stats_context_enter.allocated_chunks,
            pools_stats_context_exit.allocated_chunks,
            pools_stats_context_exit.allocated_chunks - int_data_p->pools_stats_context_enter.allocated_chunks,
            int_data_p->context_peak_allocated_pool_chunks,
            pools_stats_context_exit.global_peak_allocated_chunks);

  __printf ("\n%s--- End of interpretation of a block at position %u ---\n\n",
            indent_prefix, (uint32_t) block_position);
}

static void
interp_mem_stats_opcode_enter (opcode_counter_t opcode_position,
                               mem_heap_stats_t *out_heap_stats_p,
                               mem_pools_stats_t *out_pools_stats_p)
{
  if (likely (!interp_mem_stats_enabled))
  {
    return;
  }

  char indent_prefix[interp_mem_stats_print_indentation + 2];
  __memset (indent_prefix, ' ', sizeof (indent_prefix));
  indent_prefix [interp_mem_stats_print_indentation] = '|';
  indent_prefix [interp_mem_stats_print_indentation + 1] = '\0';
  
  interp_mem_get_stats (out_heap_stats_p,
                        out_pools_stats_p,
                        true, false);

  opcode_t opcode = read_opcode (opcode_position);

  __printf ("%s-- Opcode: %s (position %u) --\n",
            indent_prefix, __op_names [opcode.op_idx], (uint32_t) opcode_position);

  interp_mem_stats_print_indentation += 5;
}

static void
interp_mem_stats_opcode_exit (int_data_t *int_data_p,
                              opcode_counter_t opcode_position,
                              mem_heap_stats_t *heap_stats_before_p,
                              mem_pools_stats_t *pools_stats_before_p)
{
  if (likely (!interp_mem_stats_enabled))
  {
    return;
  }

  interp_mem_stats_print_indentation -= 5;

  char indent_prefix[interp_mem_stats_print_indentation + 2];
  __memset (indent_prefix, ' ', sizeof (indent_prefix));
  indent_prefix [interp_mem_stats_print_indentation] = '|';
  indent_prefix [interp_mem_stats_print_indentation + 1] = '\0';
  
  mem_heap_stats_t heap_stats_after;
  mem_pools_stats_t pools_stats_after;

  interp_mem_get_stats (&heap_stats_after,
                        &pools_stats_after,
                        false, true);

  int_data_p->context_peak_allocated_heap_bytes = JERRY_MAX (int_data_p->context_peak_allocated_heap_bytes,
                                                             heap_stats_after.allocated_bytes);
  int_data_p->context_peak_waste_heap_bytes = JERRY_MAX (int_data_p->context_peak_waste_heap_bytes,
                                                         heap_stats_after.waste_bytes);
  int_data_p->context_peak_pools_count = JERRY_MAX (int_data_p->context_peak_pools_count,
                                                    pools_stats_after.pools_count);
  int_data_p->context_peak_allocated_pool_chunks = JERRY_MAX (int_data_p->context_peak_allocated_pool_chunks,
                                                              pools_stats_after.allocated_chunks);

  opcode_t opcode = read_opcode (opcode_position);

  __printf ("%s Allocated heap bytes:  %5u -> %5u (%+5d, local %5u, peak %5u)\n",
            indent_prefix,
            heap_stats_before_p->allocated_bytes,
            heap_stats_after.allocated_bytes,
            heap_stats_after.allocated_bytes - heap_stats_before_p->allocated_bytes,
            heap_stats_after.peak_allocated_bytes - JERRY_MAX (heap_stats_before_p->allocated_bytes,
                                                               heap_stats_after.allocated_bytes),
            heap_stats_after.global_peak_allocated_bytes);

  if (heap_stats_before_p->waste_bytes != heap_stats_after.waste_bytes)
  {
    __printf ("%s Waste heap bytes:      %5u -> %5u (%+5d, local %5u, peak %5u)\n",
              indent_prefix,
              heap_stats_before_p->waste_bytes,
              heap_stats_after.waste_bytes,
              heap_stats_after.waste_bytes - heap_stats_before_p->waste_bytes,
              heap_stats_after.peak_waste_bytes - JERRY_MAX (heap_stats_before_p->waste_bytes,
                                                             heap_stats_after.waste_bytes),
              heap_stats_after.global_peak_waste_bytes);
  }

  if (pools_stats_before_p->pools_count != pools_stats_after.pools_count)
  {
    __printf ("%s Pools:                 %5u -> %5u (%+5d, local %5u, peak %5u)\n",
              indent_prefix,
              pools_stats_before_p->pools_count,
              pools_stats_after.pools_count,
              pools_stats_after.pools_count - pools_stats_before_p->pools_count,
              pools_stats_after.peak_pools_count - JERRY_MAX (pools_stats_before_p->pools_count,
                                                              pools_stats_after.pools_count),
              pools_stats_after.global_peak_pools_count);
  }

  if (pools_stats_before_p->allocated_chunks != pools_stats_after.allocated_chunks)
  {
    __printf ("%s Allocated pool chunks: %5u -> %5u (%+5d, local %5u, peak %5u)\n",
              indent_prefix,
              pools_stats_before_p->allocated_chunks,
              pools_stats_after.allocated_chunks,
              pools_stats_after.allocated_chunks - pools_stats_before_p->allocated_chunks,
              pools_stats_after.peak_allocated_chunks - JERRY_MAX (pools_stats_before_p->allocated_chunks,
                                                                   pools_stats_after.allocated_chunks),
              pools_stats_after.global_peak_allocated_chunks);
  }

  __printf ("%s-- End of execution of opcode %s (position %u) --\n\n",
            indent_prefix, __op_names [opcode.op_idx], opcode_position);
}
#endif /* MEM_STATS */

/**
 * Initialize interpreter.
 */
void
init_int (const opcode_t *program_p, /**< pointer to byte-code program */
          bool dump_mem_stats) /** dump per-opcode memory usage change statistics */
{
#ifdef MEM_STATS
  interp_mem_stats_enabled = dump_mem_stats;
#else /* MEM_STATS */
  JERRY_ASSERT (!dump_mem_stats);
#endif /* !MEM_STATS */

  JERRY_ASSERT (__program == NULL);

  __program = program_p;
} /* init_int */

bool
run_int (void)
{
  JERRY_ASSERT (__program != NULL);

#ifdef MEM_STATS
  interp_mem_stats_print_legend ();
#endif /* MEM_STATS */

  bool is_strict = false;
  opcode_counter_t start_pos = 0;

  opcode_t first_opcode = read_opcode (start_pos);
  if (first_opcode.op_idx == __op__idx_meta
      && first_opcode.data.meta.type == OPCODE_META_TYPE_STRICT_CODE)
  {
    is_strict = true;
    start_pos++;
  }

  ecma_init_builtins ();

  ecma_object_t *glob_obj_p = ecma_builtin_get_global_object ();

  ecma_object_t *lex_env_p = ecma_op_create_global_environment (glob_obj_p);
  ecma_value_t this_binding_value = ecma_make_object_value (glob_obj_p);

  ecma_completion_value_t completion = run_int_from_pos (start_pos,
                                                         this_binding_value,
                                                         lex_env_p,
                                                         is_strict,
                                                         false);

  switch ((ecma_completion_type_t) completion.type)
  {
    case ECMA_COMPLETION_TYPE_NORMAL:
    case ECMA_COMPLETION_TYPE_META:
    {
      JERRY_UNREACHABLE ();
    }
    case ECMA_COMPLETION_TYPE_EXIT:
    {
      ecma_deref_object (glob_obj_p);
      ecma_deref_object (lex_env_p);
      ecma_finalize_builtins ();
      ecma_gc_run (ECMA_GC_GEN_COUNT - 1);

      return ecma_is_value_true (completion.u.value);
    }
    case ECMA_COMPLETION_TYPE_BREAK:
    case ECMA_COMPLETION_TYPE_CONTINUE:
    case ECMA_COMPLETION_TYPE_RETURN:
    {
      /* SyntaxError should be treated as an early error */
      JERRY_UNREACHABLE ();
    }
#ifdef CONFIG_ECMA_EXCEPTION_SUPPORT
    case ECMA_COMPLETION_TYPE_THROW:
    {
      jerry_exit (ERR_UNHANDLED_EXCEPTION);
    }
#endif /* CONFIG_ECMA_EXCEPTION_SUPPORT */
  }

  JERRY_UNREACHABLE ();
}

ecma_completion_value_t
run_int_loop (int_data_t *int_data)
{
  ecma_completion_value_t completion;

#ifdef MEM_STATS
  mem_heap_stats_t heap_stats_before;
  mem_pools_stats_t pools_stats_before;

  __memset (&heap_stats_before, 0, sizeof (heap_stats_before));
  __memset (&pools_stats_before, 0, sizeof (pools_stats_before));
#endif /* MEM_STATS */

  while (true)
  {
    do
    {
      const opcode_t *curr = &__program[int_data->pos];

#ifdef MEM_STATS
      const opcode_counter_t opcode_pos = int_data->pos;

      interp_mem_stats_opcode_enter (opcode_pos,
                                     &heap_stats_before,
                                     &pools_stats_before);
#endif /* MEM_STATS */

      completion = __opfuncs[curr->op_idx] (*curr, int_data);

#ifdef MEM_STATS
      interp_mem_stats_opcode_exit (int_data,
                                    opcode_pos,
                                    &heap_stats_before,
                                    &pools_stats_before);
#endif /* MEM_STATS */

      JERRY_ASSERT (!ecma_is_completion_value_normal (completion)
                    || ecma_is_completion_value_empty (completion));
    }
    while (ecma_is_completion_value_normal (completion));

    if (completion.type == ECMA_COMPLETION_TYPE_BREAK
        || completion.type == ECMA_COMPLETION_TYPE_CONTINUE)
    {
      JERRY_UNIMPLEMENTED ();

      continue;
    }

    if (ecma_is_completion_value_meta (completion))
    {
      completion = ecma_make_empty_completion_value ();
    }

    return completion;
  }
}

ecma_completion_value_t
run_int_from_pos (opcode_counter_t start_pos,
                  ecma_value_t this_binding_value,
                  ecma_object_t *lex_env_p,
                  bool is_strict,
                  bool is_eval_code)
{
  ecma_completion_value_t completion;

  const opcode_t *curr = &__program[start_pos];
  JERRY_ASSERT (curr->op_idx == __op__idx_reg_var_decl);

  const idx_t min_reg_num = curr->data.reg_var_decl.min;
  const idx_t max_reg_num = curr->data.reg_var_decl.max;
  JERRY_ASSERT (max_reg_num >= min_reg_num);

  const uint32_t regs_num = (uint32_t) (max_reg_num - min_reg_num + 1);

  ecma_value_t regs[ regs_num ];

  /* memseting with zero initializes each 'register' to empty value */
  __memset (regs, 0, sizeof (regs));
  JERRY_ASSERT (ecma_is_value_empty (regs[0]));

  int_data_t int_data;
  int_data.pos = (opcode_counter_t) (start_pos + 1);
  int_data.this_binding = this_binding_value;
  int_data.lex_env_p = lex_env_p;
  int_data.is_strict = is_strict;
  int_data.is_eval_code = is_eval_code;
  int_data.min_reg_num = min_reg_num;
  int_data.max_reg_num = max_reg_num;
  int_data.regs_p = regs;

#ifdef MEM_STATS
  interp_mem_stats_context_enter (&int_data, start_pos);
#endif /* MEM_STATS */

  completion = run_int_loop (&int_data);

  for (uint32_t reg_index = 0;
       reg_index < regs_num;
       reg_index++)
  {
    ecma_free_value (regs[ reg_index ], true);
  }

#ifdef MEM_STATS
  interp_mem_stats_context_exit (&int_data, start_pos);
#endif /* MEM_STATS */

  return completion;
}

/**
 * Get specified opcode from the program.
 */
opcode_t
read_opcode (opcode_counter_t counter) /**< opcode counter */
{
  return __program[ counter ];
} /* read_opcode */
