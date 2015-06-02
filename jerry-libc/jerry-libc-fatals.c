/* Copyright 2015 Samsung Electronics Co., Ltd.
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

/**
 * Jerry libc's fatal errors handlers
 */

#include <stdio.h>
#include <stdlib.h>

#include "jerry-libc-defs.h"

/**
 * Fatal error handler
 *
 * Arguments may be NULL. If so, they are ignored.
 */
void __attr_noreturn___
libc_fatal (const char *msg, /**< fatal error description */
            const char *file_name, /**< file name */
            const char *function_name, /**< function name */
            const unsigned int line_number) /**< line number */
{
  if (msg != NULL
      && file_name != NULL
      && function_name != NULL)
  {
    printf ("Assertion '%s' failed at %s (%s:%u).\n",
            msg, function_name, file_name, line_number);
  }

  abort ();
} /* libc_fatal */
