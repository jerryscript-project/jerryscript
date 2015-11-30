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
#include "main.h"

/**
 * Standalone Jerry exit codes
 */
#define JERRY_STANDALONE_EXIT_CODE_OK   (0)
#define JERRY_STANDALONE_EXIT_CODE_FAIL (1)

#include JERRY_MCU_SCRIPT_HEADER
#include "jerry-core/jerry-api.h"

static const char generated_source[] = JERRY_MCU_SCRIPT;

int
main (void)
{
  const char *source_p = generated_source;
  const size_t source_size = sizeof (generated_source);

  jerry_completion_code_t ret_code = jerry_run_simple ((jerry_api_char_t *) source_p, source_size, JERRY_FLAG_EMPTY);

  if (ret_code == JERRY_COMPLETION_CODE_OK)
  {
    return JERRY_STANDALONE_EXIT_CODE_OK;
  }
  else
  {
    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }
}
