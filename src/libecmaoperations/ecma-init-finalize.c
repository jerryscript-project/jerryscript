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
#include "ecma-helpers.h"
#include "ecma-operations.h"

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
  ecma_strings_init ();
  ecma_init_builtins ();
} /* ecma_init */

/**
 * Finalize ECMA components
 */
void
ecma_finalize (void)
{
  ecma_finalize_builtins ();
} /* ecma_finalize */

/**
 * @}
 * @}
 */
