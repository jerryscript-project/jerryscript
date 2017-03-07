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

#ifndef JERRY_DEBUGGER_H
#define JERRY_DEBUGGER_H

#ifdef JERRY_DEBUGGER

#include "jerry-debugger-ws.h"
#include "ecma-globals.h"

/* JerryScript debugger protocol is a simplified version of RFC-6455 (WebSockets). */

/**
 * Frequency of calling jerry_debugger_receive() by the VM.
 */
#define JERRY_DEBUGGER_MESSAGE_FREQUENCY 5

/**
 * Limited resources available for the engine, so it is important to
 * check the maximum buffer size. It needs to be between 64 and 256 bytes.
 */
#if JERRY_DEBUGGER_MAX_BUFFER_SIZE < 64 || JERRY_DEBUGGER_MAX_BUFFER_SIZE > 256
#error Please define the MAX_BUFFER_SIZE between 64 and 256 bytes.
#endif /* JERRY_DEBUGGER_MAX_BUFFER_SIZE < 64 || JERRY_DEBUGGER_MAX_BUFFER_SIZE > 256 */

/**
 * Calculate the maximum number of items for a given type
 * which can be transmitted by one message.
 */
#define JERRY_DEBUGGER_SEND_MAX(type) \
 ((JERRY_DEBUGGER_MAX_SEND_SIZE - sizeof (jerry_debugger_send_header_t) - 1) / sizeof (type))

/**
 * Debugger operation modes:
 *
 * The debugger has two operation modes: run mode and breakpoint mode.
 *
 * In run mode the debugger server accepts only a limited number of message
 * types from the debugger client (e.g. stop execution, set breakpoint).
 *
 * In breakpoint mode the JavaScript execution is stopped at a breakpoint and
 * more message types are accepted (e.g. get backtrace, evaluate expression).
 *
 * Switching between modes:
 *
 * When the JavaScript execution stops at a breakpoint the server sends a
 * JERRY_DEBUGGER_BREAKPOINT_HIT message to the client. The client can only
 * issue breakpoint mode commands after this message is received.
 *
 * Certain breakpoint mode commands (e.g. continue) resumes the JavaScript
 * execution and the client must not send any breakpoint mode messages
 * until the JERRY_DEBUGGER_BREAKPOINT_HIT is received again.
 *
 * The debugger server starts in run mode but stops at the first available
 * breakpoint.
 */

/**
 * Debugger option flags.
 */
typedef enum
{
  JERRY_DEBUGGER_CONNECTED = 1u << 0, /**< debugger is connected */
  JERRY_DEBUGGER_BREAKPOINT_MODE = 1u << 1, /**< debugger waiting at a breakpoint */
  JERRY_DEBUGGER_VM_STOP = 1u << 2, /**< stop at the next breakpoint regardless it is enabled */
  JERRY_DEBUGGER_VM_IGNORE = 1u << 3, /**< ignore all breakpoints */
} jerry_debugger_flags_t;

/**
 * Types for the package.
 */
typedef enum
{
  /* Messages sent by the server to client. */
  JERRY_DEBUGGER_CONFIGURATION = 1, /**< debugger configuration */
  JERRY_DEBUGGER_PARSE_ERROR = 2, /**< parse error */
  JERRY_DEBUGGER_BYTE_CODE_CP = 3, /**< byte code compressed pointer */
  JERRY_DEBUGGER_PARSE_FUNCTION = 4, /**< parsing a new function */
  JERRY_DEBUGGER_BREAKPOINT_LIST = 5, /**< list of line offsets */
  JERRY_DEBUGGER_BREAKPOINT_OFFSET_LIST = 6, /**< list of byte code offsets */
  JERRY_DEBUGGER_SOURCE_CODE = 7, /**< source code fragment */
  JERRY_DEBUGGER_SOURCE_CODE_END = 8, /**< source code last fragment */
  JERRY_DEBUGGER_SOURCE_CODE_NAME = 9, /**< source code name fragment */
  JERRY_DEBUGGER_SOURCE_CODE_NAME_END = 10, /**< source code name last fragment */
  JERRY_DEBUGGER_FUNCTION_NAME = 11, /**< function name fragment */
  JERRY_DEBUGGER_FUNCTION_NAME_END = 12, /**< function name last fragment */
  JERRY_DEBUGGER_RELEASE_BYTE_CODE_CP = 13, /**< invalidate byte code compressed pointer */
  JERRY_DEBUGGER_BREAKPOINT_HIT = 14, /**< notify breakpoint hit */
  JERRY_DEBUGGER_BACKTRACE = 15, /**< backtrace data */
  JERRY_DEBUGGER_BACKTRACE_END = 16, /**< last backtrace data */
  JERRY_DEBUGGER_EVAL_RESULT = 17, /**< eval result */
  JERRY_DEBUGGER_EVAL_RESULT_END = 18, /**< last part of eval result */
  JERRY_DEBUGGER_EVAL_ERROR = 19, /**< eval result when an error is occured */
  JERRY_DEBUGGER_EVAL_ERROR_END = 20, /**< last part of eval result when an error is occured */

  /* Messages sent by the client to server. */

  /* The following messages are accepted in both run and breakpoint modes. */
  JERRY_DEBUGGER_FREE_BYTE_CODE_CP = 1, /**< free byte code compressed pointer */
  JERRY_DEBUGGER_UPDATE_BREAKPOINT = 2, /**< update breakpoint status */
  JERRY_DEBUGGER_STOP = 3, /**< stop execution */
  /* The following messages are only available in breakpoint
   * mode and they switch the engine to run mode. */
  JERRY_DEBUGGER_CONTINUE = 4, /**< continue execution */
  JERRY_DEBUGGER_STEP = 5, /**< next breakpoint, step into functions */
  JERRY_DEBUGGER_NEXT = 6, /**< next breakpoint in the same context */
  /* The following messages are only available in breakpoint
   * mode and this mode is kept after the message is processed. */
  JERRY_DEBUGGER_GET_BACKTRACE = 7, /**< get backtrace */
  JERRY_DEBUGGER_EVAL = 8, /**< first message of evaluating a string */
  JERRY_DEBUGGER_EVAL_PART = 9, /**< next message of evaluating a string */
} jerry_debugger_header_type_t;

/**
 * Delayed free of byte code data.
 */
typedef struct
{
  uint16_t size; /**< size of the byte code header divided by JMEM_ALIGNMENT */
  jmem_cpointer_t prev_cp; /**< previous byte code data to be freed */
} jerry_debugger_byte_code_free_t;

/**
 * Outgoing message: JerryScript configuration.
 */
typedef struct
{
  jerry_debugger_send_header_t header; /**< message header */
  uint8_t type; /**< type of the message */
  uint8_t max_message_size; /**< maximum incoming message size */
  uint8_t cpointer_size; /**< size of compressed pointers */
  uint8_t little_endian; /**< little endian machine */
} jerry_debugger_send_configuration_t;

/**
 * Outgoing message: message without arguments.
 */
typedef struct
{
  jerry_debugger_send_header_t header; /**< message header */
  uint8_t type; /**< type of the message */
} jerry_debugger_send_type_t;

/**
 * Incoming message: message without arguments.
 */
typedef struct
{
  uint8_t type; /**< type of the message */
} jerry_debugger_receive_type_t;

/**
 * Outgoing message: string (Source file name or function name).
 */
typedef struct
{
  jerry_debugger_send_header_t header; /**< message header */
  uint8_t type; /**< type of the message */
  uint8_t string[JERRY_DEBUGGER_SEND_MAX (uint8_t)]; /**< string data */
} jerry_debugger_send_string_t;

/**
 * Outgoing message: byte code compressed pointer.
 */
typedef struct
{
  jerry_debugger_send_header_t header; /**< message header */
  uint8_t type; /**< type of the message */
  uint8_t byte_code_cp[sizeof (jmem_cpointer_t)]; /**< byte code compressed pointer */
} jerry_debugger_send_byte_code_cp_t;

/**
 * Incoming message: byte code compressed pointer.
 */
typedef struct
{
  uint8_t type; /**< type of the message */
  uint8_t byte_code_cp[sizeof (jmem_cpointer_t)]; /**< byte code compressed pointer */
} jerry_debugger_receive_byte_code_cp_t;

/**
 * Incoming message: update (enable/disable) breakpoint status.
 */
typedef struct
{
  uint8_t type; /**< type of the message */
  uint8_t is_set_breakpoint; /**< set or clear breakpoint */
  uint8_t byte_code_cp[sizeof (jmem_cpointer_t)]; /**< byte code compressed pointer */
  uint8_t offset[sizeof (uint32_t)]; /**< breakpoint offset */
} jerry_debugger_receive_update_breakpoint_t;

/**
 * Outgoing message: notify breakpoint hit.
 */
typedef struct
{
  jerry_debugger_send_header_t header; /**< message header */
  uint8_t type; /**< type of the message */
  uint8_t byte_code_cp[sizeof (jmem_cpointer_t)]; /**< byte code compressed pointer */
  uint8_t offset[sizeof (uint32_t)]; /**< breakpoint offset */
} jerry_debugger_send_breakpoint_hit_t;

/**
 * Stack frame descriptor for sending backtrace information.
 */
typedef struct
{
  uint8_t byte_code_cp[sizeof (jmem_cpointer_t)]; /**< byte code compressed pointer */
  uint8_t offset[sizeof (uint32_t)]; /**< last breakpoint offset */
} jerry_debugger_frame_t;

/**
 * Outgoing message: backtrace information.
 */
typedef struct
{
  jerry_debugger_send_header_t header; /**< message header */
  uint8_t type; /**< type of the message */
  jerry_debugger_frame_t frames[JERRY_DEBUGGER_SEND_MAX (jerry_debugger_frame_t)]; /**< frames */
} jerry_debugger_send_backtrace_t;

/**
 * Incoming message: get backtrace.
 */
typedef struct
{
  uint8_t type; /**< type of the message */
  uint8_t max_depth[sizeof (uint32_t)]; /**< maximum depth (0 - unlimited) */
} jerry_debugger_receive_get_backtrace_t;

/**
 * Incoming message: first message of evaluating expression.
 */
typedef struct
{
  uint8_t type; /**< type of the message */
  uint8_t eval_size[sizeof (uint32_t)]; /**< total size of the message */
} jerry_debugger_receive_eval_first_t;

/**
 * Incoming message: next message of evaluating expression.
 */
typedef struct
{
  uint8_t type; /**< type of the message */
} jerry_debugger_receive_eval_part_t;

/**
 * Data for evaluating expressions
 */
typedef struct
{
  uint32_t eval_size; /**< total size of the eval string */
  uint32_t eval_offset; /**< current offset in the eval string */
} jerry_debugger_eval_data_t;

void jerry_debugger_free_unreferenced_byte_code (void);

bool jerry_debugger_process_message (uint8_t *recv_buffer_p, uint32_t message_size,
                                     bool *resume_exec_p, uint8_t *expected_message_p, void **message_data_p);
void jerry_debugger_breakpoint_hit (void);

void jerry_debugger_send_type (jerry_debugger_header_type_t type);
bool jerry_debugger_send_configuration (uint8_t max_message_size);
void jerry_debugger_send_data (jerry_debugger_header_type_t type, const void *data, size_t size);
bool jerry_debugger_send_string (uint8_t message_type, const uint8_t *string_p, size_t string_length);
void jerry_debugger_send_function_name (const uint8_t *function_name_p, size_t function_name_length);
bool jerry_debugger_send_function_cp (jerry_debugger_header_type_t type, ecma_compiled_code_t *compiled_code_p);

#endif /* JERRY_DEBUGGER */

#endif /* JERRY_DEBUGGER_H */
