/* Copyright 2016 Intel Corporation
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
/* infra */
#include "infra/log.h"
#include "infra/bsp.h"
#include "infra/xloop.h"
#include "cfw/cfw.h"

static xloop_t loop;

void main (void)
{
  T_QUEUE queue = bsp_init ();

  pr_info (LOG_MODULE_MAIN, "BSP init done");

  cfw_init (queue);
  pr_info (LOG_MODULE_MAIN, "CFW init done");

  xloop_init_from_queue (&loop, queue);

  xloop_run (&loop);
}
