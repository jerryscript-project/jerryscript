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

#include "ecma-builtins.h"
#include "ecma-gc.h"
#include "ecma-helpers.h"
#include "ecma-init-finalize.h"
#include "ecma-lex-env.h"
#include "ecma-literal-storage.h"
#include "jmem.h"
#include "jcontext.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmainitfinalize Initialization and finalization of ECMA components
 * @{
 */

/**
 * Maximum number of GC loops on cleanup.
 */
#define JERRY_GC_LOOP_LIMIT 100

/**
 * Initialize ECMA components
 */
void
ecma_init (void)
{
#if (JERRY_GC_MARK_LIMIT != 0)
  JERRY_CONTEXT (ecma_gc_mark_recursion_limit) = JERRY_GC_MARK_LIMIT;
#endif /* (JERRY_GC_MARK_LIMIT != 0) */

  ecma_init_global_environment ();

#if ENABLED (JERRY_PROPRETY_HASHMAP)
  JERRY_CONTEXT (ecma_prop_hashmap_alloc_state) = ECMA_PROP_HASHMAP_ALLOC_ON;
  JERRY_CONTEXT (status_flags) &= (uint32_t) ~ECMA_STATUS_HIGH_PRESSURE_GC;
#endif /* ENABLED (JERRY_PROPRETY_HASHMAP) */

#if (JERRY_STACK_LIMIT != 0)
  volatile int sp;
  JERRY_CONTEXT (stack_base) = (uintptr_t) &sp;
#endif /* (JERRY_STACK_LIMIT != 0) */

#if ENABLED (JERRY_ES2015_BUILTIN_PROMISE)
  ecma_job_queue_init ();
#endif /* ENABLED (JERRY_ES2015_BUILTIN_PROMISE) */

#if ENABLED (JERRY_ES2015)
  JERRY_CONTEXT (current_new_target) = NULL;
  JERRY_CONTEXT (current_function_obj_p) = NULL;
#endif /* ENABLED (JERRY_ES2015) */
} /* ecma_init */

/**
 * Finalize ECMA components
 */
void
ecma_finalize (void)
{
#if ENABLED (JERRY_ES2015)
  JERRY_ASSERT (JERRY_CONTEXT (current_new_target) == NULL);
  JERRY_ASSERT (JERRY_CONTEXT (current_function_obj_p) == NULL);
#endif /* ENABLED (JERRY_ES2015) */

  ecma_finalize_global_environment ();
  uint8_t runs = 0;
  do
  {
    ecma_finalize_builtins ();
    ecma_gc_run ();
    if (++runs >= JERRY_GC_LOOP_LIMIT)
    {
      jerry_fatal (ERR_UNTERMINATED_GC_LOOPS);
    }
  }
  while (JERRY_CONTEXT (ecma_gc_new_objects) != 0);
  ecma_finalize_lit_storage ();
} /* ecma_finalize */

/**
 * @}
 * @}
 */
