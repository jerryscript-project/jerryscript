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

#include "los_sys.h"
#include "los_tick.h"
#include "los_task.ph"
#include "los_config.h"
#include "jerry_run.h"


static UINT32 g_taskid;

/**
 * The task function
 */
static void
example_taskfunc (void)
{
  while (1)
  {
    run ();
  }
}/* example_taskfunc */

/**
 * The example entry
 */
static void
example_entry (void)
{
  UINT32 uwRet;
  TSK_INIT_PARAM_S stTaskInitParam =
  {
    .pfnTaskEntry = (TSK_ENTRY_FUNC)example_taskfunc,
    .uwStackSize = LOSCFG_BASE_CORE_TSK_IDLE_STACK_SIZE,
    .pcName = "BoardDemo",
    .usTaskPrio = 10
  };

  uwRet = LOS_TaskCreate (&g_taskid, &stTaskInitParam);
  if (uwRet != LOS_OK)
  {
    return;
  }
  return;
}/* example_entry */


/**
 * Main program.
 */
int 
main (void)
{
  UINT32 uwRet;

  LOS_EvbSetup ();
  uwRet = LOS_KernelInit ();
  if (uwRet != LOS_OK) 
  {
    return LOS_NOK;
  }
  LOS_EnableTick ();
  example_entry ();
		
  LOS_Start ();
  for (;;);
}/* main */
