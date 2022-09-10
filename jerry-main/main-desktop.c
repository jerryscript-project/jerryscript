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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jerryscript-port.h"
#include "jerryscript.h"

#include "arguments/options.h"
#include "jerryscript-ext/debugger.h"
#include "jerryscript-ext/handlers.h"
#include "jerryscript-ext/print.h"
#include "jerryscript-ext/properties.h"
#include "jerryscript-ext/repl.h"
#include "jerryscript-ext/sources.h"
#include "jerryscript-ext/test262.h"

/**
 * Initialize random seed
 */
static void
main_init_random_seed (void)
{
  union
  {
    double d;
    unsigned u;
  } now = { .d = jerry_port_current_time () };

  srand (now.u);
} /* main_init_random_seed */

/**
 * Initialize debugger
 */
static bool
main_init_debugger (main_args_t *arguments_p) /**< main arguments */
{
  bool result = false;

  if (!strcmp (arguments_p->debug_protocol, "tcp"))
  {
    result = jerryx_debugger_tcp_create (arguments_p->debug_port);
  }
  else
  {
    assert (!strcmp (arguments_p->debug_protocol, "serial"));
    result = jerryx_debugger_serial_create (arguments_p->debug_serial_config);
  }

  if (!strcmp (arguments_p->debug_channel, "rawpacket"))
  {
    result = result && jerryx_debugger_rp_create ();
  }
  else
  {
    assert (!strcmp (arguments_p->debug_channel, "websocket"));
    result = result && jerryx_debugger_ws_create ();
  }

  jerryx_debugger_after_connect (result);
  return result;
} /* main_init_debugger */

/**
 * Inits the engine and the debugger
 */
static void
main_init_engine (main_args_t *arguments_p) /**< main arguments */
{
  jerry_init (arguments_p->init_flags);

  jerry_promise_on_event (JERRY_PROMISE_EVENT_FILTER_ERROR, jerryx_handler_promise_reject, NULL);

  if (arguments_p->option_flags & OPT_FLAG_DEBUG_SERVER)
  {
    if (!main_init_debugger (arguments_p))
    {
      jerry_log (JERRY_LOG_LEVEL_WARNING, "Failed to initialize debugger!\n");
    }
  }

  if (arguments_p->option_flags & OPT_FLAG_TEST262_OBJECT)
  {
    jerryx_test262_register ();
  }

  jerryx_register_global ("assert", jerryx_handler_assert);
  jerryx_register_global ("gc", jerryx_handler_gc);
  jerryx_register_global ("print", jerryx_handler_print);
  jerryx_register_global ("sourceName", jerryx_handler_source_name);
  jerryx_register_global ("createRealm", jerryx_handler_create_realm);
} /* main_init_engine */

int
main (int argc, char **argv)
{
  main_init_random_seed ();
  JERRY_VLA (main_source_t, sources_p, argc);

  main_args_t arguments;
  arguments.sources_p = sources_p;

  if (!main_parse_args (argc, argv, &arguments))
  {
    return arguments.parse_result;
  }

restart:
  main_init_engine (&arguments);
  int return_code = JERRY_STANDALONE_EXIT_CODE_FAIL;
  jerry_value_t result;

  for (uint32_t source_index = 0; source_index < arguments.source_count; source_index++)
  {
    main_source_t *source_file_p = sources_p + source_index;
    const char *file_path_p = argv[source_file_p->path_index];

    switch (source_file_p->type)
    {
      case SOURCE_MODULE:
      {
        result = jerryx_source_exec_module (file_path_p);
        break;
      }
      case SOURCE_SNAPSHOT:
      {
        result = jerryx_source_exec_snapshot (file_path_p, source_file_p->snapshot_index);
        break;
      }
      default:
      {
        assert (source_file_p->type == SOURCE_SCRIPT);

        if ((arguments.option_flags & OPT_FLAG_PARSE_ONLY) != 0)
        {
          result = jerryx_source_parse_script (file_path_p);
        }
        else
        {
          result = jerryx_source_exec_script (file_path_p);
        }

        break;
      }
    }

    if (jerry_value_is_exception (result))
    {
      if (jerryx_debugger_is_reset (result))
      {
        jerry_cleanup ();

        goto restart;
      }

      jerryx_print_unhandled_exception (result);
      goto exit;
    }

    jerry_value_free (result);
  }

  if (arguments.option_flags & OPT_FLAG_WAIT_SOURCE)
  {
    while (true)
    {
      jerry_debugger_wait_for_source_status_t receive_status;
      receive_status = jerry_debugger_wait_for_client_source (jerryx_handler_source_received, NULL, &result);

      if (receive_status == JERRY_DEBUGGER_SOURCE_RECEIVE_FAILED)
      {
        jerry_log (JERRY_LOG_LEVEL_ERROR, "Connection aborted before source arrived.");
        goto exit;
      }

      if (receive_status == JERRY_DEBUGGER_SOURCE_END)
      {
        jerry_log (JERRY_LOG_LEVEL_DEBUG, "No more client source.\n");
        break;
      }

      assert (receive_status == JERRY_DEBUGGER_CONTEXT_RESET_RECEIVED
              || receive_status == JERRY_DEBUGGER_SOURCE_RECEIVED);

      if (receive_status == JERRY_DEBUGGER_CONTEXT_RESET_RECEIVED || jerryx_debugger_is_reset (result))
      {
        jerry_cleanup ();
        goto restart;
      }

      assert (receive_status == JERRY_DEBUGGER_SOURCE_RECEIVED);
      jerry_value_free (result);
    }
  }
  else if (arguments.option_flags & OPT_FLAG_USE_STDIN)
  {
    result = jerryx_source_exec_stdin ();

    if (jerry_value_is_exception (result))
    {
      jerryx_print_unhandled_exception (result);
      goto exit;
    }

    jerry_value_free (result);
  }
  else if (arguments.source_count == 0)
  {
    if ((arguments.option_flags & OPT_FLAG_NO_PROMPT))
    {
      jerryx_repl (JERRY_ZSTR_ARG (""));
    }
    else
    {
      jerryx_repl (JERRY_ZSTR_ARG ("jerry> "));
    }
  }

  result = jerry_run_jobs ();

  if (jerry_value_is_exception (result))
  {
    jerryx_print_unhandled_exception (result);
    goto exit;
  }

  jerry_value_free (result);

  if (arguments.exit_cb_name_p != NULL)
  {
    jerry_value_t global = jerry_current_realm ();
    jerry_value_t callback_fn = jerry_object_get_sz (global, arguments.exit_cb_name_p);
    jerry_value_free (global);

    if (jerry_value_is_function (callback_fn))
    {
      result = jerry_call (callback_fn, jerry_undefined (), NULL, 0);

      if (jerry_value_is_exception (result))
      {
        jerryx_print_unhandled_exception (result);
        goto exit;
      }

      jerry_value_free (result);
    }

    jerry_value_free (callback_fn);
  }

  return_code = JERRY_STANDALONE_EXIT_CODE_OK;

exit:
  jerry_cleanup ();

  return return_code;
} /* main */
