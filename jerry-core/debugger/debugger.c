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

#include "byte-code.h"
#include "debugger.h"
#include "ecma-builtin-helpers.h"
#include "ecma-conversion.h"
#include "ecma-eval.h"
#include "ecma-objects.h"
#include "jcontext.h"
#include "lit-char-helpers.h"

#ifdef JERRY_DEBUGGER

#ifdef HAVE_TIME_H
#include <time.h>
#elif defined (HAVE_UNISTD_H)
#include <unistd.h>
#endif /* HAVE_TIME_H */

/**
 * Type cast the debugger send buffer into a specific type.
 */
#define JERRY_DEBUGGER_SEND_BUFFER_AS(type, name_p) \
  type *name_p = (type *) (&JERRY_CONTEXT (debugger_send_buffer))

/**
 * Type cast the debugger receive buffer into a specific type.
 */
#define JERRY_DEBUGGER_RECEIVE_BUFFER_AS(type, name_p) \
  type *name_p = ((type *) recv_buffer_p)

/**
 * Free all unreferenced byte code structures which
 * were not acknowledged by the debugger client.
 */
void
jerry_debugger_free_unreferenced_byte_code (void)
{
  jerry_debugger_byte_code_free_t *byte_code_free_p;

  byte_code_free_p = JMEM_CP_GET_POINTER (jerry_debugger_byte_code_free_t,
                                          JERRY_CONTEXT (debugger_byte_code_free_tail));

  while (byte_code_free_p != NULL)
  {
    jerry_debugger_byte_code_free_t *prev_byte_code_free_p;
    prev_byte_code_free_p = JMEM_CP_GET_POINTER (jerry_debugger_byte_code_free_t,
                                                 byte_code_free_p->prev_cp);

    jmem_heap_free_block (byte_code_free_p,
                          ((size_t) byte_code_free_p->size) << JMEM_ALIGNMENT_LOG);

    byte_code_free_p = prev_byte_code_free_p;
  }
} /* jerry_debugger_free_unreferenced_byte_code */

/**
 * Send backtrace.
 */
static void
jerry_debugger_send_backtrace (uint8_t *recv_buffer_p) /**< pointer the the received data */
{
  JERRY_DEBUGGER_RECEIVE_BUFFER_AS (jerry_debugger_receive_get_backtrace_t, get_backtrace_p);

  uint32_t max_depth;
  memcpy (&max_depth, get_backtrace_p->max_depth, sizeof (uint32_t));

  if (max_depth == 0)
  {
    max_depth = UINT32_MAX;
  }

  JERRY_DEBUGGER_SEND_BUFFER_AS (jerry_debugger_send_backtrace_t, backtrace_p);

  JERRY_DEBUGGER_INIT_SEND_MESSAGE (backtrace_p);
  JERRY_DEBUGGER_SET_SEND_MESSAGE_SIZE_FROM_TYPE (backtrace_p, jerry_debugger_send_backtrace_t);
  backtrace_p->type = JERRY_DEBUGGER_BACKTRACE;

  vm_frame_ctx_t *frame_ctx_p = JERRY_CONTEXT (vm_top_context_p);

  uint32_t current_frame = 0;

  while (frame_ctx_p != NULL && max_depth > 0)
  {
    if (frame_ctx_p->bytecode_header_p->status_flags & CBC_CODE_FLAGS_DEBUGGER_IGNORE)
    {
      frame_ctx_p = frame_ctx_p->prev_context_p;
      continue;
    }

    if (current_frame >= JERRY_DEBUGGER_SEND_MAX (jerry_debugger_frame_t))
    {
      if (!jerry_debugger_send (sizeof (jerry_debugger_send_backtrace_t)))
      {
        return;
      }
      current_frame = 0;
    }

    jerry_debugger_frame_t *frame_p = backtrace_p->frames + current_frame;

    jmem_cpointer_t byte_code_cp;
    JMEM_CP_SET_NON_NULL_POINTER (byte_code_cp, frame_ctx_p->bytecode_header_p);
    memcpy (frame_p->byte_code_cp, &byte_code_cp, sizeof (jmem_cpointer_t));

    uint32_t offset = (uint32_t) (frame_ctx_p->byte_code_p - (uint8_t *) frame_ctx_p->bytecode_header_p);
    memcpy (frame_p->offset, &offset, sizeof (uint32_t));

    frame_ctx_p = frame_ctx_p->prev_context_p;
    current_frame++;
    max_depth--;
  }

  size_t message_size = current_frame * sizeof (jerry_debugger_frame_t);

  JERRY_DEBUGGER_SET_SEND_MESSAGE_SIZE (backtrace_p, 1 + message_size);
  backtrace_p->type = JERRY_DEBUGGER_BACKTRACE_END;

  jerry_debugger_send (sizeof (jerry_debugger_send_type_t) + message_size);
} /* jerry_debugger_send_backtrace */

/**
 * Send result of evaluated expression.
 *
 * @return true - if no error is occured
 *         false - otherwise
 */
static bool
jerry_debugger_send_eval (const lit_utf8_byte_t *eval_string_p, /**< evaluated string */
                          size_t eval_string_size) /**< evaluated string size */
{
  JERRY_ASSERT (JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED);
  JERRY_ASSERT (!(JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_VM_IGNORE));

  JERRY_CONTEXT (debugger_flags) = (uint8_t) (JERRY_CONTEXT (debugger_flags) | JERRY_DEBUGGER_VM_IGNORE);
  ecma_value_t result = ecma_op_eval_chars_buffer (eval_string_p, eval_string_size, true, false);
  JERRY_CONTEXT (debugger_flags) = (uint8_t) (JERRY_CONTEXT (debugger_flags) & ~JERRY_DEBUGGER_VM_IGNORE);

  if (!ECMA_IS_VALUE_ERROR (result))
  {
    ecma_value_t to_string_value = ecma_op_to_string (result);
    ecma_free_value (result);
    result = to_string_value;
  }

  ecma_value_t message = result;
  uint8_t type = JERRY_DEBUGGER_EVAL_OK;

  if (ECMA_IS_VALUE_ERROR (result))
  {
    type = JERRY_DEBUGGER_EVAL_ERROR;
    result = JERRY_CONTEXT (error_value);

    if (ecma_is_value_object (result))
    {
      ecma_string_t *message_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_MESSAGE);

      message = ecma_op_object_find (ecma_get_object_from_value (result),
                                     message_string_p);

      ecma_deref_ecma_string (message_string_p);

      if (!ecma_is_value_string (message)
          || ecma_string_is_empty (ecma_get_string_from_value (message)))
      {
        ecma_free_value (message);
        lit_magic_string_id_t id = ecma_object_get_class_name (ecma_get_object_from_value (result));
        ecma_free_value (result);

        const lit_utf8_byte_t *string_p = lit_get_magic_string_utf8 (id);
        return jerry_debugger_send_string (JERRY_DEBUGGER_EVAL_RESULT,
                                           type,
                                           string_p,
                                           strlen ((const char *) string_p));
      }
    }
    else
    {
      /* Primitve type. */
      message = ecma_op_to_string (result);
      JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (message));
    }

    ecma_free_value (result);
  }

  ecma_string_t *string_p = ecma_get_string_from_value (message);

  ECMA_STRING_TO_UTF8_STRING (string_p, buffer_p, buffer_size);
  bool success = jerry_debugger_send_string (JERRY_DEBUGGER_EVAL_RESULT, type, buffer_p, buffer_size);
  ECMA_FINALIZE_UTF8_STRING (buffer_p, buffer_size);

  ecma_free_value (message);

  return success;
} /* jerry_debugger_send_eval */

/**
 * Suspend execution for a given time.
 * Note: If the platform does not have nanosleep or usleep, this function does not sleep at all.
 */
void
jerry_debugger_sleep (void)
{
#ifdef HAVE_TIME_H
  nanosleep (&(const struct timespec)
  {
    JERRY_DEBUGGER_TIMEOUT / 1000, (JERRY_DEBUGGER_TIMEOUT % 1000) * 1000000L /* Seconds, nanoseconds */
  }
  , NULL);
#elif defined (HAVE_UNISTD_H)
  usleep ((useconds_t) JERRY_DEBUGGER_TIMEOUT * 1000);
#endif /* HAVE_TIME_H */
} /* jerry_debugger_sleep */

/**
 * Check received packet size.
 */
#define JERRY_DEBUGGER_CHECK_PACKET_SIZE(type) \
  if (message_size != sizeof (type)) \
  { \
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Invalid message size\n"); \
    jerry_debugger_close_connection (); \
    return false; \
  }

/**
 * Receive message from the client.
 *
 * @return true - if message is processed successfully
 *         false - otherwise
 */
inline bool __attr_always_inline___
jerry_debugger_process_message (uint8_t *recv_buffer_p, /**< pointer the the received data */
                                uint32_t message_size, /**< message size */
                                bool *resume_exec_p, /**< pointer to the resume exec flag */
                                uint8_t *expected_message_type_p, /**< message type */
                                jerry_debugger_uint8_data_t **message_data_p) /**< custom message data */
{
  /* Process the received message. */

  if (recv_buffer_p[0] >= JERRY_DEBUGGER_CONTINUE
      && !(JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_BREAKPOINT_MODE))
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Message requires breakpoint mode\n");
    jerry_debugger_close_connection ();
    return false;
  }

  if (*expected_message_type_p != 0)
  {
    JERRY_ASSERT (*expected_message_type_p == JERRY_DEBUGGER_EVAL_PART
                  || *expected_message_type_p == JERRY_DEBUGGER_CLIENT_SOURCE_PART);

    jerry_debugger_uint8_data_t *uint8_data_p = (jerry_debugger_uint8_data_t *) *message_data_p;

    if (recv_buffer_p[0] != *expected_message_type_p)
    {
      jmem_heap_free_block (uint8_data_p, uint8_data_p->uint8_size + sizeof (jerry_debugger_uint8_data_t));
      jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Unexpected message\n");
      jerry_debugger_close_connection ();
      return false;
    }

    JERRY_DEBUGGER_RECEIVE_BUFFER_AS (jerry_debugger_receive_uint8_data_part_t, uint8_data_part_p);

    if (message_size < sizeof (jerry_debugger_receive_uint8_data_part_t) + 1)
    {
      jmem_heap_free_block (uint8_data_p, uint8_data_p->uint8_size + sizeof (jerry_debugger_uint8_data_t));
      jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Invalid message size\n");
      jerry_debugger_close_connection ();
      return false;
    }

    uint32_t expected_data = uint8_data_p->uint8_size - uint8_data_p->uint8_offset;

    message_size -= (uint32_t) sizeof (jerry_debugger_receive_uint8_data_part_t);

    if (message_size > expected_data)
    {
      jmem_heap_free_block (uint8_data_p, uint8_data_p->uint8_size + sizeof (jerry_debugger_uint8_data_t));
      jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Invalid message size\n");
      jerry_debugger_close_connection ();
      return false;
    }

    lit_utf8_byte_t *string_p = (lit_utf8_byte_t *) (uint8_data_p + 1);
    memcpy (string_p + uint8_data_p->uint8_offset,
            (lit_utf8_byte_t *) (uint8_data_part_p + 1),
            message_size);

    if (message_size < expected_data)
    {
      uint8_data_p->uint8_offset += message_size;
      return true;
    }

    bool result = false;
    if (*expected_message_type_p == JERRY_DEBUGGER_EVAL_PART)
    {
      result = jerry_debugger_send_eval (string_p, uint8_data_p->uint8_size);
    }
    else
    {
      result = true;
      JERRY_CONTEXT (debugger_flags) = (uint8_t) (JERRY_CONTEXT (debugger_flags)
                                       & ~JERRY_DEBUGGER_CLIENT_SOURCE_MODE);
      *resume_exec_p = true;
    }

    *expected_message_type_p = 0;
    return result;
  }

  switch (recv_buffer_p[0])
  {
    case JERRY_DEBUGGER_FREE_BYTE_CODE_CP:
    {
      JERRY_DEBUGGER_CHECK_PACKET_SIZE (jerry_debugger_receive_byte_code_cp_t);

      JERRY_DEBUGGER_RECEIVE_BUFFER_AS (jerry_debugger_receive_byte_code_cp_t, byte_code_p);

      jmem_cpointer_t byte_code_free_cp;
      memcpy (&byte_code_free_cp, byte_code_p->byte_code_cp, sizeof (jmem_cpointer_t));

      if (byte_code_free_cp != JERRY_CONTEXT (debugger_byte_code_free_tail))
      {
        jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Invalid byte code free order\n");
        jerry_debugger_close_connection ();
        return false;
      }

      jerry_debugger_byte_code_free_t *byte_code_free_p;
      byte_code_free_p = JMEM_CP_GET_NON_NULL_POINTER (jerry_debugger_byte_code_free_t,
                                                       byte_code_free_cp);

      if (byte_code_free_p->prev_cp != ECMA_NULL_POINTER)
      {
        JERRY_CONTEXT (debugger_byte_code_free_tail) = byte_code_free_p->prev_cp;
      }
      else
      {
        JERRY_CONTEXT (debugger_byte_code_free_head) = ECMA_NULL_POINTER;
        JERRY_CONTEXT (debugger_byte_code_free_tail) = ECMA_NULL_POINTER;
      }

#ifdef JMEM_STATS
      jmem_stats_free_byte_code_bytes (((size_t) byte_code_free_p->size) << JMEM_ALIGNMENT_LOG);
#endif /* JMEM_STATS */

      jmem_heap_free_block (byte_code_free_p,
                            ((size_t) byte_code_free_p->size) << JMEM_ALIGNMENT_LOG);
      return true;
    }

    case JERRY_DEBUGGER_UPDATE_BREAKPOINT:
    {
      JERRY_DEBUGGER_CHECK_PACKET_SIZE (jerry_debugger_receive_update_breakpoint_t);

      JERRY_DEBUGGER_RECEIVE_BUFFER_AS (jerry_debugger_receive_update_breakpoint_t, update_breakpoint_p);

      jmem_cpointer_t byte_code_cp;
      memcpy (&byte_code_cp, update_breakpoint_p->byte_code_cp, sizeof (jmem_cpointer_t));
      uint8_t *byte_code_p = JMEM_CP_GET_NON_NULL_POINTER (uint8_t, byte_code_cp);

      uint32_t offset;
      memcpy (&offset, update_breakpoint_p->offset, sizeof (uint32_t));
      byte_code_p += offset;

      JERRY_ASSERT (*byte_code_p == CBC_BREAKPOINT_ENABLED || *byte_code_p == CBC_BREAKPOINT_DISABLED);

      *byte_code_p = update_breakpoint_p->is_set_breakpoint ? CBC_BREAKPOINT_ENABLED : CBC_BREAKPOINT_DISABLED;
      return true;
    }

    case JERRY_DEBUGGER_MEMSTATS:
    {
      JERRY_DEBUGGER_CHECK_PACKET_SIZE (jerry_debugger_receive_type_t);

      jerry_debugger_send_memstats ();
      return true;
    }

    case JERRY_DEBUGGER_STOP:
    {
      JERRY_DEBUGGER_CHECK_PACKET_SIZE (jerry_debugger_receive_type_t);

      JERRY_CONTEXT (debugger_flags) = (uint8_t) (JERRY_CONTEXT (debugger_flags) | JERRY_DEBUGGER_VM_STOP);
      JERRY_CONTEXT (debugger_stop_context) = NULL;
      *resume_exec_p = false;
      return true;
    }

    case JERRY_DEBUGGER_CONTINUE:
    {
      JERRY_DEBUGGER_CHECK_PACKET_SIZE (jerry_debugger_receive_type_t);

      JERRY_CONTEXT (debugger_flags) = (uint8_t) (JERRY_CONTEXT (debugger_flags) & ~JERRY_DEBUGGER_VM_STOP);
      JERRY_CONTEXT (debugger_stop_context) = NULL;
      *resume_exec_p = true;
      return true;
    }

    case JERRY_DEBUGGER_STEP:
    {
      JERRY_DEBUGGER_CHECK_PACKET_SIZE (jerry_debugger_receive_type_t);

      JERRY_CONTEXT (debugger_flags) = (uint8_t) (JERRY_CONTEXT (debugger_flags) | JERRY_DEBUGGER_VM_STOP);
      JERRY_CONTEXT (debugger_stop_context) = NULL;
      *resume_exec_p = true;
      return true;
    }

    case JERRY_DEBUGGER_NEXT:
    {
      JERRY_DEBUGGER_CHECK_PACKET_SIZE (jerry_debugger_receive_type_t);

      JERRY_CONTEXT (debugger_flags) = (uint8_t) (JERRY_CONTEXT (debugger_flags) | JERRY_DEBUGGER_VM_STOP);
      JERRY_CONTEXT (debugger_stop_context) = JERRY_CONTEXT (vm_top_context_p);
      *resume_exec_p = true;
      return true;
    }

    case JERRY_DEBUGGER_GET_BACKTRACE:
    {
      JERRY_DEBUGGER_CHECK_PACKET_SIZE (jerry_debugger_receive_get_backtrace_t);

      jerry_debugger_send_backtrace (recv_buffer_p);
      return true;
    }

    case JERRY_DEBUGGER_EXCEPTION_CONFIG:
    {
      JERRY_DEBUGGER_CHECK_PACKET_SIZE (jerry_debugger_receive_exception_config_t);
      JERRY_DEBUGGER_RECEIVE_BUFFER_AS (jerry_debugger_receive_exception_config_t, exception_config_p);

      uint8_t debugger_flags = JERRY_CONTEXT (debugger_flags);

      if (exception_config_p->enable == 0)
      {
        debugger_flags = (uint8_t) (debugger_flags | JERRY_DEBUGGER_VM_IGNORE_EXCEPTION);
        jerry_port_log (JERRY_LOG_LEVEL_DEBUG, "Stop at exception disabled\n");
      }
      else
      {
        debugger_flags = (uint8_t) (debugger_flags & ~JERRY_DEBUGGER_VM_IGNORE_EXCEPTION);
        jerry_port_log (JERRY_LOG_LEVEL_DEBUG, "Stop at exception enabled\n");
      }

      JERRY_CONTEXT (debugger_flags)  = debugger_flags;
      return true;
    }

    case JERRY_DEBUGGER_EVAL:
    {
      if (message_size < sizeof (jerry_debugger_receive_eval_first_t) + 1)
      {
        jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Invalid message size\n");
        jerry_debugger_close_connection ();
        return false;
      }

      JERRY_DEBUGGER_RECEIVE_BUFFER_AS (jerry_debugger_receive_eval_first_t, eval_first_p);

      uint32_t eval_size;
      memcpy (&eval_size, eval_first_p->eval_size, sizeof (uint32_t));

      if (eval_size <= JERRY_DEBUGGER_MAX_RECEIVE_SIZE - sizeof (jerry_debugger_receive_eval_first_t))
      {
        if (eval_size != message_size - sizeof (jerry_debugger_receive_eval_first_t))
        {
          jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Invalid message size\n");
          jerry_debugger_close_connection ();
          return false;
        }

        return jerry_debugger_send_eval ((lit_utf8_byte_t *) (eval_first_p + 1), eval_size);
      }

      jerry_debugger_uint8_data_t *eval_uint8_data_p;
      size_t eval_data_size = sizeof (jerry_debugger_uint8_data_t) + eval_size;

      eval_uint8_data_p = (jerry_debugger_uint8_data_t *) jmem_heap_alloc_block (eval_data_size);

      eval_uint8_data_p->uint8_size = eval_size;
      eval_uint8_data_p->uint8_offset = (uint32_t) (message_size - sizeof (jerry_debugger_receive_eval_first_t));

      lit_utf8_byte_t *eval_string_p = (lit_utf8_byte_t *) (eval_uint8_data_p + 1);
      memcpy (eval_string_p,
              (lit_utf8_byte_t *) (eval_first_p + 1),
              message_size - sizeof (jerry_debugger_receive_eval_first_t));

      *message_data_p = eval_uint8_data_p;
      *expected_message_type_p = JERRY_DEBUGGER_EVAL_PART;
      return true;
    }

    case JERRY_DEBUGGER_CLIENT_SOURCE:
    {
      if (message_size <= sizeof (jerry_debugger_receive_client_source_first_t))
      {
        jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Invalid message size\n");
        jerry_debugger_close_connection ();
        return false;
      }

      if (!(JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CLIENT_SOURCE_MODE))
      {
        jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Not in client source mode\n");
        jerry_debugger_close_connection ();
        return false;
      }

      JERRY_DEBUGGER_RECEIVE_BUFFER_AS (jerry_debugger_receive_client_source_first_t, client_source_first_p);

      uint32_t client_source_size;
      memcpy (&client_source_size, client_source_first_p->code_size, sizeof (uint32_t));

      if (client_source_size <= JERRY_DEBUGGER_MAX_RECEIVE_SIZE - sizeof (jerry_debugger_receive_client_source_first_t)
          && client_source_size != message_size - sizeof (jerry_debugger_receive_client_source_first_t))
      {
        jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Invalid message size\n");
        jerry_debugger_close_connection ();
        return false;
      }

      jerry_debugger_uint8_data_t *client_source_data_p;
      size_t client_source_data_size = sizeof (jerry_debugger_uint8_data_t) + client_source_size;

      client_source_data_p = (jerry_debugger_uint8_data_t *) jmem_heap_alloc_block (client_source_data_size);

      client_source_data_p->uint8_size = client_source_size;
      client_source_data_p->uint8_offset = (uint32_t) (message_size
                                            - sizeof (jerry_debugger_receive_client_source_first_t));

      lit_utf8_byte_t *client_source_string_p = (lit_utf8_byte_t *) (client_source_data_p + 1);
      memcpy (client_source_string_p,
              (lit_utf8_byte_t *) (client_source_first_p + 1),
              message_size - sizeof (jerry_debugger_receive_client_source_first_t));

      *message_data_p = client_source_data_p;

      if (client_source_data_p->uint8_size != client_source_data_p->uint8_offset)
      {
        *expected_message_type_p = JERRY_DEBUGGER_CLIENT_SOURCE_PART;
      }
      else
      {
        JERRY_CONTEXT (debugger_flags) = (uint8_t) (JERRY_CONTEXT (debugger_flags)
                                         & ~JERRY_DEBUGGER_CLIENT_SOURCE_MODE);
        *resume_exec_p = true;
      }
      return true;
    }

    case JERRY_DEBUGGER_NO_MORE_SOURCES:
    {
      if (!(JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CLIENT_SOURCE_MODE))
      {
        jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Not in client source mode\n");
        jerry_debugger_close_connection ();
        return false;
      }

      JERRY_DEBUGGER_CHECK_PACKET_SIZE (jerry_debugger_receive_type_t);

      JERRY_CONTEXT (debugger_flags) = (uint8_t) (JERRY_CONTEXT (debugger_flags) & ~JERRY_DEBUGGER_CLIENT_SOURCE_MODE);
      JERRY_CONTEXT (debugger_flags) = (uint8_t) (JERRY_CONTEXT (debugger_flags) | JERRY_DEBUGGER_CLIENT_NO_SOURCE);

      *resume_exec_p = true;

      return true;
    }

    case JERRY_DEBUGGER_CONTEXT_RESET:
    {
      if (!(JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CLIENT_SOURCE_MODE))
      {
        jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Not in client source mode\n");
        jerry_debugger_close_connection ();
        return false;
      }

      JERRY_DEBUGGER_CHECK_PACKET_SIZE (jerry_debugger_receive_type_t);

      JERRY_CONTEXT (debugger_flags) = (uint8_t) (JERRY_CONTEXT (debugger_flags) & ~JERRY_DEBUGGER_CLIENT_SOURCE_MODE);
      JERRY_CONTEXT (debugger_flags) = (uint8_t) (JERRY_CONTEXT (debugger_flags) | JERRY_DEBUGGER_CONTEXT_RESET_MODE);

      *resume_exec_p = true;

      return true;
    }

    default:
    {
      jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Unexpected message.");
      jerry_debugger_close_connection ();
      return false;
    }
  }
} /* jerry_debugger_process_message */

#undef JERRY_DEBUGGER_CHECK_PACKET_SIZE

/**
 * Tell the client that a breakpoint has been hit and wait for further debugger commands.
 */
void
jerry_debugger_breakpoint_hit (uint8_t message_type) /**< message type */
{
  JERRY_ASSERT (JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED);

  JERRY_DEBUGGER_SEND_BUFFER_AS (jerry_debugger_send_breakpoint_hit_t, breakpoint_hit_p);

  JERRY_DEBUGGER_INIT_SEND_MESSAGE (breakpoint_hit_p);
  JERRY_DEBUGGER_SET_SEND_MESSAGE_SIZE_FROM_TYPE (breakpoint_hit_p, jerry_debugger_send_breakpoint_hit_t);
  breakpoint_hit_p->type = message_type;

  vm_frame_ctx_t *frame_ctx_p = JERRY_CONTEXT (vm_top_context_p);

  jmem_cpointer_t byte_code_header_cp;
  JMEM_CP_SET_NON_NULL_POINTER (byte_code_header_cp, frame_ctx_p->bytecode_header_p);
  memcpy (breakpoint_hit_p->byte_code_cp, &byte_code_header_cp, sizeof (jmem_cpointer_t));

  uint32_t offset = (uint32_t) (frame_ctx_p->byte_code_p - (uint8_t *) frame_ctx_p->bytecode_header_p);
  memcpy (breakpoint_hit_p->offset, &offset, sizeof (uint32_t));

  if (!jerry_debugger_send (sizeof (jerry_debugger_send_breakpoint_hit_t)))
  {
    return;
  }

  JERRY_CONTEXT (debugger_flags) = (uint8_t) (JERRY_CONTEXT (debugger_flags) | JERRY_DEBUGGER_BREAKPOINT_MODE);

  jerry_debugger_uint8_data_t *uint8_data = NULL;

  while (!jerry_debugger_receive (&uint8_data))
  {
    jerry_debugger_sleep ();
  }

  if (uint8_data != NULL)
  {
    jmem_heap_free_block (uint8_data,
                          uint8_data->uint8_size + sizeof (jerry_debugger_uint8_data_t));
  }

  JERRY_CONTEXT (debugger_flags) = (uint8_t) (JERRY_CONTEXT (debugger_flags) & ~JERRY_DEBUGGER_BREAKPOINT_MODE);

  JERRY_CONTEXT (debugger_message_delay) = JERRY_DEBUGGER_MESSAGE_FREQUENCY;
} /* jerry_debugger_breakpoint_hit */

/**
 * Send the type signal to the client.
 */
void
jerry_debugger_send_type (jerry_debugger_header_type_t type) /**< message type */
{
  JERRY_ASSERT (JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED);

  JERRY_DEBUGGER_SEND_BUFFER_AS (jerry_debugger_send_type_t, message_type_p);

  JERRY_DEBUGGER_INIT_SEND_MESSAGE (message_type_p);
  JERRY_DEBUGGER_SET_SEND_MESSAGE_SIZE_FROM_TYPE (message_type_p, jerry_debugger_send_type_t);
  message_type_p->type = (uint8_t) type;

  jerry_debugger_send (sizeof (jerry_debugger_send_type_t));
} /* jerry_debugger_send_type */


/**
 * Send the type signal to the client.
 *
 * @return true - if the data sent successfully to the debugger client,
 *         false - otherwise
 */
bool
jerry_debugger_send_configuration (uint8_t max_message_size) /**< maximum message size */
{
  JERRY_DEBUGGER_SEND_BUFFER_AS (jerry_debugger_send_configuration_t, configuration_p);

  /* Helper structure for endianness check. */
  union
  {
    uint16_t uint16_value; /**< a 16-bit value */
    uint8_t uint8_value[2]; /**< lower and upper byte of a 16-bit value */
  } endian_data;

  endian_data.uint16_value = 1;

  JERRY_DEBUGGER_INIT_SEND_MESSAGE (configuration_p);
  JERRY_DEBUGGER_SET_SEND_MESSAGE_SIZE_FROM_TYPE (configuration_p, jerry_debugger_send_configuration_t);
  configuration_p->type = JERRY_DEBUGGER_CONFIGURATION;
  configuration_p->max_message_size = max_message_size;
  configuration_p->cpointer_size = sizeof (jmem_cpointer_t);
  configuration_p->little_endian = (endian_data.uint8_value[0] == 1);

  return jerry_debugger_send (sizeof (jerry_debugger_send_configuration_t));
} /* jerry_debugger_send_configuration */

/**
 * Send raw data to the debugger client.
 */
void
jerry_debugger_send_data (jerry_debugger_header_type_t type, /**< message type */
                          const void *data, /**< raw data */
                          size_t size) /**< size of data */
{
  JERRY_ASSERT (size <= JERRY_DEBUGGER_SEND_MAX (uint8_t));

  JERRY_DEBUGGER_SEND_BUFFER_AS (jerry_debugger_send_type_t, message_type_p);

  JERRY_DEBUGGER_INIT_SEND_MESSAGE (message_type_p);
  JERRY_DEBUGGER_SET_SEND_MESSAGE_SIZE (message_type_p, 1 + size);
  message_type_p->type = type;
  memcpy (message_type_p + 1, data, size);

  jerry_debugger_send (sizeof (jerry_debugger_send_type_t) + size);
} /* jerry_debugger_send_data */

/**
 * Send string to the debugger client.
 *
 * @return true - if the data sent successfully to the debugger client,
 *         false - otherwise
 */
bool
jerry_debugger_send_string (uint8_t message_type, /**< message type */
                            uint8_t sub_type, /**< subtype of the string */
                            const uint8_t *string_p, /**< string data */
                            size_t string_length) /**< length of string */
{
  JERRY_ASSERT (JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED);

  const size_t max_fragment_len = JERRY_DEBUGGER_SEND_MAX (uint8_t);

  JERRY_DEBUGGER_SEND_BUFFER_AS (jerry_debugger_send_string_t, message_string_p);

  JERRY_DEBUGGER_INIT_SEND_MESSAGE (message_string_p);
  JERRY_DEBUGGER_SET_SEND_MESSAGE_SIZE_FROM_TYPE (message_string_p, jerry_debugger_send_string_t);
  message_string_p->type = message_type;

  while (string_length > max_fragment_len)
  {
    memcpy (message_string_p->string, string_p, max_fragment_len);

    if (!jerry_debugger_send (sizeof (jerry_debugger_send_string_t)))
    {
      return false;
    }

    string_length -= max_fragment_len;
    string_p += max_fragment_len;
  }

  if (sub_type != JERRY_DEBUGGER_NO_SUBTYPE)
  {
    string_length += 1;
  }

  JERRY_DEBUGGER_SET_SEND_MESSAGE_SIZE (message_string_p, 1 + string_length);
  message_string_p->type = (uint8_t) (message_type + 1);

  memcpy (message_string_p->string, string_p, string_length);
  if (sub_type != JERRY_DEBUGGER_NO_SUBTYPE)
  {
    message_string_p->string[string_length - 1] = sub_type;
  }

  return jerry_debugger_send (sizeof (jerry_debugger_send_type_t) + string_length);
} /* jerry_debugger_send_string */

/**
 * Send the function compressed pointer to the debugger client.
 *
 * @return true - if the data was sent successfully to the debugger client,
 *         false - otherwise
 */
bool
jerry_debugger_send_function_cp (jerry_debugger_header_type_t type, /**< message type */
                                 ecma_compiled_code_t *compiled_code_p) /**< byte code pointer */
{
  JERRY_ASSERT (JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED);

  JERRY_DEBUGGER_SEND_BUFFER_AS (jerry_debugger_send_byte_code_cp_t, byte_code_cp_p);

  JERRY_DEBUGGER_INIT_SEND_MESSAGE (byte_code_cp_p);
  JERRY_DEBUGGER_SET_SEND_MESSAGE_SIZE_FROM_TYPE (byte_code_cp_p, jerry_debugger_send_byte_code_cp_t);
  byte_code_cp_p->type = (uint8_t) type;

  jmem_cpointer_t compiled_code_cp;
  JMEM_CP_SET_NON_NULL_POINTER (compiled_code_cp, compiled_code_p);
  memcpy (byte_code_cp_p->byte_code_cp, &compiled_code_cp, sizeof (jmem_cpointer_t));

  return jerry_debugger_send (sizeof (jerry_debugger_send_byte_code_cp_t));
} /* jerry_debugger_send_function_cp */

/**
 * Send function data to the debugger client.
 *
 * @return true - if the data sent successfully to the debugger client,
 *         false - otherwise
 */
bool
jerry_debugger_send_parse_function (uint32_t line, /**< line */
                                    uint32_t column) /**< column */
{
  JERRY_ASSERT (JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED);

  JERRY_DEBUGGER_SEND_BUFFER_AS (jerry_debugger_send_parse_function_t, message_parse_function_p);

  JERRY_DEBUGGER_INIT_SEND_MESSAGE (message_parse_function_p);
  JERRY_DEBUGGER_SET_SEND_MESSAGE_SIZE_FROM_TYPE (message_parse_function_p, jerry_debugger_send_parse_function_t);
  message_parse_function_p->type = JERRY_DEBUGGER_PARSE_FUNCTION;
  memcpy (message_parse_function_p->line, &line, sizeof (uint32_t));
  memcpy (message_parse_function_p->column, &column, sizeof (uint32_t));

  return jerry_debugger_send (sizeof (jerry_debugger_send_parse_function_t));
} /* jerry_debugger_send_parse_function */

/**
 * Send memory statistics to the debugger client.
 */
void
jerry_debugger_send_memstats (void)
{
  JERRY_ASSERT (JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED);

  JERRY_DEBUGGER_SEND_BUFFER_AS (jerry_debugger_send_memstats_t, memstats_p);
  JERRY_DEBUGGER_INIT_SEND_MESSAGE (memstats_p);
  JERRY_DEBUGGER_SET_SEND_MESSAGE_SIZE_FROM_TYPE (memstats_p, jerry_debugger_send_memstats_t);

  memstats_p->type = JERRY_DEBUGGER_MEMSTATS_RECEIVE;

#ifdef JMEM_STATS /* if jmem_stats are defined */
  jmem_heap_stats_t *heap_stats = &JERRY_CONTEXT (jmem_heap_stats);

  uint32_t allocated_bytes = (uint32_t) heap_stats->allocated_bytes;
  memcpy (memstats_p->allocated_bytes, &allocated_bytes, sizeof (uint32_t));
  uint32_t byte_code_bytes = (uint32_t) heap_stats->byte_code_bytes;
  memcpy (memstats_p->byte_code_bytes, &byte_code_bytes, sizeof (uint32_t));
  uint32_t string_bytes = (uint32_t) heap_stats->string_bytes;
  memcpy (memstats_p->string_bytes, &string_bytes, sizeof (uint32_t));
  uint32_t object_bytes = (uint32_t) heap_stats->object_bytes;
  memcpy (memstats_p->object_bytes, &object_bytes, sizeof (uint32_t));
  uint32_t property_bytes = (uint32_t) heap_stats->property_bytes;
  memcpy (memstats_p->property_bytes, &property_bytes, sizeof (uint32_t));
#else /* if not, just put zeros */
  memset (memstats_p->allocated_bytes, 0, sizeof (uint32_t));
  memset (memstats_p->byte_code_bytes, 0, sizeof (uint32_t));
  memset (memstats_p->string_bytes, 0, sizeof (uint32_t));
  memset (memstats_p->object_bytes, 0, sizeof (uint32_t));
  memset (memstats_p->property_bytes, 0, sizeof (uint32_t));
#endif

  jerry_debugger_send (sizeof (jerry_debugger_send_memstats_t));
} /* jerry_debugger_send_memstats */

/*
 * Converts an standard error into a string.
 *
 * @return standard error string
 */
static ecma_string_t *
jerry_debugger_exception_object_to_string (ecma_value_t exception_obj_value) /**< exception object */
{
  ecma_object_t *object_p = ecma_get_object_from_value (exception_obj_value);

  ecma_object_t *prototype_p = ecma_get_object_prototype (object_p);

  if (prototype_p == NULL
      || ecma_get_object_type (prototype_p) != ECMA_OBJECT_TYPE_GENERAL
      || !ecma_get_object_is_builtin (prototype_p))
  {
    return NULL;
  }

  lit_magic_string_id_t string_id;

  switch (((ecma_extended_object_t *) prototype_p)->u.built_in.id)
  {
#ifndef CONFIG_DISABLE_ERROR_BUILTINS
    case ECMA_BUILTIN_ID_EVAL_ERROR_PROTOTYPE:
    {
      string_id = LIT_MAGIC_STRING_EVAL_ERROR_UL;
      break;
    }
    case ECMA_BUILTIN_ID_RANGE_ERROR_PROTOTYPE:
    {
      string_id = LIT_MAGIC_STRING_RANGE_ERROR_UL;
      break;
    }
    case ECMA_BUILTIN_ID_REFERENCE_ERROR_PROTOTYPE:
    {
      string_id = LIT_MAGIC_STRING_REFERENCE_ERROR_UL;
      break;
    }
    case ECMA_BUILTIN_ID_SYNTAX_ERROR_PROTOTYPE:
    {
      string_id = LIT_MAGIC_STRING_SYNTAX_ERROR_UL;
      break;
    }
    case ECMA_BUILTIN_ID_TYPE_ERROR_PROTOTYPE:
    {
      string_id = LIT_MAGIC_STRING_TYPE_ERROR_UL;
      break;
    }
    case ECMA_BUILTIN_ID_URI_ERROR_PROTOTYPE:
    {
      string_id = LIT_MAGIC_STRING_URI_ERROR_UL;
      break;
    }
#endif /* !CONFIG_DISABLE_ERROR_BUILTINS */
    case ECMA_BUILTIN_ID_ERROR_PROTOTYPE:
    {
      string_id = LIT_MAGIC_STRING_ERROR_UL;
      break;
    }
    default:
    {
      return NULL;
    }
  }

  lit_utf8_size_t size = lit_get_magic_string_size (string_id);
  JERRY_ASSERT (size <= 14);

  lit_utf8_byte_t data[16];
  memcpy (data, lit_get_magic_string_utf8 (string_id), size);

  ecma_string_t message_string;
  ecma_init_ecma_magic_string (&message_string, LIT_MAGIC_STRING_MESSAGE);

  ecma_property_t *property_p;
  property_p = ecma_find_named_property (ecma_get_object_from_value (exception_obj_value),
                                         &message_string);

  if (property_p == NULL
      || ECMA_PROPERTY_GET_TYPE (*property_p) != ECMA_PROPERTY_TYPE_NAMEDDATA)
  {
    return ecma_new_ecma_string_from_utf8 (data, size);
  }

  ecma_property_value_t *prop_value_p = ECMA_PROPERTY_VALUE_PTR (property_p);

  if (!ecma_is_value_string (prop_value_p->value))
  {
    return ecma_new_ecma_string_from_utf8 (data, size);
  }

  data[size] = LIT_CHAR_COLON;
  data[size + 1] = LIT_CHAR_SP;

  return ecma_concat_ecma_strings (ecma_new_ecma_string_from_utf8 (data, size + 2),
                                   ecma_get_string_from_value (prop_value_p->value));
} /* jerry_debugger_exception_object_to_string */

/**
 * Send string representation of exception to the client.
 *
 * @return true - if the data sent successfully to the debugger client,
 *         false - otherwise
 */
bool
jerry_debugger_send_exception_string (void)
{
  ecma_string_t *string_p = NULL;

  ecma_value_t exception_value = JERRY_CONTEXT (error_value);

  if (ecma_is_value_object (exception_value))
  {
    string_p = jerry_debugger_exception_object_to_string (exception_value);

    if (string_p == NULL)
    {
      string_p = ecma_get_string_from_value (ecma_builtin_helper_object_to_string (exception_value));
    }
  }
  else if (ecma_is_value_string (exception_value))
  {
    string_p = ecma_get_string_from_value (exception_value);
    ecma_ref_ecma_string (string_p);
  }
  else
  {
    exception_value = ecma_op_to_string (exception_value);
    string_p = ecma_get_string_from_value (exception_value);
  }

  ECMA_STRING_TO_UTF8_STRING (string_p, string_data_p, string_size);

  bool result = jerry_debugger_send_string (JERRY_DEBUGGER_EXCEPTION_STR,
                                            JERRY_DEBUGGER_NO_SUBTYPE,
                                            string_data_p,
                                            string_size);

  ECMA_FINALIZE_UTF8_STRING (string_data_p, string_size);

  ecma_deref_ecma_string (string_p);
  return result;
} /* jerry_debugger_send_exception_string */

#endif /* JERRY_DEBUGGER */
