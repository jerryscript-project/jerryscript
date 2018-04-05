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

#ifndef DEBUGGER_WS_H
#define DEBUGGER_WS_H

#include "ecma-globals.h"

#ifdef JERRY_DEBUGGER

/* JerryScript debugger protocol is a simplified version of RFC-6455 (WebSockets). */

/**
 * Maximum number of bytes transmitted or received.
 */
#define JERRY_DEBUGGER_MAX_BUFFER_SIZE 128

/**
 * Incoming message: next message of string data.
 */
typedef struct
{
  uint8_t type; /**< type of the message */
} jerry_debugger_receive_uint8_data_part_t;

/**
 * Byte data for evaluating expressions and receiving client source.
 */
typedef struct
{
  uint32_t uint8_size; /**< total size of the client source */
  uint32_t uint8_offset; /**< current offset in the client source */
} jerry_debugger_uint8_data_t;

bool jerry_debugger_accept_connection (void);
void jerry_debugger_close_connection (void);

bool jerry_debugger_send (size_t data_size);
bool jerry_debugger_receive (jerry_debugger_uint8_data_t **message_data_p);

void jerry_debugger_compute_sha1 (const uint8_t *input1, size_t input1_len,
                                  const uint8_t *input2, size_t input2_len,
                                  uint8_t output[20]);

#endif /* JERRY_DEBUGGER */

#endif /* !DEBUGGER_WS_H */
