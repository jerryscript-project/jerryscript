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

#include "bytecode-data.h"
#include "ecma-alloc.h"
#include "ecma-builtins.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "jrt.h"
#include "jrt-libc-includes.h"
#include "mem-allocator.h"
#include "vm.h"
#include "vm-stack.h"
#include "opcodes.h"
#include "opcodes-ecma-support.h"

/**
 * Top (current) interpreter context
 */
vm_frame_ctx_t *vm_top_context_p = NULL;

static const opfunc __opfuncs[VM_OP__COUNT] =
{
#define VM_OP_0(opcode_name, opcode_name_uppercase) \
  [ VM_OP_ ## opcode_name_uppercase ] = opfunc_ ## opcode_name,
#define VM_OP_1(opcode_name, opcode_name_uppercase, arg1, arg1_type) \
  [ VM_OP_ ## opcode_name_uppercase ] = opfunc_ ## opcode_name,
#define VM_OP_2(opcode_name, opcode_name_uppercase, arg1, arg1_type, arg2, arg2_type) \
  [ VM_OP_ ## opcode_name_uppercase ] = opfunc_ ## opcode_name,
#define VM_OP_3(opcode_name, opcode_name_uppercase, arg1, arg1_type, arg2, arg2_type, arg3, arg3_type) \
  [ VM_OP_ ## opcode_name_uppercase ] = opfunc_ ## opcode_name,

#include "vm-opcodes.inc.h"
};

JERRY_STATIC_ASSERT (sizeof (vm_instr_t) <= 4);

const bytecode_data_header_t *__program = NULL;

#ifdef MEM_STATS
static const char *__op_names[VM_OP__COUNT] =
{
#define VM_OP_0(opcode_name, opcode_name_uppercase) \
  #opcode_name,
#define VM_OP_1(opcode_name, opcode_name_uppercase, arg1, arg1_type) \
  #opcode_name,
#define VM_OP_2(opcode_name, opcode_name_uppercase, arg1, arg1_type, arg2, arg2_type) \
  #opcode_name,
#define VM_OP_3(opcode_name, opcode_name_uppercase, arg1, arg1_type, arg2, arg2_type, arg3, arg3_type) \
  #opcode_name,

#include "vm-opcodes.inc.h"
};

#define INTERP_MEM_PRINT_INDENTATION_STEP (5)
#define INTERP_MEM_PRINT_INDENTATION_MAX  (125)
static uint32_t interp_mem_stats_print_indentation = 0;
static bool interp_mem_stats_enabled = false;

static void
interp_mem_stats_print_legend (void)
{
  if (likely (!interp_mem_stats_enabled))
  {
    return;
  }

  printf ("----- Legend of memory usage trace during interpretation -----\n\n"
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

  /* Requesting to free as much memory as we currently can */
  ecma_try_to_give_back_some_memory (MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_HIGH);

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
interp_mem_stats_context_enter (vm_frame_ctx_t *frame_ctx_p,
                                vm_instr_counter_t block_position)
{
  if (likely (!interp_mem_stats_enabled))
  {
    return;
  }

  const uint32_t indentation = JERRY_MIN (interp_mem_stats_print_indentation,
                                          INTERP_MEM_PRINT_INDENTATION_MAX);

  char indent_prefix[INTERP_MEM_PRINT_INDENTATION_MAX + 2];
  memset (indent_prefix, ' ', sizeof (indent_prefix));
  indent_prefix[indentation] = '|';
  indent_prefix[indentation + 1] = '\0';

  frame_ctx_p->context_peak_allocated_heap_bytes = 0;
  frame_ctx_p->context_peak_waste_heap_bytes = 0;
  frame_ctx_p->context_peak_pools_count = 0;
  frame_ctx_p->context_peak_allocated_pool_chunks = 0;

  interp_mem_get_stats (&frame_ctx_p->heap_stats_context_enter,
                        &frame_ctx_p->pools_stats_context_enter,
                        false, false);

  printf ("\n%s--- Beginning interpretation of a block at position %u ---\n"
          "%s Allocated heap bytes:  %5u\n"
          "%s Waste heap bytes:      %5u\n"
          "%s Pools:                 %5u\n"
          "%s Allocated pool chunks: %5u\n\n",
          indent_prefix, (uint32_t) block_position,
          indent_prefix, (uint32_t) frame_ctx_p->heap_stats_context_enter.allocated_bytes,
          indent_prefix, (uint32_t) frame_ctx_p->heap_stats_context_enter.waste_bytes,
          indent_prefix, (uint32_t) frame_ctx_p->pools_stats_context_enter.pools_count,
          indent_prefix, (uint32_t) frame_ctx_p->pools_stats_context_enter.allocated_chunks);
}

static void
interp_mem_stats_context_exit (vm_frame_ctx_t *frame_ctx_p,
                               vm_instr_counter_t block_position)
{
  if (likely (!interp_mem_stats_enabled))
  {
    return;
  }

  const uint32_t indentation = JERRY_MIN (interp_mem_stats_print_indentation,
                                          INTERP_MEM_PRINT_INDENTATION_MAX);

  char indent_prefix[INTERP_MEM_PRINT_INDENTATION_MAX + 2];
  memset (indent_prefix, ' ', sizeof (indent_prefix));
  indent_prefix[indentation] = '|';
  indent_prefix[indentation + 1] = '\0';

  mem_heap_stats_t heap_stats_context_exit;
  mem_pools_stats_t pools_stats_context_exit;

  interp_mem_get_stats (&heap_stats_context_exit,
                        &pools_stats_context_exit,
                        false, true);

  frame_ctx_p->context_peak_allocated_heap_bytes -= JERRY_MAX (frame_ctx_p->heap_stats_context_enter.allocated_bytes,
                                                              heap_stats_context_exit.allocated_bytes);
  frame_ctx_p->context_peak_waste_heap_bytes -= JERRY_MAX (frame_ctx_p->heap_stats_context_enter.waste_bytes,
                                                          heap_stats_context_exit.waste_bytes);
  frame_ctx_p->context_peak_pools_count -= JERRY_MAX (frame_ctx_p->pools_stats_context_enter.pools_count,
                                                     pools_stats_context_exit.pools_count);
  frame_ctx_p->context_peak_allocated_pool_chunks -= JERRY_MAX (frame_ctx_p->pools_stats_context_enter.allocated_chunks,
                                                               pools_stats_context_exit.allocated_chunks);

  printf ("%sAllocated heap bytes in the context:  %5u -> %5u (%+5d, local %5u, peak %5u)\n",
          indent_prefix,
          (uint32_t) frame_ctx_p->heap_stats_context_enter.allocated_bytes,
          (uint32_t) heap_stats_context_exit.allocated_bytes,
          (uint32_t) (heap_stats_context_exit.allocated_bytes - frame_ctx_p->heap_stats_context_enter.allocated_bytes),
          (uint32_t) frame_ctx_p->context_peak_allocated_heap_bytes,
          (uint32_t) heap_stats_context_exit.global_peak_allocated_bytes);

  printf ("%sWaste heap bytes in the context:      %5u -> %5u (%+5d, local %5u, peak %5u)\n",
          indent_prefix,
          (uint32_t) frame_ctx_p->heap_stats_context_enter.waste_bytes,
          (uint32_t) heap_stats_context_exit.waste_bytes,
          (uint32_t) (heap_stats_context_exit.waste_bytes - frame_ctx_p->heap_stats_context_enter.waste_bytes),
          (uint32_t) frame_ctx_p->context_peak_waste_heap_bytes,
          (uint32_t) heap_stats_context_exit.global_peak_waste_bytes);

  printf ("%sPools count in the context:           %5u -> %5u (%+5d, local %5u, peak %5u)\n",
          indent_prefix,
          (uint32_t) frame_ctx_p->pools_stats_context_enter.pools_count,
          (uint32_t) pools_stats_context_exit.pools_count,
          (uint32_t) (pools_stats_context_exit.pools_count - frame_ctx_p->pools_stats_context_enter.pools_count),
          (uint32_t) frame_ctx_p->context_peak_pools_count,
          (uint32_t) pools_stats_context_exit.global_peak_pools_count);

  printf ("%sAllocated pool chunks in the context: %5u -> %5u (%+5d, local %5u, peak %5u)\n",
          indent_prefix,
          (uint32_t) frame_ctx_p->pools_stats_context_enter.allocated_chunks,
          (uint32_t) pools_stats_context_exit.allocated_chunks,
          (uint32_t) (pools_stats_context_exit.allocated_chunks -
                      frame_ctx_p->pools_stats_context_enter.allocated_chunks),
          (uint32_t) frame_ctx_p->context_peak_allocated_pool_chunks,
          (uint32_t) pools_stats_context_exit.global_peak_allocated_chunks);

  printf ("\n%s--- End of interpretation of a block at position %u ---\n\n",
          indent_prefix, (uint32_t) block_position);
}

static void
interp_mem_stats_opcode_enter (const vm_instr_t *instrs_p,
                               vm_instr_counter_t instr_position,
                               mem_heap_stats_t *out_heap_stats_p,
                               mem_pools_stats_t *out_pools_stats_p)
{
  if (likely (!interp_mem_stats_enabled))
  {
    return;
  }

  const uint32_t indentation = JERRY_MIN (interp_mem_stats_print_indentation,
                                          INTERP_MEM_PRINT_INDENTATION_MAX);

  char indent_prefix[INTERP_MEM_PRINT_INDENTATION_MAX + 2];
  memset (indent_prefix, ' ', sizeof (indent_prefix));
  indent_prefix[indentation] = '|';
  indent_prefix[indentation + 1] = '\0';

  interp_mem_get_stats (out_heap_stats_p,
                        out_pools_stats_p,
                        true, false);

  vm_instr_t instr = vm_get_instr (instrs_p, instr_position);

  printf ("%s-- Opcode: %s (position %u) --\n",
          indent_prefix, __op_names[instr.op_idx], (uint32_t) instr_position);

  interp_mem_stats_print_indentation += INTERP_MEM_PRINT_INDENTATION_STEP;
}

static void
interp_mem_stats_opcode_exit (vm_frame_ctx_t *frame_ctx_p,
                              vm_instr_counter_t instr_position,
                              mem_heap_stats_t *heap_stats_before_p,
                              mem_pools_stats_t *pools_stats_before_p)
{
  if (likely (!interp_mem_stats_enabled))
  {
    return;
  }

  interp_mem_stats_print_indentation -= INTERP_MEM_PRINT_INDENTATION_STEP;

  const uint32_t indentation = JERRY_MIN (interp_mem_stats_print_indentation,
                                          INTERP_MEM_PRINT_INDENTATION_MAX);

  char indent_prefix[INTERP_MEM_PRINT_INDENTATION_MAX + 2];
  memset (indent_prefix, ' ', sizeof (indent_prefix));
  indent_prefix[indentation] = '|';
  indent_prefix[indentation + 1] = '\0';

  mem_heap_stats_t heap_stats_after;
  mem_pools_stats_t pools_stats_after;

  interp_mem_get_stats (&heap_stats_after,
                        &pools_stats_after,
                        false, true);

  frame_ctx_p->context_peak_allocated_heap_bytes = JERRY_MAX (frame_ctx_p->context_peak_allocated_heap_bytes,
                                                             heap_stats_after.allocated_bytes);
  frame_ctx_p->context_peak_waste_heap_bytes = JERRY_MAX (frame_ctx_p->context_peak_waste_heap_bytes,
                                                         heap_stats_after.waste_bytes);
  frame_ctx_p->context_peak_pools_count = JERRY_MAX (frame_ctx_p->context_peak_pools_count,
                                                    pools_stats_after.pools_count);
  frame_ctx_p->context_peak_allocated_pool_chunks = JERRY_MAX (frame_ctx_p->context_peak_allocated_pool_chunks,
                                                              pools_stats_after.allocated_chunks);

  vm_instr_t instr = vm_get_instr (frame_ctx_p->bytecode_header_p->instrs_p, instr_position);

  printf ("%s Allocated heap bytes:  %5u -> %5u (%+5d, local %5u, peak %5u)\n",
          indent_prefix,
          (uint32_t) heap_stats_before_p->allocated_bytes,
          (uint32_t) heap_stats_after.allocated_bytes,
          (uint32_t) (heap_stats_after.allocated_bytes - heap_stats_before_p->allocated_bytes),
          (uint32_t) (heap_stats_after.peak_allocated_bytes - JERRY_MAX (heap_stats_before_p->allocated_bytes,
                                                                         heap_stats_after.allocated_bytes)),
          (uint32_t) heap_stats_after.global_peak_allocated_bytes);

  if (heap_stats_before_p->waste_bytes != heap_stats_after.waste_bytes)
  {
    printf ("%s Waste heap bytes:      %5u -> %5u (%+5d, local %5u, peak %5u)\n",
            indent_prefix,
            (uint32_t) heap_stats_before_p->waste_bytes,
            (uint32_t) heap_stats_after.waste_bytes,
            (uint32_t) (heap_stats_after.waste_bytes - heap_stats_before_p->waste_bytes),
            (uint32_t) (heap_stats_after.peak_waste_bytes - JERRY_MAX (heap_stats_before_p->waste_bytes,
                                                                       heap_stats_after.waste_bytes)),
            (uint32_t) heap_stats_after.global_peak_waste_bytes);
  }

  if (pools_stats_before_p->pools_count != pools_stats_after.pools_count)
  {
    printf ("%s Pools:                 %5u -> %5u (%+5d, local %5u, peak %5u)\n",
            indent_prefix,
            (uint32_t) pools_stats_before_p->pools_count,
            (uint32_t) pools_stats_after.pools_count,
            (uint32_t) (pools_stats_after.pools_count - pools_stats_before_p->pools_count),
            (uint32_t) (pools_stats_after.peak_pools_count - JERRY_MAX (pools_stats_before_p->pools_count,
                                                                        pools_stats_after.pools_count)),
            (uint32_t) pools_stats_after.global_peak_pools_count);
  }

  if (pools_stats_before_p->allocated_chunks != pools_stats_after.allocated_chunks)
  {
    printf ("%s Allocated pool chunks: %5u -> %5u (%+5d, local %5u, peak %5u)\n",
            indent_prefix,
            (uint32_t) pools_stats_before_p->allocated_chunks,
            (uint32_t) pools_stats_after.allocated_chunks,
            (uint32_t) (pools_stats_after.allocated_chunks - pools_stats_before_p->allocated_chunks),
            (uint32_t) (pools_stats_after.peak_allocated_chunks - JERRY_MAX (pools_stats_before_p->allocated_chunks,
                                                                             pools_stats_after.allocated_chunks)),
            (uint32_t) pools_stats_after.global_peak_allocated_chunks);
  }

  printf ("%s-- End of execution of opcode %s (position %u) --\n\n",
          indent_prefix, __op_names[instr.op_idx], instr_position);
}
#endif /* MEM_STATS */

/**
 * Initialize interpreter.
 */
void
vm_init (const bytecode_data_header_t *program_p, /**< pointer to byte-code data */
         bool dump_mem_stats) /** dump per-instruction memory usage change statistics */
{
#ifdef MEM_STATS
  interp_mem_stats_enabled = dump_mem_stats;
#else /* MEM_STATS */
  JERRY_ASSERT (!dump_mem_stats);
#endif /* !MEM_STATS */

  JERRY_ASSERT (__program == NULL);

  vm_stack_init ();

  __program = program_p;
} /* vm_init */

/**
 * Cleanup interpreter
 */
void
vm_finalize (void)
{
  vm_stack_finalize ();

  __program = NULL;
} /* vm_finalize */

/**
 * Run global code
 */
jerry_completion_code_t
vm_run_global (void)
{
  JERRY_ASSERT (__program != NULL);
  JERRY_ASSERT (vm_top_context_p == NULL);

#ifdef MEM_STATS
  interp_mem_stats_print_legend ();
#endif /* MEM_STATS */

  bool is_strict = __program->is_strict;
  vm_instr_counter_t start_pos = 0;

  ecma_object_t *glob_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_GLOBAL);
  ecma_object_t *lex_env_p = ecma_get_global_environment ();

  ecma_completion_value_t completion = vm_run_from_pos (__program,
                                                        start_pos,
                                                        ecma_make_object_value (glob_obj_p),
                                                        lex_env_p,
                                                        is_strict,
                                                        false,
                                                        NULL);

  jerry_completion_code_t ret_code;

  if (ecma_is_completion_value_return (completion))
  {
    JERRY_ASSERT (ecma_is_value_undefined (ecma_get_completion_value_value (completion)));

    ret_code = JERRY_COMPLETION_CODE_OK;
  }
  else
  {
    JERRY_ASSERT (ecma_is_completion_value_throw (completion));

    ret_code = JERRY_COMPLETION_CODE_UNHANDLED_EXCEPTION;
  }

  ecma_free_completion_value (completion);

  ecma_deref_object (glob_obj_p);
  ecma_deref_object (lex_env_p);

  JERRY_ASSERT (vm_top_context_p == NULL);

  return ret_code;
} /* vm_run_global */

/**
 * Run specified eval-mode bytecode
 *
 * @return completion value
 */
ecma_completion_value_t
vm_run_eval (const bytecode_data_header_t *bytecode_data_p, /**< byte-code data header */
             bool is_direct) /**< is eval called in direct mode? */
{
  vm_instr_counter_t first_instr_index = 0u;

  bool is_strict = bytecode_data_p->is_strict;

  ecma_value_t this_binding;
  ecma_object_t *lex_env_p;

  /* ECMA-262 v5, 10.4.2 */
  if (is_direct)
  {
    this_binding = vm_get_this_binding ();
    lex_env_p = vm_get_lex_env ();
  }
  else
  {
    this_binding = ecma_make_object_value (ecma_builtin_get (ECMA_BUILTIN_ID_GLOBAL));
    lex_env_p = ecma_get_global_environment ();
  }

  if (is_strict)
  {
    ecma_object_t *strict_lex_env_p = ecma_create_decl_lex_env (lex_env_p);
    ecma_deref_object (lex_env_p);

    lex_env_p = strict_lex_env_p;
  }

  ecma_completion_value_t completion = vm_run_from_pos (bytecode_data_p,
                                                        first_instr_index,
                                                        this_binding,
                                                        lex_env_p,
                                                        is_strict,
                                                        true,
                                                        NULL);

  if (ecma_is_completion_value_return (completion))
  {
    completion = ecma_make_normal_completion_value (ecma_get_completion_value_value (completion));
  }
  else
  {
    JERRY_ASSERT (ecma_is_completion_value_throw (completion));
  }

  ecma_deref_object (lex_env_p);
  ecma_free_value (this_binding, true);

  return completion;
} /* vm_run_eval */

/**
 * Run interpreter loop using specified context
 *
 * Note:
 *      The interpreter loop stops upon receiving completion value that is normal completion value.
 *
 * @return If the received completion value is not meta completion value (ECMA_COMPLETION_TYPE_META), then
 *          the completion value is returned as is;
 *         Otherwise - the completion value is discarded and normal empty completion value is returned.
 */
ecma_completion_value_t
vm_loop (vm_frame_ctx_t *frame_ctx_p, /**< interpreter context */
         vm_run_scope_t *run_scope_p) /**< current run scope,
                                       *   or NULL - if there is no active run scope */
{
  ecma_completion_value_t completion;

#ifdef MEM_STATS
  mem_heap_stats_t heap_stats_before;
  mem_pools_stats_t pools_stats_before;

  memset (&heap_stats_before, 0, sizeof (heap_stats_before));
  memset (&pools_stats_before, 0, sizeof (pools_stats_before));
#endif /* MEM_STATS */

  while (true)
  {
    do
    {
      JERRY_ASSERT (run_scope_p == NULL
                    || (run_scope_p->start_oc <= frame_ctx_p->pos
                        && frame_ctx_p->pos <= run_scope_p->end_oc));

      const vm_instr_t *curr = &frame_ctx_p->bytecode_header_p->instrs_p[frame_ctx_p->pos];

#ifdef MEM_STATS
      const vm_instr_counter_t instr_pos = frame_ctx_p->pos;

      interp_mem_stats_opcode_enter (frame_ctx_p->bytecode_header_p->instrs_p,
                                     instr_pos,
                                     &heap_stats_before,
                                     &pools_stats_before);
#endif /* MEM_STATS */

      completion = __opfuncs[curr->op_idx] (*curr, frame_ctx_p);

#ifdef CONFIG_VM_RUN_GC_AFTER_EACH_OPCODE
      ecma_gc_run ();
#endif /* CONFIG_VM_RUN_GC_AFTER_EACH_OPCODE */

#ifdef MEM_STATS
      interp_mem_stats_opcode_exit (frame_ctx_p,
                                    instr_pos,
                                    &heap_stats_before,
                                    &pools_stats_before);
#endif /* MEM_STATS */

      JERRY_ASSERT (!ecma_is_completion_value_normal (completion)
                    || ecma_is_completion_value_empty (completion));
    }
    while (ecma_is_completion_value_normal (completion));

    if (ecma_is_completion_value_jump (completion))
    {
      vm_instr_counter_t target = ecma_get_jump_target_from_completion_value (completion);

      /*
       * TODO:
       *      Implement instantiation of run scopes for global scope, functions and eval scope.
       *      Currently, correctness of jumps without run scope set is guaranteed through byte-code semantics.
       */
      if (run_scope_p == NULL /* if no run scope set */
          || (target >= run_scope_p->start_oc /* or target is within the current run scope */
              && target <= run_scope_p->end_oc))
      {
        frame_ctx_p->pos = target;

        continue;
      }
    }

    if (ecma_is_completion_value_meta (completion))
    {
      completion = ecma_make_empty_completion_value ();
    }

    return completion;
  }
} /* vm_loop */

/**
 * Run the code, starting from specified instruction position
 */
ecma_completion_value_t
vm_run_from_pos (const bytecode_data_header_t *header_p, /**< byte-code data header */
                 vm_instr_counter_t start_pos, /**< position of starting instruction */
                 ecma_value_t this_binding_value, /**< value of 'ThisBinding' */
                 ecma_object_t *lex_env_p, /**< lexical environment to use */
                 bool is_strict, /**< is the code is strict mode code (ECMA-262 v5, 10.1.1) */
                 bool is_eval_code, /**< is the code is eval code (ECMA-262 v5, 10.1) */
                 ecma_collection_header_t *arg_collection_p) /**<
                                                              * - collection of function call arguments,
                                                              *   if arguments for the called function
                                                              *   are placed on registers;
                                                              * - NULL - otherwise.
                                                              */
{
  ecma_completion_value_t completion = ecma_make_empty_completion_value ();

  const vm_instr_t *instrs_p = header_p->instrs_p;
  const vm_instr_t *curr = &instrs_p[start_pos];
  JERRY_ASSERT (curr->op_idx == VM_OP_REG_VAR_DECL);

  mem_cpointer_t *declarations_p = MEM_CP_GET_POINTER (mem_cpointer_t, header_p->declarations_cp);
  for (uint16_t func_scope_index = 0;
       func_scope_index < header_p->func_scopes_count && ecma_is_completion_value_empty (completion);
       func_scope_index++)
  {
    bytecode_data_header_t *func_bc_header_p = MEM_CP_GET_NON_NULL_POINTER (bytecode_data_header_t,
                                                                            declarations_p[func_scope_index]);

    if (func_bc_header_p->instrs_p[0].op_idx == VM_OP_FUNC_DECL_N)
    {
      completion = vm_function_declaration (func_bc_header_p,
                                            is_strict,
                                            is_eval_code,
                                            lex_env_p);

    }
  }

  lit_cpointer_t *lit_ids_p = (lit_cpointer_t *) (declarations_p + header_p->func_scopes_count);
  for (uint16_t var_decl_index = 0;
       var_decl_index < header_p->var_decls_count && ecma_is_completion_value_empty (completion);
       var_decl_index++)
  {
    lit_cpointer_t lit_cp = lit_ids_p[var_decl_index];

    if (lit_cp.packed_value != NOT_A_LITERAL.packed_value)
    {
      ecma_string_t *var_name_string_p = ecma_new_ecma_string_from_lit_cp (lit_cp);

      if (!ecma_op_has_binding (lex_env_p, var_name_string_p))
      {
        const bool is_configurable_bindings = is_eval_code;

        completion = ecma_op_create_mutable_binding (lex_env_p,
                                                     var_name_string_p,
                                                     is_configurable_bindings);

        JERRY_ASSERT (ecma_is_completion_value_empty (completion));

        /* Skipping SetMutableBinding as we have already checked that there were not
         * any binding with specified name in current lexical environment
         * and CreateMutableBinding sets the created binding's value to undefined */
        JERRY_ASSERT (ecma_is_completion_value_normal_simple_value (ecma_op_get_binding_value (lex_env_p,
                                                                                               var_name_string_p,
                                                                                               true),
                                                                    ECMA_SIMPLE_VALUE_UNDEFINED));
      }

      ecma_deref_ecma_string (var_name_string_p);
    }
  }

  if (!ecma_is_completion_value_empty (completion))
  {
    JERRY_ASSERT (ecma_is_completion_value_throw (completion));
  }
  else
  {
    const uint32_t tmp_regs_num = curr->data.reg_var_decl.tmp_regs_num;
    const uint32_t local_var_regs_num = curr->data.reg_var_decl.local_var_regs_num;
    const uint32_t arg_regs_num = curr->data.reg_var_decl.arg_regs_num;

    uint32_t regs_num = VM_SPECIAL_REGS_NUMBER + tmp_regs_num + local_var_regs_num + arg_regs_num;

    MEM_DEFINE_LOCAL_ARRAY (regs, regs_num, ecma_value_t);

    vm_frame_ctx_t frame_ctx;
    frame_ctx.bytecode_header_p = header_p;
    frame_ctx.pos = (vm_instr_counter_t) (start_pos + 1);
    frame_ctx.lex_env_p = lex_env_p;
    frame_ctx.is_strict = is_strict;
    frame_ctx.is_eval_code = is_eval_code;
    frame_ctx.is_call_in_direct_eval_form = false;
    frame_ctx.tmp_num_p = ecma_alloc_number ();

    vm_stack_add_frame (&frame_ctx.stack_frame, regs, regs_num, local_var_regs_num, arg_regs_num, arg_collection_p);
    vm_stack_frame_set_reg_value (&frame_ctx.stack_frame,
                                  VM_REG_SPECIAL_THIS_BINDING,
                                  ecma_copy_value (this_binding_value, false));

    vm_frame_ctx_t *prev_context_p = vm_top_context_p;
    vm_top_context_p = &frame_ctx;

#ifdef MEM_STATS
    interp_mem_stats_context_enter (&frame_ctx, start_pos);
#endif /* MEM_STATS */

    completion = vm_loop (&frame_ctx, NULL);

    JERRY_ASSERT (ecma_is_completion_value_throw (completion)
                  || ecma_is_completion_value_return (completion));

    vm_top_context_p = prev_context_p;

    vm_stack_free_frame (&frame_ctx.stack_frame);

    ecma_dealloc_number (frame_ctx.tmp_num_p);

#ifdef MEM_STATS
    interp_mem_stats_context_exit (&frame_ctx, start_pos);
#endif /* MEM_STATS */

    MEM_FINALIZE_LOCAL_ARRAY (regs);
  }

  return completion;
} /* vm_run_from_pos */

/**
 * Get specified instruction from the program.
 */
vm_instr_t
vm_get_instr (const vm_instr_t *instrs_p, /**< byte-code array */
              vm_instr_counter_t counter) /**< instruction counter */
{
  return instrs_p[ counter ];
} /* vm_get_instr */

/**
 * Get arguments number, encoded in specified reg_var_decl instruction
 *
 * @return value of "arguments number" reg_var_decl's parameter
 */
uint8_t
vm_get_scope_args_num (const bytecode_data_header_t *bytecode_header_p, /**< byte-code data */
                       vm_instr_counter_t reg_var_decl_oc) /**< position of reg_var_decl instruction */
{
  const vm_instr_t *instrs_p = bytecode_header_p->instrs_p;
  const vm_instr_t *reg_var_decl_instr_p = &instrs_p[reg_var_decl_oc];
  JERRY_ASSERT (reg_var_decl_instr_p->op_idx == VM_OP_REG_VAR_DECL);

  return reg_var_decl_instr_p->data.reg_var_decl.arg_regs_num;
} /* vm_get_scope_args_num */

/**
 * Check whether currently executed code is strict mode code
 *
 * @return true - current code is executed in strict mode,
 *         false - otherwise.
 */
bool
vm_is_strict_mode (void)
{
  JERRY_ASSERT (vm_top_context_p != NULL);

  return vm_top_context_p->is_strict;
} /* vm_is_strict_mode */

/**
 * Check whether currently performed call (on top of call-stack) is performed in form,
 * meeting conditions of 'Direct Call to Eval' (see also: ECMA-262 v5, 15.1.2.1.1)
 *
 * Warning:
 *         the function should only be called from implementation
 *         of built-in 'eval' routine of Global object
 *
 * @return true - currently performed call is performed through 'eval' identifier,
 *                without 'this' argument,
 *         false - otherwise.
 */
bool
vm_is_direct_eval_form_call (void)
{
  if (vm_top_context_p != NULL)
  {
    return vm_top_context_p->is_call_in_direct_eval_form;
  }
  else
  {
    /*
     * There is no any interpreter context, so call is performed not from a script.
     * This implies that the call is indirect.
     */
    return false;
  }
} /* vm_is_direct_eval_form_call */

/**
 * Get this binding of current execution context
 *
 * @return ecma-value
 */
ecma_value_t
vm_get_this_binding (void)
{
  JERRY_ASSERT (vm_top_context_p != NULL);

  return ecma_copy_value (vm_stack_frame_get_reg_value (&vm_top_context_p->stack_frame,
                                                        VM_REG_SPECIAL_THIS_BINDING),
                                                        true);
} /* vm_get_this_binding */

/**
 * Get top lexical environment (variable environment) of current execution context
 *
 * @return lexical environment
 */
ecma_object_t*
vm_get_lex_env (void)
{
  JERRY_ASSERT (vm_top_context_p != NULL);

  ecma_ref_object (vm_top_context_p->lex_env_p);

  return vm_top_context_p->lex_env_p;
} /* vm_get_lex_env */
