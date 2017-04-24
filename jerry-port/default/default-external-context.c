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

#include "jerryscript-port.h"
#include "jerryscript-port-default.h"

/**
 * Pointer to the current instance.
 * Note that it is a global variable, and is not a thread safe implementation.
 */
static jerry_instance_t *current_instance_p = NULL;

/**
 * Set the current_instance_p as the passed pointer.
 */
void
jerry_port_default_set_instance (jerry_instance_t *instance_p) /**< points to the created instance */
{
  current_instance_p = instance_p;
} /* jerry_port_default_set_instance */

/**
 * Get the current instance.
 *
 * @return the pointer to the current instance
 */
jerry_instance_t *
jerry_port_get_current_instance (void)
{
  return current_instance_p;
} /* jerry_port_get_current_instance */
