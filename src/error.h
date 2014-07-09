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

#ifndef ERROR_H
#define ERROR_H 

#include "mappings.h"

extern void lexer_dump_buffer_state ();

static inline void
fatal (int code)
{
  printf ("FATAL: %d\n", code);
  lexer_dump_buffer_state ();
  JERRY_UNREACHABLE ();
  exit (code);
}

#define ERR_IO (-1)
#define ERR_BUFFER_SIZE (-2)
#define ERR_SEVERAL_FILES (-3)
#define ERR_NO_FILES (-4)
#define ERR_NON_CHAR (-5)
#define ERR_UNCLOSED (-6)
#define ERR_INT_LITERAL (-7)
#define ERR_STRING (-8)
#define ERR_PARSER (-9)

#endif /* ERROR_H */