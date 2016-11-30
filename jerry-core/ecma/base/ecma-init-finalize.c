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
#include "ecma-lcache.h"
#include "ecma-lex-env.h"
#include "ecma-literal-storage.h"
#include "jmem-allocator.h"
#include "jcontext.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmainitfinalize Initialization and finalization of ECMA components
 * @{
 */

/**
 * Initialize ECMA components
 */
void
ecma_init (void)
{
  ecma_lcache_init ();
  ecma_init_global_lex_env ();

  jmem_register_free_unused_memory_callback (ecma_free_unused_memory);

#ifndef CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE
  JERRY_CONTEXT (ecma_prop_hashmap_alloc_state) = ECMA_PROP_HASHMAP_ALLOC_ON;
  JERRY_CONTEXT (ecma_prop_hashmap_alloc_last_is_hs_gc) = false;
#endif /* !CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE */

} /* ecma_init */

/**
 * Finalize ECMA components
 */
void
ecma_finalize (void)
{
  jmem_unregister_free_unused_memory_callback (ecma_free_unused_memory);

  ecma_finalize_global_lex_env ();
  ecma_finalize_builtins ();
  ecma_gc_run (JMEM_FREE_UNUSED_MEMORY_SEVERITY_LOW);
  ecma_finalize_lit_storage ();
} /* ecma_finalize */

/**
 * @}
 * @}
 */
