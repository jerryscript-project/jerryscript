/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
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
#include "jmem-allocator.h"

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
  ecma_gc_init ();
  ecma_init_builtins ();
  ecma_lcache_init ();
  ecma_init_environment ();

  jmem_register_a_try_give_memory_back_callback (ecma_try_to_give_back_some_memory);
} /* ecma_init */

/**
 * Finalize ECMA components
 */
void
ecma_finalize (void)
{
  jmem_unregister_a_try_give_memory_back_callback (ecma_try_to_give_back_some_memory);

  ecma_finalize_environment ();
  ecma_finalize_builtins ();
  ecma_gc_run ();
} /* ecma_finalize */

/**
 * @}
 * @}
 */
