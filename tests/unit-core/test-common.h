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

#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include "jrt.h"

#include <math.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define TEST_ASSERT(x) \
  do \
  { \
    if (unlikely (!(x))) \
    { \
      jerry_port_log (JERRY_LOG_LEVEL_ERROR, \
                      "TEST: Assertion '%s' failed at %s(%s):%lu.\n", \
                      #x, \
                      __FILE__, \
                      __func__, \
                      (unsigned long) __LINE__); \
      jerry_fatal (ERR_FAILED_INTERNAL_ASSERTION); \
    } \
  } while (0)

/**
 * Test initialization statement that should be included
 * at the beginning of main function in every unit test.
 */
#define TEST_INIT() \
do \
{ \
  srand ((unsigned) jerry_port_get_current_time ()); \
} while (0)

#endif /* !TEST_COMMON_H */
