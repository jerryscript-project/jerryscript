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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

using namespace std;

/**
 * Verify that unit tests are built with all debug checks enabled
 */
#if defined (JERRY_NDEBUG) || defined (JERRY_DISABLE_HEAVY_DEBUG)
# error "defined (JERRY_NDEBUG) || defined (JERRY_DISABLE_HEAVY_DEBUG) in a unit test"
#endif /* JERRY_NDEBUG || JERRY_DISABLE_HEAVY_DEBUG */

#define TEST_RANDOMIZE() \
do \
{ \
  struct timeval current_time; \
  gettimeofday (&current_time, NULL); \
  \
  time_t seconds = current_time.tv_sec; \
  suseconds_t microseconds = current_time.tv_usec; \
  \
  uint32_t seed = (uint32_t) (seconds + microseconds); \
  \
  printf ("TEST_RANDOMIZE: seed is %u\n", seed); \
  \
  srand (seed); \
} \
while (0)

#endif /* TEST_COMMON_H */
