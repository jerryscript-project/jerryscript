/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

#include "fdlibm-math.h"
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

/**
 * Verify that unit tests are built with all debug checks enabled
 */
#if defined (JERRY_NDEBUG) || defined (JERRY_DISABLE_HEAVY_DEBUG)
# error "defined (JERRY_NDEBUG) || defined (JERRY_DISABLE_HEAVY_DEBUG) in a unit test"
#endif /* JERRY_NDEBUG || JERRY_DISABLE_HEAVY_DEBUG */

/**
 * Test initialization statement that should be included
 * at the beginning of main function in every unit test.
 */
#define TEST_INIT() \
do \
{ \
  FILE *f_rnd = fopen ("/dev/urandom", "r"); \
 \
  if (f_rnd == NULL) \
  { \
    return 1; \
  } \
 \
  uint32_t seed; \
 \
  size_t bytes_read = fread (&seed, 1, sizeof (seed), f_rnd); \
 \
 fclose (f_rnd); \
 \
  if (bytes_read != sizeof (seed)) \
  { \
    return 1; \
  } \
 \
  srand (seed); \
} while (0)

#endif /* TEST_COMMON_H */
