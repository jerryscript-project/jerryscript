/* Copyright 2015 Samsung Electronics Co., Ltd.
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

#include "jerry.h"

#include "common-io.h"
#include "actuators.h"
#include "sensors.h"

#include JERRY_MCU_SCRIPT_HEADER
static const char generated_source [] = JERRY_MCU_SCRIPT;

static uint32_t start __unused;
static uint32_t finish_native_ms __unused;
static uint32_t finish_parse_ms __unused;
static uint32_t finish_int_ms __unused;

int
main (void)
{
  initialize_sys_tick ();
  initialize_leds ();
  initialize_timer ();

  const char *source_p = generated_source;
  const size_t source_size = sizeof (generated_source);

  set_sys_tick_counter ((uint32_t) - 1);
  start = get_sys_tick_counter ();

  jerry_completion_code_t ret_code = jerry_run_simple (source_p, source_size, JERRY_FLAG_EMPTY);

  finish_parse_ms = (start - get_sys_tick_counter ()) / 1000;

  if (ret_code == JERRY_COMPLETION_CODE_OK)
  {
    return JERRY_STANDALONE_EXIT_CODE_OK;
  }
  else
  {
    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }
}
