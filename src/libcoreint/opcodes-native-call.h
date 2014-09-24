/* Copyright 2014 Samsung Electronics Co., Ltd.
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

#ifndef OPCODES_NATIVE_CALL_H
#define OPCODES_NATIVE_CALL_H

/**
 * Identifier of a native call
 */
typedef enum
{
  OPCODE_NATIVE_CALL_LED_TOGGLE,
  OPCODE_NATIVE_CALL_LED_ON,
  OPCODE_NATIVE_CALL_LED_OFF,
  OPCODE_NATIVE_CALL_LED_ONCE,
  OPCODE_NATIVE_CALL_WAIT,
  OPCODE_NATIVE_CALL_PRINT,
  OPCODE_NATIVE_CALL__COUNT
} opcode_native_call_t;

#endif /* !OPCODES_NATIVE_CALL_H */
