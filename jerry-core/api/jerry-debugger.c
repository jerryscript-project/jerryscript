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
#if ENABLED (JERRY_DEBUGGER)
  return JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED;
#else /* !ENABLED (JERRY_DEBUGGER) */
  return false;
#endif /* ENABLED (JERRY_DEBUGGER) */
} /* jerry_debugger_is_connected */

/**
 * Stop execution at the next available breakpoint.
 */
void
jerry_debugger_stop (void)
{
#if ENABLED (JERRY_DEBUGGER)
  if ((JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED)
      && !(JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_BREAKPOINT_MODE))
  {
    JERRY_DEBUGGER_SET_FLAGS (JERRY_DEBUGGER_VM_STOP);
    JERRY_CONTEXT (debugger_stop_context) = NULL;
  }
#endif /* ENABLED (JERRY_DEBUGGER) */
} /* jerry_debugger_stop */

/**
 * Continue execution.
 */
void
jerry_debugger_continue (void)
{
#if ENABLED (JERRY_DEBUGGER)
  if ((JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED)
      && !(JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_BREAKPOINT_MODE))
  {
    JERRY_DEBUGGER_CLEAR_FLAGS (JERRY_DEBUGGER_VM_STOP);
    JERRY_CONTEXT (debugger_stop_context) = NULL;
  }
#endif /* ENABLED (JERRY_DEBUGGER) */
} /* jerry_debugger_continue */

/**
 * Sets whether the engine should stop at breakpoints.
 */
void
jerry_debugger_stop_at_breakpoint (bool enable_stop_at_breakpoint) /**< enable/disable stop at breakpoint */
{
#if ENABLED (JERRY_DEBUGGER)
  if (JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED
      && !(JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_BREAKPOINT_MODE))
  {
    if (enable_stop_at_breakpoint)
    {
      JERRY_DEBUGGER_SET_FLAGS (JERRY_DEBUGGER_VM_IGNORE);
    }
    else
    {
      JERRY_DEBUGGER_CLEAR_FLAGS (JERRY_DEBUGGER_VM_IGNORE);
    }
  }
#else /* !ENABLED (JERRY_DEBUGGER) */
  JERRY_UNUSED (enable_stop_at_breakpoint);
#endif /* ENABLED (JERRY_DEBUGGER) */
} /* jerry_debugger_stop_at_breakpoint */

/**
 * Sets whether the engine should wait and run a source.
 *
 * @return enum JERRY_DEBUGGER_SOURCE_RECEIVE_FAILED - if the source is not received
 *              JERRY_DEBUGGER_SOURCE_RECEIVED - if a source code received
 *              JERRY_DEBUGGER_SOURCE_END - the end of the source codes
 *              JERRY_DEBUGGER_CONTEXT_RESET_RECEIVED - the end of the context
 */
jerry_debugger_wait_for_source_status_t
jerry_debugger_wait_for_client_source (jerry_debugger_wait_for_source_callback_t callback_p, /**< callback function */
                                       void *user_p, /**< user pointer passed to the callback */
                                       jerry_value_t *return_value) /**< [out] parse and run return value */
{
  *return_value = jerry_create_undefined ();

#if ENABLED (JERRY_DEBUGGER)
  if ((JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED)
      && !(JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_BREAKPOINT_MODE))
  {
    JERRY_DEBUGGER_SET_FLAGS (JERRY_DEBUGGER_CLIENT_SOURCE_MODE);
    jerry_debugger_uint8_data_t *client_source_data_p = NULL;
    jerry_debugger_wait_for_source_status_t ret_type = JERRY_DEBUGGER_SOURCE_RECEIVE_FAILED;

    /* Notify the client about that the engine is waiting for a source. */
    jerry_debugger_send_type (JERRY_DEBUGGER_WAIT_FOR_SOURCE);

    while (true)
    {
      if (jerry_debugger_receive (&client_source_data_p))
      {
        if (!(JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED))
        {
          break;
        }

        /* Stop executing the current context. */
        if ((JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONTEXT_RESET_MODE))
        {
          ret_type = JERRY_DEBUGGER_CONTEXT_RESET_RECEIVED;
          JERRY_DEBUGGER_CLEAR_FLAGS (JERRY_DEBUGGER_CONTEXT_RESET_MODE);
          break;
        }

        /* Stop waiting for a new source file. */
        if ((JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CLIENT_NO_SOURCE))
        {
          ret_type = JERRY_DEBUGGER_SOURCE_END;
          JERRY_DEBUGGER_CLEAR_FLAGS (JERRY_DEBUGGER_CLIENT_SOURCE_MODE);
          break;
        }

        /* The source arrived. */
        if (!(JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CLIENT_SOURCE_MODE))
        {
          JERRY_ASSERT (client_source_data_p != NULL);

          jerry_char_t *resource_name_p = (jerry_char_t *) (client_source_data_p + 1);
          size_t resource_name_size = strlen ((const char *) resource_name_p);

          *return_value = callback_p (resource_name_p,
                                      resource_name_size,
                                      resource_name_p + resource_name_size + 1,
                                      client_source_data_p->uint8_size - resource_name_size - 1,
                                      user_p);

          ret_type = JERRY_DEBUGGER_SOURCE_RECEIVED;
          break;
        }
      }

      jerry_debugger_transport_sleep ();
    }

    JERRY_ASSERT (!(JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CLIENT_SOURCE_MODE)
                  || !(JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED));

    if (client_source_data_p != NULL)
    {
      /* The data may partly arrived. */
      jmem_heap_free_block (client_source_data_p,
                            client_source_data_p->uint8_size + sizeof (jerry_debugger_uint8_data_t));
    }

    return ret_type;
  }

  return JERRY_DEBUGGER_SOURCE_RECEIVE_FAILED;
#else /* !ENABLED (JERRY_DEBUGGER) */
  JERRY_UNUSED (callback_p);
  JERRY_UNUSED (user_p);

  return JERRY_DEBUGGER_SOURCE_RECEIVE_FAILED;
#endif /* ENABLED (JERRY_DEBUGGER) */
} /* jerry_debugger_wait_for_client_source */

/**
 * Send the output of the program to the debugger client.
 * Currently only sends print output.
 */
void
jerry_debugger_send_output (const jerry_char_t *buffer, /**< buffer */
                            jerry_size_t str_size) /**< string size */
{
#if ENABLED (JERRY_DEBUGGER)
  if (JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED)
  {
    jerry_debugger_send_string (JERRY_DEBUGGER_OUTPUT_RESULT,
                                JERRY_DEBUGGER_OUTPUT_OK,
                                (const uint8_t *) buffer,
                                sizeof (uint8_t) * str_size);
  }
#else /* !ENABLED (JERRY_DEBUGGER) */
  JERRY_UNUSED (buffer);
  JERRY_UNUSED (str_size);
#endif /* ENABLED (JERRY_DEBUGGER) */
} /* jerry_debugger_send_output */

/**
 * Send the log of the program to the debugger client.
 */
void
jerry_debugger_send_log (jerry_log_level_t level, /**< level of the diagnostics message */
                         const jerry_char_t *buffer, /**< buffer */
                         jerry_size_t str_size) /**< string size */
{
#if ENABLED (JERRY_DEBUGGER)
  if (JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED)
  {
    jerry_debugger_send_string (JERRY_DEBUGGER_OUTPUT_RESULT,
                                (uint8_t) (level + 2),
                                (const uint8_t *) buffer,
                                sizeof (uint8_t) * str_size);
  }
#else /* !ENABLED (JERRY_DEBUGGER) */
  JERRY_UNUSED (level);
  JERRY_UNUSED (buffer);
  JERRY_UNUSED (str_size);
#endif /* ENABLED (JERRY_DEBUGGER) */
} /* jerry_debugger_send_log */
