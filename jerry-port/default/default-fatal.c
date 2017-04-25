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

#include <stdlib.h>

#include "jerryscript-port.h"
#include "jerryscript-port-default.h"

#ifndef DISABLE_EXTRA_API

static bool abort_on_fail = false;

/**
 * Sets whether 'abort' should be called instead of 'exit' upon exiting with
 * non-zero exit code in the default implementation of jerry_port_fatal.
 *
 * Note:
 *      This function is only available if the port implementation library is
 *      compiled without the DISABLE_EXTRA_API macro.
 */
void jerry_port_default_set_abort_on_fail (bool flag) /**< new value of 'abort on fail' flag */
{
  abort_on_fail = flag;
} /* jerry_port_default_set_abort_on_fail */

/**
 * Check whether 'abort' should be called instead of 'exit' upon exiting with
 * non-zero exit code in the default implementation of jerry_port_fatal.
 *
 * @return true - if 'abort on fail' flag is set,
 *         false - otherwise
 *
 * Note:
 *      This function is only available if the port implementation library is
 *      compiled without the DISABLE_EXTRA_API macro.
 */
bool jerry_port_default_is_abort_on_fail (void)
{
  return abort_on_fail;
} /* jerry_port_default_is_abort_on_fail */

#endif /* !DISABLE_EXTRA_API */

/**
 * Default implementation of jerry_port_fatal. Calls 'abort' if exit code is
 * non-zero and "abort-on-fail" behaviour is enabled, 'exit' otherwise.
 *
 * Note:
 *      The "abort-on-fail" behaviour is only available if the port
 *      implementation library is compiled without the DISABLE_EXTRA_API macro.
 */
void jerry_port_fatal (jerry_fatal_code_t code)
{
#ifndef DISABLE_EXTRA_API
  if (code != 0
      && code != ERR_OUT_OF_MEMORY
      && jerry_port_default_is_abort_on_fail ())
  {
    abort ();
  }
  else
  {
#endif /* !DISABLE_EXTRA_API */
    exit (code);
#ifndef DISABLE_EXTRA_API
  }
#endif /* !DISABLE_EXTRA_API */
} /* jerry_port_fatal */
