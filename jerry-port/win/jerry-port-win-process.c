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

#if defined(_WIN32)

#include <windows.h>

/**
 * Default implementation of jerry_port_sleep, uses 'Sleep'.
 */
void
jerry_port_sleep (uint32_t sleep_time) /**< milliseconds to sleep */
{
  Sleep (sleep_time);
} /* jerry_port_sleep */

#endif /* defined(_WIN32) */
