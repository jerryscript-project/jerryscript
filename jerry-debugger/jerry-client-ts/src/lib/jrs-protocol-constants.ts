// Copyright JS Foundation and other contributors, http://js.foundation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// TODO: consider generating this file

// expected JerryScript debugger protocol version
export const JERRY_DEBUGGER_VERSION = 2;

// server
export const JERRY_DEBUGGER_CONFIGURATION = 1;
export const JERRY_DEBUGGER_PARSE_ERROR = 2;
export const JERRY_DEBUGGER_BYTE_CODE_CP = 3;
export const JERRY_DEBUGGER_PARSE_FUNCTION = 4;
export const JERRY_DEBUGGER_BREAKPOINT_LIST = 5;
export const JERRY_DEBUGGER_BREAKPOINT_OFFSET_LIST = 6;
export const JERRY_DEBUGGER_SOURCE_CODE = 7;
export const JERRY_DEBUGGER_SOURCE_CODE_END = 8;
export const JERRY_DEBUGGER_SOURCE_CODE_NAME = 9;
export const JERRY_DEBUGGER_SOURCE_CODE_NAME_END = 10;
export const JERRY_DEBUGGER_FUNCTION_NAME = 11;
export const JERRY_DEBUGGER_FUNCTION_NAME_END = 12;
export const JERRY_DEBUGGER_WAITING_AFTER_PARSE = 13;
export const JERRY_DEBUGGER_RELEASE_BYTE_CODE_CP = 14;
export const JERRY_DEBUGGER_MEMSTATS_RECEIVE = 15;
export const JERRY_DEBUGGER_BREAKPOINT_HIT = 16;
export const JERRY_DEBUGGER_EXCEPTION_HIT = 17;
export const JERRY_DEBUGGER_EXCEPTION_STR = 18;
export const JERRY_DEBUGGER_EXCEPTION_STR_END = 19;
export const JERRY_DEBUGGER_BACKTRACE = 20;
export const JERRY_DEBUGGER_BACKTRACE_END = 21;
export const JERRY_DEBUGGER_EVAL_RESULT = 22;
export const JERRY_DEBUGGER_EVAL_RESULT_END = 23;
export const JERRY_DEBUGGER_WAIT_FOR_SOURCE = 24;
export const JERRY_DEBUGGER_OUTPUT_RESULT = 25;
export const JERRY_DEBUGGER_OUTPUT_RESULT_END = 26;

export const JERRY_DEBUGGER_EVAL_OK = 1;
export const JERRY_DEBUGGER_EVAL_ERROR = 2;

export const JERRY_DEBUGGER_OUTPUT_OK = 1;
export const JERRY_DEBUGGER_OUTPUT_ERROR = 2;
export const JERRY_DEBUGGER_OUTPUT_WARNING = 3;
export const JERRY_DEBUGGER_OUTPUT_DEBUG = 4;
export const JERRY_DEBUGGER_OUTPUT_TRACE = 5;

// client
export const JERRY_DEBUGGER_FREE_BYTE_CODE_CP = 1;
export const JERRY_DEBUGGER_UPDATE_BREAKPOINT = 2;
export const JERRY_DEBUGGER_EXCEPTION_CONFIG = 3;
export const JERRY_DEBUGGER_PARSER_CONFIG = 4;
export const JERRY_DEBUGGER_MEMSTATS = 5;
export const JERRY_DEBUGGER_STOP = 6;
export const JERRY_DEBUGGER_PARSER_RESUME = 7;
export const JERRY_DEBUGGER_CLIENT_SOURCE = 8;
export const JERRY_DEBUGGER_CLIENT_SOURCE_PART = 9;
export const JERRY_DEBUGGER_NO_MORE_SOURCES = 10;
export const JERRY_DEBUGGER_CONTEXT_RESET = 11;
export const JERRY_DEBUGGER_CONTINUE = 12;
export const JERRY_DEBUGGER_STEP = 13;
export const JERRY_DEBUGGER_NEXT = 14;
export const JERRY_DEBUGGER_FINISH = 15;
export const JERRY_DEBUGGER_GET_BACKTRACE = 16;
export const JERRY_DEBUGGER_EVAL = 17;
export const JERRY_DEBUGGER_EVAL_PART = 18;
