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

#include "debugger.h"
#include "jcontext.h"
#include "jerryscript.h"

/**
 * Checks whether the debugger is connected.
 *
 * @return true - if the debugger is connected
 *         false - otherwise
 */
bool
jerry_debugger_is_connected (void)
{
#ifdef JERRY_DEBUGGER
  return JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED;
#else
  return false;
#endif /* JERRY_DEBUGGER */
} /* jerry_debugger_is_connected */

/**
 * Stop execution at the next available breakpoint.
 */
void
jerry_debugger_stop (void)
{
#ifdef JERRY_DEBUGGER
  if ((JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED)
      && !(JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_BREAKPOINT_MODE))
  {
    JERRY_CONTEXT (debugger_flags) = (uint8_t) (JERRY_CONTEXT (debugger_flags) | JERRY_DEBUGGER_VM_STOP);
    JERRY_CONTEXT (debugger_stop_context) = NULL;
  }
#endif /* JERRY_DEBUGGER */
} /* jerry_debugger_stop */

/**
 * Continue execution.
 */
void
jerry_debugger_continue (void)
{
#ifdef JERRY_DEBUGGER
  if ((JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED)
      && !(JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_BREAKPOINT_MODE))
  {
    JERRY_CONTEXT (debugger_flags) = (uint8_t) (JERRY_CONTEXT (debugger_flags) & ~JERRY_DEBUGGER_VM_STOP);
    JERRY_CONTEXT (debugger_stop_context) = NULL;
  }
#endif /* JERRY_DEBUGGER */
} /* jerry_debugger_continue */

/**
 * Sets whether the engine should stop at breakpoints.
 */
void
jerry_debugger_stop_at_breakpoint (bool enable_stop_at_breakpoint) /**< enable/disable stop at breakpoint */
{
#ifdef JERRY_DEBUGGER
  if (JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED
      && !(JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_BREAKPOINT_MODE))
  {
    if (enable_stop_at_breakpoint)
    {
      JERRY_CONTEXT (debugger_flags) = (uint8_t) (JERRY_CONTEXT (debugger_flags) | JERRY_DEBUGGER_VM_IGNORE);
    }
    else
    {
      JERRY_CONTEXT (debugger_flags) = (uint8_t) (JERRY_CONTEXT (debugger_flags) & ~JERRY_DEBUGGER_VM_IGNORE);
    }
  }
#else /* !JERRY_DEBUGGER */
  JERRY_UNUSED (enable_stop_at_breakpoint);
#endif /* JERRY_DEBUGGER */
} /* jerry_debugger_stop_at_breakpoint */

/**
 * Debugger server initialization. Must be called after jerry_init.
 */
void
jerry_debugger_init (uint16_t port) /**< server port number */
{
#ifdef JERRY_DEBUGGER
  JERRY_CONTEXT (debugger_port) = port;
  jerry_debugger_accept_connection ();
#else /* !JERRY_DEBUGGER */
  JERRY_UNUSED (port);
#endif /* JERRY_DEBUGGER */
} /* jerry_debugger_init */

/**
 * Debugger server shutdown. Must be called before jerry_cleanup.
 */
void
jerry_debugger_cleanup (void)
{
#ifdef JERRY_DEBUGGER
  if (JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED)
  {
    jerry_debugger_close_connection ();
  }
#endif /* JERRY_DEBUGGER */
} /* jerry_debugger_cleanup */
